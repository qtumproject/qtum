#include <wallet/stake.h>
#include <wallet/receive.h>
#include <node/miner.h>
#include <qtum/qtumledger.h>
#include <pos.h>
#include <key_io.h>
#include <common/args.h>

namespace wallet {

void StakeQtums(CWallet& wallet, bool fStake)
{
    node::StakeQtums(fStake, &wallet);
}

void StartStake(CWallet& wallet)
{
    if(wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS))
    {
        wallet.m_enabled_staking = node::ENABLE_HARDWARE_STAKE && !wallet.m_ledger_id.empty() && QtumLedger::instance().toolExists();
    }
    else
    {
        wallet.m_enabled_staking = true;
    }

    wallet.m_is_staking_thread_stopped = false;
    StakeQtums(wallet, true);
}

void StopStake(CWallet& wallet)
{
    if(wallet.m_is_staking_thread_stopped)
        return;

    if(!wallet.stakeThread)
    {
        if(wallet.m_enabled_staking)
            wallet.m_enabled_staking = false;
    }
    else
    {
        wallet.m_stop_staking_thread = true;
        wallet.m_enabled_staking = false;
        StakeQtums(wallet, false);
        wallet.stakeThread = 0;
        wallet.m_stop_staking_thread = false;
    }

    wallet.m_is_staking_thread_stopped = true;
}

bool CreateCoinStakeFromMine(CWallet& wallet, unsigned int nBits, const CAmount& nTotalFees, uint32_t nTimeBlock, CMutableTransaction& tx, PKHash& pkhash, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoins, std::vector<COutPoint>& setSelectedCoins, bool selectedOnly, bool sign, COutPoint& headerPrevout)
{
    bool fAllowWatchOnly = wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    CBlockIndex* pindexPrev = wallet.chain().getTip();
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    struct CMutableTransaction txNew(tx);
    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    // Choose coins to use
    const auto bal = GetBalance(wallet);
    CAmount nBalance = bal.m_mine_trusted;
    if(fAllowWatchOnly)
        nBalance += bal.m_watchonly_trusted;

    if (nBalance <= wallet.m_reserve_balance)
        return false;

    std::vector<std::pair<const CWalletTx*,unsigned int>> vwtxPrev;

    if (setCoins.empty())
        return false;

    if(wallet.stakeCache.size() > setCoins.size() + 100){
        //Determining if the cache is still valid is harder than just clearing it when it gets too big, so instead just clear it
        //when it has more than 100 entries more than the actual setCoins.
        wallet.stakeCache.clear();
    }
    if(!wallet.fHasMinerStakeCache && gArgs.GetBoolArg("-stakecache", node::DEFAULT_STAKE_CACHE)) {

        for(const std::pair<const CWalletTx*,unsigned int> &pcoin : setCoins)
        {
            boost::this_thread::interruption_point();
            COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);
            CacheKernel(wallet.stakeCache, prevoutStake, pindexPrev, wallet.chain().getCoinsTip()); //this will do a 2 disk loads per op
        }
    }
    std::map<COutPoint, CStakeCache>& cache = wallet.fHasMinerStakeCache ? wallet.minerStakeCache : wallet.stakeCache;
    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    CScript aggregateScriptPubKeyHashKernel;

    // Populate the list with the selected coins
    std::set<std::pair<const CWalletTx*,unsigned int> > setSelected;
    if(selectedOnly)
    {
        for(const COutPoint& prevoutStake : setSelectedCoins)
        {
            auto it = wallet.mapWallet.find(prevoutStake.hash);
            if (it != wallet.mapWallet.end()) {
                setSelected.insert(std::make_pair(&it->second, prevoutStake.n));
            }
        }
    }

    std::set<std::pair<const CWalletTx*,unsigned int> >& setPrevouts = selectedOnly ? setSelected : setCoins;
    for(const std::pair<const CWalletTx*,unsigned int> &pcoin : setPrevouts)
    {
        bool fKernelFound = false;
        boost::this_thread::interruption_point();
        // Search backward in time from the given txNew timestamp
        // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
        COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);
        if (CheckKernel(pindexPrev, nBits, nTimeBlock, prevoutStake, wallet.chain().getCoinsTip(), cache, wallet.chain().chainman().ActiveChainstate()))
        {
            // Found a kernel
            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : kernel found\n");
            std::vector<valtype> vSolutions;
            CScript scriptPubKeyOut;
            scriptPubKeyKernel = pcoin.first->tx->vout[pcoin.second].scriptPubKey;
            TxoutType whichType = Solver(scriptPubKeyKernel, vSolutions);
            if (whichType == TxoutType::NONSTANDARD)
            {
                LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to parse kernel\n");
                break;
            }
            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : parsed kernel type=%d\n", (int)whichType);
            if (whichType != TxoutType::PUBKEY && whichType != TxoutType::PUBKEYHASH)
            {
                LogPrint(BCLog::COINSTAKE, "CreateCoinStake : no support for kernel type=%d\n", (int)whichType);
                break;  // only support pay to public key and pay to address
            }
            if (whichType == TxoutType::PUBKEYHASH) // pay to address type
            {
                // convert to pay to public key type
                uint160 hash160(vSolutions[0]);
                pkhash = PKHash(hash160);
                CPubKey pubKeyStake;
                if (!wallet.HasPrivateKey(pkhash, fAllowWatchOnly) || !wallet.GetPubKey(pkhash, pubKeyStake))
                {
                    LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to get key for kernel type=%d\n", (int)whichType);
                    break;  // unable to find corresponding public key
                }
                scriptPubKeyOut << pubKeyStake.getvch() << OP_CHECKSIG;
                aggregateScriptPubKeyHashKernel = scriptPubKeyKernel;
            }
            if (whichType == TxoutType::PUBKEY)
            {
                valtype& vchPubKey = vSolutions[0];
                CPubKey pubKey(vchPubKey);
                uint160 hash160(Hash160(vchPubKey));
                pkhash = PKHash(hash160);
                CPubKey pubKeyStake;
                if (!wallet.HasPrivateKey(pkhash, fAllowWatchOnly) || !wallet.GetPubKey(pkhash, pubKeyStake))
                {
                    LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to get key for kernel type=%d\n", (int)whichType);
                    break;  // unable to find corresponding public key
                }

                if (pubKeyStake != pubKey)
                {
                    LogPrint(BCLog::COINSTAKE, "CreateCoinStake : invalid key for kernel type=%d\n", (int)whichType);
                    break; // keys mismatch
                }

                scriptPubKeyOut = scriptPubKeyKernel;
                aggregateScriptPubKeyHashKernel = CScript() << OP_DUP << OP_HASH160 << ToByteVector(hash160) << OP_EQUALVERIFY << OP_CHECKSIG;
            }

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->tx->vout[pcoin.second].nValue;
            vwtxPrev.push_back(pcoin);
            txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));

            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : added kernel type=%d\n", (int)whichType);
            fKernelFound = true;
        }

        if (fKernelFound)
        {
            headerPrevout = prevoutStake;
            break; // if kernel is found stop searching
        }
    }

    if (nCredit == 0 || nCredit > nBalance - wallet.m_reserve_balance)
        return false;

    for(const std::pair<const CWalletTx*,unsigned int> &pcoin : setCoins)
    {
        // Attempt to add more inputs
        // Only add coins of the same key/address as kernel
        if (txNew.vout.size() == 2 && ((pcoin.first->tx->vout[pcoin.second].scriptPubKey == scriptPubKeyKernel || pcoin.first->tx->vout[pcoin.second].scriptPubKey == aggregateScriptPubKeyHashKernel))
                && pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            // Stop adding more inputs if already too many inputs
            if (txNew.vin.size() >= GetStakeMaxCombineInputs())
                break;
            // Stop adding inputs if reached reserve limit
            if (nCredit + pcoin.first->tx->vout[pcoin.second].nValue > nBalance - wallet.m_reserve_balance)
                break;
            // Do not add additional significant input
            if (pcoin.first->tx->vout[pcoin.second].nValue >= GetStakeCombineThreshold())
                continue;

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->tx->vout[pcoin.second].nValue;
            vwtxPrev.push_back(pcoin);
        }
    }

    const Consensus::Params& consensusParams = Params().GetConsensus();
    int64_t nRewardPiece = 0;
    // Calculate reward
    {
        int64_t nReward = nTotalFees + GetBlockSubsidy(pindexPrev->nHeight + 1, consensusParams);
        if (nReward < 0)
            return false;

        if(pindexPrev->nHeight < consensusParams.nFirstMPoSBlock || pindexPrev->nHeight >= consensusParams.nLastMPoSBlock)
        {
            // Keep whole reward
            nCredit += nReward;
        }
        else
        {
            // Split the reward when mpos is used
            nRewardPiece = nReward / consensusParams.nMPoSRewardRecipients;
            nCredit += nRewardPiece + nReward % consensusParams.nMPoSRewardRecipients;
        }
   }

    if (nCredit >= GetStakeSplitThreshold())
    {
        for(unsigned int i = 0; i < GetStakeSplitOutputs() - 1; i++)
            txNew.vout.push_back(CTxOut(0, txNew.vout[1].scriptPubKey)); //split stake
    }

    // Set output amount
    if (txNew.vout.size() == GetStakeSplitOutputs() + 1)
    {
        CAmount nValue = (nCredit / GetStakeSplitOutputs() / CENT) * CENT;
        for(unsigned int i = 1; i < GetStakeSplitOutputs(); i++)
            txNew.vout[i].nValue = nValue;
        txNew.vout[GetStakeSplitOutputs()].nValue = nCredit - nValue * (GetStakeSplitOutputs() - 1);
    }
    else
        txNew.vout[1].nValue = nCredit;

    if(pindexPrev->nHeight >= consensusParams.nFirstMPoSBlock && pindexPrev->nHeight < consensusParams.nLastMPoSBlock)
    {
        if(!CreateMPoSOutputs(txNew, nRewardPiece, pindexPrev->nHeight, consensusParams, wallet.chain().chainman().ActiveChain(),  wallet.chain().chainman().m_blockman)) {
            LogError("CreateCoinStake : failed to create MPoS reward outputs");
            return false;
        }
    }

    // Append the Refunds To Sender to the transaction outputs
    for(unsigned int i = 2; i < tx.vout.size(); i++)
    {
        txNew.vout.push_back(tx.vout[i]);
    }

    // Sign the input coins
    if(sign && !wallet.SignTransactionStake(txNew, vwtxPrev)) {
        LogError("CreateCoinStake : failed to sign coinstake");
        return false;
    }

    // Successfully generated coinstake
    tx = txNew;
    return true;
}

bool CreateCoinStakeFromDelegate(CWallet& wallet, unsigned int nBits, const CAmount& nTotalFees, uint32_t nTimeBlock, CMutableTransaction& tx, PKHash& pkhash, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoins, std::vector<COutPoint>& setDelegateCoins, bool sign, std::vector<unsigned char>& vchPoD, COutPoint& headerPrevout)
{
    bool fAllowWatchOnly = wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    CBlockIndex* pindexPrev = wallet.chain().getTip();
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    struct CMutableTransaction txNew(tx);
    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    std::vector<std::pair<const CWalletTx*,unsigned int>> vwtxPrev;
    if (setDelegateCoins.empty())
        return false;

    if(wallet.stakeDelegateCache.size() > setDelegateCoins.size() + 100){
        //Determining if the cache is still valid is harder than just clearing it when it gets too big, so instead just clear it
        //when it has more than 100 entries more than the actual setCoins.
        wallet.stakeDelegateCache.clear();
    }
    if(!wallet.fHasMinerStakeCache && gArgs.GetBoolArg("-stakecache", node::DEFAULT_STAKE_CACHE)) {

        for(const COutPoint &prevoutStake : setDelegateCoins)
        {
            boost::this_thread::interruption_point();
            CacheKernel(wallet.stakeDelegateCache, prevoutStake, pindexPrev, wallet.chain().getCoinsTip()); //this will do a 2 disk loads per op
        }
    }
    std::map<COutPoint, CStakeCache>& cache = wallet.fHasMinerStakeCache ? wallet.minerStakeCache : wallet.stakeDelegateCache;
    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    CScript scriptPubKeyStaker;
    Delegation delegation;
    bool delegateOutputExist = false;

    for(const COutPoint &prevoutStake : setDelegateCoins)
    {
        bool fKernelFound = false;
        boost::this_thread::interruption_point();
        // Search backward in time from the given txNew timestamp
        // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
        if (CheckKernel(pindexPrev, nBits, nTimeBlock, prevoutStake, wallet.chain().getCoinsTip(), cache, wallet.chain().chainman().ActiveChainstate()))
        {
            // Found a kernel
            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : kernel found\n");
            std::vector<valtype> vSolutions;

            Coin coinPrev;
            if(!wallet.chain().getUnspentOutput(prevoutStake, coinPrev)){
                if(!GetSpentCoinFromMainChain(pindexPrev, prevoutStake, &coinPrev, wallet.chain().chainman().ActiveChainstate())) {
                    LogError("CreateCoinStake: Could not find coin and it was not at the tip");
                    return false;
                }
            }

            scriptPubKeyKernel = coinPrev.out.scriptPubKey;
            TxoutType whichType = Solver(scriptPubKeyKernel, vSolutions);
            if (whichType == TxoutType::NONSTANDARD)
            {
                LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to parse kernel\n");
                break;
            }
            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : parsed kernel type=%d\n", (int)whichType);
            if (whichType != TxoutType::PUBKEY && whichType != TxoutType::PUBKEYHASH)
            {
                LogPrint(BCLog::COINSTAKE, "CreateCoinStake : no support for kernel type=%d\n", (int)whichType);
                break;  // only support pay to public key and pay to address
            }
            if (whichType == TxoutType::PUBKEYHASH) // pay to address type
            {
                // convert to pay to public key type
                uint160 hash160(vSolutions[0]);

                if(!wallet.GetDelegationStaker(hash160, delegation)) {
                    LogError("CreateCoinStake: Failed to find delegation");
                    return false;
                }

                pkhash = PKHash(delegation.staker);
                CPubKey pubKeyStake;
                if (!wallet.HasPrivateKey(pkhash, fAllowWatchOnly) || !wallet.GetPubKey(pkhash, pubKeyStake))
                {
                    LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to get staker key for kernel type=%d\n", (int)whichType);
                    break;  // unable to find corresponding public key
                }
                scriptPubKeyStaker << pubKeyStake.getvch() << OP_CHECKSIG;
            }
            if (whichType == TxoutType::PUBKEY)
            {
                valtype& vchPubKey = vSolutions[0];
                uint160 hash160(Hash160(vchPubKey));;

                if(!wallet.GetDelegationStaker(hash160, delegation)) {
                    LogError("CreateCoinStake: Failed to find delegation");
                    return false;
                }

                pkhash = PKHash(delegation.staker);
                CPubKey pubKeyStake;
                if (!wallet.HasPrivateKey(pkhash, fAllowWatchOnly) || !wallet.GetPubKey(pkhash, pubKeyStake))
                {
                    LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to get staker key for kernel type=%d\n", (int)whichType);
                    break;  // unable to find corresponding public key
                }

                scriptPubKeyStaker << pubKeyStake.getvch() << OP_CHECKSIG;
            }

            delegateOutputExist = IsDelegateOutputExist(delegation.fee);
            PKHash superStakerAddress(delegation.staker);
            COutPoint prevoutSuperStaker;
            CAmount nValueSuperStaker = 0;
            const CWalletTx* pcoinSuperStaker = wallet.GetCoinSuperStaker(setCoins, superStakerAddress, prevoutSuperStaker, nValueSuperStaker);
            if(!pcoinSuperStaker)
            {
                LogPrint(BCLog::COINSTAKE, "CreateCoinStake : failed to get utxo for super staker %s\n", EncodeDestination(superStakerAddress));
                break;  // unable to find utxo from the super staker
            }

            txNew.vin.push_back(CTxIn(prevoutSuperStaker));
            nCredit += nValueSuperStaker;
            vwtxPrev.push_back(std::make_pair(pcoinSuperStaker, prevoutSuperStaker.n));
            txNew.vout.push_back(CTxOut(0, scriptPubKeyStaker));
            if(delegateOutputExist)
            {
                txNew.vout.push_back(CTxOut(0, scriptPubKeyKernel));
            }

            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : added kernel type=%d\n", (int)whichType);
            fKernelFound = true;
        }

        if (fKernelFound)
        {
            headerPrevout = prevoutStake;
            break; // if kernel is found stop searching
        }
    }

    if (nCredit == 0)
        return false;

    const Consensus::Params& consensusParams = Params().GetConsensus();
    int64_t nRewardPiece = 0;
    int64_t nRewardOffline = 0;
    // Calculate reward
    {
        int64_t nTotalReward = nTotalFees + GetBlockSubsidy(pindexPrev->nHeight + 1, consensusParams);
        if (nTotalReward < 0)
            return false;

        if(pindexPrev->nHeight < consensusParams.nFirstMPoSBlock || pindexPrev->nHeight >= consensusParams.nLastMPoSBlock)
        {
            // Keep whole reward
            int64_t nRewardStaker = 0;
            if(!SplitOfflineStakeReward(nTotalReward, delegation.fee, nRewardOffline, nRewardStaker)) {
                LogError("CreateCoinStake: Failed to split reward");
                return false;
            }
            nCredit += nRewardStaker;
        }
        else
        {
            // Split the reward when mpos is used
            nRewardPiece = nTotalReward / consensusParams.nMPoSRewardRecipients;
            int64_t nRewardStaker = 0;
            int64_t nReward = nRewardPiece + nTotalReward % consensusParams.nMPoSRewardRecipients;
            if(!SplitOfflineStakeReward(nReward, delegation.fee, nRewardOffline, nRewardStaker)) {
                LogError("CreateCoinStake: Failed to split reward");
                return false;
            }
            nCredit += nRewardStaker;
        }
    }

    // Set output amount
    txNew.vout[1].nValue = nCredit;
    if(delegateOutputExist)
    {
        txNew.vout[2].nValue = nRewardOffline;
    }

    if(pindexPrev->nHeight >= consensusParams.nFirstMPoSBlock && pindexPrev->nHeight < consensusParams.nLastMPoSBlock)
    {
        if(!CreateMPoSOutputs(txNew, nRewardPiece, pindexPrev->nHeight, consensusParams, wallet.chain().chainman().ActiveChain(),  wallet.chain().chainman().m_blockman)) {
            LogError("CreateCoinStake : failed to create MPoS reward outputs");
            return false;
        }
    }

    // Append the Refunds To Sender to the transaction outputs
    for(unsigned int i = 2; i < tx.vout.size(); i++)
    {
        txNew.vout.push_back(tx.vout[i]);
    }

    // Sign the input coins
    if(sign && !wallet.SignTransactionStake(txNew, vwtxPrev)) {
        LogError("CreateCoinStake : failed to sign coinstake");
        return false;
    }

    // Successfully generated coinstake
    tx = txNew;

    // Save Proof Of Delegation
    vchPoD = delegation.PoD;

    return true;
}

bool CreateCoinStake(CWallet& wallet, unsigned int nBits, const CAmount& nTotalFees, uint32_t nTimeBlock, CMutableTransaction& tx, PKHash& pkhash, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoins, std::vector<COutPoint>& setSelectedCoins, std::vector<COutPoint>& setDelegateCoins, bool selectedOnly, bool sign, std::vector<unsigned char>& vchPoD, COutPoint& headerPrevout)
{
    // Can super stake
    bool canSuperStake = wallet.CanSuperStake(setCoins, setDelegateCoins);

    // Create coinstake from coins that are delegated to me
    if(canSuperStake && CreateCoinStakeFromDelegate(wallet, nBits, nTotalFees, nTimeBlock, tx, pkhash, setCoins, setDelegateCoins, sign, vchPoD, headerPrevout))
        return true;

    // Create coinstake from coins that are mine
    if(setCoins.size() > 0 && CreateCoinStakeFromMine(wallet, nBits, nTotalFees, nTimeBlock, tx, pkhash, setCoins, setSelectedCoins, selectedOnly, sign, headerPrevout))
        return true;

    // Fail to create coinstake
    return false;
}

void AvailableCoinsForStaking(const CWallet& wallet, const std::vector<uint256>& maturedTx, size_t from, size_t to, const std::map<COutPoint, uint32_t>& immatureStakes, std::vector<std::pair<const CWalletTx *, unsigned int> >& vCoins, std::map<COutPoint, CScriptCache>* insertScriptCache, bool isDescriptorWallet, std::map<uint160, bool>* insertAddressStake)
{
    for(size_t i = from; i < to; i++)
    {
        auto it = wallet.mapWallet.find(maturedTx[i]);
        if(it == wallet.mapWallet.end()) continue;
        const uint256& wtxid = it->first;
        const CWalletTx* pcoin = &(*it).second;
        for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
            COutPoint prevout = COutPoint(Txid::FromUint256(wtxid), i);
            isminetype mine = wallet.IsMine(pcoin->tx->vout[i]);
            if (!(wallet.IsSpent(prevout)) && mine != ISMINE_NO &&
                !wallet.IsLockedCoin(prevout) && (pcoin->tx->vout[i].nValue > 0) &&
                // Check if the staking coin is dust
                pcoin->tx->vout[i].nValue >= wallet.m_staker_min_utxo_size)
            {
                // Get the script data for the coin
                const CScriptCache& scriptCache = wallet.GetScriptCache(prevout, pcoin->tx->vout[i].scriptPubKey, insertScriptCache);

                // Check that the script is not a contract script
                if(scriptCache.contract || !scriptCache.keyIdOk)
                    continue;

                // Check that the address is not delegated to other staker
                if(wallet.m_my_delegations.find(scriptCache.keyId) != wallet.m_my_delegations.end())
                    continue;

                // Check that both pkh and pk descriptors are present
                if(isDescriptorWallet && !wallet.HasAddressStakeScripts(scriptCache.keyId, insertAddressStake))
                    continue;

                // Check prevout maturity
                if(immatureStakes.find(prevout) == immatureStakes.end())
                {
                    // Check if script is spendable
                    bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && scriptCache.solvable);
                    if(spendable)
                        vCoins.push_back(std::make_pair(pcoin, i));
                }
            }
        }
    }
}

bool AvailableDelegateCoinsForStaking(const CWallet& wallet, const std::vector<uint160>& delegations, size_t from, size_t to, int32_t height, const std::map<COutPoint, uint32_t>& immatureStakes,  const std::map<uint256, CSuperStakerInfo>& mapStakers, std::vector<std::pair<COutPoint,CAmount>>& vUnsortedDelegateCoins, std::map<uint160, CAmount> &mDelegateWeight)
{
    for(size_t i = from; i < to; i++)
    {
        std::map<uint160, Delegation>::const_iterator it = wallet.m_delegations_staker.find(delegations[i]);
        if(it == wallet.m_delegations_staker.end()) continue;

        const PKHash& keyid = PKHash(it->first);
        const Delegation* delegation = &(*it).second;

        // Set default delegate stake weight
        CAmount weight = 0;
        mDelegateWeight[it->first] = weight;

        // Get super staker custom configuration
        CAmount staking_min_utxo_value = wallet.m_staking_min_utxo_value;
        uint8_t staking_min_fee = wallet.m_staking_min_fee;
        for (std::map<uint256, CSuperStakerInfo>::const_iterator it=mapStakers.begin(); it!=mapStakers.end(); it++)
        {
            if(it->second.stakerAddress == delegation->staker)
            {
                CSuperStakerInfo info = it->second;
                if(info.fCustomConfig)
                {
                    staking_min_utxo_value = info.nMinDelegateUtxo;
                    staking_min_fee = info.nMinFee;
                }
            }
        }

        // Check for min staking fee
        if(delegation->fee < staking_min_fee)
            continue;

        // Decode address
        uint256 hashBytes;
        int type = 0;
        if (!DecodeIndexKey(EncodeDestination(keyid), hashBytes, type)) {
            LogError("Invalid address");
            return false;
        }

        // Get address utxos
        std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
        if (!GetAddressUnspent(hashBytes, type, unspentOutputs, wallet.chain().chainman().m_blockman)) {
            LogError("No information available for address");
            return false;
        }

        // Add the utxos to the list if they are mature and at least the minimum value
        int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(height + 1);
        for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator i=unspentOutputs.begin(); i!=unspentOutputs.end(); i++) {

            int nDepth = height - i->second.blockHeight + 1;
            if (nDepth < coinbaseMaturity)
                continue;

            if(i->second.satoshis < staking_min_utxo_value)
                continue;

            COutPoint prevout = COutPoint(Txid::FromUint256(i->first.txhash), i->first.index);
            if(immatureStakes.find(prevout) == immatureStakes.end())
            {
                vUnsortedDelegateCoins.push_back(std::make_pair(prevout, i->second.satoshis));
                weight+= i->second.satoshis;
            }
        }

        // Update delegate stake weight
        mDelegateWeight[it->first] = weight;
    }

    return true;
}

void AvailableAddress(const CWallet& wallet, const std::vector<uint256> &maturedTx, size_t from, size_t to, std::map<uint160, bool> &mapAddress, std::map<COutPoint, CScriptCache> *insertScriptCache)
{
    for(size_t i = from; i < to; i++)
    {
        auto it = wallet.mapWallet.find(maturedTx[i]);
        if(it == wallet.mapWallet.end()) continue;
        const uint256& wtxid = it->first;
        const CWalletTx* pcoin = &(*it).second;
        for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
            COutPoint prevout = COutPoint(Txid::FromUint256(wtxid), i);
            isminetype mine = wallet.IsMine(pcoin->tx->vout[i]);
            if (!(wallet.IsSpent(prevout)) && mine != ISMINE_NO &&
                !wallet.IsLockedCoin(prevout) && (pcoin->tx->vout[i].nValue > 0) &&
                // Check if the staking coin is dust
                pcoin->tx->vout[i].nValue >= wallet.m_staker_min_utxo_size)
            {
                // Get the script data for the coin
                const CScriptCache& scriptCache = wallet.GetScriptCache(prevout, pcoin->tx->vout[i].scriptPubKey, insertScriptCache);

                // Check that the script is not a contract script
                if(scriptCache.contract || !scriptCache.keyIdOk)
                    continue;

                bool spendable = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) && scriptCache.solvable);
                if(spendable)
                {
                    if(mapAddress.find(scriptCache.keyId) == mapAddress.end())
                    {
                        mapAddress[scriptCache.keyId] = true;
                    }
                }
            }
        }
    }
}

bool SelectCoinsForStaking(const CWallet& wallet, CAmount &nTargetValue, std::set<std::pair<const CWalletTx *, unsigned int> > &setCoinsRet, CAmount &nValueRet)
{
    std::vector<std::pair<const CWalletTx *, unsigned int> > vCoins;
    vCoins.clear();

    bool isDescriptorWallet = wallet.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS);
    int nHeight = wallet.GetLastBlockHeight() + 1;
    int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(nHeight);
    std::map<COutPoint, uint32_t> immatureStakes = wallet.chain().getImmatureStakes();
    std::vector<uint256> maturedTx;
    const bool include_watch_only = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (auto it = wallet.mapWallet.begin(); it != wallet.mapWallet.end(); ++it)
    {
        // Check the cached data for available coins for the tx
        const CWalletTx* pcoin = &(*it).second;
        const CAmount tx_credit_mine{CachedTxGetAvailableCredit(wallet, *pcoin, is_mine_filter | ISMINE_NO)};
        if(tx_credit_mine == 0)
            continue;

        const uint256& wtxid = it->first;
        int nDepth = wallet.GetTxDepthInMainChain(*pcoin);

        if (nDepth < 1)
            continue;

        if (nDepth < coinbaseMaturity)
            continue;

        if (wallet.GetTxBlocksToMaturity(*pcoin) > 0)
            continue;

        maturedTx.push_back(wtxid);
    }

    size_t listSize = maturedTx.size();
    int numThreads = std::min(wallet.m_num_threads, (int)listSize);
    if(numThreads < 2)
    {
        AvailableCoinsForStaking(wallet, maturedTx, 0, listSize, immatureStakes, vCoins, nullptr, isDescriptorWallet, nullptr);
    }
    else
    {
        size_t chunk = listSize / numThreads;
        for(int i = 0; i < numThreads; i++)
        {
            size_t from = i * chunk;
            size_t to = i == (numThreads -1) ? listSize : from + chunk;
            wallet.threads.create_thread([&wallet, from, to, &maturedTx, &immatureStakes, &vCoins, isDescriptorWallet]{
                std::vector<std::pair<const CWalletTx *, unsigned int> > tmpCoins;
                std::map<COutPoint, CScriptCache> tmpInsertScriptCache;
                std::map<uint160, bool> tmpInsertAddressStake;
                AvailableCoinsForStaking(wallet, maturedTx, from, to, immatureStakes, tmpCoins, &tmpInsertScriptCache, isDescriptorWallet, &tmpInsertAddressStake);

                LOCK(wallet.cs_worker);
                vCoins.insert(vCoins.end(), tmpCoins.begin(), tmpCoins.end());
                if((int32_t)wallet.prevoutScriptCache.size() > wallet.m_staker_max_utxo_script_cache)
                {
                    wallet.prevoutScriptCache.clear();
                }
                wallet.prevoutScriptCache.insert(tmpInsertScriptCache.begin(), tmpInsertScriptCache.end());

                // Insert the stake address cache and print warnings
                for(std::map<uint160, bool>::iterator it = tmpInsertAddressStake.begin(); it != tmpInsertAddressStake.end(); ++it)
                {
                    if(wallet.addressStakeCache.find(it->first) == wallet.addressStakeCache.end())
                    {
                        if(!it->second)
                        {
                            // Log warning that descriptor is missing
                            std::string strAddress = EncodeDestination(PKHash(it->first));
                            wallet.WalletLogPrintf("Both pkh and pk descriptors are needed for %s address to do staking\n", strAddress);
                        }
                        wallet.addressStakeCache[it->first] = it->second;
                    }
                }
            });
        }
        wallet.threads.join_all();
    }

    setCoinsRet.clear();
    nValueRet = 0;

    for(const std::pair<const CWalletTx*,unsigned int> &output : vCoins)
    {
        const CWalletTx *pcoin = output.first;
        int i = output.second;

        // Stop if we've chosen enough inputs
        if (nValueRet >= nTargetValue)
            break;

        int64_t n = pcoin->tx->vout[i].nValue;

        std::pair<int64_t,std::pair<const CWalletTx*,unsigned int> > coin = std::make_pair(n,std::make_pair(pcoin, i));

        if (n >= nTargetValue)
        {
            // If input value is greater or equal to target then simply insert
            // it into the current subset and exit
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            break;
        }
        else if (n < nTargetValue + CENT)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
        }
    }

    return true;
}

bool valueUtxoSort(const std::pair<COutPoint,CAmount>& a,
                const std::pair<COutPoint,CAmount>& b) {
    return a.second > b.second;
}

bool SelectDelegateCoinsForStaking(const CWallet& wallet, std::vector<COutPoint> &setDelegateCoinsRet, std::map<uint160, CAmount> &mDelegateWeight)
{
    AssertLockHeld(wallet.cs_wallet);

    setDelegateCoinsRet.clear();

    std::vector<std::pair<COutPoint,CAmount>> vUnsortedDelegateCoins;

    int32_t const height = wallet.chain().getHeight().value_or(-1);
    if (height == -1) {
        LogError("Invalid blockchain height");
        return false;
    }

    std::map<COutPoint, uint32_t> immatureStakes = wallet.chain().getImmatureStakes();
    std::map<uint256, CSuperStakerInfo> mapStakers = wallet.mapSuperStaker;

    std::vector<uint160> delegations;
    for (std::map<uint160, Delegation>::const_iterator it = wallet.m_delegations_staker.begin(); it != wallet.m_delegations_staker.end(); ++it)
    {
        delegations.push_back(it->first);
    }
    size_t listSize = delegations.size();
    int numThreads = std::min(wallet.m_num_threads, (int)listSize);
    bool ret = true;
    if(numThreads < 2)
    {
        ret = AvailableDelegateCoinsForStaking(wallet, delegations, 0, listSize, height, immatureStakes, mapStakers, vUnsortedDelegateCoins, mDelegateWeight);
    }
    else
    {
        size_t chunk = listSize / numThreads;
        for(int i = 0; i < numThreads; i++)
        {
            size_t from = i * chunk;
            size_t to = i == (numThreads -1) ? listSize : from + chunk;
            wallet.threads.create_thread([&wallet, from, to, height, &delegations, &immatureStakes, &mapStakers, &ret, &vUnsortedDelegateCoins, &mDelegateWeight]{
                std::vector<std::pair<COutPoint,CAmount>> tmpUnsortedDelegateCoins;
                std::map<uint160, CAmount> tmpDelegateWeight;
                bool tmpRet = AvailableDelegateCoinsForStaking(wallet, delegations, from, to, height, immatureStakes, mapStakers, tmpUnsortedDelegateCoins, tmpDelegateWeight);

                LOCK(wallet.cs_worker);
                ret &= tmpRet;
                vUnsortedDelegateCoins.insert(vUnsortedDelegateCoins.end(), tmpUnsortedDelegateCoins.begin(), tmpUnsortedDelegateCoins.end());
                mDelegateWeight.insert(tmpDelegateWeight.begin(), tmpDelegateWeight.end());
            });
        }
        wallet.threads.join_all();
    }

    std::sort(vUnsortedDelegateCoins.begin(), vUnsortedDelegateCoins.end(), valueUtxoSort);

    for(auto utxo : vUnsortedDelegateCoins){
        setDelegateCoinsRet.push_back(utxo.first);
    }

    vUnsortedDelegateCoins.clear();

    return ret;
}

void SelectAddress(const CWallet& wallet, std::map<uint160, bool> &mapAddress)
{
    std::vector<uint256> maturedTx;
    const bool include_watch_only = wallet.GetLegacyScriptPubKeyMan() && wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter = include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (auto it = wallet.mapWallet.begin(); it != wallet.mapWallet.end(); ++it)
    {
        // Check the cached data for available coins for the tx
        const CWalletTx* pcoin = &(*it).second;
        const CAmount tx_credit_mine{CachedTxGetAvailableCredit(wallet, *pcoin, is_mine_filter | ISMINE_NO)};
        if(tx_credit_mine == 0)
            continue;

        const uint256& wtxid = it->first;
        int nDepth = wallet.GetTxDepthInMainChain(*pcoin);

        if (nDepth < 1)
            continue;

        if (wallet.GetTxBlocksToMaturity(*pcoin) > 0)
            continue;

        maturedTx.push_back(wtxid);
    }

    size_t listSize = maturedTx.size();
    int numThreads = std::min(wallet.m_num_threads, (int)listSize);
    if(numThreads < 2)
    {
        AvailableAddress(wallet, maturedTx, 0, listSize, mapAddress, nullptr);
    }
    else
    {
        size_t chunk = listSize / numThreads;
        for(int i = 0; i < numThreads; i++)
        {
            size_t from = i * chunk;
            size_t to = i == (numThreads -1) ? listSize : from + chunk;
            wallet.threads.create_thread([&wallet, from, to, &maturedTx, &mapAddress]{
                std::map<uint160, bool> tmpAddresses;
                std::map<COutPoint, CScriptCache> tmpInsertScriptCache;
                AvailableAddress(wallet, maturedTx, from, to, tmpAddresses, &tmpInsertScriptCache);

                LOCK(wallet.cs_worker);
                mapAddress.insert(tmpAddresses.begin(), tmpAddresses.end());
                if((int32_t)wallet.prevoutScriptCache.size() > wallet.m_staker_max_utxo_script_cache)
                {
                    wallet.prevoutScriptCache.clear();
                }
                wallet.prevoutScriptCache.insert(tmpInsertScriptCache.begin(), tmpInsertScriptCache.end());
            });
        }
        wallet.threads.join_all();
    }
}

void UpdateMinerStakeCache(CWallet& wallet, bool fStakeCache, const std::vector<COutPoint> &prevouts, CBlockIndex *pindexPrev )
{
    if(wallet.minerStakeCache.size() > prevouts.size() + 100){
        wallet.minerStakeCache.clear();
    }

    if(fStakeCache)
    {
        for(const COutPoint &prevoutStake : prevouts)
        {
            boost::this_thread::interruption_point();
            CacheKernel(wallet.minerStakeCache, prevoutStake, pindexPrev, wallet.chain().getCoinsTip());
        }
        if(!wallet.fHasMinerStakeCache) wallet.fHasMinerStakeCache = true;
    }
}

uint64_t GetStakeWeight(const CWallet& wallet, uint64_t* pStakerWeight, uint64_t* pDelegateWeight)
{
    uint64_t nWeight = 0;
    uint64_t nStakerWeight = 0;
    uint64_t nDelegateWeight = 0;
    if(pStakerWeight) *pStakerWeight = nStakerWeight;
    if(pDelegateWeight) *pDelegateWeight = nDelegateWeight;

    // Choose coins to use
    const auto bal = GetBalance(wallet);
    CAmount nBalance = bal.m_mine_trusted;
    if(wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS))
        nBalance += bal.m_watchonly_trusted;

    if (nBalance <= wallet.m_reserve_balance)
        return nWeight;

    std::set<std::pair<const CWalletTx*,unsigned int> > setCoins;
    CAmount nValueIn = 0;

    CAmount nTargetValue = nBalance - wallet.m_reserve_balance;
    if (!SelectCoinsForStaking(wallet, nTargetValue, setCoins, nValueIn))
        return nWeight;

    if (setCoins.empty())
        return nWeight;

    int nHeight = wallet.GetLastBlockHeight() + 1;
    int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(nHeight);
    bool canSuperStake = false;
    for(std::pair<const CWalletTx*,unsigned int> pcoin : setCoins)
    {
        if (wallet.GetTxDepthInMainChain(*pcoin.first) >= coinbaseMaturity)
        {
            // Compute staker weight
            CAmount nValue = pcoin.first->tx->vout[pcoin.second].nValue;
            nStakerWeight += nValue;

            // Check if the staker can super stake
            if(!canSuperStake && nValue >= DEFAULT_STAKING_MIN_UTXO_VALUE)
                canSuperStake = true;
        }
    }

    if(canSuperStake)
    {
        // Get the weight of the delegated coins
        std::vector<COutPoint> vDelegateCoins;
        std::map<uint160, CAmount> mDelegateWeight;
        SelectDelegateCoinsForStaking(wallet, vDelegateCoins, mDelegateWeight);
        for(const COutPoint &prevout : vDelegateCoins)
        {
            Coin coinPrev;
            if(!wallet.chain().getUnspentOutput(prevout, coinPrev)){
                continue;
            }

            nDelegateWeight += coinPrev.out.nValue;
        }
    }

    nWeight = nStakerWeight + nDelegateWeight;
    if(pStakerWeight) *pStakerWeight = nStakerWeight;
    if(pDelegateWeight) *pDelegateWeight = nDelegateWeight;

    return nWeight;
}

} // namespace wallet

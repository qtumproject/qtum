#include <wallet/stake.h>
#include <wallet/receive.h>
#include <node/miner.h>
#include <qtum/qtumledger.h>
#include <pos.h>
#include <key_io.h>

namespace wallet {

void StakeQtums(CWallet& wallet, bool fStake)
{
    node::StakeQtums(fStake, &wallet, wallet.stakeThread);
}

void StartStake(CWallet& wallet)
{
    if(wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS))
    {
        wallet.m_enabled_staking = !wallet.m_ledger_id.empty() && QtumLedger::instance().toolExists();
    }
    else
    {
        wallet.m_enabled_staking = true;
    }

    StakeQtums(wallet, true);
}

void StopStake(CWallet& wallet)
{
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
        if (CheckKernel(pindexPrev, nBits, nTimeBlock, prevoutStake, wallet.chain().getCoinsTip(), cache, wallet.chain().chainman().ActiveChain()))
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
        if(!CreateMPoSOutputs(txNew, nRewardPiece, pindexPrev->nHeight, consensusParams, wallet.chain().chainman().ActiveChain()))
            return error("CreateCoinStake : failed to create MPoS reward outputs");
    }

    // Append the Refunds To Sender to the transaction outputs
    for(unsigned int i = 2; i < tx.vout.size(); i++)
    {
        txNew.vout.push_back(tx.vout[i]);
    }

    // Sign the input coins
    if(sign && !wallet.SignTransactionStake(txNew, vwtxPrev))
        return error("CreateCoinStake : failed to sign coinstake");

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
        if (CheckKernel(pindexPrev, nBits, nTimeBlock, prevoutStake, wallet.chain().getCoinsTip(), cache, wallet.chain().chainman().ActiveChain()))
        {
            // Found a kernel
            LogPrint(BCLog::COINSTAKE, "CreateCoinStake : kernel found\n");
            std::vector<valtype> vSolutions;

            Coin coinPrev;
            if(!wallet.chain().getUnspentOutput(prevoutStake, coinPrev)){
                if(!GetSpentCoinFromMainChain(pindexPrev, prevoutStake, &coinPrev, wallet.chain().chainman().ActiveChain())) {
                    return error("CreateCoinStake: Could not find coin and it was not at the tip");
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

                if(!wallet.GetDelegationStaker(hash160, delegation))
                    return error("CreateCoinStake: Failed to find delegation");

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

                if(!wallet.GetDelegationStaker(hash160, delegation))
                    return error("CreateCoinStake: Failed to find delegation");

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
            if(!SplitOfflineStakeReward(nTotalReward, delegation.fee, nRewardOffline, nRewardStaker))
                return error("CreateCoinStake: Failed to split reward");
            nCredit += nRewardStaker;
        }
        else
        {
            // Split the reward when mpos is used
            nRewardPiece = nTotalReward / consensusParams.nMPoSRewardRecipients;
            int64_t nRewardStaker = 0;
            int64_t nReward = nRewardPiece + nTotalReward % consensusParams.nMPoSRewardRecipients;
            if(!SplitOfflineStakeReward(nReward, delegation.fee, nRewardOffline, nRewardStaker))
                return error("CreateCoinStake: Failed to split reward");
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
        if(!CreateMPoSOutputs(txNew, nRewardPiece, pindexPrev->nHeight, consensusParams, wallet.chain().chainman().ActiveChain()))
            return error("CreateCoinStake : failed to create MPoS reward outputs");
    }

    // Append the Refunds To Sender to the transaction outputs
    for(unsigned int i = 2; i < tx.vout.size(); i++)
    {
        txNew.vout.push_back(tx.vout[i]);
    }

    // Sign the input coins
    if(sign && !wallet.SignTransactionStake(txNew, vwtxPrev))
        return error("CreateCoinStake : failed to sign coinstake");

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

} // namespace wallet

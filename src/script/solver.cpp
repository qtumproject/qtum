// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <span.h>
#include <qtum/qtumDGP.h>
#include <qtum/qtumtransaction.h>
#include <validation.h>
#include <addresstype.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <streams.h>

typedef std::vector<unsigned char> valtype;

std::string GetTxnOutputType(TxoutType t)
{
    switch (t) {
    case TxoutType::NONSTANDARD: return "nonstandard";
    case TxoutType::PUBKEY: return "pubkey";
    case TxoutType::PUBKEYHASH: return "pubkeyhash";
    case TxoutType::SCRIPTHASH: return "scripthash";
    case TxoutType::MULTISIG: return "multisig";
    case TxoutType::NULL_DATA: return "nulldata";
    case TxoutType::ANCHOR: return "anchor";
    case TxoutType::WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TxoutType::WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TxoutType::WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TxoutType::WITNESS_UNKNOWN: return "witness_unknown";
    case TxoutType::CREATE_SENDER: return "create_sender";
    case TxoutType::CALL_SENDER: return "call_sender";
    case TxoutType::CREATE: return "create";
    case TxoutType::CALL: return "call";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin () + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

/** Retrieve a minimally-encoded number in range [min,max] from an (opcode, data) pair,
 *  whether it's OP_n or through a push. */
static std::optional<int> GetScriptNumber(opcodetype opcode, valtype data, int min, int max)
{
    int count;
    if (IsSmallInteger(opcode)) {
        count = CScript::DecodeOP_N(opcode);
    } else if (IsPushdataOp(opcode)) {
        if (!CheckMinimalPush(data, opcode)) return {};
        try {
            count = CScriptNum(data, /* fRequireMinimal = */ true).getint();
        } catch (const scriptnum_error&) {
            return {};
        }
    } else {
        return {};
    }
    if (count < min || count > max) return {};
    return count;
}

static bool MatchMultisig(const CScript& script, int& required_sigs, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;

    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data)) return false;
    auto req_sigs = GetScriptNumber(opcode, data, 1, MAX_PUBKEYS_PER_MULTISIG);
    if (!req_sigs) return false;
    required_sigs = *req_sigs;
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    auto num_keys = GetScriptNumber(opcode, data, required_sigs, MAX_PUBKEYS_PER_MULTISIG);
    if (!num_keys) return false;
    if (pubkeys.size() != static_cast<unsigned long>(*num_keys)) return false;

    return (it + 1 == script.end());
}

std::optional<std::pair<int, std::vector<Span<const unsigned char>>>> MatchMultiA(const CScript& script)
{
    std::vector<Span<const unsigned char>> keyspans;

    // Redundant, but very fast and selective test.
    if (script.size() == 0 || script[0] != 32 || script.back() != OP_NUMEQUAL) return {};

    // Parse keys
    auto it = script.begin();
    while (script.end() - it >= 34) {
        if (*it != 32) return {};
        ++it;
        keyspans.emplace_back(&*it, 32);
        it += 32;
        if (*it != (keyspans.size() == 1 ? OP_CHECKSIG : OP_CHECKSIGADD)) return {};
        ++it;
    }
    if (keyspans.size() == 0 || keyspans.size() > MAX_PUBKEYS_PER_MULTI_A) return {};

    // Parse threshold.
    opcodetype opcode;
    std::vector<unsigned char> data;
    if (!script.GetOp(it, opcode, data)) return {};
    if (it == script.end()) return {};
    if (*it != OP_NUMEQUAL) return {};
    ++it;
    if (it != script.end()) return {};
    auto threshold = GetScriptNumber(opcode, data, 1, (int)keyspans.size());
    if (!threshold) return {};

    // Construct result.
    return std::pair{*threshold, std::move(keyspans)};
}

static bool MatchContract(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet, bool contractConsensus, bool allowEmptySenderSig, TxoutType& typeRet )
{
    //contractConsesus is true when evaluating if a contract tx is "standard" for consensus purposes
    //It is false in all other cases, so to prevent a particular contract tx from being broadcast on mempool, but allowed in blocks,
    //one should ensure that contractConsensus is false

    // Templates
    static std::multimap<TxoutType, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Contract creation tx with sender
        mTemplates.insert(std::make_pair(TxoutType::CREATE_SENDER, CScript() << OP_ADDRESS_TYPE << OP_ADDRESS << OP_SCRIPT_SIG << OP_SENDER << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_CREATE));

        // Call contract tx with sender
        mTemplates.insert(std::make_pair(TxoutType::CALL_SENDER, CScript() << OP_ADDRESS_TYPE << OP_ADDRESS << OP_SCRIPT_SIG << OP_SENDER << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_PUBKEYHASH << OP_CALL));

        // Contract creation tx
        mTemplates.insert(std::make_pair(TxoutType::CREATE, CScript() << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_CREATE));

        // Call contract tx
        mTemplates.insert(std::make_pair(TxoutType::CALL, CScript() << OP_VERSION << OP_GAS_LIMIT << OP_GAS_PRICE << OP_DATA << OP_PUBKEYHASH << OP_CALL));
    }

    // Scan templates
    typeRet = TxoutType::NONSTANDARD;
    const CScript& script1 = scriptPubKey;
    for (const std::pair<TxoutType, CScript>& tplate : mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        uint64_t addressType = 0;
        VersionVM version;
        version.rootVM=20; //set to some invalid value

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet =  tplate.first;
                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }
            else if (opcode2 == OP_VERSION)
            {
                if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
                {
                    if(vch1.empty() || vch1.size() > 4 || (vch1.back() & 0x80))
                        return false;

                    version = VersionVM::fromRaw(CScriptNum::vch_to_uint64(vch1));
                    if(!(version.toRaw() == VersionVM::GetEVMDefault().toRaw() || version.toRaw() == VersionVM::GetNoExec().toRaw())){
                        // only allow standard EVM and no-exec transactions to live in mempool
                        return false;
                    }
                }
            }
            else if(opcode2 == OP_GAS_LIMIT) {
                try {
                    uint64_t val = CScriptNum::vch_to_uint64(vch1);
                    if(contractConsensus) {
                        //consensus rules (this is checked more in depth later using DGP)
                        if (version.rootVM != 0 && val < 1) {
                            return false;
                        }
                        if (val > MAX_BLOCK_GAS_LIMIT_DGP) {
                            //do not allow transactions that could use more gas than is in a block
                            return false;
                        }
                    }else{
                        //standard mempool rules for contracts
                        //consensus rules for contracts
                        if (version.rootVM != 0 && val < STANDARD_MINIMUM_GAS_LIMIT) {
                            return false;
                        }
                        if (val > DEFAULT_BLOCK_GAS_LIMIT_DGP / 2) {
                            //don't allow transactions that use more than 1/2 block of gas to be broadcast on the mempool
                            return false;
                        }

                    }
                }
                catch (const scriptnum_error &err) {
                    return false;
                }
            }
            else if(opcode2 == OP_GAS_PRICE) {
                try {
                    uint64_t val = CScriptNum::vch_to_uint64(vch1);
                    if(contractConsensus) {
                        //consensus rules (this is checked more in depth later using DGP)
                        if (version.rootVM != 0 && val < 1) {
                            return false;
                        }
                    }else{
                        //standard mempool rules
                        if (version.rootVM != 0 && val < STANDARD_MINIMUM_GAS_PRICE) {
                            return false;
                        }
                    }
                }
                catch (const scriptnum_error &err) {
                    return false;
                }
            }
            else if(opcode2 == OP_DATA)
            {
                if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
                {
                    if(vch1.empty())
                        break;
                }
            }
            else if(opcode2 == OP_ADDRESS_TYPE)
            {
                try {
                    uint64_t val = CScriptNum::vch_to_uint64(vch1);
                    if(val < addresstype::PUBKEYHASH || val > addresstype::NONSTANDARD)
                        break;

                    addressType = val;
                }
                catch (const scriptnum_error &err) {
                    break;
                }
            }
            else if(opcode2 == OP_ADDRESS)
            {
                // Get the destination
                CTxDestination dest;
                if(addressType == addresstype::PUBKEYHASH && vch1.size() == sizeof(PKHash))
                {
                    dest = PKHash(uint160(vch1));
                }
                else
                    break;

                // Get the public key for the destination
                CScript senderPubKey = GetScriptForDestination(dest);
                DataStream ss;
                ss << senderPubKey;
                Span<const uint8_t> sp = MakeUCharSpan(ss);
                vSolutionsRet.push_back(std::vector<unsigned char>(sp.begin(), sp.end()));
            }
            else if(opcode2 == OP_SCRIPT_SIG)
            {
                if(0 <= opcode1 && opcode1 <= OP_PUSHDATA4)
                {
                    if(!allowEmptySenderSig && vch1.empty())
                        break;

                    // Check the max size of the signature script
                    if(vch1.size() > MAX_BASE_SCRIPT_SIZE)
                        return false;

                    vSolutionsRet.push_back(vch1);
                }
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    return false;
}

TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet, bool contractConsensus, bool allowEmptySenderSig)
{
    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return TxoutType::SCRIPTHASH;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
        }
        if (scriptPubKey.IsPayToAnchor()) {
            return TxoutType::ANCHOR;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_UNKNOWN;
        }
        return TxoutType::NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        return TxoutType::NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEYHASH;
    }

    int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..20
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..20
        return TxoutType::MULTISIG;
    }

    TxoutType typeContract = TxoutType::NONSTANDARD;
    if(MatchContract(scriptPubKey, vSolutionsRet, contractConsensus, allowEmptySenderSig, typeContract))
    {
        return typeContract;
    }

    vSolutionsRet.clear();
    return TxoutType::NONSTANDARD;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << nRequired;
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << keys.size() << OP_CHECKMULTISIG;

    return script;
}

bool ExtractSenderData(const CScript &outputPubKey, CScript *senderPubKey, CScript *senderSig)
{
    if(outputPubKey.HasOpSender())
    {
        try
        {
            // Solve the contract with or without contract consensus
            std::vector<valtype> vSolutions;
            if (TxoutType::NONSTANDARD == Solver(outputPubKey, vSolutions, true) &&
                    TxoutType::NONSTANDARD == Solver(outputPubKey, vSolutions, false))
                return false;

            // Check the size of the returned data
            if(vSolutions.size() < 2)
                return false;

            // Get the sender public key
            if(senderPubKey)
            {
                DataStream ss(vSolutions[0]);
                ss >> *senderPubKey;
            }

            // Get the sender signature
            if(senderSig)
            {
                DataStream ss(vSolutions[1]);
                ss >> *senderSig;
            }
        }
        catch(...)
        {
            return false;
        }

        return true;
    }
    return false;
}

bool GetSenderPubKey(const CScript &outputPubKey, CScript &senderPubKey)
{
    if(outputPubKey.HasOpSender())
    {
        try
        {
            // Solve the contract with or without contract consensus
            std::vector<valtype> vSolutions;
            if (TxoutType::NONSTANDARD == Solver(outputPubKey, vSolutions, true, true) &&
                    TxoutType::NONSTANDARD == Solver(outputPubKey, vSolutions, false, true))
                return false;

            // Check the size of the returned data
            if(vSolutions.size() < 1)
                return false;

            // Get the sender public key
            DataStream ss(vSolutions[0]);
            ss >> senderPubKey;
        }
        catch(...)
        {
            return false;
        }

        return true;
    }
    return false;
}

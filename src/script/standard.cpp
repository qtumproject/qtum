// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>

#include <qtum/qtumstate.h>
#include <qtum/qtumDGP.h>
#include <qtum/qtumtransaction.h>
#include <validation.h>
#include <streams.h>

#include <string>

typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : BaseHash(Hash160(in)) {}
CScriptID::CScriptID(const ScriptHash& in) : BaseHash(static_cast<uint160>(in)) {}

ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash(static_cast<uint160>(in)) {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash(static_cast<uint160>(pubkey_hash)) {}

CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}

CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

std::string GetTxnOutputType(TxoutType t)
{
    switch (t)
    {
    case TxoutType::NONSTANDARD: return "nonstandard";
    case TxoutType::PUBKEY: return "pubkey";
    case TxoutType::PUBKEYHASH: return "pubkeyhash";
    case TxoutType::SCRIPTHASH: return "scripthash";
    case TxoutType::MULTISIG: return "multisig";
    case TxoutType::NULL_DATA: return "nulldata";
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

static bool MatchMultisig(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;
    return (it + 1 == script.end());
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
                CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                ss << senderPubKey;
                vSolutionsRet.push_back(std::vector<unsigned char>(ss.begin(), ss.end()));
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
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
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

    unsigned int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
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

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet, TxoutType *typeRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    if(typeRet){
        *typeRet = whichType;
    }

    if (whichType == TxoutType::PUBKEY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    else if (whichType == TxoutType::PUBKEYHASH)
    {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TxoutType::SCRIPTHASH)
    {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TxoutType::WITNESS_V0_KEYHASH) {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TxoutType::WITNESS_V0_SCRIPTHASH) {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TxoutType::WITNESS_UNKNOWN || whichType == TxoutType::WITNESS_V1_TAPROOT) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, TxoutType& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet, bool contractConsensus)
{
    addressRet.clear();
    std::vector<valtype> vSolutions;
    typeRet = Solver(scriptPubKey, vSolutions, contractConsensus);
    if (typeRet == TxoutType::NONSTANDARD) {
        return false;
    } else if (typeRet == TxoutType::NULL_DATA) {
        // This is data, not addresses
        return false;
    }

    if (typeRet == TxoutType::MULTISIG)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = PKHash(pubKey);
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    }
    else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace
{
class CScriptVisitor : public boost::static_visitor<CScript>
{
public:
    CScript operator()(const CNoDestination& dest) const
    {
        return CScript();
    }

    CScript operator()(const PKHash& keyID) const
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    CScript operator()(const ScriptHash& scriptID) const
    {
        return CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    }

    CScript operator()(const WitnessV0KeyHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessV0ScriptHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessUnknown& id) const
    {
        return CScript() << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
    }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    return boost::apply_visitor(CScriptVisitor(), dest);
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.which() != 0;
}

bool IsValidContractSenderAddress(const CTxDestination &dest)
{
    const PKHash *keyID = boost::get<PKHash>(&dest);
    return keyID != 0;
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
                CDataStream ss(vSolutions[0], SER_NETWORK, PROTOCOL_VERSION);
                ss >> *senderPubKey;
            }

            // Get the sender signature
            if(senderSig)
            {
                CDataStream ss(vSolutions[1], SER_NETWORK, PROTOCOL_VERSION);
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
            CDataStream ss(vSolutions[0], SER_NETWORK, PROTOCOL_VERSION);
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

PKHash ExtractPublicKeyHash(const CScript& scriptPubKey, bool* OK)
{
    if(OK) *OK = false;
    CTxDestination address;
    TxoutType txType=TxoutType::NONSTANDARD;
    if(ExtractDestination(scriptPubKey, address, &txType)){
        if ((txType == TxoutType::PUBKEY || txType == TxoutType::PUBKEYHASH) && address.type() == typeid(PKHash)) {
            if(OK) *OK = true;
            return boost::get<PKHash>(address);
        }
    }

    return PKHash();
}

valtype DataVisitor::operator()(const CNoDestination& noDest) const { return valtype(); }
valtype DataVisitor::operator()(const PKHash& keyID) const { return valtype(keyID.begin(), keyID.end()); }
valtype DataVisitor::operator()(const ScriptHash& scriptID) const { return valtype(scriptID.begin(), scriptID.end()); }
valtype DataVisitor::operator()(const WitnessV0ScriptHash& witnessScriptHash) const { return valtype(witnessScriptHash.begin(), witnessScriptHash.end()); }
valtype DataVisitor::operator()(const WitnessV0KeyHash& witnessKeyHash) const { return valtype(witnessKeyHash.begin(), witnessKeyHash.end()); }
valtype DataVisitor::operator()(const WitnessUnknown&) const { return valtype(); }

bool ExtractDestination(const COutPoint& prevout, const CScript& scriptPubKey, CTxDestination& addressRet, TxoutType* typeRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    if(typeRet){
        *typeRet = whichType;
    }


    if (whichType == TxoutType::PUBKEY)
    {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    else if (whichType == TxoutType::PUBKEYHASH)
    {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TxoutType::SCRIPTHASH)
    {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    else if(whichType == TxoutType::CALL){
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if(whichType == TxoutType::WITNESS_V0_KEYHASH)
    {
        addressRet = WitnessV0KeyHash(uint160(vSolutions[0]));
        return true;
    }
    else if(whichType == TxoutType::WITNESS_V0_SCRIPTHASH)
    {
        addressRet = WitnessV0ScriptHash(uint256(vSolutions[0]));
        return true;
    }
    else if (whichType == TxoutType::WITNESS_UNKNOWN) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    else if (whichType == TxoutType::CREATE) {
        addressRet = PKHash(uint160(QtumState::createQtumAddress(uintToh256(prevout.hash), prevout.n).asBytes()));
        return true;
    }
    return false;
}

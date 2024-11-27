// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>

#include <crypto/sha256.h>
#include <hash.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/solver.h>
#include <uint256.h>
#include <util/hash_type.h>
#include <qtum/qtumstate.h>

#include <cassert>
#include <vector>


ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash{in} {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash{pubkey_hash} {}

CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{uint160{key_hash}};
}

CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
    return CKeyID{uint160{key_hash}};
}

CScriptID ToScriptID(const ScriptHash& script_hash)
{
    return CScriptID{uint160{script_hash}};
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet, TxoutType* typeRet, bool convertPublicKeyToHash)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    if(typeRet){
        *typeRet = whichType;
    }

    switch (whichType) {
    case TxoutType::PUBKEY: {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid()) {
            addressRet = CNoDestination(scriptPubKey);
        } else if (convertPublicKeyToHash) {
            addressRet = PKHash(pubKey);
            return true;
        } else {
            addressRet = PubKeyDestination(pubKey);
        }
        return false;
    }
    case TxoutType::PUBKEYHASH: {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::WITNESS_V0_KEYHASH: {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH: {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V1_TAPROOT: {
        WitnessV1Taproot tap;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
        addressRet = tap;
        return true;
    }
    case TxoutType::ANCHOR: {
        addressRet = PayToAnchor();
        return true;
    }
    case TxoutType::WITNESS_UNKNOWN: {
        addressRet = WitnessUnknown{vSolutions[0][0], vSolutions[1]};
        return true;
    }
    case TxoutType::MULTISIG:
    case TxoutType::NULL_DATA:
    case TxoutType::NONSTANDARD:
    case TxoutType::CREATE_SENDER:
    case TxoutType::CALL_SENDER:
    case TxoutType::CREATE:
    case TxoutType::CALL:
        addressRet = CNoDestination(scriptPubKey);
        return false;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

namespace {
class CScriptVisitor
{
public:
    CScript operator()(const CNoDestination& dest) const
    {
        return dest.GetScript();
    }

    CScript operator()(const PubKeyDestination& dest) const
    {
        return CScript() << ToByteVector(dest.GetPubKey()) << OP_CHECKSIG;
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

    CScript operator()(const WitnessV1Taproot& tap) const
    {
        return CScript() << OP_1 << ToByteVector(tap);
    }

    CScript operator()(const WitnessUnknown& id) const
    {
        return CScript() << CScript::EncodeOP_N(id.GetWitnessVersion()) << id.GetWitnessProgram();
    }
};

class ValidDestinationVisitor
{
public:
    bool operator()(const CNoDestination& dest) const { return false; }
    bool operator()(const PubKeyDestination& dest) const { return false; }
    bool operator()(const PKHash& dest) const { return true; }
    bool operator()(const ScriptHash& dest) const { return true; }
    bool operator()(const WitnessV0KeyHash& dest) const { return true; }
    bool operator()(const WitnessV0ScriptHash& dest) const { return true; }
    bool operator()(const WitnessV1Taproot& dest) const { return true; }
    bool operator()(const WitnessUnknown& dest) const { return true; }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    return std::visit(CScriptVisitor(), dest);
}

bool IsValidDestination(const CTxDestination& dest) {
    return std::visit(ValidDestinationVisitor(), dest);
}

PKHash ExtractPublicKeyHash(const CScript& scriptPubKey, bool* OK)
{
    if(OK) *OK = false;
    CTxDestination address;
    TxoutType txType=TxoutType::NONSTANDARD;
    if(ExtractDestination(scriptPubKey, address, &txType, true)){
        if ((txType == TxoutType::PUBKEY || txType == TxoutType::PUBKEYHASH) && std::holds_alternative<PKHash>(address)) {
            if(OK) *OK = true;
            return std::get<PKHash>(address);
        }
    }

    return PKHash();
}

bool IsValidContractSenderAddress(const CTxDestination& dest) {
    return std::holds_alternative<PKHash>(dest);
}

valtype DataVisitor::operator()(const CNoDestination& noDest) const { return valtype(); }
valtype DataVisitor::operator()(const PKHash& keyID) const { return valtype(keyID.begin(), keyID.end()); }
valtype DataVisitor::operator()(const PubKeyDestination& pubKey) const {
    PKHash keyID(pubKey.GetPubKey());
    return valtype(keyID.begin(), keyID.end());
}
valtype DataVisitor::operator()(const ScriptHash& scriptID) const { return valtype(scriptID.begin(), scriptID.end()); }
valtype DataVisitor::operator()(const WitnessV0ScriptHash& witnessScriptHash) const { return valtype(witnessScriptHash.begin(), witnessScriptHash.end()); }
valtype DataVisitor::operator()(const WitnessV0KeyHash& witnessKeyHash) const { return valtype(witnessKeyHash.begin(), witnessKeyHash.end()); }
valtype DataVisitor::operator()(const WitnessV1Taproot& witnessTaproot) const { return valtype(witnessTaproot.begin(), witnessTaproot.end()); }
valtype DataVisitor::operator()(const PayToAnchor& payToAnchor) const { return valtype(); }
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
    else if(whichType == TxoutType::ANCHOR)
    {
        addressRet = PayToAnchor();
        return true;
    }
    else if(whichType == TxoutType::WITNESS_V1_TAPROOT)
    {
        WitnessV1Taproot tap;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), tap.begin());
        addressRet = tap;
        return true;
    }
    else if (whichType == TxoutType::WITNESS_UNKNOWN) {
        addressRet = WitnessUnknown{vSolutions[0][0], vSolutions[1]};
        return true;
    }
    else if (whichType == TxoutType::CREATE) {
        addressRet = PKHash(uint160(QtumState::createQtumAddress(uintToh256(prevout.hash), prevout.n).asBytes()));
        return true;
    }
    return false;
}

int GetAddressIndexType(const CTxDestination &dest)
{
    if(dest.index() > 1)
        return dest.index() - 1; // PubKeyDestination and PKHash are considered the same
    return dest.index();
}

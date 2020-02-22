// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>

#include <primitives/transaction.h>
#include <consensus/validation.h>
#include <script/standard.h>

bool CheckTransaction(const CTransaction& tx, CValidationState &state, bool fCheckDuplicateInputs)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > MAX_TRANSACTION_BASE_SIZE || 
        ::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > dgpMaxBlockWeight)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values (see CVE-2010-5139)
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.IsEmpty() && !tx.IsCoinBase() && !tx.IsCoinStake())
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-vout-empty");
        if (txout.nValue < 0)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");

        /////////////////////////////////////////////////////////// // qtum
        if (txout.scriptPubKey.HasOpCall() || txout.scriptPubKey.HasOpCreate() || txout.scriptPubKey.HasOpSender()) {
            std::vector<valtype> vSolutions;
            txnouttype whichType = Solver(txout.scriptPubKey, vSolutions, true);
            if (whichType == TX_NONSTANDARD) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-contract-nonstandard");
            }
        }
        ///////////////////////////////////////////////////////////
    }

    // Check for duplicate inputs - note that this check is slow so we skip it in CheckBlock
    if (fCheckDuplicateInputs) {
        std::set<COutPoint> vInOutPoints;
        for (const auto& txin : tx.vin)
        {
            if (!vInOutPoints.insert(txin.prevout).second)
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        }
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

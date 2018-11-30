#ifndef BITCOIN_STAKE_STAKETX_H
#define BITCOIN_STAKE_STAKETX_H

#include <stake/staketx.h>
//@author yaopf(yaopf@eb-tech.cn)
// this file is ported from decred staketx.go


// generation transaction.  It does some simple validation steps to make sure
// the number of inputs, number of outputs, and the input/output scripts are
// valid.
//
// This does NOT check to see if the subsidy is valid or whether or not the
// value of input[0] + subsidy = value of the outputs.
//
// SSGen transactions are specified as below.
// Inputs:
// Stakebase null input [index 0]
// SStx-tagged output [index 1]
//
// Outputs:
// OP_RETURN push of 40 bytes containing: [index 0]
//     i. 32-byte block header of block being voted on.
//     ii. 8-byte int of this block's height.
// OP_RETURN push of 2 bytes containing votebits [index 1]
// SSGen-tagged output to address from SStx-tagged output's tx index output 1
//     [index 2]
// SSGen-tagged output to address from SStx-tagged output's tx index output 2
//     [index 3]
// ...
// SSGen-tagged output to address from SStx-tagged output's tx index output
//     MaxInputsPerSStx [index MaxOutputsPerSSgen - 1]
bool CheckSSgen(const CTransaction& tx, CValidationState &state)
{

}

#endif //BITCOIN_STAKE_STAKETX_H

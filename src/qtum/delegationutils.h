#ifndef DELEGATIONUTILS_H
#define DELEGATIONUTILS_H

#include <string>
#include <vector>

/**
 * delegationutils Provides utility functions for qtum delegation
 */
namespace delegationutils
{
/**
 * @brief IsAddBytecode Quick check for if the bytecode is for addDelegation method
 * @param data Bytecode of contract
 * @return true/false
 */
bool IsAddBytecode(const std::vector<unsigned char>& data);

/**
 * @brief GetUnsignedStaker Get unsigned staker address from PoD
 * @param data Bytecode of contract
 * @param hexStaker Staker hex address
 * @return true/false
 */
bool GetUnsignedStaker(const std::vector<unsigned char>& data, std::string& hexStaker);

/**
 * @brief SetSignedStaker Set signed staker address into PoD
 * @param data Bytecode of contract
 * @param base64PoD Staker address signed in base64
 * @return true/false
 */
bool SetSignedStaker(std::vector<unsigned char>& data, const std::string& base64PoD);
}

#endif

/**
 * Token parsing/formatting utilities.
 */
#ifndef UTIL_TOKENSTR_H
#define UTIL_TOKENSTR_H

#include <libdevcore/Common.h>
#include <attributes.h>

#include <string>

std::string FormatToken(const unsigned int& decimals, const dev::s256& n);
NODISCARD bool ParseToken(const unsigned int& decimals, const std::string& str, dev::s256& nRet);

#endif // UTIL_TOKENSTR_H

#ifndef QTUM_CONVERT_H
#define QTUM_CONVERT_H

////////////////////////////////////////////////////// qtum
#include <uint256.h>
#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/FixedHash.h>

inline dev::h256 uintToh256(const uint256& in)
{
    std::vector<unsigned char> vHashBlock;
    vHashBlock.assign(in.begin(), in.end());
    return dev::h256(vHashBlock);
}

inline uint256 h256Touint(const dev::h256& in)
{
	std::vector<unsigned char> vHashBlock = in.asBytes();
	return uint256(vHashBlock);
}

inline dev::u256 uintTou256(const uint256& in)
{
    std::vector<unsigned char> rawValue;
    rawValue.assign(in.begin(), in.end());
    return dev::fromBigEndian<dev::u256, dev::bytes>(rawValue);
}

inline uint256 u256Touint(const dev::u256& in)
{
    std::vector<unsigned char> rawValue(32, 0);
    dev::toBigEndian<dev::u256, dev::bytes>(in, rawValue);
	return uint256(rawValue);
}
//////////////////////////////////////////////////////

#endif // QTUM_CONVERT_H

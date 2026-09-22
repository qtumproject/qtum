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

inline dev::h160 uintToh160(const uint160& in)
{
    std::vector<unsigned char> vHashBlock;
    vHashBlock.assign(in.begin(), in.end());
    return dev::h160(vHashBlock);
}

inline uint160 h160Touint(const dev::h160& in)
{
	std::vector<unsigned char> vHashBlock = in.asBytes();
	return uint160(vHashBlock);
}

inline dev::u160 uintTou160(const uint160& in)
{
    std::vector<unsigned char> rawValue;
    rawValue.assign(in.begin(), in.end());
    return dev::fromBigEndian<dev::u160, dev::bytes>(rawValue);
}

inline uint160 u160Touint(const dev::u160& in)
{
    std::vector<unsigned char> rawValue(20, 0);
    dev::toBigEndian<dev::u160, dev::bytes>(in, rawValue);
	return uint160(rawValue);
}
//////////////////////////////////////////////////////

#endif // QTUM_CONVERT_H

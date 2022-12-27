/**
 * Sign verify message utilities.
 */
#ifndef UTIL_SIGNSTR_H
#define UTIL_SIGNSTR_H

#include <string>
#include <key.h>
#include <hash.h>


namespace SignStr
{
const std::string strMessageMagic = "Qtum Signed Message:\n";

inline bool SignMessage(const CKey& key, const std::string& strMessage, std::vector<unsigned char>& vchSig)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    return key.SignCompact(ss.GetHash(), vchSig);
}

inline bool VerifyMessage(const CKeyID& keyID, const std::string& strMessage, const std::vector<unsigned char>& vchSig)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

inline bool GetKeyIdMessage(const std::string& strMessage, const std::vector<unsigned char>& vchSig, CKeyID& keyID)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    keyID = pubkey.GetID();
    return true;
}
}

#endif // UTIL_SIGNSTR_H

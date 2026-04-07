#include <util/tokenstr.h>

#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

dev::s256 powBase10(unsigned int exponent)
{
    if(exponent == 0)
        return 0;
    dev::s256 power = 1;
    for(unsigned int i = 0; i < exponent; i++){
        power *= 10;
    }
    return power;
}

std::string FormatToken(const unsigned int& decimals, const dev::s256& n)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    dev::s256 coin = powBase10(decimals);
    dev::s256 n_abs = (n > 0 ? n : -n);
    dev::s256 quotient = n_abs/coin;
    dev::s256 remainder = n_abs%coin;
    std::string strQuotient = quotient.str();
    std::string strRemainder = remainder.str();
    while(strRemainder.size() < decimals)
        strRemainder.insert(0, 1, '0');
    std::string str = strprintf("%s.%s", strQuotient, strRemainder);

    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    return str;
}


bool ParseToken(const unsigned int& decimals, const std::string& token_string, dev::s256& nRet)
{
    if (!util::ContainsNoNUL(token_string)) {
        return false;
    }
    const std::string str = util::TrimString(token_string);
    if (str.empty()) {
        return false;
    }

    dev::s256 coin = powBase10(decimals);
    std::string strWhole;
    dev::s256 nUnits = 0;
    const char* p = str.c_str();
    for (; *p; p++)
    {
        if (*p == '.')
        {
            p++;
            dev::s256 nMult = coin / 10;
            while (IsDigit(*p) && (nMult > 0))
            {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }
            break;
        }
        if (IsSpace(*p))
            return false;
        if (!IsDigit(*p))
            return false;
        strWhole.insert(strWhole.end(), *p);
    }
    if (*p) {
        return false;
    }
    if (strWhole.size() > (77 - decimals)) // guard against 256 bit overflow
        return false;
    if (nUnits < 0 || nUnits > coin)
        return false;
    dev::s256 nWhole = dev::s256(strWhole);
    dev::s256 nValue = nWhole*coin + nUnits;

    nRet = nValue;
    return true;
}

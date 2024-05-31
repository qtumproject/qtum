#ifndef CONTRACTUTIL_H
#define CONTRACTUTIL_H
#include <util/contractabi.h>
#include <QRegularExpression>
#include <QStringList>
#include <QString>
#include <QMap>

#include <string>
#include <map>

#define paternUint "^[0-9]{1,77}$"
#define paternInt "^\\-{0,1}[0-9]{1,76}$"
#define paternAddress "^[a-fA-F0-9]{40,40}$"
#define paternBool "^true$|^false$"
#define paternHex "^[a-fA-F0-9]{1,}$"
#define paternBytes paternHex
#define paternBytes32 "^[a-fA-F0-9]{%1,%1}$"

class ContractUtil
{
public:
    static bool getRegularExpession(const ParameterType &paramType, QRegularExpression &regEx);
    static QString errorMessage(const FunctionABI& function, const std::vector<ParameterABI::ErrorType>& errors, bool in);
    static QMap<QString, QString> fromStdMap(const std::map<std::string, std::string>& fromMap);
};

#endif // CONTRACTUTIL_H

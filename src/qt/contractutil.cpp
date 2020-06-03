#include <qt/contractutil.h>

QString ContractUtil::errorMessage(const FunctionABI& function, const std::vector<ParameterABI::ErrorType> &errors, bool in)
{
    if(in && errors.size() != function.inputs.size())
        return "";
    if(!in && errors.size() != function.outputs.size())
        return "";
    const std::vector<ParameterABI>& params = in ? function.inputs : function.outputs;

    QStringList messages;
    messages.append(QObject::tr("ABI parsing error:"));
    for(size_t i = 0; i < errors.size(); i++)
    {
        ParameterABI::ErrorType err = errors[i];
        if(err == ParameterABI::Ok) continue;
        const ParameterABI& param = params[i];
        QString _type = QString::fromStdString(param.type);
        QString _name = QString::fromStdString(param.name);

        switch (err) {
        case ParameterABI::UnsupportedABI:
            messages.append(QObject::tr("Unsupported type %1 %2.").arg(_type, _name));
            break;
        case ParameterABI::EncodingError:
            messages.append(QObject::tr("Error encoding parameter %1 %2.").arg(_type, _name));
            break;
        case ParameterABI::DecodingError:
            messages.append(QObject::tr("Error decoding parameter %1 %2.").arg(_type, _name));
            break;
        default:
            break;
        }
    }
    return messages.join('\n');
}

bool ContractUtil::getRegularExpession(const ParameterType &paramType, QRegularExpression &regEx)
{
    bool ret = false;
    switch (paramType.type()) {
    case ParameterType::abi_bytes:
    {
        if(paramType.isDynamic())
        {
            regEx.setPattern(paternBytes);
        }
        else
        {
            // Expression to check the number of bytes encoded in hex (1-32)
            regEx.setPattern(QString(paternBytes32).arg(paramType.totalBytes()*2));
        }
        ret = true;
        break;
    }
    case ParameterType::abi_uint:
    {
        regEx.setPattern(paternUint);
        ret = true;
        break;
    }
    case ParameterType::abi_int:
    {
        regEx.setPattern(paternInt);
        ret = true;
        break;
    }
    case ParameterType::abi_address:
    {
        regEx.setPattern(paternAddress);
        ret = true;
        break;
    }
    case ParameterType::abi_bool:
    {
        regEx.setPattern(paternBool);
        ret = true;
        break;
    }
    default:
    {
        ret = false;
        break;
    }
    }
    return ret;
}

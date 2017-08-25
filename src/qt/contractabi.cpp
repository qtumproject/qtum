#include "contractabi.h"

ContractABI::ContractABI()
{}

bool ContractABI::loads(const std::string &json_data)
{
    clean();

    // Not implemented

    return false;
}

void ContractABI::clean()
{
    functions.clear();
}

FunctionABI::FunctionABI(const std::string &_name,
                         const std::string &_type,
                         const std::vector<ParameterABI> &_inputs,
                         const std::vector<ParameterABI> &_outputs,
                         bool _payable, bool _constant, bool _anonymous):
    name(_name),
    type(_type),
    inputs(_inputs),
    outputs(_outputs),
    payable(_payable),
    constant(_constant),
    anonymous(_anonymous)
{}

bool FunctionABI::abiIn(const std::vector<std::string> &values, std::string &data)
{
    bool ret = inputs.size() == values.size();
    data = selector();
    for(size_t i = 0; (i < inputs.size() || !ret); i++)
    {
        ret &= inputs[i].abiIn(values[i], data);
    }
    return ret;
}

bool FunctionABI::abiOut(const std::string &data, std::vector<std::string> &values)
{
    size_t pos = 0;
    bool ret = true;
    for(size_t i = 0; (i < outputs.size() || !ret); i++)
    {
        std::string value;
        ret &= outputs[i].abiOut(data, pos, value);
        values.push_back(value);
    }
    return ret;
}

std::string FunctionABI::selector()
{
    // Not implemented
    return "";
}

ParameterABI::ParameterABI(const std::string &_name, const std::string &_type, bool _indexed):
    name(_name),
    type(_type),
    indexed(_indexed)
{}

bool ParameterABI::abiIn(const std::string &value, std::string &data)
{
    // Not implemented
    return false;
}

bool ParameterABI::abiOut(const std::string &data, size_t &pos, std::string &value)
{
    // Not implemented
    return false;
}

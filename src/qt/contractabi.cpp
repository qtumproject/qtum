#include "contractabi.h"
#include "univalue.h"
#include "libethcore/ABI.h"

// Defining json preprocessor functions in order to avoid repetitive code with slight difference
#define ReadJsonString(json, param, result) if(json.exists(#param) && json[#param].isStr())\
                                                result.param = json[#param].get_str();
#define ReadJsonBool(json, param, result) if(json.exists(#param) && json[#param].isBool())\
                                              result.param = json[#param].get_bool();
#define ReadJsonArray(json, param, result) if(json.exists(#param) && json[#param].isArray())\
                                              result = json[#param].get_array();

ContractABI::ContractABI()
{}

bool ContractABI::loads(const std::string &json_data)
{
    clean();

    UniValue json_contract;
    bool ret = json_contract.read(json_data);
    if(ret && json_contract.isArray())
    {
        // Read all functions from the contract
        size_t size = json_contract.size();
        for(size_t i = 0; i < size; i++)
        {
            const UniValue& json_function = json_contract[i];
            FunctionABI function;
            ReadJsonString(json_function, name, function);
            ReadJsonString(json_function, type, function);
            ReadJsonBool(json_function, payable, function);
            ReadJsonBool(json_function, constant, function);
            ReadJsonBool(json_function, anonymous, function);

            UniValue json_inputs;
            ReadJsonArray(json_function, inputs, json_inputs);
            for(size_t j = 0; j < json_inputs.size(); j++)
            {
                const UniValue& json_param = json_inputs[j];
                ParameterABI param;
                ReadJsonString(json_param, name, param);
                ReadJsonString(json_param, type, param);
                function.inputs.push_back(param);
            }

            UniValue json_outputs;
            ReadJsonArray(json_function, outputs, json_outputs);
            for(size_t j = 0; j < json_outputs.size(); j++)
            {
                const UniValue& json_param = json_outputs[j];
                ParameterABI param;
                ReadJsonString(json_param, name, param);
                ReadJsonString(json_param, type, param);
                ReadJsonBool(json_param, indexed, param);
                function.outputs.push_back(param);
            }

            functions.push_back(function);
        }
    }

    return ret;
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

bool FunctionABI::abiIn(const std::vector<std::string> &values, std::string &data) const
{
    bool ret = inputs.size() == values.size();
    data = selector();
    for(size_t i = 0; (i < inputs.size() || !ret); i++)
    {
        ret &= inputs[i].abiIn(values[i], data);
    }
    return ret;
}

bool FunctionABI::abiOut(const std::string &data, std::vector<std::string> &values) const
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

std::string FunctionABI::selector() const
{
    if(type == "constructor")
    {
        return "";
    }

    std::stringstream id;
    id << name;
    id << "(";
    if(inputs.size() > 0)
    {
        id << inputs[0].type;
    }
    for(size_t i = 1; i < inputs.size(); i++)
    {
        id << "," << inputs[i].type;
    }
    id << ")";
    std::string sig = id.str();
    dev::bytes hash = dev::sha3(sig).ref().cropped(0, 4).toBytes();

    return dev::toHex(hash);
}

ParameterABI::ParameterABI(const std::string &_name, const std::string &_type, bool _indexed):
    name(_name),
    type(_type),
    indexed(_indexed)
{}

bool ParameterABI::abiIn(const std::string &value, std::string &data) const
{
    // Not implemented
    return false;
}

bool ParameterABI::abiOut(const std::string &data, size_t &pos, std::string &value) const
{
    // Not implemented
    return false;
}

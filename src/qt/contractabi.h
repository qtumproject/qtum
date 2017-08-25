#ifndef CONTRACTABI_H
#define CONTRACTABI_H
#include <string>
#include <vector>

class ParameterABI
{
public:
    ParameterABI(const std::string& _name, const std::string& _type, bool _indexed);
    bool abiIn(const std::string &value, std::string &data);
    bool abiOut(const std::string &data, size_t& pos, std::string &value);

    std::string name; // The name of the parameter;
    std::string type; // The canonical type of the parameter.
    bool indexed; // True if the field is part of the log's topics, false if it one of the log's data segment.
    // Indexed is only used with event function
};

class FunctionABI
{
public:
    FunctionABI(const std::string& _name,
                const std::string& _type = std::string(),
                const std::vector<ParameterABI>& _inputs = std::vector<ParameterABI>(),
                const std::vector<ParameterABI>& _outputs = std::vector<ParameterABI>(),
                bool _payable = false,
                bool _constant = false,
                bool _anonymous = false);

    bool abiIn(const std::vector<std::string>& values, std::string& data);

    bool abiOut(const std::string& data, std::vector<std::string>& values);

    std::string selector();

    std::string name; // The name of the function;
    std::string type; // Function types: "function", "constructor", "fallback" or "event"
    std::vector<ParameterABI> inputs; // Array of input parameters
    std::vector<ParameterABI> outputs; // Array of output parameters, can be omitted if function doesn't return
    bool payable; // True if function accepts ether, defaults to false.
    bool constant; // True if function is specified to not modify blockchain state.
    bool anonymous; // True if the event was declared as anonymous.

    // Constructor and fallback function never have name or outputs.
    // Fallback function doesn't have inputs either.
    // Event function is the only one that have anonymous.
    // Sending non-zero ether to non-payable function will throw.
    // Type can be omitted, defaulting to "function".
};

class ContractABI
{
public:
    ContractABI();
    bool loads(const std::string& json_data);
    void clean();

    std::vector<FunctionABI> functions;
};

#endif // CONTRACTABI_H

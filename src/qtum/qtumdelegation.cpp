#include <qtum/qtumdelegation.h>
#include <chainparams.h>
#include <util/contractabi.h>
#include <util/convert.h>
#include <validation.h>
#include <util/signstr.h>

const std::string strDelegationsABI = "[{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_delegate\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"uint8\",\"name\":\"fee\",\"type\":\"uint8\"},{\"indexed\":false,\"internalType\":\"uint256\",\"name\":\"blockHeight\",\"type\":\"uint256\"},{\"indexed\":false,\"internalType\":\"bytes\",\"name\":\"PoD\",\"type\":\"bytes\"}],\"name\":\"AddDelegation\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_delegate\",\"type\":\"address\"}],\"name\":\"RemoveDelegation\",\"type\":\"event\"},{\"constant\":false,\"inputs\":[{\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"_fee\",\"type\":\"uint8\"},{\"internalType\":\"bytes\",\"name\":\"_PoD\",\"type\":\"bytes\"}],\"name\":\"addDelegation\",\"outputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"name\":\"delegations\",\"outputs\":[{\"internalType\":\"address\",\"name\":\"staker\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"fee\",\"type\":\"uint8\"},{\"internalType\":\"uint256\",\"name\":\"blockHeight\",\"type\":\"uint256\"},{\"internalType\":\"bytes\",\"name\":\"PoD\",\"type\":\"bytes\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[],\"name\":\"removeDelegation\",\"outputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]";
const ContractABI contractDelegationABI = strDelegationsABI;

class QtumDelegationPriv
{
public:
    QtumDelegationPriv():
        m_pfDelegations(0)
    {
        // Initialize parameters
        delegationsAddress = uintToh160(Params().GetConsensus().delegationsAddress);

        // Get the ABI for the functions
        for(const FunctionABI& func : contractDelegationABI.functions)
        {
            if(func.name == "delegations" && m_pfDelegations == 0)
                m_pfDelegations = new FunctionABI(func);
        }
        assert(m_pfDelegations);
    }

    virtual ~QtumDelegationPriv()
    {
        if(m_pfDelegations)
            delete m_pfDelegations;
        m_pfDelegations = 0;
    }

    FunctionABI* m_pfDelegations;
    dev::Address delegationsAddress;
};

QtumDelegation::QtumDelegation():
    priv(0)
{
    priv = new QtumDelegationPriv();
}

QtumDelegation::~QtumDelegation()
{
    if(priv)
        delete priv;
    priv = 0;
}

bool QtumDelegation::GetDelegation(const uint160 &address, Delegation &delegation) const
{
    // Contract exist check
    if(!globalState->addressInUse(priv->delegationsAddress))
        return error("Delegation contract address does not exist");

    // Get delegation ABI check
    if(!priv->m_pfDelegations)
        return error("Get delegation ABI does not exist");

    // Serialize the input parameters for get delegation
    std::vector<std::vector<std::string>> inputValues;
    std::vector<std::string> paramAddress;
    paramAddress.push_back(address.GetReverseHex());
    inputValues.push_back(paramAddress);
    std::vector<ParameterABI::ErrorType> inputErrors;
    std::string inputData;
    if(priv->m_pfDelegations->abiIn(inputValues, inputData, inputErrors))
        return error("Failed to serialize get delegation input parameters");

    // Get delegation for address
    std::vector<ResultExecute> execResults = CallContract(priv->delegationsAddress, ParseHex(inputData));
    if(execResults.size() < 1)
        return error("Failed to CallContract to get delegation for address");

    // Deserialize the output parameters for get delegation
    std::string outputData = HexStr(execResults[0].execRes.output);
    std::vector<std::vector<std::string>> outputValues;
    std::vector<ParameterABI::ErrorType> outputErrors;
    if(priv->m_pfDelegations->abiOut(outputData, outputValues, outputErrors))
        return error("Failed to deserialize get delegation output parameters");

    // Output parameters size check
    if(outputValues.size() != priv->m_pfDelegations->outputs.size())
        return error("Failed to deserialize get delegation outputs, size doesn't match");

    // Parse the values of the output parameters for get delegation
    try
    {
        for(size_t i = 0; i < outputValues.size(); i++)
        {
            std::vector<std::string> value = outputValues[i];
            if(value.size() < 1)
                return error("Failed to get delegation output value");

            std::string name = priv->m_pfDelegations->outputs[i].name;
            if(name == "staker")
            {
                delegation.staker = uint160(ParseHex(value[0]));
            }
            else if(name == "fee")
            {
                delegation.fee = (uint8_t)atoi64(value[0]);
            }
            else if(name == "blockHeight")
            {
                delegation.blockHeight = (uint)atoi64(value[0]);
            }
            else if(name == "PoD")
            {
                delegation.PoD = ParseHex(value[0]);
            }
            else
            {
                return error("Invalid get delegation output name");
            }
        }
    }
    catch(...)
    {
        return error("Parsing failed for get delegation outputs");
    }

    return true;
}

bool QtumDelegation::VerifyDelegation(const uint160 &address, const Delegation &delegation)
{
    if(address == uint160() || delegation.IsNull() || delegation.fee > 100)
        return false;

    return SignStr::VerifyMessage(CKeyID(address), delegation.staker.GetReverseHex(), delegation.PoD);
}

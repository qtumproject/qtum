#include <qtum/qtumdelegation.h>
#include <chainparams.h>
#include <util/contractabi.h>
#include <util/convert.h>
#include <validation.h>
#include <util/signstr.h>
#include <util/strencodings.h>
#include <libdevcore/Common.h>

const std::string strDelegationsABI = "[{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_delegate\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"uint8\",\"name\":\"fee\",\"type\":\"uint8\"},{\"indexed\":false,\"internalType\":\"uint256\",\"name\":\"blockHeight\",\"type\":\"uint256\"},{\"indexed\":false,\"internalType\":\"bytes\",\"name\":\"PoD\",\"type\":\"bytes\"}],\"name\":\"AddDelegation\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"indexed\":true,\"internalType\":\"address\",\"name\":\"_delegate\",\"type\":\"address\"}],\"name\":\"RemoveDelegation\",\"type\":\"event\"},{\"constant\":false,\"inputs\":[{\"internalType\":\"address\",\"name\":\"_staker\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"_fee\",\"type\":\"uint8\"},{\"internalType\":\"bytes\",\"name\":\"_PoD\",\"type\":\"bytes\"}],\"name\":\"addDelegation\",\"outputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"constant\":true,\"inputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"name\":\"delegations\",\"outputs\":[{\"internalType\":\"address\",\"name\":\"staker\",\"type\":\"address\"},{\"internalType\":\"uint8\",\"name\":\"fee\",\"type\":\"uint8\"},{\"internalType\":\"uint256\",\"name\":\"blockHeight\",\"type\":\"uint256\"},{\"internalType\":\"bytes\",\"name\":\"PoD\",\"type\":\"bytes\"}],\"payable\":false,\"stateMutability\":\"view\",\"type\":\"function\"},{\"constant\":false,\"inputs\":[],\"name\":\"removeDelegation\",\"outputs\":[],\"payable\":false,\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]";
const ContractABI contractDelegationABI = strDelegationsABI;
const size_t nPoDStartPosition = 131;

const ContractABI &DelegationABI()
{
    return contractDelegationABI;
}

bool AbiOutEvent(FunctionABI* func, const std::vector<std::string>& topics, const std::string& data, std::vector<std::vector<std::string>>& values)
{
    std::vector<ParameterABI::ErrorType> errors;
    values.clear();
    return func->abiOut(topics, data, values, errors);
}

class QtumDelegationPriv
{
public:
    QtumDelegationPriv():
        m_pfDelegations(0),
        m_pfAddDelegationEvent(0),
        m_pfRemoveDelegationEvent(0)
    {
        // Initialize parameters
        delegationsAddress = uintToh160(Params().GetConsensus().delegationsAddress);

        // Get the ABI for the functions
        for(const FunctionABI& func : contractDelegationABI.functions)
        {
            if(func.name == "delegations" && m_pfDelegations == 0)
            {
                m_pfDelegations = new FunctionABI(func);
            }
            else if(func.name == "AddDelegation" && m_pfAddDelegationEvent == 0)
            {
                m_pfAddDelegationEvent = new FunctionABI(func);
            }
            else if(func.name == "RemoveDelegation" && m_pfRemoveDelegationEvent == 0)
            {
                m_pfRemoveDelegationEvent = new FunctionABI(func);
            }

        }
        assert(m_pfDelegations);
        assert(m_pfAddDelegationEvent);
        assert(m_pfRemoveDelegationEvent);
    }

    virtual ~QtumDelegationPriv()
    {
        if(m_pfDelegations)
            delete m_pfDelegations;
        m_pfDelegations = 0;

        if(m_pfAddDelegationEvent)
            delete m_pfAddDelegationEvent;
        m_pfAddDelegationEvent = 0;

        if(m_pfRemoveDelegationEvent)
            delete m_pfRemoveDelegationEvent;
        m_pfRemoveDelegationEvent = 0;
    }

    bool GetDelegationEvent(const dev::eth::LogEntry& log, DelegationEvent& event) const
    {
        if(log.address != delegationsAddress)
            return false;

        std::vector<std::string> topics;
        for(dev::h256 topic : log.topics)
            topics.push_back(dev::toHex(topic));

        std::string data = dev::toHex(log.data);

        return GetDelegationEvent(topics, data, event);
    }

    bool GetDelegationEvent(const std::vector<std::string>& topics, const std::string& data, DelegationEvent& event) const
    {
        //  Get the event data
        std::vector<std::vector<std::string>> values;
        FunctionABI* func = m_pfRemoveDelegationEvent;
        DelegationType type = DelegationType::DELEGATION_REMOVE;
        bool ret = AbiOutEvent(func, topics, data, values);
        if(!ret)
        {
            func = m_pfAddDelegationEvent;
            type = DelegationType::DELEGATION_ADD;
            ret = AbiOutEvent(func, topics, data, values);
        }
        if(!ret)
        {
            return false;
        }


        // Parse the values of the input parameters for delegation event
        try
        {
            for(size_t i = 0; i < values.size(); i++)
            {
                std::vector<std::string> value = values[i];
                if(value.size() < 1)
                    return error("Failed to get delegation output value");

                std::string name = func->inputs[i].name;
                if(name == "_staker")
                {
                    event.item.staker = uint160(ParseHex(value[0]));
                }
                else if(name == "_delegate")
                {
                    event.item.delegate = uint160(ParseHex(value[0]));
                }
                else if(name == "fee")
                {
                    event.item.fee = (uint8_t)atoi64(value[0]);
                }
                else if(name == "blockHeight")
                {
                    event.item.blockHeight = (uint32_t)atoi64(value[0]);
                }
                else if(name == "PoD")
                {
                    event.item.PoD = ParseHex(value[0]);
                }
                else
                {
                    return error("Invalid delegation event input name");
                }
            }
        }
        catch(...)
        {
            return error("Parsing failed for delegation event inputs");
        }
        event.type = type;

        return true;
    }

    FunctionABI* m_pfDelegations;
    FunctionABI* m_pfAddDelegationEvent;
    FunctionABI* m_pfRemoveDelegationEvent;
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

bool QtumDelegation::GetDelegation(const uint160 &address, Delegation &delegation, CChainState& chainstate) const
{
    // Contract exist check
    if(!ExistDelegationContract())
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
    if(!priv->m_pfDelegations->abiIn(inputValues, inputData, inputErrors))
        return error("Failed to serialize get delegation input parameters");

    // Get delegation for address
    std::vector<ResultExecute> execResults;
    {
        LOCK(cs_main);
        execResults = CallContract(priv->delegationsAddress, ParseHex(inputData), chainstate);
    }
    if(execResults.size() < 1)
        return error("Failed to CallContract to get delegation for address");

    // Deserialize the output parameters for get delegation
    std::string outputData = HexStr(execResults[0].execRes.output);
    std::vector<std::vector<std::string>> outputValues;
    std::vector<ParameterABI::ErrorType> outputErrors;
    if(!priv->m_pfDelegations->abiOut(outputData, outputValues, outputErrors))
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
                delegation.blockHeight = (uint32_t)atoi64(value[0]);
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

bool QtumDelegation::FilterDelegationEvents(std::vector<DelegationEvent> &events, const IDelegationFilter &filter, ChainstateManager &chainman, int fromBlock, int toBlock, int minconf) const
{
    // Check if log events are enabled
    if(!fLogEvents)
        return error("Events indexing disabled");

    // Contract exist check
    if(!ExistDelegationContract())
        return error("Delegation contract address does not exist");

    // Add delegation event ABI check
    if(!priv->m_pfAddDelegationEvent)
        return error("Add delegation event ABI does not exist");

    // Remove delegation event ABI check
    if(!priv->m_pfRemoveDelegationEvent)
        return error("Remove delegation ABI does not exist");

    LOCK(cs_main);
    int curheight = 0;
    std::set<dev::h160> addresses;
    addresses.insert(priv->delegationsAddress);
    std::vector<std::vector<uint256>> hashesToBlock;
    curheight = pblocktree->ReadHeightIndex(fromBlock, toBlock, minconf, hashesToBlock, addresses, chainman);

    if (curheight == -1) {
        return error("Incorrect params");
    }

    // Search for delegation events
    std::set<uint256> dupes;
    for(const auto& hashesTx : hashesToBlock)
    {
        for(const auto& e : hashesTx)
        {

            if(dupes.find(e) != dupes.end()) {
                continue;
            }
            dupes.insert(e);

            std::vector<TransactionReceiptInfo> receipts = pstorageresult->getResult(uintToh256(e));
            for(const auto& receipt : receipts) {
                if(receipt.logs.empty()) {
                    continue;
                }

                for(const dev::eth::LogEntry& log : receipt.logs)
                {
                    DelegationEvent event;
                    if(priv->GetDelegationEvent(log, event) && filter.Match(event))
                    {
                        events.push_back(event);
                    }
                }
            }
        }
    }

    return true;
}

std::map<uint160, Delegation> QtumDelegation::DelegationsFromEvents(const std::vector<DelegationEvent> &events)
{
    std::map<uint160, Delegation> delegations;
    UpdateDelegationsFromEvents(events, delegations);
    return delegations;
}

void QtumDelegation::UpdateDelegationsFromEvents(const std::vector<DelegationEvent> &events, std::map<uint160, Delegation> &delegations)
{
    for(const DelegationEvent& event : events)
    {
        switch (event.type) {
        case DELEGATION_ADD:
        {
            delegations[event.item.delegate] = event.item;
            break;
        }
        case DELEGATION_REMOVE:
        {
            auto it = delegations.find(event.item.delegate);
            if (it != delegations.end())
              delegations.erase (it);
            break;
        }
        default:
            break;
        }
    }
}

bool QtumDelegation::ExistDelegationContract() const
{
    LOCK(cs_main);
    // Delegation contract exist check
    return globalState && globalState->addressInUse(priv->delegationsAddress);
}

std::string QtumDelegation::BytecodeRemove()
{
    return DelegationABI()["removeDelegation"].selector();
}

bool QtumDelegation::BytecodeAdd(const std::string &hexStaker, const int &fee, const std::vector<unsigned char> &PoD, std::string &datahex, std::string &errorMessage)
{
    FunctionABI func = DelegationABI()["addDelegation"];
    std::vector<std::vector<std::string>> values;
    std::vector<ParameterABI::ErrorType> errors;

    for(size_t i = 0; i < func.inputs.size(); i++)
    {
        std::string name = func.inputs[i].name;
        if(name == "_staker")
        {
            std::vector<std::string> value;
            value.push_back(hexStaker);
            values.push_back(value);
        }
        else if(name == "_fee")
        {
            std::vector<std::string> value;
            value.push_back(i64tostr(fee));
            values.push_back(value);
        }
        else if(name == "_PoD")
        {
            std::vector<std::string> value;
            value.push_back(HexStr(PoD));
            values.push_back(value);
        }
        else
        {
            errorMessage = "Invalid add delegation input name";
            return false;
        }
    }

    if(!func.abiIn(values, datahex, errors))
    {
        errorMessage = "Fail to serialize data for add delegation";
        return false;
    }

    return true;
}

bool QtumDelegation::IsAddBytecode(const std::vector<unsigned char> &data)
{
    // Quick check for is set delegate address
    size_t size = data.size();
    if(size < 228)
        return false;
    if(data[0] != 76 || data[1] != 14 || data[2] != 150 || data[3] != 140 || data[nPoDStartPosition] != CPubKey::COMPACT_SIGNATURE_SIZE)
        return false;
    return true;
}

bool QtumDelegation::GetUnsignedStaker(const std::vector<unsigned char> &data, std::string &hexStaker)
{
    if(!IsAddBytecode(data))
        return false;

    // Init variables
    size_t from = nPoDStartPosition + 1;
    size_t to = from + CPubKey::COMPACT_SIGNATURE_SIZE;
    size_t stakerSize = 40;
    size_t remainSize = CPubKey::COMPACT_SIGNATURE_SIZE - stakerSize;
    std::string strStaker;
    std::string strRemain;

    strStaker.reserve(stakerSize);
    strRemain.reserve(remainSize);

    // Get unsigned staker address from PoD
    for(size_t i = from; i < to; i++)
    {
        char c = (char)data[i];
        if(strStaker.size() < stakerSize)
        {
            strStaker.push_back(c);
        }
        else
        {
            strRemain.push_back(c);
        }
    }

    // Check formatting
    if(IsHex(strStaker) && !IsHex(strRemain))
    {
        hexStaker = strStaker;
        return true;
    }

    return false;
}

bool QtumDelegation::SetSignedStaker(std::vector<unsigned char> &data, const std::string &base64PoD)
{
    if(!IsAddBytecode(data))
        return false;

    bool invalid = false;
    std::string strPoD = DecodeBase64(base64PoD, &invalid);
    if(invalid || strPoD.size() < CPubKey::COMPACT_SIGNATURE_SIZE)
        return false;

    size_t offset = nPoDStartPosition + 1;
    for(size_t i = 0; i < CPubKey::COMPACT_SIGNATURE_SIZE; i++)
    {
        data[offset + i] = strPoD[i];
    }

    return true;
}

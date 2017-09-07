# helpers for qtum
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *

# Default min gas price in satoshis
QTUM_MIN_GAS_PRICE = 40

# For the dgp
BLOCKS_BEFORE_PROPOSAL_EXPIRATION = 216

def make_vin(node, value):
    addr = node.getrawchangeaddress()
    txid_hex = node.sendtoaddress(addr, value/COIN)
    txid = int(txid_hex, 16)
    node.generate(1)
    raw_tx = node.getrawtransaction(txid_hex, 1)

    for vout_index, txout in enumerate(raw_tx['vout']):
        if txout['scriptPubKey']['addresses'] == [addr]:
            break
    else:
        assert False

    return CTxIn(COutPoint(txid, vout_index), nSequence=0)

def make_op_call_output(value, version, gas_limit, gas_price, data, contract):
    scriptPubKey = CScript()
    scriptPubKey += version
    scriptPubKey += gas_limit
    scriptPubKey += gas_price
    scriptPubKey += data
    scriptPubKey += contract
    scriptPubKey += OP_CALL
    return CTxOut(value, scriptPubKey)

def make_transaction(node, vin, vout):
    tx = CTransaction()
    tx.vin = vin
    tx.vout = vout
    tx.rehash()
    
    unsigned_raw_tx = bytes_to_hex_str(tx.serialize_without_witness())
    signed_raw_tx = node.signrawtransaction(unsigned_raw_tx)['hex']
    return signed_raw_tx

def make_op_create_output(node, value, version, gas_limit, gas_price, data):
    scriptPubKey = CScript()
    scriptPubKey += version
    scriptPubKey += gas_limit
    scriptPubKey += gas_price
    scriptPubKey += data
    scriptPubKey += OP_CREATE
    return CTxOut(value, scriptPubKey)

def assert_vout(tx, expected_vout):
    assert_equal(len(tx['vout']), len(expected_vout))
    matches = []
    for expected in expected_vout:
        for i in range(len(tx['vout'])):
            if i not in matches and expected[0] == tx['vout'][i]['value'] and expected[1] == tx['vout'][i]['scriptPubKey']['type']:
                matches.append(i)
                break
    assert_equal(len(matches), len(expected_vout))

def assert_vin(tx, expected_vin):
    assert_equal(len(tx['vin']), len(expected_vin))
    matches = []
    for expected in expected_vin:
        for i in range(len(tx['vin'])):
            if i not in matches and expected[0] == tx['vin'][i]['scriptSig']['asm']:
                matches.append(i)
                break
    assert_equal(len(matches), len(expected_vin))


def read_evm_array(node, address, abi, ignore_nulls=True):
    arr = []
    index = 0
    ret = node.callcontract(address, abi + hex(index)[2:].zfill(64))
    while ret['executionResult']['excepted'] == 'None':
        if int(ret['executionResult']['output'], 16) != 0 or not ignore_nulls:
            arr.append(ret['executionResult']['output'])
        index += 1
        ret = node.callcontract(address, abi + hex(index)[2:].zfill(64))
    return arr

def p2pkh_to_hex_hash(address):
    return str(base58_to_byte(address, 25)[1])[2:-1]

def hex_hash_to_p2pkh(hex_hash):
    return keyhash_to_p2pkh(hex_str_to_bytes(hex_hash))    


class DGPState:
    def __init__(self, node, contract_address):
        self.last_state_assert_block_height = 0
        self.node = node
        self.contract_address = contract_address
        self.param_count = 0
        self.gov_keys = []
        self.admin_keys = []
        self.params_for_block = []
        self.param_address_at_indices = []
        self.required_votes = [0, 0, 0]
        self.current_on_vote_statuses = [
            [False, False, False],
            [False, False, False],
            [False, False]
        ]
        self.current_on_vote_address_proposals = [
            ["0", "0", "0"],
            ["0", "0"]
        ]
        self.abiAddAddressProposal = "bf5f1e83" #addAddressProposal(address,uint256)
        self.abiRemoveAddressProposal = "4cc0e2bc" # removeAddressProposal(address,uint256)
        self.abiChangeValueProposal = "19971cbd" # changeValueProposal(uint256,uint256)
        self.abiAlreadyVoted = "e9944a81" # alreadyVoted(address,address[])
        self.abiGetAddressesList = "850d9758" # getAddressesList(uint256)
        self.abiGetArrayNonNullLength = "9b216626" # getArrayNonNullLenght(address[])
        self.abiGetCurrentOnVoteAddressProposal = "0c83ebac" # getCurrentOnVoteAddressProposal(uint256,uint256)
        self.abiGetCurrentOnVoteStatus = "5f302e8b" # getCurrentOnVoteStatus(uint256,uint256)
        self.abiGetCurrentOnVoteValueProposal = "4364725c" # getCurrentOnVoteValueProposal(uint256)
        self.abiGetCurrentOnVoteVotes = "f9f51401" # getCurrentOnVoteVotes(uint256,uint256)
        self.abiGetParamAddressAtIndex = "15341747" # getParamAddressAtIndex(uint256)
        self.abiGetParamCount = "27e35746" # getParamCount()
        self.abiGetParamHeightAtIndex = "8a5a9d07" # getParamHeightAtIndex(uint256)
        self.abiGetParamsForBlock = "f769ac48" # getParamsForBlock(uint256)
        self.abiGetRequiredVotes = "1ec28e0f" # getRequiredVotes(uint256)
        self.abiIsAdminKey = "6b102c49" # isAdminKey(address)
        self.abiIsGovKey = "7b993bf3" # isGovKey(address)
        self.abiSetInitialAdmin = "6fb81cbb" # setInitialAdmin()
        self.abiTallyAdminVotes = "bec171e5" # tallyAdminVotes(address[])
        self.abiTallyGovVotes = "4afb4f11" # tallyGovVotes(address[])
        self.abiGovKeys = "30a79873" # govKeys(uint256)
        self.abiAdminKeys = "aff125f6" # adminKeys(uint256)

    def send_set_initial_admin(self, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiSetInitialAdmin, 0, 2000000, QTUM_MIN_GAS_PRICE/COIN, sender)

    def send_add_address_proposal(self, proposal_address, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiAddAddressProposal + proposal_address.zfill(64) + hex(type1)[2:].zfill(64), 0, 2000000, QTUM_MIN_GAS_PRICE/COIN, sender)

    def send_remove_address_proposal(self, proposal_address, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiRemoveAddressProposal + proposal_address.zfill(64) + hex(type1)[2:].zfill(64), 0, 2000000, QTUM_MIN_GAS_PRICE/COIN, sender)

    def send_change_value_proposal(self, uint_proposal, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiChangeValueProposal + hex(uint_proposal)[2:].zfill(64) + hex(type1)[2:].zfill(64), 0, 2000000, QTUM_MIN_GAS_PRICE/COIN, sender)

    def assert_state(self):
        # This assertion is only to catch potential errors in the test code (if we forget to add a generate after an evm call)
        assert(self.last_state_assert_block_height < self.node.getblockcount())
        self.last_state_assert_block_height = self.node.getblockcount()

        self._assert_param_count(self.param_count)
        self._assert_gov_keys_equal(self.gov_keys)
        self._assert_admin_keys_equal(self.admin_keys)
        for block_height, param_for_block in self.params_for_block:
            self._assert_params_for_block(block_height, param_for_block)
        # Make sure that there are no subsequent params for blocks
        if self.params_for_block:
            ret = self.node.callcontract(self.contract_address, self.abiGetParamsForBlock + hex(0x2fff)[2:].zfill(64))
            assert_equal(int(ret['executionResult']['output'], 16), int(param_for_block, 16))
        else:
            ret = self.node.callcontract(self.contract_address, self.abiGetParamsForBlock + hex(0x2fff)[2:].zfill(64))
            assert_equal(int(ret['executionResult']['output'], 16), 0)


        for index, param_address_at_index in enumerate(self.param_address_at_indices):
            self._assert_param_address_at_index(index, param_address_at_index)
        # Make sure that there are no subsequent params at the next index
        if self.param_address_at_indices:
            ret = self.node.callcontract(self.contract_address, self.abiGetParamAddressAtIndex + hex(index+1)[2:].zfill(64))
            assert(ret['executionResult']['excepted'] != 'None')
        else:
            ret = self.node.callcontract(self.contract_address, self.abiGetParamAddressAtIndex + hex(0x0)[2:].zfill(64))
            assert(ret['executionResult']['excepted'] != 'None')


        for type1, required_votes in enumerate(self.required_votes):
            self._assert_required_votes(type1, required_votes)
        for type1, arr1 in enumerate(self.current_on_vote_statuses):
            for type2, current_on_vote_status in enumerate(arr1):
                self._assert_current_on_vote_status(type1, type2, current_on_vote_status)
        for type1, arr1 in enumerate(self.current_on_vote_address_proposals):
            for type2, current_on_vote_address_proposal in enumerate(arr1):
                self._assert_current_on_vote_address_proposal(type1, type2, current_on_vote_address_proposal)
                
    """
    function getRequiredVotes(uint _type) constant returns (uint val){
        // type 0: adminVotesForParams
        // type 1: govVotesForParams
        // type 2: adminVotesForManagement
        if(_type>2) throw; // invalid type
        if(_type==0)return activeVotesRequired.adminVotesForParams;
        if(_type==1)return activeVotesRequired.govVotesForParams;
        if(_type==2)return activeVotesRequired.adminVotesForManagement;
    }
    """
    def _assert_required_votes(self, type1, expected_required_votes):
        ret = self.node.callcontract(self.contract_address, self.abiGetRequiredVotes + str(type1).zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), expected_required_votes)

    """
   function getCurrentOnVoteStatus(uint _type, uint _type2) constant returns (bool val){
        // type 0: addAddress
        // type 1: changeValue
        // type 2: removeAddress    

        // type2 0: adminKey
        // type2 1: govKey
        // type2 2: paramsAddress

        if(_type>2 || _type2>2) throw; // invalid type
        if(_type==0)return currentProposals.keys[_type2].onVote;
        if(_type==1)return currentProposals.uints[_type2].onVote;
        if(_type==2)return currentProposals.removeKeys[_type2].onVote;
    }
    """
    def _assert_current_on_vote_status(self, type1, type2, expected_current_on_vote_status):
        ret = self.node.callcontract(self.contract_address, self.abiGetCurrentOnVoteStatus + str(type1).zfill(64) + str(type2).zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), expected_current_on_vote_status)

    """
    function getCurrentOnVoteAddressProposal(uint _type, uint _type2) constant returns (address val){
        // type 0: addAddress
        // type 1: removeAddress

        // type2 0: adminKey
        // type2 1: govKey
        // type2 2: paramsAddress

        if(_type>1 || _type2>2) throw; // invalid type
        if(_type==0)return currentProposals.keys[_type2].proposal;
        if(_type==1)return currentProposals.removeKeys[_type2].proposal;
    }
    """
    def _assert_current_on_vote_address_proposal(self, type1, type2, expected_address):
        ret = self.node.callcontract(self.contract_address, self.abiGetCurrentOnVoteAddressProposal + hex(type1)[2:].zfill(64) + hex(type2)[2:].zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), int(expected_address, 16))

    """
    function getCurrentOnVoteValueProposal(uint _type) constant returns (uint val){
        // type 0: adminVotesForParams
        // type 1: govVotesForParams
        // type 2: adminVotesForManagement

        if(_type>2) throw; // invalid type
        return currentProposals.uints[_type].proposal;
    }
    """
    def _assert_current_on_vote_value_proposal(self, type1, expected_proposal):
        ret = self.node.callcontract(self.contract_address, self.abiGetCurrentOnVoteValueProposal + hex(type1)[2:].zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), expected_proposal)

    """
    function getParamsForBlock(uint _reqBlockHeight) constant returns (address paramsAddress){
        uint i;
        for(i=paramsHistory.length-1;i>0;i--){
            if(paramsHistory[i].blockHeight<=_reqBlockHeight)return paramsHistory[i].paramsAddress;
        }
        if(paramsHistory[0].blockHeight<=_reqBlockHeight)return paramsHistory[0].paramsAddress;
        return 0;
    }
    """
    def _assert_params_for_block(self, required_block_height, expected_param_address):
        ret = self.node.callcontract(self.contract_address, self.abiGetParamsForBlock + hex(required_block_height)[2:].zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), int(expected_param_address, 16))

    """
    function getParamAddressAtIndex(uint _paramIndex) constant returns (address paramsAddress){
        return paramsHistory[_paramIndex].paramsAddress;
    }
    """
    def _assert_param_address_at_index(self, param_index, expected_param_address):
        ret = self.node.callcontract(self.contract_address, self.abiGetParamAddressAtIndex + hex(param_index)[2:].zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), int(expected_param_address, 16))


    """
    function getParamHeightAtIndex(uint _paramIndex) constant returns (uint paramsHeight){
        return paramsHistory[_paramIndex].blockHeight;
    }
    """
    def _assert_param_block_height_at_index(self, param_index, expected_block_height):
        ret = self.node.callcontract(self.contract_address, self.abiGetParamHeightAtIndex + hex(param_index)[2:].zfill(64))
        assert_equal(int(ret['executionResult']['output'], 16), expected_block_height)

    """
    function getParamCount() constant returns (uint paramsCount){
        return paramsHistory.length;
    }
    """
    def _assert_param_count(self, expected_param_count):
        ret = self.node.callcontract(self.contract_address, self.abiGetParamCount)
        assert_equal(int(ret['executionResult']['output'], 16), expected_param_count)

    def _assert_gov_keys_equal(self, expected_gov_keys):
        real_gov_keys = read_evm_array(self.node, self.contract_address, self.abiGovKeys)
        assert_equal(len(real_gov_keys), len(expected_gov_keys))
        for real, expected in zip(real_gov_keys, expected_gov_keys):
            assert_equal(int(real, 16), int(expected, 16))

    def _assert_admin_keys_equal(self, expected_admin_keys):
        real_admin_keys = read_evm_array(self.node, self.contract_address, self.abiAdminKeys)
        assert_equal(len(real_admin_keys), len(expected_admin_keys))
        for real, expected in zip(real_admin_keys, expected_admin_keys):
            assert_equal(int(real, 16), int(expected, 16))

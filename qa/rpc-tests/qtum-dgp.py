#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.qtum import *
from test_framework.address import *
import sys
import time


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
        self.node.sendtocontract(self.contract_address, self.abiSetInitialAdmin, 0, 500000000, 0.00000001, sender)

    def send_add_address_proposal(self, proposal_address, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiAddAddressProposal + proposal_address.zfill(64) + hex(type1)[2:].zfill(64), 0, 500000000, 0.00000001, sender)

    def send_remove_address_proposal(self, proposal_address, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiRemoveAddressProposal + proposal_address.zfill(64) + hex(type1)[2:].zfill(64), 0, 500000000, 0.00000001, sender)

    def send_change_value_proposal(self, uint_proposal, type1, sender):
        self.node.sendtoaddress(sender, 1)
        self.node.sendtocontract(self.contract_address, self.abiChangeValueProposal + hex(uint_proposal)[2:].zfill(64) + hex(type1)[2:].zfill(64), 0, 500000000, 0.00000001, sender)

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
            assert(ret['executionResult']['excepted'] != 'None')


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




class QtumDGPTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-txindex=1']])
        self.node = self.nodes[0]
        self.is_network_split = False

    def create_dgp_contract(self):
        """
        pragma solidity ^0.4.11;

        contract dgp{

            struct paramsInstance{
                uint blockHeight;
                address paramsAddress;
            }

            paramsInstance[] paramsHistory;
            address[] public adminKeys;
            address[] public govKeys; 
            uint private maxKeys=30;
            bool private initialAdminSet=false;

            struct addressProposal{
                bool onVote;
                address[] votes;
                address proposal;
            }

            struct uintProposal{
                bool onVote;
                address[] votes;
                uint proposal;
            }

            struct proposals{
                mapping(uint=>addressProposal) keys;
                mapping(uint=>uintProposal) uints;
                mapping(uint=>addressProposal) removeKeys;
            }

            struct votesRequired{
                uint adminVotesForParams;
                uint govVotesForParams;
                uint adminVotesForManagement;
            }

            proposals currentProposals;
            votesRequired activeVotesRequired;

            modifier onlyAdmin{
                require(isAdminKey(msg.sender));
                _;
            }

            modifier onlyAdminOrGov{
                require(isAdminKey(msg.sender) || isGovKey(msg.sender));
                _;
            }

            function setInitialAdmin(){
                if(initialAdminSet)throw; // call only once
                adminKeys.push(msg.sender);
                initialAdminSet=true;
            }

            function addAddressProposal(address _proposalAddress, uint _type) onlyAdminOrGov{
                // type 0: adminKey
                // type 1: govKey
                // type 2: paramsAddress
                if(_type==0 && getArrayNonNullLenght(adminKeys)>=maxKeys) throw; // we have too many admin keys
                if(_type==1 && getArrayNonNullLenght(govKeys)>=maxKeys) throw; // we have too many gov keys
                if(_proposalAddress==0) throw; // invalid address
                if(_type>2) throw; // invalid type
                if((_type==0 || _type==1) && (isAdminKey(_proposalAddress) || isGovKey(_proposalAddress))) throw; // don't add existing keys as proposals
                if(!currentProposals.keys[_type].onVote){
                    if(isGovKey(msg.sender)) throw; // Only Admin can initiate vote
                    currentProposals.keys[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.keys[_type].proposal=_proposalAddress; // set new proposal for vote
                    currentProposals.keys[_type].votes.length=0; // clear votes
                    currentProposals.keys[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(currentProposals.keys[_type].proposal!=_proposalAddress) throw; // can only vote for current on vote address
                    if(alreadyVoted(msg.sender, currentProposals.keys[_type].votes)) throw; // cannot vote twice            
                    currentProposals.keys[_type].votes.push(msg.sender); // add sender vote
                }
                if(_type==0 || _type==1){
                    if(tallyAdminVotes(currentProposals.keys[_type].votes)>=activeVotesRequired.adminVotesForManagement){
                        if(isAdminKey(currentProposals.keys[_type].proposal) || isGovKey(currentProposals.keys[_type].proposal)) throw; // don't add existing keys
                        if(_type==0)adminKeys.push(currentProposals.keys[_type].proposal); // elected
                        if(_type==1)govKeys.push(currentProposals.keys[_type].proposal); // elected
                        clearAddressProposal(_type);
                    }
                }
                if(_type==2){
                    if(tallyAdminVotes(currentProposals.keys[_type].votes)>=activeVotesRequired.adminVotesForParams && tallyGovVotes(currentProposals.keys[_type].votes)>=activeVotesRequired.govVotesForParams){
                        if(paramsHistory.length>0 && paramsHistory[paramsHistory.length-1].blockHeight==block.number+1) throw; // don't add activate params on a height having existing params
                        paramsHistory.push(paramsInstance(block.number+1,currentProposals.keys[_type].proposal)); // save params activation block and address               
                        clearAddressProposal(_type);
                    }
                }
            }

            function removeAddressProposal(address _proposalAddress, uint _type) onlyAdmin{
                // type 0: adminKey
                // type 1: govKey
                if(_proposalAddress==0) throw; // invalid address
                if(_type>1) throw; // invalid type
                if(_type==0){
                uint adminsCount=getArrayNonNullLenght(adminKeys);
                if(adminsCount==activeVotesRequired.adminVotesForParams || adminsCount==activeVotesRequired.adminVotesForManagement) throw; // cannot reduce the number of admins below the required ones
                if(!isAdminKey(_proposalAddress)) throw; // don't remove non existent address
                }
                if(_type==1){
                if(getArrayNonNullLenght(govKeys)==activeVotesRequired.govVotesForParams) throw; // cannot reduce the number of govs below the required ones
                if(!isGovKey(_proposalAddress)) throw; // don't remove non existent address
                }
                if(!currentProposals.removeKeys[_type].onVote){
                    currentProposals.removeKeys[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.removeKeys[_type].proposal=_proposalAddress; // set new proposal for vote
                    currentProposals.removeKeys[_type].votes.length=0; // clear votes
                    currentProposals.removeKeys[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(currentProposals.removeKeys[_type].proposal!=_proposalAddress) throw; // can only vote for current on vote address
                    if(alreadyVoted(msg.sender, currentProposals.removeKeys[_type].votes)) throw; // cannot vote twice          
                    currentProposals.removeKeys[_type].votes.push(msg.sender); // add sender vote
                }
                if(tallyAdminVotes(currentProposals.removeKeys[_type].votes)>=activeVotesRequired.adminVotesForManagement){
                    if(_type==0 && isAdminKey(currentProposals.removeKeys[_type].proposal))deleteAddress(_type, currentProposals.removeKeys[_type].proposal); // elected            
                    if(_type==1 && isGovKey(currentProposals.removeKeys[_type].proposal))deleteAddress(_type, currentProposals.removeKeys[_type].proposal); // elected
                    uint i;
                    for(i=0;i<3;i++){
                        clearAddressProposal(i); // clear any pending address votes because voters list changed
                    }
                    clearAddressRemovalProposal(_type);
                }
            }

            function clearAddressProposal(uint _type) private{
                currentProposals.keys[_type].proposal=0; // clear current proposal address
                currentProposals.keys[_type].votes.length=0; // clear votes
                currentProposals.keys[_type].onVote=false; // open submission
            }

            function clearAddressRemovalProposal(uint _type) private{
                currentProposals.removeKeys[_type].proposal=0; // clear current proposal address
                currentProposals.removeKeys[_type].votes.length=0; // clear votes
                currentProposals.removeKeys[_type].onVote=false; // open submission
            }

            function deleteAddress(uint _type, address _address) private{
                uint i;
                if(_type==0)
                for(i=0;i<adminKeys.length;i++){
                    if(adminKeys[i]==_address)delete adminKeys[i];
                }
                if(_type==1)
                for(i=0;i<govKeys.length;i++){
                    if(govKeys[i]==_address)delete govKeys[i];
                }
            }

            function changeValueProposal(uint _proposalUint, uint _type) onlyAdmin{
                // type 0: adminVotesForParams
                // type 1: govVotesForParams
                // type 2: adminVotesForManagement
                if(_type>2) throw; // invalid type
                if((_type==0 || _type==2) && _proposalUint>getArrayNonNullLenght(adminKeys)) throw; // required number cannot be greater than active admin keys count
                if(_type==1 && _proposalUint>getArrayNonNullLenght(govKeys)) throw; // required number cannot be greater than active gov keys count
                if(_type==0)if(activeVotesRequired.adminVotesForParams==_proposalUint) throw; // cannot put a proposal for the same active value
                if(_type==1)if(activeVotesRequired.govVotesForParams==_proposalUint) throw; // cannot put a proposal for the same active value
                if(_type==2)if(activeVotesRequired.adminVotesForManagement==_proposalUint) throw; // cannot put a proposal for the same active value
                if(!currentProposals.uints[_type].onVote){
                    currentProposals.uints[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.uints[_type].proposal=_proposalUint; // set new proposal for vote
                    currentProposals.uints[_type].votes.length=0; // clear votes
                    currentProposals.uints[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(currentProposals.uints[_type].proposal!=_proposalUint) throw; // can only vote for current on vote value
                    if(alreadyVoted(msg.sender, currentProposals.uints[_type].votes)) throw; // cannot vote twice           
                    currentProposals.uints[_type].votes.push(msg.sender); // add sender vote
                }
                if(tallyAdminVotes(currentProposals.uints[_type].votes)>=activeVotesRequired.adminVotesForManagement){
                    if(_type==0 || _type==1){
                        clearAddressProposal(2); // clear any pending params address votes because of rule change
                    }
                    if(_type==0)activeVotesRequired.adminVotesForParams=currentProposals.uints[_type].proposal; // elected
                    if(_type==2){
                        clearAddressProposal(0); // clear any pending adminKey address votes because of rule change
                        clearAddressProposal(1); // clear any pending govKey address votes because of rule change
                    }
                    if(_type==1)activeVotesRequired.govVotesForParams=currentProposals.uints[_type].proposal; // elected
                    if(_type==2)activeVotesRequired.adminVotesForManagement=currentProposals.uints[_type].proposal; // elected
                    clearChangeValueProposal(_type);
                }
            }

            function clearChangeValueProposal(uint _type) private{
                currentProposals.uints[_type].proposal=0; // clear current proposal address
                currentProposals.uints[_type].votes.length=0; // clear votes
                currentProposals.uints[_type].onVote=false; // open submission
            }

            function isAdminKey(address _adminAddress) constant returns (bool isAdmin){
                uint i;
                for(i=0;i<adminKeys.length;i++){
                    if(adminKeys[i]==_adminAddress)return true;
                }
                return false;
            } 

            function isGovKey(address _govAddress) constant returns (bool isGov){
                uint i;
                for(i=0;i<govKeys.length;i++){
                    if(govKeys[i]==_govAddress)return true;
                }
                return false;
            } 

            function alreadyVoted(address _voterAddress, address[] votes) constant returns (bool voted){
                uint i;
                for(i=0;i<votes.length;i++){
                    if(votes[i]==_voterAddress)return true;
                }
                return false;
            }

            function tallyAdminVotes(address[] votes) constant returns (uint votesCount){
                uint i;
                uint count=0;
                for(i=0;i<votes.length;i++){
                    if(votes[i]!=0 && isAdminKey(votes[i]))count++;
                }
                return count;
            }

            function tallyGovVotes(address[] votes) constant returns (uint votesCount){
                uint i;
                uint count=0;
                for(i=0;i<votes.length;i++){
                    if(votes[i]!=0 && isGovKey(votes[i]))count++;
                }
                return count;
            }

            function getArrayNonNullLenght(address[] valsArr) constant returns (uint valsCount){
                uint i;
                uint count=0;
                for(i=0;i<valsArr.length;i++){
                    if(valsArr[i]!=0)count++;
                }
                return count;
            }

            function getAddressesList(uint _type) constant returns (address[] vals){
                // type 0: adminKeys
                // type 1: govKeys
                if(_type>1) throw; // invalid type
                if(_type==0)return adminKeys;
                if(_type==1)return govKeys;
            }

            function getRequiredVotes(uint _type) constant returns (uint val){
                // type 0: adminVotesForParams
                // type 1: govVotesForParams
                // type 2: adminVotesForManagement
                if(_type>2) throw; // invalid type
                if(_type==0)return activeVotesRequired.adminVotesForParams;
                if(_type==1)return activeVotesRequired.govVotesForParams;
                if(_type==2)return activeVotesRequired.adminVotesForManagement;
            }

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

            function getCurrentOnVoteVotes(uint _type, uint _type2) constant returns (address[] vals){
                // type 0: addAddress
                // type 1: changeValue
                // type 2: removeAddress

                // type2 0: adminKey
                // type2 1: govKey
                // type2 2: paramsAddress

                if(_type>2 || _type2>2) throw; // invalid type
                if(_type==0)return currentProposals.keys[_type2].votes;
                if(_type==1)return currentProposals.uints[_type2].votes;
                if(_type==2)return currentProposals.removeKeys[_type2].votes;
            }

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

            function getCurrentOnVoteValueProposal(uint _type) constant returns (uint val){
                // type 0: adminVotesForParams
                // type 1: govVotesForParams
                // type 2: adminVotesForManagement

                if(_type>2) throw; // invalid type
                return currentProposals.uints[_type].proposal;
            }

            function getParamsForBlock(uint _reqBlockHeight) constant returns (address paramsAddress){
                uint i;
                for(i=paramsHistory.length-1;i>0;i--){
                    if(paramsHistory[i].blockHeight<=_reqBlockHeight)return paramsHistory[i].paramsAddress;
                }
                if(paramsHistory[0].blockHeight<=_reqBlockHeight)return paramsHistory[0].paramsAddress;
                return 0;
            }

            function getParamAddressAtIndex(uint _paramIndex) constant returns (address paramsAddress){
                return paramsHistory[_paramIndex].paramsAddress;
            }

            function getParamHeightAtIndex(uint _paramIndex) constant returns (uint paramsHeight){
                return paramsHistory[_paramIndex].blockHeight;
            }

            function getParamCount() constant returns (uint paramsCount){
                return paramsHistory.length;
            }
        }
        """
        contract_data = self.node.createcontract("6060604052601e6003556000600460006101000a81548160ff021916908315150217905550341561002c57fe5b5b6130288061003c6000396000f30060606040523615610126576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680630c83ebac14610128578063153417471461019157806319971cbd146101f15780631ec28e0f1461021a57806327e357461461024e57806330a79873146102745780634364725c146102d45780634afb4f11146103085780634cc0e2bc146103735780635f302e8b146103b25780636b102c49146103f35780636fb81cbb146104415780637b993bf314610453578063850d9758146104a15780638a5a9d07146105245780639b21662614610558578063aff125f6146105c3578063bec171e514610623578063bf5f1e831461068e578063e9944a81146106cd578063f769ac481461075b578063f9f51401146107bb575bfe5b341561013057fe5b61014f6004808035906020019091908035906020019091905050610847565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561019957fe5b6101af60048080359060200190919050506108fe565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156101f957fe5b610218600480803590602001909190803590602001909190505061094d565b005b341561022257fe5b6102386004808035906020019091905050610f24565b6040518082815260200191505060405180910390f35b341561025657fe5b61025e610f7d565b6040518082815260200191505060405180910390f35b341561027c57fe5b6102926004808035906020019091905050610f8b565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156102dc57fe5b6102f26004808035906020019091905050610fcb565b6040518082815260200191505060405180910390f35b341561031057fe5b61035d600480803590602001908201803590602001908080602002602001604051908101604052809392919081815260200183836020028082843782019150505050505091905050610ffe565b6040518082815260200191505060405180910390f35b341561037b57fe5b6103b0600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091908035906020019091905050611097565b005b34156103ba57fe5b6103d9600480803590602001909190803590602001909190505061178e565b604051808215151515815260200191505060405180910390f35b34156103fb57fe5b610427600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050611856565b604051808215151515815260200191505060405180910390f35b341561044957fe5b6104516118fd565b005b341561045b57fe5b610487600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190505061199a565b604051808215151515815260200191505060405180910390f35b34156104a957fe5b6104bf6004808035906020019091905050611a41565b6040518080602001828103825283818151815260200191508051906020019060200280838360008314610511575b805182526020831115610511576020820191506020810190506020830392506104ed565b5050509050019250505060405180910390f35b341561052c57fe5b6105426004808035906020019091905050611b8c565b6040518082815260200191505060405180910390f35b341561056057fe5b6105ad600480803590602001908201803590602001908080602002602001604051908101604052809392919081815260200183836020028082843782019150505050505091905050611bbb565b6040518082815260200191505060405180910390f35b34156105cb57fe5b6105e16004808035906020019091905050611c2c565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561062b57fe5b610678600480803590602001908201803590602001908080602002602001604051908101604052809392919081815260200183836020028082843782019150505050505091905050611c6c565b6040518082815260200191505060405180910390f35b341561069657fe5b6106cb600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091908035906020019091905050611d05565b005b34156106d557fe5b610741600480803573ffffffffffffffffffffffffffffffffffffffff1690602001909190803590602001908201803590602001908080602002602001604051908101604052809392919081815260200183836020028082843782019150505050505091905050612770565b604051808215151515815260200191505060405180910390f35b341561076357fe5b61077960048080359060200190919050506127f0565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156107c357fe5b6107e26004808035906020019091908035906020019091905050612917565b6040518080602001828103825283818151815260200191508051906020019060200280838360008314610834575b80518252602083111561083457602082019150602081019050602083039250610810565b5050509050019250505060405180910390f35b600060018311806108585750600282115b156108635760006000fd5b60008314156108ad576005600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690506108f8565b60018314156108f7576005600201600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690506108f8565b5b92915050565b600060008281548110151561090f57fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b919050565b61095633611856565b15156109625760006000fd5b60028111156109715760006000fd5b60008114806109805750600281145b8015610a195750610a166001805480602002602001604051908101604052809291908181526020018280548015610a0c57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116109c2575b5050505050611bbb565b82115b15610a245760006000fd5b600181148015610ac15750610abe6002805480602002602001604051908101604052809291908181526020018280548015610ab457602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610a6a575b5050505050611bbb565b82115b15610acc5760006000fd5b6000811415610ae957816008600001541415610ae85760006000fd5b5b6001811415610b0657816008600101541415610b055760006000fd5b5b6002811415610b2357816008600201541415610b225760006000fd5b5b6005600101600082815260200190815260200160002060000160009054906101000a900460ff161515610c475760016005600101600083815260200190815260200160002060000160006101000a81548160ff02191690831515021790555081600560010160008381526020019081526020016000206002018190555060006005600101600083815260200190815260200160002060010181610bc69190612eea565b50600560010160008281526020019081526020016000206001018054806001018281610bf29190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050610d9d565b816005600101600083815260200190815260200160002060020154141515610c6f5760006000fd5b610d163360056001016000848152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015610d0c57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610cc2575b5050505050612770565b15610d215760006000fd5b600560010160008281526020019081526020016000206001018054806001018281610d4c9190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b600860020154610e4960056001016000848152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015610e3f57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610df5575b5050505050611c6c565b101515610f1e576000811480610e5f5750600181145b15610e6f57610e6e6002612b4a565b5b6000811415610e9c5760056001016000828152602001908152602001600020600201546008600001819055505b6002811415610eba57610eaf6000612b4a565b610eb96001612b4a565b5b6001811415610ee75760056001016000828152602001908152602001600020600201546008600101819055505b6002811415610f145760056001016000828152602001908152602001600020600201546008600201819055505b610f1d81612c00565b5b5b5b5050565b60006002821115610f355760006000fd5b6000821415610f4b576008600001549050610f78565b6001821415610f61576008600101549050610f78565b6002821415610f77576008600201549050610f78565b5b919050565b600060008054905090505b90565b600281815481101515610f9a57fe5b906000526020600020900160005b915054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60006002821115610fdc5760006000fd5b600560010160008381526020019081526020016000206002015490505b919050565b60006000600060009050600091505b835182101561108c576000848381518110151561102657fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff1614158015611070575061106f848381518110151561106057fe5b9060200190602002015161199a565b5b1561107e5780806001019150505b5b818060010192505061100d565b8092505b5050919050565b600060006110a433611856565b15156110b05760006000fd5b60008473ffffffffffffffffffffffffffffffffffffffff1614156110d55760006000fd5b60018311156110e45760006000fd5b60008314156111b65761117c600180548060200260200160405190810160405280929190818152602001828054801561117257602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611128575b5050505050611bbb565b9150600860000154821480611195575060086002015482145b156111a05760006000fd5b6111a984611856565b15156111b55760006000fd5b5b600183141561127657600860010154611254600280548060200260200160405190810160405280929190818152602001828054801561124a57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611200575b5050505050611bbb565b14156112605760006000fd5b6112698461199a565b15156112755760006000fd5b5b6005600201600084815260200190815260200160002060000160009054906101000a900460ff1615156113d45760016005600201600085815260200190815260200160002060000160006101000a81548160ff021916908315150217905550836005600201600085815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550600060056002016000858152602001908152602001600020600101816113539190612eea565b5060056002016000848152602001908152602001600020600101805480600101828161137f9190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050611576565b8373ffffffffffffffffffffffffffffffffffffffff166005600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415156114485760006000fd5b6114ef33600560020160008681526020019081526020016000206001018054806020026020016040519081016040528092919081815260200182805480156114e557602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001906001019080831161149b575b5050505050612770565b156114fa5760006000fd5b6005600201600084815260200190815260200160002060010180548060010182816115259190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b6008600201546116226005600201600086815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561161857602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116115ce575b5050505050611c6c565b1015156117865760008314801561167757506116766005600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611856565b5b156116c0576116bf836005600201600086815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16612c7c565b5b60018314801561170e575061170d6005600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1661199a565b5b1561175757611756836005600201600086815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16612c7c565b5b600090505b600381101561177c5761176e81612b4a565b5b808060010191505061175c565b61178583612e34565b5b5b5b50505050565b6000600283118061179f5750600282115b156117aa5760006000fd5b60008314156117e1576005600001600083815260200190815260200160002060000160009054906101000a900460ff169050611850565b6001831415611818576005600101600083815260200190815260200160002060000160009054906101000a900460ff169050611850565b600283141561184f576005600201600083815260200190815260200160002060000160009054906101000a900460ff169050611850565b5b92915050565b60006000600090505b6001805490508110156118f2578273ffffffffffffffffffffffffffffffffffffffff1660018281548110151561189257fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1614156118e457600191506118f7565b5b808060010191505061185f565b600091505b50919050565b600460009054906101000a900460ff16156119185760006000fd5b6001805480600101828161192c9190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550506001600460006101000a81548160ff0219169083151502179055505b565b60006000600090505b600280549050811015611a36578273ffffffffffffffffffffffffffffffffffffffff166002828154811015156119d657fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415611a285760019150611a3b565b5b80806001019150506119a3565b600091505b50919050565b611a49612f42565b6001821115611a585760006000fd5b6000821415611aef576001805480602002602001604051908101604052809291908181526020018280548015611ae357602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611a99575b50505050509050611b87565b6001821415611b86576002805480602002602001604051908101604052809291908181526020018280548015611b7a57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611b30575b50505050509050611b87565b5b919050565b6000600082815481101515611b9d57fe5b906000526020600020906002020160005b506000015490505b919050565b60006000600060009050600091505b8351821015611c215760008483815181101515611be357fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff16141515611c135780806001019150505b5b8180600101925050611bca565b8092505b5050919050565b600181815481101515611c3b57fe5b906000526020600020900160005b915054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60006000600060009050600091505b8351821015611cfa5760008483815181101515611c9457fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff1614158015611cde5750611cdd8483815181101515611cce57fe5b90602001906020020151611856565b5b15611cec5780806001019150505b5b8180600101925050611c7b565b8092505b5050919050565b611d0e33611856565b80611d1e5750611d1d3361199a565b5b1515611d2a5760006000fd5b600081148015611dca5750600354611dc76001805480602002602001604051908101604052809291908181526020018280548015611dbd57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611d73575b5050505050611bbb565b10155b15611dd55760006000fd5b600181148015611e755750600354611e726002805480602002602001604051908101604052809291908181526020018280548015611e6857602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611e1e575b5050505050611bbb565b10155b15611e805760006000fd5b60008273ffffffffffffffffffffffffffffffffffffffff161415611ea55760006000fd5b6002811115611eb45760006000fd5b6000811480611ec35750600181145b8015611ee45750611ed382611856565b80611ee35750611ee28261199a565b5b5b15611eef5760006000fd5b6005600001600082815260200190815260200160002060000160009054906101000a900460ff16151561206157611f253361199a565b15611f305760006000fd5b60016005600001600083815260200190815260200160002060000160006101000a81548160ff021916908315150217905550816005600001600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060006005600001600083815260200190815260200160002060010181611fe09190612eea565b5060056000016000828152602001908152602001600020600101805480600101828161200c9190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050612203565b8173ffffffffffffffffffffffffffffffffffffffff166005600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415156120d55760006000fd5b61217c336005600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561217257602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612128575b5050505050612770565b156121875760006000fd5b6005600001600082815260200190815260200160002060010180548060010182816121b29190612f16565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b60008114806122125750600181145b156124b9576008600201546122c3600560000160008481526020019081526020016000206001018054806020026020016040519081016040528092919081815260200182805480156122b957602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001906001019080831161226f575b5050505050611c6c565b1015156124b85761230c6005600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611856565b8061235557506123546005600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1661199a565b5b156123605760006000fd5b6000811415612407576001805480600101828161237d9190612f16565b916000526020600020900160005b6005600001600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b60018114156124ae57600280548060010182816124249190612f16565b916000526020600020900160005b6005600001600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b6124b781612b4a565b5b5b600281141561276a5760086000015461256e6005600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561256457602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001906001019080831161251a575b5050505050611c6c565b1015801561262657506008600101546126236005600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561261957602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116125cf575b5050505050610ffe565b10155b1561276957600060008054905011801561266e575060014301600060016000805490500381548110151561265657fe5b906000526020600020906002020160005b5060000154145b156126795760006000fd5b6000805480600101828161268d9190612f56565b916000526020600020906002020160005b6040604051908101604052806001430181526020016005600001600087815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681525090919091506000820151816000015560208201518160010160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050505061276881612b4a565b5b5b5b5b5050565b60006000600090505b82518110156127e4578373ffffffffffffffffffffffffffffffffffffffff1683828151811015156127a757fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff1614156127d657600191506127e9565b5b8080600101915050612779565b600091505b5092915050565b6000600060016000805490500390505b6000811115612891578260008281548110151561281957fe5b906000526020600020906002020160005b50600001541115156128825760008181548110151561284557fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169150612911565b5b808060019003915050612800565b82600060008154811015156128a257fe5b906000526020600020906002020160005b506000015411151561290c57600060008154811015156128cf57fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169150612911565b600091505b50919050565b61291f612f42565b600283118061292e5750600282115b156129395760006000fd5b60008314156129e757600560000160008381526020019081526020016000206001018054806020026020016040519081016040528092919081815260200182805480156129db57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612991575b50505050509050612b44565b6001831415612a955760056001016000838152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015612a8957602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612a3f575b50505050509050612b44565b6002831415612b435760056002016000838152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015612b3757602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612aed575b50505050509050612b44565b5b92915050565b60006005600001600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060006005600001600083815260200190815260200160002060010181612bc99190612eea565b5060006005600001600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b6000600560010160008381526020019081526020016000206002018190555060006005600101600083815260200190815260200160002060010181612c459190612eea565b5060006005600101600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b60006000831415612d5657600090505b600180549050811015612d55578173ffffffffffffffffffffffffffffffffffffffff16600182815481101515612cbf57fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415612d4757600181815481101515612d1757fe5b906000526020600020900160005b6101000a81549073ffffffffffffffffffffffffffffffffffffffff02191690555b5b8080600101915050612c8c565b5b6001831415612e2e57600090505b600280549050811015612e2d578173ffffffffffffffffffffffffffffffffffffffff16600282815481101515612d9757fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415612e1f57600281815481101515612def57fe5b906000526020600020900160005b6101000a81549073ffffffffffffffffffffffffffffffffffffffff02191690555b5b8080600101915050612d64565b5b5b505050565b60006005600201600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060006005600201600083815260200190815260200160002060010181612eb39190612eea565b5060006005600201600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b815481835581811511612f1157818360005260206000209182019101612f109190612f88565b5b505050565b815481835581811511612f3d57818360005260206000209182019101612f3c9190612f88565b5b505050565b602060405190810160405280600081525090565b815481835581811511612f8357600202816002028360005260206000209182019101612f829190612fad565b5b505050565b612faa91905b80821115612fa6576000816000905550600101612f8e565b5090565b90565b612ff991905b80821115612ff557600060008201600090556001820160006101000a81549073ffffffffffffffffffffffffffffffffffffffff021916905550600201612fb3565b5090565b905600a165627a7a7230582027c884a554cedf19638ad5307d61b3e5739d70ee5c1780a257becf9b100b4afe0029", 500000000, 0.00000001)
        self.contract_address = contract_data['address']
        self.node.generate(1)

    def create_proposal_contract(self):
        """
        pragma solidity ^0.4.11;

        contract gasSchedule{
            uint32[39] _gasSchedule=[
                10, //0: tierStepGas0
                10, //1: tierStepGas1
                10, //2: tierStepGas2
                10, //3: tierStepGas3
                10, //4: tierStepGas4
                10, //5: tierStepGas5
                10, //6: tierStepGas6
                10, //7: tierStepGas7
                10, //8: expGas
                50, //9: expByteGas
                30, //10: sha3Gas
                6, //11: sha3WordGas
                200, //12: sloadGas
                20000, //13: sstoreSetGas
                5000, //14: sstoreResetGas
                15000, //15: sstoreRefundGas
                1, //16: jumpdestGas
                375, //17: logGas
                8, //18: logDataGas
                375, //19: logTopicGas
                32000, //20: createGas
                700, //21: callGas
                2300, //22: callStipend
                9000, //23: callValueTransferGas
                25000, //24: callNewAccountGas
                24000, //25: suicideRefundGas
                3, //26: memoryGas
                512, //27: quadCoeffDiv
                200, //28: createDataGas
                21000, //29: txGas
                53000, //30: txCreateGas
                4, //31: txDataZeroGas
                68, //32: txDataNonZeroGas
                3, //33: copyGas
                700, //34: extcodesizeGas
                700, //35: extcodecopyGas
                400, //36: balanceGas
                5000, //37: suicideGas
                24576 //38: maxCodeSize
            ];
            function getSchedule() constant returns(uint32[39] _schedule){
                return _gasSchedule;
            }
        }
        """
        contract_data = self.node.createcontract("60606040526104e060405190810160405280600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161138861ffff16815260200161600061ffff1681525060009060276101df9291906101ed565b5034156101e857fe5b6102c1565b82602760070160089004810192821561027d5791602002820160005b8382111561024b57835183826101000a81548163ffffffff021916908361ffff1602179055509260200192600401602081600301049283019260010302610209565b801561027b5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261024b565b505b50905061028a919061028e565b5090565b6102be91905b808211156102ba57600081816101000a81549063ffffffff021916905550600101610294565b5090565b90565b61016a806102d06000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003b575bfe5b341561004357fe5b61004b610097565b6040518082602760200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000602780602002604051908101604052809291908260278015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6104e0604051908101604052806027905b600063ffffffff1681526020019060019003908161012057905050905600a165627a7a72305820a5a4ef1cfc85ef81e4f55205cd6a07d4cd7f65c3e87f1f5dd2f0fd8e22471c910029", 10000000, 0.00000001)
        self.proposal_address = contract_data['address']
        self.abiGetSchedule = "26fadbe2"

    def run_test(self):
        self.node.generate(COINBASE_MATURITY+100)
        self.create_dgp_contract()
        state = DGPState(self.node, self.contract_address)
        # Our initial admin key
        admin_address = self.node.getnewaddress()

        # Set the initial admin
        state.send_set_initial_admin(admin_address)
        self.node.generate(1)
        state.admin_keys.append(p2pkh_to_hex_hash(admin_address))
        state.assert_state()

        # Verify that we can't use the setInitialAdmin method again
        throwaway_admin_address = self.node.getnewaddress()
        state.send_set_initial_admin(throwaway_admin_address)
        self.node.generate(1)
        # The state does not change
        state.assert_state()

        # Create the maximum number of admin keys (30), i.e. 29 new keys
        for i in range(1, 30):
            admin_address = self.node.getnewaddress()
            state.send_add_address_proposal(p2pkh_to_hex_hash(admin_address), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
            self.node.generate(1)
            state.admin_keys.append(p2pkh_to_hex_hash(admin_address))
            state.assert_state()

        # Try to add another admin key (should not succeed since we already have 30 admin keys)
        admin_address = self.node.getnewaddress()
        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_address), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        # The state should remain the same
        state.assert_state()

        # Try to delete all admin keys (this would effectively deadlock the contract)
        # State by removing 29 admin keys, indices 0 - 28
        for i in range(0, 29):
            removed_admin_key = state.admin_keys.pop(-1)
            state.send_remove_address_proposal(removed_admin_key, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
            self.node.generate(1)
            state.assert_state()

        # Finally try to remove the last one
        admin_key = state.admin_keys[0]
        #state.send_remove_address_proposal(admin_key, 0, keyhash_to_p2pkh(hex_str_to_bytes(admin_key)))
        #self.node.generate(1)
        # The state should not have changed, i.e. no admin key should be removed
        #state.assert_state()

        self.create_proposal_contract()
        # Add an address proposal by using the last removed admin key, expected fail
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(removed_admin_key)))
        self.node.generate(1)
        state.assert_state()

        # Add a value proposal by using the last removed admin key, expected fail
        state.send_change_value_proposal(1, 2, keyhash_to_p2pkh(hex_str_to_bytes(removed_admin_key)))
        self.node.generate(1)
        state.assert_state()

        # Try to remove an already removed admin key, expected fail
        state.send_remove_address_proposal(removed_admin_key, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.assert_state()

        # Try to set the adminVotesForParams > number of admin keys
        state.send_change_value_proposal(2, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.assert_state()

        # Try to set the govVotesForParams > number of gov keys
        state.send_change_value_proposal(1, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.assert_state()

        # Try to set the adminVotesForManagement > number of admin keys
        state.send_change_value_proposal(2, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.assert_state()


        # Add 29 new admin keys so that we have 30 in total again
        for i in range(1, 30):
            admin_address = self.node.getnewaddress()
            state.send_add_address_proposal(p2pkh_to_hex_hash(admin_address), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
            self.node.generate(1)
            state.admin_keys.append(p2pkh_to_hex_hash(admin_address))
            state.assert_state()


        # A valid admin key makes an address params proposal, should activate immediately
        self.create_proposal_contract()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()


        # Increase the number of admin keys required for approving param proposals
        state.send_change_value_proposal(5, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-1])))
        self.node.generate(1)
        state.required_votes[0] = 5
        state.assert_state()

        self.create_proposal_contract()
        for i in range(4):
            state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-1*i])))
            self.node.generate(1)
            state.current_on_vote_statuses[0][2] = True
            state.current_on_vote_address_proposals[0][2] = self.proposal_address
            state.assert_state()

        # vote once more, but for a different proposal
        state.send_add_address_proposal("123456789123456789123456789123456789", 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-5])))
        self.node.generate(1)
        state.assert_state()

        # The params proposal is still not accepted, make sure that it is not deleted when we change the number of admin votes required for management.
        state.send_change_value_proposal(2, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-1])))
        self.node.generate(1)
        # adminVotesForManagement should be 2 now
        state.required_votes[2] = 2
        # The old param proposal should not be deleted due to the required votes changed
        state.assert_state()

        # The params proposal is still not accepted, make sure that it is deleted when we change the number of admin votes required for params.
        # Requires 2 votes due to the last adminvotesForManagement change
        state.send_change_value_proposal(4, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-1])))
        state.send_change_value_proposal(4, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-2])))
        self.node.generate(1)
        # adminVotesForParams should be 4 now
        state.required_votes[0] = 4
        # The params vote should have been deleted
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        # The old param proposal should not be deleted due to the required votes changed
        state.assert_state()


        # Redo the params vote, now it only requires 4 admin votes to activate. Start with 3 votes
        for i in range(3):
            state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
            self.node.generate(1)
            state.current_on_vote_statuses[0][2] = True
            state.current_on_vote_address_proposals[0][2] = self.proposal_address
            state.assert_state()

        # Submit a duplicate vote
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        # The state should not have changed
        state.assert_state()

        # Submit the final vote
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[4])))
        self.node.generate(1)
        # The params proposal should have been accepted.
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        # When the vote is activated, the current vote should be removed.
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.assert_state()

        # Set both adminVotesForManagement and adminVotesForParams to 30
        state.send_change_value_proposal(30, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.send_change_value_proposal(30, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][0] = True
        state.current_on_vote_statuses[1][2] = True
        # The vote should not have activated yet (it requires 2 admin votes)
        state.assert_state()

        # Try to submit two proposals with different uint values
        state.send_change_value_proposal(29, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.send_change_value_proposal(29, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        # Nothing should have changed
        state.assert_state()

        # Continue and activate both votes
        state.send_change_value_proposal(30, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.send_change_value_proposal(30, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][0] = False
        state.current_on_vote_statuses[1][2] = False
        state.required_votes[0] = 30
        state.required_votes[2] = 30
        state.assert_state()
        

        # Try to remove an admin key such that we have one less than being able to accept proposals.
        state.send_remove_address_proposal(state.admin_keys[0], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[-1])))
        self.node.generate(1)
        state.assert_state()


        # Start including gov votes in tests.
        for i in range(5):
            gov_address = self.node.getnewaddress()
            for j in range(len(state.admin_keys)):
                state.send_add_address_proposal(p2pkh_to_hex_hash(gov_address), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[j])))
            self.node.generate(1)
            state.gov_keys.append(p2pkh_to_hex_hash(gov_address))
            state.assert_state()

        # Lower the adminVotesForParams to 2 (so that we can submit a proposal without having it immediately accepted)
        for i in range(29):
            state.send_change_value_proposal(2, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
            self.node.generate(1)
            state.current_on_vote_statuses[1][0] = True
            state.assert_state()
        # Send the final vote
        state.send_change_value_proposal(2, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[29])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][0] = False
        state.required_votes[0] = 2
        state.assert_state()


        # Make sure that no gov votes are required to accept a proposal
        self.create_proposal_contract()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[8])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[9])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()


        # Increase the number of gov votes required for accepting new params
        for i in range(30):
            state.send_change_value_proposal(2, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
        self.node.generate(1)
        state.required_votes[1] = 2
        state.assert_state()

        # Make sure that govs can't initiate new votes
        self.create_proposal_contract()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[0])))
        self.node.generate(1)
        # No state changes because a gov can't initiate votes
        state.assert_state()

        # Create a new proposal which will now require 2 gov votes to activate in addidion to 2 admin votes
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        # Add 1 gov vote and 1 more admin vote, make sure it does not activate (needs 1 more gov vote)
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[0])))
        self.node.generate(1)
        state.assert_state()

        # Add the final gov vote, make sure it activates
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()

        # Try to delete all govs, (should only be possible to delete 3 of 5 govs since we need 2 govs to activate any param proposal)
        for i in range(len(state.gov_keys)):
            gov_key = state.gov_keys[i]
            for i in range(len(state.admin_keys)):
                state.send_remove_address_proposal(gov_key, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
            self.node.generate(1)
        # Make sure that exactly 2 gov keys remain (the last ones)
        for _ in range(3):
            state.gov_keys.pop(0)
        state.assert_state()

        # Lower the amount of admin keys required for management to 1.
        for i in range(len(state.admin_keys)):
            state.send_change_value_proposal(1, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
        self.node.generate(1)
        state.required_votes[2] = 1
        state.assert_state()

        # Try to use a gov key to call methods that should only be calleable by the admin
        for type1 in range(2):
            state.send_remove_address_proposal(state.gov_keys[0], type1, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[-1])))
            self.node.generate(1)
        state.assert_state()

        for type1 in range(3):
            state.send_change_value_proposal(1, type1, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[-1])))
            self.node.generate(1)
        state.assert_state()

        for type1 in range(3):
            state.send_add_address_proposal("0123456789012345678901234567890123456789", type1, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[-1])))
            self.node.generate(1)
        state.assert_state()

        # Try to remove an admin key that is in fact a gov key
        state.send_remove_address_proposal(state.gov_keys[0], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Try to remove a gov key that is in fact an admin key
        state.send_remove_address_proposal(state.admin_keys[0], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Try to add an admin key that already exists
        state.send_add_address_proposal(state.admin_keys[0], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Try to add a gov key that already exists
        state.send_add_address_proposal(state.gov_keys[0], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Make sure that we can still add a new gov key
        gov_address = self.node.getnewaddress()
        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_address), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        state.gov_keys.append(p2pkh_to_hex_hash(gov_address))
        self.node.generate(1)
        state.assert_state()

        # Submit a new proposal where votes are submitted in the following block order (only 2 admin votes and 2 gov votes are required for the proposal to be accepted): 1 admin | 2 gov votes | 29 admin votes
        self.create_proposal_contract()
        # Add the first admin vote which initializes the vote
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()
        # Add 2 gov votes
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[0])))
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[1])))
        self.node.generate(1)
        state.assert_state()
        # Now add 29 more admin votes in one block
        for i in range(1, 30):
            state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
        self.node.generate(1)
        
        # This is probably not wanted behaviour
        #state.current_on_vote_statuses[0][2] = False
        #state.current_on_vote_address_proposals[0][2] = "0"
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()

        # Remove the last proposal, we do this by changing the number of gov votes required for a param proposal
        for i in range(0, 30):
            state.send_change_value_proposal(3, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[i])))
        self.node.generate(1)
        state.required_votes[1] = 3
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.assert_state()

        # Delete one admin key to prepare for the next tests and set all all of the required votes to 2
        state.send_remove_address_proposal(state.admin_keys.pop(-1), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.send_change_value_proposal(2, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.required_votes[0] = 2
        state.assert_state()
        state.send_change_value_proposal(0, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.required_votes[1] = 0
        state.assert_state()
        state.send_change_value_proposal(2, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.required_votes[2] = 2
        state.assert_state()

        # Create one of each type of proposal
        self.create_proposal_contract()
        admin_key = self.node.getnewaddress()
        gov_key = self.node.getnewaddress()
        state.send_change_value_proposal(3, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][0] = True
        state.assert_state()

        state.send_change_value_proposal(3, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][1] = True
        state.assert_state()

        state.send_change_value_proposal(3, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][2] = True
        state.assert_state()

        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_key), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][0] = True
        state.current_on_vote_address_proposals[0][0] = p2pkh_to_hex_hash(admin_key)
        state.assert_state()

        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_key), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][1] = True
        state.current_on_vote_address_proposals[0][1] = p2pkh_to_hex_hash(gov_key)
        state.assert_state()

        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        state.send_remove_address_proposal(state.admin_keys[-1], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][0] = True
        state.current_on_vote_address_proposals[1][0] = state.admin_keys[-1]
        state.assert_state()

        state.send_remove_address_proposal(state.gov_keys[-1], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][1] = True
        state.current_on_vote_address_proposals[1][1] = state.gov_keys[-1]
        state.assert_state()

        # Now make sure that accepting one proposal has the desired affect on the the other proposals
        # Accepting a proposal for removing keys should reset the addAddressProposals
        state.send_remove_address_proposal(state.admin_keys[-1], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][0] = False
        state.current_on_vote_address_proposals[1][0] = "0"
        for type1 in range(3):
            state.current_on_vote_statuses[0][type1] = False
            state.current_on_vote_address_proposals[0][type1] = "0"
        state.admin_keys.pop(-1)
        state.assert_state()

        # Restore the state
        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_key), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][0] = True
        state.current_on_vote_address_proposals[0][0] = p2pkh_to_hex_hash(admin_key)
        state.assert_state()
        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_key), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][1] = True
        state.current_on_vote_address_proposals[0][1] = p2pkh_to_hex_hash(gov_key)
        state.assert_state()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()
        state.send_remove_address_proposal(state.admin_keys[-1], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][0] = True
        state.current_on_vote_address_proposals[1][0] = state.admin_keys[-1]
        state.assert_state()

        # Accepting an address should not cause other proposals to be altered
        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_key), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][0] = False
        state.current_on_vote_address_proposals[0][0] = "0"
        state.admin_keys.append(p2pkh_to_hex_hash(admin_key))
        state.assert_state()
        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_key), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][1] = False
        state.current_on_vote_address_proposals[0][1] = "0"
        state.gov_keys.append(p2pkh_to_hex_hash(gov_key))
        state.assert_state()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()

        # Restore state
        admin_key = self.node.getnewaddress()
        gov_key = self.node.getnewaddress()
        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_key), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][0] = True
        state.current_on_vote_address_proposals[0][0] = p2pkh_to_hex_hash(admin_key)
        state.assert_state()
        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_key), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][1] = True
        state.current_on_vote_address_proposals[0][1] = p2pkh_to_hex_hash(gov_key)
        state.assert_state()
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        # Make sure that sending a changeValueProposal of type 1 (gov keys) resets the params proposals
        state.send_change_value_proposal(3, 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][1] = False
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.required_votes[1] = 3
        state.assert_state()

        # Restore state
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[0])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        # Finally verify that accepting all proposals result in the correct DGP state.
        # Accept the new admin key
        state.send_add_address_proposal(p2pkh_to_hex_hash(admin_key), 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][0] = False
        state.current_on_vote_address_proposals[0][0] = "0"
        state.admin_keys.append(p2pkh_to_hex_hash(admin_key))
        state.assert_state()

        # Accept the new gov key
        state.send_add_address_proposal(p2pkh_to_hex_hash(gov_key), 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][1] = False
        state.current_on_vote_address_proposals[0][1] = "0"
        state.gov_keys.append(p2pkh_to_hex_hash(gov_key))
        state.assert_state()

        # Accept the new proposal key
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        for i in range(3):
            state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.gov_keys[i])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.param_count += 1
        state.params_for_block.append((self.node.getblockcount()+1, self.proposal_address))
        state.param_address_at_indices.append(self.proposal_address)
        state.assert_state()

        # Accept the new adminVotesForParams
        state.send_change_value_proposal(3, 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][0] = False
        state.required_votes[0] = 3
        state.assert_state()

        # Accept the removeAddress of an admin key
        state.send_remove_address_proposal(state.admin_keys[-3], 0, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][0] = False
        state.current_on_vote_address_proposals[1][0] = "0"
        state.admin_keys.pop(-3)
        state.assert_state()

        # Accept the removeAddress of an gov key
        state.send_remove_address_proposal(state.gov_keys[-3], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][1] = False
        state.current_on_vote_address_proposals[1][1] = "0"
        state.gov_keys.pop(-3)
        state.assert_state()

        # Finally accept the last adminVotesForManagement
        state.send_change_value_proposal(3, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][2] = False
        state.required_votes[2] = 3
        state.assert_state()
        print(state.current_on_vote_address_proposals)
        print(state.current_on_vote_statuses)

if __name__ == '__main__':
    QtumDGPTest().main()
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
        pragma solidity ^0.4.8;

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
            uint private proposalExpiryBlocks=216;

            struct addressProposal{
                bool onVote;
                address[] votes;
                address proposal;
                uint proposalHeight;
            }

            struct uintProposal{
                bool onVote;
                address[] votes;
                uint proposal;
                uint proposalHeight;
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
                if(!isAdminKey(msg.sender))throw;
                _;
            }

            modifier onlyAdminOrGov{
                if(!isAdminKey(msg.sender) && !isGovKey(msg.sender))throw;
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
                if(_type==0 && getArrayNonNullLength(adminKeys)>=maxKeys) throw; // we have too many admin keys
                if(_type==1 && getArrayNonNullLength(govKeys)>=maxKeys) throw; // we have too many gov keys
                if(_proposalAddress==0) throw; // invalid address
                if(_type>2) throw; // invalid type
                if((_type==0 || _type==1) && (isAdminKey(_proposalAddress) || isGovKey(_proposalAddress))) throw; // don't add existing keys as proposals
                if(!currentProposals.keys[_type].onVote){
                    if(isGovKey(msg.sender)) throw; // Only Admin can initiate vote
                    currentProposals.keys[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.keys[_type].proposal=_proposalAddress; // set new proposal for vote
                    currentProposals.keys[_type].proposalHeight=block.number; // set new proposal initial height
                    currentProposals.keys[_type].votes.length=0; // clear votes
                    currentProposals.keys[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(block.number-currentProposals.keys[_type].proposalHeight>proposalExpiryBlocks){
                        clearAddressProposal(_type); //clear expired proposals
                        return;
                    }
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
                uint adminsCount=getArrayNonNullLength(adminKeys);
                if(adminsCount==activeVotesRequired.adminVotesForParams || adminsCount==activeVotesRequired.adminVotesForManagement) throw; // cannot reduce the number of admins below the required ones
                if(!isAdminKey(_proposalAddress)) throw; // don't remove non existent address
                }
                if(_type==1){
                if(getArrayNonNullLength(govKeys)==activeVotesRequired.govVotesForParams) throw; // cannot reduce the number of govs below the required ones
                if(!isGovKey(_proposalAddress)) throw; // don't remove non existent address
                }
                if(!currentProposals.removeKeys[_type].onVote){
                    currentProposals.removeKeys[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.removeKeys[_type].proposal=_proposalAddress; // set new proposal for vote
                    currentProposals.removeKeys[_type].proposalHeight=block.number; // set new proposal initial height
                    currentProposals.removeKeys[_type].votes.length=0; // clear votes
                    currentProposals.removeKeys[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(block.number-currentProposals.removeKeys[_type].proposalHeight>proposalExpiryBlocks){
                        clearAddressRemovalProposal(_type); //clear expired proposals
                        return;
                    }
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
                currentProposals.keys[_type].proposalHeight=0; // clear proposal height
                currentProposals.keys[_type].onVote=false; // open submission
            }

            function clearAddressRemovalProposal(uint _type) private{
                currentProposals.removeKeys[_type].proposal=0; // clear current proposal address
                currentProposals.removeKeys[_type].votes.length=0; // clear votes
                currentProposals.removeKeys[_type].proposalHeight=0; // clear proposal height
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
                if((_type==0 || _type==2) && _proposalUint>getArrayNonNullLength(adminKeys)) throw; // required number cannot be greater than active admin keys count
                if(_type==1 && _proposalUint>getArrayNonNullLength(govKeys)) throw; // required number cannot be greater than active gov keys count
                if(_type==0)if(activeVotesRequired.adminVotesForParams==_proposalUint) throw; // cannot put a proposal for the same active value
                if(_type==1)if(activeVotesRequired.govVotesForParams==_proposalUint) throw; // cannot put a proposal for the same active value
                if(_type==2)if(activeVotesRequired.adminVotesForManagement==_proposalUint) throw; // cannot put a proposal for the same active value
                if(!currentProposals.uints[_type].onVote){
                    currentProposals.uints[_type].onVote=true; // put proposal on vote, no changes until vote is setteled or removed
                    currentProposals.uints[_type].proposal=_proposalUint; // set new proposal for vote
                    currentProposals.uints[_type].proposalHeight=block.number; // set new proposal initial height
                    currentProposals.uints[_type].votes.length=0; // clear votes
                    currentProposals.uints[_type].votes.push(msg.sender); // add sender vote
                }else{
                    if(block.number-currentProposals.uints[_type].proposalHeight>proposalExpiryBlocks){
                        clearChangeValueProposal(_type); //clear expired proposals
                        return;
                    }
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
                currentProposals.uints[_type].proposalHeight=0; // clear proposal height
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

            function getArrayNonNullLength(address[] valsArr) constant returns (uint valsCount){
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
                if(paramsHistory.length==0)return 0;
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
        contract_data = self.node.createcontract("6060604052601e6003556000600460006101000a81548160ff02191690831515021790555060d8600555341561003457600080fd5b5b613183806100446000396000f30060606040523615610126576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680630c83ebac1461012b578063153417471461019757806319971cbd146101fa5780631ec28e0f1461022657806327e357461461025d57806330a79873146102865780633a32306c146102e95780634364725c146103575780634afb4f111461038e5780634cc0e2bc146103fc5780635f302e8b1461043e5780636b102c49146104825780636fb81cbb146104d35780637b993bf3146104e8578063850d9758146105395780638a5a9d07146105b2578063aff125f6146105e9578063bec171e51461064c578063bf5f1e83146106ba578063e9944a81146106fc578063f769ac481461078d578063f9f51401146107f0575b600080fd5b341561013657600080fd5b6101556004808035906020019091908035906020019091905050610872565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156101a257600080fd5b6101b86004808035906020019091905050610928565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561020557600080fd5b6102246004808035906020019091908035906020019091905050610976565b005b341561023157600080fd5b6102476004808035906020019091905050610f95565b6040518082815260200191505060405180910390f35b341561026857600080fd5b610270610fed565b6040518082815260200191505060405180910390f35b341561029157600080fd5b6102a76004808035906020019091905050610ffa565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156102f457600080fd5b61034160048080359060200190820180359060200190808060200260200160405190810160405280939291908181526020018383602002808284378201915050505050509190505061103a565b6040518082815260200191505060405180910390f35b341561036257600080fd5b61037860048080359060200190919050506110a9565b6040518082815260200191505060405180910390f35b341561039957600080fd5b6103e66004808035906020019082018035906020019080806020026020016040519081016040528093929190818152602001838360200280828437820191505050505050919050506110db565b6040518082815260200191505060405180910390f35b341561040757600080fd5b61043c600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091908035906020019091905050611172565b005b341561044957600080fd5b61046860048080359060200190919080359060200190919050506118b0565b604051808215151515815260200191505060405180910390f35b341561048d57600080fd5b6104b9600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050611977565b604051808215151515815260200191505060405180910390f35b34156104de57600080fd5b6104e6611a1d565b005b34156104f357600080fd5b61051f600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050611ab9565b604051808215151515815260200191505060405180910390f35b341561054457600080fd5b61055a6004808035906020019091905050611b5f565b6040518080602001828103825283818151815260200191508051906020019060200280838360005b8381101561059e5780820151818401525b602081019050610582565b505050509050019250505060405180910390f35b34156105bd57600080fd5b6105d36004808035906020019091905050611ca9565b6040518082815260200191505060405180910390f35b34156105f457600080fd5b61060a6004808035906020019091905050611cd7565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b341561065757600080fd5b6106a4600480803590602001908201803590602001908080602002602001604051908101604052809392919081815260200183836020028082843782019150505050505091905050611d17565b6040518082815260200191505060405180910390f35b34156106c557600080fd5b6106fa600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091908035906020019091905050611dae565b005b341561070757600080fd5b610773600480803573ffffffffffffffffffffffffffffffffffffffff169060200190919080359060200190820180359060200190808060200260200160405190810160405280939291908181526020018383602002808284378201915050505050509190505061285f565b604051808215151515815260200191505060405180910390f35b341561079857600080fd5b6107ae60048080359060200190919050506128de565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156107fb57600080fd5b61081a6004808035906020019091908035906020019091905050612a18565b6040518080602001828103825283818151815260200191508051906020019060200280838360005b8381101561085e5780820151818401525b602081019050610842565b505050509050019250505060405180910390f35b600060018311806108835750600282115b1561088d57600080fd5b60008314156108d7576006600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169050610922565b6001831415610921576006600201600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169050610922565b5b92915050565b6000808281548110151561093857fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b919050565b61097f33611977565b151561098a57600080fd5b600281111561099857600080fd5b60008114806109a75750600281145b8015610a405750610a3d6001805480602002602001604051908101604052809291908181526020018280548015610a3357602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116109e9575b505050505061103a565b82115b15610a4a57600080fd5b600181148015610ae75750610ae46002805480602002602001604051908101604052809291908181526020018280548015610ada57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610a90575b505050505061103a565b82115b15610af157600080fd5b6000811415610b0d57816009600001541415610b0c57600080fd5b5b6001811415610b2957816009600101541415610b2857600080fd5b5b6002811415610b4557816009600201541415610b4457600080fd5b5b6006600101600082815260200190815260200160002060000160009054906101000a900460ff161515610c875760016006600101600083815260200190815260200160002060000160006101000a81548160ff02191690831515021790555081600660010160008381526020019081526020016000206002018190555043600660010160008381526020019081526020016000206003018190555060006006600101600083815260200190815260200160002060010181610c069190613046565b50600660010160008281526020019081526020016000206001018054806001018281610c329190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050610e0e565b600554600660010160008381526020019081526020016000206003015443031115610cba57610cb581612c4a565b610f90565b816006600101600083815260200190815260200160002060020154141515610ce157600080fd5b610d883360066001016000848152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015610d7e57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610d34575b505050505061285f565b15610d9257600080fd5b600660010160008281526020019081526020016000206001018054806001018281610dbd9190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b600960020154610eba60066001016000848152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015610eb057602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311610e66575b5050505050611d17565b101515610f8f576000811480610ed05750600181145b15610ee057610edf6002612ce5565b5b6000811415610f0d5760066001016000828152602001908152602001600020600201546009600001819055505b6002811415610f2b57610f206000612ce5565b610f2a6001612ce5565b5b6001811415610f585760066001016000828152602001908152602001600020600201546009600101819055505b6002811415610f855760066001016000828152602001908152602001600020600201546009600201819055505b610f8e81612c4a565b5b5b5b5050565b60006002821115610fa557600080fd5b6000821415610fbb576009600001549050610fe8565b6001821415610fd1576009600101549050610fe8565b6002821415610fe7576009600201549050610fe8565b5b919050565b6000808054905090505b90565b60028181548110151561100957fe5b906000526020600020900160005b915054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b6000806000809050600091505b835182101561109e576000848381518110151561106057fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff161415156110905780806001019150505b5b8180600101925050611047565b8092505b5050919050565b600060028211156110b957600080fd5b600660010160008381526020019081526020016000206002015490505b919050565b6000806000809050600091505b8351821015611167576000848381518110151561110157fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff161415801561114b575061114a848381518110151561113b57fe5b90602001906020020151611ab9565b5b156111595780806001019150505b5b81806001019250506110e8565b8092505b5050919050565b60008061117e33611977565b151561118957600080fd5b60008473ffffffffffffffffffffffffffffffffffffffff1614156111ad57600080fd5b60018311156111bb57600080fd5b600083141561128b57611253600180548060200260200160405190810160405280929190818152602001828054801561124957602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116111ff575b505050505061103a565b915060096000015482148061126c575060096002015482145b1561127657600080fd5b61127f84611977565b151561128a57600080fd5b5b600183141561134957600960010154611329600280548060200260200160405190810160405280929190818152602001828054801561131f57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116112d5575b505050505061103a565b141561133457600080fd5b61133d84611ab9565b151561134857600080fd5b5b6006600201600084815260200190815260200160002060000160009054906101000a900460ff1615156114c55760016006600201600085815260200190815260200160002060000160006101000a81548160ff021916908315150217905550836006600201600085815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550436006600201600085815260200190815260200160002060030181905550600060066002016000858152602001908152602001600020600101816114449190613046565b506006600201600084815260200190815260200160002060010180548060010182816114709190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050611698565b6005546006600201600085815260200190815260200160002060030154430311156114f8576114f383612dba565b6118a9565b8373ffffffffffffffffffffffffffffffffffffffff166006600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1614151561156b57600080fd5b611612336006600201600086815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561160857602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116115be575b505050505061285f565b1561161c57600080fd5b6006600201600084815260200190815260200160002060010180548060010182816116479190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b6009600201546117446006600201600086815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561173a57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116116f0575b5050505050611d17565b1015156118a85760008314801561179957506117986006600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611977565b5b156117e2576117e1836006600201600086815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16612e8f565b5b600183148015611830575061182f6006600201600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611ab9565b5b1561187957611878836006600201600086815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16612e8f565b5b600090505b600381101561189e5761189081612ce5565b5b808060010191505061187e565b6118a783612dba565b5b5b5b50505050565b600060028311806118c15750600282115b156118cb57600080fd5b6000831415611902576006600001600083815260200190815260200160002060000160009054906101000a900460ff169050611971565b6001831415611939576006600101600083815260200190815260200160002060000160009054906101000a900460ff169050611971565b6002831415611970576006600201600083815260200190815260200160002060000160009054906101000a900460ff169050611971565b5b92915050565b600080600090505b600180549050811015611a12578273ffffffffffffffffffffffffffffffffffffffff166001828154811015156119b257fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415611a045760019150611a17565b5b808060010191505061197f565b600091505b50919050565b600460009054906101000a900460ff1615611a3757600080fd5b60018054806001018281611a4b9190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550506001600460006101000a81548160ff0219169083151502179055505b565b600080600090505b600280549050811015611b54578273ffffffffffffffffffffffffffffffffffffffff16600282815481101515611af457fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415611b465760019150611b59565b5b8080600101915050611ac1565b600091505b50919050565b611b6761309e565b6001821115611b7557600080fd5b6000821415611c0c576001805480602002602001604051908101604052809291908181526020018280548015611c0057602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611bb6575b50505050509050611ca4565b6001821415611ca3576002805480602002602001604051908101604052809291908181526020018280548015611c9757602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611c4d575b50505050509050611ca4565b5b919050565b60008082815481101515611cb957fe5b906000526020600020906002020160005b506000015490505b919050565b600181815481101515611ce657fe5b906000526020600020900160005b915054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b6000806000809050600091505b8351821015611da35760008483815181101515611d3d57fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff1614158015611d875750611d868483815181101515611d7757fe5b90602001906020020151611977565b5b15611d955780806001019150505b5b8180600101925050611d24565b8092505b5050919050565b611db733611977565b158015611dca5750611dc833611ab9565b155b15611dd457600080fd5b600081148015611e745750600354611e716001805480602002602001604051908101604052809291908181526020018280548015611e6757602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611e1d575b505050505061103a565b10155b15611e7e57600080fd5b600181148015611f1e5750600354611f1b6002805480602002602001604051908101604052809291908181526020018280548015611f1157602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311611ec7575b505050505061103a565b10155b15611f2857600080fd5b60008273ffffffffffffffffffffffffffffffffffffffff161415611f4c57600080fd5b6002811115611f5a57600080fd5b6000811480611f695750600181145b8015611f8a5750611f7982611977565b80611f895750611f8882611ab9565b5b5b15611f9457600080fd5b6006600001600082815260200190815260200160002060000160009054906101000a900460ff16151561212357611fca33611ab9565b15611fd457600080fd5b60016006600001600083815260200190815260200160002060000160006101000a81548160ff021916908315150217905550816006600001600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550436006600001600083815260200190815260200160002060030181905550600060066000016000838152602001908152602001600020600101816120a29190613046565b506006600001600082815260200190815260200160002060010180548060010182816120ce9190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550506122f6565b6005546006600001600083815260200190815260200160002060030154430311156121565761215181612ce5565b61285a565b8173ffffffffffffffffffffffffffffffffffffffff166006600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415156121c957600080fd5b612270336006600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561226657602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001906001019080831161221c575b505050505061285f565b1561227a57600080fd5b6006600001600082815260200190815260200160002060010180548060010182816122a59190613072565b916000526020600020900160005b33909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b60008114806123055750600181145b156125ab576009600201546123b6600660000160008481526020019081526020016000206001018054806020026020016040519081016040528092919081815260200182805480156123ac57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612362575b5050505050611d17565b1015156125aa576123ff6006600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611977565b8061244857506124476006600001600083815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16611ab9565b5b1561245257600080fd5b60008114156124f9576001805480600101828161246f9190613072565b916000526020600020900160005b6006600001600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b60018114156125a057600280548060010182816125169190613072565b916000526020600020900160005b6006600001600085815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff160217905550505b6125a981612ce5565b5b5b6002811415612859576009600001546126606006600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561265657602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001906001019080831161260c575b5050505050611d17565b1015801561271857506009600101546127156006600001600084815260200190815260200160002060010180548060200260200160405190810160405280929190818152602001828054801561270b57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190600101908083116126c1575b50505050506110db565b10155b15612858576000808054905011801561275f575060014301600060016000805490500381548110151561274757fe5b906000526020600020906002020160005b5060000154145b1561276957600080fd5b6000805480600101828161277d91906130b2565b916000526020600020906002020160005b60408051908101604052806001430181526020016006600001600087815260200190815260200160002060020160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681525090919091506000820151816000015560208201518160010160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050505061285781612ce5565b5b5b5b5b5050565b600080600090505b82518110156128d2578373ffffffffffffffffffffffffffffffffffffffff16838281518110151561289557fe5b9060200190602002015173ffffffffffffffffffffffffffffffffffffffff1614156128c457600191506128d7565b5b8080600101915050612867565b600091505b5092915050565b6000806000808054905014156128f75760009150612a12565b60016000805490500390505b6000811115612994578260008281548110151561291c57fe5b906000526020600020906002020160005b50600001541115156129855760008181548110151561294857fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169150612a12565b5b808060019003915050612903565b826000808154811015156129a457fe5b906000526020600020906002020160005b5060000154111515612a0d576000808154811015156129d057fe5b906000526020600020906002020160005b5060010160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff169150612a12565b600091505b50919050565b612a2061309e565b6002831180612a2f5750600282115b15612a3957600080fd5b6000831415612ae75760066000016000838152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015612adb57602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612a91575b50505050509050612c44565b6001831415612b955760066001016000838152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015612b8957602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612b3f575b50505050509050612c44565b6002831415612c435760066002016000838152602001908152602001600020600101805480602002602001604051908101604052809291908181526020018280548015612c3757602002820191906000526020600020905b8160009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019060010190808311612bed575b50505050509050612c44565b5b92915050565b6000600660010160008381526020019081526020016000206002018190555060006006600101600083815260200190815260200160002060010181612c8f9190613046565b506000600660010160008381526020019081526020016000206003018190555060006006600101600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b60006006600001600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060006006600001600083815260200190815260200160002060010181612d649190613046565b506000600660000160008381526020019081526020016000206003018190555060006006600001600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b60006006600201600083815260200190815260200160002060020160006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060006006600201600083815260200190815260200160002060010181612e399190613046565b506000600660020160008381526020019081526020016000206003018190555060006006600201600083815260200190815260200160002060000160006101000a81548160ff0219169083151502179055505b50565b600080831415612f6857600090505b600180549050811015612f67578173ffffffffffffffffffffffffffffffffffffffff16600182815481101515612ed157fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff161415612f5957600181815481101515612f2957fe5b906000526020600020900160005b6101000a81549073ffffffffffffffffffffffffffffffffffffffff02191690555b5b8080600101915050612e9e565b5b600183141561304057600090505b60028054905081101561303f578173ffffffffffffffffffffffffffffffffffffffff16600282815481101515612fa957fe5b906000526020600020900160005b9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1614156130315760028181548110151561300157fe5b906000526020600020900160005b6101000a81549073ffffffffffffffffffffffffffffffffffffffff02191690555b5b8080600101915050612f76565b5b5b505050565b81548183558181151161306d5781836000526020600020918201910161306c91906130e4565b5b505050565b8154818355818115116130995781836000526020600020918201910161309891906130e4565b5b505050565b602060405190810160405280600081525090565b8154818355818115116130df576002028160020283600052602060002091820191016130de9190613109565b5b505050565b61310691905b808211156131025760008160009055506001016130ea565b5090565b90565b61315491905b80821115613150576000808201600090556001820160006101000a81549073ffffffffffffffffffffffffffffffffffffffff02191690555060020161310f565b5090565b905600a165627a7a723058203193cc570fd198d6b9da1b2fcac2a6332e140446e820454c1ac4467f811341e30029", 4000000, QTUM_MIN_GAS_PRICE/COIN)
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
        contract_data = self.node.createcontract("60606040526104e060405190810160405280600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161138861ffff16815260200161600061ffff1681525060009060276101df9291906101ed565b5034156101e857fe5b6102c1565b82602760070160089004810192821561027d5791602002820160005b8382111561024b57835183826101000a81548163ffffffff021916908361ffff1602179055509260200192600401602081600301049283019260010302610209565b801561027b5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261024b565b505b50905061028a919061028e565b5090565b6102be91905b808211156102ba57600081816101000a81549063ffffffff021916905550600101610294565b5090565b90565b61016a806102d06000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003b575bfe5b341561004357fe5b61004b610097565b6040518082602760200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000602780602002604051908101604052809291908260278015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6104e0604051908101604052806027905b600063ffffffff1681526020019060019003908161012057905050905600a165627a7a72305820a5a4ef1cfc85ef81e4f55205cd6a07d4cd7f65c3e87f1f5dd2f0fd8e22471c910029", 10000000, QTUM_MIN_GAS_PRICE/COIN)
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
        self.node.generate(1)
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
        self.node.generate(1)
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
        self.node.generate(1)
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
        self.node.generate(1)
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
        self.node.generate(1)
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
        self.node.generate(1)
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
        self.node.generate(1)
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


        # Tests for the proposal expiration
        # Note: We set a lower block expiration limit in the contract above to avoid having to generate 21600 blocks,
        # instead we set proposals to expire after BLOCKS_BEFORE_PROPOSAL_EXPIRATION (216) blocks.

        # Verify that a remove address proposal is removed after the correct amount of blocks
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = True
        state.current_on_vote_address_proposals[0][2] = self.proposal_address
        state.assert_state()

        # Now generate 215 blocks and make sure that the proposal still exists.
        for i in range(BLOCKS_BEFORE_PROPOSAL_EXPIRATION-1):
            self.node.generate(1)
            state.assert_state()

        # When we send another vote it should not be removed
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Now it should be removed after sending a new vote, this implicitly also verifies that the proposal was not accepted
        state.send_add_address_proposal(self.proposal_address, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[0][2] = False
        state.current_on_vote_address_proposals[0][2] = "0"
        state.assert_state()


        # Verify that a change value proposal is removed after the correct amount of blocks
        state.send_change_value_proposal(10, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][2] = True
        state.assert_state()

        # Now generate 216 blocks and make sure that the proposal still exists.
        for i in range(BLOCKS_BEFORE_PROPOSAL_EXPIRATION-1):
            self.node.generate(1)
            state.assert_state()

        # When we send another vote it should not be removed
        state.send_change_value_proposal(10, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Check that it is removed after sending a new vote, this implicitly also verifies that the proposal was not accepted
        state.send_change_value_proposal(10, 2, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[1][2] = False
        state.assert_state()


        # Verify that a remove address proposal is removed after the correct amount of blocks
        state.send_remove_address_proposal(state.gov_keys[0], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][1] = True
        state.current_on_vote_address_proposals[1][1] = state.gov_keys[0]
        state.assert_state()

        # Now generate 216 blocks and make sure that the proposal still exists.
        for i in range(BLOCKS_BEFORE_PROPOSAL_EXPIRATION-1):
            self.node.generate(1)
            state.assert_state()

        # When we send another vote it should not be removed
        state.send_remove_address_proposal(state.gov_keys[0], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.assert_state()

        # Check that it is removed after sending a new vote, this implicitly also verifies that the proposal was not accepted
        state.send_remove_address_proposal(state.gov_keys[0], 1, keyhash_to_p2pkh(hex_str_to_bytes(state.admin_keys[1])))
        self.node.generate(1)
        state.current_on_vote_statuses[2][1] = False
        state.current_on_vote_address_proposals[1][1] = "0"
        state.assert_state()

if __name__ == '__main__':
    QtumDGPTest().main()
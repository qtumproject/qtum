#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
from test_framework.util import *
import pprint
import shutil
pp = pprint.PrettyPrinter()


OFFLINE_STAKING_ACTIVATION_HEIGHT = 3*COINBASE_MATURITY+101

class QtumSimpleDelegationContractTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6
        self.extra_args = [ ['-txindex=1', '-logevents=1', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'],
                            ['-txindex=1', '-logevents=1', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'],
                            ['-txindex', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'],
                            ['-logevents', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'],
                            ['-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'],
                            ['-txindex=1', '-logevents=1', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_unused_delegator_prevouts(self):
        delegator_prevouts = collect_prevouts(self.delegator, address=self.delegator_address, min_confirmations=COINBASE_MATURITY)
        i = 0
        while i < len(delegator_prevouts):
            for used_delegator_prevout in self.used_delegator_prevouts:
                if delegator_prevouts[i][0].serialize() == used_delegator_prevout.serialize():
                    delegator_prevouts.pop(i)
                    break
            else:
                i += 1
        return delegator_prevouts

    def remove_delegation(self):
        abi = "3d666e8b"
        txid = self.delegator.sendtocontract(DELEGATION_CONTRACT_ADDRESS, abi, 0, 200000, 0.00000040, self.delegator_address)['txid']
        self.delegator.generate(1)
        receipt = self.delegator.gettransactionreceipt(txid)[0]

    def stake_one_block(self, node, t, timeout=600):
        blockcount = node.getblockcount()
        for i in range(0, 10):
            for n in self.nodes: n.setmocktime(t)

            for j in range(0, timeout):
                if node.getblockcount() != blockcount:
                    return
                time.sleep(0.1)
            t += 21
            print(node.getstakinginfo())


        print(node.getstakinginfo())
        assert(False)

    def vary_fee_test(self):
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        for fee in range(0, 101, 20): # inc by 5 to lower the execution time
            # send the evm tx that delegates the address to the staker address
            delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
            self.delegator.generatetoaddress(1, self.delegator_address)
            t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
            self.sync_all()
            t += 0x10
            for n in self.nodes: n.setmocktime(t+5)


            delegator_prevouts = self.get_unused_delegator_prevouts()
            staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
            staker_prevout_for_nas = staker_prevouts.pop(0)[0]
            use_pos_reward = True if self.staker.getblockcount() > 5000 else False
            block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
            assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), None)
            assert_equal(self.staker.getbestblockhash(), block.hash)
            self.used_delegator_prevouts.append(block.prevoutStake)
            self.sync_all()

    """
        1. Generate a delegated staking block
        2. Generate blocks the maturity number of blocks - 1
        3. Check if it is possible to submit a new block reusing the delegated staking prevout from 1. (should fail)
        4. Generate one new block
        5. Redo 3. (should succeed)
    """
    def cannot_reuse_delegated_stake_for_offline_staking_within_maturity_interval_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        # 1.
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 50
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        assert_equal(self.staker.submitblock(bytes_to_hex_str(block.serialize())), None)
        assert_equal(self.staker.getbestblockhash(), block.hash)
        self.sync_all()

        # 2.
        generatesynchronized(self.staker, COINBASE_MATURITY-2, self.staker_address, self.nodes)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        dupe_delegator_prevouts = []
        for delegator_prevout in delegator_prevouts:
            if delegator_prevout[0].serialize() == block.prevoutStake.serialize():
                dupe_delegator_prevouts.append(delegator_prevout)

        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        dupe_prevout_block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, dupe_delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        while dupe_prevout_block == None:
            for n in self.nodes: n.setmocktime(t+5)
            t += 0x10
            dupe_prevout_block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, dupe_delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)

        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(dupe_prevout_block.serialize())), 'stake-prevout-not-mature')
        for dupe_delegator_prevout in dupe_delegator_prevouts:
            self.used_delegator_prevouts.append(dupe_delegator_prevout[0])


        # generate one more block and submit the duplicate
        self.staker.generatetoaddress(1, self.staker_address)
        t += 0x10
        dupe_prevout_block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, dupe_delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        while dupe_prevout_block == None:
            t += 0x10
            for n in self.nodes: n.setmocktime(t+5)
            dupe_prevout_block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, dupe_delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        assert_equal(self.staker.submitblock(bytes_to_hex_str(dupe_prevout_block.serialize())), None)



    """
        Should this behavior be here? (probably a good idea to restrict online staking while there is offline staking enabled, simplifies analysis)
    """
    def cannot_do_online_staking_while_active_for_offline_staking_test(self):
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 51
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.delegator.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0

        delegator_prevouts = self.get_unused_delegator_prevouts()
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        tmp_normal_block = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=t)
        while tmp_normal_block == None:
            t += 0x10
            for n in self.nodes: n.setmocktime(t+5)
            tmp_normal_block = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=t)
        tmp_normal_block = tmp_normal_block[0]

        tmp_normal_block.vtx[1].vout[1].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        tmp_normal_block.vtx[1].vout[2].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        tmp_normal_block.vtx[1] = rpc_sign_transaction(self.delegator, tmp_normal_block.vtx[1])
        tmp_normal_block.vtx[1].rehash()
        tmp_normal_block.hashMerkleRoot = tmp_normal_block.calc_merkle_root()
        tmp_normal_block.rehash()
        tmp_normal_block.sign_block(self.delegator_eckey)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(tmp_normal_block.serialize())), 'stake-delegation-not-used')


    def staker_must_be_the_delegated_staker_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 52
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]
        invalid_staker_prevout_for_nas = delegator_prevouts.pop(-1)[0]

        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        block.vtx[1].vout[1].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        block.vtx[1].vin[0] = CTxIn(invalid_staker_prevout_for_nas)
        block.vtx[1] = rpc_sign_transaction(self.delegator, block.vtx[1])
        block.vtx[1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.sign_block(self.delegator_eckey, pod=pod)
        block.vchBlockSig += pod
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'bad-cb-header')


    """
        - Check that it is not possible to use an input with less value than 100 qtum as the first input to the coinstake.
        - Check that 100 qtum is just enough for the first input to the coinstake.
    """
    def nas_staker_must_be_100_qtum_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        staker_prevouts = collect_prevouts(self.staker, min_confirmations=COINBASE_MATURITY, amount=20000)
        prevout = staker_prevouts[0]
        tx = CTransaction()
        tx.vin.append(CTxIn(prevout[0]))
        tx.vout = [CTxOut(9900000000, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(self.staker_address_hex), OP_EQUALVERIFY, OP_CHECKSIG])) for i in range(0, 100)]
        tx.vout += [CTxOut(10000000000, scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(self.staker_address_hex), OP_EQUALVERIFY, OP_CHECKSIG])) for i in range(0, 10)]
        tx.vout.append(CTxOut(int((20000-9900-1000-0.15)*COIN), scriptPubKey=CScript([OP_DUP, OP_HASH160, hex_str_to_bytes(self.staker_address_hex), OP_EQUALVERIFY, OP_CHECKSIG])))
        tx = rpc_sign_transaction(self.staker, tx)
        self.staker.sendrawtransaction(bytes_to_hex_str(tx.serialize()))
        generatesynchronized(self.staker, COINBASE_MATURITY, self.staker_address, self.nodes)
        self.sync_all()

        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 53
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        delegator_prevouts = self.get_unused_delegator_prevouts()

        staker_prevouts = collect_prevouts(self.staker, min_confirmations=COINBASE_MATURITY, amount=99)
        too_low_staker_prevout_for_nas = staker_prevouts.pop(0)[0]
        block = create_delegated_pos_block(self.staker, self.staker_eckey, too_low_staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'stake-delegation-not-min-utxo')

        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)
        
        staker_prevouts = collect_prevouts(self.staker, min_confirmations=COINBASE_MATURITY, amount=100)
        just_enough_staker_prevout_for_nas = staker_prevouts.pop(0)[0]
        block = create_delegated_pos_block(self.staker, self.staker_eckey, just_enough_staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        assert_equal(self.delegator.submitblock(bytes_to_hex_str(block.serialize())), None)
        self.used_delegator_prevouts.append(block.prevoutStake)


    def invalid_op_return_coinstake_output_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 54
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x20
        for n in self.nodes: n.setmocktime(t+0x15)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        block.vtx[1].vout[1].scriptPubKey = CScript([OP_RETURN, self.staker_eckey.get_pubkey().get_bytes()])
        block.vtx[1] = rpc_sign_transaction(self.staker, block.vtx[1])
        block.vtx[1].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.sign_block(self.staker_eckey, pod=pod)
        block.vchBlockSig += pod
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'stake-verify-delegation-failed')


    def cannot_do_delegated_staking_after_removal_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        # Create a delegation
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 55
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)

        # remove the delegation
        self.remove_delegation()
        self.delegator.generatetoaddress(1, self.delegator_address)

        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        t += 0x10
        for n in self.nodes: n.setmocktime(t+5)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'bad-delegate-output')
        self.used_delegator_prevouts.append(block.prevoutStake)


        for n in self.nodes: n.setmocktime(t+5)
        t += 0x10
        # Make sure that we can do normal staking after the removal test
        normal_block = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=t)[0]
        normal_block.vtx[1].vout[1].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1].vout[2].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1] = rpc_sign_transaction(self.delegator, normal_block.vtx[1])
        normal_block.vtx[1].rehash()
        normal_block.hashMerkleRoot = normal_block.calc_merkle_root()
        normal_block.rehash()
        normal_block.sign_block(self.delegator_eckey)
        assert_equal(self.delegator.submitblock(bytes_to_hex_str(normal_block.serialize())), None)
        self.used_delegator_prevouts.append(block.prevoutStake)


    def cannot_do_delegation_before_activation_height_test(self):
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        abi = get_delegate_abi(self.staker_address, 50, pod)
        # this should fail, since the contract address does not exist yet
        assert_raises_rpc_error(-5, 'contract address does not exist', self.delegator.sendtocontract, DELEGATION_CONTRACT_ADDRESS, abi, 0, 2250000, 0.00000040, self.delegator_address)


    def cannot_submit_delegated_block_before_activation_height_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 50

        self.sync_all()
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)
        print(t)

        print(self.staker.getblockcount())

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        print(block.nTime)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'bad-cb-header')
        self.sync_all()

    def cannot_submit_compact_sig_block_before_activation_height_test(self):
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        normal_block = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=t)[0]
        normal_block.vtx[1].vout[1].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1].vout[2].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1] = rpc_sign_transaction(self.delegator, normal_block.vtx[1])
        normal_block.vtx[1].rehash()
        normal_block.hashMerkleRoot = normal_block.calc_merkle_root()
        normal_block.rehash()
        normal_block.sign_block(self.delegator_eckey)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(normal_block.serialize())), 'bad-cb-header')
        #self.used_delegator_prevouts.append(block.prevoutStake)

    def can_submit_der_sig_block_before_activation_height_test(self):
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        normal_block = create_unsigned_pos_block(self.delegator, delegator_prevouts, nTime=t)[0]
        normal_block.vtx[1].vout[1].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1].vout[2].scriptPubKey = CScript([self.delegator_eckey.get_pubkey().get_bytes(), OP_CHECKSIG])
        normal_block.vtx[1] = rpc_sign_transaction(self.delegator, normal_block.vtx[1])
        normal_block.vtx[1].rehash()
        normal_block.hashMerkleRoot = normal_block.calc_merkle_root()
        normal_block.rehash()
        normal_block.sign_block(self.delegator_eckey, der_sig=True)
        assert_equal(self.delegator.submitblock(bytes_to_hex_str(normal_block.serialize())), None)


    def advance_to_activation_of_offlinestaking(self):
        # Allow the staker itself to advance past the activation block (to make sure there is no off-by-one error between the validation and the staker code)
        generatesynchronized(self.staker, OFFLINE_STAKING_ACTIVATION_HEIGHT-3-self.staker.getblockcount(), self.staker_address, self.nodes)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        self.stop_node(1)
        # delete the wallet so that the node will not immediately begin staking
        # We need to have time so set the correct mocktime before
        shutil.rmtree(self.wallet_path)
        shutil.copytree(self.wallet_backup_path, self.wallet_path)
        self.start_node(1, ['-txindex=1', '-logevents=1', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-staking=1', '-londonheight=100000'])
        #self.restart_node(1, ['-txindex=1', '-logevents=1', '-offlinestakingheight=1601'])
        t += 0x20
        for n in self.nodes: n.setmocktime(t)
        
        for i in range(len(self.nodes)):
            if i == 1: continue
            self.connect_nodes(1, i)


        # We need to generate one block to get clear the staking cache, since it will effectively cache forever due to the diff between mocktime and "real" time at startup
        generatesynchronized(self.staker, 1, None, self.nodes)

        # reimport the staker privkey to begin staking
        self.staker.importprivkey(self.staker_privkey)

        self.stake_one_block(self.staker, t)

        self.sync_all()

        t += 0x10

        assert_raises_rpc_error(-5, 'Address does not exist', self.staker.getaccountinfo, DELEGATION_CONTRACT_ADDRESS)

        self.stake_one_block(self.staker, t)
        self.sync_all()
        

        # Done, we should be active with offlinestaking now, which means that the delegation contract should exist
        self.staker.getaccountinfo(DELEGATION_CONTRACT_ADDRESS)
        self.restart_node(1, ['-txindex=1', '-logevents=1', '-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'])
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        for n in self.nodes: n.setmocktime(t+5)
        for i in range(len(self.nodes)):
            if i == 1: continue
            self.connect_nodes(1, i)



    """
    First create a chain that advances a few blocks via delegated staking, then remove the address from delegated staking
    Submit a block that forks before the removal and make sure that it succeeds.
    """
    def delegation_can_continue_on_forked_chain_before_removal_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 58
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)
        self.delegator.generatetoaddress(1, self.delegator_address)
        self.sync_all()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        # Create a delegated block but do not submit it
        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)

        # We split the chain at this point.
        # Node 4 will mine on a separate chain from the rest of the nodes
        # for Node 0-3 the delegation will be removed, but not for node 4
        self.restart_node(4, ['-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'])
        for n in self.nodes: n.setmocktime(t+5)
        assert_equal(self.nodes[4].getpeerinfo(), [])

        self.remove_delegation()
        self.staker.generatetoaddress(2, self.staker_address)

        self.nodes[4].submitblock(bytes_to_hex_str(block.serialize()), None)
        self.nodes[4].generatetoaddress(4, self.staker_address)
        time.sleep(2)

        for i in range(len(self.nodes)):
            if i == 4: continue
            self.connect_nodes(4, i)
        self.sync_blocks()

        for node_send in self.nodes:
            if node_send.getrawmempool():
                for node_receive in self.nodes:
                    node_receive.sendrawtransaction(node_send.getrawtransaction(node_send.getrawmempool()[0], False))
        self.staker.generatetoaddress(1, self.staker_address)
        self.used_delegator_prevouts.append(block.prevoutStake)


    def delegation_cannot_continue_if_not_existing_after_reorg_test(self):
        use_pos_reward = True if self.staker.getblockcount() > 5000 else False
        self.remove_delegation()
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)
        self.delegator.generatetoaddress(1, self.delegator_address)

        self.restart_node(4, ['-offlinestakingheight=' + str(OFFLINE_STAKING_ACTIVATION_HEIGHT), '-londonheight=100000'])
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & 0xfffffff0
        for n in self.nodes: n.setmocktime(t+5)
        assert_equal(self.nodes[4].getpeerinfo(), [])

        # Create a new delegation
        pod = create_POD(self.delegator, self.delegator_address, self.staker_address)
        fee = 58
        delegate_to_staker(self.delegator, self.delegator_address, self.staker_address, fee, pod)

        # Make a longer chain
        best_block_hash = self.nodes[4].generatetoaddress(10, self.staker_address)[-1]

        # and reconnect
        for i in range(len(self.nodes)):
            if i == 4: continue
            self.connect_nodes(4, i)

        self.sync_blocks()
        assert_equal(self.staker.getbestblockhash(), best_block_hash)

        delegator_prevouts = self.get_unused_delegator_prevouts()
        staker_prevouts = collect_prevouts(self.staker, address=self.staker_address, min_confirmations=COINBASE_MATURITY, min_amount=100)
        staker_prevout_for_nas = staker_prevouts.pop(0)[0]

        # Create a delegated block but do not submit it
        t = self.staker.getblock(self.staker.getbestblockhash())['time'] & (0xffffffff - TIMESTAMP_MASK)
        t += TIMESTAMP_MASK+1
        for n in self.nodes: n.setmocktime(t)
        block = create_delegated_pos_block(self.staker, self.staker_eckey, staker_prevout_for_nas, self.delegator_address_hex, pod, fee, delegator_prevouts, nFees=0, nTime=t, use_pos_reward=use_pos_reward)
        for n in self.nodes:
            assert_equal(n.submitblock(bytes_to_hex_str(block.serialize())), 'bad-delegate-output')
        self.staker.generatetoaddress(1, self.staker_address)


    def run_test(self):
        for n in self.nodes: n.setmocktime(int(time.time()) - 1199)
        self.used_delegator_prevouts = []
        self.delegator = self.nodes[0]
        self.delegator_address = self.delegator.getnewaddress()
        self.delegator_address_hex = p2pkh_to_hex_hash(self.delegator_address)
        self.delegator_privkey = self.delegator.dumpprivkey(self.delegator_address)
        self.delegator_eckey = wif_to_ECKey(self.delegator_privkey)

        self.staker = self.nodes[1]
        self.staker_address = self.staker.getnewaddress()
        self.staker_address_hex = p2pkh_to_hex_hash(self.staker_address)
        self.staker_privkey = self.staker.dumpprivkey(self.staker_address)
        self.staker_eckey = wif_to_ECKey(self.staker_privkey)

        self.wallet_clean_path = os.path.join(self.nodes[5].datadir, 'regtest', 'wallets')
        self.wallet_path = os.path.join(self.staker.datadir, 'regtest', 'wallets')
        self.wallet_backup_path = os.path.join(self.staker.datadir, 'regtest', 'backup_wallets')
        shutil.copytree(self.wallet_clean_path, self.wallet_backup_path)

        self.staker.generatetoaddress(1, self.staker_address)
        self.sync_all()
        generatesynchronized(self.delegator, COINBASE_MATURITY+100, self.delegator_address, self.nodes)
        self.sync_all()
        generatesynchronized(self.staker, COINBASE_MATURITY, self.staker_address, self.nodes)
        self.sync_all()


        # Now let's do some tests BEFORE delegated staking is activated
        print("cannot_do_delegation_before_activation_height_test")
        self.cannot_do_delegation_before_activation_height_test()
        self.sync_all()

        print("cannot_submit_delegated_block_before_activation_height_test")
        self.cannot_submit_delegated_block_before_activation_height_test()
        self.sync_all()

        print("cannot_submit_compact_sig_block_before_activation_height_test")
        self.cannot_submit_compact_sig_block_before_activation_height_test()
        self.sync_all()

        print("can_submit_der_sig_block_before_activation_height_test")
        self.can_submit_der_sig_block_before_activation_height_test()
        self.sync_all()

        # activate offlinestaking
        print("advance_to_activation_of_offlinestaking")
        self.advance_to_activation_of_offlinestaking()
        self.sync_all()

        # begin offlinestaking tests
        print("vary_fee_test")
        self.vary_fee_test()
        self.sync_all()

        print("cannot_reuse_delegated_stake_for_offline_staking_within_maturity_interval_test")
        self.cannot_reuse_delegated_stake_for_offline_staking_within_maturity_interval_test()
        self.sync_all()
        
        print("cannot_do_online_staking_while_active_for_offline_staking_test")
        self.cannot_do_online_staking_while_active_for_offline_staking_test()
        self.sync_all()

        print("staker_must_be_the_delegated_staker_test")
        self.staker_must_be_the_delegated_staker_test()
        self.sync_all()

        print("nas_staker_must_be_100_qtum_test")
        self.nas_staker_must_be_100_qtum_test()
        self.sync_all()
        
        print("invalid_op_return_coinstake_output_test")
        self.invalid_op_return_coinstake_output_test()
        self.sync_all()
        
        print("cannot_do_delegated_staking_after_removal_test")
        self.cannot_do_delegated_staking_after_removal_test()
        self.sync_all()

        print("delegation_can_continue_on_forked_chain_before_removal_test")
        self.delegation_can_continue_on_forked_chain_before_removal_test()
        self.sync_all()
        
        print("delegation_cannot_continue_if_not_existing_after_reorg_test")
        self.delegation_cannot_continue_if_not_existing_after_reorg_test()
        self.sync_all()

if __name__ == '__main__':
    QtumSimpleDelegationContractTest().main()

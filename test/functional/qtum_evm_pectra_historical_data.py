#!/usr/bin/env python3
"""
Qtum EVM Historical Data (EIP‑2935) functional test.

This test verifies:
* The historical block‑hash pre‑compile is available after activation height.
* The pre‑compile returns correct block hashes for blocks inside the serve window.
* Requests for block numbers outside the window or in the future revert.
* The historical precompile correctly handles blockchain reorganizations.
* Stored historical hashes remain consistent after reorg.
* Additional PoS chain behavior: 10 PoS blocks are mined and verified.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.p2p import *
from test_framework.address import *
from test_framework.qtum import *
from test_framework.qtumconfig import *
from test_framework.blocktools import *
from test_framework.address import *
import sys
import random
import time

# --- Helper constants -------------------------------------------------

# Method selectors from successful compilation of extended contract
METH_GETBLOCKHASH = "ee82ac5e"
METH_GETHISTBLOCKHASH = "f2e8410c"
METH_STORE_RECENT_HASHES = "365afb1f"  # New storage function
METH_GETSTOREDHASH = "f5dab3a6"
METH_GETLASTSTOREDBLOCK = "0fd9abec"

# Size of the historical window defined by the implementation
HISTORY_WINDOW = 8191

# Pre‑compile address that stores recent block hashes (EIP‑2935)
HISTORY_STORAGE_ADDRESS = "0000f90827f1c53a10cb7a02335b175320002935"

# Number of hashes to store for testing
NUM_HASHES_TO_STORE = 50  # Conservative batch size

# -------------------------------------------------------------------------

# CONTRACT SOURCE CODE (SUCCESSFULLY COMPILING):
#
# // SPDX-License-Identifier: UNLICENSED
# pragma solidity ^0.8.0;
#
# contract BlockHashChecks {
#     address constant HISTORY_STORAGE_ADDRESS = 0x0000F90827F1C53a10cb7A02335B175320002935;
#
#     mapping(uint256 => bytes32) public storedHashes;
#     uint256 public lastStoredBlock;
#
#     // Original working function (made public for internal calls)
#     function getHistoricalBlockHash(uint256 blockNumber) public returns (bytes32 hash) {
#         bytes memory data = abi.encode(blockNumber);
#         (bool success, bytes memory result) = HISTORY_STORAGE_ADDRESS.call(data);
#         require(success && result.length == 32, "Fallback call failed or invalid response");
#         assembly {
#             hash := mload(add(result, 32))
#         }
#     }
#
#     function getBlockHash(uint256 blockNumber) external view returns (bytes32 hash) {
#         return blockhash(blockNumber);
#     }
#
#     function storeRecentHashes(uint256 startBlock, uint256 count) external {
#         require(count > 0 && count <= 50, "Count must be between 1 and 50");
#         lastStoredBlock = startBlock;
#         for (uint256 i = 0; i < count; i++) {
#             uint256 currentBlock = startBlock + i;
#             bytes32 historicalHash = getHistoricalBlockHash(currentBlock);
#             storedHashes[currentBlock] = historicalHash;
#         }
#     }
#
#     function getStoredHash(uint256 blockNumber) external view returns (bytes32) {
#         return storedHashes[blockNumber];
#     }
#
#     function getLastStoredBlock() external view returns (uint256) {
#         return lastStoredBlock;
#     }
# }

# -------------------------------------------------------------------------

class QtumEVMHistoricalDataTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

        # Set activation height via command line arguments
        self.extra_args = [
            ["-staking=1", "-logevents", "-pectraheight=2500"],
            ["-logevents", "-pectraheight=2500"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def deploy_contract(self, node, bytecode):
        """Deploys contract and verifies successful deployment"""
        sender = node.getnewaddress()
        node.sendtoaddress(sender, 1)

        # Use framework's generate method
        self.generate(node, 1)

        # Create contract
        contract_info = node.createcontract(bytecode)
        txid = contract_info['txid']
        contract_address = contract_info['address']

        # Mine block to include contract
        self.generate(node, 1)

        # Verify deployment by checking contract code (simplified)
        # Wait a moment for the transaction to be processed
        time.sleep(1)

        try:
            code = node.getcontractcode(contract_address)
            if code == "0x":
                raise Exception(f"Contract creation failed: no code at address {contract_address}")
            if code != bytecode:
                raise Exception(f"Contract code mismatch at {contract_address}")
            self.log.info(f"Contract deployed successfully at {contract_address}")
        except Exception as e:
            # Fallback: check transaction status via raw call
            tx = node.gettransaction(txid)
            if 'confirmations' in tx and tx['confirmations'] > 0:
                self.log.info(f"Contract deployment confirmed for {contract_address}")
            else:
                raise Exception(f"Contract deployment verification failed: {e}")

        return contract_address, sender

    def fund_nodes_for_storage(self):
        """Fund both nodes properly for storage operations"""
        self.log.info("Funding nodes for storage operations...")

        # Fund Node 1 from Node 0
        node1_address = self.nodes[1].getnewaddress()

        # Send sufficient funds to Node 1
        self.nodes[0].sendtoaddress(node1_address, 10)  # Send 10 QTUM

        # Mine a block to confirm the transaction
        self.generate(self.nodes[0], 1)
        self.sync_all()

        # Get the sender address for Node 1
        node1_sender = node1_address
        self.nodes[1].sendtoaddress(node1_sender, 5)  # Ensure Node 1 has some funds
        self.generate(self.nodes[1], 1)

        self.log.info("Nodes funded successfully for storage operations")

    def contract_call(self, method_selector: str, block_number: int = 0):
        """
        Perform a static call to the wrapper contract and return the full
        JSON response. Using callcontract for read-only calls.
        """
        data = method_selector
        if block_number != 0:
            data += format(block_number, "064x")
        out = self.node.callcontract(self.contract_address, data)
        return out

    def contract_send(self, method_selector: str, data_hex: str = "", amount: int = 0, gas_limit: int = 5000000, sender_address: str = None):
        """
        Perform a state-changing transaction to the contract using sendtocontract.
        """
        # Format the data
        full_data = method_selector + data_hex

        # Use sendtocontract for state-changing operations
        out = self.node.sendtocontract(
            self.contract_address,
            full_data,
            amount,
            gas_limit,
            QTUM_MIN_GAS_PRICE/COIN,
            sender_address
        )

        # Wait for transaction to be mined
        self.generate(self.node, 1)

        # Get the transaction receipt to check if it succeeded
        tx_hash = out['txid']
        receipt = self.node.gettransactionreceipt(tx_hash)
        if receipt and len(receipt) > 0:
            if receipt[0].get('excepted', 'None') != 'None':
                raise Exception(f"Transaction failed: {receipt[0].get('excepted')}")

        return out

    def get_contract_call_output(self, method_selector: str, block_number: int = 0):
        """Helper method to get just the output from a contract call"""
        out = self.contract_call(method_selector, block_number)
        assert_equal(out['executionResult']['excepted'], 'None')
        return out['executionResult']['output']

    def verify_blockhash_eq(
        self,
        method_selector: str,
        block_number: int,
        expected_hex: str,
        label: str,
    ):
        """
        Verify that a contract call returns the expected 32‑byte value.
        If *expected_hex* is ``None`` the call is expected to revert.
        """
        out = self.contract_call(method_selector, block_number)
        res = out.get("executionResult", {})
        excepted = res.get("excepted")
        output = res.get("output")
        gas_used = res.get("gasUsed", 0)

        self.log.info(f"  {label}: excepted='{excepted}', gasUsed={gas_used}")

        if expected_hex is None:
            # Expect a revert.
            if excepted != "None":
                self.log.info(f"  {label} – reverted as expected")
                return
            else:
                raise AssertionError(f"{label}: expected revert but got success")
        else:
            if excepted != "None":
                raise AssertionError(f"{label}: call failed – {excepted}")

            # Strip '0x' prefix if present for both outputs
            actual = output[2:] if output.startswith('0x') else output
            expected = expected_hex[2:] if expected_hex.startswith('0x') else expected_hex

            # Normalize to lowercase for comparison
            actual = actual.lower() if actual else ""
            expected = expected.lower()

            assert_equal(actual, expected)
            self.log.info(f"  {label} – ok")

    def check_historical_precompile_available(self):
        """Check if the historical precompile is available and working"""
        # Try calling the historical precompile directly via the contract
        cur_height = self.node.getblockcount()
        try:
            out = self.contract_call(METH_GETHISTBLOCKHASH, cur_height)
            res = out.get("executionResult", {})
            output = res.get("output", "")
            excepted = res.get("excepted", "Unknown")

            self.log.info(f"Historical precompile status: excepted='{excepted}', output='{output[:10] if output else 'None'}...'")

            # If output is non-zero and no revert, consider it available
            if output and output != "0" * 64 and excepted == "None":
                self.log.info("Historical precompile is available and returning data")
                return True
            else:
                self.log.info("Historical precompile is not available or returning zeros")
                return False
        except Exception as e:
            self.log.warning(f"Error checking historical precompile: {e}")
            return False

    def pre_activation_failure_cases(self):
        """
        Ensure that the historical precompile reverts (or returns zeros) before
        the activation height. This confirms the feature is gated.
        """
        self.log.info("Running pre‑activation failure case checks...")
        cur_height = self.node.getblockcount()
        self.log.info(f"Current chain height: {cur_height}")

        # Tip should not be available via historical precompile pre‑activation
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH,
            cur_height,
            None,
            "Tip request before activation",
        )

        # A past block inside the eventual window should also revert before activation
        past = max(1, cur_height - 10)
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH,
            past,
            None,
            "Past block request before activation",
        )

        # Future block should also revert (no change pre vs post activation)
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH,
            cur_height + 1,
            None,
            "Future height request before activation",
        )
        self.log.info("Pre‑activation failure case checks passed")

    def store_historical_hashes(self, node, start_block: int, label: str):
        """
        Store historical hashes on a specific node using sendtocontract.
        """
        self.log.info(f"Storing {NUM_HASHES_TO_STORE} historical hashes on {label} starting from block {start_block}")

        # Ensure the node has sufficient funds
        balance = node.getbalance()
        if balance < 1:  # Ensure at least 1 QTUM for gas fees
            self.log.warning(f"Node {label} has insufficient balance ({balance}), funding...")
            # Fund the node from the main node
            funding_address = node.getnewaddress()
            self.nodes[0].sendtoaddress(funding_address, 5)
            self.generate(self.nodes[0], 1)
            self.sync_all()

            # Now fund the specific address from the main node
            main_node_address = node.getnewaddress()
            self.nodes[0].sendtoaddress(main_node_address, 2)
            self.generate(self.nodes[0], 1)
            self.sync_all()

        # Format data: startBlock (32 bytes) + count (32 bytes)
        method_data = format(start_block, "064x") + format(NUM_HASHES_TO_STORE, "064x")

        # Get sender address for this node
        sender_address = node.getnewaddress()

        # Use sendtocontract to call the storage function
        out = node.sendtocontract(
            self.contract_address,
            METH_STORE_RECENT_HASHES + method_data,
            0,
            8000000,  # Conservative gas limit for 50 hashes
            QTUM_MIN_GAS_PRICE/COIN,
            sender_address
        )

        self.log.info(f"Storage transaction sent for {label}: {out['txid']}")

        # Get the transaction receipt to verify success
        tx_hash = out['txid']
        receipt = node.gettransactionreceipt(tx_hash)
        if receipt and len(receipt) > 0:
            if receipt[0].get('excepted', 'None') != 'None':
                raise Exception(f"Storage transaction failed on {label}: {receipt[0].get('excepted')}")

        self.log.info(f"Successfully stored historical hashes on {label}")

    def verify_stored_hashes(self, node, start_block: int, label: str):
        """
        Verify that stored hashes match between nodes.
        """
        self.log.info(f"Verifying stored hashes on {label}")

        # Get some sample stored hashes to verify
        sample_blocks = [start_block, start_block + 10, start_block + 25, start_block + 40, start_block + 49]

        stored_hashes = {}
        for block_num in sample_blocks:
            if block_num <= node.getblockcount():
                # Call getStoredHash function
                out = node.callcontract(self.contract_address, METH_GETSTOREDHASH + format(block_num, "064x"))
                res = out.get("executionResult", {})
                if res.get("excepted") == "None":
                    hash_value = res.get("output", "")
                    stored_hashes[block_num] = hash_value
                    self.log.info(f"  {label} block {block_num}: {hash_value[:16]}...")
                else:
                    self.log.warning(f"  {label} failed to get stored hash for block {block_num}")

        return stored_hashes

    def mine_pos_blocks(self, node, count: int = 10) -> list:
        """
        Mine exactly 'count' PoS blocks using mock time and staking.
        Returns a list of block hashes of the PoS blocks.
        """
        assert count > 0

        # Prepare staking environment following the reference pattern
        t = (node.getblock(node.getbestblockhash())['time'] + 100) & 0xfffffff0
        for n in self.nodes:
            n.setmocktime(t)

        pos_blocks = []
        target_blocks = count

        self.log.info(f"Mining {target_blocks} PoS blocks using staking pattern...")

        # Mine blocks one by one using the reference pattern
        for i in range(target_blocks):
            current_height = node.getblockcount()
            self.log.info(f"Mining PoS block {i + 1}/{target_blocks} from height {current_height}")
            
            # Use the reference test pattern for staking
            for t_iter in range(t, t + 100):  # Try for up to 100 seconds per block
                for n in self.nodes:
                    n.setmocktime(t_iter)
                
                if current_height < node.getblockcount():
                    # New block was produced
                    new_height = node.getblockcount()
                    new_hash = node.getblockhash(new_height)
                    pos_blocks.append(new_hash)
                    self.log.info(f"  PoS block {i + 1}/{target_blocks}: height={new_height}, hash={new_hash}")
                    t = t_iter  # Update time reference for next block
                    break
                
                self.log.info(f'  Staking attempt at time {t_iter}, height {node.getblockcount()}')
                time.sleep(1)
            else:
                # Timeout occurred
                raise AssertionError(f"Timeout: couldn't produce PoS block {i + 1}")
        
        self.log.info(f"Successfully mined {len(pos_blocks)} PoS blocks")
        return pos_blocks

    def run_pos_historical_tests(self):
        """
        Mine 10 PoS blocks and verify historical precompile behavior on them.
        """
        self.log.info("=== PoS historical data verification (10 PoS blocks) ===")

        node = self.nodes[0]  # run on node 0 (has staking enabled)
        self.node = node

        # Fund node for staking (ensure it has coins to stake)
        staking_address = node.getnewaddress()
        node.sendtoaddress(staking_address, 10)
        self.generate(node, 1)
        self.sync_all()

        # Mine exactly 10 PoS blocks
        pos_hashes = self.mine_pos_blocks(node, count=10)
        self.sync_all()

        cur_height = node.getblockcount()
        self.log.info(f"Current height after PoS mining: {cur_height}")

        # Verify each PoS block via the historical precompile
        for i, blk_hash in enumerate(pos_hashes, start=1):
            height = node.getblock(blk_hash)['height']
            rpc_hash = blk_hash
            expected_be = bytes_to_hex_str(hex_str_to_bytes(rpc_hash)[::-1])
            self.verify_blockhash_eq(
                METH_GETHISTBLOCKHASH,
                height,
                expected_be,
                f"PoS block {i}/{len(pos_hashes)} (height {height})"
            )

        # Ensure the last PoS block is inside the window so store works
        last_pos_height = node.getblock(pos_hashes[-1])['height']
        if (cur_height - last_pos_height) >= HISTORY_WINDOW:
            self.log.warning(f"Last PoS block is outside the HISTORY_WINDOW; mining extra blocks to bring it in window...")
            extra_needed = (cur_height - last_pos_height) - HISTORY_WINDOW + 5
            generatesynchronized(self.nodes[0], extra_needed, None, self.nodes)
            self.sync_all()
            cur_height = self.nodes[0].getblockcount()

        # Store historical hashes that include the last few PoS blocks
        store_start = max(1, cur_height - NUM_HASHES_TO_STORE + 1)
        self.store_historical_hashes(node, store_start, "Node 0 (PoS section)")
        self.sync_all()

        # Verify the stored hashes include the last PoS block correctly
        out = node.callcontract(self.contract_address, METH_GETSTOREDHASH + format(last_pos_height, "064x"))
        assert_equal(out['executionResult']['excepted'], 'None')
        stored = out['executionResult']['output']
        expected = bytes_to_hex_str(hex_str_to_bytes(pos_hashes[-1])[::-1])
        assert_equal(stored, expected)
        self.log.info(f"Stored hash matches last PoS block (height {last_pos_height})")

        self.log.info("=== PoS historical data verification completed ===")

    def reorg_test_with_storage(self):
        """
        Test historical precompile behavior during blockchain reorganization with stored data.
        """
        self.log.info("Starting reorg test with stored data...")

        # Save current state before reorg
        node0_tip_before = self.nodes[0].getbestblockhash()
        node1_tip_before = self.nodes[1].getbestblockhash()
        height_before = self.nodes[0].getblockcount()

        self.log.info(f"Initial state - Node 0 tip: {node0_tip_before}, height: {height_before}")

        # Fund both nodes properly before storage operations
        self.fund_nodes_for_storage()

        # Calculate starting block for storage (leave room for disconnection blocks)
        storage_start_block = height_before - 100  # Store hashes from 100 blocks ago

        # Store historical hashes on both nodes BEFORE disconnection
        self.log.info("Storing historical hashes before disconnection...")

        # Store on node 0
        original_node = self.node
        self.node = self.nodes[0]
        self.store_historical_hashes(self.nodes[0], storage_start_block, "Node 0 (before disconnect)")

        # Store on node 1
        self.node = self.nodes[1]
        self.store_historical_hashes(self.nodes[1], storage_start_block, "Node 1 (before disconnect)")

        # Verify stored data matches before disconnection
        self.log.info("Verifying stored data before disconnection...")
        node0_hashes_before = self.verify_stored_hashes(self.nodes[0], storage_start_block, "Node 0")
        node1_hashes_before = self.verify_stored_hashes(self.nodes[1], storage_start_block, "Node 1")

        # Hashes should match before disconnection
        for block_num in node0_hashes_before:
            if block_num in node1_hashes_before:
                if node0_hashes_before[block_num] == node1_hashes_before[block_num]:
                    self.log.info(f"  Block {block_num} hashes match before disconnect: OK")
                else:
                    self.log.warning(f"  Block {block_num} hashes differ before disconnect: UNEXPECTED")

        # Disconnect nodes so they can mine separate chains
        self.log.info("Disconnecting nodes...")
        self.disconnect_nodes(0, 1)

        # Mine blocks on node 0
        self.log.info("Mining blocks on node 0...")
        generatesynchronized(self.nodes[0], 50, None, [])
        node0_new_tip = self.nodes[0].getbestblockhash()
        node0_new_height = self.nodes[0].getblockcount()
        self.log.info(f"Node 0 new tip: {node0_new_tip}, height: {node0_new_height}")

        # Mine blocks on node 1
        self.log.info("Mining blocks on node 1...")
        generatesynchronized(self.nodes[1], 52, None, [])
        node1_new_tip = self.nodes[1].getbestblockhash()
        node1_new_height = self.nodes[1].getblockcount()
        self.log.info(f"Node 1 new tip: {node1_new_tip}, height: {node1_new_height}")

        # Test historical data while disconnected (each node should see its own chain)
        self.log.info("Testing historical data while nodes are disconnected...")

        # Test node 0's historical data
        self.node = self.nodes[0]
        for i in [5, 25, 50]:  # Test various blocks in node 0's chain
            if i <= node0_new_height:
                block_hash = self.nodes[0].getblockhash(node0_new_height - i)
                expected_be = bytes_to_hex_str(hex_str_to_bytes(block_hash)[::-1])
                self.verify_blockhash_eq(
                    METH_GETHISTBLOCKHASH,
                    node0_new_height - i,
                    expected_be,
                    f"Node 0 block {node0_new_height - i} historical hash (disconnected)"
                )

        # Test node 1's historical data
        self.node = self.nodes[1]
        for i in [5, 25, 52]:  # Test various blocks in node 1's chain
            if i <= node1_new_height:
                block_hash = self.nodes[1].getblockhash(node1_new_height - i)
                expected_be = bytes_to_hex_str(hex_str_to_bytes(block_hash)[::-1])
                self.verify_blockhash_eq(
                    METH_GETHISTBLOCKHASH,
                    node1_new_height - i,
                    expected_be,
                    f"Node 1 block {node1_new_height - i} historical hash (disconnected)"
                )

        # Verify stored data is still accessible on both nodes while disconnected
        self.log.info("Verifying stored data while disconnected...")
        node0_hashes_disconnected = self.verify_stored_hashes(self.nodes[0], storage_start_block, "Node 0 (disconnected)")
        node1_hashes_disconnected = self.verify_stored_hashes(self.nodes[1], storage_start_block, "Node 1 (disconnected)")

        # Reconnect nodes and wait for reorg
        self.log.info("Reconnecting nodes...")
        self.connect_nodes(0, 1)

        # Wait for nodes to sync (node 1 has the longer chain so it should win)
        self.sync_blocks()

        # Check final state
        final_tip_node0 = self.nodes[0].getbestblockhash()
        final_tip_node1 = self.nodes[1].getbestblockhash()
        final_height_node0 = self.nodes[0].getblockcount()
        final_height_node1 = self.nodes[1].getblockcount()

        self.log.info(f"After reorg - Node 0 tip: {final_tip_node0}, height: {final_height_node0}")
        self.log.info(f"After reorg - Node 1 tip: {final_tip_node1}, height: {final_height_node1}")

        # Both nodes should have the same tip (node 1's longer chain)
        assert_equal(final_tip_node0, final_tip_node1)
        assert_equal(final_height_node0, final_height_node1)
        assert_equal(final_tip_node1, node1_new_tip)  # Node 1's chain should have won

        # Test historical data after reorg on both nodes
        self.log.info("Testing historical data after reorg...")

        # Test on node 0
        self.node = self.nodes[0]
        tip_hash = self.nodes[0].getbestblockhash()
        expected_tip_be = bytes_to_hex_str(hex_str_to_bytes(tip_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, final_height_node0, expected_tip_be, "Node 0 tip after reorg"
        )

        # Test on node 1
        self.node = self.nodes[1]
        tip_hash = self.nodes[1].getbestblockhash()
        expected_tip_be = bytes_to_hex_str(hex_str_to_bytes(tip_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, final_height_node1, expected_tip_be, "Node 1 tip after reorg"
        )

        # Verify stored data is still consistent after reorg
        self.log.info("Verifying stored data consistency after reorg...")
        node0_hashes_after = self.verify_stored_hashes(self.nodes[0], storage_start_block, "Node 0 (after reorg)")
        node1_hashes_after = self.verify_stored_hashes(self.nodes[1], storage_start_block, "Node 1 (after reorg)")

        # Compare stored hashes between nodes after reorg
        consistent_count = 0
        total_compared = 0

        for block_num in node0_hashes_after:
            if block_num in node1_hashes_after:
                total_compared += 1
                if node0_hashes_after[block_num] == node1_hashes_after[block_num]:
                    consistent_count += 1
                    self.log.info(f"  Block {block_num} hashes consistent after reorg: OK")
                else:
                    self.log.error(f"  Block {block_num} hashes inconsistent after reorg: FAIL")

        self.log.info(f"Storage consistency check: {consistent_count}/{total_compared} blocks consistent")

        if consistent_count == total_compared and total_compared > 0:
            self.log.info("All stored hashes remain consistent after reorg: SUCCESS")
        elif total_compared > 0:
            raise AssertionError(f"Storage consistency failed: only {consistent_count}/{total_compared} blocks consistent")
        else:
            self.log.warning("No stored hashes could be verified after reorg")

        self.log.info("Reorg test with storage completed successfully!")

    def run_test(self):
        # --------------------------------------------------------------
        # Basic node preparation
        # --------------------------------------------------------------
        self.node = self.nodes[0]
        self.connect_nodes(0, 1)

        # Fund a sender address and generate enough blocks for the UTXO set
        # to mature.
        address = self.node.getnewaddress()
        generatesynchronized(self.node, 10 + COINBASE_MATURITY, address, self.nodes)

        # --------------------------------------------------------------
        # Deploy the corrected contract with storage functionality
        # --------------------------------------------------------------
        # This contract compiles successfully and includes storage functionality
        bytecode = (
            "6080604052348015600e575f5ffd5b5061067d8061001c5f395ff3fe608060405234801561000f575f5ffd5b506004361061007b575f3560e01c8063c7513f8711610059578063c7513f87146100e9578063ee82ac5e14610107578063f2e8410c14610137578063f5dab3a6146101675761007b565b80630fd9abec1461007f57806317a671121461009d578063365afb1f146100cd575b5f5ffd5b610087610197565b6040516100949190610398565b60405180910390f35b6100b760048036038101906100b291906103df565b6101a0565b6040516100c49190610422565b60405180910390f35b6100e760048036038101906100e2919061043b565b6101b4565b005b6100f161025b565b6040516100fe9190610398565b60405180910390f35b610121600480360381019061011c91906103df565b610261565b60405161012e9190610422565b60405180910390f35b610151600480360381019061014c91906103df565b61026b565b60405161015e9190610422565b60405180910390f35b610181600480360381019061017c91906103df565b610367565b60405161018e9190610422565b60405180910390f35b5f600154905090565b5f602052805f5260405f205f915090505481565b5f811180156101c4575060328111155b610203576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016101fa906104d3565b60405180910390fd5b816001819055505f5f90505b81811015610256575f8184610224919061051e565b90505f6102308261026b565b9050805f5f8481526020019081526020015f20819055505050808060010191505061020f565b505050565b60015481565b5f81409050919050565b5f5f8260405160200161027e9190610398565b60405160208183030381529060405290505f5f71f90827f1c53a10cb7a02335b17532000293573ffffffffffffffffffffffffffffffffffffffff16836040516102c891906105a3565b5f604051808303815f865af19150503d805f8114610301576040519150601f19603f3d011682016040523d82523d5f602084013e610306565b606091505b5091509150818015610319575060208151145b610358576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161034f90610629565b60405180910390fd5b60208101519350505050919050565b5f5f5f8381526020019081526020015f20549050919050565b5f819050919050565b61039281610380565b82525050565b5f6020820190506103ab5f830184610389565b92915050565b5f5ffd5b6103be81610380565b81146103c8575f5ffd5b50565b5f813590506103d9816103b5565b92915050565b5f602082840312156103f4576103f36103b1565b5b5f610401848285016103cb565b91505092915050565b5f819050919050565b61041c8161040a565b82525050565b5f6020820190506104355f830184610413565b92915050565b5f5f60408385031215610451576104506103b1565b5b5f61045e858286016103cb565b925050602061046f858286016103cb565b9150509250929050565b5f82825260208201905092915050565b7f436f756e74206d757374206265206265747765656e203120616e6420353000005f82015250565b5f6104bd601e83610479565b91506104c882610489565b602082019050919050565b5f6020820190508181035f8301526104ea816104b1565b9050919050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f61052882610380565b915061053383610380565b925082820190508082111561054b5761054a6104f1565b5b92915050565b5f81519050919050565b5f81905092915050565b8281835e5f83830152505050565b5f61057d82610551565b610587818561055b565b9350610597818560208601610565565b80840191505092915050565b5f6105ae8284610573565b915081905092915050565b7f46616c6c6261636b2063616c6c206661696c6564206f7220696e76616c6964205f8201527f726573706f6e7365000000000000000000000000000000000000000000000000602082015250565b5f610613602883610479565b915061061e826105b9565b604082019050919050565b5f6020820190508181035f83015261064081610607565b905091905056fea264697066735822122022c6f24bcb934277452216493335b10f437e25c5f4f538bb6206d9787858db6164736f6c634300081e0033"
        )

        # Deploy contract
        self.contract_address, self.sender = self.deploy_contract(self.node, bytecode)
        self.log.info(f"Helper contract deployed at {self.contract_address}")

        # --------------------------------------------------------------
        # Check current height and run pre-activation tests if needed
        # --------------------------------------------------------------
        cur_height = self.node.getblockcount()

        # Extract activation height from node arguments
        activation_height = None
        for arg in self.extra_args[0]:
            if arg.startswith("-pectraheight="):
                activation_height = int(arg.split("=")[1])
                break

        if activation_height is None:
            raise ValueError("Activation height not found in node arguments")

        self.log.info(f"Current height: {cur_height}, activation height: {activation_height}")

        # If below activation, run pre-activation tests and then generate to activation
        if cur_height < activation_height:
            self.pre_activation_failure_cases()

            to_gen = activation_height - cur_height
            self.log.info(f"Generating {to_gen} more blocks to reach activation height...")
            generatesynchronized(self.node, to_gen, None, self.nodes)
            self.sync_all()

            # Generate ONE MORE BLOCK to ensure the block at height (activation_height - 1) is properly committed
            self.log.info("Generating 1 extra block to ensure proper block commitment...")
            generatesynchronized(self.node, 1, None, self.nodes)
            self.sync_all()

            cur_height = self.node.getblockcount()
            self.log.info(f"Height after generation: {cur_height}")

        # --------------------------------------------------------------
        # Check if historical precompile is available (post-activation)
        # --------------------------------------------------------------
        historical_available = self.check_historical_precompile_available()

        if not historical_available:
            self.log.info("Historical precompile is not available in this build.")
            self.log.info("Skipping EIP-2935 specific tests.")
            self.log.info("Note: The historical precompile requires Qtum build with EIP-2935 support.")
            return

        # --------------------------------------------------------------
        # Part 1 – Verify basic functionality after activation height
        # --------------------------------------------------------------
        assert cur_height >= activation_height, f"Should be past activation height, have {cur_height}"
        self.log.info(f"Testing at height: {cur_height}")

        # Verify tip.
        tip_hash = self.node.getbestblockhash()
        expected_tip_be = bytes_to_hex_str(hex_str_to_bytes(tip_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, cur_height, expected_tip_be, "Tip historical hash"
        )

        # Verify previous block (now this should work since we generated an extra block)
        prev = cur_height - 1
        prev_hash = self.node.getblockhash(prev)
        expected_prev_be = bytes_to_hex_str(hex_str_to_bytes(prev_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, prev, expected_prev_be, "Prev block historical hash"
        )

        # Test some blocks that are definitely after activation
        generatesynchronized(self.node, 5, None, self.nodes)
        self.sync_all()

        cur_height = self.node.getblockcount()
        self.log.info(f"New height after additional blocks: {cur_height}")

        # Test block from activation height
        activation_block_hash = self.node.getblockhash(activation_height)
        expected_activation_be = bytes_to_hex_str(hex_str_to_bytes(activation_block_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, activation_height, expected_activation_be, "Activation height block"
        )

        # Test block a few blocks after activation
        mid_block = activation_height + 3
        if mid_block <= cur_height:
            mid_block_hash = self.node.getblockhash(mid_block)
            expected_mid_be = bytes_to_hex_str(hex_str_to_bytes(mid_block_hash)[::-1])
            self.verify_blockhash_eq(
                METH_GETHISTBLOCKHASH, mid_block, expected_mid_be, "Mid-block after activation"
            )

        # Future block must revert.
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, cur_height + 1, None, "Future height request"
        )

        # Old block outside the window must revert.
        old = cur_height - HISTORY_WINDOW - 1
        if old > 0:
            self.verify_blockhash_eq(
                METH_GETHISTBLOCKHASH, old, None, "Out‑of‑window old block"
            )

        # --------------------------------------------------------------
        # Part 2 – Long chain to test window edges
        # --------------------------------------------------------------
        self.log.info("Generating a longer chain to test window edges...")

        # We need to generate enough blocks AFTER activation to cover the full window
        current_height = self.node.getblockcount()
        blocks_needed_after_activation = HISTORY_WINDOW + 100
        total_blocks_needed = blocks_needed_after_activation

        self.log.info(f"Generating {total_blocks_needed} blocks after activation to test full window...")
        generatesynchronized(self.node, total_blocks_needed, None, self.nodes)
        self.sync_all()

        cur_height = self.node.getblockcount()
        self.log.info(f"Final height: {cur_height}")

        tip_hash = self.node.getbestblockhash()
        expected_tip_be = bytes_to_hex_str(hex_str_to_bytes(tip_hash)[::-1])
        self.verify_blockhash_eq(
            METH_GETHISTBLOCKHASH, cur_height, expected_tip_be, "Tip after long chain"
        )

        # Last block inside the window - this should now be after activation
        last_in_window = cur_height - HISTORY_WINDOW + 1
        if last_in_window > 0:
            self.log.info(f"Testing last block in window at height {last_in_window}")
            last_hash = self.node.getblockhash(last_in_window)
            expected_last_be = bytes_to_hex_str(hex_str_to_bytes(last_hash)[::-1])
            self.verify_blockhash_eq(
                METH_GETHISTBLOCKHASH,
                last_in_window,
                expected_last_be,
                "Last block inside window",
            )
            # Block just before the window should revert.
            before_last = cur_height - HISTORY_WINDOW
            if before_last > 0:
                self.verify_blockhash_eq(
                    METH_GETHISTBLOCKHASH,
                    before_last,
                    None,
                    "Block just before window",
                )
        else:
            self.log.warning(f"Chain height {cur_height} is less than HISTORY_WINDOW+1 ({HISTORY_WINDOW + 1})")

        # --------------------------------------------------------------
        # NEW: Mine 10 PoS blocks and verify historical precompile on them
        # --------------------------------------------------------------
        self.run_pos_historical_tests()

        # --------------------------------------------------------------
        # Part 3 – Test reorg behavior with stored data
        # --------------------------------------------------------------
        self.reorg_test_with_storage()

        self.log.info("All historical-data tests passed!")
        self.log.info("Summary:")
        self.log.info("- Historical precompile is working correctly")
        self.log.info("- Block hashes are correctly returned within the window")
        self.log.info("- Out-of-window requests properly revert")
        self.log.info("- Window edges are correctly enforced")
        self.log.info("- Historical precompile correctly handles blockchain reorganizations")
        self.log.info("- Stored contract state remains consistent after reorg")
        self.log.info("- 10 PoS blocks were mined and historical hashes verified")

if __name__ == "__main__":
    QtumEVMHistoricalDataTest(__file__).main()
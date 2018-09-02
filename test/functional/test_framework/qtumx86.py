from .address import *
from .script import *
from .mininode import *
from .util import *
from .qtumconfig import *
from .blocktools import *
from .key import *
import io
import subprocess
import tempfile
import random

QtumSystem = 0x40
QtumTrustedLibrary = 0x41
InteralUI = 0x50

QSCBlockGasLimit = 1
QSCBlockCreator = 2
QSCBlockDifficulty = 3
QSCBlockHeight = 4
QSCGetBlockHash = 5
QSCIsCreate = 6
QSCSelfAddress = 7
QSCPreviousBlockTime = 8
QSCUsedGas = 9
QSCSetFaultHandler = 10
QSCSetDoubleFaultHandler = 11
QSCCodeSize = 12
QSCDataSize = 13
QSCScratchSize = 14
QSCSelfDestruct = 15


# storage commands, 0x1000
QSCReadStorage = 0x1000
QSCWriteStorage = 0x1001

# value commands, 0x2000
QSCSendValue = 0x2000 # send coins somewhere
QSCGetBalance = 0x2001
QSCSendValueAndCall = 0x2002

# caller commands, 0x3000
QSCGasProvided = 0x3000
QSCCallerTransaction = 0x3001 # provides output scripts, etc
QSCValueProvided = 0x3002
QSCOriginAddress = 0x3003
QSCSenderAddress = 0x3004
QSCCallStackSize = 0x3005

# call commands, 0x4000
QSCCallContract = 0x4000
QSCCallLibrary = 0x4001



QX86_CODE_ADDRESS = 0x1000
QX86_MAX_CODE_SIZE = 0x10000
QX86_DATA_ADDRESS = 0x100000
QX86_MAX_DATA_SIZE = 0x10000
QX86_STACK_ADDRESS = 0x200000
QX86_MAX_STACK_SIZE = 1024 * 8

QX86_TX_DATA_ADDRESS = 0xD0000000
QX86_TX_DATA_ADDRESS_END = 0xF0000000

QX86_TX_CALL_DATA_ADDRESS = 0x210000

# Encodes the x86 binary data to the format expected by sendtocontract and createcontract i.e.
# [uint32 : optionsLength] | [uint32 : codeLength] | [uint32 : dataLength] | [uint32 : reserved] | [bytes : options] | [bytes : code] | [bytes : data]
def encode_x86_bytecode(bytecode):
    header_and_bytecode = "0"*8
    header_and_bytecode += hex(len(bytecode))[2:][::-1].zfill(8)[::-1]
    header_and_bytecode += "0"*8
    header_and_bytecode += "0"*8
    header_and_bytecode += bytes_to_hex_str(bytecode)
    return header_and_bytecode

# Note, this function will be used until a more stable version of x86 is implemented so that
# it easier to edit x86 test cases
# yasm must be in the PATH
def yasm_compile(code, prefix_header=True):
    if prefix_header:
        code = """
            CPU i386
            BITS 32
            SECTION .text progbits alloc exec nowrite align=1
            ORG 0x1000
            GLOBAL start
        """ + code

    tmp_out_file_path = tempfile.gettempdir() + '/qtumx86_out_' + hex(random.randint(0, 2**256)) + ""
    tmp_in_file_path = tempfile.gettempdir() + '/qtumx86_in_' + hex(random.randint(0, 2**256)) + ".S"
    with open(tmp_in_file_path, 'w') as f:
        f.write(code)

    try:
        print(subprocess.check_output(["yasm", tmp_in_file_path, "-o", tmp_out_file_path], stderr=subprocess.PIPE))
        with open(tmp_out_file_path, 'rb') as f:
            b = f.read()
            print('bytecode length', len(b))
            return encode_x86_bytecode(b)
    except:
        print(code)
        print("yasm compilation failed", file=sys.stderr)

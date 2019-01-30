#ifndef SHARED_X86_H
#define SHARED_X86_H

#include <stdint.h>
#include <stddef.h>

//structs
#define ADDRESS_DATA_SIZE 64

typedef struct{
    //Do not modify this struct's fields
    //This is consensus critical!
    uint32_t version;
    uint8_t data[ADDRESS_DATA_SIZE];
} __attribute__((__packed__)) UniversalAddressABI;

//constants below this line should exactly match libqtum's qtum.h! 

//Note: we don't use Params().Base58Prefixes for these version numbers because otherwise
//contracts would need to use two different SDKs since the the base58 version for pubkeyhash etc changes
//between regtest, testnet, and mainnet
enum AddressVersion{
    UNKNOWN = 0,
    //legacy is either pubkeyhash or EVM, depending on if the address already exists
    LEGACYEVM = 1,
    PUBKEYHASH = 2,
    EVM = 3,
    X86 = 4,
    SCRIPTHASH = 5,
    P2WSH = 6,
    P2WPKH = 7,
};


static const int QTUM_SYSTEM_ERROR_INT = 0xFF;

typedef struct qtum_hash32 { uint8_t data[32]; } __attribute__((__packed__)) qtum_hash32;

//interrupt 0x40
//QSC = Qtum System Call
//use defines here so that we can use it in C easily

//basic execution calls, 0x0000
#define QSC_UsedGas                 9
#define QSC_SetFaultHandler         10
#define QSC_SetDoubleFaultHandler   11
#define QSC_CodeSize                12
#define QSC_DataSize                13
#define QSC_ScratchSize             14
#define QSC_SelfDestruct            15

#define QSC_AddEvent                16
/* -- this quickly gets very complicated. Defer/cancel implementation
#define QSC_GetEvent                17
#define QSC_GetEventSize            18
#define QSC_ExecutingCallID         19
#define QSC_NextCallID              20
*/

#define QSC_ParseAddress          21  

    //storage commands, 0x1000
#define QSC_ReadStorage             0x1000
#define QSC_WriteStorage            0x1001
#define QSC_ReadExternalStorage     0x1002
#define QSC_UpdateBytecode          0x1003

    //value commands, 0x2000
#define QSC_SendValue               0x2000 //send coins somewhere
#define QSC_GetBalance              0x2001
#define QSC_SendValueAndCall        0x2002

    //callee commands, 0x3000
#define QSC_CallStackSize           0x3005
//SCCS = Smart Contract Communication Stack
//note: Upon contract error, this stack is not cleared. Thus an error contract can have side effects
#define QSC_SCCSItemCount               0x3006
//#define QSC_SCCSMaxItems            0x3007
//#define QSC_SCCSMaxSize             0x3008
#define QSC_SCCSSize                0x3009
#define QSC_SCCSItemSize            0x300A
#define QSC_SCCSPop                 0x300B
#define QSC_SCCSPeek                0x300C
#define QSC_SCCSPush                0x300D
#define QSC_SCCSDiscard             0x300E //pops off the stack without any data transfer possible (for cheaper gas cost)
#define QSC_SCCSClear               0x300F
#define QSC_SelfCalled              0x3010

    //caller commands, 0x4000
#define QSC_CallContract            0x4000
#define QSC_CallLibrary             0x4001


//error code types
//These will cause appropriate revert of state etc
//note, this is the last value pushed onto SCCS upon contract termination
#define QTUM_EXIT_SUCCESS 0 //successful execution
#define QTUM_EXIT_USER 1 //user defined flag (optional, no consensus function, treated the same as success)
#define QTUM_EXIT_REVERT 2 //execution that reverted state
#define QTUM_EXIT_ERROR 4 //error execution (which may or may not revert state)
#define QTUM_EXIT_OUT_OF_GAS 8 //execution which ended in out of gas exception
#define QTUM_EXIT_CRASH 16 //execution which ended due to CPU or memory errors
#define QTUM_EXIT_SYSCALL_EXCEPTION 32 //execution which ended due to an exception by a syscall, such as transfering more money than the contract owns

//NOTE: only QTUM_EXIT_SUCCESS, QTUM_EXIT_ERROR, QTUM_EXIT_REVERT, and QTUM_USER may be specified by __qtum_terminate


//ABI type prefixes
//note the limit for this is 15, since it should fit into a nibble
#define ABI_TYPE_UNKNOWN 0
#define ABI_TYPE_INT 1
#define ABI_TYPE_UINT 2
#define ABI_TYPE_HEX 3
#define ABI_TYPE_STRING 4
#define ABI_TYPE_BOOL 5
#define ABI_TYPE_ADDRESS 6

enum QtumEndpoint{
    QtumExit = 0xF0,
    QtumSystem = 0x40,
    QtumTrustedLibrary = 0x41,
    InteralUI = 0x50
};


//Syscall parameter/return value ABIs
typedef struct{
    uint64_t usedGas;
    uint64_t refundedValue;
    uint32_t errorCode;
    
}  __attribute__((__packed__)) QtumCallResultABI;

#ifndef QTUM_MOCK
//Don't expose these in the mocking because it's not really possible to keep it equivalent
//Really these shouldn't be used in actual contract code anyway

//memory areas
#define CODE_ADDRESS 0x1000
#define MAX_CODE_SIZE 0x10000
#define DATA_ADDRESS 0x100000
#define MAX_DATA_SIZE 0x10000
#define STACK_ADDRESS 0x200000
#define MAX_STACK_SIZE 1024 * 8

#define EXEC_DATA_ADDRESS 0xD0000000
#define EXEC_DATA_SIZE 0x100000
#define TX_DATA_ADDRESS 0xD1000000
#define TX_DATA_SIZE 0x100000
#define BLOCK_DATA_ADDRESS 0xD2000000
#define BLOCK_DATA_SIZE 0x100000

#endif

//Read only fixed-size per-execution data
struct ExecDataABI{
    //total size of txdata area
    uint32_t size;

    uint32_t isCreate;
    UniversalAddressABI sender;
    uint64_t gasLimit;
    uint64_t valueSent;
    UniversalAddressABI origin;
    UniversalAddressABI self;
    uint32_t nestLevel; //how many calls exist above this one, 0 = top level
} __attribute__((__packed__));

//Read only fixed-size per-block data
struct BlockDataABI{
    //total size of blockdata area
    uint32_t size;

    UniversalAddressABI blockCreator;
    uint64_t blockGasLimit;
    uint64_t blockDifficulty;
    uint32_t blockHeight;
    uint64_t previousTime;
    qtum_hash32 blockHashes[256];
} __attribute__((__packed__));


struct QtumTxInput{
    UniversalAddressABI sender;
    uint64_t value;
}  __attribute__((__packed__));

struct QtumTxOutput{
    UniversalAddressABI reciever;
    uint64_t value;
}  __attribute__((__packed__));

//Read only dynamic size per-tx data
struct TxDataABI{
    //total size of txdata area
    uint32_t size; 
    
    const struct QtumTxInput *inputsBegin; //points to VM memory space, NOT physical memory space
    const struct QtumTxInput *inputsEnd; //points to VM memory space, NOT physical memory space

    const struct QtumTxOutput *outputsBegin; //points to VM memory space, NOT physical memory space
    const struct QtumTxOutput *outputsEnd; //points to VM memory space, NOT physical memory space
    
    uint32_t rawTxDataSize;
    uint8_t beginRawTxData; //at this point is where raw transaction bytes begin
    //The parsed inputs and outputs follow, but are at a dynamic location pointed to by the inputs and outputs fields below
}  __attribute__((__packed__));

#endif
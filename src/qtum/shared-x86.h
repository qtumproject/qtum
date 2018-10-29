#ifndef SHARED_X86_H
#define SHARED_X86_H

//constants below this line should exactly match libqtum's qtum.h! 

static const int QTUM_SYSTEM_ERROR_INT = 0xFF;

typedef struct qtum_hash32 { uint8_t data[32]; } __attribute__((__packed__)) qtum_hash32;

//interrupt 0x40
//QSC = Qtum System Call
//use defines here so that we can use it in C easily

//basic execution calls, 0x0000
#define QSC_BlockGasLimit           1
#define QSC_BlockCreator            2
#define QSC_BlockDifficulty         3
#define QSC_BlockHeight             4
#define QSC_GetBlockHash            5
#define QSC_IsCreate                6
#define QSC_SelfAddress             7
#define QSC_PreviousBlockTime       8
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


    //storage commands, 0x1000
#define QSC_ReadStorage             0x1000
#define QSC_WriteStorage            0x1001

    //value commands, 0x2000
#define QSC_SendValue               0x2000 //send coins somewhere
#define QSC_GetBalance              0x2001
#define QSC_SendValueAndCall        0x2002

    //callee commands, 0x3000
#define QSC_GasProvided             0x3000
#define QSC_CallerTransaction       0x3001 //provides output scripts, etc
#define QSC_ValueProvided           0x3002
#define QSC_OriginAddress           0x3003
#define QSC_SenderAddress           0x3004
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

    //caller commands, 0x4000
#define QSC_CallContract            0x4000
#define QSC_CallLibrary             0x4001

//error code types
//These will cause appropriate revert of state etc
//note, this is the last value pushed onto SCCS upon contract termination
#define QTUM_EXIT_SUCCESS 0 //successful execution
#define QTUM_EXIT_HAS_DATA 1 //there is user defined data pushed onto the stack (optional, no consensus function)
#define QTUM_EXIT_REVERT 2 //execution that reverted state
#define QTUM_EXIT_ERROR 4 //error execution (which may or may not revert state)
#define QTUM_EXIT_OUT_OF_GAS 8 //execution which ended in out of gas exception
#define QTUM_EXIT_CRASH 16 //execution which ended due to CPU or memory errors
#define QTUM_EXIT_SYSCALL_EXCEPTION 32 //execution which ended due to an exception by a syscall, such as transfering more money than the contract owns

//NOTE: only QTUM_EXIT_SUCCESS, QTUM_EXIT_ERROR, QTUM_EXIT_REVERT, and QTUM_HAS_DATA may be specified by __qtum_terminate


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
     
}  __attribute__((aligned(4)));

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
}  __attribute__((aligned(4)));


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
    
    const QtumTxInput const *inputsBegin; //points to VM memory space, NOT physical memory space
    const QtumTxInput const *inputsEnd; //points to VM memory space, NOT physical memory space

    const QtumTxOutput const *outputsBegin; //points to VM memory space, NOT physical memory space
    const QtumTxOutput const *outputsEnd; //points to VM memory space, NOT physical memory space
    
    uint32_t rawTxDataSize;
    uint8_t beginRawTxData; //at this point is where raw transaction bytes begin
    //The parsed inputs and outputs follow, but are at a dynamic location pointed to by the inputs and outputs fields below
}  __attribute__((__packed__));

#endif
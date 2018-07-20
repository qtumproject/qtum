
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned char uint8_t;
typedef unsigned int size_t;
//#include <stdint.h>

// copy-paste some library functions until we get a basic libc working
void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dstptr;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
    const unsigned char* a = (const unsigned char*) aptr;
    const unsigned char* b = (const unsigned char*) bptr;
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i])
            return -1;
        else if (b[i] < a[i])
            return 1;
    }
    return 0;
}

static void outd(uint16_t port, uint32_t val)
{
    asm volatile ( "out %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

static uint32_t ind(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "in %1, %0"
    : "=a"(ret)
    : "Nd"(port) );
    return ret;
}

struct ABI_MakeLog{
    uint16_t dataSize;
    uint32_t dataAddress;
    uint16_t topicCount;
    uint32_t topicsAddress;
}__attribute__((__packed__));

struct ABI_Exit{
    uint32_t resultAddress;
    uint16_t resultSize;
}__attribute__((__packed__));

struct ABI_TransferValueSilent{
    uint64_t value;
    uint8_t address[32]; //normal addresses are only 20 bytes, but we use 32 bytes for future use
}__attribute__((__packed__));

struct ABI_TransferValue{
    uint64_t value;
    uint8_t address[32];
    uint16_t dataSize;
    uint32_t dataAddress;
}__attribute__((__packed__));

struct ABI_GetGasInfo{
    uint32_t gasLimit;
    uint32_t blockGasLimit;
    uint64_t gasPrice;
}__attribute__((__packed__));

struct ABI_AllocateMemory{
    uint32_t desiredAddress; //can not be less than 0x100000
    uint32_t size;
}__attribute__((__packed__));

struct ABI_PersistMemory{
    uint32_t address; //this will become storageAddress for restoring
    uint32_t size;
    uint32_t storageAddress;
}__attribute__((__packed__));

struct ABI_RestoreMemory{
    uint32_t desiredAddress;
    uint32_t size; //should be a multiple of 32
    uint32_t storageAddress;
}__attribute__((__packed__));


struct ABI_GetMsgInfo{
    uint8_t sender[32];
}__attribute__((__packed__));

struct ABI_Selfdestruct{
    uint8_t transferTo[32];
}__attribute__((__packed__));

//out ports
#define ABI_EXIT 255
#define ABI_PERSIST_INITIAL_MEMORY 254
#define ABI_SELFDESTRUCT 253
#define ABI_GETMSGINFO 7
#define ABI_RESTORE_MEMORY 6
#define ABI_PERSIST_MEMORY 5
#define ABI_ALLOCATE_MEMORY 4
#define ABI_MAKE_LOG 1

//in ports
#define ABI_IN_ISCREATE 0xF0


//size of initial executable area
#define INITIAL_AREA 0x100000


//first is a 16bit size, then data follows
volatile uint8_t* abi_area = (void*)0xF0000;

uint8_t owner[32];

void exitResult(volatile void* result, size_t size){
    volatile struct ABI_Exit abi;
    abi.resultAddress = (uint32_t)result;
    abi.resultSize = size;
    outd(ABI_EXIT, (uint32_t) &abi);
    while(1);
    //never returns
}
void exit(){
    outd(ABI_EXIT, 0);
    while(1);
    //never returns
}

extern char __binary_end;
void persistInitialArea(){
    //this works by magical linking script, __binary_end will be the end of .text, .data, and .bss sections
    outd(ABI_PERSIST_INITIAL_MEMORY, (uint32_t)&__binary_end);
}

int isCreation(){
    return ind(ABI_IN_ISCREATE);
}

void getSender(uint8_t* output){
   // struct ABI_GetMsgInfo abi; //right now there is only sender, so send it straight to output
    outd(ABI_GETMSGINFO, (uint32_t)&output);
}

void allocateMemory(volatile void* where, int size){
    volatile struct ABI_AllocateMemory abi;
    abi.desiredAddress=(uint32_t) where;
    abi.size=size;
    outd(ABI_ALLOCATE_MEMORY, (uint32_t) &abi);
}
void persistRestoreMemory(volatile void* where, int size, volatile void* storageAddress, int restore){
    volatile struct ABI_PersistMemory abi;
    abi.address=(uint32_t) where;
    abi.storageAddress=(uint32_t)storageAddress;
    size+= 32 - (size % 32); //round up to next 32nd byte
    abi.size=size;
    uint16_t port = restore ? ABI_RESTORE_MEMORY : ABI_PERSIST_MEMORY;
    outd(port, (uint32_t)&abi);
}

//we store greeting as first uint16_t is size, follows is string data
volatile uint16_t* greetingSize = (void*) (INITIAL_AREA + 0x1000);
volatile char* greeting = (void*) (INITIAL_AREA + 0x1002);



void kill(){
    uint8_t buffer[32];
    getSender(buffer);
    if(!memcmp(buffer, owner, 32)){
        //keys are equal
        struct ABI_Selfdestruct abi;
        memcpy(abi.transferTo, owner, 32);
        outd(ABI_SELFDESTRUCT, (uint32_t) &abi);
    }
}
void greet(){
    //we must explicitly restore memory that we need
    allocateMemory(greetingSize, 0x10000);
    persistRestoreMemory(greetingSize, 32, greetingSize, 1);
    if(*greetingSize > 30){
        persistRestoreMemory(greetingSize+32, *greetingSize-30, greetingSize+32, 1);
    }
    exitResult(greeting, *greetingSize);
}
void setGreeting(volatile void* begin){
    //greeting should already be in size+data format, so just store directly
    uint32_t size = *(uint16_t*)begin;
    //no need to copy, just tell storage layer what address to store the data under
    persistRestoreMemory(begin, size+2, greetingSize, 0);
}
//handles external calls
void handleAbi(){
    uint8_t abi = abi_area[2];
    switch(abi){
        case 0:
            //get greeting
            greet();
            break;
        case 1:
            //set greeting
            outd(0xEE, *(uint32_t*)&abi_area[3]);
            setGreeting(&abi_area[3]);
            break;
        case 255:
            //kill
            kill();
            break;
    }
}


void start() __attribute__((section(".text.start")));
void start(){
    if(isCreation()){
        getSender(owner);
        persistInitialArea(); //save owner into contract code
        exit();
    }
    handleAbi();
    exit();
    while(1);
}

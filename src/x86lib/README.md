# Qtum's x86lib VM

This VM is based on a project made by [Jordan Earls](https://bitbucket.org/earlz/x86lib) with the original goal of fully emulating the 8086 processor. It has now been adapted into a specialized VM for running smart contracts and other untrusted code. 

The instruction set to eventually be supported is a modified version of i686 with most system-level and ring0 opcodes removed. It also might support the standard i686 FPU instructions (TBD). 

When running make, it will have several outputs. It will output the library files necessary for using it in your own code, and it will also output "x86testbench". The x86testbench program is built for free-form testing of the x86 VM and will eventually emulate the complete smart contract environment. (with mocks and stubs etc so that you can test your own contracts for impossible conditions, etc).

Running the testbench is very simple:

    ./x86testbench myfile.bin

You can also use single-step mode which will print diagnostic info after every instruction:

    ./x86testbench myfile.bin -singlestep

## Machine Info

Memory map:

* 0-0xFFF: read/write config area (unused right now)
* 0x1000 - 0xFFFFF: Read-only code memory (code is loaded here)
* 0x100000 - 0x1FFFFF: read/write scratch memory

The following OUT ports are supported:

* 0: print single character
* 1: print a byte's value in hex
* 2: print a word's value in hex
* 3: print a dword's value in hex
* 4: cause an interrupt (in-progress)
* 0xF0: exit testbench with provided return code
* 0xF3: Dump memory to the external file mem_dump.bin

The following IN ports are supported:

* 0x30: read a single ascii character from stdin

The majority of opcodes are untested and a significant amount of opcodes are not yet implemented. It is recommended to use assembly for any testing right now, though it has run some simple C programs in the past (based in i386 opcode set). 

An example linker script is included in the testbench directory. Note a freestanding no-operating system compiler is required. 

## Example

Example minimum assembly program written for the yasm x86 assembler:

    CPU i386
    BITS 32
    ORG 0x1000 ;required because code is loaded at address 0x1000

    start:
    mov eax, 0 
    out 0xF0, eax ;exit with error code 0
    hlt ;shouldn't reach here

And the program can be asembled and executed very simply:

    yasm testassembly.asm
    ./x86testbench testassembly

With the expected result being:

    Loaded! Beginning execution...
    exiting with code 0

You can also try -singlestep and should see:

    Loaded! Beginning execution...
    OPCODE: op_mov_rW_immW; hex: 0xb8
    EAX: 0
    ECX: 0
    EDX: 0
    EBX: 0
    ESP: 1ff000
    EBP: 0
    ESI: 0
    EDI: 0
    CS: 0
    SS: 0
    DS: 0
    ES: 0
    FS: 0
    GS: 0
    EIP: 1005
    --Flags:0
    CF: 0
    PF: 0
    AF: 0
    ZF: 0
    SF: 0
    TF: 0
    IF: 0
    DF: 0
    OF: 0
    -------------------------------
    exiting with code 0

## Status

x86lib is very much still pre-alpha. If you would like to help contribute, please make a pull request or contact earlz@qtum.org.

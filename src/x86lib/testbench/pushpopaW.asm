
CPU i386
BITS 32
ORG 0x1000

start:
mov eax, 1 
mov ecx, 2 
mov edx, 3
mov ebx, 4

mov ebp, 6
mov esi, 7
mov edi, 8 

PUSHAD

mov eax, 0
mov ecx, 0
mov edx, 0
mov ebx, 0
 
mov ebp, 0
mov esi, 0
mov edi, 0


POPAD
hlt

rep lodsw

;add 10 bytes of padding for reasons
nop
nop
nop
nop

nop
nop
nop
nop

nop
nop

#include <stdint.h>
#include <x86lib.h>

namespace x86Lib{



uint16_t ModRM::GetRegD(){ //This returns the register displacement value
    switch(modrm.rm){
        case 0:
            return this_cpu->Reg16(BX)+this_cpu->Reg16(SI);
            break;
        case 1:
            return this_cpu->Reg16(BX)+this_cpu->Reg16(DI);
            break;
        case 2:
            use_ss=1;
            return this_cpu->Reg16(BP)+this_cpu->Reg16(SI);
            break;
        case 3:
            use_ss=1;
            return this_cpu->Reg16(BP)+this_cpu->Reg16(DI);
            break;
        case 4:
            return this_cpu->Reg16(SI);
            break;
        case 5:
            return this_cpu->Reg16(DI);
        case 6: //immediate Displacement only, so no register displace..
            return 0;
            break;
        case 7:
            return this_cpu->Reg16(BX);
            break;
    }
    return 0;
}

uint32_t ModRM::GetRegD32(){ //This returns the register displacement value
    switch(modrm.rm){
        case 0:
            return this_cpu->regs32[EAX];
        case 1:
            return this_cpu->regs32[ECX];
        case 2:
            return this_cpu->regs32[EDX];
        case 3:
            return this_cpu->regs32[EBX];
            break;
        case 4:
            // SIB
            return 0;
            break;
        case 5: //immediate Displacement only, so no register displace..
            if(modrm.mod != 0){
                return this_cpu->regs32[EBP];
            }
            return 0;
        case 6:
            return this_cpu->regs32[ESI];
        case 7:
            return this_cpu->regs32[EDI];
    }
    return 0;
}

uint16_t ModRM::GetDisp(){
    uint16_t reg;
    reg=GetRegD();
    if(modrm.rm==6){ //Don't worry, it's safe...
        use_ss=1;
        reg=this_cpu->Reg16(BP);
    }
    switch(modrm.mod){
        case 0: //no displacement

            if(modrm.rm==6){ //if only word displacement...
                use_ss=0;
                //eip++;
                //eip++;
                return this_cpu->ReadCode16(1);
            }else{
                return reg;
            }
            break;
        case 1: //byte displacement(signed)
            //eip++;
            return (int16_t)reg+(int8_t)this_cpu->ReadCode8(1);
            break;
        case 2: //word displacement(signed)
            return (int16_t)reg+(int16_t)(this_cpu->ReadCode16(1));
            break;
        case 3: //opcode specific...
            op_specific=1;
            return 0;
            break;
    }
    return 0;
}

uint32_t ModRM::GetDisp32(){
    uint32_t reg;
    reg=GetRegD32();
    switch(modrm.mod){
        case 0: //no displacement
            if(modrm.rm==5){ //only dword displacement...
                return this_cpu->ReadDword(this_cpu->CS, this_cpu->eip+1, CodeFetch);
            }else if(modrm.rm == 4){ //if SIB
                return GetSIBDisp();
            }
            return reg;
            break;
        case 1: //byte displacement(signed)
            if(modrm.rm == 4){
                return (int32_t)GetSIBDisp() + (int8_t)this_cpu->ReadCode8(2);
            }else{
                return (int32_t)reg + (int8_t)this_cpu->ReadCode8(1);
            }
            break;
        case 2: //dword displacement(signed)
            if(modrm.rm == 4){
                //make sure to use eip+2 here to account for SIB
                return (int32_t)GetSIBDisp() + (int32_t)this_cpu->ReadCode32(2);
            }else{
                return (int32_t)reg + (int32_t)this_cpu->ReadCode32(1);
            }
            break;
        case 3: //opcode specific...
            op_specific=1;
            return 0;
            break;
    }
    return 0;
}

uint32_t ModRM::GetSIBDisp(){
    int32_t regindex=0;
    int32_t regbase=0;
    if(sib.index == 4){
        regindex=0;
    }
    int32_t mul=1;
    switch(sib.ss){
        case 1: //skip 0
            mul = 2;
            break;
        case 2:
            mul = 4;
            break;
        case 3:
            mul = 8;
            break;
    }
    switch(sib.index){
        case 0:
            regindex = this_cpu->regs32[EAX];
            break;
        case 1:
            regindex = this_cpu->regs32[ECX];
            break;
        case 2:
            regindex = this_cpu->regs32[EDX];
            break;
        case 3:
            regindex = this_cpu->regs32[EBX];
            break;
        case 4:
            regindex = 0;
            break;
        case 5:
            regindex = this_cpu->regs32[EBP];
            break;
        case 6:
            regindex = this_cpu->regs32[ESI];
            break;
        case 7:
            regindex = this_cpu->regs32[EDI];
            break;
    }

    switch(sib.base){
        case 0:
            regbase = this_cpu->regs32[EAX];
            break;
        case 1:
            regbase = this_cpu->regs32[ECX];
            break;
        case 2:
            regbase = this_cpu->regs32[EDX];
            break;
        case 3:
            regbase = this_cpu->regs32[EBX];
            break;
        case 4:
            regbase = this_cpu->regs32[ESP];
            break;
        case 5:
            if(modrm.mod == 0){
                regbase = 0;
            }else{
                regbase = this_cpu->regs32[EBP];
            }
            break;
        case 6:
            regbase = this_cpu->regs32[ESI];
            break;
        case 7:
            regbase = this_cpu->regs32[EDI];
            break;
    }
    return regindex * mul + regbase;
}


ModRM::ModRM(x86CPU *this_cpu_){
    use_ss=0;
    op_specific=0;
    this_cpu=this_cpu_;
    this_cpu->eip++; //to get past opcode and onto modrm
    this_cpu->ReadCode(&modrm, 0, 1); //sizeof would work here, but weird compilers could potentially introduce consensus break
    this_cpu->ReadCode(&sib, 1, 1);
    jumpBehavior = false;
}

ModRM::~ModRM(){
    if(!jumpBehavior){
        this_cpu->eip+=GetLength() - 1;
    }
}

//reads immediate after ModRM
uint8_t ModRM::Imm8(){
    return this_cpu->ReadCode8(GetLength() + 1);
}
uint16_t ModRM::Imm16(){
    return this_cpu->ReadCode8(GetLength() + 1);
}
uint32_t ModRM::Imm32(){
    return this_cpu->ReadCode8(GetLength() + 1);
}
uint32_t ModRM::ImmW(){
    return this_cpu->ReadCodeW(GetLength() + 1);
}

//The r suffix means /r, which means for op_specific=1, use general registers
uint8_t ModRM::ReadByte(){
    if(this_cpu->Use32BitAddress()){
        return ReadByte32();
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        return this_cpu->Reg8(modrm.rm);
    }else{
        if(use_ss==1){
            return this_cpu->ReadByte(this_cpu->SS,disp);
        }else{
            return this_cpu->ReadByte(this_cpu->DS,disp);
        }
    }
}

uint16_t ModRM::ReadWord(){
    if(this_cpu->Use32BitAddress()){
        return ReadWord32();
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        return this_cpu->Reg16(modrm.rm);
    }else{

        if(use_ss==1){
            return this_cpu->ReadWord(this_cpu->SS,disp);
        }else{
            return this_cpu->ReadWord(this_cpu->DS,disp);
        }
    }
}

uint32_t ModRM::ReadW(){
    if(this_cpu->Use32BitOperand()){
        return ReadDword();
    }else{
        return ReadWord();
    }
}

uint32_t ModRM::ReadA(){
    if(this_cpu->Use32BitAddress()){
        return ReadDword();
    }else{
        return ReadWord();
    }
}

uint32_t ModRM::ReadDword(){
    if(this_cpu->Use32BitAddress()){
        return ReadDword32();
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        return this_cpu->regs32[modrm.rm];
    }else{

        if(use_ss==1){
            return this_cpu->ReadDword(this_cpu->SS,disp);
        }else{
            return this_cpu->ReadDword(this_cpu->DS,disp);
        }
    }
}

void ModRM::WriteByte(uint8_t byte){
    if(this_cpu->Use32BitAddress()){
        return WriteByte32(byte);
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        this_cpu->SetReg8(modrm.rm, byte);
    }else{

        if(use_ss==1){
            this_cpu->WriteByte(this_cpu->SS,disp,byte);
        }else{
            this_cpu->WriteByte(this_cpu->DS,disp,byte);
        }
    }
}
void ModRM::WriteWord(uint16_t word){
    if(this_cpu->Use32BitAddress()){
        return WriteWord32(word);
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        this_cpu->SetReg16(modrm.rm, word);
    }else{

        if(use_ss==1){
            this_cpu->WriteWord(this_cpu->SS,disp,word);
        }else{
            this_cpu->WriteWord(this_cpu->DS,disp,word);
        }
    }
}

void ModRM::WriteW(uint32_t val){
    if(this_cpu->Use32BitOperand()){
        WriteDword(val);
    }else{
        WriteWord(val);
    }
}

void ModRM::WriteDword(uint32_t dword){
    if(this_cpu->Use32BitAddress()){
        return WriteDword32(dword);
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        this_cpu->regs32[modrm.rm]=dword;
    }else{

        if(use_ss==1){
            this_cpu->WriteDword(this_cpu->SS,disp,dword);
        }else{
            this_cpu->WriteDword(this_cpu->DS,disp,dword);
        }
    }
}



//The r suffix means /r, which means for op_specific=1, use general registers
uint8_t ModRM::ReadByte32(){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        return this_cpu->Reg8(modrm.rm);
    }else{
        return this_cpu->ReadByte(this_cpu->DS,disp);
    }
}

uint16_t ModRM::ReadWord32(){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        //don't think this is actually possible in 32bit mode, but ok
        return this_cpu->Reg16(modrm.rm);
    }else{
        return this_cpu->ReadWord(this_cpu->DS,disp);
    }
}
uint32_t ModRM::ReadDword32(){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        return this_cpu->regs32[modrm.rm];
    }else{
        return this_cpu->ReadDword(this_cpu->DS,disp);
    }
}

void ModRM::WriteByte32(uint8_t byte){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        this_cpu->SetReg8(modrm.rm, byte);
    }else{
        this_cpu->WriteByte(this_cpu->DS,disp,byte);
    }
}
void ModRM::WriteWord32(uint16_t word){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        this_cpu->SetReg16(modrm.rm, word);
    }else{
        this_cpu->WriteWord(this_cpu->DS,disp,word);
    }
}
void ModRM::WriteDword32(uint32_t dword){
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        this_cpu->regs32[modrm.rm]=dword;
    }else{
        this_cpu->WriteDword(this_cpu->DS,disp,dword);
    }
}

uint8_t ModRM::GetLength(){ //This returns how many total bytes the modrm block consumes
    if(this_cpu->Use32BitAddress()){
        if((modrm.mod==0) && (modrm.rm==5)){
            return 5;
        }
        if(modrm.mod == 3){
            return 1;
        }
        int count=1; //1 for modrm byte
        if(modrm.rm == 4){
            count++; //SIB byte
        }
        switch(modrm.mod){
            case 0:
                count += 0;
                break;
            case 1:
                count += 1;
                break;
            case 2:
                count += 4;
                break;
        }
        return count;
    }else{
        if((modrm.mod==0) && (modrm.rm==6)){
            return 3;
        }
        switch(modrm.mod){
            case 0:
                return 1;
                break;
            case 1:
                return 2;
                break;
            case 2:
                return 3;
                break;
            case 3:
                return 1;
                break;
        }
    }
    return 1; //should never reach here, but to avoid warnings...

} //that was easier than I first thought it would be...
uint8_t ModRM::GetExtra(){ //Get the extra fied from mod_rm
    return modrm.extra;
}

uint32_t ModRM::ReadOffset(){ //This is only used by LEA. It will obtain the offset and not dereference it...
    if(this_cpu->Use32BitAddress()){
        return ReadOffset32();
    }
    use_ss=0;
    op_specific=0;
    uint16_t disp=GetDisp();
    if(op_specific==1){
        throw CpuInt_excp(UNK_IEXCP);
        //We can't return regs16 because it can't get address of a register!
        //return *regs16[modrm.rm];
    }else{
        return disp;
    }
}

uint32_t ModRM::ReadOffset32(){ //This is only used by LEA. It will obtain the offset and not dereference it...
    use_ss=0;
    op_specific=0;
    uint32_t disp=GetDisp32();
    if(op_specific==1){
        throw CpuInt_excp(UNK_IEXCP);
        //We can't return regs16 because it can't get address of a register!
        //return *regs16[modrm.rm];
    }else{
        return disp;
    }
}


}





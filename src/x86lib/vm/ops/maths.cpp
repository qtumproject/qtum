/**
Copyright (c) 2007 - 2009 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This file is part of the x86Lib project.
**/
#include <x86lib.h>
namespace x86Lib{
using namespace std;

//see http://stackoverflow.com/questions/4513746/explain-how-the-af-flag-works-in-an-x86-instructions
#define CalculateSubAF(base, subt) freg.bits.af = ((int32_t)(base&0x0F) - (int32_t)(subt&0x0F) < 0)

uint8_t x86CPU::Sub8(uint8_t base,uint8_t subt){
    uint8_t result = base - subt;
    if(subt>base){freg.bits.cf=1;}else{freg.bits.cf=0;}
    CalculateOF8(result, base, -subt);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF8(result); //do sf
    CalculateSubAF(base, subt);
    return result;
}

uint16_t x86CPU::Sub16(uint16_t base,uint16_t subt){
    uint16_t result = base - subt;
    if(subt>base){freg.bits.cf=1;}else{freg.bits.cf=0;}
    CalculateOF16(result, base, -subt);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF16(result); //do sf
    CalculateSubAF(base, subt);
    return result;
}

uint32_t x86CPU::Sub32(uint32_t base,uint32_t subt){
    uint32_t result = base - subt;
    if(subt>base){freg.bits.cf=1;}else{freg.bits.cf=0;}
    CalculateOF32(result, base, -subt);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF32(result); //do sf
    CalculateSubAF(base, subt);
    return result;
}
uint32_t x86CPU::SubW(uint32_t base,uint32_t subt){
	if(OperandSize16){
		return Sub16(base, subt);
	}else{
		return Sub32(base, subt);
	}
}

#define CalculateAddAF(base, adder) freg.bits.af = ((base&0x0F)+(adder&0x0F) > 15)

uint8_t x86CPU::Add8(uint8_t base,uint8_t adder){
    uint8_t result = base + adder;
    freg.bits.cf = (result < min(base, adder));
    CalculateOF8(result, base, adder);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF8(result); //do sf
    CalculateAddAF(base, adder);
    return result;
}

uint16_t x86CPU::Add16(uint16_t base,uint16_t adder){
    uint16_t result = base + adder;
    freg.bits.cf = (result < min(base, adder));
    CalculateOF16(result, base, adder);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF16(result); //do sf
    CalculateAddAF(base, adder);
    return result;
}

uint32_t x86CPU::Add32(uint32_t base,uint32_t adder){
    uint32_t result = base + adder;
    freg.bits.cf = (result < min(base, adder));
    CalculateOF32(result, base, adder);
    CalculateZF(result);
    CalculatePF(result); //do pf
    CalculateSF32(result); //do sf
    CalculateAddAF(base, adder);
    return result;
}

uint32_t x86CPU::AddW(uint32_t base,uint32_t adder){
	if(OperandSize16){
		return Add16(base, adder);
	}else{
		return Add32(base, adder);
	}
}



uint8_t x86CPU::And8(uint8_t base,uint8_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base&mask;
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
	return base;
}

uint16_t x86CPU::And16(uint16_t base,uint16_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base&mask;
	CalculatePF(base);
	CalculateSF16(base);
    CalculateZF(base);
	return base;
}

uint32_t x86CPU::And32(uint32_t base,uint32_t mask){
    freg.bits.of=0;
    freg.bits.cf=0;
    base=base&mask;
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    return base;
}

uint32_t x86CPU::AndW(uint32_t base,uint32_t mask){
	if(OperandSize16){
		return And16(base, mask);
	}else{
		return And32(base, mask);
	}
}

//Not affects no flags, so just use ~

uint8_t x86CPU::Or8(uint8_t base,uint8_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base|mask;
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
	return base;
}

uint16_t x86CPU::Or16(uint16_t base,uint16_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base|mask;
	CalculatePF(base);
	CalculateSF16(base);
    CalculateZF(base);
	return base;
}

uint32_t x86CPU::Or32(uint32_t base,uint32_t mask){
    freg.bits.of=0;
    freg.bits.cf=0;
    base=base|mask;
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    return base;
}
uint32_t x86CPU::OrW(uint32_t base,uint32_t arg){
	if(OperandSize16){
		return Or16(base, arg);
	}else{
		return Or32(base, arg);
	}
}

uint8_t x86CPU::Xor8(uint8_t base,uint8_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base^mask;
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
	return base;
}

uint16_t x86CPU::Xor16(uint16_t base,uint16_t mask){
	freg.bits.of=0;
	freg.bits.cf=0;
	base=base^mask;
	CalculatePF(base);
	CalculateSF16(base);
    CalculateZF(base);
	return base;
}
uint32_t x86CPU::Xor32(uint32_t base,uint32_t mask){
    freg.bits.of=0;
    freg.bits.cf=0;
    base=base^mask;
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    return base;
}
uint32_t x86CPU::XorW(uint32_t base,uint32_t arg){
	if(OperandSize16){
		return Xor16(base, arg);
	}else{
		return Xor32(base, arg);
	}
}

#define MSB8(x) (((x) & 0x80) > 0)
#define MSB16(x) (((x) & 0x8000) > 0)
#define MSB32(x) (((x) & 0x80000000) > 0)

uint8_t x86CPU::ShiftLogicalRight8(uint8_t base,uint8_t count){
	count&=0x1F; //only use bottom 5 bits
	if(count==0){
		return base;
	}
    if(count == 1){
	   freg.bits.of=MSB8(base);
    }else{
        freg.bits.of = 0; //of is undefined in this condition
    }
	freg.bits.cf=((base>>(count-1))&1) > 0;
	base=base>>count;
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
	return base;
}

uint16_t x86CPU::ShiftLogicalRight16(uint16_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        return base;
    }
    if(count == 1){
       freg.bits.of=MSB16(base);
    }else{
        freg.bits.of = 0; //of is undefined in this condition
    }
    freg.bits.cf=((base>>(count-1))&1) > 0;
    base=base>>count;
    CalculatePF(base);
    CalculateSF16(base);
    CalculateZF(base);
    return base;
}

uint32_t x86CPU::ShiftLogicalRight32(uint32_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        return base;
    }
    if(count == 1){
       freg.bits.of=MSB32(base);
    }else{
        freg.bits.of = 0; //of is undefined in this condition
    }
    freg.bits.cf=((base>>(count-1))&1) > 0;
    base=base>>count;
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    return base;
}

uint32_t x86CPU::ShiftLogicalRightW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return ShiftLogicalRight16(base, arg);
	}else{
		return ShiftLogicalRight32(base, arg);
	}
}

uint8_t x86CPU::ShiftArithmeticRight8(uint8_t base,uint8_t count){
	count&=0x1F; //only use bottom 5 bits
	if(count==0){
		return base;
	}
	freg.bits.cf = ((base >> (count-1))&1) > 0;
	if(MSB8(base)){ //if signed
		base=(base>>count)|(~(0xFF>>count)); //Replace shifted in 0 bits with 1 bits for sign
	}else{
		base=(base>>count);
	}
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
    freg.bits.of=0; //SAR clears OF for all 1 bit shifts (>1 bit is undefined)
	return base;
}
uint16_t x86CPU::ShiftArithmeticRight16(uint16_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        return base;
    }
    freg.bits.cf = ((base >> (count-1))&1) > 0;
    if(MSB16(base)){ //if signed
        base=(base>>count)|(~(0xFFFF>>count)); //Replace shifted in 0 bits with 1 bits for sign
    }else{
        base=(base>>count);
    }
    CalculatePF(base);
    CalculateSF16(base);
    CalculateZF(base);
    freg.bits.of=0; //SAR clears OF for all 1 bit shifts (>1 bit is undefined)
    return base;
}

uint32_t x86CPU::ShiftArithmeticRight32(uint32_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        return base;
    }
    freg.bits.cf = ((base >> (count-1))&1) > 0;
    if(MSB32(base)){ //if signed
        base=(base>>count)|(~(0xFFFFFFFF>>count)); //Replace shifted in 0 bits with 1 bits for sign
    }else{
        base=(base>>count);
    }
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    freg.bits.of=0; //SAR clears OF for all 1 bit shifts (>1 bit is undefined)
    return base;
}

uint32_t x86CPU::ShiftArithmeticRightW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return ShiftArithmeticRight16(base, arg);
	}else{
		return ShiftArithmeticRight32(base, arg);
	}
}

uint8_t x86CPU::ShiftLogicalLeft8(uint8_t base,uint8_t count){
	count&=0x1F; //only use bottom 5 bits
	if(count==0){
        //no flags modified if 0
		return base;
	}
	freg.bits.cf=((base<<(count-1))&0x80) > 0;
	base=base << count;
    if(count == 1){
        //OF = MSB(Destination) ^ CF
        freg.bits.of = MSB8(base) ^ freg.bits.cf;
    }
	CalculatePF(base);
	CalculateSF8(base);
    CalculateZF(base);
	return base;
}

uint16_t x86CPU::ShiftLogicalLeft16(uint16_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        //no flags modified if 0
        return base;
    }
    freg.bits.cf=((base<<(count-1))&0x8000) > 0;
    base=base << count;
    if(count == 1){
        //OF = MSB(Destination) ^ CF
        freg.bits.of = MSB16(base) ^ freg.bits.cf;
    }
    CalculatePF(base);
    CalculateSF16(base);
    CalculateZF(base);
    return base;
}

uint32_t x86CPU::ShiftLogicalLeft32(uint32_t base,uint8_t count){
    count&=0x1F; //only use bottom 5 bits
    if(count==0){
        //no flags modified if 0
        return base;
    }
    freg.bits.cf=((base<<(count-1))&0x80000000) > 0;
    base=base << count;
    if(count == 1){
        //OF = MSB(Destination) ^ CF
        freg.bits.of = MSB32(base) ^ freg.bits.cf;
    }
    CalculatePF(base);
    CalculateSF32(base);
    CalculateZF(base);
    return base;
}

uint32_t x86CPU::ShiftLogicalLeftW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return ShiftLogicalLeft16(base, arg);
	}else{
		return ShiftLogicalLeft32(base, arg);
	}
}

uint16_t x86CPU::ShiftLeftDoublePrecision16(uint16_t des, uint16_t src, uint8_t count){
    count %= 16;
    if(count==0){
        return des;
    }
    freg.bits.cf=((des<<(count-1))&0x8000) > 0;
    uint8_t rcount = 16 - count;
    des = des << count;
    src = src >> rcount;
    des = des | src;
    if(count == 1) {
        freg.bits.of = MSB16(des) ^ freg.bits.cf;
    }
    CalculatePF(des);
    CalculateSF16(des);
    CalculateZF(des);
    return des;
}

uint32_t x86CPU::ShiftLeftDoublePrecision32(uint32_t des, uint32_t src, uint8_t count){
    count %= 32;
    if(count==0){
        return des;
    }
    freg.bits.cf=((des<<(count-1))&0x80000000) > 0;
    uint8_t rcount = 32 - count;
    des = des << count;
    src = src >> rcount;
    des = des | src;
    if(count == 1) {
        freg.bits.of = MSB32(des) ^ freg.bits.cf;
    }
    CalculatePF(des);
    CalculateSF32(des);
    CalculateZF(des);
    return des;
}

uint32_t x86CPU::ShiftLeftDoublePrecisionW(uint32_t des, uint32_t src, uint8_t arg){
	if(OperandSize16){
		return ShiftLeftDoublePrecision16(des, src, arg);
	}else{
		return ShiftLeftDoublePrecision32(des, src, arg);
	}
}

/**ToDo: Possibly adapt BOCHS source so that we avoid this loop crap...**/
uint8_t x86CPU::RotateRight8(uint8_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 8; //rotate 8 should be the same as rotate 16, etc
    if(count == 0){
        return base; //do nothing
    }
    uint8_t result = (base >> count) | (base << (8-count));
    freg.bits.cf = MSB8(result);
    if(count == 1){
        freg.bits.of = MSB8(result) ^ ((result & 0x40) > 0);
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
    return result;
}

uint16_t x86CPU::RotateRight16(uint16_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 16; //rotate 8 should be the same as rotate 16, etc
    if(count == 0){
        return base; //do nothing
    }
    uint16_t result = (base >> count) | (base << (16-count));
    freg.bits.cf = MSB16(result);
    if(count == 1){
        freg.bits.of = MSB16(result) ^ ((result & 0x4000) > 0);
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
    return result;
}
uint32_t x86CPU::RotateRight32(uint32_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    if(count == 0){
        return base; //do nothing
    }

    uint32_t result = (base >> count) | (base << (32-count));
    freg.bits.cf = MSB32(result);
    if(count == 1){
        freg.bits.of = MSB32(result) ^ ((result & 0x40000000) > 0);
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
    return result;
}

uint32_t x86CPU::RotateRightW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return RotateRight16(base, arg);
	}else{
		return RotateRight32(base, arg);
	}
}

uint16_t x86CPU::ShiftRightDoublePrecision16(uint16_t des, uint16_t src, uint8_t count){
    count %= 16;
    if(count==0){
        return des;
    }
    freg.bits.cf=((des>>(count-1))&0x1) > 0;
    des = (des >> count) | (src << (16-count));
    if(count == 1) {
        freg.bits.of = MSB16(des) ^ ((des & 0x4000) > 0);
    }else{
        freg.bits.of = 0;
    }
    CalculatePF(des);
    CalculateSF16(des);
    CalculateZF(des);
    return des;
}

uint32_t x86CPU::ShiftRightDoublePrecision32(uint32_t des, uint32_t src, uint8_t count){
    count %= 32;
    if(count==0){
        return des;
    }
    freg.bits.cf=((des>>(count-1))&0x1) > 0;
    des = (des >> count) | (src << (32-count));
    if(count == 1) {
        freg.bits.of = MSB32(des) ^ ((des & 0x40000000) > 0);
    }else{
        freg.bits.of = 0;
    }
    CalculatePF(des);
    CalculateSF32(des);
    CalculateZF(des);
    return des;
}

uint32_t x86CPU::ShiftRightDoublePrecisionW(uint32_t des, uint32_t src, uint8_t arg){
	if(OperandSize16){
		return ShiftRightDoublePrecision16(des, src, arg);
	}else{
		return ShiftRightDoublePrecision32(des, src, arg);
	}
}

uint8_t x86CPU::RotateLeft8(uint8_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 8; //rotate 8 should be the same as rotate 16, etc
    //TODO: validate on hardware. Does ROL 8 affect flags?
    if(count == 0){
        return base; //do nothing
    }
    //NOTE: be careful changing this logic, this is optimized to generate an ROL/ROR instruction
    uint8_t result = (base << count) | (base >> (8-count));
    freg.bits.cf = result & 1;
    if(count == 1){
        freg.bits.of = MSB8(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
	return result;
}

uint16_t x86CPU::RotateLeft16(uint16_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 16;
    if(count == 0){
        return base; //do nothing
    }
    uint16_t result = (base << count) | (base >> (16-count));
    freg.bits.cf = result & 1;
    if(count == 1){
        freg.bits.of = MSB16(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
    return result;
}

uint32_t x86CPU::RotateLeft32(uint32_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    if(count == 0){
        return base; //do nothing
    }
    uint32_t result = (base << count) | (base >> (32-count));
    freg.bits.cf = result & 1;
    if(count == 1){
        freg.bits.of = MSB32(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    //does not affect SF, ZF, AF, or PF
    return result;
}

uint32_t x86CPU::RotateLeftW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return RotateLeft16(base, arg);
	}else{
		return RotateLeft32(base, arg);
	}
}

uint8_t x86CPU::RotateCarryLeft8(uint8_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 8;
    if(count == 0){
        return base; //do nothing
    }
    uint16_t bigbase = base << 1 | freg.bits.cf; //make room for carry
    //now have a 9 bit structure...
    uint16_t result = (bigbase << count) | (bigbase >> (9-count));
    freg.bits.cf = (result & (1 << 9)) > 0;
    result >>= 1; //adjust for removal of CF
    result &= 0xFF;
    if(count == 1){
        freg.bits.of = MSB8(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint16_t x86CPU::RotateCarryLeft16(uint16_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 16;
    if(count == 0){
        return base; //do nothing
    }
    uint32_t bigbase = base << 1 | freg.bits.cf; //make room for carry
    //now have a 17 bit structure...
    uint32_t result = (bigbase << count) | (bigbase >> (17-count));
    freg.bits.cf = (result & (1 << 17)) > 0;
    result >>= 1;
    result &= 0xFFFF;
    if(count == 1){
        freg.bits.of = MSB16(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint32_t x86CPU::RotateCarryLeft32(uint32_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    if(count == 0){
        return base; //do nothing
    }
    uint64_t bigbase = ((uint64_t)base << 1) | freg.bits.cf; //make room for carry
    //now have a 33 bit structure...
    uint64_t result = (bigbase << count) | (bigbase >> (33-count));
    freg.bits.cf = (result & (1l << 33)) > 0;
    result >>= 1;
    result &= 0xFFFFFFFF;
    if(count == 1){
        freg.bits.of = MSB32(result) ^ freg.bits.cf;
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint32_t x86CPU::RotateCarryLeftW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return RotateCarryLeft16(base, arg);
	}else{
		return RotateCarryLeft32(base, arg);
	}
}

uint8_t x86CPU::RotateCarryRight8(uint8_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 8;
    if(count == 0){
        return base; //do nothing
    }
    uint16_t bigbase = base | freg.bits.cf << 8; //add carry
    //now have a 9 bit structure...
    uint16_t result = (bigbase >> count) | (bigbase << (9-count));
    freg.bits.cf = (result & (1 << 8)) > 0;
    result &= 0xFF;
    if(count == 1){
        freg.bits.of = MSB8(result) ^ ((result & 0x40) > 0);
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint16_t x86CPU::RotateCarryRight16(uint16_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    count %= 16;
    if(count == 0){
        return base; //do nothing
    }
    uint32_t bigbase = base | freg.bits.cf << 16; //add carry
    //now have a 9 bit structure...
    uint16_t result = (bigbase >> count) | (bigbase << (17-count));
    freg.bits.cf = (result & (1 << 16)) > 0;
    result &= 0xFFFF;
    if(count == 1){
        freg.bits.of = MSB16(result) ^ ((result & 0x4000) > 0);
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint32_t x86CPU::RotateCarryRight32(uint32_t base,uint8_t count){
    count &= 0x1F; //only use bottom 5 bits
    if(count == 0){
        return base; //do nothing
    }
    uint64_t bigbase = base | (uint64_t)freg.bits.cf << 32; //add carry
    //now have a 9 bit structure...
    uint64_t result = (bigbase >> count) | (bigbase << (33-count));
    freg.bits.cf = (result & (1l << 32)) > 0;
    result &= 0xFFFFFFFF;
    if(count == 1){
        freg.bits.of = MSB32(result) ^ ((result & 0x40000000) > 0);
    }else{
        freg.bits.of = 0;
    }
    return result;
}

uint32_t x86CPU::RotateCarryRightW(uint32_t base,uint8_t arg){
	if(OperandSize16){
		return RotateCarryRight16(base, arg);
	}else{
		return RotateCarryRight32(base, arg);
	}
}







void x86CPU::op_sub_al_imm8(){ //0x2C
	SetReg8(AL, Sub8(Reg8(AL),ReadCode8(1)));
	eip++;
}

void x86CPU::op_sub_axW_immW(){ //0x2D
	SetReg(AX, SubW(Reg(AX),ImmW()));
}


void x86CPU::op_sub_rm8_r8(){
	ModRM rm8(this);
	rm8.WriteByte(Sub8(rm8.ReadByte(),Reg8(rm8.GetExtra())));
}

void x86CPU::op_sub_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(SubW(rm.ReadW(),Reg(rm.GetExtra())));
}
void x86CPU::op_sub_r8_rm8(){
	ModRM rm8(this);
	SetReg8(rm8.GetExtra(), Sub8(Reg8(rm8.GetExtra()),rm8.ReadByte()));
}

void x86CPU::op_sub_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), SubW(Reg(rm.GetExtra()), rm.ReadW()));
}

void x86CPU::op_sub_rm8_imm8(ModRM &rm8){ //group 0x80 /5
	rm8.WriteByte(Sub8(rm8.ReadByte(),ReadByte(cCS,eip+rm8.GetLength(), CodeFetch)));
}

void x86CPU::op_sub_rmW_immW(ModRM &rm){ //Group 0x81 /5
	rm.WriteW(SubW(rm.ReadW(), ReadW(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_sub_rmW_imm8(ModRM &rm){ //group 0x83 /5
	rm.WriteW(SubW(rm.ReadW(), SignExtend8to32(ReadByte(cCS,eip+rm.GetLength(),CodeFetch))));
}

/****/
void x86CPU::op_sbb_al_imm8(){
	SetReg8(AL, Sub8(Reg8(AL),ReadCode8(1)-freg.bits.cf));
	eip++;
}

void x86CPU::op_sbb_axW_immW(){
	SetReg(AX, SubW(Reg(AX), ImmW()-freg.bits.cf));
}

void x86CPU::op_sbb_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(SubW(rm.ReadW(), Reg(rm.GetExtra())-freg.bits.cf)); //is this correct with CF subtracting as a 32bit integer?
}
void x86CPU::op_sbb_r8_rm8(){
	ModRM rm8(this);
	SetReg8(rm8.GetExtra(), Sub8(Reg8(rm8.GetExtra()),rm8.ReadByte()-freg.bits.cf));
}
void x86CPU::op_sbb_rm8_r8(){
    ModRM rm8(this);
    rm8.WriteByte(Sub8(rm8.ReadByte(), Reg8(rm8.GetExtra()) - freg.bits.cf));
}
void x86CPU::op_sbb_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), SubW(Reg(rm.GetExtra()), rm.ReadW()-freg.bits.cf));
}

void x86CPU::op_sbb_rm8_imm8(ModRM &rm8){ //group 0x80
	rm8.WriteByte(Sub8(rm8.ReadByte(),ReadByte(cCS,eip+rm8.GetLength(),CodeFetch)-freg.bits.cf));
}

void x86CPU::op_sbb_rmW_immW(ModRM &rm){ //Group 0x81
	rm.WriteW(SubW(rm.ReadW(), ReadW(cCS,eip+rm.GetLength(),CodeFetch)-freg.bits.cf));
}

void x86CPU::op_sbb_rmW_imm8(ModRM &rm){ //group 0x83
	rm.WriteW(SubW(rm.ReadW(), SignExtend8to32(ReadByte(cCS,eip+rm.GetLength(),CodeFetch)-freg.bits.cf)));
}


void x86CPU::op_dec_rW(){ //0x48+r
	freg.bits.r0=freg.bits.cf;
	SetReg(opbyte-0x48, SubW(Reg(opbyte-0x48), 1));
	freg.bits.cf=freg.bits.r0;
}

void x86CPU::op_dec_rm8(ModRM& rm){
	freg.bits.r0=freg.bits.cf;
	rm.WriteByte(Sub8(rm.ReadByte(),1));
	freg.bits.cf=freg.bits.r0;
}

void x86CPU::op_dec_rmW(ModRM& rm){
	freg.bits.r0=freg.bits.cf;
	rm.WriteW(SubW(rm.ReadW(),1));
	freg.bits.cf=freg.bits.r0;
}


//cmp and sub are so similar, that they are both going in here...
void x86CPU::op_cmp_al_imm8(){
	Sub8(Reg8(AL),ReadCode8(1));
	eip++;
}

void x86CPU::op_cmp_axW_immW(){
	SubW(Reg(AX), ImmW());
}

void x86CPU::op_cmp_rm8_r8(){
	ModRM rm(this);
	Sub8(rm.ReadByte(),Reg8(rm.GetExtra()));
}

void x86CPU::op_cmp_rmW_rW(){
	ModRM rm(this);
	SubW(rm.ReadW(),Reg(rm.GetExtra()));
}

void x86CPU::op_cmp_r8_rm8(){
	ModRM rm(this);
	Sub8(Reg8(rm.GetExtra()),rm.ReadByte());
}

void x86CPU::op_cmp_rW_rmW(){
	ModRM rm(this);
	SubW(Reg(rm.GetExtra()),rm.ReadW());
}

void x86CPU::op_cmp_rm8_imm8(ModRM &rm){ //group 80 /7
	Sub8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch));
}

void x86CPU::op_cmp_rmW_immW(ModRM &rm){ //group 81 /7
	SubW(rm.ReadW(), ReadW(cCS, eip+rm.GetLength(), CodeFetch));
}

void x86CPU::op_cmp_rmW_imm8(ModRM &rm){ //group 83 /7
	SubW(rm.ReadW(),SignExtend8to32(ReadByte(cCS, eip+rm.GetLength(),CodeFetch)));
}


void x86CPU::op_add_al_imm8(){
	SetReg8(AL, Add8(Reg8(AL),ReadCode8(1)));
	eip++;
}

void x86CPU::op_add_axW_immW(){
	SetReg(AX, AddW(Reg(AX), ImmW()));
}

void x86CPU::op_add_rm8_r8(){
	ModRM rm8(this);
	rm8.WriteByte(Add8(rm8.ReadByte(),Reg8(rm8.GetExtra())));
}

void x86CPU::op_add_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(AddW(rm.ReadW(),Reg(rm.GetExtra())));
}


void x86CPU::op_add_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), Add8(Reg8(rm.GetExtra()),rm.ReadByte()));
}

void x86CPU::op_add_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), AddW(Reg(rm.GetExtra()), rm.ReadW()));
}

void x86CPU::op_add_rm8_imm8(ModRM &rm){ //Group 0x80 /0
	rm.WriteByte(Add8(rm.ReadWord(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_add_rmW_immW(ModRM &rm){ //Group 0x81 /0
	rm.WriteW(AddW(rm.ReadW(), ReadW(cCS, eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_add_rmW_imm8(ModRM &rm){ //group 0x83 /0
	rm.WriteW(AddW(rm.ReadW(),SignExtend8to32(ReadByte(cCS, eip+rm.GetLength(), CodeFetch))));
}


/****/
void x86CPU::op_adc_al_imm8(){
	SetReg8(AL, Add8(Reg8(AL),ReadCode8(1)+freg.bits.cf));
	eip++;
}

void x86CPU::op_adc_axW_immW(){
	SetReg(AX, AddW(Reg(AX), ImmW() + freg.bits.cf));
}

void x86CPU::op_adc_rm8_r8(){
	ModRM rm8(this);
	rm8.WriteByte(Add8(rm8.ReadByte(),Reg8(rm8.GetExtra())+freg.bits.cf));
}

void x86CPU::op_adc_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(AddW(rm.ReadW(),Reg(rm.GetExtra())+freg.bits.cf));
}

void x86CPU::op_adc_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), Add8(Reg8(rm.GetExtra()),rm.ReadByte()+freg.bits.cf));
}

void x86CPU::op_adc_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), AddW(Reg(rm.GetExtra()), rm.ReadW() + freg.bits.cf));
}

void x86CPU::op_adc_rm8_imm8(ModRM &rm){ //Group 0x80 /2
	rm.WriteByte(Add8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)+freg.bits.cf));
}

void x86CPU::op_adc_rmW_immW(ModRM &rm){ //Group 0x81 /2
	rm.WriteW(AddW(rm.ReadW(),ReadW(cCS, eip + rm.GetLength(), CodeFetch) + freg.bits.cf));
}

void x86CPU::op_adc_rmW_imm8(ModRM &rm){ //group 0x83 /2
	rm.WriteW(AddW(rm.ReadW(),SignExtend8to32(ReadByte(cCS, eip + rm.GetLength(), CodeFetch)) + freg.bits.cf));
}


void x86CPU::op_inc_rW(){ //0x40+r
	freg.bits.r0=freg.bits.cf;
	SetReg(opbyte-0x40, AddW(Reg((opbyte - 0x40)), 1));
	freg.bits.cf=freg.bits.r0;
}

void x86CPU::op_inc_rm8(ModRM &rm){
	freg.bits.r0=freg.bits.cf; //yay for reserved flags! TODO check for qtum
	rm.WriteByte(Add8(rm.ReadByte(),1));
	freg.bits.cf=freg.bits.r0;
}

void x86CPU::op_inc_rmW(ModRM &rm){
	freg.bits.r0=freg.bits.cf;
	rm.WriteW(AddW(rm.ReadW(), 1));
	freg.bits.cf=freg.bits.r0;
}

void x86CPU::op_neg_rm8(ModRM &rm){
	uint8_t tmp=rm.ReadByte();
	if(tmp==0xFF){
		freg.bits.of=1;
		return;
	}
	rm.WriteByte(Sub8(0,tmp));
	if(tmp==0){
		freg.bits.cf=0;
	}else{
		freg.bits.cf=1;
	}
}
void x86CPU::op_neg_rmW(ModRM &rm){
	uint32_t tmp=rm.ReadW();
	rm.WriteW(SubW(0, tmp));
	freg.bits.cf = tmp != 0;
}

void x86CPU::op_div_rm8(ModRM &rm){
	if(rm.ReadByte() == 0){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	if(((Reg16(AX))/rm.ReadByte())>0xFF){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	SetReg8(AL, Reg16(AX)/rm.ReadByte());
	SetReg8(AL, Reg16(AX)%rm.ReadByte());
}

void x86CPU::op16_div_rm16(ModRM &rm){
	if(rm.ReadWord() == 0){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	if((((Reg16(DX)<<16)|(Reg16(AX)))/rm.ReadWord())>0xFFFF){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	SetReg16(AX, ((Reg16(DX)<<16)|(Reg16(AX)))/rm.ReadWord());
	SetReg16(DX, ((Reg16(DX)<<16)|(Reg16(AX)))%rm.ReadWord());
}

void x86CPU::op32_div_rm32(ModRM &rm){
    if(rm.ReadDword() == 0){
        throw CpuInt_excp(DIV0_IEXCP);
    }
    if(((((uint64_t)regs32[EDX]<<32)|((uint64_t)regs32[EAX]))/(uint64_t)rm.ReadDword()) > 0xFFFFFFFF){
        throw CpuInt_excp(DIV0_IEXCP);
    }
    regs32[EAX]=(((uint64_t)regs32[EDX]<<32)|(regs32[EAX]))/rm.ReadDword();
    regs32[EDX]=(((uint64_t)regs32[EDX]<<32)|(regs32[EAX]))%rm.ReadDword();
}


void x86CPU::op_idiv_rm8(ModRM &rm){
	if(rm.ReadByte() ==0){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	uint8_t tmp=rm.ReadByte();
	bool store1,store2;

	tmp=Unsign8(tmp,store1);
	SetReg16(AX, Unsign16(Reg16(AX),store2));
	if(((Reg16(AX))/tmp)>0xFF){
		throw CpuInt_excp(DIV0_IEXCP);
	}

	SetReg8(AL, (Reg16(AX))/tmp);
	SetReg8(AH, (Reg16(AX))%tmp);

	SetReg8(AL, Resign8(Reg8(AL),store1^store2));
}

void x86CPU::op16_idiv_rm16(ModRM &rm){
	if(rm.ReadWord() ==0){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	uint16_t tmp=rm.ReadWord();
	bool store1,store2;
	tmp=Unsign16(tmp,store1);
	uint32_t tmp2=Unsign32(((Reg16(DX))<<16)|(Reg16(AX)),store2);

	if((tmp2/tmp)>0xFFFF){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	SetReg16(AX, tmp2/tmp);
	SetReg16(DX, tmp2%tmp);

	SetReg16(AX, Resign16(Reg16(AX),store1^store2));
}

void x86CPU::op32_idiv_rm32(ModRM &rm){
    uint32_t tmp= rm.ReadDword();
    if(tmp == 0){
        throw CpuInt_excp(DIV0_IEXCP);
    }
    bool store1,store2;
    tmp=Unsign32(tmp,store1);
    uint64_t tmp2=Unsign64((((uint64_t)regs32[EDX])<<32)|(regs32[EAX]),store2);

    if((((uint64_t)tmp2)/((uint64_t)tmp))>0xFFFFFFFF){
        throw CpuInt_excp(DIV0_IEXCP);
    }
    regs32[EAX]=tmp2/tmp;
    regs32[EDX]=tmp2%tmp;

    regs32[EAX]=Resign32(regs32[EAX],store1^store2);
}



void x86CPU::op_mul_rm8(ModRM &rm){
    SetReg16(AX, (Reg8(AL))*rm.ReadByte());
    if((Reg8(AH))>0){ //if tophalf of result has anything in it
        freg.bits.cf=1;
        freg.bits.of=1;
    }else{
        freg.bits.cf=0;
        freg.bits.of=0;
    }
}


void x86CPU::op16_mul_rm16(ModRM &rm){
	uint32_t result;
    result=(Reg16(AX))*rm.ReadWord();
    SetReg16(AX, result&0x0000FFFF);
    SetReg16(DX, (result&0xFFFF0000)>>16);
    if((Reg16(DX))>0){ //if tophalf of result has anything in it
        freg.bits.cf=1;
        freg.bits.of=1;
    }else{
        freg.bits.cf=0;
        freg.bits.of=0;
    }
}

void x86CPU::op32_mul_rm32(ModRM &rm){
    uint64_t result;
    result=(regs32[EAX])*(uint64_t)rm.ReadDword();
    regs32[EAX]=result&0x00000000FFFFFFFF;
    regs32[EDX]=(result&0xFFFFFFFF00000000)>>32;
    if((regs32[EDX])>0){ //if tophalf of result has anything in it
        freg.bits.cf=1;
        freg.bits.of=1;
    }else{
        freg.bits.cf=0;
        freg.bits.of=0;
    }
}

void x86CPU::op_imul_rm8(ModRM &rm){
	bool store1,store2;
	SetReg16(AX, Unsign8(Reg8(AL),store1)*Unsign8(rm.ReadByte(),store2));
	if(Reg16(AX)>0){
		freg.bits.of=1;
		freg.bits.cf=1;
	}else{
		freg.bits.of=0;
		freg.bits.cf=0;
	}
	SetReg16(AX, Resign16(Reg16(AX),store1^store2));
}

void x86CPU::op16_imul_rm16(ModRM &rm){
	bool store1,store2;
	uint32_t result=Unsign16(Reg16(AX),store1)*Unsign16(rm.ReadWord(),store2);
	if((result&0xFFFF0000)>0){
		freg.bits.of=1;
		freg.bits.cf=1;
	}else{
		freg.bits.of=0;
		freg.bits.cf=0;
	}
	result=Resign32(result,store1^store2);
	SetReg16(DX, (result&0xFFFF0000)>>16);
	SetReg16(AX, (result&0xFFFF));
}

void x86CPU::op32_imul_rm32(ModRM &rm){
    bool store1,store2;
    uint64_t result=Unsign32(regs32[EAX],store1) * (uint64_t)Unsign32(rm.ReadDword(),store2);
    if((result&0xFFFFFFFF00000000)>0){
        freg.bits.of=1;
        freg.bits.cf=1;
    }else{
        freg.bits.of=0;
        freg.bits.cf=0;
    }
    result=Resign64(result,store1^store2);
    regs32[EDX]=(result&0xFFFFFFFF00000000)>>32;
    regs32[EAX]=(result&0xFFFFFFFF);
}



void x86CPU::op_and_rm8_r8(){
	ModRM rm(this);
	rm.WriteByte(And8(rm.ReadByte(),Reg8(rm.GetExtra())));
}

void x86CPU::op_and_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(AndW(rm.ReadW(),Reg(rm.GetExtra())));
}

void x86CPU::op_and_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), And8(Reg8(rm.GetExtra()),rm.ReadByte()));
}

void x86CPU::op_and_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), AndW(Reg(rm.GetExtra()), rm.ReadW()));
}

void x86CPU::op_and_al_imm8(){
	SetReg8(AL, And8(Reg8(AL),ReadCode8(1)));
    eip++;
}

void x86CPU::op_and_axW_immW(){
	SetReg(AX, AndW(Reg(AX), ImmW()));
}

void x86CPU::op_and_rm8_imm8(ModRM& rm){
	rm.WriteByte(And8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_and_rmW_immW(ModRM& rm){
	rm.WriteW(AndW(rm.ReadW(), ReadW(cCS, eip + rm.GetLength(), CodeFetch)));
}

void x86CPU::op_and_rmW_imm8(ModRM& rm){ //TODO is AND sign extended!? Intel manuals say nothing
	rm.WriteW(AndW(rm.ReadW(), SignExtend8to32(ReadByte(cCS, eip + rm.GetLength(), CodeFetch))));
}

void x86CPU::op_or_rm8_r8(){
	ModRM rm(this);
	rm.WriteByte(Or8(rm.ReadByte(),Reg8(rm.GetExtra())));
}

void x86CPU::op_or_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(OrW(rm.ReadW(),Reg(rm.GetExtra())));
}

void x86CPU::op_or_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), Or8(Reg8(rm.GetExtra()),rm.ReadByte()));
}

void x86CPU::op_or_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), OrW(Reg(rm.GetExtra()), rm.ReadW()));
}

void x86CPU::op_or_al_imm8(){
	SetReg8(AL, Or8(Reg8(AL),ReadCode8(1)));
    eip++;
}

void x86CPU::op_or_axW_immW(){
	SetReg(AX, OrW(Reg(AX), ImmW()));
}

void x86CPU::op_or_rm8_imm8(ModRM& rm){
	rm.WriteByte(Or8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_or_rmW_immW(ModRM& rm){
	rm.WriteW(OrW(rm.ReadW(), ReadW(cCS, eip + rm.GetLength(), CodeFetch)));
}

void x86CPU::op_or_rmW_imm8(ModRM& rm){
	rm.WriteW(OrW(rm.ReadW(), SignExtend8to32(ReadByte(cCS, eip + rm.GetLength(), CodeFetch))));
}


void x86CPU::op_xor_rm8_r8(){
	ModRM rm(this);
	rm.WriteByte(Xor8(rm.ReadByte(),Reg8(rm.GetExtra())));
}

void x86CPU::op_xor_rmW_rW(){
	ModRM rm(this);
	rm.WriteW(XorW(rm.ReadW(), Reg(rm.GetExtra())));
}

void x86CPU::op_xor_r8_rm8(){
	ModRM rm(this);
	SetReg8(rm.GetExtra(), Xor8(Reg8(rm.GetExtra()),rm.ReadByte()));
}

void x86CPU::op_xor_rW_rmW(){
	ModRM rm(this);
	SetReg(rm.GetExtra(), XorW(Reg(rm.GetExtra()), rm.ReadW()));
}

void x86CPU::op_xor_al_imm8(){
	SetReg8(AL, Xor8(Reg8(AL),ReadCode8(1)));
    eip++;
}

void x86CPU::op_xor_axW_immW(){
	SetReg(AX, XorW(Reg(AX), ImmW()));
}

void x86CPU::op_xor_rm8_imm8(ModRM& rm){
	rm.WriteByte(Xor8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_xor_rmW_immW(ModRM& rm){
	rm.WriteW(XorW(rm.ReadW(), ReadW(cCS, eip + rm.GetLength(), CodeFetch)));
}

void x86CPU::op_xor_rmW_imm8(ModRM& rm){
	rm.WriteW(XorW(rm.ReadW(),SignExtend8to32(ReadByte(cCS, eip + rm.GetLength(), CodeFetch))));
}

void x86CPU::op_test_rm8_r8(){
	ModRM rm(this);
	And8(rm.ReadByte(),Reg8(rm.GetExtra()));
}

void x86CPU::op_test_rmW_rW(){
	ModRM rm(this);
	AndW(rm.ReadW(),Reg(rm.GetExtra()));
}

void x86CPU::op_test_al_imm8(){
	And8(Reg8(AL),ReadCode8(1));
    eip++;
}
void x86CPU::op_test_axW_immW(){
	AndW(Reg(AX), ImmW());
}

void x86CPU::op_test_rm8_imm8(ModRM& rm){
	And8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch));
}

void x86CPU::op_test_rmW_immW(ModRM& rm){
	AndW(rm.ReadW(),ReadW(cCS, eip + rm.GetLength(), CodeFetch));
}

void x86CPU::op_test_rmW_imm8(ModRM& rm){
	AndW(rm.ReadW(),SignExtend8to32(ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_shr_rm8_cl(ModRM &rm){
	rm.WriteByte(ShiftLogicalRight8(rm.ReadByte(),Reg8(CL)));
}
void x86CPU::op_shr_rmW_cl(ModRM &rm){
	rm.WriteW(ShiftLogicalRightW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_shr_rm8_imm8(ModRM &rm){
	rm.WriteByte(ShiftLogicalRight8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_shr_rmW_imm8(ModRM &rm){
	rm.WriteW(ShiftLogicalRightW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_shl_rm8_cl(ModRM &rm){
	rm.WriteByte(ShiftLogicalLeft8(rm.ReadByte(),Reg8(CL)));
}

void x86CPU::op_shl_rmW_cl(ModRM &rm){
	rm.WriteW(ShiftLogicalLeftW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_shl_rm8_imm8(ModRM &rm){
	rm.WriteByte(ShiftLogicalLeft8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_shl_rmW_imm8(ModRM &rm){
	rm.WriteW(ShiftLogicalLeftW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_shld_rmW_rW_imm8(){
    ModRM rm(this);
    rm.WriteW(ShiftLeftDoublePrecisionW(rm.ReadW(),Reg(rm.GetExtra()),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
    eip++;
}

void x86CPU::op_shld_rmW_rW_cl(){
    ModRM rm(this);
    rm.WriteW(ShiftLeftDoublePrecisionW(rm.ReadW(),Reg(rm.GetExtra()),Reg8(CL)));
}

void x86CPU::op_shrd_rmW_rW_imm8(){
    ModRM rm(this);
    rm.WriteW(ShiftRightDoublePrecisionW(rm.ReadW(),Reg(rm.GetExtra()),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
    eip++;
}

void x86CPU::op_shrd_rmW_rW_cl(){
    ModRM rm(this);
    rm.WriteW(ShiftRightDoublePrecisionW(rm.ReadW(),Reg(rm.GetExtra()),Reg8(CL)));
}

void x86CPU::op_sar_rm8_cl(ModRM &rm){
	rm.WriteByte(ShiftArithmeticRight8(rm.ReadByte(),Reg8(CL)));
}

void x86CPU::op_sar_rmW_cl(ModRM &rm){
	rm.WriteW(ShiftArithmeticRightW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_sar_rm8_imm8(ModRM &rm){
	rm.WriteByte(ShiftArithmeticRight8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_sar_rmW_imm8(ModRM &rm){
	rm.WriteByte(ShiftArithmeticRight8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rol_rm8_cl(ModRM &rm){
	rm.WriteByte(RotateLeft8(rm.ReadByte(),Reg8(CL)));
}

void x86CPU::op_rol_rmW_cl(ModRM &rm){
	rm.WriteW(RotateLeftW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_rol_rm8_imm8(ModRM &rm){
	rm.WriteByte(RotateLeft8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rol_rmW_imm8(ModRM &rm){
	rm.WriteW(RotateLeftW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_ror_rm8_cl(ModRM &rm){
	rm.WriteByte(RotateRight8(rm.ReadByte(),Reg8(CL)));
}
void x86CPU::op_ror_rmW_cl(ModRM &rm){
	rm.WriteW(RotateRightW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_ror_rm8_imm8(ModRM &rm){
	rm.WriteByte(RotateRight8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_ror_rmW_imm8(ModRM &rm){
	rm.WriteW(RotateRightW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rcl_rm8_cl(ModRM &rm){
	rm.WriteByte(RotateCarryLeft8(rm.ReadByte(),Reg8(CL)));
}
void x86CPU::op_rcl_rmW_cl(ModRM &rm){
	rm.WriteW(RotateCarryLeftW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_rcl_rm8_imm8(ModRM &rm){
	rm.WriteByte(RotateCarryLeft8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rcl_rmW_imm8(ModRM &rm){
	rm.WriteW(RotateCarryLeftW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rcr_rm8_cl(ModRM &rm){
	rm.WriteByte(RotateCarryRight8(rm.ReadByte(),Reg8(CL)));
}
void x86CPU::op_rcr_rmW_cl(ModRM &rm){
	rm.WriteW(RotateCarryRightW(rm.ReadW(),Reg8(CL)));
}

void x86CPU::op_rcr_rm8_imm8(ModRM &rm){
	rm.WriteByte(RotateCarryRight8(rm.ReadByte(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}

void x86CPU::op_rcr_rmW_imm8(ModRM &rm){
	rm.WriteW(RotateCarryRightW(rm.ReadW(),ReadByte(cCS,eip+rm.GetLength(),CodeFetch)));
}


/****/
void x86CPU::op_shr_rm8_1(ModRM &rm){
	rm.WriteByte(ShiftLogicalRight8(rm.ReadByte(),1));
}
void x86CPU::op_shr_rmW_1(ModRM &rm){
	rm.WriteW(ShiftLogicalRightW(rm.ReadW(),1));
}

void x86CPU::op_shl_rm8_1(ModRM &rm){
	rm.WriteByte(ShiftLogicalLeft8(rm.ReadByte(),1));
}
void x86CPU::op_shl_rmW_1(ModRM &rm){
	rm.WriteW(ShiftLogicalLeftW(rm.ReadW(),1));
}

void x86CPU::op_sar_rm8_1(ModRM &rm){
	rm.WriteByte(ShiftArithmeticRight8(rm.ReadByte(),1));
}
void x86CPU::op_sar_rmW_1(ModRM &rm){
	rm.WriteW(ShiftArithmeticRightW(rm.ReadW(),1));
}

void x86CPU::op_rol_rm8_1(ModRM &rm){
	rm.WriteByte(RotateLeft8(rm.ReadByte(),1));
}
void x86CPU::op_rol_rmW_1(ModRM &rm){
	rm.WriteW(RotateLeftW(rm.ReadW(),1));
}

void x86CPU::op_ror_rm8_1(ModRM &rm){
	rm.WriteByte(RotateRight8(rm.ReadByte(),1));
}
void x86CPU::op_ror_rmW_1(ModRM &rm){
	rm.WriteW(RotateRightW(rm.ReadW(),1));
}

void x86CPU::op_rcl_rm8_1(ModRM &rm){
	rm.WriteByte(RotateCarryLeft8(rm.ReadByte(),1));
}
void x86CPU::op_rcl_rmW_1(ModRM &rm){
	rm.WriteW(RotateCarryLeftW(rm.ReadW(),1));
}

void x86CPU::op_rcr_rm8_1(ModRM &rm){
	rm.WriteByte(RotateCarryRight8(rm.ReadByte(),1));
}
void x86CPU::op_rcr_rmW_1(ModRM &rm){
	rm.WriteW(RotateCarryRightW(rm.ReadW(),1));
}

void x86CPU::op_not_rm8(ModRM &rm){
	rm.WriteByte(~(rm.ReadByte()));
}

void x86CPU::op_not_rmW(ModRM &rm){
	rm.WriteW(~(rm.ReadW()));
}



/**
BCD Opcodes for Open86
By: Alboin, 3-16-07
Modified to work with x86Lib by Jordan(hckr83)
**/


/* Opcode: 0x37 */
void x86CPU::op_aaa() {
	if((Reg8(AL) & 0x0f) > 9 || freg.bits.af == 1) {
		SetReg8(AL, Reg8(AL) + 6);
		SetReg8(AH, Reg8(AH) + 1);
		freg.bits.af = 1;
		freg.bits.cf = 1;
	}
	else {
		freg.bits.af = 0;
		freg.bits.cf = 0;
	}
	SetReg8(AL, Reg8(AL) & 0x0f);
}


/* Opcode: 0x27 */
void x86CPU::op_daa() {
	if((Reg8(AL) & 0x0f) > 9 || freg.bits.af == 1) {
		SetReg8(AL, Reg8(AL) + 6);
		SetReg8(AH, Reg8(AH) + 1);
	}
	else
		freg.bits.af = 0;

	if(Reg8(AL) > 0x9f || freg.bits.cf == 1) {
			SetReg8(AL, Reg8(AL) + 0x60);
			freg.bits.cf = 1;
	}
	else
		freg.bits.cf = 0;
}

/* Opcode: 0x2F */
void x86CPU::op_das() {
	if((Reg8(AL) & 0x0f) > 9 || freg.bits.af == 1) {
		SetReg8(AL, Reg8(AL) - 6);
		freg.bits.af = 1;
	}
	else
		freg.bits.af = 0;

	if(Reg8(AL) > 0x9f || freg.bits.cf == 1) {
			SetReg8(AL, Reg8(AL) - 0x60);
			freg.bits.cf = 1;
	}
	else
		freg.bits.cf = 0;
}

/* Opcode: 0x3F */
void x86CPU::op_aas() {
	if((Reg8(AL) & 0x0f) > 9 || freg.bits.af == 1) {
		SetReg8(AL, Reg8(AL) -6);
		SetReg8(AH, Reg8(AH) - 1);
		freg.bits.af = 1;
		freg.bits.cf = 1;
	}
	else {
		freg.bits.af = 0;
		freg.bits.cf = 0;
	}
	SetReg8(AL, Reg8(AL) & 0x0f);
}



void x86CPU::op_aad_imm8(){
	//AAD is a weird opcode because it is two bytes
	//for "apparently" no reason. But really, it is just
	//undocumented... the 0x0A in the second byte is for
	//multiplecation...but it can be changed...
	SetReg8(AL, (Reg8(AH))*(ReadCode8(1))+Reg8(AL));
	SetReg8(AH, 0);
    eip++;
}

void x86CPU::op_aam_imm8(){
	//same wiht the 0x0A operand as above..
    uint8_t imm = ReadCode8(1);
	if(imm==0){
		throw CpuInt_excp(DIV0_IEXCP);
	}
	SetReg8(AH, (Reg8(AL))/imm);
	SetReg8(AL, (Reg8(AL))%imm);
    eip++;
}


void x86CPU::op_imul_rW_rmW_immW(){

ModRM rm(this);

if(OperandSize16){
	short  dest,source0,source1;
	int    temp;

    source0 = rm.ReadW();
	source1 = ReadWord(cCS,eip+rm.GetLength(),CodeFetch);
	dest = source0 * source1;
	temp = source0 * source1;
	SetReg(rm.GetExtra(), dest);
	if(dest == temp){
		freg.bits.cf = 0;
		freg.bits.of = 0;
	}else{
		freg.bits.cf = 1;
		freg.bits.of = 1;
	}
	eip+=2;
}else{
	int  dest,source0,source1;
	long    temp;
	
	source0 = rm.ReadW();
	source1 = ReadDword(cCS,eip+rm.GetLength(),CodeFetch);
	dest = source0 * source1;
	temp = source0 * source1;
	SetReg(rm.GetExtra(), dest);
	if(dest == temp){
		freg.bits.cf = 0;
		freg.bits.of = 0;
	}else{
		freg.bits.cf = 1;
		freg.bits.of = 1;
	}
	eip+=4;
}

}


void x86CPU::op_imul_rW_rmW_imm8(){

ModRM rm(this);

if(OperandSize16){
	uint16_t  dest,source0,source1;
	uint32_t  temp;

    source0 = rm.ReadW();
	source1 = SignExtend8to16(ReadByte(cCS,eip+rm.GetLength(),CodeFetch));
	dest = source0 * source1;
	temp = source0 * source1;
	SetReg(rm.GetExtra(), dest);
	if(dest == temp){
		freg.bits.cf = 0;
		freg.bits.of = 0;
	}else{
		freg.bits.cf = 1;
		freg.bits.of = 1;
	}
	eip+=1;
}else{
	uint32_t  dest,source0,source1;
	uint64_t    temp;
	
	source0 = rm.ReadW();
	source1 = SignExtend8to32(ReadByte(cCS,eip+rm.GetLength(),CodeFetch));
	dest = source0 * source1;
	temp = source0 * source1;
	SetReg(rm.GetExtra(), dest);
	if(dest == temp){
		freg.bits.cf = 0;
		freg.bits.of = 0;
	}else{
		freg.bits.cf = 1;
		freg.bits.of = 1;
	}
	eip+=1;
}

}





};




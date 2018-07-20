/**
Copyright (c) 2007 - 2010 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
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


/*Opcode naming key:

sizes:
W    - Universal word (32bit by default, but can be overridden to 16bits by operand size prefix 0x66)
E    - Universal Extended Word (64bit by default, but can be overridden to 32bits by operand size prefix 0x66)
F    - Universal "far" word (48bit by default, can be overridden to 32bit by operand size prefix)
A    - Universal address (32bit by default, but can be overridden to 16bits by address size prefix 0x67)


argument types:
imm  - immediate followed by size in bits, or W for universal word
m    - immediate memory address followed by size in bits of data stored there. Always uses universal address for actual address size
rel  - immediate relative jump target followed by size in bits
rm   - Mod R/M byte, followed by size in bits or W for universal word (note: rm8 means 8 bit data size, not address)
r    - Direct register
sr   - segment register

opcode types:

op   - universal opcode
op16 - 16bit only opcode
op32 - 32bit only opcode (useful for when 16bit and 32bit functions are drastically different)

For opcodes with fixed arguments that take a fixed universal word operand, it uses W instead of the typical 'd' or 'w' suffix.
For example, movsw and movsd are separate instructions but both handled by op_movsW()

Opcodes with registers in the name can have W after them to indicate that they operate on either the 32bit or 16bit register

Sizes should be specified explicitly. It should never be like `op_inc_rm`, it should be `op_inc_rmW`
**/

/**This file contains the opcode function definitions and prototypes for x86CPU**/

/**NOTE! this is included INSIDE of a class, so this file is somewhat limited...**/
uint8_t Add8(uint8_t,uint8_t);
uint16_t Add16(uint16_t,uint16_t);
uint32_t Add32(uint32_t,uint32_t);
uint32_t AddW(uint32_t,uint32_t);
uint8_t Sub8(uint8_t,uint8_t);
uint16_t Sub16(uint16_t,uint16_t);
uint32_t Sub32(uint32_t,uint32_t);
uint32_t SubW(uint32_t,uint32_t);
uint8_t And8(uint8_t,uint8_t);
uint16_t And16(uint16_t,uint16_t);
uint32_t And32(uint32_t,uint32_t);
uint32_t AndW(uint32_t,uint32_t);
uint8_t Or8(uint8_t,uint8_t);
uint16_t Or16(uint16_t,uint16_t);
uint32_t Or32(uint32_t,uint32_t);
uint32_t OrW(uint32_t,uint32_t);
uint8_t Xor8(uint8_t,uint8_t);
uint16_t Xor16(uint16_t,uint16_t);
uint32_t Xor32(uint32_t,uint32_t);
uint32_t XorW(uint32_t,uint32_t);
uint8_t ShiftLogicalRight8(uint8_t,uint8_t);
uint16_t ShiftLogicalRight16(uint16_t,uint8_t);
uint32_t ShiftLogicalRight32(uint32_t,uint8_t);
uint32_t ShiftLogicalRightW(uint32_t,uint8_t);
uint8_t ShiftArithmeticRight8(uint8_t,uint8_t);
uint16_t ShiftArithmeticRight16(uint16_t,uint8_t);
uint32_t ShiftArithmeticRight32(uint32_t,uint8_t);
uint32_t ShiftArithmeticRightW(uint32_t,uint8_t);
uint16_t ShiftRightDoublePrecision16(uint16_t,uint16_t,uint8_t);
uint32_t ShiftRightDoublePrecision32(uint32_t,uint32_t,uint8_t);
uint32_t ShiftRightDoublePrecisionW(uint32_t,uint32_t,uint8_t);
uint8_t ShiftLogicalLeft8(uint8_t,uint8_t);
uint16_t ShiftLogicalLeft16(uint16_t,uint8_t);
uint32_t ShiftLogicalLeft32(uint32_t,uint8_t);
uint32_t ShiftLogicalLeftW(uint32_t,uint8_t);
uint16_t ShiftLeftDoublePrecision16(uint16_t,uint16_t,uint8_t);
uint32_t ShiftLeftDoublePrecision32(uint32_t,uint32_t,uint8_t);
uint32_t ShiftLeftDoublePrecisionW(uint32_t,uint32_t,uint8_t);
uint8_t RotateRight8(uint8_t,uint8_t);
uint16_t RotateRight16(uint16_t,uint8_t);
uint32_t RotateRight32(uint32_t,uint8_t);
uint32_t RotateRightW(uint32_t,uint8_t);
uint8_t RotateLeft8(uint8_t,uint8_t);
uint16_t RotateLeft16(uint16_t,uint8_t);
uint32_t RotateLeft32(uint32_t,uint8_t);
uint32_t RotateLeftW(uint32_t,uint8_t);
uint8_t RotateCarryRight8(uint8_t,uint8_t);
uint16_t RotateCarryRight16(uint16_t,uint8_t);
uint32_t RotateCarryRight32(uint32_t,uint8_t);
uint32_t RotateCarryRightW(uint32_t,uint8_t);
uint8_t RotateCarryLeft8(uint8_t,uint8_t);
uint16_t RotateCarryLeft16(uint16_t,uint8_t);
uint32_t RotateCarryLeft32(uint32_t,uint8_t);
uint32_t RotateCarryLeftW(uint32_t,uint8_t);



void InstallOp(uint8_t num,opcode op,opcode *table=NULL);
void InitOpcodes();
void InstallExtGroupOp(uint8_t opcode, uint8_t r_op, groupOpcode func);

void op_ext_group();

void op_unknown();
void op_unknownG(ModRM &rm);
void op_mov_r8_imm8(); //Tested, pass #1;
void op_hlt(); //Tested, pass #1;
void op_nop(); //Tested, pass #1;
void op_nop_rmW();
void op_mov_rW_immW(); //Tested, pass #1;
void op_jmp_rel8(); //Tested, pass #1;
void op_sub_al_imm8(); //Tested, pass #1;
void op_sub_axW_immW(); //Tested, pass #1;
void op_jcc_rel8();
void op_jcc_relW();
void op_setcc_rm8();
void op_mov_sr_rm16(); //Tested, pass #1;
void op_mov_rm16_sr(); //Tested, pass #1;
void op_pop_rW(); //Tested, pass #1;
void op_push_immW(); //Tested, pass #1;
void op_push_rW(); //Tested, pass #1;
void op_push_es(); //Tested, pass #1;
void op_push_cs(); //Tested, pass #1;
void op_push_ss(); //Tested, pass #1;
void op_push_ds(); //Tested, pass #1;
void op_push_fs();
void op_push_gs();
void op_pop_es(); //Tested, pass #1;
void op_pop_ss(); //Tested, pass #1;
void op_pop_ds(); //Tested, pass #1;
void op_pop_fs();
void op_pop_gs();
void op_mov_rW_rmW(); //Tested, pass #1;
void op_mov_rmW_rW(); //Tested, pass #1;
void op_call_relW(); //Tested, pass #1;
void op_retn(); //Tested, pass #1;
void op_pre_cs_override(); //Tested, pass #1;
void op_pre_ds_override(); //Tested, pass #1;
void op_pre_es_override(); //Tested, pass #1;
void op_pre_ss_override(); //Tested, pass #1;
void op_movsW(); //Tested, pass #1;
void op_movsb(); //Tested, pass #1;
void op_clc();
void op_cld(); //Tested, pass #1;
void op_cli(); //Tested, pass #1;
void op_stc();
void op_std(); //Tested, pass #1;
void op_sti(); //Tested, pass #1;
void op16_rep(); //Tested, pass #1;(only rep, not conditionals)
void op_out_imm8_al(); //Tested, pass #1;
void op_out_imm8_axW(); //Tested, pass #1;
void op_call_immF(); //Tested, pass #1;
void op_retf(); //Tested, pass #1;
void op_int_imm8(); //Tested, pass #1;
void op_iret(); //Tested, pass #1;
void op_into();
void op_int3();
void op_in_al_imm8();
void op_in_axW_imm8();
void op_add_al_imm8(); //Tested, pass #1;
void op_add_axW_immW(); //Tested, pass #1;
void op_sub_rm8_r8(); //Tested, pass #1;
void op_group_80(); //Tested, pass #1;
void op_sub_r8_rm8();
void op_sub_rmW_rW();
void op_sub_rW_rmW();
void op_group_81(); //Tested, pass #1;
void op_add_rm8_r8();
void op_add_rmW_rW();
void op_add_r8_rm8();
void op_add_rW_rmW();
void op_mov_al_m8();
void op_mov_axW_mW();
void op_mov_rm8_r8();
void op_mov_r8_rm8();
void op_mov_m8_al(); //Tested, pass #1;
void op_mov_mW_axW(); //Tested, pass #1;
void op_mov_rm8_imm8(); //Tested, pass #1;
void op_mov_rmW_immW();  //Tested, pass #1;//currently have 85 instructions or prefixes implemented(actually more, not counting group instructions)
void op_cmp_rm8_r8();
void op_cmp_rmW_rW();
void op_cmp_r8_rm8();
void op_cmp_rW_rmW();
void op_cmp_al_imm8(); //Tested, pass #1;
void op_cmp_axW_immW(); //Tested, pass #1;
void op_group_83(); //Tested, pass #1;
void op_jmp_relW(); //Tested, pass #1
void op_jmp_immF(); //Tested, pass #1
void op_group_FF();
void op_push_imm8();
void op_group_8F();
void op_salc(); //Undocumented -- Set AL on Carry
void op_cmc();
void op_cbW();
void op_aaa();
void op_daa();
void op_das();
void op_aas();
void op_aad_imm8();
void op_aam_imm8();
void op_inc_rW();
void op_dec_rW();
void op_group_FE();
void op_group_F6();
void op_group_F7();
void op_cwE();
void op_and_rm8_r8();
void op_and_rmW_rW();
void op_and_r8_rm8();
void op_and_rW_rmW();
void op_and_al_imm8();
void op_and_axW_immW();
void op_or_rm8_r8();
void op_or_rmW_rW();
void op_or_r8_rm8();
void op_or_rW_rmW();
void op_or_al_imm8();
void op_or_axW_immW();
void op_escape();
void op_cmpsb(); //Tested, pass #1, full
void op_cmpsW(); //tested, pass #1, full
void op_jcxzW_rel8();
void op_adc_al_imm8();
void op_adc_axW_immW();
void op_adc_rm8_r8();
void op_adc_rmW_rW();
void op_adc_r8_rm8();
void op_adc_rW_rmW();
void op_lahf();
void op_sahf();
void op_loopcc_rel8();
void op_lds();
void op_les();
void op_lea();
void op_lock(); //funcitonally does nothing...
void op_xor_rm8_r8();
void op_xor_rmW_rW();
void op_xor_r8_rm8();
void op_xor_rW_rmW();
void op_xor_al_imm8();
void op_xor_axW_immW();
void op_sbb_rm8_r8();
void op_sbb_rmW_rW();
void op_sbb_r8_rm8();
void op_sbb_rW_rmW();
void op_sbb_al_imm8();
void op_sbb_axW_immW();
void op_test_al_imm8();
void op_test_axW_immW();
void op_test_rm8_r8();
void op_test_rmW_rW();
void op_xchg_rm8_r8();
void op_xchg_rmW_rW();
void op_xchg_axW_rW();
void op_group_D2();
void op_group_D3();
void op_group_D0();
void op_group_D1();
void op_group_C0();
void op_group_C1();
void op_lodsb();
void op_lodsW();
void op_scasb();
void op_scasW();
void op_stosb();
void op_stosW();
void op_wait();
void op_xlatb();
void op_enter();
void op_leave();
void op_movsx_rW_rm8();

void op_in_al_dx();
void op_in_axW_dx();
void op_out_dx_al();
void op_out_dx_axW();

void op_insb_m8_dx();
void op_insW_mW_dx();
void op_outsb_dx_m8();
void op_outsW_dx_mW();


void op_address_override();
void op_operand_override();


void op_rep();
void op32_rep();

void op_na();

void op_retf_imm16();
void op_retn_imm16();
void op_int1();
void op_pre_gs_override();
void op_pre_fs_override();
void op_bound_rW_mW();
void op_group_82();

void op_pushf();
void op_popf();

void op_bt_rmW_rW();
void op_bts_rmW_rW();
void op_shld_rmW_rW_imm8();
void op_shld_rmW_rW_cl();
void op_shrd_rmW_rW_imm8();
void op_shrd_rmW_rW_cl();


/**Group Include Functions(not direct opcodes)**/
void op_sub_rm8_imm8(ModRM&); //group 0x80 /5
void op_sub_rmW_immW(ModRM&);
void op_add_rm8_imm8(ModRM&);
void op_add_rmW_immW(ModRM&);
void op_cmp_rm8_imm8(ModRM&); //Tested, pass #1
void op_cmp_rmW_immW(ModRM&);
void op_sub_rmW_imm8(ModRM&);
void op_add_rmW_imm8(ModRM&);
void op_cmp_rmW_imm8(ModRM&); //Tested, pass #1
void op_jmp_rmW(ModRM&); //Tested, pass #1
void op_jmp_mF(ModRM&); //Tested, pass #1
void op_push_rmW(ModRM&);
void op_pop_rmW(ModRM&);
void op_inc_rm8(ModRM&);
void op_inc_rmW(ModRM&);
void op_dec_rm8(ModRM&);
void op_dec_rmW(ModRM&);
void op_div_rm8(ModRM &rm);//Tested, pass #1
void op16_div_rm16(ModRM &rm);//Tested, pass #1
void op_idiv_rm8(ModRM &rm);//Tested, pass #1
void op16_idiv_rm16(ModRM &rm);//Tested, pass #1
void op_mul_rm8(ModRM &rm);//Tested, pass #1
void op16_mul_rm16(ModRM &rm);//Tested, pass #1
void op_imul_rm8(ModRM &rm);
void op16_imul_rm16(ModRM &rm);
void op16_and_rm8_imm8(ModRM &rm);
void op_and_rmW_immW(ModRM &rm);
void op_and_rmW_imm8(ModRM &rm);
void op_and_rm8_imm8(ModRM &rm);
void op_or_rm8_imm8(ModRM& rm);
void op_or_rmW_immW(ModRM &rm);
void op_or_rmW_imm8(ModRM &rm);
void op_adc_rm8_imm8(ModRM&);
void op_adc_rmW_immW(ModRM&);
void op_adc_rmW_imm8(ModRM&);
void op_neg_rmW(ModRM&);
void op_neg_rm8(ModRM&);
void op_xor_rm8_imm8(ModRM& rm);
void op_xor_rmW_immW(ModRM &rm);
void op_xor_rmW_imm8(ModRM &rm);
void op_sbb_rm8_imm8(ModRM &rm);
void op_sbb_rmW_immW(ModRM &rm);
void op_sbb_rmW_imm8(ModRM &rm);
void op_test_rm8_imm8(ModRM &rm);
void op_test_rmW_immW(ModRM &rm);
void op_test_rmW_imm8(ModRM &rm); /**This needs to be added**/
void op_shr_rm8_cl(ModRM &rm);
void op_shr_rmW_cl(ModRM &rm);
void op_sar_rm8_cl(ModRM &rm);
void op_sar_rmW_cl(ModRM &rm);
void op_sar_rm8_imm8(ModRM &rm);
void op_sar_rmW_imm8(ModRM &rm);
void op_shl_rm8_cl(ModRM &rm);
void op_shl_rmW_cl(ModRM &rm);
void op_shl_rm8_imm8(ModRM &rm);
void op_shl_rmW_imm8(ModRM &rm);
void op_rol_rm8_cl(ModRM &rm);
void op_rol_rmW_cl(ModRM &rm);
void op_rol_rm8_imm8(ModRM &rm);
void op_rol_rmW_imm8(ModRM &rm);
void op_ror_rm8_cl(ModRM &rm);
void op_ror_rmW_cl(ModRM &rm);
void op_ror_rm8_imm8(ModRM &rm);
void op_ror_rmW_imm8(ModRM &rm);
void op_rcl_rm8_cl(ModRM &rm);
void op_rcl_rmW_cl(ModRM &rm);
void op_rcl_rm8_imm8(ModRM &rm);
void op_rcl_rmW_imm8(ModRM &rm);
void op_rcr_rm8_cl(ModRM &rm);
void op_rcr_rmW_cl(ModRM &rm);
void op_rcr_rm8_imm8(ModRM &rm);
void op_rcr_rmW_imm8(ModRM &rm);
void op_shr_rm8_1(ModRM &rm);
void op_shr_rmW_1(ModRM &rm);
void op_shr_rm8_imm8(ModRM &rm);
void op_shr_rmW_imm8(ModRM &rm);
void op_sar_rm8_1(ModRM &rm);
void op_sar_rmW_1(ModRM &rm);
void op_shl_rm8_1(ModRM &rm);
void op_shl_rmW_1(ModRM &rm);
void op_rol_rm8_1(ModRM &rm);
void op_rol_rmW_1(ModRM &rm);
void op_ror_rm8_1(ModRM &rm);
void op_ror_rmW_1(ModRM &rm);
void op_rcl_rm8_1(ModRM &rm);
void op_rcl_rmW_1(ModRM &rm);
void op_rcr_rm8_1(ModRM &rm);
void op_rcr_rmW_1(ModRM &rm);
void op_not_rm8(ModRM &rm);
void op_not_rmW(ModRM &rm);
void op_call_rmW(ModRM &rm);
void op_call_mF(ModRM &rm);

void op_pushaW();
void op_popaW();

void op32_mul_rm32(ModRM &rm);
void op32_imul_rm32(ModRM &rm);
void op32_div_rm32(ModRM &rm);
void op32_idiv_rm32(ModRM &rm);

void op_imul_rW_rmW_immW();
void op_imul_rW_rmW_imm8();


//2-byte opcodes
void op_movzx_rW_rm8();
void op_movzx_r32_rmW();

void op_ext_0F();

//helpers
void Push(uint32_t val);
uint32_t Pop();
void SetIndex8();
void SetIndex16();
void SetIndex32();
void SetIndex();
void CalculatePF(uint32_t val);
void CalculateSF8(uint8_t val);
void CalculateSF16(uint16_t val);
void CalculateSF32(uint32_t val);
void CalculateSF(uint32_t val);
void CalculateOF8(uint8_t result, uint8_t v1, uint8_t v2);
void CalculateOF16(uint16_t result, uint16_t v1, uint16_t v2);
void CalculateOF32(uint32_t result, uint32_t v1, uint32_t v2);
void CalculateOFW(uint32_t result, uint32_t v1, uint32_t v2);
void CalculateZF(uint32_t result);
void Jmp_nearW(uint32_t off);
void Jmp_near32(uint32_t off);
void Jmp_near16(uint16_t off);
void Jmp_near8(uint8_t off);
void Int16(uint8_t num);
void ResetSegments();
void SetSegments(uint8_t segm);
uint8_t ReadByte(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint8_t ReadByteA(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint16_t ReadWord(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint32_t ReadDword(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint32_t ReadW(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint32_t ReadWA(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
uint64_t ReadQword(uint8_t segm,uint32_t off, x86Lib::MemAccessReason reason = Data);
void WriteByte(uint8_t segm,uint32_t off,uint8_t val, x86Lib::MemAccessReason reason = Data);
void WriteByteA(uint8_t segm,uint32_t off,uint8_t val, x86Lib::MemAccessReason reason = Data);
void WriteWord(uint8_t segm,uint32_t off,uint16_t val, x86Lib::MemAccessReason reason = Data);
void WriteDword(uint8_t segm,uint32_t off,uint32_t val, x86Lib::MemAccessReason reason = Data);
void WriteW(uint8_t segm,uint32_t off,uint32_t val, x86Lib::MemAccessReason reason = Data);
void WriteWA(uint8_t segm,uint32_t off,uint32_t val, x86Lib::MemAccessReason reason = Data);



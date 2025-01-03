/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <memory/vaddr.h>
#include <utils.h>
#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, TYPE_R, TYPE_J,
  TYPE_B// none
};
void align(word_t* x) {
  *x = (*x + 3) & ~3;
}
void branch(Decode* s, word_t src1, word_t src2, sword_t imm) {
  s->dnpc = s->pc + (sword_t)imm * 2; 
  Log("imm is %x", imm);
}
// set the immidate through macro definition
// all the influence to the registers are done there
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
// my definitions for imm
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1)) << 10 | BITS(i, 7, 7) << 9 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8); } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1)) << 19 | BITS(i,19,12) << 11 | BITS(i,20,20) << 10 | BITS(i,30,21); } while(0)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  // the rs field are fixed to these five bits
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    // all the shits are from register, so do not do anything to imm
    case TYPE_R: src1R(); src2R();          break;
  /*B-type指令操作由7bit的opcode、3位的func3来决定；指令中包含两个源寄存器（rs1，rs2）与一个12位立即数，
  B-typed 一般表示条件跳转操作指令（分支指令），如相等(beq)、不相等(bne)、大于等于(bge)以及小于(blt)等跳
  转指令。*/
    case TYPE_B: src1R(); src2R(); immB();  break;
    /*J-type指令操作仅由7位opcode决定，与U-type一样只有一个目的寄存器rd和20位的立即数，但是立即数的位域
    与U-type的组成不同，J-type一般用于无条件跳转，如jal指令，RV32I一共有1条J-type指令。*/
    case TYPE_J: immJ();                    break;
    case TYPE_N: 
      Warn("Not impl instruction at %x", s->pc);
    break;
    default: 
    panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;
  Log("Decoding pc = %x", s->pc);
#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  INSTPAT_START();
  //INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // 这个指令类型会被macro concat成TYPE_指令类型
  // 指令执行操作会被macro 注入到实际执行的地方
  // 去看手册, 指令类型有U, I, S, B, J, R
  // decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); 
  // src1 是 源操作数1, src2 是源操作数2, rd 是目的操作数 imm 是立即数 
  // R(i) is register i
  //void vaddr_write(vaddr_t addr, int len, word_t data);
  // Mw: write to addr, len(bytes), data
  //   int rd = 0; 
  // word_t src1 = 0, src2 = 0, imm = 0; 
  
  /*  
  rd is destination register
  src1 is source register 1
  src2 is source register 2
  imm is immediate value*/
  // i may did it correctly???

  // given U type instructions, no need to have more of them in rv32i
  
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm << 12);

  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  // my instructions

  
  // my R instructions
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << src2);
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = ((word_t)src1) >> src2);
  INSTPAT("0100000 ????? ????? 001 ????? 01100 11", sra    , R, R(rd) = ((sword_t)src1) >> src2);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, if((sword_t)src1 < (sword_t)src2) R(rd) = 1; else R(rd) = 0);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, if((word_t)src1 < (word_t)src2) R(rd) = 1; else R(rd) = 0);
  
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  // my I instructions
  // bit-wise add does not differentiate between signed and unsigned
  // INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("?????? ?????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("?????? ?????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ (sword_t)imm);
  INSTPAT("?????? ?????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | (sword_t)imm);
  INSTPAT("?????? ?????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & (sword_t)imm);
  INSTPAT("000000 ?????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << imm);
  INSTPAT("000000 ?????? ????? 101 ????? 00100 11", srli   , I, R(rd) = ((word_t)src1) >> imm);
  INSTPAT("010000 ?????? ????? 101 ????? 00100 11", srai   , I, R(rd) = ((sword_t)src1) << imm);

  INSTPAT("?????? ?????? ????? 010 ????? 00100 11", slti   , I, if((sword_t)src1 < (sword_t)imm) R(rd) = 1; else R(rd) = 0);
  INSTPAT("?????? ?????? ????? 011 ????? 00100 11", sltiu  , I, if((word_t)src1 < (word_t)imm) R(rd) = 1; else R(rd) = 0);
  // load one byte
  INSTPAT("?????? ?????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(vaddr_ifetch(src1 + (sword_t)imm, 1), 8));
  INSTPAT("?????? ?????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(vaddr_ifetch(src1 + (sword_t)imm, 2), 16));
  // 1 word is equals to 4 bytes
  INSTPAT("?????? ?????? ????? 010 ????? 00000 11", lw     , I, R(rd) = vaddr_ifetch(src1 + (sword_t)imm, 4));
  INSTPAT("?????? ?????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = vaddr_ifetch(src1 + (sword_t)imm, 1));

  INSTPAT("?????? ?????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = vaddr_ifetch(src1 + (sword_t)imm, 2));
  INSTPAT("?????? ?????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->pc; s->dnpc = src1 + (sword_t)imm);
  INSTPAT("000000 000000 00000 000 00000 11100 11", ecall  , I, panic("ecall not implemented"));

  // my S series
  INSTPAT("?????? ?????? ????? 000 ????? 01000 11", sb     , S, vaddr_write(src1 + (sword_t)imm, 1, (word_t)src2));
  INSTPAT("?????? ?????? ????? 001 ????? 01000 11", sh     , S, vaddr_write(src1 + (sword_t)imm, 2, (word_t)src2));
  INSTPAT("?????? ?????? ????? 010 ????? 01000 11", sw     , S, vaddr_write(src1 + (sword_t)imm, 4, (word_t)src2));
  // my B series
  INSTPAT("?????? ?????? ????? 000 ????? 11000 11", beq    , B, if (src1 == src2) branch(s, src1, src2, ((sword_t)imm)););
  INSTPAT("?????? ?????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) branch(s, src1, src2, ((sword_t)imm)););
  // attention: the 0 bit of imm is not specified cause we neet to << 1 to imm
  // thus imm[0] === 0
  INSTPAT("?????? ?????? ????? 100 ????? 11000 11", blt    , B, if (((sword_t)src1) < ((sword_t)src2)) branch(s, src1, src2, ((sword_t)imm)););
  INSTPAT("?????? ?????? ????? 101 ????? 11000 11", bge    , B, if (((sword_t)src1) >= ((sword_t)src2)) branch(s, src1, src2, ((sword_t)imm)););
  INSTPAT("?????? ?????? ????? 110 ????? 11000 11", bltu   , B, if (((word_t)src1) < ((word_t)src2)) branch(s, src1, src2, ((sword_t)imm)););
  INSTPAT("?????? ?????? ????? 111 ????? 11000 11", bgeu   , B, if (((word_t)src1) >= ((word_t)src2)) branch(s, src1, src2, ((sword_t)imm)););
  // my J series
  INSTPAT("?????? ?????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc; branch(s, src1, src2, ((sword_t)imm)););
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0
  // align(&s->dnpc);
  Log("snpc: %x, dnpc:%x", s->snpc, s->dnpc);
  return 0;
}

int isa_exec_once(Decode *s) {
  // in inst fetch, pc is incremented by 4(rv32)
  s->isa.inst = inst_fetch(&s->snpc, 4);
  // decode exec receives the pc of the next instruction
  // so do not increment pc in decode_exec
  return decode_exec(s);
}

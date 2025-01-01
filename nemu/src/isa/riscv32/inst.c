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

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, TYPE_R, TYPE_J,
  TYPE_B// none
};


// set the immidate through macro definition
// all the influence to the registers are done there
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
// my definitions for imm
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1)) << 10 | BITS(i, 7, 7) << 9 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8); } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1)) << 18 | BITS(i,19,12) << 9 | BITS(i,20,20) << 8 | BITS(i,30,21); } while(0)
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

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  // my instructions
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm << 12);
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << src2);
  // INSTPAT("", jal,  );
  // INSTPAT("", li,  );
  // INSTPAT("", ret,  );
  // INSTPAT("", sw,  );
// 80000000 <_start>:
// 80000000:	00000413          	li	s0,0
// 80000004:	00009117          	auipc	sp,0x9
// 80000008:	ffc10113          	addi	sp,sp,-4 # 80009000 <_end>
// 8000000c:	00c000ef          	jal	80000018 <_trm_init>

// 80000010 <main>:
// 80000010:	00000513          	li	a0,0
// 80000014:	00008067          	ret
//   80000018 <_trm_init>:
// 80000018:	ff010113          	addi	sp,sp,-16
// 8000001c:	00000517          	auipc	a0,0x0
// 80000020:	01c50513          	addi	a0,a0,28 # 80000038 <_etext>
// 80000024:	00112623          	sw	ra,12(sp)
// 80000028:	fe9ff0ef          	jal	80000010 <main>
// 8000002c:	00050513          	mv	a0,a0
// 80000030:	00100073          	ebreak
// 80000034:	0000006f          	j	80000034 <_trm_init+0x1c>
  // INSTPAT("");



  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}

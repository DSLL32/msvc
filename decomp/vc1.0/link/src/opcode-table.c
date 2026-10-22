/* One-byte i386 opcode descriptors encountered by decode_opcode.
 * Original table [00081e60,00083260): 256 typed 20-byte entries.
 * Handler addresses are named graph vertices; pending handlers trap. */
#include "disassembler.h"
#include <recon.h>

enum DisassemblyMnemonic {
    MN_NONE=0,
    MN_AAA=1,
    MN_AAD=2,
    MN_AAM=3,
    MN_AAS=4,
    MN_ADC=5,
    MN_ADD=6,
    MN_AND=7,
    MN_ARPL=8,
    MN_BOUND=9,
    MN_CALL=17,
    MN_CBW=18,
    MN_CLC=20,
    MN_CLD=21,
    MN_CLI=22,
    MN_CMC=24,
    MN_CMP=25,
    MN_CMPS=26,
    MN_CWD=28,
    MN_DAA=30,
    MN_DAS=31,
    MN_DEC=32,
    MN_ENTER=34,
    MN_HLT=120,
    MN_IMUL=123,
    MN_IN=124,
    MN_INC=125,
    MN_INS=126,
    MN_INT=127,
    MN_INTO=128,
    MN_IRET=132,
    MN_JB=134,
    MN_JAE=135,
    MN_JBE=136,
    MN_JA=137,
    MN_JE=138,
    MN_JNE=139,
    MN_JL=140,
    MN_JGE=141,
    MN_JLE=142,
    MN_JG=143,
    MN_JS=144,
    MN_JNS=145,
    MN_JO=148,
    MN_JNO=149,
    MN_JP=150,
    MN_JNP=151,
    MN_JCXZ=152,
    MN_JMP=154,
    MN_LAHF=155,
    MN_LDS=157,
    MN_LES=158,
    MN_LEA=163,
    MN_LEAVE=164,
    MN_LOCK=169,
    MN_LODS=170,
    MN_LOOP=171,
    MN_LOOPE=174,
    MN_LOOPNE=177,
    MN_MOV=182,
    MN_MOVS=183,
    MN_NOP=188,
    MN_OR=190,
    MN_OUT=191,
    MN_OUTS=192,
    MN_POP=193,
    MN_POPA=194,
    MN_POPF=197,
    MN_PUSH=199,
    MN_PUSHA=202,
    MN_PUSHF=205,
    MN_REP=211,
    MN_REPNZ=213,
    MN_RET=214,
    MN_RETF=216,
    MN_SAHF=217,
    MN_SBB=218,
    MN_SCAS=219,
    MN_STC=248,
    MN_STI=250,
    MN_STOS=251,
    MN_SUB=253,
    MN_TEST=254,
    MN_WAIT=257,
    MN_XCHG=261,
    MN_XLAT=262,
    MN_XOR=263
};

const OpcodeEntry opcode_table[256]={
    {MN_ADD,0x11f,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 00 */
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 01 */
    {MN_ADD,0x11f,0x0,0x0,decode_op_4d6f0,decode_op_4c750,1}, /* 02 */
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 03 */
    {MN_ADD,0x11f,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* 04 */
    {MN_ADD,0x11f,0x0,0x0,decode_op_4ddb0,decode_op_4c0e0,1}, /* 05 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4da30,1}, /* 06 */
    {MN_POP,0x0,0x0,0x0,decode_op_4da30,update_stack_registers,1}, /* 07 */
    {MN_OR,0x11b,0x0,0x4,decode_op_4c750,decode_op_4d6f0,1}, /* 08 */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 09 */
    {MN_OR,0x11b,0x0,0x4,decode_op_4d6f0,decode_op_4c750,1}, /* 0A */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 0B */
    {MN_OR,0x11b,0x0,0x4,decode_op_4dbb0,decode_op_4c080,1}, /* 0C */
    {MN_OR,0x11b,0x0,0x4,decode_op_4ddb0,decode_op_4c0e0,1}, /* 0D */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4da70,1}, /* 0E */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b5f0,NULL,1}, /* 0F */
    {MN_ADC,0x11f,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 10 */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 11 */
    {MN_ADC,0x11f,0x0,0x0,decode_op_4d6f0,decode_op_4c750,1}, /* 12 */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 13 */
    {MN_ADC,0x11f,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* 14 */
    {MN_ADC,0x11f,0x0,0x0,decode_op_4ddb0,decode_op_4c0e0,1}, /* 15 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4dab0,1}, /* 16 */
    {MN_POP,0x0,0x0,0x0,decode_op_4dab0,update_stack_registers,1}, /* 17 */
    {MN_SBB,0x11f,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 18 */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 19 */
    {MN_SBB,0x11f,0x0,0x0,decode_op_4d6f0,decode_op_4c750,1}, /* 1A */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 1B */
    {MN_SBB,0x11f,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* 1C */
    {MN_SBB,0x11f,0x0,0x0,decode_op_4ddb0,decode_op_4c0e0,1}, /* 1D */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4daf0,1}, /* 1E */
    {MN_POP,0x0,0x0,0x0,decode_op_4daf0,update_stack_registers,1}, /* 1F */
    {MN_AND,0x11b,0x0,0x4,decode_op_4c750,decode_op_4d6f0,1}, /* 20 */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 21 */
    {MN_AND,0x11b,0x0,0x4,decode_op_4d6f0,decode_op_4c750,1}, /* 22 */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 23 */
    {MN_AND,0x11b,0x0,0x4,decode_op_4dbb0,decode_op_4c080,1}, /* 24 */
    {MN_AND,0x11b,0x0,0x4,decode_op_4ddb0,decode_op_4c0e0,1}, /* 25 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd10,1}, /* 26 */
    {MN_DAA,0x1f,0x5,0x100,decode_op_4ba50,decode_op_4ba50,1}, /* 27 */
    {MN_SUB,0x11f,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 28 */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 29 */
    {MN_SUB,0x11f,0x0,0x0,decode_op_4d6f0,decode_op_4c750,1}, /* 2A */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 2B */
    {MN_SUB,0x11f,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* 2C */
    {MN_SUB,0x11f,0x0,0x0,decode_op_4ddb0,decode_op_4c0e0,1}, /* 2D */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd40,1}, /* 2E */
    {MN_DAS,0x1f,0x5,0x100,decode_op_4ba50,decode_op_4ba50,1}, /* 2F */
    {MN_XOR,0x11b,0x0,0x4,decode_op_4c750,decode_op_4d6f0,1}, /* 30 */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 31 */
    {MN_XOR,0x11b,0x0,0x4,decode_op_4d6f0,decode_op_4c750,1}, /* 32 */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 33 */
    {MN_XOR,0x11b,0x0,0x4,decode_op_4dbb0,decode_op_4c080,1}, /* 34 */
    {MN_XOR,0x11b,0x0,0x4,decode_op_4ddb0,decode_op_4c0e0,1}, /* 35 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd70,1}, /* 36 */
    {MN_AAA,0x5,0x4,0x11b,decode_op_4ba90,NULL,1}, /* 37 */
    {MN_CMP,0x11f,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 38 */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 39 */
    {MN_CMP,0x11f,0x0,0x0,decode_op_4d6f0,decode_op_4c750,1}, /* 3A */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 3B */
    {MN_CMP,0x11f,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* 3C */
    {MN_CMP,0x11f,0x0,0x0,decode_op_4ddb0,decode_op_4c0e0,1}, /* 3D */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bda0,1}, /* 3E */
    {MN_AAS,0x5,0x4,0x11b,decode_op_4ba90,NULL,1}, /* 3F */
    {MN_INC,0x11e,0x0,0x0,decode_op_4ddb0,decode_op_4bb10,1}, /* 40 */
    {MN_INC,0x11e,0x0,0x0,decode_op_4ded0,decode_op_4bb90,1}, /* 41 */
    {MN_INC,0x11e,0x0,0x0,decode_op_4df50,decode_op_4bbd0,1}, /* 42 */
    {MN_INC,0x11e,0x0,0x0,decode_ebx_register,decode_op_4bb50,1}, /* 43 */
    {MN_INC,0x11e,0x0,0x0,decode_op_4dfa0,decode_op_4bc10,1}, /* 44 */
    {MN_INC,0x11e,0x0,0x0,decode_ebp_register,decode_op_4bc50,1}, /* 45 */
    {MN_INC,0x11e,0x0,0x0,decode_esi_register,decode_op_4bc90,1}, /* 46 */
    {MN_INC,0x11e,0x0,0x0,decode_edi_register,decode_op_4bcd0,1}, /* 47 */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4ddb0,decode_op_4bb10,1}, /* 48 */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4ded0,decode_op_4bb90,1}, /* 49 */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4df50,decode_op_4bbd0,1}, /* 4A */
    {MN_DEC,0x11e,0x0,0x0,decode_ebx_register,decode_op_4bb50,1}, /* 4B */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4dfa0,decode_op_4bc10,1}, /* 4C */
    {MN_DEC,0x11e,0x0,0x0,decode_ebp_register,decode_op_4bc50,1}, /* 4D */
    {MN_DEC,0x11e,0x0,0x0,decode_esi_register,decode_op_4bc90,1}, /* 4E */
    {MN_DEC,0x11e,0x0,0x0,decode_edi_register,decode_op_4bcd0,1}, /* 4F */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4ddb0,1}, /* 50 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4ded0,1}, /* 51 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4df50,1}, /* 52 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_ebx_register,1}, /* 53 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4dfa0,1}, /* 54 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_ebp_register,1}, /* 55 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_esi_register,1}, /* 56 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_edi_register,1}, /* 57 */
    {MN_POP,0x0,0x0,0x0,decode_op_4ddb0,update_stack_registers,1}, /* 58 */
    {MN_POP,0x0,0x0,0x0,decode_op_4ded0,update_stack_registers,1}, /* 59 */
    {MN_POP,0x0,0x0,0x0,decode_op_4df50,update_stack_registers,1}, /* 5A */
    {MN_POP,0x0,0x0,0x0,decode_ebx_register,update_stack_registers,1}, /* 5B */
    {MN_POP,0x0,0x0,0x0,decode_op_4dfa0,update_stack_registers,1}, /* 5C */
    {MN_POP,0x0,0x0,0x0,decode_ebp_register,update_stack_registers,1}, /* 5D */
    {MN_POP,0x0,0x0,0x0,decode_esi_register,update_stack_registers,1}, /* 5E */
    {MN_POP,0x0,0x0,0x0,decode_edi_register,update_stack_registers,1}, /* 5F */
    {MN_PUSHA,0x0,0x0,0x0,update_stack_registers,decode_op_4bae0,1}, /* 60 */
    {MN_POPA,0x0,0x0,0x0,decode_op_4bae0,update_stack_registers,1}, /* 61 */
    {MN_BOUND,0x0,0x0,0x0,decode_modrm_register,decode_op_4c970,7}, /* 62 */
    {MN_ARPL,0x8,0x0,0x0,decode_op_4c830,decode_op_4c4c0,1}, /* 63 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bdd0,1}, /* 64 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4be00,1}, /* 65 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b880,NULL,1}, /* 66 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b8a0,NULL,1}, /* 67 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4c0e0,1}, /* 68 */
    {MN_IMUL,0x101,0x0,0x1e,decode_modrm_register,decode_op_4c910,1}, /* 69 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4c080,1}, /* 6A */
    {MN_IMUL,0x101,0x0,0x1e,decode_modrm_register,decode_op_4c940,1}, /* 6B */
    {MN_INS,0x0,0x80,0x0,decode_op_4be30,decode_op_4df20,1}, /* 6C */
    {MN_INS,0x0,0x80,0x0,decode_op_4be90,decode_op_4df20,1}, /* 6D */
    {MN_OUTS,0x0,0x80,0x0,decode_op_4df20,decode_op_4bf00,1}, /* 6E */
    {MN_OUTS,0x0,0x80,0x0,decode_op_4df20,decode_op_4bfe0,1}, /* 6F */
    {MN_JO,0x0,0x100,0x0,NULL,decode_op_4c250,14}, /* 70 */
    {MN_JNO,0x0,0x100,0x0,NULL,decode_op_4c250,14}, /* 71 */
    {MN_JB,0x0,0x1,0x0,NULL,decode_op_4c250,14}, /* 72 */
    {MN_JAE,0x0,0x1,0x0,NULL,decode_op_4c250,14}, /* 73 */
    {MN_JE,0x0,0x8,0x0,NULL,decode_op_4c250,14}, /* 74 */
    {MN_JNE,0x0,0x8,0x0,NULL,decode_op_4c250,14}, /* 75 */
    {MN_JBE,0x0,0x9,0x0,NULL,decode_op_4c250,14}, /* 76 */
    {MN_JA,0x0,0x9,0x0,NULL,decode_op_4c250,14}, /* 77 */
    {MN_JS,0x0,0x10,0x0,NULL,decode_op_4c250,14}, /* 78 */
    {MN_JNS,0x0,0x10,0x0,NULL,decode_op_4c250,14}, /* 79 */
    {MN_JP,0x0,0x2,0x0,NULL,decode_op_4c250,14}, /* 7A */
    {MN_JNP,0x0,0x2,0x0,NULL,decode_op_4c250,14}, /* 7B */
    {MN_JL,0x0,0x110,0x0,NULL,decode_op_4c250,14}, /* 7C */
    {MN_JGE,0x0,0x110,0x0,NULL,decode_op_4c250,14}, /* 7D */
    {MN_JLE,0x0,0x18,0x0,NULL,decode_op_4c250,14}, /* 7E */
    {MN_JG,0x0,0x18,0x0,NULL,decode_op_4c250,14}, /* 7F */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b670,decode_op_4c080,1}, /* 80 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6a0,decode_op_4c0e0,1}, /* 81 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_op_4c080,1}, /* 82 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6a0,decode_op_4c080,1}, /* 83 */
    {MN_TEST,0x11b,0x0,0x4,decode_op_4c750,decode_op_4d6f0,1}, /* 84 */
    {MN_TEST,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 85 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4c750,decode_op_4d6f0,1}, /* 86 */
    {MN_XCHG,0x0,0x0,0x0,decode_modrm_operand_effects,decode_op_4d690,1}, /* 87 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c750,decode_op_4d640,1}, /* 88 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_operand_effects,decode_modrm_register,1}, /* 89 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d6f0,decode_op_4c750,1}, /* 8A */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_register,decode_modrm_operand_effects,1}, /* 8B */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c830,decode_op_4d5e0,1}, /* 8C */
    {MN_LEA,0x0,0x0,0x0,decode_modrm_register,decode_op_4ca10,1}, /* 8D */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d5e0,decode_op_4c830,1}, /* 8E */
    {MN_POP,0x0,0x0,0x0,decode_modrm_operand_effects,update_stack_registers,1}, /* 8F */
    {MN_NOP,0x0,0x0,0x0,NULL,NULL,1}, /* 90 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4ded0,decode_op_4de40,1}, /* 91 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4df50,decode_op_4de40,1}, /* 92 */
    {MN_XCHG,0x0,0x0,0x0,decode_ebx_register,decode_op_4de40,1}, /* 93 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4dfa0,decode_op_4de40,1}, /* 94 */
    {MN_XCHG,0x0,0x0,0x0,decode_ebp_register,decode_op_4de40,1}, /* 95 */
    {MN_XCHG,0x0,0x0,0x0,decode_esi_register,decode_op_4de40,1}, /* 96 */
    {MN_XCHG,0x0,0x0,0x0,decode_edi_register,decode_op_4de40,1}, /* 97 */
    {MN_CBW,0x0,0x0,0x0,decode_op_4bb10,decode_op_4ba10,1}, /* 98 */
    {MN_CWD,0x0,0x0,0x0,decode_op_4bbd0,decode_op_4b960,1}, /* 99 */
    {MN_CALL,0x0,0x0,0x0,decode_op_4b9c0,decode_op_4c1a0,3}, /* 9A */
    {MN_WAIT,0x0,0x0,0x0,NULL,NULL,1}, /* 9B */
    {MN_PUSHF,0x0,0x1ff,0x0,update_stack_registers,NULL,1}, /* 9C */
    {MN_POPF,0x1ff,0x0,0x0,update_stack_registers,NULL,1}, /* 9D */
    {MN_SAHF,0x1f,0x0,0x0,NULL,decode_op_4ba70,1}, /* 9E */
    {MN_LAHF,0x0,0x0,0x0,decode_op_4ba70,NULL,1}, /* 9F */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_op_4c3d0,1}, /* A0 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4ddb0,decode_op_4c440,1}, /* A1 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c3d0,decode_op_4dbb0,1}, /* A2 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c440,decode_op_4ddb0,1}, /* A3 */
    {MN_MOVS,0x0,0x80,0x0,decode_op_4be30,decode_op_4bf00,1}, /* A4 */
    {MN_MOVS,0x0,0x80,0x0,decode_op_4be90,decode_op_4bfe0,1}, /* A5 */
    {MN_CMPS,0x11f,0x80,0x0,decode_op_4bf00,decode_op_4be30,1}, /* A6 */
    {MN_CMPS,0x11f,0x80,0x0,decode_op_4bfe0,decode_op_4be90,1}, /* A7 */
    {MN_TEST,0x11b,0x0,0x4,decode_op_4dbb0,decode_op_4c080,1}, /* A8 */
    {MN_TEST,0x11b,0x0,0x4,decode_op_4ddb0,decode_op_4c0e0,1}, /* A9 */
    {MN_STOS,0x0,0x80,0x0,decode_op_4be30,decode_op_4ba50,1}, /* AA */
    {MN_STOS,0x0,0x80,0x0,decode_op_4be90,decode_op_4bb10,1}, /* AB */
    {MN_LODS,0x0,0x80,0x0,decode_op_4ba50,decode_op_4bf00,1}, /* AC */
    {MN_LODS,0x0,0x80,0x0,decode_op_4bb10,decode_op_4bfe0,1}, /* AD */
    {MN_SCAS,0x11f,0x80,0x0,decode_op_4be30,decode_op_4ba50,1}, /* AE */
    {MN_SCAS,0x11f,0x80,0x0,decode_op_4be90,decode_op_4bb10,1}, /* AF */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_op_4c080,1}, /* B0 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dc30,decode_op_4c080,1}, /* B1 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dc70,decode_op_4c080,1}, /* B2 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbf0,decode_op_4c080,1}, /* B3 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dcb0,decode_op_4c080,1}, /* B4 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dd30,decode_op_4c080,1}, /* B5 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dd70,decode_op_4c080,1}, /* B6 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dcf0,decode_op_4c080,1}, /* B7 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4ddb0,decode_op_4c0e0,1}, /* B8 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4ded0,decode_op_4c0e0,1}, /* B9 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4df50,decode_op_4c0e0,1}, /* BA */
    {MN_MOV,0x0,0x0,0x11f,decode_ebx_register,decode_op_4c0e0,1}, /* BB */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dfa0,decode_op_4c0e0,1}, /* BC */
    {MN_MOV,0x0,0x0,0x11f,decode_ebp_register,decode_op_4c0e0,1}, /* BD */
    {MN_MOV,0x0,0x0,0x11f,decode_esi_register,decode_op_4c0e0,1}, /* BE */
    {MN_MOV,0x0,0x0,0x11f,decode_edi_register,decode_op_4c0e0,1}, /* BF */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_op_4c080,1}, /* C0 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b700,decode_op_4c080,1}, /* C1 */
    {MN_RET,0x0,0x0,0x0,update_stack_registers,decode_op_4c0b0,11}, /* C2 */
    {MN_RET,0x0,0x0,0x0,update_stack_registers,NULL,11}, /* C3 */
    {MN_LES,0x0,0x0,0x0,decode_op_4d8a0,decode_op_4ca60,1}, /* C4 */
    {MN_LDS,0x0,0x0,0x0,decode_op_4d900,decode_op_4ca60,1}, /* C5 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c750,decode_op_4c080,1}, /* C6 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_operand_effects,decode_op_4c0e0,1}, /* C7 */
    {MN_ENTER,0x0,0x0,0x0,decode_op_4d990,decode_op_4c080,1}, /* C8 */
    {MN_LEAVE,0x0,0x0,0x0,update_stack_registers,update_frame_registers,1}, /* C9 */
    {MN_RETF,0x0,0x0,0x0,decode_op_4b9c0,decode_op_4c0b0,12}, /* CA */
    {MN_RETF,0x0,0x0,0x0,decode_op_4b9c0,NULL,12}, /* CB */
    {MN_INT,0x30,0x0,0x0,decode_op_4b9c0,decode_op_4c170,9}, /* CC */
    {MN_INT,0x30,0x0,0x0,decode_op_4b9c0,decode_op_4c080,8}, /* CD */
    {MN_INTO,0x30,0x0,0x0,decode_op_4b9c0,NULL,7}, /* CE */
    {MN_IRET,0x0,0x0,0x0,decode_op_4b9c0,NULL,10}, /* CF */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_op_4c140,1}, /* D0 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b700,decode_op_4c140,1}, /* D1 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_op_4dc30,1}, /* D2 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b700,decode_op_4dc30,1}, /* D3 */
    {MN_AAM,0x1a,0x101,0x105,decode_op_4ba90,decode_op_4c060,1}, /* D4 */
    {MN_AAD,0x1a,0x0,0x105,decode_op_4ba90,decode_op_4c060,1}, /* D5 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,NULL,1}, /* D6 */
    {MN_XLAT,0x0,0x0,0x0,decode_op_4b9f0,decode_op_4bf70,1}, /* D7 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4cac0,NULL,1}, /* D8 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4cc90,NULL,1}, /* D9 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4cdf0,NULL,1}, /* DA */
    {MN_NONE,0x0,0x0,0x0,decode_op_4ce80,NULL,1}, /* DB */
    {MN_NONE,0x0,0x0,0x0,decode_op_4cf10,NULL,1}, /* DC */
    {MN_NONE,0x0,0x0,0x0,decode_op_4d0e0,NULL,1}, /* DD */
    {MN_NONE,0x0,0x0,0x0,decode_op_4d1e0,NULL,1}, /* DE */
    {MN_NONE,0x0,0x0,0x0,decode_op_4d330,NULL,1}, /* DF */
    {MN_LOOPNE,0x0,0x8,0x0,decode_op_4b990,decode_op_4c250,20}, /* E0 */
    {MN_LOOPE,0x0,0x8,0x0,decode_op_4b990,decode_op_4c250,20}, /* E1 */
    {MN_LOOP,0x0,0x0,0x0,decode_op_4b990,decode_op_4c250,20}, /* E2 */
    {MN_JCXZ,0x0,0x0,0x0,decode_op_4b990,decode_op_4c250,21}, /* E3 */
    {MN_IN,0x0,0x0,0x0,decode_op_4dbb0,decode_op_4c080,1}, /* E4 */
    {MN_IN,0x0,0x0,0x0,decode_op_4ddb0,decode_op_4c080,1}, /* E5 */
    {MN_OUT,0x0,0x0,0x0,decode_op_4c080,decode_op_4dbb0,1}, /* E6 */
    {MN_OUT,0x0,0x0,0x0,decode_op_4c080,decode_op_4ddb0,1}, /* E7 */
    {MN_CALL,0x0,0x0,0x0,update_stack_registers,decode_near_relative,2}, /* E8 */
    {MN_JMP,0x0,0x0,0x0,NULL,decode_near_relative,15}, /* E9 */
    {MN_JMP,0x0,0x0,0x0,decode_op_4bac0,decode_op_4c1a0,18}, /* EA */
    {MN_JMP,0x0,0x0,0x0,NULL,decode_op_4c250,13}, /* EB */
    {MN_IN,0x0,0x0,0x0,decode_op_4dbb0,decode_op_4df20,1}, /* EC */
    {MN_IN,0x0,0x0,0x0,decode_op_4ddb0,decode_op_4df20,1}, /* ED */
    {MN_OUT,0x0,0x0,0x0,decode_op_4df20,decode_op_4dbb0,1}, /* EE */
    {MN_OUT,0x0,0x0,0x0,decode_op_4df20,decode_op_4de00,1}, /* EF */
    {MN_LOCK,0x0,0x0,0x0,NULL,NULL,1}, /* F0 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,NULL,1}, /* F1 */
    {MN_REPNZ,0x0,0x8,0x0,decode_op_4b8e0,NULL,1}, /* F2 */
    {MN_REP,0x0,0x8,0x0,decode_op_4b8c0,NULL,1}, /* F3 */
    {MN_HLT,0x0,0x0,0x0,NULL,NULL,4}, /* F4 */
    {MN_CMC,0x1,0x1,0x0,NULL,NULL,1}, /* F5 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b730,NULL,1}, /* F6 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b760,NULL,1}, /* F7 */
    {MN_CLC,0x1,0x0,0x0,NULL,NULL,1}, /* F8 */
    {MN_STC,0x1,0x0,0x0,NULL,NULL,1}, /* F9 */
    {MN_CLI,0x30,0x0,0x0,NULL,NULL,1}, /* FA */
    {MN_STI,0x30,0x0,0x0,NULL,NULL,1}, /* FB */
    {MN_CLD,0x80,0x0,0x0,NULL,NULL,1}, /* FC */
    {MN_OR,0x11b,0x0,0x4,NULL,NULL,1}, /* FD */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b790,NULL,1}, /* FE */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b7c0,NULL,1}, /* FF */
};

/* Encoding class to instruction category, shared by opcode descriptors. */
const unsigned char instruction_categories[22]={
    0,1,6,6,7,7,7,9,7,7,3,3,3,2,4,2,4,3,2,3,4,4
};

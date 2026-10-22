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
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 00 */
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 01 */
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 02 */
    {MN_ADD,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 03 */
    {MN_ADD,0x11f,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* 04 */
    {MN_ADD,0x11f,0x0,0x0,decode_eax_register,decode_immediate,1}, /* 05 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4da30,1}, /* 06 */
    {MN_POP,0x0,0x0,0x0,decode_op_4da30,update_stack_registers,1}, /* 07 */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 08 */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 09 */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 0A */
    {MN_OR,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 0B */
    {MN_OR,0x11b,0x0,0x4,decode_op_4dbb0,decode_byte_immediate,1}, /* 0C */
    {MN_OR,0x11b,0x0,0x4,decode_eax_register,decode_immediate,1}, /* 0D */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4da70,1}, /* 0E */
    {MN_NONE,0x0,0x0,0x0,decode_extended_opcode,NULL,1}, /* 0F */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 10 */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 11 */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 12 */
    {MN_ADC,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 13 */
    {MN_ADC,0x11f,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* 14 */
    {MN_ADC,0x11f,0x0,0x0,decode_eax_register,decode_immediate,1}, /* 15 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4dab0,1}, /* 16 */
    {MN_POP,0x0,0x0,0x0,decode_op_4dab0,update_stack_registers,1}, /* 17 */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 18 */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 19 */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 1A */
    {MN_SBB,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 1B */
    {MN_SBB,0x11f,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* 1C */
    {MN_SBB,0x11f,0x0,0x0,decode_eax_register,decode_immediate,1}, /* 1D */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4daf0,1}, /* 1E */
    {MN_POP,0x0,0x0,0x0,decode_op_4daf0,update_stack_registers,1}, /* 1F */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 20 */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 21 */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 22 */
    {MN_AND,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 23 */
    {MN_AND,0x11b,0x0,0x4,decode_op_4dbb0,decode_byte_immediate,1}, /* 24 */
    {MN_AND,0x11b,0x0,0x4,decode_eax_register,decode_immediate,1}, /* 25 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd10,1}, /* 26 */
    {MN_DAA,0x1f,0x5,0x100,update_al_register,update_al_register,1}, /* 27 */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 28 */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 29 */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 2A */
    {MN_SUB,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 2B */
    {MN_SUB,0x11f,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* 2C */
    {MN_SUB,0x11f,0x0,0x0,decode_eax_register,decode_immediate,1}, /* 2D */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd40,1}, /* 2E */
    {MN_DAS,0x1f,0x5,0x100,update_al_register,update_al_register,1}, /* 2F */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 30 */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 31 */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 32 */
    {MN_XOR,0x11b,0x0,0x4,decode_modrm_register,decode_modrm_operand_effects,1}, /* 33 */
    {MN_XOR,0x11b,0x0,0x4,decode_op_4dbb0,decode_byte_immediate,1}, /* 34 */
    {MN_XOR,0x11b,0x0,0x4,decode_eax_register,decode_immediate,1}, /* 35 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4bd70,1}, /* 36 */
    {MN_AAA,0x5,0x4,0x11b,decode_op_4ba90,NULL,1}, /* 37 */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 38 */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* 39 */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 3A */
    {MN_CMP,0x11f,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* 3B */
    {MN_CMP,0x11f,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* 3C */
    {MN_CMP,0x11f,0x0,0x0,decode_eax_register,decode_immediate,1}, /* 3D */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_ds_prefix,1}, /* 3E */
    {MN_AAS,0x5,0x4,0x11b,decode_op_4ba90,NULL,1}, /* 3F */
    {MN_INC,0x11e,0x0,0x0,decode_eax_register,decode_op_4bb10,1}, /* 40 */
    {MN_INC,0x11e,0x0,0x0,decode_ecx_register,decode_op_4bb90,1}, /* 41 */
    {MN_INC,0x11e,0x0,0x0,decode_op_4df50,decode_op_4bbd0,1}, /* 42 */
    {MN_INC,0x11e,0x0,0x0,decode_ebx_register,decode_op_4bb50,1}, /* 43 */
    {MN_INC,0x11e,0x0,0x0,decode_op_4dfa0,decode_op_4bc10,1}, /* 44 */
    {MN_INC,0x11e,0x0,0x0,decode_ebp_register,decode_op_4bc50,1}, /* 45 */
    {MN_INC,0x11e,0x0,0x0,decode_esi_register,decode_op_4bc90,1}, /* 46 */
    {MN_INC,0x11e,0x0,0x0,decode_edi_register,decode_op_4bcd0,1}, /* 47 */
    {MN_DEC,0x11e,0x0,0x0,decode_eax_register,decode_op_4bb10,1}, /* 48 */
    {MN_DEC,0x11e,0x0,0x0,decode_ecx_register,decode_op_4bb90,1}, /* 49 */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4df50,decode_op_4bbd0,1}, /* 4A */
    {MN_DEC,0x11e,0x0,0x0,decode_ebx_register,decode_op_4bb50,1}, /* 4B */
    {MN_DEC,0x11e,0x0,0x0,decode_op_4dfa0,decode_op_4bc10,1}, /* 4C */
    {MN_DEC,0x11e,0x0,0x0,decode_ebp_register,decode_op_4bc50,1}, /* 4D */
    {MN_DEC,0x11e,0x0,0x0,decode_esi_register,decode_op_4bc90,1}, /* 4E */
    {MN_DEC,0x11e,0x0,0x0,decode_edi_register,decode_op_4bcd0,1}, /* 4F */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_eax_register,1}, /* 50 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_ecx_register,1}, /* 51 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4df50,1}, /* 52 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_ebx_register,1}, /* 53 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4dfa0,1}, /* 54 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_ebp_register,1}, /* 55 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_esi_register,1}, /* 56 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_edi_register,1}, /* 57 */
    {MN_POP,0x0,0x0,0x0,decode_eax_register,update_stack_registers,1}, /* 58 */
    {MN_POP,0x0,0x0,0x0,decode_ecx_register,update_stack_registers,1}, /* 59 */
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
    {MN_NONE,0x0,0x0,0x0,NULL,decode_fs_prefix,1}, /* 64 */
    {MN_NONE,0x0,0x0,0x0,NULL,decode_op_4be00,1}, /* 65 */
    {MN_NONE,0x0,0x0,0x0,decode_operand_size_prefix,NULL,1}, /* 66 */
    {MN_NONE,0x0,0x0,0x0,decode_address_size_prefix,NULL,1}, /* 67 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_immediate,1}, /* 68 */
    {MN_IMUL,0x101,0x0,0x1e,decode_modrm_register,decode_op_4c910,1}, /* 69 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_byte_immediate,1}, /* 6A */
    {MN_IMUL,0x101,0x0,0x1e,decode_modrm_register,decode_op_4c940,1}, /* 6B */
    {MN_INS,0x0,0x80,0x0,decode_string_byte_destination,decode_op_4df20,1}, /* 6C */
    {MN_INS,0x0,0x80,0x0,decode_op_4be90,decode_op_4df20,1}, /* 6D */
    {MN_OUTS,0x0,0x80,0x0,decode_op_4df20,decode_string_byte_source,1}, /* 6E */
    {MN_OUTS,0x0,0x80,0x0,decode_op_4df20,decode_op_4bfe0,1}, /* 6F */
    {MN_JO,0x0,0x100,0x0,NULL,decode_short_relative,14}, /* 70 */
    {MN_JNO,0x0,0x100,0x0,NULL,decode_short_relative,14}, /* 71 */
    {MN_JB,0x0,0x1,0x0,NULL,decode_short_relative,14}, /* 72 */
    {MN_JAE,0x0,0x1,0x0,NULL,decode_short_relative,14}, /* 73 */
    {MN_JE,0x0,0x8,0x0,NULL,decode_short_relative,14}, /* 74 */
    {MN_JNE,0x0,0x8,0x0,NULL,decode_short_relative,14}, /* 75 */
    {MN_JBE,0x0,0x9,0x0,NULL,decode_short_relative,14}, /* 76 */
    {MN_JA,0x0,0x9,0x0,NULL,decode_short_relative,14}, /* 77 */
    {MN_JS,0x0,0x10,0x0,NULL,decode_short_relative,14}, /* 78 */
    {MN_JNS,0x0,0x10,0x0,NULL,decode_short_relative,14}, /* 79 */
    {MN_JP,0x0,0x2,0x0,NULL,decode_short_relative,14}, /* 7A */
    {MN_JNP,0x0,0x2,0x0,NULL,decode_short_relative,14}, /* 7B */
    {MN_JL,0x0,0x110,0x0,NULL,decode_short_relative,14}, /* 7C */
    {MN_JGE,0x0,0x110,0x0,NULL,decode_short_relative,14}, /* 7D */
    {MN_JLE,0x0,0x18,0x0,NULL,decode_short_relative,14}, /* 7E */
    {MN_JG,0x0,0x18,0x0,NULL,decode_short_relative,14}, /* 7F */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b670,decode_byte_immediate,1}, /* 80 */
    {MN_NONE,0x0,0x0,0x0,decode_signed_byte_group,decode_immediate,1}, /* 81 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_byte_immediate,1}, /* 82 */
    {MN_NONE,0x0,0x0,0x0,decode_signed_byte_group,decode_byte_immediate,1}, /* 83 */
    {MN_TEST,0x11b,0x0,0x4,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 84 */
    {MN_TEST,0x11b,0x0,0x4,decode_modrm_operand_effects,decode_modrm_register,1}, /* 85 */
    {MN_XCHG,0x0,0x0,0x0,decode_modrm_byte_operand,decode_modrm_byte_register,1}, /* 86 */
    {MN_XCHG,0x0,0x0,0x0,decode_modrm_operand_effects,decode_op_4d690,1}, /* 87 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_byte_operand,decode_op_4d640,1}, /* 88 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_operand_effects,decode_modrm_register,1}, /* 89 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_byte_register,decode_modrm_byte_operand,1}, /* 8A */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_register,decode_modrm_operand_effects,1}, /* 8B */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c830,decode_op_4d5e0,1}, /* 8C */
    {MN_LEA,0x0,0x0,0x0,decode_modrm_register,decode_memory_operand,1}, /* 8D */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d5e0,decode_op_4c830,1}, /* 8E */
    {MN_POP,0x0,0x0,0x0,decode_modrm_operand_effects,update_stack_registers,1}, /* 8F */
    {MN_NOP,0x0,0x0,0x0,NULL,NULL,1}, /* 90 */
    {MN_XCHG,0x0,0x0,0x0,decode_ecx_register,decode_eax_destination,1}, /* 91 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4df50,decode_eax_destination,1}, /* 92 */
    {MN_XCHG,0x0,0x0,0x0,decode_ebx_register,decode_eax_destination,1}, /* 93 */
    {MN_XCHG,0x0,0x0,0x0,decode_op_4dfa0,decode_eax_destination,1}, /* 94 */
    {MN_XCHG,0x0,0x0,0x0,decode_ebp_register,decode_eax_destination,1}, /* 95 */
    {MN_XCHG,0x0,0x0,0x0,decode_esi_register,decode_eax_destination,1}, /* 96 */
    {MN_XCHG,0x0,0x0,0x0,decode_edi_register,decode_eax_destination,1}, /* 97 */
    {MN_CBW,0x0,0x0,0x0,decode_op_4bb10,decode_op_4ba10,1}, /* 98 */
    {MN_CWD,0x0,0x0,0x0,decode_op_4bbd0,decode_op_4b960,1}, /* 99 */
    {MN_CALL,0x0,0x0,0x0,decode_op_4b9c0,decode_op_4c1a0,3}, /* 9A */
    {MN_WAIT,0x0,0x0,0x0,NULL,NULL,1}, /* 9B */
    {MN_PUSHF,0x0,0x1ff,0x0,update_stack_registers,NULL,1}, /* 9C */
    {MN_POPF,0x1ff,0x0,0x0,update_stack_registers,NULL,1}, /* 9D */
    {MN_SAHF,0x1f,0x0,0x0,NULL,decode_op_4ba70,1}, /* 9E */
    {MN_LAHF,0x0,0x0,0x0,decode_op_4ba70,NULL,1}, /* 9F */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_op_4c3d0,1}, /* A0 */
    {MN_MOV,0x0,0x0,0x11f,decode_eax_register,decode_absolute_memory,1}, /* A1 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c3d0,decode_op_4dbb0,1}, /* A2 */
    {MN_MOV,0x0,0x0,0x11f,decode_absolute_memory,decode_eax_register,1}, /* A3 */
    {MN_MOVS,0x0,0x80,0x0,decode_string_byte_destination,decode_string_byte_source,1}, /* A4 */
    {MN_MOVS,0x0,0x80,0x0,decode_op_4be90,decode_op_4bfe0,1}, /* A5 */
    {MN_CMPS,0x11f,0x80,0x0,decode_string_byte_source,decode_string_byte_destination,1}, /* A6 */
    {MN_CMPS,0x11f,0x80,0x0,decode_op_4bfe0,decode_op_4be90,1}, /* A7 */
    {MN_TEST,0x11b,0x0,0x4,decode_op_4dbb0,decode_byte_immediate,1}, /* A8 */
    {MN_TEST,0x11b,0x0,0x4,decode_eax_register,decode_immediate,1}, /* A9 */
    {MN_STOS,0x0,0x80,0x0,decode_string_byte_destination,update_al_register,1}, /* AA */
    {MN_STOS,0x0,0x80,0x0,decode_op_4be90,decode_op_4bb10,1}, /* AB */
    {MN_LODS,0x0,0x80,0x0,update_al_register,decode_string_byte_source,1}, /* AC */
    {MN_LODS,0x0,0x80,0x0,decode_op_4bb10,decode_op_4bfe0,1}, /* AD */
    {MN_SCAS,0x11f,0x80,0x0,decode_string_byte_destination,update_al_register,1}, /* AE */
    {MN_SCAS,0x11f,0x80,0x0,decode_op_4be90,decode_op_4bb10,1}, /* AF */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbb0,decode_byte_immediate,1}, /* B0 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dc30,decode_byte_immediate,1}, /* B1 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dc70,decode_byte_immediate,1}, /* B2 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dbf0,decode_byte_immediate,1}, /* B3 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dcb0,decode_byte_immediate,1}, /* B4 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dd30,decode_byte_immediate,1}, /* B5 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dd70,decode_byte_immediate,1}, /* B6 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dcf0,decode_byte_immediate,1}, /* B7 */
    {MN_MOV,0x0,0x0,0x11f,decode_eax_register,decode_immediate,1}, /* B8 */
    {MN_MOV,0x0,0x0,0x11f,decode_ecx_register,decode_immediate,1}, /* B9 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4df50,decode_immediate,1}, /* BA */
    {MN_MOV,0x0,0x0,0x11f,decode_ebx_register,decode_immediate,1}, /* BB */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4dfa0,decode_immediate,1}, /* BC */
    {MN_MOV,0x0,0x0,0x11f,decode_ebp_register,decode_immediate,1}, /* BD */
    {MN_MOV,0x0,0x0,0x11f,decode_esi_register,decode_immediate,1}, /* BE */
    {MN_MOV,0x0,0x0,0x11f,decode_edi_register,decode_immediate,1}, /* BF */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_byte_immediate,1}, /* C0 */
    {MN_NONE,0x0,0x0,0x0,decode_shift_group,decode_byte_immediate,1}, /* C1 */
    {MN_RET,0x0,0x0,0x0,update_stack_registers,decode_op_4c0b0,11}, /* C2 */
    {MN_RET,0x0,0x0,0x0,update_stack_registers,NULL,11}, /* C3 */
    {MN_LES,0x0,0x0,0x0,decode_op_4d8a0,decode_op_4ca60,1}, /* C4 */
    {MN_LDS,0x0,0x0,0x0,decode_op_4d900,decode_op_4ca60,1}, /* C5 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_byte_operand,decode_byte_immediate,1}, /* C6 */
    {MN_MOV,0x0,0x0,0x11f,decode_modrm_operand_effects,decode_immediate,1}, /* C7 */
    {MN_ENTER,0x0,0x0,0x0,decode_op_4d990,decode_byte_immediate,1}, /* C8 */
    {MN_LEAVE,0x0,0x0,0x0,update_stack_registers,update_frame_registers,1}, /* C9 */
    {MN_RETF,0x0,0x0,0x0,decode_op_4b9c0,decode_op_4c0b0,12}, /* CA */
    {MN_RETF,0x0,0x0,0x0,decode_op_4b9c0,NULL,12}, /* CB */
    {MN_INT,0x30,0x0,0x0,decode_op_4b9c0,decode_op_4c170,9}, /* CC */
    {MN_INT,0x30,0x0,0x0,decode_op_4b9c0,decode_byte_immediate,8}, /* CD */
    {MN_INTO,0x30,0x0,0x0,decode_op_4b9c0,NULL,7}, /* CE */
    {MN_IRET,0x0,0x0,0x0,decode_op_4b9c0,NULL,10}, /* CF */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_op_4c140,1}, /* D0 */
    {MN_NONE,0x0,0x0,0x0,decode_shift_group,decode_op_4c140,1}, /* D1 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b6d0,decode_op_4dc30,1}, /* D2 */
    {MN_NONE,0x0,0x0,0x0,decode_shift_group,decode_op_4dc30,1}, /* D3 */
    {MN_AAM,0x1a,0x101,0x105,decode_op_4ba90,decode_op_4c060,1}, /* D4 */
    {MN_AAD,0x1a,0x0,0x105,decode_op_4ba90,decode_op_4c060,1}, /* D5 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,NULL,1}, /* D6 */
    {MN_XLAT,0x0,0x0,0x0,decode_op_4b9f0,decode_op_4bf70,1}, /* D7 */
    {MN_NONE,0x0,0x0,0x0,decode_x87_d8,NULL,1}, /* D8 */
    {MN_NONE,0x0,0x0,0x0,decode_x87_d9,NULL,1}, /* D9 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4cdf0,NULL,1}, /* DA */
    {MN_NONE,0x0,0x0,0x0,decode_x87_db,NULL,1}, /* DB */
    {MN_NONE,0x0,0x0,0x0,decode_x87_dc,NULL,1}, /* DC */
    {MN_NONE,0x0,0x0,0x0,decode_x87_dd,NULL,1}, /* DD */
    {MN_NONE,0x0,0x0,0x0,decode_x87_de,NULL,1}, /* DE */
    {MN_NONE,0x0,0x0,0x0,decode_x87_df,NULL,1}, /* DF */
    {MN_LOOPNE,0x0,0x8,0x0,update_counter_registers,decode_short_relative,20}, /* E0 */
    {MN_LOOPE,0x0,0x8,0x0,update_counter_registers,decode_short_relative,20}, /* E1 */
    {MN_LOOP,0x0,0x0,0x0,update_counter_registers,decode_short_relative,20}, /* E2 */
    {MN_JCXZ,0x0,0x0,0x0,update_counter_registers,decode_short_relative,21}, /* E3 */
    {MN_IN,0x0,0x0,0x0,decode_op_4dbb0,decode_byte_immediate,1}, /* E4 */
    {MN_IN,0x0,0x0,0x0,decode_eax_register,decode_byte_immediate,1}, /* E5 */
    {MN_OUT,0x0,0x0,0x0,decode_byte_immediate,decode_op_4dbb0,1}, /* E6 */
    {MN_OUT,0x0,0x0,0x0,decode_byte_immediate,decode_eax_register,1}, /* E7 */
    {MN_CALL,0x0,0x0,0x0,update_stack_registers,decode_near_relative,2}, /* E8 */
    {MN_JMP,0x0,0x0,0x0,NULL,decode_near_relative,15}, /* E9 */
    {MN_JMP,0x0,0x0,0x0,decode_op_4bac0,decode_op_4c1a0,18}, /* EA */
    {MN_JMP,0x0,0x0,0x0,NULL,decode_short_relative,13}, /* EB */
    {MN_IN,0x0,0x0,0x0,decode_op_4dbb0,decode_op_4df20,1}, /* EC */
    {MN_IN,0x0,0x0,0x0,decode_eax_register,decode_op_4df20,1}, /* ED */
    {MN_OUT,0x0,0x0,0x0,decode_op_4df20,decode_op_4dbb0,1}, /* EE */
    {MN_OUT,0x0,0x0,0x0,decode_op_4df20,decode_op_4de00,1}, /* EF */
    {MN_LOCK,0x0,0x0,0x0,NULL,NULL,1}, /* F0 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,NULL,1}, /* F1 */
    {MN_REPNZ,0x0,0x8,0x0,decode_repne_prefix,NULL,1}, /* F2 */
    {MN_REP,0x0,0x8,0x0,decode_repe_prefix,NULL,1}, /* F3 */
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

/* ModRM.reg arithmetic selectors for opcode83, original [00083bc0,00083c60). */
const OpcodeEntry disassembly_signed_byte_group[8]={
    {MN_ADD,0x11f,0,0,decode_modrm_operand_effects,0,1},
    {MN_OR, 0x11b,0,4,decode_modrm_operand_effects,0,1},
    {MN_ADC,0x11f,0,0,decode_modrm_operand_effects,0,1},
    {MN_SBB,0x11f,0,0,decode_modrm_operand_effects,0,1},
    {MN_AND,0x11b,0,4,decode_modrm_operand_effects,0,1},
    {MN_SUB,0x11f,0,0,decode_modrm_operand_effects,0,1},
    {MN_XOR,0x11b,0,4,decode_modrm_operand_effects,0,1},
    {MN_CMP,0x11f,0,0,decode_modrm_operand_effects,0,1}
};

/* Names used by the compressed0F descriptor pages. */
enum ExtendedMnemonic {
    MN_BSF=10,
    MN_BSR=11,
    MN_BSWAP=12,
    MN_BT=13,
    MN_BTC=14,
    MN_BTR=15,
    MN_BTS=16,
    MN_CLTS=23,
    MN_CMPXCHG=27,
    MN_IBTS=121,
    MN_INVD=130,
    MN_LAR=156,
    MN_LFS=159,
    MN_LGS=160,
    MN_LOADALL=161,
    MN_LSS=162,
    MN_LSL=180,
    MN_MOVSX=184,
    MN_MOVZX=185,
    MN_SETB=220,
    MN_SETAE=221,
    MN_SETBE=222,
    MN_SETA=223,
    MN_SETE=224,
    MN_SETNE=225,
    MN_SETL=226,
    MN_SETGE=227,
    MN_SETLE=228,
    MN_SETG=229,
    MN_SETS=230,
    MN_SETNS=231,
    MN_SETO=234,
    MN_SETNO=235,
    MN_SETP=236,
    MN_SETNP=237,
    MN_SHLD=245,
    MN_SHRD=246,
    MN_WBINVD=258,
    MN_XADD=259,
    MN_XBTS=260
};

/* High-nibble page bases; unsupported pages use the native -1 sentinel. */
const long disassembly_extended_pages[16]={
    0,-1,16,-1,-1,-1,-1,-1,32,48,64,80,96,-1,-1,-1
};

/* Actual112 compressed0F descriptors, original [00083260,00083b20). */
const OpcodeEntry disassembly_extended_opcodes[112]={
    {MN_NONE,0x0,0x0,0x0,decode_op_4b7f0,0,1}, /* page slot 00 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b820,0,1}, /* page slot 01 */
    {MN_LAR,0x8,0x0,0x0,decode_modrm_register,decode_op_4c830,1}, /* page slot 02 */
    {MN_LSL,0x8,0x0,0x0,decode_modrm_register,decode_op_4c830,1}, /* page slot 03 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 04 */
    {MN_LOADALL,0x0,0x0,0x0,0,0,1}, /* page slot 05 */
    {MN_CLTS,0x0,0x0,0x0,0,0,1}, /* page slot 06 */
    {MN_LOADALL,0x0,0x0,0x0,decode_string_byte_destination,0,1}, /* page slot 07 */
    {MN_INVD,0x0,0x0,0x0,0,0,1}, /* page slot 08 */
    {MN_WBINVD,0x0,0x0,0x0,0,0,1}, /* page slot 09 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0A */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0B */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0C */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0D */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0E */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 0F */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d520,decode_op_4c530,1}, /* page slot 10 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d560,decode_op_4c530,1}, /* page slot 11 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c530,decode_op_4d520,1}, /* page slot 12 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c530,decode_op_4d560,1}, /* page slot 13 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4d5a0,decode_op_4c530,1}, /* page slot 14 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 15 */
    {MN_MOV,0x0,0x0,0x11f,decode_op_4c530,decode_op_4d5a0,1}, /* page slot 16 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 17 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 18 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 19 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1A */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1B */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1C */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1D */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1E */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 1F */
    {MN_JO,0x0,0x100,0x0,0,decode_near_relative,16}, /* page slot 20 */
    {MN_JNO,0x0,0x100,0x0,0,decode_near_relative,16}, /* page slot 21 */
    {MN_JB,0x0,0x1,0x0,0,decode_near_relative,16}, /* page slot 22 */
    {MN_JAE,0x0,0x1,0x0,0,decode_near_relative,16}, /* page slot 23 */
    {MN_JE,0x0,0x8,0x0,0,decode_near_relative,16}, /* page slot 24 */
    {MN_JNE,0x0,0x8,0x0,0,decode_near_relative,16}, /* page slot 25 */
    {MN_JBE,0x0,0x9,0x0,0,decode_near_relative,16}, /* page slot 26 */
    {MN_JA,0x0,0x9,0x0,0,decode_near_relative,16}, /* page slot 27 */
    {MN_JS,0x0,0x10,0x0,0,decode_near_relative,16}, /* page slot 28 */
    {MN_JNS,0x0,0x10,0x0,0,decode_near_relative,16}, /* page slot 29 */
    {MN_JP,0x0,0x2,0x0,0,decode_near_relative,16}, /* page slot 2A */
    {MN_JNP,0x0,0x2,0x0,0,decode_near_relative,16}, /* page slot 2B */
    {MN_JL,0x0,0x110,0x0,0,decode_near_relative,16}, /* page slot 2C */
    {MN_JGE,0x0,0x110,0x0,0,decode_near_relative,16}, /* page slot 2D */
    {MN_JLE,0x0,0x18,0x0,0,decode_near_relative,16}, /* page slot 2E */
    {MN_JG,0x0,0x18,0x0,0,decode_near_relative,16}, /* page slot 2F */
    {MN_SETO,0x0,0x100,0x0,decode_modrm_byte_operand,0,1}, /* page slot 30 */
    {MN_SETNO,0x0,0x100,0x0,decode_modrm_byte_operand,0,1}, /* page slot 31 */
    {MN_SETB,0x0,0x1,0x0,decode_modrm_byte_operand,0,1}, /* page slot 32 */
    {MN_SETAE,0x0,0x1,0x0,decode_modrm_byte_operand,0,1}, /* page slot 33 */
    {MN_SETE,0x0,0x8,0x0,decode_modrm_byte_operand,0,1}, /* page slot 34 */
    {MN_SETNE,0x0,0x8,0x0,decode_modrm_byte_operand,0,1}, /* page slot 35 */
    {MN_SETBE,0x0,0x9,0x0,decode_modrm_byte_operand,0,1}, /* page slot 36 */
    {MN_SETA,0x0,0x9,0x0,decode_modrm_byte_operand,0,1}, /* page slot 37 */
    {MN_SETS,0x0,0x10,0x0,decode_modrm_byte_operand,0,1}, /* page slot 38 */
    {MN_SETNS,0x0,0x10,0x0,decode_modrm_byte_operand,0,1}, /* page slot 39 */
    {MN_SETP,0x0,0x2,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3A */
    {MN_SETNP,0x0,0x2,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3B */
    {MN_SETL,0x0,0x110,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3C */
    {MN_SETGE,0x0,0x110,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3D */
    {MN_SETLE,0x0,0x18,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3E */
    {MN_SETG,0x0,0x18,0x0,decode_modrm_byte_operand,0,1}, /* page slot 3F */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4db30,1}, /* page slot 40 */
    {MN_POP,0x0,0x0,0x0,decode_op_4db30,update_stack_registers,1}, /* page slot 41 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 42 */
    {MN_BT,0x1,0x0,0x0,decode_op_4c6d0,decode_modrm_register,1}, /* page slot 43 */
    {MN_SHLD,0x1b,0x0,0x104,decode_modrm_operand_effects,decode_register_and_byte_immediate,1}, /* page slot 44 */
    {MN_SHLD,0x1b,0x0,0x104,decode_modrm_operand_effects,decode_op_4d810,1}, /* page slot 45 */
    {MN_XBTS,0x0,0x0,0x0,decode_op_4d850,decode_modrm_operand_effects,1}, /* page slot 46 */
    {MN_IBTS,0x0,0x0,0x0,decode_modrm_operand_effects,decode_op_4d850,1}, /* page slot 47 */
    {MN_PUSH,0x0,0x0,0x0,update_stack_registers,decode_op_4db70,1}, /* page slot 48 */
    {MN_POP,0x0,0x0,0x0,decode_op_4db70,update_stack_registers,1}, /* page slot 49 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 4A */
    {MN_BTS,0x1,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* page slot 4B */
    {MN_SHRD,0x1b,0x0,0x104,decode_modrm_operand_effects,decode_register_and_byte_immediate,1}, /* page slot 4C */
    {MN_SHRD,0x1b,0x0,0x104,decode_modrm_operand_effects,decode_op_4d810,1}, /* page slot 4D */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 4E */
    {MN_IMUL,0x101,0x0,0x1e,decode_modrm_register,decode_modrm_operand_effects,1}, /* page slot 4F */
    {MN_CMPXCHG,0x11f,0x0,0x0,decode_op_4d9d0,decode_modrm_byte_register,1}, /* page slot 50 */
    {MN_CMPXCHG,0x11f,0x0,0x0,decode_op_4da00,decode_modrm_register,1}, /* page slot 51 */
    {MN_LSS,0x0,0x0,0x0,decode_op_4d8d0,decode_op_4ca60,1}, /* page slot 52 */
    {MN_BTR,0x1,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* page slot 53 */
    {MN_LFS,0x0,0x0,0x0,decode_op_4d930,decode_op_4ca60,1}, /* page slot 54 */
    {MN_LGS,0x0,0x0,0x0,decode_op_4d960,decode_op_4ca60,1}, /* page slot 55 */
    {MN_MOVZX,0x0,0x0,0x0,decode_modrm_register,decode_modrm_byte_operand,1}, /* page slot 56 */
    {MN_MOVZX,0x0,0x0,0x0,decode_modrm_register,decode_op_4c830,1}, /* page slot 57 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 58 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 59 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b850,0,1}, /* page slot 5A */
    {MN_BTC,0x1,0x0,0x0,decode_modrm_operand_effects,decode_modrm_register,1}, /* page slot 5B */
    {MN_BSF,0x8,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* page slot 5C */
    {MN_BSR,0x8,0x0,0x0,decode_modrm_register,decode_modrm_operand_effects,1}, /* page slot 5D */
    {MN_MOVSX,0x0,0x0,0x0,decode_modrm_register,decode_modrm_byte_operand,1}, /* page slot 5E */
    {MN_MOVSX,0x0,0x0,0x0,decode_modrm_register,decode_op_4c830,1}, /* page slot 5F */
    {MN_XADD,0x11f,0x0,0x0,decode_modrm_byte_operand,decode_op_4d640,1}, /* page slot 60 */
    {MN_XADD,0x11f,0x0,0x0,decode_modrm_operand_effects,decode_op_4c710,1}, /* page slot 61 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 62 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 63 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 64 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 65 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 66 */
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1}, /* page slot 67 */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bb10,0,1}, /* page slot 68 */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bb90,0,1}, /* page slot 69 */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bbd0,0,1}, /* page slot 6A */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bb50,0,1}, /* page slot 6B */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bc10,0,1}, /* page slot 6C */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bc50,0,1}, /* page slot 6D */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bc90,0,1}, /* page slot 6E */
    {MN_BSWAP,0x0,0x0,0x0,decode_op_4bcd0,0,1}, /* page slot 6F */
};

/* Shift/rotate display identities. */
enum ShiftMnemonic {
    MN_RCL=207,
    MN_RCR=208,
    MN_ROL=209,
    MN_ROR=210,
    MN_SHL=241,
    MN_SHR=242,
    MN_SAL=243,
    MN_SAR=244
};

/* Native eight-entry shift/rotate descriptors at00083d00. */
const OpcodeEntry disassembly_shift_group[8]={
    {MN_ROL,0x101,0x0,0x0,decode_modrm_operand_effects,0,1},
    {MN_ROR,0x101,0x0,0x0,decode_modrm_operand_effects,0,1},
    {MN_RCL,0x101,0x0,0x0,decode_modrm_operand_effects,0,1},
    {MN_RCR,0x101,0x0,0x0,decode_modrm_operand_effects,0,1},
    {MN_SHL,0x11b,0x0,0x4,decode_modrm_operand_effects,0,1},
    {MN_SHR,0x11b,0x0,0x4,decode_modrm_operand_effects,0,1},
    {MN_SAL,0x11b,0x0,0x4,decode_modrm_operand_effects,0,1},
    {MN_SAR,0x11b,0x0,0x4,decode_modrm_operand_effects,0,1},
};

/* Display identities used by disassembly_x87_dd_memory. */
enum disassembly_x87_dd_memory_mnemonics {
    MN_FLD=61,
    MN_FRSTOR=83,
    MN_FNSAVE=86,
    MN_FST=94,
    MN_FSTP=95,
    MN_FNSTSW=103
};

/* Contextual8-entry descriptor table at00084520. */
const OpcodeEntry disassembly_x87_dd_memory[8]={
    {MN_FLD,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FST,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FSTP,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FRSTOR,0x0,0x0,0x0,decode_x87_memory,0,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FNSAVE,0x0,0x0,0x0,decode_x87_memory,0,1},
    {MN_FNSTSW,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
};

/* Display identities used by disassembly_x87_d9_memory. */
enum disassembly_x87_d9_memory_mnemonics {
    MN_FLDCW=70,
    MN_FLDENV=71,
    MN_FNSTCW=99,
    MN_FNSTENV=100
};

/* Contextual8-entry descriptor table at000842a0. */
const OpcodeEntry disassembly_x87_d9_memory[8]={
    {MN_FLD,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FST,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FSTP,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FLDENV,0x0,0x0,0x0,decode_x87_memory,0,1},
    {MN_FLDCW,0x0,0x0,0x0,decode_x87_memory,0,1},
    {MN_FNSTENV,0x0,0x0,0x0,decode_x87_memory,0,1},
    {MN_FNSTCW,0x0,0x0,0x0,decode_x87_memory,0,1},
};

/* Display identities used by disassembly_x87_d8_memory. */
enum disassembly_x87_d8_memory_mnemonics {
    MN_FADD=37,
    MN_FCOM=43,
    MN_FCOMP=44,
    MN_FDIV=51,
    MN_FDIVR=54,
    MN_FMUL=74,
    MN_FSUB=104,
    MN_FSUBR=107
};

/* Contextual8-entry descriptor table at00084200. */
const OpcodeEntry disassembly_x87_d8_memory[8]={
    {MN_FADD,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FMUL,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FCOM,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FCOMP,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FSUB,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FSUBR,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FDIV,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FDIVR,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
};

/* Contextual8-entry descriptor table at00084480. */
const OpcodeEntry disassembly_x87_dc_memory[8]={
    {MN_FADD,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FMUL,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FCOM,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FCOMP,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FSUB,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FSUBR,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FDIV,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FDIVR,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
};

/* Display identities used by disassembly_x87_db_memory. */
enum disassembly_x87_db_memory_mnemonics {
    MN_FILD=62,
    MN_FIST=96,
    MN_FISTP=97
};

/* Contextual8-entry descriptor table at000843e0. */
const OpcodeEntry disassembly_x87_db_memory[8]={
    {MN_FILD,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FIST,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_FISTP,0x0,0x0,0x0,decode_x87_memory,size_x87_dword,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FLD,0x0,0x0,0x0,decode_x87_memory,decode_op_4d510,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FSTP,0x0,0x0,0x0,decode_x87_memory,decode_op_4d510,1},
};

/* Contextual8-entry descriptor table at000845c0. */
const OpcodeEntry disassembly_x87_de_memory[8]={
    {MN_FADD,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FMUL,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FCOM,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FCOMP,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FSUB,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FSUBR,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FDIV,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FDIVR,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
};

/* Display identities used by disassembly_x87_df_memory. */
enum disassembly_x87_df_memory_mnemonics {
    MN_FBLD=40,
    MN_FBSTP=98
};

/* Contextual8-entry descriptor table at00084660. */
const OpcodeEntry disassembly_x87_df_memory[8]={
    {MN_FILD,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_NONE,0x0,0x0,0x0,decode_op_4b650,0,1},
    {MN_FIST,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FISTP,0x0,0x0,0x0,decode_x87_memory,decode_op_4d4e0,1},
    {MN_FBLD,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FILD,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FBSTP,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
    {MN_FISTP,0x0,0x0,0x0,decode_x87_memory,size_x87_qword,1},
};

/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Inline assembler instruction definitions and initialization.
*
****************************************************************************/


#ifndef WPP_ASM_H
#define WPP_ASM_H
#include <recon.h>
enum asm_cpu {
        /* bit count from left: ( need at least 7 bits )
           bit 0-2:   Math coprocessor
           bit 3:   Protected mode
           bit 4-6: cpu type */

        P_NO87  = 0x00,         /* no FPU */
        P_87    = 0x01,         /* 8087 */
        P_287   = 0x02,         /* 80287 */
        P_387   = 0x04,         /* 80387 */

        P_PM    = 0x08,         /* protect-mode */

        P_86    = 0x00,         /* 8086, default */
        P_186   = 0x10,         /* 80186 */
        P_286   = 0x20,         /* 80286 */
        P_286p  = P_286 | P_PM, /* 80286, protected mode */
        P_386   = 0x30,         /* 80386 */
        P_386p  = P_386 | P_PM, /* 80386, protected mode */
        P_486   = 0x40,         /* 80486 */
        P_486p  = P_486 | P_PM, /* 80486, protected mode */
        P_586   = 0x50,         /* pentium */
        P_586p  = P_586 | P_PM, /* pentium, protected mode */
        P_686   = 0x60,         /* later value; no 686 opcode rows here */
        P_686p  = P_686 | P_PM, /* pentium, protected mode */

        P_END  = 0x70,      /* END */


        P_FPU_MASK = 0x07,
        P_CPU_MASK = 0x70
};


#define NO_PREFIX   0x00
#define LOCK        0x01
#define REPxx       0x02
#define FWAIT       0x03

#define PREFIX_ES       0x26
#define PREFIX_CS       0x2E
#define PREFIX_SS       0x36
#define PREFIX_DS       0x3E
#define PREFIX_FS       0x64
#define PREFIX_GS       0x65

#define F_0F        0x2
#define F_16        0x1
#define F_32        0x3

#define no_RM       0x1
#define no_WDS      0x2
#define R_in_OP     0x3

#define IREG    1       /* indicates INDEX register for an OP_REGISTER entry*/

#define CPU_DIRECTIVE 4
/* This earlier inlined assembler packs a9-bit token before the CPU bits. */
#pragma pack(1)
struct asm_ins {
    unsigned token:9,cpu:7,allowed_prefix:3,byte1_info:2,rm_info:2,reserved:9;
    unsigned long opnd_type[2];
    unsigned char opcode,rm_byte;
};
struct asm_tok { signed char token; long value; char *string_ptr; };
struct asm_code {
    signed short prefix,mem_type;
    long data[2];
    struct asm_ins info;
    signed char seg_prefix,extended_ins,adrsiz,opsiz;
    unsigned char sib,use32;
    signed short distance;
    unsigned mem_type_fixed:1;
};
#pragma pack()
struct AsmCodeName {
    unsigned short position;
    unsigned short len:4,index:12;
    struct AsmCodeName *next;
};
enum asm_token {
    T_186,
    T_286,
    T_286C,
    T_286P,
    T_287,
    T_386,
    T_386P,
    T_387,
    T_486,
    T_486P,
    T_586,
    T_586P,
    T_8086,
    T_8087,
    T_AAA,
    T_AAD,
    T_AAM,
    T_AAS,
    T_ADC,
    T_ADD,
    T_AH,
    T_AL,
    T_AND,
    T_ARPL,
    T_AX,
    T_BH,
    T_BL,
    T_BOUND,
    T_BP,
    T_BSF,
    T_BSR,
    T_BSWAP,
    T_BT,
    T_BTC,
    T_BTR,
    T_BTS,
    T_BX,
    T_BYTE,
    T_CALL,
    T_CALLF,
    T_CBW,
    T_CDQ,
    T_CH,
    T_CL,
    T_CLC,
    T_CLD,
    T_CLI,
    T_CLTS,
    T_CMC,
    T_CMP,
    T_CMPS,
    T_CMPSB,
    T_CMPSD,
    T_CMPSW,
    T_CMPXCHG,
    T_CMPXCHG8B,
    T_CPUID,
    T_CR0,
    T_CR2,
    T_CR3,
    T_CR4,
    T_CS,
    T_CWD,
    T_CWDE,
    T_CX,
    T_DAA,
    T_DAS,
    T_DB,
    T_DD,
    T_DEC,
    T_DF,
    T_DH,
    T_DI,
    T_DIV,
    T_DL,
    T_DP,
    T_DR0,
    T_DR1,
    T_DR2,
    T_DR3,
    T_DR6,
    T_DR7,
    T_DS,
    T_DUP,
    T_DW,
    T_DWORD,
    T_DX,
    T_EAX,
    T_EBP,
    T_EBX,
    T_ECX,
    T_EDI,
    T_EDX,
    T_ENTER,
    T_ES,
    T_ESI,
    T_ESP,
    T_F2XM1,
    T_FABS,
    T_FADD,
    T_FADDP,
    T_FAR,
    T_FBLD,
    T_FBSTP,
    T_FCHS,
    T_FCLEX,
    T_FCOM,
    T_FCOMP,
    T_FCOMPP,
    T_FCOS,
    T_FDECSTP,
    T_FDISI,
    T_FDIV,
    T_FDIVP,
    T_FDIVR,
    T_FDIVRP,
    T_FENI,
    T_FFREE,
    T_FIADD,
    T_FICOM,
    T_FICOMP,
    T_FIDIV,
    T_FIDIVR,
    T_FILD,
    T_FIMUL,
    T_FINCSTP,
    T_FINIT,
    T_FIST,
    T_FISTP,
    T_FISUB,
    T_FISUBR,
    T_FLD,
    T_FLD1,
    T_FLDCW,
    T_FLDENV,
    T_FLDENVD,
    T_FLDENVW,
    T_FLDL2E,
    T_FLDL2T,
    T_FLDLG2,
    T_FLDLN2,
    T_FLDPI,
    T_FLDZ,
    T_FMUL,
    T_FMULP,
    T_FNCLEX,
    T_FNDISI,
    T_FNENI,
    T_FNINIT,
    T_FNOP,
    T_FNRSTOR,
    T_FNRSTORD,
    T_FNRSTORW,
    T_FNSAVE,
    T_FNSAVED,
    T_FNSAVEW,
    T_FNSTCW,
    T_FNSTENV,
    T_FNSTENVD,
    T_FNSTENVW,
    T_FNSTSW,
    T_FPATAN,
    T_FPREM,
    T_FPREM1,
    T_FPTAN,
    T_FRNDINT,
    T_FRSTOR,
    T_FRSTORD,
    T_FRSTORW,
    T_FS,
    T_FSAVE,
    T_FSAVED,
    T_FSAVEW,
    T_FSCALE,
    T_FSETPM,
    T_FSIN,
    T_FSINCOS,
    T_FSQRT,
    T_FST,
    T_FSTCW,
    T_FSTENV,
    T_FSTENVD,
    T_FSTENVW,
    T_FSTP,
    T_FSTSW,
    T_FSUB,
    T_FSUBP,
    T_FSUBR,
    T_FSUBRP,
    T_FTST,
    T_FUCOM,
    T_FUCOMP,
    T_FUCOMPP,
    T_FWAIT,
    T_FWORD,
    T_FXAM,
    T_FXCH,
    T_FXTRACT,
    T_FYL2X,
    T_FYL2XP1,
    T_GS,
    T_HLT,
    T_IDIV,
    T_IMUL,
    T_IN,
    T_INC,
    T_INS2,
    T_INSB,
    T_INSD,
    T_INSW,
    T_INT,
    T_INTO,
    T_INVD,
    T_INVLPG,
    T_IRET,
    T_IRETD,
    T_JA,
    T_JAE,
    T_JB,
    T_JBE,
    T_JC,
    T_JCXZ,
    T_JE,
    T_JECXZ,
    T_JG,
    T_JGE,
    T_JL,
    T_JLE,
    T_JMP,
    T_JMPF,
    T_JNA,
    T_JNAE,
    T_JNB,
    T_JNBE,
    T_JNC,
    T_JNE,
    T_JNG,
    T_JNGE,
    T_JNL,
    T_JNLE,
    T_JNO,
    T_JNP,
    T_JNS,
    T_JNZ,
    T_JO,
    T_JP,
    T_JPE,
    T_JPO,
    T_JS,
    T_JZ,
    T_LAHF,
    T_LAR,
    T_LDS,
    T_LEA,
    T_LEAVE,
    T_LES,
    T_LFS,
    T_LGDT,
    T_LGS,
    T_LIDT,
    T_LLDT,
    T_LMSW,
    T_LOCK,
    T_LODS,
    T_LODSB,
    T_LODSD,
    T_LODSW,
    T_LOOP,
    T_LOOPE,
    T_LOOPNE,
    T_LOOPNZ,
    T_LOOPZ,
    T_LSL,
    T_LSS,
    T_LTR,
    T_MOV,
    T_MOVS,
    T_MOVSB,
    T_MOVSD,
    T_MOVSW,
    T_MOVSX,
    T_MOVZX,
    T_MUL,
    T_NEAR,
    T_NEG,
    T_NO87,
    T_NOP,
    T_NOT,
    T_OFFSET,
    T_OR,
    T_OUT,
    T_OUTS,
    T_OUTSB,
    T_OUTSD,
    T_OUTSW,
    T_POP,
    T_POPA,
    T_POPAD,
    T_POPF,
    T_POPFD,
    T_PTR,
    T_PUSH,
    T_PUSHA,
    T_PUSHAD,
    T_PUSHF,
    T_PUSHFD,
    T_PWORD,
    T_QWORD,
    T_RCL,
    T_RCR,
    T_RDMSR,
    T_RDTSC,
    T_REP,
    T_REPE,
    T_REPNE,
    T_REPNZ,
    T_REPZ,
    T_RET,
    T_RETF,
    T_RETN,
    T_ROL,
    T_ROR,
    T_RSM,
    T_SAHF,
    T_SAL,
    T_SAR,
    T_SBB,
    T_SCAS,
    T_SCASB,
    T_SCASD,
    T_SCASW,
    T_SEG2,
    T_SETA,
    T_SETAE,
    T_SETB,
    T_SETBE,
    T_SETC,
    T_SETE,
    T_SETG,
    T_SETGE,
    T_SETL,
    T_SETLE,
    T_SETNA,
    T_SETNAE,
    T_SETNB,
    T_SETNBE,
    T_SETNC,
    T_SETNE,
    T_SETNG,
    T_SETNGE,
    T_SETNL,
    T_SETNLE,
    T_SETNO,
    T_SETNP,
    T_SETNS,
    T_SETNZ,
    T_SETO,
    T_SETP,
    T_SETPE,
    T_SETPO,
    T_SETS,
    T_SETZ,
    T_SGDT,
    T_SHL,
    T_SHLD,
    T_SHORT,
    T_SHR,
    T_SHRD,
    T_SI,
    T_SIDT,
    T_SLDT,
    T_SMSW,
    T_SP,
    T_SS,
    T_ST,
    T_STC,
    T_STD,
    T_STI,
    T_STOS,
    T_STOSB,
    T_STOSD,
    T_STOSW,
    T_STR,
    T_SUB,
    T_TBYTE,
    T_TEST,
    T_TR3,
    T_TR4,
    T_TR5,
    T_TR6,
    T_TR7,
    T_VERR,
    T_VERW,
    T_WAIT,
    T_WBINVD,
    T_WORD,
    T_WRMSR,
    T_XADD,
    T_XCHG,
    T_XLAT,
    T_XLATB,
    T_XOR,
    T_NULL
};
extern const struct asm_ins AsmOpTable[771];
extern struct AsmCodeName AsmOpcode[T_NULL+1];
extern char AsmChars[];
extern struct asm_tok *AsmBuffer[100];
extern struct asm_code *Code;
void AsmInit(int cpu,int fpu,int use32);
void AsmError(int code);
void make_inst_hash_table(void);
struct AsmCodeName *add(struct AsmCodeName *instruction);
struct AsmCodeName **find(char *name);
unsigned AsmHashPJW(char *name);
#endif

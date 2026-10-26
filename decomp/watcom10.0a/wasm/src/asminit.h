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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

#ifndef ASMINIT_H
#define ASMINIT_H
#include <stddef.h>
#include "wasm.h"
#include "asmopnds.h"
#include <recon.h>

enum asm_token {
    T_186 = 0,
    T_286 = 1,
    T_286C = 2,
    T_286P = 3,
    T_287 = 4,
    T_386 = 5,
    T_386P = 6,
    T_387 = 7,
    T_486 = 8,
    T_486P = 9,
    T_586 = 10,
    T_586P = 11,
    T_8086 = 12,
    T_8087 = 13,
    T_ALPHA = 14,
    T_BREAK = 15,
    T_CODE = 16,
    T_CONST = 17,
    T_CONTINUE = 18,
    T_CREF = 19,
    T_DATA = 20,
    T_DATA_UN = 21,
    T_DOSSEG = 22,
    T_ENDW = 23,
    T_ERR = 24,
    T_ERRB = 25,
    T_ERRDEF = 26,
    T_ERRDIF = 27,
    T_ERRDIFI = 28,
    T_ERRE = 29,
    T_ERRIDN = 30,
    T_ERRIDNI = 31,
    T_ERRNB = 32,
    T_ERRNDEF = 33,
    T_ERRNZ = 34,
    T_EXIT = 35,
    T_FARDATA = 36,
    T_FARDATA_UN = 37,
    T_LFCOND = 38,
    T_LIST = 39,
    T_LISTALL = 40,
    T_LISTIF = 41,
    T_LISTMACRO = 42,
    T_LISTMACROALL = 43,
    T_MODEL = 44,
    T_NOCREF = 45,
    T_NOLIST = 46,
    T_RADIX = 47,
    T_REPEAT = 48,
    T_SALL = 49,
    T_SEQ = 50,
    T_SFCOND = 51,
    T_STACK = 52,
    T_STARTUP = 53,
    T_TFCOND = 54,
    T_UNTIL = 55,
    T_WHILE = 56,
    T_XCREF = 57,
    T_XLIST = 58,
    T_AAA = 59,
    T_AAD = 60,
    T_AAM = 61,
    T_AAS = 62,
    T_ABS2 = 63,
    T_ADC = 64,
    T_ADD = 65,
    T_ADDR = 66,
    T_AH = 67,
    T_AL = 68,
    T_ALIAS = 69,
    T_ALIGN = 70,
    T_AND = 71,
    T_ARPL = 72,
    T_ASSUME = 73,
    T_AT = 74,
    T_AX = 75,
    T_BASIC = 76,
    T_BH = 77,
    T_BL = 78,
    T_BOUND = 79,
    T_BP = 80,
    T_BSF = 81,
    T_BSR = 82,
    T_BSWAP = 83,
    T_BT = 84,
    T_BTC = 85,
    T_BTR = 86,
    T_BTS = 87,
    T_BX = 88,
    T_BYTE = 89,
    T_C = 90,
    T_CALL = 91,
    T_CALLF = 92,
    T_CASEMAP = 93,
    T_CATSTR = 94,
    T_CBW = 95,
    T_CDQ = 96,
    T_CH = 97,
    T_CL = 98,
    T_CLC = 99,
    T_CLD = 100,
    T_CLI = 101,
    T_CLTS = 102,
    T_CMC = 103,
    T_CMP = 104,
    T_CMPS = 105,
    T_CMPSB = 106,
    T_CMPSD = 107,
    T_CMPSW = 108,
    T_CMPXCHG = 109,
    T_CMPXCHG8B = 110,
    T_COMM = 111,
    T_COMMENT = 112,
    T_COMMON = 113,
    T_COMPACT = 114,
    T_CPUID = 115,
    T_CR0 = 116,
    T_CR2 = 117,
    T_CR3 = 118,
    T_CR4 = 119,
    T_CS = 120,
    T_CWD = 121,
    T_CWDE = 122,
    T_CX = 123,
    T_DAA = 124,
    T_DAS = 125,
    T_DB = 126,
    T_DD = 127,
    T_DEC = 128,
    T_DF = 129,
    T_DH = 130,
    T_DI = 131,
    T_DIV = 132,
    T_DL = 133,
    T_DP = 134,
    T_DQ = 135,
    T_DR0 = 136,
    T_DR1 = 137,
    T_DR2 = 138,
    T_DR3 = 139,
    T_DR6 = 140,
    T_DR7 = 141,
    T_DS = 142,
    T_DT = 143,
    T_DUP = 144,
    T_DW = 145,
    T_DWORD = 146,
    T_DX = 147,
    T_EAX = 148,
    T_EBP = 149,
    T_EBX = 150,
    T_ECHO = 151,
    T_ECX = 152,
    T_EDI = 153,
    T_EDX = 154,
    T_ELSE = 155,
    T_ELSEIF = 156,
    T_END = 157,
    T_ENDIF = 158,
    T_ENDP = 159,
    T_ENDS = 160,
    T_ENTER = 161,
    T_EQ = 162,
    T_EQU = 163,
    T_EQU2 = 164,
    T_ERROR = 165,
    T_ES = 166,
    T_ESI = 167,
    T_ESP = 168,
    T_EVEN = 169,
    T_EXPORT = 170,
    T_EXTERN = 171,
    T_EXTERNDEF = 172,
    T_EXTRN = 173,
    T_F2XM1 = 174,
    T_FABS = 175,
    T_FADD = 176,
    T_FADDP = 177,
    T_FAR = 178,
    T_FARSTACK = 179,
    T_FBLD = 180,
    T_FBSTP = 181,
    T_FCHS = 182,
    T_FCLEX = 183,
    T_FCOM = 184,
    T_FCOMP = 185,
    T_FCOMPP = 186,
    T_FCOS = 187,
    T_FDECSTP = 188,
    T_FDISI = 189,
    T_FDIV = 190,
    T_FDIVP = 191,
    T_FDIVR = 192,
    T_FDIVRP = 193,
    T_FENI = 194,
    T_FFREE = 195,
    T_FIADD = 196,
    T_FICOM = 197,
    T_FICOMP = 198,
    T_FIDIV = 199,
    T_FIDIVR = 200,
    T_FILD = 201,
    T_FIMUL = 202,
    T_FINCSTP = 203,
    T_FINIT = 204,
    T_FIST = 205,
    T_FISTP = 206,
    T_FISUB = 207,
    T_FISUBR = 208,
    T_FLAT = 209,
    T_FLD = 210,
    T_FLD1 = 211,
    T_FLDCW = 212,
    T_FLDENV = 213,
    T_FLDENVD = 214,
    T_FLDENVW = 215,
    T_FLDL2E = 216,
    T_FLDL2T = 217,
    T_FLDLG2 = 218,
    T_FLDLN2 = 219,
    T_FLDPI = 220,
    T_FLDZ = 221,
    T_FMUL = 222,
    T_FMULP = 223,
    T_FNCLEX = 224,
    T_FNDISI = 225,
    T_FNENI = 226,
    T_FNINIT = 227,
    T_FNOP = 228,
    T_FNRSTOR = 229,
    T_FNRSTORD = 230,
    T_FNRSTORW = 231,
    T_FNSAVE = 232,
    T_FNSAVED = 233,
    T_FNSAVEW = 234,
    T_FNSTCW = 235,
    T_FNSTENV = 236,
    T_FNSTENVD = 237,
    T_FNSTENVW = 238,
    T_FNSTSW = 239,
    T_FOR = 240,
    T_FORC = 241,
    T_FORTRAN = 242,
    T_FPATAN = 243,
    T_FPREM = 244,
    T_FPREM1 = 245,
    T_FPTAN = 246,
    T_FRNDINT = 247,
    T_FRSTOR = 248,
    T_FRSTORD = 249,
    T_FRSTORW = 250,
    T_FS = 251,
    T_FSAVE = 252,
    T_FSAVED = 253,
    T_FSAVEW = 254,
    T_FSCALE = 255,
    T_FSETPM = 256,
    T_FSIN = 257,
    T_FSINCOS = 258,
    T_FSQRT = 259,
    T_FST = 260,
    T_FSTCW = 261,
    T_FSTENV = 262,
    T_FSTENVD = 263,
    T_FSTENVW = 264,
    T_FSTP = 265,
    T_FSTSW = 266,
    T_FSUB = 267,
    T_FSUBP = 268,
    T_FSUBR = 269,
    T_FSUBRP = 270,
    T_FTST = 271,
    T_FUCOM = 272,
    T_FUCOMP = 273,
    T_FUCOMPP = 274,
    T_FWAIT = 275,
    T_FWORD = 276,
    T_FXAM = 277,
    T_FXCH = 278,
    T_FXTRACT = 279,
    T_FYL2X = 280,
    T_FYL2XP1 = 281,
    T_GE = 282,
    T_GLOBAL = 283,
    T_GROUP = 284,
    T_GS = 285,
    T_GT = 286,
    T_HIGH = 287,
    T_HIGHWORD = 288,
    T_HLT = 289,
    T_HUGE = 290,
    T_IDIV = 291,
    T_IF = 292,
    T_IF1 = 293,
    T_IF2 = 294,
    T_IFB = 295,
    T_IFDEF = 296,
    T_IFDIF = 297,
    T_IFDIFI = 298,
    T_IFE = 299,
    T_IFIDN = 300,
    T_IFIDNI = 301,
    T_IFNB = 302,
    T_IFNDEF = 303,
    T_IGNORE = 304,
    T_IMUL = 305,
    T_IN = 306,
    T_INC = 307,
    T_INCLUDE = 308,
    T_INCLUDELIB = 309,
    T_INS2 = 310,
    T_INSB = 311,
    T_INSD = 312,
    T_INSW = 313,
    T_INT = 314,
    T_INTO = 315,
    T_INVD = 316,
    T_INVLPG = 317,
    T_INVOKE = 318,
    T_IRET = 319,
    T_IRETD = 320,
    T_IRP = 321,
    T_JA = 322,
    T_JAE = 323,
    T_JB = 324,
    T_JBE = 325,
    T_JC = 326,
    T_JCXZ = 327,
    T_JE = 328,
    T_JECXZ = 329,
    T_JG = 330,
    T_JGE = 331,
    T_JL = 332,
    T_JLE = 333,
    T_JMP = 334,
    T_JMPF = 335,
    T_JNA = 336,
    T_JNAE = 337,
    T_JNB = 338,
    T_JNBE = 339,
    T_JNC = 340,
    T_JNE = 341,
    T_JNG = 342,
    T_JNGE = 343,
    T_JNL = 344,
    T_JNLE = 345,
    T_JNO = 346,
    T_JNP = 347,
    T_JNS = 348,
    T_JNZ = 349,
    T_JO = 350,
    T_JP = 351,
    T_JPE = 352,
    T_JPO = 353,
    T_JS = 354,
    T_JZ = 355,
    T_LABEL = 356,
    T_LAHF = 357,
    T_LAR = 358,
    T_LARGE = 359,
    T_LDS = 360,
    T_LE = 361,
    T_LEA = 362,
    T_LEAVE = 363,
    T_LENGTH = 364,
    T_LENGTHOF = 365,
    T_LES = 366,
    T_LFS = 367,
    T_LGDT = 368,
    T_LGS = 369,
    T_LIDT = 370,
    T_LLDT = 371,
    T_LMSW = 372,
    T_LOCAL = 373,
    T_LOCK = 374,
    T_LODS = 375,
    T_LODSB = 376,
    T_LODSD = 377,
    T_LODSW = 378,
    T_LOOP = 379,
    T_LOOPE = 380,
    T_LOOPNE = 381,
    T_LOOPNZ = 382,
    T_LOOPZ = 383,
    T_LOW = 384,
    T_LOWWORD = 385,
    T_LROFFSET = 386,
    T_LSL = 387,
    T_LSS = 388,
    T_LT = 389,
    T_LTR = 390,
    T_MACRO = 391,
    T_MASK = 392,
    T_MEDIUM = 393,
    T_MEMORY = 394,
    T_MOD = 395,
    T_MOV = 396,
    T_MOVS = 397,
    T_MOVSB = 398,
    T_MOVSD = 399,
    T_MOVSW = 400,
    T_MOVSX = 401,
    T_MOVZX = 402,
    T_MUL = 403,
    T_NAME = 404,
    T_NE = 405,
    T_NEAR = 406,
    T_NEARSTACK = 407,
    T_NEG = 408,
    T_NO87 = 409,
    T_NOP = 410,
    T_NOT = 411,
    T_NOTHING = 412,
    T_OFFSET = 413,
    T_OPATTR = 414,
    T_OPTION = 415,
    T_OR = 416,
    T_ORG = 417,
    T_OS_DOS = 418,
    T_OS_OS2 = 419,
    T_OUT = 420,
    T_OUTS = 421,
    T_OUTSB = 422,
    T_OUTSD = 423,
    T_OUTSW = 424,
    T_PAGE = 425,
    T_PARA = 426,
    T_PASCAL = 427,
    T_POP = 428,
    T_POPA = 429,
    T_POPAD = 430,
    T_POPCONTEXT = 431,
    T_POPF = 432,
    T_POPFD = 433,
    T_PRIVATE = 434,
    T_PROC = 435,
    T_PROTO = 436,
    T_PTR = 437,
    T_PUBLIC = 438,
    T_PURGE = 439,
    T_PUSH = 440,
    T_PUSHA = 441,
    T_PUSHAD = 442,
    T_PUSHCONTEXT = 443,
    T_PUSHF = 444,
    T_PUSHFD = 445,
    T_PWORD = 446,
    T_QWORD = 447,
    T_RCL = 448,
    T_RCR = 449,
    T_RDMSR = 450,
    T_RDTSC = 451,
    T_READONLY = 452,
    T_RECORD = 453,
    T_REP = 454,
    T_REPE = 455,
    T_REPNE = 456,
    T_REPNZ = 457,
    T_REPZ = 458,
    T_RET = 459,
    T_RETF = 460,
    T_RETN = 461,
    T_ROL = 462,
    T_ROR = 463,
    T_RSM = 464,
    T_SAHF = 465,
    T_SAL = 466,
    T_SAR = 467,
    T_SBB = 468,
    T_SBYTE = 469,
    T_SCAS = 470,
    T_SCASB = 471,
    T_SCASD = 472,
    T_SCASW = 473,
    T_SDWORD = 474,
    T_SEG2 = 475,
    T_SEGMENT = 476,
    T_SETA = 477,
    T_SETAE = 478,
    T_SETB = 479,
    T_SETBE = 480,
    T_SETC = 481,
    T_SETE = 482,
    T_SETG = 483,
    T_SETGE = 484,
    T_SETL = 485,
    T_SETLE = 486,
    T_SETNA = 487,
    T_SETNAE = 488,
    T_SETNB = 489,
    T_SETNBE = 490,
    T_SETNC = 491,
    T_SETNE = 492,
    T_SETNG = 493,
    T_SETNGE = 494,
    T_SETNL = 495,
    T_SETNLE = 496,
    T_SETNO = 497,
    T_SETNP = 498,
    T_SETNS = 499,
    T_SETNZ = 500,
    T_SETO = 501,
    T_SETP = 502,
    T_SETPE = 503,
    T_SETPO = 504,
    T_SETS = 505,
    T_SETZ = 506,
    T_SGDT = 507,
    T_SHL = 508,
    T_SHLD = 509,
    T_SHORT = 510,
    T_SHR = 511,
    T_SHRD = 512,
    T_SI = 513,
    T_SIDT = 514,
    T_SIZE = 515,
    T_SIZEOF = 516,
    T_SLDT = 517,
    T_SMALL = 518,
    T_SMSW = 519,
    T_SP = 520,
    T_SS = 521,
    T_ST = 522,
    T_STC = 523,
    T_STD = 524,
    T_STDCALL = 525,
    T_STI = 526,
    T_STOS = 527,
    T_STOSB = 528,
    T_STOSD = 529,
    T_STOSW = 530,
    T_STR = 531,
    T_STRUC = 532,
    T_STRUCT = 533,
    T_SUB = 534,
    T_SUBTITLE = 535,
    T_SUBTTL = 536,
    T_SWORD = 537,
    T_SYSCALL = 538,
    T_TBYTE = 539,
    T_TEST = 540,
    T_TEXTEQU = 541,
    T_THIS = 542,
    T_TINY = 543,
    T_TITLE = 544,
    T_TR3 = 545,
    T_TR4 = 546,
    T_TR5 = 547,
    T_TR6 = 548,
    T_TR7 = 549,
    T_TYPEDEF = 550,
    T_UNION = 551,
    T_USE16 = 552,
    T_USE32 = 553,
    T_USES = 554,
    T_VARARG = 555,
    T_VERR = 556,
    T_VERW = 557,
    T_WAIT = 558,
    T_WATCOM_C = 559,
    T_WBINVD = 560,
    T_WIDTH = 561,
    T_WORD = 562,
    T_WRMSR = 563,
    T_XADD = 564,
    T_XCHG = 565,
    T_XLAT = 566,
    T_XLATB = 567,
    T_XOR = 568,
    T_NULL = 569
};
#pragma pack(1)
struct AsmCodeName {
    unsigned short position;
    unsigned short len:4, index:12;
    struct AsmCodeName *next;
};
struct asm_tok {
    long value;
    unsigned char token;
    char *string_ptr;
};
/* Original packed17B instruction layout. CF9730 reads prefix bits0..1,
 * byte1 bits3..4 and rm bits5..6; bit2 and bits7..31 are padding.
 * Consumers guard domains whose semantic fields are unrecovered. */
struct asm_ins {
    unsigned short token;
    unsigned char cpu;
    unsigned allowed_prefix : 2;
    unsigned : 1;
    unsigned byte1_info : 2;
    unsigned rm_info : 2;
    unsigned : 25;
    unsigned long opnd_type[2];
    unsigned char opcode, rm_byte;
};
struct asm_code {
    short prefix, mem_type;
    long data[2];
    struct asm_ins info;
    signed char seg_prefix, extended_ins, adrsiz, opsiz;
    unsigned char sib, use32;
    short distance;
    unsigned mem_type_fixed : 1, : 31;
};
#pragma pack()
typedef char options_c_octals_offset_check[offsetof(global_options, allow_c_octals) == 37 ? 1 : -1];
typedef char asm_tok_stride_check[sizeof(struct asm_tok) == 9 ? 1 : -1];
typedef char asm_name_stride_check[sizeof(struct AsmCodeName) == 8 ? 1 : -1];
typedef char asm_cpu_offset_check[offsetof(struct asm_code, info.cpu) == 14 ? 1 : -1];
typedef char asm_use32_offset_check[offsetof(struct asm_code, use32) == 34 ? 1 : -1];
extern char AsmChars[];
extern struct AsmCodeName AsmOpcode[T_NULL + 1];
extern const struct asm_ins AsmOpTable[939];
typedef char asm_ins_size_check[sizeof(struct asm_ins) == 17 ? 1 : -1];
typedef char asm_ins_operand_check[offsetof(struct asm_ins,opnd_type) == 7 ? 1 : -1];
extern struct asm_tok *AsmBuffer[100];
extern struct asm_code *Code;
/* ModuleInfo through its256B basename buffer, CF274c4..275d0. */
#pragma pack(1)
typedef struct module_info {
    unsigned char distance, model, langtype, ostype;
    /* Original signed1-bit loads become0/-1 (CF10c73..10c7e). */
    int use32 : 1, init : 1;
    unsigned : 30;
    unsigned flat_idx;
    char name[256];
} module_info;
#pragma pack()
typedef char module_prefix_size_check[sizeof(module_info) == 268 ? 1 : -1];
extern module_info ModuleInfo;
unsigned int hashpjw(char *name);
struct AsmCodeName **find(char *name);
enum state {
    T_FINAL, T_INS, T_RES_ID, T_ID, T_REG, T_STRING, T_DIRECTIVE,
    T_DIRECT_EXPR, T_DEC_NUM, T_OCT_NUM, T_HEX_NUM_0, T_HEX_NUM,
    T_NUM, T_FLOAT, T_NOOP, T_POSITIVE, T_NEGATIVE, T_ID_IN_BACKQUOTES,
    T_BIN_NUM, T_PATH, T_UNARY_OPERATOR, T_BAD_NUM, T_COMMA = 44, T_QUESTION_MARK = 63,
    T_COLON = 58, T_DOT = 46, T_OP_SQ_BRACKET = 91,
    T_CL_SQ_BRACKET = 93, T_PERCENT = 37
};
#pragma pack(1)
typedef struct asm_sym {
    struct asm_sym *next;
    char *name;
    unsigned grpidx, segidx;
    unsigned long offset;
    unsigned char public, first_size, first_length, total_size, total_length;
    char *(*mangler)(struct asm_sym *sym, char *buffer);
    int mem_type;
    unsigned char state;
    struct asmfixup *fixup;
} asm_sym;
typedef struct const_info {
    int redefine:1, expand_early:1;
    unsigned int :30;
    int count;
    struct asm_tok *data;
} const_info;
/* Encountered allocation layouts in dir_insert, CFfa3c..fce6. */
typedef struct seg_info {
    obj_rec *segrec;
    unsigned idx;
    unsigned long start_loc;
    signed char readonly:1, ignore:1;
    unsigned lname_idx;
    unsigned long current_loc;
} seg_info;
typedef struct seg_list { struct seg_list *next; struct dir_node *seg; } seg_list;
typedef struct regs_list { struct regs_list *next; char *reg; } regs_list;
typedef struct label_list {
    struct label_list *next; char *label, *replace;
    int size, factor;
    int is_vararg:1;
    unsigned :31;
    /* Actual allocator reserves28B; this reached prefix consumes24B.
     * The final4B remain uninterpreted until a reached consumer uses them. */
} label_list;
typedef struct field_list {
    struct field_list *next; char *initializer, *value;
} field_list;
typedef struct grp_info {
    unsigned idx;
    struct seg_list *seglist;
    unsigned numseg, lname_idx;
} grp_info;
typedef struct ext_info { unsigned idx; } ext_info;
typedef struct comm_info { unsigned idx; unsigned long size; unsigned distance; } comm_info;
typedef struct proc_info {
    struct regs_list *regslist;
    unsigned char visibility, langtype;
    struct label_list *paralist, *locallist;
    int parasize, localsize, mem_type;
    int is_vararg:1;
    unsigned int :31;
} proc_info;
typedef struct parm_list {
    struct parm_list *next;
    char *label, *replace;
    char required;
    char *def;
} parm_list;
typedef struct asmlines {
    struct asmlines *next; char *line; unsigned char parmcount;
} asmlines;
typedef char asmlines_size_check[sizeof(asmlines) == 9 ? 1 : -1];
typedef struct macro_info {
    struct parm_list *parmlist;
    struct asmlines *data;
    char *filename;
    unsigned start_line;
    char hidden;
} macro_info;
typedef struct lname_info { unsigned idx; } lname_info;
typedef struct struct_info {
    unsigned short size, alignment;
    struct field_list *head, *tail;
} struct_info;
typedef struct dir_node {
    asm_sym sym;
    union {
        const_info *constinfo;
        seg_info *seginfo;
        grp_info *grpinfo;
        ext_info *extinfo;
        comm_info *comminfo;
        proc_info *procinfo;
        macro_info *macroinfo;
        lname_info *lnameinfo;
        struct_info *structinfo;
    } e;
    unsigned short line;
    struct dir_node *next, *prev;
} dir_node;
#pragma pack()
enum { SYM_UNDEFINED = 0, SYM_INTERNAL = 1, SYM_EXTERNAL = 2, SYM_STACK = 3, SYM_SEG = 4, SYM_GRP = 5,
       SYM_PROC = 6, SYM_MACRO = 7, SYM_CONST = 8, SYM_LNAME = 11,
       SYM_CLASS_LNAME = 12, SYM_STRUCT_FIELD = 13, SYM_STRUCT = 14, PASS_1 = 0 };
enum { TAB_SEG, TAB_GRP, TAB_PUB, TAB_LIB, TAB_EXT, TAB_CONST, TAB_PROC,
       TAB_MACRO, TAB_LNAME, TAB_CLASS_LNAME, TAB_STRUCT, TAB_GLOBAL,
       TAB_COMM, TAB_LAST };
typedef struct symbol_queue { dir_node *head, *tail; } symbol_queue;
extern symbol_queue Tables[TAB_LAST];
typedef char seg_info_size_check[sizeof(seg_info) == 21 ? 1 : -1];
typedef char proc_info_size_check[sizeof(proc_info) == 30 ? 1 : -1];
typedef char macro_info_size_check[sizeof(macro_info) == 17 ? 1 : -1];
extern dir_node *CurrProc;
extern unsigned LnamesIdx, segdefidx, extdefidx;
asm_sym *AsmAdd(asm_sym *sym);
unsigned char AsmQueryExternal(char *name);
/* Both released interfaces share the single shipped CFdd50 body. */
#define AsmQueryType AsmQueryExternal
enum { ASSUME_DS, ASSUME_ES, ASSUME_SS, ASSUME_FS, ASSUME_GS, ASSUME_CS,
       ASSUME_LAST, ASSUME_ERROR, ASSUME_NOTHING };
#pragma pack(1)
typedef struct assume_info {
    asm_sym *symbol;
    unsigned error : 1, flat : 1;
    unsigned : 30;
} assume_info;
#pragma pack()
typedef char assume_info_size_check[sizeof(assume_info) == 8 ? 1 : -1];
extern assume_info AssumeTable[ASSUME_LAST];
typedef char asm_sym_size_check[sizeof(asm_sym) == 38 ? 1 : -1];
typedef char const_info_size_check[sizeof(const_info) == 12 ? 1 : -1];
typedef char dir_node_size_check[sizeof(dir_node) == 52 ? 1 : -1];
typedef char sym_state_offset_check[offsetof(asm_sym, state) == 33 ? 1 : -1];
extern char Parse_Pass;
typedef char *(*mangle_func)(asm_sym *symbol, char *buffer);
mangle_func GetMangler(char *type);
char *CMangler(asm_sym *symbol, char *buffer);
char *AsmMangler(asm_sym *symbol, char *buffer);
void AsmErr(int message, ...);
asm_sym *AsmGetSymbol(char *name);
asm_sym **AsmFind(char *name);
dir_node *dir_insert(char *name, int table);
void FreeInfo(dir_node *dir);
void FreePubQueue(void);
void FreeAliasQueue(void);
void FreeLnameQueue(void);
void InputQueueLine(char *line);
void PushMacro(char *name, unsigned char hidden);
int macro_exam(int index);
asmlines *asmline_insert(asmlines **head, void *data);
void put_parm_placeholders_in_line(asmlines *line, parm_list *parameters);
char *replace_parm(parm_list *parameters, char *start, unsigned char length, asmlines *line);
char *get_curr_filename(void);
char *fill_in_parms(asmlines *line, parm_list *parameters);
int my_sprintf(char *buffer, char *format, int max, char **parameters);
signed char lineis(char *line, char *prefix);
int macro_local(void);
void free_parmlist(parm_list *head);
int ExpandTheWorld(int start, unsigned char early_only);
int ExpandAllConsts(int start, unsigned char early_only);
int ExpandSymbol(int index, int early_only);
void AddTokens(struct asm_tok **buffer, int start, int count);
int EvalExpr(int count, int start, int end);
extern int TokCnt;
enum { EXPR_ADDR, EXPR_CONST, EXPR_REG };
/* Actual1994 expression owner is38 bytes, packed; the flags occupy
 * four bytes at29, expr_type33 and scale37 (released asmeval.h). */
#pragma pack(1)
typedef struct expr_list {
    signed char type;
    long value;
    char *string;
    int base_reg, idx_reg, label, override, instr;
    unsigned indirect:1, explicit:1, :30;
    int expr_type;
    unsigned char scale;
} expr_list;
#pragma pack()
typedef char expression_size_check[sizeof(expr_list) == 38 ? 1 : -1];
typedef char expression_type_offset_check[offsetof(expr_list,expr_type) == 33 ? 1 : -1];
expr_list *create_expr(void);
expr_list *get_operand(int index);
void optr_enqueue(int index, signed char queue[]);
void optr_dequeue(signed char queue[]);
signed char is_optr(int index);
signed char check_same(expr_list *first, expr_list *second, signed char type);
signed char check_both(expr_list *first, expr_list *second, signed char type1, signed char type2);
void index_connect(expr_list *first, expr_list *second);
void MakeConst(expr_list *token);
void TokenAssign(expr_list *first, expr_list *second);
signed char is_unary(int index, char sign);
signed char cmp_token(int index, long token);
int get_precedence(int index);
signed char calculate(expr_list *first, expr_list *second, unsigned char index);
int fix_parant(void);
int is_expr(int index);
struct expr_list *evaluate(int *index, int end);
int fix(struct expr_list *result, int start, int end);
void fix_final(void);
extern int Token_Count;
extern char *CurrString;
int get_id(unsigned int *index, char **input, char **output);
int get_inc_path(unsigned int *index, char **input, char **output);
int get_number(struct asm_tok *buf, char **input, char **output);
int get_float(struct asm_tok *buf, char **input, char **output);
label_list *label_cmp(char *word, label_list *label);
int ExpandProcString(int index);
int get_string(struct asm_tok *buf, char **input, char **output);
int get_id_in_backquotes(struct asm_tok *buf, char **input, char **output);
int get_special_symbol(struct asm_tok *buf, char **input, char **output);
struct AsmCodeName *add(struct AsmCodeName *inst);
void make_inst_hash_table(void);
void AsmError(int message);
void GetInsString(unsigned short token, char *buffer, int length);
#endif

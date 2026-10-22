/* Encoding recipes encountered by the common x86 encoder at 00025540.
   Original form/opcode choices occupy six candidate slots per operation:
   opcode bytes [00081730,00081c9a), form/modifier bytes [00081ca0,0008220a),
   extension bytes [00081648,0008172f). These are handwritten ISA recipes;
   original packed form flags are expressed as named modifiers below. */
#include "encoding-recipes.h"
#include <recon.h>
#define G(n) ((n) << 3)
#define P(form,byte,flags) { ENC_##form, byte, flags }
#define W ENCODE_WIDTH
#define S ENCODE_SIGN_EXTEND
#define D ENCODE_DIRECTION
#define EMPTY(name,extra) { MC_##name, extra, {{ ENC_INVALID, 0, 0 }} }
#define FIX(name,byte,tail) { MC_##name, tail, { P(FIXED,byte,0) } }
#define RM(name,byte,group,flags) { MC_##name, G(group), { P(MODRM,byte,flags) } }
#define GROUP_OP(name,group) { MC_##name, G(group), { P(MODRM_ALTERNATE,0xf6,W) } }
#define SHIFT(name,group) { MC_##name, G(group), { P(SHIFT,0xc0,W), P(INVALID,0xd2,0) } }
#define ARITH(name,acc,rm,group) { MC_##name, G(group), { P(ACCUMULATOR_IMMEDIATE,acc,S|W), P(RM_IMMEDIATE,0x80,S|W), P(MODRM,rm,D|W) } }
#define STRING(name,byte) { MC_##name, 0, { P(FIXED,byte,W) } }
#define X87(name,byte,extra,reverse,flags) { MC_##name, extra, { P(X87_ARITHMETIC,byte,flags), P(INVALID,reverse,0) } }
#define X87_MEMORY(name,byte,group) { MC_##name, G(group), { P(X87_MEMORY,byte,D) } }
#define MEMORY(name,byte,group) { MC_##name, G(group), { P(MEMORY_GROUP,byte,0) } }
#define EXT_RM(name,byte) RM(name,byte,0,0)
#define BIT_OP(name,byte,group) { MC_##name, G(group), { P(SPECIAL,0xba,0), P(MODRM,byte,0) } }
const InstructionEncoding instruction_encodings[MC_OPCODE_COUNT] = {
    EMPTY(FIRST,0),
    { MC_MOV, 0, { P(SEGMENT_MOVE,0x8c,D), P(ACCUMULATOR_ADDRESS,0xa0,W),
        P(REGISTER_IMMEDIATE,0xb0,0), P(RM_IMMEDIATE,0xc6,W), P(MODRM,0x88,D|W) } },
    RM(ARPL,0x63,0,0), RM(BOUND,0x62,0,0),
    { MC_ENTER, 0, { P(SPECIAL,0xc8,0) } }, EMPTY(ESC,0),
    GROUP_OP(IMUL,5), { MC_INT, 0, { P(SPECIAL,0xcc,0) } },
    { MC_RET, 0, { P(SPECIAL,0xc2,0) } },
    { MC_XCHG, 0, { P(SPECIAL,0x90,0), P(MODRM,0x86,W) } },
    { MC_IN, 0, { P(SPECIAL,0xec,0), P(INVALID,0xe4,0) } },
    { MC_OUT, 0, { P(SPECIAL,0xee,0), P(INVALID,0xe6,0) } },
    { MC_POP, 0, { P(SEGMENT_STACK,0x07,0), P(PUSH_IMMEDIATE,0,S),
        P(REGISTER_OPCODE,0x58,0), P(MODRM,0x8f,0) } },
    { MC_PUSH, G(6), { P(SEGMENT_STACK,0x06,0), P(PUSH_IMMEDIATE,0x68,S),
        P(REGISTER_OPCODE,0x50,0), P(MODRM,0xff,0) } },
    { MC_CALL, G(2), { P(RELATIVE_NEAR,0xe8,0), P(FAR_ADDRESS,0x9a,0), P(MODRM,0xff,0) } },
    { MC_JCC, 0, { P(SPECIAL,0x70,0), P(INVALID,0x80,0) } },
    { MC_JMP, G(4), { P(RELATIVE_BYTE,0xeb,0), P(RELATIVE_NEAR,0xe9,0),
        P(FAR_ADDRESS,0xea,0), P(MODRM,0xff,0) } },
    RM(LDS,0xc5,0,0), RM(LEA,0x8d,0,0), RM(LES,0xc4,0,0),
    RM(LAR,0x02,0,0), RM(LSL,0x03,0,0),
    { MC_JCXZ, 0, { P(RELATIVE_BYTE,0xe3,0) } },
    { MC_LOOP, 0, { P(RELATIVE_BYTE,0xe2,0) } },
    { MC_LOOPNZ, 0, { P(RELATIVE_BYTE,0xe0,0) } },
    { MC_LOOPZ, 0, { P(RELATIVE_BYTE,0xe1,0) } },
    RM(LGDT,0x01,2,0), RM(LIDT,0x01,3,0), RM(SGDT,0x01,0,0), RM(SIDT,0x01,1,0),
    { MC_DEC, G(1), { P(REGISTER_OPCODE,0x48,0), P(MODRM,0xfe,W) } },
    GROUP_OP(DIV,6), GROUP_OP(IDIV,7),
    { MC_INC, 0, { P(REGISTER_OPCODE,0x40,0), P(MODRM,0xfe,W) } },
    GROUP_OP(MUL,4), RM(NEG,0xf6,3,W), RM(NOT,0xf6,2,W),
    SHIFT(RCL,2), SHIFT(RCR,3), SHIFT(ROL,0), SHIFT(ROR,1),
    SHIFT(SAR,7), SHIFT(SHL,4), SHIFT(SHR,5),
    ARITH(ADC,0x14,0x10,2), ARITH(ADD,0x04,0x00,0),
    ARITH(AND,0x24,0x20,4), ARITH(CMP,0x3c,0x38,7),
    ARITH(OR,0x0c,0x08,1), ARITH(SBB,0x1c,0x18,3), ARITH(SUB,0x2c,0x28,5),
    { MC_TEST, 0, { P(ACCUMULATOR_IMMEDIATE,0xa8,W), P(RM_IMMEDIATE,0xf6,W), P(MODRM,0x84,W) } },
    ARITH(XOR,0x34,0x30,6),
    STRING(CMPS,0xa6), STRING(INS,0x6c), STRING(LODS,0xac), STRING(MOVS,0xa4),
    STRING(OUTS,0x6e), STRING(SCAS,0xae), STRING(STOS,0xaa), FIX(XLAT,0xd7,0),
    RM(LLDT,0x00,2,0), RM(LMSW,0x01,6,0), RM(LTR,0x00,3,0),
    RM(SLDT,0x00,0,0), RM(SMSW,0x01,4,0), RM(STR,0x00,1,0),
    RM(VERR,0x00,4,0), RM(VERW,0x00,5,0),
    /* x87 register forms retain their actual group/trailing opcode bits. */
    X87(FADD,0xd8,G(0),0,0), X87(FMUL,0xd8,G(1),1,0),
    X87(FSUB,0xd8,G(4),5,0), X87(FSUBR,0xd8,G(5),4,0),
    X87(FDIV,0xd8,G(6),7,0), X87(FDIVR,0xd8,G(7),6,0),
    X87(FADDP,0xda,0xc1,0,S), X87(FMULP,0xda,0xc9,1,S),
    X87(FSUBP,0xde,0xe9,5,S), X87(FSUBRP,0xde,G(4),4,S),
    X87(FDIVP,0xde,0xf9,7,S), X87(FDIVRP,0xde,G(6),6,S),
    X87_MEMORY(FIADD,0xda,0), X87_MEMORY(FICOM,0xda,2), X87_MEMORY(FICOMP,0xda,3),
    X87_MEMORY(FIDIV,0xda,6), X87_MEMORY(FIDIVR,0xda,7),
    { MC_FILD, G(0), { P(SPECIAL,0xdf,0), P(X87_MEMORY,0xdb,D) } },
    X87_MEMORY(FIMUL,0xda,1), X87_MEMORY(FISUB,0xda,4), X87_MEMORY(FISUBR,0xda,5),
    X87_MEMORY(FIST,0xdb,2),
    { MC_FISTP, G(3), { P(SPECIAL,0xdf,0), P(X87_MEMORY,0xdb,D) } },
    X87(FCOM,0xd8,G(2),2,0), X87(FCOMP,0xd8,G(3),3,0), FIX(FCOMPP,0xde,0xd9),
    X87(FXCH,0xd9,G(1),0,S),
    { MC_FLD, 0, { P(SPECIAL,0xd9,0), P(X87_MEMORY,0xd9,0) } },
    X87(FFREE,0xdd,0,0,S),
    { MC_FST, G(2), { P(SPECIAL,0xd9,0), P(X87_MEMORY,0xd9,0) } },
    { MC_FSTP, G(3), { P(SPECIAL,0xd9,0), P(X87_MEMORY,0xd9,0) } },
    MEMORY(FBLD,0xdf,4), MEMORY(FLDCW,0xd9,5), MEMORY(FLDENV,0xd9,4), MEMORY(FRSTOR,0xdd,4),
    MEMORY(FSAVE,0xdd,6), MEMORY(FBSTP,0xdf,6), MEMORY(FSTCW,0xd9,7), MEMORY(FSTENV,0xd9,6),
    { MC_FSTSW, G(7), { P(SPECIAL,0xdf,0), P(MEMORY_GROUP,0xdd,0) } },
    MEMORY(FNSTCW,0xd9,7),
    { MC_FNSTSW, G(7), { P(SPECIAL,0xdf,0), P(MEMORY_GROUP,0xdd,0) } },
    MEMORY(FNSAVE,0xdd,6), MEMORY(FNSTENV,0xd9,6),
    FIX(FNINIT,0xdb,0xe3), FIX(FNCLEX,0xdb,0xe2), FIX(FNDISI,0xdb,0xe1), FIX(FNENI,0xdb,0xe0),
    FIX(F2XM1,0xd9,0xf0), FIX(FABS,0xd9,0xe1), FIX(FCHS,0xd9,0xe0), FIX(FCLEX,0xdb,0xe2),
    FIX(FDECSTP,0xd9,0xf6), FIX(FDISI,0xdb,0xe1), FIX(FENI,0xdb,0xe0),
    FIX(FINCSTP,0xd9,0xf7), FIX(FINIT,0xdb,0xe3), EMPTY(FLDT,G(5)),
    FIX(FLDZ,0xd9,0xee), FIX(FLDPI,0xd9,0xeb), FIX(FLDL2E,0xd9,0xea), FIX(FLDL2T,0xd9,0xe9),
    FIX(FLDLG2,0xd9,0xec), FIX(FLDLN2,0xd9,0xed), FIX(FNOP,0xd9,0xd0),
    FIX(FPATAN,0xd9,0xf3), FIX(FPREM,0xd9,0xf8), FIX(FPTAN,0xd9,0xf2),
    FIX(FRNDINT,0xd9,0xfc), FIX(FSCALE,0xd9,0xfd), EMPTY(FSETPM,0xe4),
    FIX(FSQRT,0xd9,0xfa), FIX(FTST,0xd9,0xe4), FIX(FWAIT,0x9b,0), FIX(FXAM,0xd9,0xe5),
    FIX(FXTRACT,0xd9,0xf4), FIX(FYL2X,0xd9,0xf1), FIX(FYL2XP1,0xd9,0xf9),
    EMPTY(FSTTP,G(7)), FIX(FLD1,0xd9,0xe8),
    /* Fixed integer, flag and prefix instructions. */
    FIX(AAA,0x37,0), FIX(AAD,0xd5,0x0a), FIX(AAM,0xd4,0x0a), FIX(AAS,0x3f,0),
    FIX(CBW,0x66,0x98), FIX(CLC,0xf8,0), FIX(CLD,0xfc,0), FIX(CLI,0xfa,0), FIX(CLTS,0x06,0),
    FIX(CMC,0xf5,0), FIX(CWD,0x66,0x99), FIX(DAA,0x27,0), FIX(DAS,0x2f,0), FIX(HLT,0xf4,0),
    { MC_IMUL3, 0, { P(SPECIAL,0x69,0) } }, FIX(INTO,0xce,0), FIX(IRET,0xcf,0),
    FIX(LAHF,0x9f,0), FIX(LEAVE,0xc9,0), EMPTY(LJCC,0),
    { MC_NOP, 0, { P(SPECIAL,0x90,0) } },
    FIX(POPA,0x61,0), FIX(POPF,0x9d,0), FIX(PUSHA,0x60,0), FIX(PUSHF,0x9c,0), FIX(SAHF,0x9e,0),
    EMPTY(SEG,0), FIX(STC,0xf9,0), FIX(STD,0xfd,0), FIX(STI,0xfb,0), FIX(WAIT,0x9b,0),
    EMPTY(EMIT,0), FIX(REP,0xf3,0), FIX(LOCK,0xf0,0), FIX(REPNZ,0xf2,0), FIX(REPZ,0xf3,0),
    /* Pseudo saves/restores normally become POP/PUSH in the finalizer. */
    { MC_EPOP, 0, { P(SEGMENT_STACK,0,0), P(PUSH_IMMEDIATE,0,S), P(REGISTER_OPCODE,0,0), P(MODRM,0,0) } },
    { MC_EPUSH, G(6), { P(SEGMENT_STACK,0,0), P(PUSH_IMMEDIATE,0,S), P(REGISTER_OPCODE,0,0), P(MODRM,0,0) } },
    { MC_DATA, 0, { P(SPECIAL,0,0) } }, EMPTY(RPUSH,G(6)),
    { MC_SCC, 0, { P(SPECIAL,0x90,0) } },
    EXT_RM(LSS,0xb2), EXT_RM(LFS,0xb4), EXT_RM(LGS,0xb5),
    { MC_IMUL2, 0, { P(SPECIAL,0x69,0), P(MODRM,0xaf,0) } },
    EXT_RM(MOVZX,0xb6), EXT_RM(MOVZXW,0xb7), EXT_RM(MOVSX,0xbe), EXT_RM(MOVSXW,0xbf),
    EMPTY(JMP3216,0), EMPTY(JMP1632,0), EXT_RM(BSF,0xbc), EXT_RM(BSR,0xbd),
    { MC_SHLD, G(4), { P(SPECIAL,0xa4,0) } },
    { MC_SHRD, G(5), { P(SPECIAL,0xac,0) } },
    BIT_OP(BT,0xa3,4), BIT_OP(BTS,0xab,5), BIT_OP(BTR,0xb3,6), BIT_OP(BTC,0xbb,7),
    { MC_MOVSR, 0xff, { P(SPECIAL,0x20,0) } },
    X87(FUCOM,0xdd,G(4),0,0), X87(FUCOMP,0xdd,G(5),0,0), FIX(FUCOMPP,0xda,0xe9),
    FIX(FPREM1,0xd9,0xf5), FIX(FSINCOS,0xd9,0xfb), FIX(FSIN,0xd9,0xfe), FIX(FCOS,0xd9,0xff),
    { MC_BSWAP, 0xff, { P(REGISTER_OPCODE,0xc8,0) } },
    { MC_XADD, 0xff, { P(MODRM,0xc0,W) } },
    { MC_CMPXCHG, 0xff, { P(MODRM,0x06,W) } },
    FIX(INVD,0x08,0xff), FIX(WBINVD,0x09,0xff),
    { MC_INVLPG, 0xff, { P(MODRM,0x01,0) } },
    FIX(CWDE,0x98,0), FIX(CDQ,0x99,0),
    STRING(CMPS_EXPLICIT,0xa6), STRING(LODS_EXPLICIT,0xac),
    STRING(MOVS_EXPLICIT,0xa4), STRING(OUTS_EXPLICIT,0x6e), FIX(XLAT_EXPLICIT,0xd7,0),
    FIX(EFWAIT,0x9b,0), EMPTY(LAST,0)
};
#undef BIT_OP
#undef EXT_RM
#undef MEMORY
#undef X87_MEMORY
#undef X87
#undef STRING
#undef ARITH
#undef SHIFT
#undef GROUP_OP
#undef RM
#undef FIX
#undef EMPTY
#undef D
#undef S
#undef W
#undef P
#undef G

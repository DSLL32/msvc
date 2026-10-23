/* Instruction operand classes and supported processor generations.
   Recovered as logical properties, excluding the original record padding. */
#include "asm.h"
#include "control-ir.h"
#include <recon.h>

extern FrontendSymbol *translation_placeholder;
AsmInstructionProperties asm_instruction_properties[230] = {
    { -1, 0x1f }, /* _firstop, operation 0 */
    { 13, 0x1f }, /* mov, operation 1 */
    { 29, 0x3c }, /* arpl, operation 2 */
    { 7, 0x1e }, /* bound, operation 3 */
    { 19, 0x1e }, /* enter, operation 4 */
    { 25, 0x07 }, /* esc, operation 5 */
    { 9, 0x1f }, /* imul, operation 6 */
    { 26, 0x1f }, /* int, operation 7 */
    { 17, 0x1f }, /* ret, operation 8 */
    { 4, 0x1f }, /* xchg, operation 9 */
    { 28, 0x1f }, /* in, operation 10 */
    { 28, 0x1f }, /* out, operation 11 */
    { 12, 0x1f }, /* pop, operation 12 */
    { 11, 0x1f }, /* push, operation 13 */
    { 14, 0x1f }, /* call, operation 14 */
    { 16, 0x1f }, /* conditional jump, operation 15 */
    { 15, 0x1f }, /* jmp, operation 16 */
    { 7, 0x1f }, /* lds, operation 17 */
    { 7, 0x1f }, /* lea, operation 18 */
    { 7, 0x1f }, /* les, operation 19 */
    { 23, 0x3c }, /* lar, operation 20 */
    { 23, 0x3c }, /* lsl, operation 21 */
    { 27, 0x1f }, /* jcxz, operation 22 */
    { 27, 0x1f }, /* loop, operation 23 */
    { 27, 0x1f }, /* loopnz, operation 24 */
    { 27, 0x1f }, /* loopz, operation 25 */
    { 21, 0x3c }, /* lgdt, operation 26 */
    { 21, 0x3c }, /* lidt, operation 27 */
    { 21, 0x3c }, /* sgdt, operation 28 */
    { 21, 0x3c }, /* sidt, operation 29 */
    { 18, 0x1f }, /* dec, operation 30 */
    { 9, 0x1f }, /* div, operation 31 */
    { 9, 0x1f }, /* idiv, operation 32 */
    { 18, 0x1f }, /* inc, operation 33 */
    { 9, 0x1f }, /* mul, operation 34 */
    { 9, 0x1f }, /* neg, operation 35 */
    { 9, 0x1f }, /* not, operation 36 */
    { 8, 0x1f }, /* rcl, operation 37 */
    { 8, 0x1f }, /* rcr, operation 38 */
    { 8, 0x1f }, /* rol, operation 39 */
    { 8, 0x1f }, /* ror, operation 40 */
    { 8, 0x1f }, /* sar, operation 41 */
    { 8, 0x1f }, /* shl, operation 42 */
    { 8, 0x1f }, /* shr, operation 43 */
    { 6, 0x1f }, /* adc, operation 44 */
    { 6, 0x1f }, /* add, operation 45 */
    { 6, 0x1f }, /* and, operation 46 */
    { 6, 0x1f }, /* cmp, operation 47 */
    { 6, 0x1f }, /* or, operation 48 */
    { 6, 0x1f }, /* sbb, operation 49 */
    { 6, 0x1f }, /* sub, operation 50 */
    { 5, 0x1f }, /* test, operation 51 */
    { 6, 0x1f }, /* xor, operation 52 */
    { 10, 0x1f }, /* cmps, operation 53 */
    { 10, 0x1e }, /* ins, operation 54 */
    { 10, 0x1f }, /* lods, operation 55 */
    { 10, 0x1f }, /* movs, operation 56 */
    { 10, 0x1e }, /* outs, operation 57 */
    { 10, 0x1f }, /* scas, operation 58 */
    { 10, 0x1f }, /* stos, operation 59 */
    { 2, 0x1f }, /* xlat, operation 60 */
    { 22, 0x3c }, /* lldt, operation 61 */
    { 21, 0x3c }, /* lmsw, operation 62 */
    { 22, 0x3c }, /* ltr, operation 63 */
    { 22, 0x3c }, /* sldt, operation 64 */
    { 21, 0x3c }, /* smsw, operation 65 */
    { 22, 0x3c }, /* str, operation 66 */
    { 22, 0x3c }, /* verr, operation 67 */
    { 22, 0x3c }, /* verw, operation 68 */
    { 43, 0x1f }, /* fadd, operation 69 */
    { 43, 0x1f }, /* fmul, operation 70 */
    { 43, 0x1f }, /* fsub, operation 71 */
    { 43, 0x1f }, /* fsubr, operation 72 */
    { 43, 0x1f }, /* fdiv, operation 73 */
    { 43, 0x1f }, /* fdivr, operation 74 */
    { 44, 0x1f }, /* faddp, operation 75 */
    { 44, 0x1f }, /* fmulp, operation 76 */
    { 44, 0x1f }, /* fsubp, operation 77 */
    { 44, 0x1f }, /* fsubrp, operation 78 */
    { 44, 0x1f }, /* fdivp, operation 79 */
    { 44, 0x1f }, /* fdivrp, operation 80 */
    { 57, 0x1f }, /* fiadd, operation 81 */
    { 57, 0x1f }, /* ficom, operation 82 */
    { 57, 0x1f }, /* ficomp, operation 83 */
    { 57, 0x1f }, /* fidiv, operation 84 */
    { 57, 0x1f }, /* fidivr, operation 85 */
    { 58, 0x1f }, /* fild, operation 86 */
    { 57, 0x1f }, /* fimul, operation 87 */
    { 57, 0x1f }, /* fisub, operation 88 */
    { 57, 0x1f }, /* fisubr, operation 89 */
    { 58, 0x1f }, /* fist, operation 90 */
    { 58, 0x1f }, /* fistp, operation 91 */
    { 43, 0x1f }, /* fcom, operation 92 */
    { 43, 0x1f }, /* fcomp, operation 93 */
    { 36, 0x1f }, /* fcompp, operation 94 */
    { 40, 0x1f }, /* fxch, operation 95 */
    { 41, 0x1f }, /* fld, operation 96 */
    { 40, 0x1f }, /* ffree, operation 97 */
    { 41, 0x1f }, /* fst, operation 98 */
    { 41, 0x1f }, /* fstp, operation 99 */
    { 59, 0x1f }, /* fbld, operation 100 */
    { 38, 0x1f }, /* fldcw, operation 101 */
    { 38, 0x1f }, /* fldenv, operation 102 */
    { 38, 0x1f }, /* frstor, operation 103 */
    { 38, 0x1f }, /* fsave, operation 104 */
    { 59, 0x1f }, /* fbstp, operation 105 */
    { 38, 0x1f }, /* fstcw, operation 106 */
    { 38, 0x1f }, /* fstenv, operation 107 */
    { 38, 0x1f }, /* fstsw, operation 108 */
    { 39, 0x1f }, /* fnstcw, operation 109 */
    { 39, 0x1f }, /* fnstsw, operation 110 */
    { 39, 0x1f }, /* fnsave, operation 111 */
    { 39, 0x1f }, /* fnstenv, operation 112 */
    { 35, 0x1f }, /* fninit, operation 113 */
    { 35, 0x1f }, /* fnclex, operation 114 */
    { 35, 0x1f }, /* fndisi, operation 115 */
    { 35, 0x1f }, /* fneni, operation 116 */
    { 33, 0x1f }, /* f2xm1, operation 117 */
    { 32, 0x1f }, /* fabs, operation 118 */
    { 32, 0x1f }, /* fchs, operation 119 */
    { 34, 0x1f }, /* fclex, operation 120 */
    { 33, 0x1f }, /* fdecstp, operation 121 */
    { 34, 0x1f }, /* fdisi, operation 122 */
    { 34, 0x1f }, /* feni, operation 123 */
    { 33, 0x1f }, /* fincstp, operation 124 */
    { 34, 0x1f }, /* finit, operation 125 */
    { -1, 0x1f }, /* _fldt, operation 126 */
    { 31, 0x1f }, /* fldz, operation 127 */
    { 31, 0x1f }, /* fldpi, operation 128 */
    { 31, 0x1f }, /* fldl2e, operation 129 */
    { 31, 0x1f }, /* fldl2t, operation 130 */
    { 31, 0x1f }, /* fldlg2, operation 131 */
    { 32, 0x1f }, /* fldln2, operation 132 */
    { 33, 0x1f }, /* fnop, operation 133 */
    { 33, 0x1f }, /* fpatan, operation 134 */
    { 32, 0x1f }, /* fprem, operation 135 */
    { 33, 0x1f }, /* fptan, operation 136 */
    { 32, 0x1f }, /* frndint, operation 137 */
    { 32, 0x1f }, /* fscale, operation 138 */
    { 34, 0x1c }, /* fsetpm, operation 139 */
    { 32, 0x1f }, /* fsqrt, operation 140 */
    { 31, 0x1f }, /* ftst, operation 141 */
    { 3, 0x1f }, /* fwait, operation 142 */
    { 31, 0x1f }, /* fxam, operation 143 */
    { 32, 0x1f }, /* fxtract, operation 144 */
    { 33, 0x1f }, /* fyl2x, operation 145 */
    { 33, 0x1f }, /* fyl2xp1, operation 146 */
    { 36, 0x1f }, /* _fsttp, operation 147 */
    { 31, 0x1f }, /* fld1, operation 148 */
    { 2, 0x1f }, /* aaa, operation 149 */
    { 24, 0x1f }, /* aad, operation 150 */
    { 24, 0x1f }, /* aam, operation 151 */
    { 2, 0x1f }, /* aas, operation 152 */
    { 0, 0x1f }, /* cbw, operation 153 */
    { 3, 0x1f }, /* clc, operation 154 */
    { 1, 0x1f }, /* cld, operation 155 */
    { 3, 0x1f }, /* cli, operation 156 */
    { 24, 0x3c }, /* clts, operation 157 */
    { 3, 0x1f }, /* cmc, operation 158 */
    { 0, 0x1f }, /* cwd, operation 159 */
    { 2, 0x1f }, /* daa, operation 160 */
    { 2, 0x1f }, /* das, operation 161 */
    { 2, 0x1f }, /* hlt, operation 162 */
    { 20, 0x1e }, /* _imul3, operation 163 */
    { 2, 0x1f }, /* into, operation 164 */
    { 1, 0x1f }, /* iret, operation 165 */
    { 1, 0x1f }, /* lahf, operation 166 */
    { 0, 0x1e }, /* leave, operation 167 */
    { -1, 0x1f }, /* _ljcc, operation 168 */
    { 0, 0x1f }, /* nop, operation 169 */
    { 1, 0x1e }, /* popa, operation 170 */
    { 1, 0x1f }, /* popf, operation 171 */
    { 1, 0x1e }, /* pusha, operation 172 */
    { 1, 0x1f }, /* pushf, operation 173 */
    { 1, 0x1f }, /* sahf, operation 174 */
    { -1, 0x1f }, /* _seg, operation 175 */
    { 3, 0x1f }, /* stc, operation 176 */
    { 3, 0x1f }, /* std, operation 177 */
    { 3, 0x1f }, /* sti, operation 178 */
    { 3, 0x1f }, /* wait, operation 179 */
    { 26, 0x1f }, /* _emit, operation 180 */
    { 0, 0x1f }, /* rep, operation 181 */
    { 3, 0x1f }, /* lock, operation 182 */
    { 0, 0x1f }, /* repnz, operation 183 */
    { 0, 0x1f }, /* repz, operation 184 */
    { -1, 0x1f }, /* _epop, operation 185 */
    { -1, 0x1f }, /* _epush, operation 186 */
    { 47, 0x1f }, /* _data, operation 187 */
    { -1, 0x1f }, /* _rpush, operation 188 */
    { 30, 0x18 }, /* set condition, operation 189 */
    { 49, 0x18 }, /* lss, operation 190 */
    { 49, 0x18 }, /* lfs, operation 191 */
    { 49, 0x18 }, /* lgs, operation 192 */
    { 23, 0x18 }, /* _imul2, operation 193 */
    { 51, 0x18 }, /* movzx, operation 194 */
    { 51, 0x18 }, /* _movzxw, operation 195 */
    { 51, 0x18 }, /* movsx, operation 196 */
    { 51, 0x18 }, /* _movsxw, operation 197 */
    { 15, 0x18 }, /* _jmp3216, operation 198 */
    { 15, 0x18 }, /* _jmp1632, operation 199 */
    { 23, 0x18 }, /* bsf, operation 200 */
    { 23, 0x18 }, /* bsr, operation 201 */
    { 50, 0x18 }, /* shld, operation 202 */
    { 50, 0x18 }, /* shrd, operation 203 */
    { 52, 0x18 }, /* bt, operation 204 */
    { 52, 0x18 }, /* bts, operation 205 */
    { 52, 0x18 }, /* btr, operation 206 */
    { 52, 0x18 }, /* btc, operation 207 */
    { 53, 0x18 }, /* _movsr, operation 208 */
    { 43, 0x18 }, /* fucom, operation 209 */
    { 43, 0x18 }, /* fucomp, operation 210 */
    { 36, 0x18 }, /* fucompp, operation 211 */
    { 45, 0x18 }, /* fprem1, operation 212 */
    { 45, 0x18 }, /* fsincos, operation 213 */
    { 45, 0x18 }, /* fsin, operation 214 */
    { 45, 0x18 }, /* fcos, operation 215 */
    { 54, 0x10 }, /* bswap, operation 216 */
    { 55, 0x10 }, /* xadd, operation 217 */
    { 55, 0x10 }, /* cmpxchg, operation 218 */
    { 24, 0x10 }, /* invd, operation 219 */
    { 24, 0x10 }, /* wbinvd, operation 220 */
    { 56, 0x10 }, /* invlpg, operation 221 */
    { 0, 0x1f }, /* cwde, operation 222 */
    { 0, 0x1f }, /* cdq, operation 223 */
    { 10, 0x1f }, /* cmps, operation 224 */
    { 10, 0x1f }, /* lods, operation 225 */
    { 10, 0x1f }, /* movs, operation 226 */
    { 10, 0x1e }, /* outs, operation 227 */
    { 2, 0x1f }, /* xlat, operation 228 */
    { -1, 0x1f }, /* _lastop, operation 229 */
};
int asm_operand_class; /* original 0x5b014 */

/* Original 0x4de19..0x4de76. Processor bits correspond to target_cpu_mask;
   selecting an instruction starts again with operand zero. */
void __stdcall select_asm_opcode(short opcode)
{
    if (!(target_cpu_mask & asm_instruction_properties[opcode].processors))
        report_syntax_error(416);
    asm_operation = opcode;
    asm_operand_class = asm_instruction_properties[opcode].operand_class;
    asm_operand = asm_operands;
    asm_operands[0].address = asm_addresses;
}

/* Original 0x4fdb1..0x4fdfb. Invalid forms produce diagnostics without an
   instruction record. Successful operands are normalized before serialization. */
void emit_asm_instruction(void)
{
    if (!validate_asm_operand_count()) report_syntax_error(414);
    else {
        if (asm_operand_count > 0 && !validate_asm_operands()) {
            report_syntax_error(415);
            return;
        }
        prepare_asm_instruction();
        serialize_asm_instruction();
        record_line_information(0x16);
    }
}

/* Original 0x4ec52..0x4efdc; class dispatch 0x4efdd..0x4f0cc.
   Select backend instruction variants, supply implicit operands and establish
   the two parallel serialization views without moving symbol references. */
unsigned char asm_encoded_opcode, asm_instruction_size, asm_instruction_modifier;
AsmAddress *asm_output_addresses[3]; /* original 0x5b068 */
AsmOperand *asm_output_operands[3]; /* original 0x5b098 */

void prepare_asm_instruction(void)
{
    unsigned swap = 0;
    unsigned char size, segment;
    short operation;
    FrontendSymbol *symbol;
    asm_instruction_size = (unsigned char)choose_asm_instruction_size();
    size = asm_instruction_size;
    operation = asm_operation;
    switch (asm_operand_class) {
    case 0:
        if (asm_operation == 184) operation = asm_operation = 181;
        break;
    case 2:
        if ((asm_operands[0].address->flags & 63) == 28)
            asm_operands[0].address->flags &= 0xc0;
        /* XLAT and string operations share their implicit-operand rule. */
    case 10:
        operation = asm_operation;
        if (asm_operand_count < 1) break;
        switch (asm_operation) {
        case 53: operation = 224; break;
        case 55: operation = 225; break;
        case 56: operation = 226; break;
        case 57: operation = 227; break;
        case 60: operation = 228; break;
        }
        segment = asm_operands[0].address->flags;
        if (asm_operand_count >= 2 && !(segment & 63))
            segment = asm_operands[1].address->flags;
        if (!(segment & 63) || asm_operation == operation) {
            asm_operand_count = 0;
            operation = asm_operation;
        }
        break;
    case 16:
        asm_instruction_modifier = asm_opcode_aliases[asm_opcode - 230].modifier;
        /* Conditional branches carry their condition outside the size field. */
    case 27:
        size = 0;
    case 14: case 15:
        symbol = asm_operands[0].address->symbol;
        if (symbol && !(asm_operands[0].address->address_flags & 0x10) && !size) {
            if (!asm_operands[0].address->displacement || (symbol->flags & 1)) {
                if ((symbol->kind & 31) == 6 || (symbol->kind & 31) == 7) {
                    compiler_warning(3,414,symbol->name);
                    asm_instruction_size = 2;
                    operation = asm_operation;
                }
            } else {
                report_syntax_error(405);
                operation = asm_operation;
            }
        }
        break;
    case 17:
        if (!asm_operand_count) {
            asm_operand_count = 1;
            asm_operands[0].address->address_flags = 2;
            asm_operands[0].address->displacement = 0;
            operation = asm_operation;
        }
        break;
    case 18:
        asm_operand_count = 2;
        asm_operand = reset_asm_operand(1);
        asm_operands[1].address->address_flags = 2;
        asm_operands[1].address->displacement = 1;
        operation = asm_operation;
        break;
    case 20:
        if (asm_operand_count == 2) {
            asm_operand_count = 3;
            asm_operands[2] = asm_operands[1];
            asm_operands[2].address = &asm_addresses[2];
            asm_addresses[2] = *asm_operands[1].address;
            asm_operands[1] = asm_operands[0];
            asm_operands[1].address = &asm_addresses[1];
            asm_addresses[1] = *asm_operands[0].address;
        }
        break;
    case 28:
        swap = asm_operation == 10;
        break;
    case 30:
        asm_instruction_modifier = asm_opcode_aliases[asm_opcode - 230].modifier;
        break;
    case 31: case 32: case 33: case 34: case 35: case 36: case 37:
    case 40: case 41: case 43: case 44:
        asm_instruction_size |= 0x20;
    case 38: case 39: case 57: case 58: case 59:
        swap = normalize_asm_x87_operands();
        operation = asm_operation;
        break;
    case 50:
        asm_operand_count = 2;
        if (ASM_OPERAND_CATEGORY(asm_operands[2].address) == ASM_IMMEDIATE) {
            asm_operands[1].address->displacement = asm_operands[2].address->displacement;
            asm_operands[1].address->address_flags |= 2;
            operation = asm_operation;
        }
        break;
    case 51:
        if (asm_operands[1].size == 1) operation = asm_operation == 196 ? 197 : 195;
        break;
    }
    asm_operation = operation;
    normalize_asm_addresses();
    asm_output_addresses[0] = &asm_addresses[0];
    asm_output_addresses[1] = &asm_addresses[1];
    asm_output_operands[0] = &asm_operands[0];
    asm_output_operands[1] = &asm_operands[1];
    asm_output_operands[2] = &asm_operands[2];
    asm_output_addresses[2] = &asm_addresses[2];
    if (swap) {
        swap_asm_pointer_slots((void **)&asm_output_addresses[1],(void **)&asm_output_addresses[0]);
        swap_asm_pointer_slots((void **)&asm_operands[0].address,(void **)&asm_operands[1].address);
        swap_asm_pointer_slots((void **)&asm_output_operands[0],(void **)&asm_output_operands[1]);
    }
    asm_encoded_opcode = (unsigned char)asm_operation;
}

/* Original 0x4e657..0x4e97b. Callers consume AL, the operand-size code.
   Verify explicit qualifiers, infer defaults and diagnose unsupported sizes. */
unsigned char choose_asm_instruction_size(void)
{
    unsigned char size = resolve_asm_operand_size(), expected;
    int incompatible = 0;
    unsigned short rule;
    if (asm_operand_class < 28) {
        switch (asm_operand_class) {
        case 27:
            size = asm_operation == 22 ? (asm_opcode == 22 ? 1 : 2) : 0;
            break;
        case 7:
            if (asm_operation != 18) {
                expected = size == 1 ? 2 : (asm_operation == 3 ? 3 : 5);
                incompatible = !asm_operand_accepts_size(&asm_operands[1],expected);
            }
            break;
        case 15:
            if (ASM_OPERAND_CATEGORY(asm_operands[0].address) == ASM_MEMORY && (size == 0 || size == 15)) {
                incompatible = 1;
                size = 1;
            }
            break;
        case 21:
            size = asm_operation == 62 || asm_operation == 65 ? 1 : 5;
            incompatible = !asm_operand_accepts_size(&asm_operands[0],size);
            break;
        case 23:
            incompatible = !asm_operand_accepts_size(&asm_operands[1],size);
            break;
        }
    } else if (asm_operand_class == 41) {
        if (asm_operation == 98 && ASM_OPERAND_CATEGORY(asm_operands[0].address) == ASM_MEMORY) {
            size = asm_operands[0].size;
            if (size == 15) size = 3;
            if (size == 4) {
                size = 3;
                incompatible = 1;
            }
        }
    } else if (asm_operand_class == 43) {
        if (ASM_OPERAND_CATEGORY(asm_operands[0].address) == ASM_MEMORY) {
            size = asm_operands[0].size;
            if (size == 15) size = 3;
            incompatible = !asm_operand_has_type_flags(&asm_operands[0],0x100) ||
                (size != 2 && size != 3 && size != 4);
        }
    } else if (asm_operand_class >= 57 && asm_operand_class <= 58) {
        size = asm_operands[0].size;
        if (size == 15) size = 1;
        incompatible = !asm_operand_has_type_flags(&asm_operands[0],0x40) ||
            (size != 1 && size != 2 && (size != 3 || asm_operand_class != 58));
    }
    if (incompatible) compiler_warning(1,410);
    rule = asm_operand_class_rules[asm_operand_class];
    if (size != 15) {
        if ((1U << ((size + 8) & 31)) & rule) return size;
        compiler_warning(1,409);
    }
    size = 0;
    while (!((1U << ((size + 8) & 31)) & rule & 0x3f00)) ++size;
    if (!asm_operand_class) {
        if (asm_opcode == 153) size = 0;
        else if (asm_opcode == 159 || asm_opcode == 284) size = 1;
        else if (asm_opcode == 283) size = 2;
    } else if (asm_operand_class == 1) {
        if (asm_opcode == 165 || (asm_opcode > 169 && asm_opcode < 174)) size = 1;
    } else if (asm_operand_class > 37 && asm_operand_class < 40) size = 4;
    return size;
}

/* Original 0x4e4f8..0x4e656. Explicit mnemonic suffixes outrank operand
   types; otherwise prefer the operand whose type controls this instruction. */
unsigned char resolve_asm_operand_size(void)
{
    unsigned char size;
    if (asm_opcode != asm_operation && asm_opcode >= 262 && asm_opcode <= 293)
        return asm_opcode_aliases[asm_opcode - 230].modifier;
    if (asm_operand_count < 1) return 15;
    if (asm_operand_count == 2 && asm_operands[0].size != asm_operands[1].size &&
        (asm_operands[0].size | asm_operands[1].size) != 15 &&
        (asm_operand_class == 13 || asm_operand_class == 6 || asm_operand_class == 4 ||
         asm_operand_class == 5 || asm_operand_class == 55) &&
        asm_operands[1].address->symbol != translation_placeholder)
        report_syntax_error(443);
    size = asm_operands[1].size;
    if (asm_operation != 11 && asm_operation != 57) {
        size = asm_operands[0].size;
        if (asm_operation == 53 || asm_operation == 56) {
            if (size == 15) size = asm_operands[1].size;
        } else if (asm_operand_count != 1 &&
                   ASM_OPERAND_CATEGORY(asm_operands[0].address) != ASM_REGISTER &&
                   ASM_OPERAND_CATEGORY(asm_operands[1].address) != ASM_IMMEDIATE &&
                   asm_operand_class != 8 && asm_operation != 54)
            size = asm_operands[1].size;
    }
    return size;
}

/* Original0x4e991..0x4eafc, opcode dispatch0x4eafd..0x4eb14.
   Supply implicit ST operands and choose popping/reversed operation forms.
   The returned flag asks the emitter to exchange the operand slots. */
unsigned normalize_asm_x87_operands(void)
{
    short operation = asm_operation;
    int index;
    switch (operation) {
    case 69: operation = 75; break;
    case 70: operation = 76; break;
    case 71: operation = 77; break;
    case 72: operation = 78; break;
    case 73: operation = 79; break;
    case 74: operation = 80; break;
    }
    if (asm_operand_class == 40) {
        if (asm_operation == 97) return 0;
        if (!asm_operand_count) {
            asm_operand_count = 1;
            asm_operand = reset_asm_operand(0);
            make_asm_stack_register(31,1);
            return 0;
        }
    } else if (asm_operand_class == 43) {
        if (!asm_operand_count) {
            asm_operand_count = 1;
            asm_operand = reset_asm_operand(0);
            make_asm_stack_register(31,1);
            index = asm_operand_count++;
            asm_operand = reset_asm_operand(index);
            make_asm_stack_register(31,0);
            if (asm_operand_class != 43) operation = asm_operation;
            asm_operation = operation;
            return operation == 92 || operation == 93 || operation == 209 || operation == 210;
        }
    } else if (asm_operand_class == 57) {
        asm_operand_count = 2;
        asm_operand = reset_asm_operand(1);
        make_asm_stack_register(31,0);
        return 1;
    } else return 0;
    if (asm_operand_count == 1 && asm_operand_class != 40) {
        asm_operand_count = 2;
        asm_operand = reset_asm_operand(1);
        make_asm_stack_register(31,0);
        return 1;
    }
    return 0;
}

/* Original 0x4f62b..0x4f64e, RET8. The encoder compares signed
   magnitudes against an x86-masked shift count. Unsigned subtraction retains
   the original INT_MIN bit pattern without a signed-negation overflow. */
int __stdcall fits_asm_integer(unsigned char bits, unsigned value)
{
    int sign = (int)value >> 31;
    unsigned magnitude = (value ^ (unsigned)sign) - (unsigned)sign;
    unsigned limit = 1U << (bits & 31);
    return (int)magnitude < (int)limit;
}

/* Original 0x4e449..0x4e4f8, RET8. ALIGN and EVEN write the shared
   alignment-control word; invalid ALIGN values retain its previous value. */
void __stdcall set_asm_directive(short directive, unsigned value)
{
    if (directive == 328) {
        if (asm_member_mode) compiler_warning(3, 404);
        if ((int)value < 2 || (int)value > 16 || (value & (value - 1)))
            report_syntax_error(413, interned_name);
        else asm_code_alignment = (unsigned short)value;
    } else if (directive == 329) {
        if (asm_member_mode) compiler_warning(3, 404);
        if (asm_operand_count) compiler_warning(1, 406);
        asm_code_alignment = 2;
    } else {
        asm_member_mode = 0;
        return;
    }
    record_line_information(24);
    asm_member_mode = 0;
}

/* Original 0x4defc..0x4df51, RET4. The grammar supplies SHORT, NEAR
   or FAR (IDs52..54) and records whether a PTR size was explicit. */
void __stdcall set_asm_address_operator(short operation)
{
    asm_operand->size = asm_operand_size_codes[operation - 46];
    asm_operand->flags |= 1;
    if ((operation == 52) == asm_explicit_size)
        compiler_warning(1, asm_explicit_size ? 410 : 411);
}

/* Original 0x4f64e..0x4f689, RET8. Explicit operand sizes and
   unbound addresses need no inferred symbol type; otherwise require an
   unshaped numeric type in the requested class, wider than one byte. */
int __stdcall asm_operand_has_type_flags(AsmOperand *operand, unsigned short flags)
{
    CType *type;
    if (!(operand->flags & 1) && operand->address->symbol) {
        type = operand->address->symbol->type;
        if (type->related || !(type->flags & flags) || (type->flags & 15) < 2)
            return 0;
    }
    return 1;
}

/* Original 0x4e97c..0x4e991, RET8. Keep the three x87 operand
   serialization views in step through the original separate swap entry. */
void __stdcall swap_asm_pointer_slots(void **first, void **second)
{
    void *temporary = *first;
    *first = *second;
    *second = temporary;
}

/* Original 0x4f689..0x4f6ab, RET8. Only an explicit PTR size can
   conflict with this instruction's expected operand width. */
int __stdcall asm_operand_accepts_size(AsmOperand *operand, unsigned char size)
{
    if (operand->flags & 1) return operand->size == size;
    return 1;
}

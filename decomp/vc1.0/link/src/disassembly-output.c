/* Instruction display columns and shared mutable output storage. */
#include "disassembler.h"
#include <recon.h>
#pragma intrinsic(memset,strlen,strcpy)

char disassembly_lines[768];

/* Original [0004e980,0004eb91), including internal alignment. */
char *format_instruction(unsigned char format,DisassembledInstruction *state)
{
    unsigned long rows=0,address_width=0,bytes_width=0;
    unsigned long bytes_column,text_column,row,index,length,unit;
    char text[256],*address,*line,*end,*output;
    memset(disassembly_lines,' ',sizeof(disassembly_lines));
    if (format&1) {
        address=format_instruction_address(state->address);
        length=strlen(address);
        address_width=(length+5)&~3UL;
        strncpy(disassembly_lines+4,address,length);
        disassembly_lines[4+length]=':';
        rows=1;
    }
    bytes_column=4+address_width;
    if (format&2) {
        unit=format_instruction_bytes(state,sizeof(text),text);
        row=strlen(text)/unit;
        bytes_width=(unit+3)&~3UL;
        for (index=0;index<row;++index)
            strncpy(disassembly_lines+bytes_column+index*256,text+index*unit,unit);
        if (row>rows) rows=row;
    }
    text_column=bytes_column+bytes_width;
    if (format&4) {
        format_instruction_text(state,sizeof(text),text);
        strncpy(disassembly_lines+text_column,text,strlen(text));
        if (rows<2) rows=1;
    }
    if (!rows) disassembly_lines[0]=0;
    else {
        for (index=0;index<rows;++index) {
            line=disassembly_lines+index*256;
            end=line+255;
            while (line<end && *end==' ') --end;
            if (*end!=' ') ++end;
            if (line==disassembly_lines) output=end;
            else {
                *output++='\n';
                strncpy(output,line,end-line);
                output+=end-line;
            }
            *output=0;
        }
    }
    return disassembly_lines;
}

char disassembly_address[9];

/* Original [0004eba0,0004ebc4). */
char *format_instruction_address(unsigned long address)
{
    sprintf(disassembly_address,"%.8x",address);
    disassembly_address[8]=0;
    return disassembly_address;
}

/* Original [0004e870,0004e8e3). Width is unused; display rows hold six bytes. */
unsigned long format_instruction_bytes(DisassembledInstruction *state,
    unsigned long width,char *text)
{
    unsigned long index,padding;
    char *cursor=text;
    for (index=0;index<state->length;++index)
        cursor+=sprintf(cursor,"%02X ",state->bytes[index]);
    padding=((6-state->length%6)%6)*3;
    memset(cursor,' ',padding);
    cursor[padding]=0;
    return 18;
}

/* Shared by the prefix and instruction-name columns. Original [791d0,795f0). */
const char *const disassembly_mnemonics[264]={
    "", /* 0 */
    "aaa", /* 1 */
    "aad", /* 2 */
    "aam", /* 3 */
    "aas", /* 4 */
    "adc", /* 5 */
    "add", /* 6 */
    "and", /* 7 */
    "arpl", /* 8 */
    "bound", /* 9 */
    "bsf", /* 10 */
    "bsr", /* 11 */
    "bswap", /* 12 */
    "bt", /* 13 */
    "btc", /* 14 */
    "btr", /* 15 */
    "bts", /* 16 */
    "call", /* 17 */
    "cbw", /* 18 */
    "cwde", /* 19 */
    "clc", /* 20 */
    "cld", /* 21 */
    "cli", /* 22 */
    "clts", /* 23 */
    "cmc", /* 24 */
    "cmp", /* 25 */
    "cmps", /* 26 */
    "cmpxchg", /* 27 */
    "cwd", /* 28 */
    "cdq", /* 29 */
    "daa", /* 30 */
    "das", /* 31 */
    "dec", /* 32 */
    "div", /* 33 */
    "enter", /* 34 */
    "f2xm1", /* 35 */
    "fabs", /* 36 */
    "fadd", /* 37 */
    "faddp", /* 38 */
    "fiadd", /* 39 */
    "fbld", /* 40 */
    "fchs", /* 41 */
    "fnclex", /* 42 */
    "fcom", /* 43 */
    "fcomp", /* 44 */
    "fcompp", /* 45 */
    "ficom", /* 46 */
    "ficomp", /* 47 */
    "fcos", /* 48 */
    "fdecstp", /* 49 */
    "fndisi", /* 50 */
    "fdiv", /* 51 */
    "fdivp", /* 52 */
    "fidiv", /* 53 */
    "fdivr", /* 54 */
    "fdivrp", /* 55 */
    "fidivr", /* 56 */
    "fneni", /* 57 */
    "ffree", /* 58 */
    "fincstp", /* 59 */
    "fninit", /* 60 */
    "fld", /* 61 */
    "fild", /* 62 */
    "fld1", /* 63 */
    "fldz", /* 64 */
    "fldpi", /* 65 */
    "fldl2e", /* 66 */
    "fldl2t", /* 67 */
    "fldlg2", /* 68 */
    "fldln2", /* 69 */
    "fldcw", /* 70 */
    "fldenv", /* 71 */
    "fldenvw", /* 72 */
    "fldenvd", /* 73 */
    "fmul", /* 74 */
    "fmulp", /* 75 */
    "fimul", /* 76 */
    "fnop", /* 77 */
    "fpatan", /* 78 */
    "fprem", /* 79 */
    "fprem1", /* 80 */
    "fptan", /* 81 */
    "frndint", /* 82 */
    "frstor", /* 83 */
    "frstorw", /* 84 */
    "frstord", /* 85 */
    "fnsave", /* 86 */
    "fnsavew", /* 87 */
    "fnsaved", /* 88 */
    "fscale", /* 89 */
    "fsetpm", /* 90 */
    "fsin", /* 91 */
    "fsincos", /* 92 */
    "fsqrt", /* 93 */
    "fst", /* 94 */
    "fstp", /* 95 */
    "fist", /* 96 */
    "fistp", /* 97 */
    "fbstp", /* 98 */
    "fnstcw", /* 99 */
    "fnstenv", /* 100 */
    "fnstenvw", /* 101 */
    "fnstenvd", /* 102 */
    "fnstsw", /* 103 */
    "fsub", /* 104 */
    "fsubp", /* 105 */
    "fisub", /* 106 */
    "fsubr", /* 107 */
    "fsubrp", /* 108 */
    "fisubr", /* 109 */
    "ftst", /* 110 */
    "fucom", /* 111 */
    "fucomp", /* 112 */
    "fucompp", /* 113 */
    "fwait", /* 114 */
    "fxam", /* 115 */
    "fxch", /* 116 */
    "fxtract", /* 117 */
    "fyl2x", /* 118 */
    "fyl2xp1", /* 119 */
    "hlt", /* 120 */
    "ibts", /* 121 */
    "idiv", /* 122 */
    "imul", /* 123 */
    "in", /* 124 */
    "inc", /* 125 */
    "ins", /* 126 */
    "int", /* 127 */
    "into", /* 128 */
    "intd", /* 129 */
    "invd", /* 130 */
    "invlpg", /* 131 */
    "iret", /* 132 */
    "iretd", /* 133 */
    "jb", /* 134 */
    "jae", /* 135 */
    "jbe", /* 136 */
    "ja", /* 137 */
    "je", /* 138 */
    "jne", /* 139 */
    "jl", /* 140 */
    "jge", /* 141 */
    "jle", /* 142 */
    "jg", /* 143 */
    "js", /* 144 */
    "jns", /* 145 */
    "jc", /* 146 */
    "jnc", /* 147 */
    "jo", /* 148 */
    "jno", /* 149 */
    "jp", /* 150 */
    "jnp", /* 151 */
    "jcxz", /* 152 */
    "jecxz", /* 153 */
    "jmp", /* 154 */
    "lahf", /* 155 */
    "lar", /* 156 */
    "lds", /* 157 */
    "les", /* 158 */
    "lfs", /* 159 */
    "lgs", /* 160 */
    "loadall", /* 161 */
    "lss", /* 162 */
    "lea", /* 163 */
    "leave", /* 164 */
    "lgdt", /* 165 */
    "lidt", /* 166 */
    "lldt", /* 167 */
    "lmsw", /* 168 */
    "lock", /* 169 */
    "lods", /* 170 */
    "loop", /* 171 */
    "loopw", /* 172 */
    "loopd", /* 173 */
    "loope", /* 174 */
    "loopew", /* 175 */
    "looped", /* 176 */
    "loopne", /* 177 */
    "loopnew", /* 178 */
    "loopned", /* 179 */
    "lsl", /* 180 */
    "ltr", /* 181 */
    "mov", /* 182 */
    "movs", /* 183 */
    "movsx", /* 184 */
    "movzx", /* 185 */
    "mul", /* 186 */
    "neg", /* 187 */
    "nop", /* 188 */
    "not", /* 189 */
    "or", /* 190 */
    "out", /* 191 */
    "outs", /* 192 */
    "pop", /* 193 */
    "popa", /* 194 */
    "popaw", /* 195 */
    "popad", /* 196 */
    "popf", /* 197 */
    "popfd", /* 198 */
    "push", /* 199 */
    "pushw", /* 200 */
    "pushd", /* 201 */
    "pusha", /* 202 */
    "pushaw", /* 203 */
    "pushad", /* 204 */
    "pushf", /* 205 */
    "pushfd", /* 206 */
    "rcl", /* 207 */
    "rcr", /* 208 */
    "rol", /* 209 */
    "ror", /* 210 */
    "rep", /* 211 */
    "repz", /* 212 */
    "repnz", /* 213 */
    "ret", /* 214 */
    "retn", /* 215 */
    "retf", /* 216 */
    "sahf", /* 217 */
    "sbb", /* 218 */
    "scas", /* 219 */
    "setb", /* 220 */
    "setae", /* 221 */
    "setbe", /* 222 */
    "seta", /* 223 */
    "sete", /* 224 */
    "setne", /* 225 */
    "setl", /* 226 */
    "setge", /* 227 */
    "setle", /* 228 */
    "setg", /* 229 */
    "sets", /* 230 */
    "setns", /* 231 */
    "setc", /* 232 */
    "setnc", /* 233 */
    "seto", /* 234 */
    "setno", /* 235 */
    "setp", /* 236 */
    "setnp", /* 237 */
    "sgdt", /* 238 */
    "sidt", /* 239 */
    "sldt", /* 240 */
    "shl", /* 241 */
    "shr", /* 242 */
    "sal", /* 243 */
    "sar", /* 244 */
    "shld", /* 245 */
    "shrd", /* 246 */
    "smsw", /* 247 */
    "stc", /* 248 */
    "std", /* 249 */
    "sti", /* 250 */
    "stos", /* 251 */
    "str", /* 252 */
    "sub", /* 253 */
    "test", /* 254 */
    "verr", /* 255 */
    "verw", /* 256 */
    "wait", /* 257 */
    "wbinvd", /* 258 */
    "xadd", /* 259 */
    "xbts", /* 260 */
    "xchg", /* 261 */
    "xlat", /* 262 */
    "xor", /* 263 */
};

/* Original [0004e6a0,0004e869). */
char *format_instruction_text(DisassembledInstruction *state,unsigned long width,char *text)
{
    char *cursor,*limit,*name,*operand_end,operand_text[128];
    int length,count,padding,remaining,comma=0;
    unsigned long operand;
    if (!width || !text) return NULL;
    cursor=text;
    limit=text+width-1;
    *limit=0;
    if (state->prefix) {
        name=(char *)disassembly_mnemonics[state->prefix];
        length=strlen(name);
        count=limit-cursor;
        if (count>length) count=length;
        if (count>0) {
            strncpy(cursor,name,count);
            cursor+=count;
        }
        padding=8-length;
        remaining=limit-cursor;
        if (remaining>padding) remaining=padding;
        if (remaining>0) {
            memset(cursor,' ',remaining);
            cursor+=remaining;
        }
    }
    name=(char *)disassembly_mnemonics[select_disassembly_mnemonic(state)];
    length=strlen(name);
    count=limit-cursor;
    if (count>length) count=length;
    if (count>0) {
        strncpy(cursor,name,count);
        cursor+=count;
    }
    padding=8-length;
    remaining=limit-cursor;
    if (remaining>padding) remaining=padding;
    if (remaining>0) {
        memset(cursor,' ',remaining);
        cursor+=remaining;
    }
    for (operand=0;operand<3 && cursor<limit;++operand) {
        operand_end=format_disassembly_operand(state,&state->operands[operand],operand_text);
        count=limit-cursor;
        if (count>operand_end-operand_text) count=operand_end-operand_text;
        if (count>0) {
            if (comma && cursor<limit) *cursor++=',';
            strncpy(cursor,operand_text,count);
            cursor+=count;
            comma=1;
        }
    }
    *cursor=0;
    return text;
}

/* Original [0004e630,0004e695). */
unsigned long select_disassembly_mnemonic(DisassembledInstruction *state)
{
    unsigned long mnemonic=state->mnemonic;
    switch (mnemonic) {
    case 0x12: /* cbw/cwde */
    case 0x1c: /* cwd/cdq */
    case 0x84: /* iret/iretd */
    case 0xc5: /* popf/popfd */
    case 0xcd: /* pushf/pushfd */
        if (state->operand32) ++mnemonic;
        break;
    case 0x47: /* fldenv */
    case 0x53: /* frstor */
    case 0x56: /* fnsave */
    case 0x64: /* fnstenv */
    case 0xc2: /* popa */
    case 0xca: /* pusha */
        if (state->operand32!=state->default32)
            mnemonic+=state->operand32?2:1;
        break;
    }
    return mnemonic;
}

const char *const disassembly_sizes[9]={
    "", /* 0 */
    "BYTE PTR ", /* 1 */
    "WORD PTR ", /* 2 */
    "DWORD PTR ", /* 3 */
    "FWORD PTR ", /* 4 */
    "QWORD PTR ", /* 5 */
    "TBYTE PTR ", /* 6 */
    "", /* 7 */
    "", /* 8 */
};

const char *const disassembly_registers[71]={
    "", /* 0 */
    "al", /* 1 */
    "cl", /* 2 */
    "dl", /* 3 */
    "bl", /* 4 */
    "ah", /* 5 */
    "ch", /* 6 */
    "dh", /* 7 */
    "bh", /* 8 */
    "ax", /* 9 */
    "cx", /* 10 */
    "dx", /* 11 */
    "bx", /* 12 */
    "sp", /* 13 */
    "bp", /* 14 */
    "si", /* 15 */
    "di", /* 16 */
    "eax", /* 17 */
    "ecx", /* 18 */
    "edx", /* 19 */
    "ebx", /* 20 */
    "esp", /* 21 */
    "ebp", /* 22 */
    "esi", /* 23 */
    "edi", /* 24 */
    "es", /* 25 */
    "cs", /* 26 */
    "ss", /* 27 */
    "ds", /* 28 */
    "fs", /* 29 */
    "gs", /* 30 */
    "", /* 31 */
    "cr0", /* 32 */
    "cr1", /* 33 */
    "cr2", /* 34 */
    "cr3", /* 35 */
    "cr4", /* 36 */
    "cr5", /* 37 */
    "cr6", /* 38 */
    "cr7", /* 39 */
    "dr0", /* 40 */
    "dr1", /* 41 */
    "dr2", /* 42 */
    "dr3", /* 43 */
    "dr4", /* 44 */
    "dr5", /* 45 */
    "dr6", /* 46 */
    "dr7", /* 47 */
    "tr0", /* 48 */
    "tr1", /* 49 */
    "tr2", /* 50 */
    "tr3", /* 51 */
    "tr4", /* 52 */
    "tr5", /* 53 */
    "tr6", /* 54 */
    "tr7", /* 55 */
    "", /* 56 */
    "", /* 57 */
    "", /* 58 */
    "", /* 59 */
    "", /* 60 */
    "", /* 61 */
    "st", /* 62 */
    "st(0)", /* 63 */
    "st(1)", /* 64 */
    "st(2)", /* 65 */
    "st(3)", /* 66 */
    "st(4)", /* 67 */
    "st(5)", /* 68 */
    "st(6)", /* 69 */
    "st(7)", /* 70 */
};

const char *const disassembly_scales[4]={
    "", /* 0 */
    "2", /* 1 */
    "4", /* 2 */
    "8", /* 3 */
};

/* Integer byte widths1,2,4; unused width3 has no sign interpretation. */
const unsigned long disassembly_sign_bits[5]={0,0x80,0x8000,0,0x80000000UL};
const unsigned long disassembly_sign_extend[5]={0,0xffffff00UL,0xffff0000UL,0,0};

/* Original [0004e140,0004e628), all five operand forms. */
char *format_disassembly_operand(DisassembledInstruction *state,
    const DisassemblyOperand *operand,char *text)
{
    char *cursor=text;
    const char *name;
    unsigned long value,segment;
    int absolute,component=0;
    switch (operand->kind) {
    case 1: /* Memory: size, segment, base/index/scale and displacement. */
        name=disassembly_sizes[operand->size];
        strcpy(cursor,name);
        cursor+=strlen(name);
        absolute=!operand->address.base && !operand->address.index;
        if ((state->segment && operand->address.segment!=25) || (absolute && operand->address.value_bytes)) {
            segment=(state->segment && operand->address.segment!=25)?state->segment:operand->address.segment;
            name=disassembly_registers[segment];
            strcpy(cursor,name);
            cursor+=strlen(name);
            *cursor++=':';
        }
        *cursor++='[';
        if (operand->address.base) {
            name=disassembly_registers[operand->address.base];
            strcpy(cursor,name);
            cursor+=strlen(name);
            component=1;
        }
        if (operand->address.index) {
            if (component) *cursor++='+';
            name=disassembly_registers[operand->address.index];
            strcpy(cursor,name);
            cursor+=strlen(name);
            component=1;
        }
        if (operand->address.scale) {
            *cursor++='*';
            name=disassembly_scales[operand->address.scale];
            strcpy(cursor,name);
            cursor+=strlen(name);
        }
        if (operand->address.value_bytes) {
            value=(unsigned long)operand->address.value;
            if (!absolute && value>=disassembly_sign_bits[operand->address.value_bytes]) {
                *cursor++='-';
                value=-(value|disassembly_sign_extend[operand->address.value_bytes]);
            } else if (component) *cursor++='+';
            cursor+=sprintf(cursor,"%0*lX",operand->address.value_bytes*2,value);
        }
        *cursor++=']';
        return cursor;
    case 2: /* Register. */
        name=disassembly_registers[operand->address.base];
        strcpy(cursor,name);
        return cursor+strlen(name);
    case 3: /* Immediate, with its encoded signed width. */
        value=(unsigned long)operand->address.value;
        if (value>=disassembly_sign_bits[operand->address.value_bytes]) {
            *cursor++='-';
            value=-(value|disassembly_sign_extend[operand->address.value_bytes]);
        }
        return cursor+sprintf(cursor,"%0*lX",operand->address.value_bytes*2,value);
    case 4: /* Near branch destination. */
        if (state->operand32)
            return cursor+sprintf(cursor,"%08lX",state->target_address);
        return cursor+sprintf(cursor,"%04X",(unsigned short)state->target_address);
    case 5: /* Segment:offset destination. */
        if (state->operand32)
            return cursor+sprintf(cursor,"%04X:%08lX",(unsigned short)operand->address.selector,
                (unsigned long)operand->address.value);
        return cursor+sprintf(cursor,"%04X:%04X",(unsigned short)operand->address.selector,
            (unsigned short)operand->address.value);
    default:
        *cursor=0;
        return cursor;
    }
}

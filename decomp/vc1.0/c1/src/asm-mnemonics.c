/* Instruction vocabulary, including MASM aliases and internal pseudo-ops.
   The original uses parallel suffix and ID vectors, grouped by initial letter.
   A named row keeps those two pieces together; lookup order is retained. */
#include <ctype.h>
#undef tolower
#undef toupper
#include "asm.h"
#include <recon.h>

typedef struct AsmDictionaryWord { char *suffix; short operation; } AsmDictionaryWord;
typedef struct AsmDictionaryGroup { short first, limit; } AsmDictionaryGroup;
AsmDictionaryWord asm_mnemonics[330] = {
    { "seg ", 175 }, /* _seg */
    { "scc ", 189 }, /* _scc */
    { "rpush ", 188 }, /* _rpush */
    { "movzxw ", 195 }, /* _movzxw */
    { "movsxw ", 197 }, /* _movsxw */
    { "movsr ", 208 }, /* _movsr */
    { "ljcc ", 168 }, /* _ljcc */
    { "lastop ", 229 }, /* _lastop */
    { "jmp3216 ", 198 }, /* _jmp3216 */
    { "jmp1632 ", 199 }, /* _jmp1632 */
    { "jcc ", 15 }, /* _jcc */
    { "imul3 ", 163 }, /* _imul3 */
    { "imul2 ", 193 }, /* _imul2 */
    { "fsttp ", 147 }, /* _fsttp */
    { "fldt ", 126 }, /* _fldt */
    { "firstop ", 0 }, /* _firstop */
    { "epush ", 186 }, /* _epush */
    { "epop ", 185 }, /* _epop */
    { "emit", 180 }, /* _emit */
    { "data ", 187 }, /* _data */
    { "_emit", 324 }, /* __emit */
    { "rpl", 2 }, /* arpl */
    { "nd", 46 }, /* and */
    { "lign", 328 }, /* align */
    { "dd", 45 }, /* add */
    { "dc", 44 }, /* adc */
    { "as", 152 }, /* aas */
    { "am", 151 }, /* aam */
    { "ad", 150 }, /* aad */
    { "aa", 149 }, /* aaa */
    { "ts", 205 }, /* bts */
    { "tr", 206 }, /* btr */
    { "tc", 207 }, /* btc */
    { "t", 204 }, /* bt */
    { "swap", 216 }, /* bswap */
    { "sr", 201 }, /* bsr */
    { "sf", 200 }, /* bsf */
    { "ound", 3 }, /* bound */
    { "wde ", 222 }, /* cwde */
    { "wde", 284 }, /* cwde */
    { "wd", 159 }, /* cwd */
    { "mpxchg", 218 }, /* cmpxchg */
    { "mpsw", 263 }, /* cmpsw */
    { "mpsd", 264 }, /* cmpsd */
    { "mpsb", 262 }, /* cmpsb */
    { "mps ", 224 }, /* cmps */
    { "mps", 53 }, /* cmps */
    { "mp", 47 }, /* cmp */
    { "mc", 158 }, /* cmc */
    { "lts", 157 }, /* clts */
    { "li", 156 }, /* cli */
    { "ld", 155 }, /* cld */
    { "lc", 154 }, /* clc */
    { "dq ", 223 }, /* cdq */
    { "dq", 283 }, /* cdq */
    { "bw", 153 }, /* cbw */
    { "all", 14 }, /* call */
    { "iv", 31 }, /* div */
    { "ec", 30 }, /* dec */
    { "as", 161 }, /* das */
    { "aa", 160 }, /* daa */
    { "ven", 329 }, /* even */
    { "sc", 5 }, /* esc */
    { "nter", 4 }, /* enter */
    { "yl2xp1", 146 }, /* fyl2xp1 */
    { "yl2x", 145 }, /* fyl2x */
    { "xtract", 144 }, /* fxtract */
    { "xch", 95 }, /* fxch */
    { "xam", 143 }, /* fxam */
    { "wait", 142 }, /* fwait */
    { "ucompp", 211 }, /* fucompp */
    { "ucomp", 210 }, /* fucomp */
    { "ucom", 209 }, /* fucom */
    { "tst", 141 }, /* ftst */
    { "subrp", 78 }, /* fsubrp */
    { "subr", 72 }, /* fsubr */
    { "subp", 77 }, /* fsubp */
    { "sub", 71 }, /* fsub */
    { "stsw", 108 }, /* fstsw */
    { "stp", 99 }, /* fstp */
    { "stenv", 107 }, /* fstenv */
    { "stcw", 106 }, /* fstcw */
    { "st", 98 }, /* fst */
    { "sqrt", 140 }, /* fsqrt */
    { "sincos", 213 }, /* fsincos */
    { "sin", 214 }, /* fsin */
    { "setpm", 139 }, /* fsetpm */
    { "scale", 138 }, /* fscale */
    { "save", 104 }, /* fsave */
    { "rstor", 103 }, /* frstor */
    { "rndint", 137 }, /* frndint */
    { "ptan", 136 }, /* fptan */
    { "prem1", 212 }, /* fprem1 */
    { "prem", 135 }, /* fprem */
    { "patan", 134 }, /* fpatan */
    { "nstsw", 110 }, /* fnstsw */
    { "nstenv", 112 }, /* fnstenv */
    { "nstcw", 109 }, /* fnstcw */
    { "nsave", 111 }, /* fnsave */
    { "nop", 133 }, /* fnop */
    { "ninit", 113 }, /* fninit */
    { "neni", 116 }, /* fneni */
    { "ndisi", 115 }, /* fndisi */
    { "nclex", 114 }, /* fnclex */
    { "mulp", 76 }, /* fmulp */
    { "mul", 70 }, /* fmul */
    { "ldz", 127 }, /* fldz */
    { "ldpi", 128 }, /* fldpi */
    { "ldln2", 132 }, /* fldln2 */
    { "ldlg2", 131 }, /* fldlg2 */
    { "ldl2t", 130 }, /* fldl2t */
    { "ldl2e", 129 }, /* fldl2e */
    { "ldenv", 102 }, /* fldenv */
    { "ldcw", 101 }, /* fldcw */
    { "ld1", 148 }, /* fld1 */
    { "ld", 96 }, /* fld */
    { "isubr", 89 }, /* fisubr */
    { "isub", 88 }, /* fisub */
    { "istp", 91 }, /* fistp */
    { "ist", 90 }, /* fist */
    { "init", 125 }, /* finit */
    { "incstp", 124 }, /* fincstp */
    { "imul", 87 }, /* fimul */
    { "ild", 86 }, /* fild */
    { "idivr", 85 }, /* fidivr */
    { "idiv", 84 }, /* fidiv */
    { "icomp", 83 }, /* ficomp */
    { "icom", 82 }, /* ficom */
    { "iadd", 81 }, /* fiadd */
    { "free", 97 }, /* ffree */
    { "eni", 123 }, /* feni */
    { "divrp", 80 }, /* fdivrp */
    { "divr", 74 }, /* fdivr */
    { "divp", 79 }, /* fdivp */
    { "div", 73 }, /* fdiv */
    { "disi", 122 }, /* fdisi */
    { "decstp", 121 }, /* fdecstp */
    { "cos", 215 }, /* fcos */
    { "compp", 94 }, /* fcompp */
    { "comp", 93 }, /* fcomp */
    { "com", 92 }, /* fcom */
    { "clex", 120 }, /* fclex */
    { "chs", 119 }, /* fchs */
    { "bstp", 105 }, /* fbstp */
    { "bld", 100 }, /* fbld */
    { "addp", 75 }, /* faddp */
    { "add", 69 }, /* fadd */
    { "abs", 118 }, /* fabs */
    { "2xm1", 117 }, /* f2xm1 */
    { "lt", 162 }, /* hlt */
    { "retd", 291 }, /* iretd */
    { "ret", 165 }, /* iret */
    { "nvlpg", 221 }, /* invlpg */
    { "nvd", 219 }, /* invd */
    { "nto", 164 }, /* into */
    { "nt", 7 }, /* int */
    { "nsw", 266 }, /* insw */
    { "nsd", 267 }, /* insd */
    { "nsb", 265 }, /* insb */
    { "ns", 54 }, /* ins */
    { "nc", 33 }, /* inc */
    { "n", 10 }, /* in */
    { "mul", 6 }, /* imul */
    { "div", 32 }, /* idiv */
    { "z", 261 }, /* jz */
    { "s", 260 }, /* js */
    { "po", 258 }, /* jpo */
    { "pe", 257 }, /* jpe */
    { "p", 259 }, /* jp */
    { "o", 256 }, /* jo */
    { "nz", 255 }, /* jnz */
    { "ns", 254 }, /* jns */
    { "np", 253 }, /* jnp */
    { "no", 252 }, /* jno */
    { "nle", 251 }, /* jnle */
    { "nl", 250 }, /* jnl */
    { "nge", 249 }, /* jnge */
    { "ng", 248 }, /* jng */
    { "ne", 247 }, /* jne */
    { "nc", 246 }, /* jnc */
    { "nbe", 245 }, /* jnbe */
    { "nb", 244 }, /* jnb */
    { "nae", 243 }, /* jnae */
    { "na", 242 }, /* jna */
    { "mp", 16 }, /* jmp */
    { "le", 241 }, /* jle */
    { "l", 240 }, /* jl */
    { "ge", 239 }, /* jge */
    { "g", 238 }, /* jg */
    { "ecxz", 289 }, /* jecxz */
    { "e", 237 }, /* je */
    { "cxz", 22 }, /* jcxz */
    { "c", 236 }, /* jc */
    { "be", 235 }, /* jbe */
    { "b", 234 }, /* jb */
    { "ae", 233 }, /* jae */
    { "a", 232 }, /* ja */
    { "tr", 63 }, /* ltr */
    { "ss", 190 }, /* lss */
    { "sl", 21 }, /* lsl */
    { "oopz", 25 }, /* loopz */
    { "oopnz", 24 }, /* loopnz */
    { "oopne", 327 }, /* loopne */
    { "oope", 326 }, /* loope */
    { "oop", 23 }, /* loop */
    { "odsw", 269 }, /* lodsw */
    { "odsd", 270 }, /* lodsd */
    { "odsb", 268 }, /* lodsb */
    { "ods ", 225 }, /* lods */
    { "ods", 55 }, /* lods */
    { "ock", 182 }, /* lock */
    { "msw", 62 }, /* lmsw */
    { "ldt", 61 }, /* lldt */
    { "idt", 27 }, /* lidt */
    { "gs", 192 }, /* lgs */
    { "gdt", 26 }, /* lgdt */
    { "fs", 191 }, /* lfs */
    { "es", 19 }, /* les */
    { "eave", 167 }, /* leave */
    { "ea", 18 }, /* lea */
    { "ds", 17 }, /* lds */
    { "ar", 20 }, /* lar */
    { "ahf", 166 }, /* lahf */
    { "ul", 34 }, /* mul */
    { "ovzx", 194 }, /* movzx */
    { "ovsx", 196 }, /* movsx */
    { "ovsw", 272 }, /* movsw */
    { "ovsd", 273 }, /* movsd */
    { "ovsb", 271 }, /* movsb */
    { "ovs ", 226 }, /* movs */
    { "ovs", 56 }, /* movs */
    { "ov", 1 }, /* mov */
    { "ot", 36 }, /* not */
    { "op", 169 }, /* nop */
    { "eg", 35 }, /* neg */
    { "utsw", 275 }, /* outsw */
    { "utsd", 276 }, /* outsd */
    { "utsb", 274 }, /* outsb */
    { "uts ", 227 }, /* outs */
    { "uts", 57 }, /* outs */
    { "ut", 11 }, /* out */
    { "r", 48 }, /* or */
    { "ushfd", 285 }, /* pushfd */
    { "ushf", 173 }, /* pushf */
    { "ushad", 287 }, /* pushad */
    { "usha", 172 }, /* pusha */
    { "ush", 13 }, /* push */
    { "opfd", 286 }, /* popfd */
    { "opf", 171 }, /* popf */
    { "opad", 288 }, /* popad */
    { "opa", 170 }, /* popa */
    { "op", 12 }, /* pop */
    { "or", 40 }, /* ror */
    { "ol", 39 }, /* rol */
    { "etn", 292 }, /* retn */
    { "etf", 293 }, /* retf */
    { "et", 8 }, /* ret */
    { "epz", 184 }, /* repz */
    { "epnz", 183 }, /* repnz */
    { "epne", 231 }, /* repne */
    { "epe", 230 }, /* repe */
    { "ep", 181 }, /* rep */
    { "cr", 38 }, /* rcr */
    { "cl", 37 }, /* rcl */
    { "ub", 50 }, /* sub */
    { "tr", 66 }, /* str */
    { "tosw", 281 }, /* stosw */
    { "tosd", 282 }, /* stosd */
    { "tosb", 280 }, /* stosb */
    { "tos", 59 }, /* stos */
    { "ti", 178 }, /* sti */
    { "td", 177 }, /* std */
    { "tc", 176 }, /* stc */
    { "msw", 65 }, /* smsw */
    { "ldt", 64 }, /* sldt */
    { "idt", 29 }, /* sidt */
    { "hrd", 203 }, /* shrd */
    { "hr", 43 }, /* shr */
    { "hld", 202 }, /* shld */
    { "hl", 42 }, /* shl */
    { "gdt", 28 }, /* sgdt */
    { "etz", 323 }, /* setz */
    { "ets", 322 }, /* sets */
    { "etpo", 320 }, /* setpo */
    { "etpe", 319 }, /* setpe */
    { "etp", 321 }, /* setp */
    { "eto", 318 }, /* seto */
    { "etnz", 317 }, /* setnz */
    { "etns", 316 }, /* setns */
    { "etnp", 315 }, /* setnp */
    { "etno", 314 }, /* setno */
    { "etnle", 313 }, /* setnle */
    { "etnl", 312 }, /* setnl */
    { "etnge", 311 }, /* setnge */
    { "etng", 310 }, /* setng */
    { "etne", 309 }, /* setne */
    { "etnc", 308 }, /* setnc */
    { "etnbe", 307 }, /* setnbe */
    { "etnb", 306 }, /* setnb */
    { "etnae", 305 }, /* setnae */
    { "etna", 304 }, /* setna */
    { "etle", 303 }, /* setle */
    { "etl", 302 }, /* setl */
    { "etge", 301 }, /* setge */
    { "etg", 300 }, /* setg */
    { "ete", 299 }, /* sete */
    { "etc", 298 }, /* setc */
    { "etbe", 297 }, /* setbe */
    { "etb", 296 }, /* setb */
    { "etae", 295 }, /* setae */
    { "eta", 294 }, /* seta */
    { "casw", 278 }, /* scasw */
    { "casd", 279 }, /* scasd */
    { "casb", 277 }, /* scasb */
    { "cas", 58 }, /* scas */
    { "bb", 49 }, /* sbb */
    { "ar", 41 }, /* sar */
    { "al", 325 }, /* sal */
    { "ahf", 174 }, /* sahf */
    { "est", 51 }, /* test */
    { "erw", 68 }, /* verw */
    { "err", 67 }, /* verr */
    { "binvd", 220 }, /* wbinvd */
    { "ait", 179 }, /* wait */
    { "or", 52 }, /* xor */
    { "latb", 290 }, /* xlatb */
    { "lat ", 228 }, /* xlat */
    { "lat", 60 }, /* xlat */
    { "chg", 9 }, /* xchg */
    { "add", 217 }, /* xadd */
};
AsmDictionaryGroup asm_mnemonic_groups[28] = {
    { 0, 21 }, /* '_' */
    { 21, 21 }, /* '`' */
    { 21, 30 }, /* 'a' */
    { 30, 38 }, /* 'b' */
    { 38, 57 }, /* 'c' */
    { 57, 61 }, /* 'd' */
    { 61, 64 }, /* 'e' */
    { 64, 149 }, /* 'f' */
    { 149, 149 }, /* 'g' */
    { 149, 150 }, /* 'h' */
    { 150, 164 }, /* 'i' */
    { 164, 197 }, /* 'j' */
    { 197, 197 }, /* 'k' */
    { 197, 223 }, /* 'l' */
    { 223, 232 }, /* 'm' */
    { 232, 235 }, /* 'n' */
    { 235, 242 }, /* 'o' */
    { 242, 252 }, /* 'p' */
    { 252, 252 }, /* 'q' */
    { 252, 264 }, /* 'r' */
    { 264, 319 }, /* 's' */
    { 319, 320 }, /* 't' */
    { 320, 320 }, /* 'u' */
    { 320, 322 }, /* 'v' */
    { 322, 324 }, /* 'w' */
    { 324, 330 }, /* 'x' */
    { 330, 330 }, /* 'y' */
    { 330, 330 }, /* 'z' */
};

/* Original 0x4d9f6..0x4da60, callee-cleaned one stack argument. */
short __stdcall find_asm_mnemonic(char *name)
{
    int letter;
    short index, limit;
    if (*name != '$') {
        letter = tolower(*name);
        /* The vocabulary's valid identifier initials occupy '_' through 'z'. */
        if (letter >= '_' && letter <= 'z') {
            index = asm_mnemonic_groups[letter - '_'].first;
            limit = asm_mnemonic_groups[letter - '_'].limit;
            for (; index != limit; ++index)
                if (!compare_names_ignore_case(asm_mnemonics[index].suffix, name + 1))
                    return asm_mnemonics[index].operation;
        }
    }
    return -1;
}

/* Original 0x35780..0x35800. Equal bytes avoid case conversion entirely;
   unequal bytes compare folded values, but retain the signed raw difference. */
int __stdcall compare_names_ignore_case(char *first, char *second)
{
    char left, right;
    int folded_right, folded_left;
    if (first == second) return 0;
    if (!first || !second) return -1;
    for (;;) {
        left = *first++;
        right = *second++;
        if (left != right) {
            folded_right = toupper(right);
            folded_left = toupper(left);
            if (folded_right != folded_left) return (int)left - (int)right;
        } else if (!left) return 0;
    }
}

/* Operand vocabulary: register numbers, size/address qualifiers and MASM
   expression operators. Its grouping and search order match the mnemonic map. */
AsmDictionaryWord asm_keywords[76] = {
    { "rnil ", 0 }, /* _rnil */
    { "x", 9 }, /* ax */
    { "nd", 61 }, /* and */
    { "l", 17 }, /* al */
    { "h", 21 }, /* ah */
    { "yte", 51 }, /* byte */
    { "x", 12 }, /* bx */
    { "p", 14 }, /* bp */
    { "l", 20 }, /* bl */
    { "h", 24 }, /* bh */
    { "x", 10 }, /* cx */
    { "s", 26 }, /* cs */
    { "r3", 34 }, /* cr3 */
    { "r2", 33 }, /* cr2 */
    { "r0", 32 }, /* cr0 */
    { "l", 18 }, /* cl */
    { "h", 22 }, /* ch */
    { "x", 11 }, /* dx */
    { "word", 47 }, /* dword */
    { "s", 28 }, /* ds */
    { "r7", 40 }, /* dr7 */
    { "r6", 39 }, /* dr6 */
    { "r3", 38 }, /* dr3 */
    { "r2", 37 }, /* dr2 */
    { "r1", 36 }, /* dr1 */
    { "r0", 35 }, /* dr0 */
    { "l", 19 }, /* dl */
    { "i", 16 }, /* di */
    { "h", 23 }, /* dh */
    { "sp", 5 }, /* esp */
    { "si", 7 }, /* esi */
    { "s", 25 }, /* es */
    { "q", 55 }, /* eq */
    { "dx", 3 }, /* edx */
    { "di", 8 }, /* edi */
    { "cx", 2 }, /* ecx */
    { "bx", 4 }, /* ebx */
    { "bp", 6 }, /* ebp */
    { "ax", 1 }, /* eax */
    { "word", 48 }, /* fword */
    { "s", 29 }, /* fs */
    { "ar", 54 }, /* far */
    { "t", 57 }, /* gt */
    { "s", 30 }, /* gs */
    { "e", 56 }, /* ge */
    { "igh", 68 }, /* high */
    { "t", 59 }, /* lt */
    { "ow", 70 }, /* low */
    { "ength", 69 }, /* length */
    { "e", 58 }, /* le */
    { "od", 62 }, /* mod */
    { "ot", 63 }, /* not */
    { "ear", 53 }, /* near */
    { "e", 60 }, /* ne */
    { "r", 64 }, /* or */
    { "ffset", 71 }, /* offset */
    { "tr", 72 }, /* ptr */
    { "word", 49 }, /* qword */
    { "t", 31 }, /* st */
    { "s", 27 }, /* ss */
    { "p", 13 }, /* sp */
    { "ize", 74 }, /* size */
    { "i", 15 }, /* si */
    { "hr", 66 }, /* shr */
    { "hort", 52 }, /* short */
    { "hl", 65 }, /* shl */
    { "eg", 73 }, /* seg */
    { "ype", 75 }, /* type */
    { "r7", 45 }, /* tr7 */
    { "r6", 44 }, /* tr6 */
    { "r5", 43 }, /* tr5 */
    { "r4", 42 }, /* tr4 */
    { "r3", 41 }, /* tr3 */
    { "byte", 50 }, /* tbyte */
    { "ord", 46 }, /* word */
    { "or", 67 }, /* xor */
};
AsmDictionaryGroup asm_keyword_groups[28] = {
    { 0, 1 }, /* '_' */
    { 1, 1 }, /* '`' */
    { 1, 5 }, /* 'a' */
    { 5, 10 }, /* 'b' */
    { 10, 17 }, /* 'c' */
    { 17, 29 }, /* 'd' */
    { 29, 39 }, /* 'e' */
    { 39, 42 }, /* 'f' */
    { 42, 45 }, /* 'g' */
    { 45, 46 }, /* 'h' */
    { 46, 46 }, /* 'i' */
    { 46, 46 }, /* 'j' */
    { 46, 46 }, /* 'k' */
    { 46, 50 }, /* 'l' */
    { 50, 51 }, /* 'm' */
    { 51, 54 }, /* 'n' */
    { 54, 56 }, /* 'o' */
    { 56, 57 }, /* 'p' */
    { 57, 58 }, /* 'q' */
    { 58, 58 }, /* 'r' */
    { 58, 67 }, /* 's' */
    { 67, 74 }, /* 't' */
    { 74, 74 }, /* 'u' */
    { 74, 74 }, /* 'v' */
    { 74, 75 }, /* 'w' */
    { 75, 76 }, /* 'x' */
    { 76, 76 }, /* 'y' */
    { 76, 76 }, /* 'z' */
};

/* Original 0x4d98b..0x4d9f5, callee-cleaned one stack argument. */
short __stdcall find_asm_keyword(char *name)
{
    int letter;
    short index, limit;
    if (*name != '$') {
        letter = tolower(*name);
        if (letter >= '_' && letter <= 'z') {
            index = asm_keyword_groups[letter - '_'].first;
            limit = asm_keyword_groups[letter - '_'].limit;
            for (; index != limit; ++index)
                if (!compare_names_ignore_case(asm_keywords[index].suffix, name + 1))
                    return asm_keywords[index].operation;
        }
    }
    return -1;
}

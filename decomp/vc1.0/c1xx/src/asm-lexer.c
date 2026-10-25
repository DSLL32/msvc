/* C++ lexical codes as consumed by the assembler's low-six-bit vocabulary.
   Original 0xa1d47..0xa1e1b; the same bytes' operator-name bit has a separate
   logical membership view in qualified-names.c. No unrelated bits are copied. */
#include "asm.h"
#include "lexer.h"
#include "source-input.h"
#include "diagnostics.h"
#include "enumerators.h"
#include <recon.h>

unsigned char asm_token_classes[213] = {
    /* 0..18: basic tokens, numeric literals and declaration storage. */
    AS_END, AS_NEWLINE, AS_IDENTIFIER, AS_BAD, AS_BAD, AS_BAD, AS_BAD,
    AS_CONSTANT, AS_CONSTANT, AS_CONSTANT, AS_CONSTANT,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    /* char..union and declaration pseudo-tokens34..40. */
    AS_C_DATA_TYPE, AS_C_DATA_TYPE, AS_C_DATA_TYPE, AS_C_DATA_TYPE,
    AS_C_DATA_TYPE, AS_C_DATA_TYPE, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_C_DATA_TYPE, AS_C_DATA_TYPE,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_MAPPED, AS_MAPPED, AS_MAPPED, AS_MAPPED, AS_BAD, AS_BAD, AS_MAPPED,
    /* Calling conventions41..49, __asm, pointer and qualifier words. */
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_NEWLINE, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_C_DATA_TYPE, AS_C_DATA_TYPE,
    /* Operators58..101, in the C++ lexer's original order. */
    AS_AND, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_COLON, AS_COMMA, AS_BAD,
    AS_DIVIDE, AS_BAD, AS_EQ, AS_BAD, AS_BAD, AS_GT, AS_GE, AS_BAD,
    AS_LEFT_BRACKET, AS_BAD, AS_SHL, AS_LT, AS_LE, AS_MINUS, AS_BAD,
    AS_MOD, AS_BAD, AS_MULTIPLY, AS_BAD, AS_NE, AS_OR, AS_BAD, AS_BAD,
    AS_DOT, AS_PLUS, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD,
    AS_SHR, AS_NOT, AS_BAD, AS_XOR, AS_BAD,
    /* Expression forms102..107 and statements/punctuation108..147. */
    AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_BAD, AS_MAPPED, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_MAPPED, AS_IDENTIFIER, AS_BAD, AS_IDENTIFIER,
    AS_BAD, AS_END_OF_FILE, AS_BAD, AS_IDENTIFIER, AS_BAD, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_BAD, AS_BAD, AS_LEFT_BRACE, AS_LEFT_PAREN, AS_BAD,
    AS_MAPPED, AS_IDENTIFIER, AS_RIGHT_BRACKET, AS_NEWLINE, AS_MAPPED,
    AS_IDENTIFIER, AS_RIGHT_PAREN, AS_IDENTIFIER, AS_SEMICOLON, AS_MAPPED,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_MAPPED, AS_IDENTIFIER, AS_BAD, AS_BAD,
    AS_IDENTIFIER, AS_BAD,
    /* C++ access/allocation/this148..155 and internal forms156..170. */
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_BAD, AS_BAD, AS_IDENTIFIER,
    AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD,
    AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD, AS_BAD,
    /* Converted operators171..177, SEH and remaining keywords178..205. */
    AS_MULTIPLY, AS_AND, AS_MAPPED, AS_C_DATA_TYPE, AS_C_DATA_TYPE,
    AS_IDENTIFIER, AS_BAD,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_MAPPED, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_BAD, AS_BAD, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER, AS_IDENTIFIER,
    AS_IDENTIFIER, AS_IDENTIFIER,
    /* Extended data types206/207, builtins, template '>' and reserved tail. */
    AS_C_DATA_TYPE, AS_C_DATA_TYPE, AS_IDENTIFIER, AS_GT, AS_BAD, AS_BAD, AS_END
};
unsigned char asm_current_token; /* original 0x8c148 */

/* Original 0x7d16d..0x7d23f, cdecl byte result. The C++ scanner
   groups operators; mapped tokens must use their retained lexical code. */
unsigned char read_asm_token(void)
{
    unsigned char category, code, token;
    for (;;) {
        category = read_parser_token();
        code = category;
        if (!code) fatal_error(4);
        if (!asm_skip_initial_newlines) break;
        if (code != 1) {
            asm_skip_initial_newlines = 0;
            break;
        }
        begin_source_line();
    }
    if (asm_token_classes[category] == AS_MAPPED) code = current_token_code;
    token = asm_token_classes[code];
    if (token == AS_IDENTIFIER) {
        if (code != 2) current_token_value.name = interned_name;
        return classify_asm_identifier();
    }
    if (code == TOK_INT && !asm_operand_mode) {
        *(unsigned short *)current_token_value.bytes = 7;
        asm_current_token = AS_OPCODE;
        return AS_OPCODE;
    }
    if (token < AS_MAPPED) current_token_value.bytes[0] = token;
    if (token == AS_SEMICOLON) {
        copy_asm_comment();
        ++source_text_cursor;
        current_token_code = 1;
        code = 1;
    }
    asm_current_token = asm_token_classes[code];
    return asm_current_token;
}

/* Original 0x7d793..0x7d7fd, cdecl. Return a single-line form's
   delimiter to the C scanner; an explicit assembler block owns its braces. */
int finish_asm_line(void)
{
    if (current_token_code == 133) {
        if (!--asm_open_braces) return 1;
        if (asm_open_braces < 0) {
            --source_text_cursor;
            asm_open_braces = 0;
            return 1;
        }
    } else if (current_token_code != TOK_MS_ASM) {
        if (!asm_open_braces) {
            --source_text_cursor;
            return 1;
        }
        begin_source_line();
    }
    return 0;
}

/* Original alias IDs 230..327. Each entry identifies a canonical instruction
   and its condition or operand-size modifier. The branch-condition numbering
   follows the x86 condition field; size modifiers are byte/word/dword = 0/1/2. */
AsmOpcodeAlias asm_opcode_aliases[98] = {
    {184,0}, /* repe */ {183,0}, /* repne */
    {15,7}, /* ja */ {15,3}, /* jae */ {15,2}, /* jb */ {15,6}, /* jbe */
    {15,2}, /* jc */ {15,4}, /* je */ {15,15}, /* jg */ {15,13}, /* jge */
    {15,12}, /* jl */ {15,14}, /* jle */ {15,6}, /* jna */ {15,2}, /* jnae */
    {15,3}, /* jnb */ {15,7}, /* jnbe */ {15,3}, /* jnc */ {15,5}, /* jne */
    {15,14}, /* jng */ {15,12}, /* jnge */ {15,13}, /* jnl */ {15,15}, /* jnle */
    {15,1}, /* jno */ {15,11}, /* jnp */ {15,9}, /* jns */ {15,5}, /* jnz */
    {15,0}, /* jo */ {15,10}, /* jpe */ {15,11}, /* jpo */ {15,10}, /* jp */
    {15,8}, /* js */ {15,4}, /* jz */
    {53,0}, /* cmpsb */ {53,1}, /* cmpsw */ {53,2}, /* cmpsd */
    {54,0}, /* insb */ {54,1}, /* insw */ {54,2}, /* insd */
    {55,0}, /* lodsb */ {55,1}, /* lodsw */ {55,2}, /* lodsd */
    {56,0}, /* movsb */ {56,1}, /* movsw */ {56,2}, /* movsd */
    {57,0}, /* outsb */ {57,1}, /* outsw */ {57,2}, /* outsd */
    {58,0}, /* scasb */ {58,1}, /* scasw */ {58,2}, /* scasd */
    {59,0}, /* stosb */ {59,1}, /* stosw */ {59,2}, /* stosd */
    {159,2}, /* cdq */ {153,1}, /* cwde */
    {173,2}, /* pushfd */ {171,2}, /* popfd */
    {172,2}, /* pushad */ {170,2}, /* popad */
    {22,2}, /* jecxz */ {60,0}, /* xlatb */ {165,2}, /* iretd */
    {8,2}, /* retn */ {8,5}, /* retf */
    {189,7}, /* seta */ {189,3}, /* setae */ {189,2}, /* setb */ {189,6}, /* setbe */
    {189,2}, /* setc */ {189,4}, /* sete */ {189,15}, /* setg */ {189,13}, /* setge */
    {189,12}, /* setl */ {189,14}, /* setle */ {189,6}, /* setna */ {189,2}, /* setnae */
    {189,3}, /* setnb */ {189,7}, /* setnbe */ {189,3}, /* setnc */ {189,5}, /* setne */
    {189,14}, /* setng */ {189,12}, /* setnge */ {189,13}, /* setnl */ {189,15}, /* setnle */
    {189,1}, /* setno */ {189,11}, /* setnp */ {189,9}, /* setns */ {189,5}, /* setnz */
    {189,0}, /* seto */ {189,10}, /* setpe */ {189,11}, /* setpo */ {189,10}, /* setp */
    {189,8}, /* sets */ {189,4}, /* setz */
    {180,0}, /* __emit */
    {42,0}, /* sal */ {25,0}, /* loope */ {24,0} /* loopne */
};

/* Original 0x7d240..0x7d429. Mnemonics take precedence before operands;
   within an operand, registers/operators precede C symbols and enum values. */
unsigned char classify_asm_identifier(void)
{
    short keyword;
    CppSymbol *symbol;
    if (!asm_operand_mode) {
        keyword = find_asm_mnemonic(interned_name);
        if (keyword != -1) {
            if (keyword >= 0 && keyword < 230) {
                *(short *)current_token_value.bytes = keyword;
                asm_opcode = keyword;
                asm_current_token = keyword > 180 && keyword < 185 ? AS_PREFIX : AS_OPCODE;
                return asm_current_token;
            }
            if (keyword > 229 && keyword < 328) {
                *(short *)current_token_value.bytes = asm_opcode_aliases[keyword - 230].opcode;
                asm_opcode = keyword;
                asm_current_token = keyword < 232 ? AS_PREFIX : AS_OPCODE;
                return asm_current_token;
            }
            *(short *)current_token_value.bytes = keyword;
            asm_current_token = AS_DIRECTIVE;
            return AS_DIRECTIVE;
        }
        if (find_asm_keyword(interned_name) != -1) compiler_warning(1,405,interned_name);
    } else if (!asm_member_mode) {
        keyword = find_asm_keyword(interned_name);
        if (keyword != -1) {
            if (keyword >= 0 && keyword < 46) asm_current_token = AS_REGISTER;
            else if (keyword > 45 && keyword < 52) asm_current_token = AS_ASM_DATA_TYPE;
            else if (keyword > 51 && keyword < 55) asm_current_token = AS_ASM_CODE_TYPE;
            else {
                asm_current_token = (unsigned char)(keyword - 54);
                current_token_value.bytes[0] = asm_current_token;
                return asm_current_token;
            }
            *(short *)current_token_value.bytes = keyword;
            return asm_current_token;
        }
        if (find_asm_mnemonic(interned_name) != -1) compiler_warning(1,405,interned_name);
        symbol = find_outer_name(current_token_value.name);
        if (symbol && (symbol->kind & 31) == 5) {
            asm_current_token = AS_ENUM_MEMBER;
            current_token_value.words.low = ((EnumeratorSymbol *)symbol)->value;
            return AS_ENUM_MEMBER;
        }
    }
    asm_current_token = AS_IDENTIFIER;
    return AS_IDENTIFIER;
}

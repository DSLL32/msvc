/* C lexer categories interpreted by the assembler grammar. The original
   byte vocabulary also carries C expression flags; this owner contains only
   its low six-bit assembly token classes, which are the part consumed here. */
#include "asm.h"
#include "source-input.h"
#include "builtins.h"
#include <recon.h>

unsigned char asm_token_classes[153] = {
    AS_END, /* TOK_NONE */
    AS_NEWLINE, /* TOK_NEWLINE */
    AS_IDENTIFIER, /* TOK_IDENTIFIER */
    AS_BAD, /* TOK_STRING */
    AS_BAD, /* TOK_FLOAT_CONSTANT */
    AS_BAD, /* TOK_DOUBLE_CONSTANT */
    AS_BAD, /* TOK_LONG_DOUBLE_CONSTANT */
    AS_CONSTANT, /* TOK_INT_CONSTANT */
    AS_CONSTANT, /* TOK_LONG_CONSTANT */
    AS_CONSTANT, /* TOK_UINT_CONSTANT */
    AS_CONSTANT, /* TOK_ULONG_CONSTANT */
    AS_IDENTIFIER, /* TOK_AUTO */
    AS_IDENTIFIER, /* TOK_STATIC */
    AS_IDENTIFIER, /* TOK_EXTERN */
    AS_IDENTIFIER, /* TOK_INLINE */
    AS_IDENTIFIER, /* TOK_REGISTER */
    AS_IDENTIFIER, /* TOK_TYPEDEF */
    AS_IDENTIFIER, /* TOK_FORTRAN */
    AS_IDENTIFIER, /* TOK_PASCAL */
    AS_IDENTIFIER, /* TOK_CDECL */
    AS_IDENTIFIER, /* TOK_FASTCALL */
    AS_IDENTIFIER, /* TOK_SYSCALL */
    AS_IDENTIFIER, /* TOK_INTERRUPT */
    AS_IDENTIFIER, /* TOK_SAVEREGS */
    AS_IDENTIFIER, /* TOK_LOADDS */
    AS_IDENTIFIER, /* TOK_EXPORT */
    AS_NEWLINE, /* TOK_ASM */
    AS_IDENTIFIER, /* TOK_BASED */
    AS_IDENTIFIER, /* TOK_NEAR */
    AS_IDENTIFIER, /* TOK_FAR */
    AS_IDENTIFIER, /* TOK_FAR16 */
    AS_IDENTIFIER, /* TOK_HUGE */
    AS_C_DATA_TYPE, /* TOK_CONST */
    AS_C_DATA_TYPE, /* TOK_VOLATILE */
    AS_C_DATA_TYPE, /* TOK_CHAR */
    AS_C_DATA_TYPE, /* TOK_INT */
    AS_C_DATA_TYPE, /* TOK_FLOAT */
    AS_C_DATA_TYPE, /* TOK_DOUBLE */
    AS_C_DATA_TYPE, /* TOK_SHORT */
    AS_C_DATA_TYPE, /* TOK_LONG */
    AS_IDENTIFIER, /* TOK_VOID */
    AS_IDENTIFIER, /* TOK_SEGMENT */
    AS_IDENTIFIER, /* TOK_SEGNAME */
    AS_C_DATA_TYPE, /* TOK_SIGNED */
    AS_C_DATA_TYPE, /* TOK_UNSIGNED */
    AS_IDENTIFIER, /* TOK_ENUM */
    AS_IDENTIFIER, /* TOK_STRUCT */
    AS_IDENTIFIER, /* TOK_UNION */
    AS_AND, /* TOK_BIT_AND */
    AS_BAD, /* TOK_LOGICAL_AND */
    AS_BAD, /* TOK_AND_ASSIGN */
    AS_BAD, /* TOK_ASSIGN */
    AS_BAD, /* TOK_BIT_FIELD */
    AS_COLON, /* TOK_COLON */
    AS_COMMA, /* TOK_COMMA */
    AS_BAD, /* TOK_PREDECREMENT */
    AS_DIVIDE, /* TOK_DIVIDE */
    AS_BAD, /* TOK_DIVIDE_ASSIGN */
    AS_EQ, /* TOK_EQUAL */
    AS_BAD, /* TOK_LOGICAL_NOT */
    AS_BAD, /* TOK_DEREFERENCE */
    AS_GT, /* TOK_GREATER */
    AS_GE, /* TOK_GREATER_EQUAL */
    AS_BAD, /* TOK_PREINCREMENT */
    AS_LEFT_BRACKET, /* TOK_LEFT_BRACKET */
    AS_BAD, /* TOK_LEFT_SHIFT_ASSIGN */
    AS_SHL, /* TOK_LEFT_SHIFT */
    AS_LT, /* TOK_LESS */
    AS_LE, /* TOK_LESS_EQUAL */
    AS_MINUS, /* TOK_MINUS */
    AS_BAD, /* TOK_MINUS_ASSIGN */
    AS_MOD, /* TOK_REMAINDER */
    AS_BAD, /* TOK_REMAINDER_ASSIGN */
    AS_MULTIPLY, /* TOK_MULTIPLY */
    AS_BAD, /* TOK_MULTIPLY_ASSIGN */
    AS_NE, /* TOK_NOT_EQUAL */
    AS_OR, /* TOK_BIT_OR */
    AS_BAD, /* TOK_OR_ASSIGN */
    AS_BAD, /* TOK_LOGICAL_OR */
    AS_DOT, /* TOK_MEMBER */
    AS_PLUS, /* TOK_PLUS */
    AS_BAD, /* TOK_PLUS_ASSIGN */
    AS_BAD, /* TOK_ARROW */
    AS_BAD, /* TOK_POSTDECREMENT */
    AS_BAD, /* TOK_POSTINCREMENT */
    AS_BAD, /* TOK_QUESTION */
    AS_BAD, /* TOK_RIGHT_SHIFT_ASSIGN */
    AS_SHR, /* TOK_RIGHT_SHIFT */
    AS_NOT, /* TOK_BIT_NOT */
    AS_BAD, /* TOK_UNARY_MINUS */
    AS_XOR, /* TOK_BIT_XOR */
    AS_BAD, /* TOK_XOR_ASSIGN */
    AS_BAD, /* TOK_BASED_REFERENCE */
    AS_BAD, /* TOK_SEGMENT_OFFSET */
    AS_BAD, /* TOK_DESEG */
    AS_BAD, /* TOK_ARGUMENT */
    AS_BAD, /* TOK_ASSIGNMENT_OPERATOR */
    AS_IDENTIFIER, /* TOK_BREAK */
    AS_IDENTIFIER, /* TOK_CASE */
    AS_BAD, /* TOK_CAST */
    AS_IDENTIFIER, /* TOK_STORAGE_CLASS */
    AS_MAPPED, /* TOK_CONSTANT */
    AS_IDENTIFIER, /* TOK_CONTINUE */
    AS_IDENTIFIER, /* TOK_DEFAULT */
    AS_MAPPED, /* TOK_DIVISION_OPERATOR */
    AS_IDENTIFIER, /* TOK_DO */
    AS_BAD, /* TOK_ELLIPSIS */
    AS_IDENTIFIER, /* TOK_ELSE */
    AS_BAD, /* TOK_ENTRY */
    AS_END_OF_FILE, /* TOK_END_OF_FILE */
    AS_BAD, /* TOK_EQUALITY_OPERATOR */
    AS_IDENTIFIER, /* TOK_EXCEPT */
    AS_IDENTIFIER, /* TOK_FINALLY */
    AS_IDENTIFIER, /* TOK_FOR */
    AS_BAD, /* TOK_ARGUMENT_END */
    AS_IDENTIFIER, /* TOK_GOTO */
    AS_IDENTIFIER, /* TOK_IF */
    AS_BAD, /* TOK_INCREMENT_OPERATOR */
    AS_BAD, /* TOK_INITIALIZING */
    AS_LEFT_BRACE, /* TOK_LEFT_BRACE */
    AS_LEFT_PAREN, /* TOK_LEFT_PAREN */
    AS_BAD, /* TOK_INTRINSIC */
    AS_MAPPED, /* TOK_MODIFIER */
    AS_IDENTIFIER, /* TOK_OLDCALL */
    AS_RIGHT_BRACKET, /* TOK_RIGHT_BRACKET */
    AS_NEWLINE, /* TOK_RIGHT_BRACE */
    AS_MAPPED, /* TOK_RELATIONAL_OPERATOR */
    AS_IDENTIFIER, /* TOK_RETURN */
    AS_RIGHT_PAREN, /* TOK_RIGHT_PAREN */
    AS_IDENTIFIER, /* TOK_SELF */
    AS_SEMICOLON, /* TOK_SEMICOLON */
    AS_MAPPED, /* TOK_SHIFT_OPERATOR */
    AS_IDENTIFIER, /* TOK_SIZEOF */
    AS_IDENTIFIER, /* TOK_STDCALL */
    AS_MAPPED, /* TOK_MEMBER_OPERATOR */
    AS_IDENTIFIER, /* TOK_SWITCH */
    AS_BAD, /* TOK_RESERVED_136 */
    AS_IDENTIFIER, /* TOK_TRY */
    AS_MAPPED, /* TOK_TYPE */
    AS_TYPEDEF_NAME, /* TOK_TYPEDEF_NAME */
    AS_BAD, /* TOK_UNARY_PLUS */
    AS_IDENTIFIER, /* TOK_WHILE */
    AS_END, /* TOK_PP_OUTPUT */
    AS_MAPPED, /* TOK_TYPE_NAME */
    AS_BAD, /* TOK_CLASS_HEAD */
    AS_BAD, /* TOK_ENUM_TAG */
    AS_MAPPED, /* TOK_STORAGE_MODIFIER */
    AS_C_DATA_TYPE, /* TOK_UNALIGNED */
    AS_IDENTIFIER, /* TOK_BUILTIN_ALIGNOF */
    AS_IDENTIFIER, /* TOK_LEAVE */
    AS_IDENTIFIER, /* TOK_SYSAPI */
    AS_IDENTIFIER, /* TOK_DECLSPEC */
    AS_BAD, /* TOK_ALIGNMENT_NOTIFIER */
};
unsigned char asm_current_token; /* original 0x5b148 */

/* Original 0x4d6ce..0x4d7a0. Read C tokens, retaining assembler line
   boundaries and using the separate instruction/register vocabulary. */
unsigned char read_asm_token(void)
{
    unsigned char category, code, token;
    for (;;) {
        category = read_parser_token();
        code = category;
        if (!code) fatal_error(4);
        if (!asm_skip_initial_newlines) break;
        if (code != TOK_NEWLINE) {
            asm_skip_initial_newlines = 0;
            break;
        }
        begin_source_line();
    }
    if (asm_token_classes[category] == AS_MAPPED) code = current_token_code;
    token = asm_token_classes[code];
    if (token == AS_IDENTIFIER) {
        if (code != TOK_IDENTIFIER) current_token_value.identifier = interned_name;
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
        current_token_code = TOK_NEWLINE;
        code = TOK_NEWLINE;
    }
    asm_current_token = asm_token_classes[code];
    return asm_current_token;
}

/* Original 0x4dcf4..0x4dd5e. A single-line form returns its delimiter to
   the C scanner; braces count only the assembler's explicit block. */
int finish_asm_line(void)
{
    if (current_token_code == TOK_RIGHT_BRACE) {
        if (!--asm_open_braces) return 1;
        if (asm_open_braces < 0) {
            --source_text_cursor;
            asm_open_braces = 0;
            return 1;
        }
    } else if (current_token_code != TOK_ASM) {
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

/* Original 0x4d7a1..0x4d98a. Mnemonics take precedence before operands;
   within an operand, registers/operators precede C symbols and enum values. */
unsigned char classify_asm_identifier(void)
{
    short keyword;
    FrontendSymbol *symbol;
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
        if (find_asm_keyword(interned_name) != -1) compiler_warning(1,405);
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
        if (find_asm_mnemonic(interned_name) != -1) compiler_warning(1,405);
        symbol = find_identifier_symbol(current_token_value.identifier);
        if (symbol && (symbol->kind & 31) == 5) {
            asm_current_token = AS_ENUM_MEMBER;
            current_token_value.words.low = ((EnumeratorSymbol *)symbol)->value;
            return AS_ENUM_MEMBER;
        }
    }
    asm_current_token = AS_IDENTIFIER;
    return AS_IDENTIFIER;
}

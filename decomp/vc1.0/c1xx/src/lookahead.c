/* Deferred C++ declaration/expression lookahead and token reinterpretation. */
#include <stdlib.h>
#include "lookahead.h"
#include "source-input.h"
#include "numeric-conversion.h"
#include <recon.h>

int parenthesis_lookahead_depth; /* original 0x9d7cc */

/* Original 0x18ea0..0x18f12, stdcall 8. Replay the declaration token
   as a type expression, retaining its original type-specifier symbol. */
void __stdcall convert_deferred_type_token(DeferredToken *token, CType *type)
{
    ArenaPool *pool;
    IrToken *expression;
    if (token_allocation_flags & 0x20000020UL)
        expression = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken))
            refill_arena(&compiler_arena, pool);
        expression = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    expression->flags = 0;
    expression->type = type;
    expression->kind = 0xa3;
    expression->value.pointer = token->type_specifier;
    current_token_code = 0xa3;
    token->lexical_token = token->parser_token = 0xa3;
    token->value.expression = expression;
}

/* Original 0x19080..0x19152, stdcall 4. Parentheses in the deferred
   sequence delimit declaration contexts; the mode argument is not read. */
void __stdcall resolve_nested_deferred_types(unsigned char unused_mode)
{
    DeferredToken *token = deferred_token_cursor + 1, *saved_cursor;
    unsigned char replayed;
    int depth = 0;
    while (token != deferred_token_end) {
        switch (token->lexical_token) {
        case 0x80:
            ++depth;
            if (++declaration_depth >= 60) fatal_error(13);
            declaration_spec_flags[declaration_depth] &= ~1;
            declaration_spec_flags[declaration_depth] &= ~2;
            break;
        case 0x88:
            if (--depth < 0) return;
            if (declaration_depth > 0) --declaration_depth;
            break;
        case 0xb5:
            if (depth <= 1)
                convert_deferred_type_token(token, token->value.declaration.type);
            else {
                token_allocation_flags &= ~0x80000000UL;
                saved_cursor = deferred_token_cursor;
                deferred_token_cursor = token;
                replayed = resolve_parenthesis_candidate(0);
                unget_token(replayed);
                deferred_token_cursor = saved_cursor;
            }
            break;
        }
        ++token;
    }
}

/* Original 0x19190..0x1933e. The auxiliary parser may recurse through the
   lexer, so restore its caller's declaration depth and token cursor before
   deciding how the saved type token is to be replayed. */
unsigned char __stdcall resolve_parenthesis_candidate(int allow_expression)
{
    DeferredToken *saved_cursor = deferred_token_cursor;
    CType *saved_type = saved_cursor->value.declaration.type;
    int saved_depth, result;
    unsigned char token;
    ++parenthesis_lookahead_depth;
    if (saved_cursor->lexical_token == 181) {
        if (token_allocation_flags & 0x80000000UL) {
            classify_deferred_type(saved_cursor);
            goto read_token;
        }
        set_deferred_type_name(deferred_token_cursor);
    }
    if (declaration_spec_flags[declaration_depth] & 2) goto expression;
    saved_depth = declaration_depth;
    if (parenthesis_lookahead_depth == 1) {
        if ((token_allocation_flags & 0x40000000UL) &&
            deferred_token_cursor == deferred_tokens) {
            compiler_warning(3, 504, 200);
            classify_deferred_type(deferred_token_cursor);
            goto resolved;
        }
        initialize_lookahead_parser();
        result = lookahead_parse();
    } else result = recursive_lookahead_parse();
    declaration_depth = saved_depth;
    deferred_token_cursor = saved_cursor;
    switch (result) {
    case 0:
        goto expression;
    case 1:
        if ((saved_depth > 0 && !deferred_scope_allows_expression(saved_depth)) ||
            !allow_expression || allow_incomplete_class > 0) goto expression;
        break;
    case 5:
        if ((cpp_parser_flags & 0x40) && !saved_depth) goto expression;
        /* The other type-like result follows the same classification path. */
    case 3:
        classify_deferred_type(saved_cursor);
        break;
    }
    goto resolved;
expression:
    convert_deferred_type_token(deferred_token_cursor, saved_type);
resolved:
    token_allocation_flags |= 0x80000000UL;
    if (deferred_token_cursor->lexical_token == 40) {
        declaration_spec_stack[declaration_depth] = deferred_token_cursor->value.declaration;
        declaration_spec_flags[declaration_depth] |= 1;
    } else if (deferred_token_cursor->lexical_token == 166) {
        deferred_token_cursor->value.declaration.storage[0] |= 0x10;
        declaration_spec_stack[declaration_depth] = deferred_token_cursor->value.declaration;
        declaration_spec_flags[declaration_depth] |= 1;
    } else if (deferred_token_cursor->lexical_token != 2)
        resolve_nested_deferred_types(34);
read_token:
    token = scan_token();
    --parenthesis_lookahead_depth;
    return token;
}

LookaheadParserStacks lookahead_stacks; /* original 0x9a178..0x9a75e */
int lookahead_token = -1; /* original 0xa68dc */
int lookahead_error_count; /* original 0xa68e0 */
short lookahead_recovery_count; /* original 0xa68e4 */
short lookahead_state; /* original 0xa68e8 */
short *lookahead_state_top = lookahead_stacks.state.cells; /* original 0xa68ec */
ParserValue *lookahead_value_top = lookahead_stacks.value_cells; /* original 0xa68f0 */

/* Original 0x751f0..0x7522a. Stack arrays retain their contents; only the
   parser controls and the two before-first-cell cursors are reset. */
void initialize_lookahead_parser(void)
{
    lookahead_token = -1;
    lookahead_error_count = 0;
    lookahead_recovery_count = 0;
    lookahead_state = 0;
    lookahead_state_top = lookahead_stacks.state.cells;
    lookahead_value_top = lookahead_stacks.value_cells;
}

/* Original 0x18f20..0x19074. The argument supplies the candidate type, while
   bare-class classification deliberately rewrites the global replay cursor.
   Those two record pointers need not be the same on recursive paths. */
void __stdcall classify_deferred_type(DeferredToken *token)
{
    CType *type = token->value.declaration.type;
    CppSymbol *tag;
    unsigned char replayed;
    if ((type->flags & 0x9c0) || type->related) {
        if (declaration_spec_flags[declaration_depth] & 3) {
            if (token->type_specifier) {
                token->lexical_token = token->parser_token = 2;
                token->value.name = token->type_specifier->name;
            } else convert_deferred_type_token(token, type);
            return;
        }
        if (token->lexical_token == 181 && (token_allocation_flags & 0x80000000UL)) {
            token_allocation_flags &= ~0x80000000UL;
            replayed = resolve_parenthesis_candidate(1);
            unget_token(replayed);
            return;
        }
        set_deferred_type_name(deferred_token_cursor);
        return;
    }
    if (current_class_context >= 0) {
        tag = cpp_context_stack->items[type->field08];
        if (is_current_class(tag)) {
            deferred_token_cursor->lexical_token = deferred_token_cursor->parser_token = 166;
            return;
        }
    } else if (deferred_token_cursor->type_specifier->kind & 0xc0) {
        convert_deferred_type_token(deferred_token_cursor, type);
        return;
    }
    deferred_token_cursor->lexical_token = deferred_token_cursor->parser_token = 2;
    tag = cpp_context_stack->items[type->field08];
    deferred_token_cursor->value.name = tag->name;
    declaration_spec_flags[declaration_depth] |= 2;
}

/* Original 0x18bb0..0x18cf6. Consume deferred tokens while checking
   whether a type-like token can still start a declaration. Parentheses and
   brackets protect interior stars, and an unmatched closing parenthesis
   terminates lookahead without advancing past it. */
int deferred_tokens_may_be_declaration(void)
{
    int parentheses = 0, brackets = 0, may_start_type = 1;
    if (token_allocation_flags & 0x40000000UL) return 1;
    while ((token_allocation_flags & 0x10000000UL) &&
        deferred_token_cursor != deferred_token_end) {
        switch (deferred_token_cursor->parser_token) {
        case 2: case 3: case 34: case 161: case 181:
            if (may_start_type) return 1;
            break;
        case 61:
            may_start_type = 1;
            break;
        case 64:
            if (!parentheses && !brackets) may_start_type = 0;
            break;
        case 74: ++brackets; break;
        case 128: ++parentheses; break;
        case 132: --brackets; break;
        case 136:
            if (--parentheses < 0) return 0;
            break;
        }
        ++deferred_token_cursor;
    }
    return 0;
}

/* Original 0x750f0..0x750f2 is only RET4. The lookahead parser's
   error callback deliberately emits no diagnostic and does not read its text. */
void __stdcall report_lookahead_error(char *unused_text)
{
}

/* Original0x18e90..0x18e9e, stdcall4. Reclassify both token views
   together; the deferred value and type-specifier owner remain intact. */
void __stdcall set_deferred_type_name(DeferredToken *token)
{
    token->lexical_token = token->parser_token = 0x28;
}

unsigned char *__stdcall copy_bytes_forward(unsigned char *destination,
    const unsigned char *source, unsigned bytes);
unsigned char *__stdcall copy_argument_spelling(unsigned char *destination, char *text);
unsigned char *__stdcall copy_argument_type(unsigned char *destination, CType *type);
void __stdcall copy_qualified_argument_name(QualifiedDeclarator *qualified);

/* Original0x195d0..0x19b71, cdecl0/RET0; numeric-class dispatch owns
   seven jump targets19b74..19b90. Capture real tokens into the existing text
   workspace while ordinary preprocessor output is temporarily suppressed. */
unsigned char read_deferred_argument_text(void)
{
    WorkingStack *saved_stack;
    unsigned saved_line, length;
    unsigned char *start;
    unsigned char token;
    int parentheses = 0, braces = 0;
    char *spelling;
    IrToken *number;
    double real;
    token_allocation_flags &= ~0x20;
    saved_stack = compiler_stack;
    compiler_stack = 0;
    saved_line = source_line_number;
    argument_text_mode = 1;
    preprocessor_stream = 0;
    start = text_work_cursor;
    for (;;) {
        token = read_parser_token();
        switch (token) {
        case 1: case 0x93:
            break;
        case 0:
            source_line_number = saved_line;
            fatal_error(502);
            /* The original falls into the left-brace arm if fatal returns. */
        case 0x7f:
            ++braces;
            *text_work_cursor++ = '{';
            break;
        case 2:
            if (token_allocation_flags & 0x10000000UL) {
                text_work_cursor = copy_argument_spelling(text_work_cursor, current_token_value.name);
                *text_work_cursor++ = ' ';
            }
            break;
        case 3: case 0xc3:
            if (token_allocation_flags & 0x10000000UL) {
                if (current_token_value.string.flags & 1) *text_work_cursor++ = 'L';
                *text_work_cursor++ = '"';
                text_work_cursor = copy_bytes_forward(text_work_cursor,
                    (unsigned char *)current_token_value.string.text,
                    (unsigned)current_token_value.string.length - 1);
                *text_work_cursor++ = '"';
            }
            break;
        case 0x28: case 0xb5:
            text_work_cursor = copy_argument_type(text_work_cursor, current_token_value.declaration.type);
            break;
        case 0x4a:
            *text_work_cursor++ = '[';
            break;
        case 0x6f:
            number = current_token_value.expression;
            switch (current_token_code) {
            case 4: case 5: case 6:
                if (!(token_allocation_flags & 0x10000000UL)) break;
#ifdef RECON_NATIVE_TEST
                /* Original ignores failure and then consumes an unwritten
                   double at entryESP-8. Trap before that exact native use. */
                if (extended_to_double(number->value.pointer, &real)) { __asm { int 3 } }
#else
                extended_to_double(number->value.pointer, &real);
#endif
                _gcvt(real, 16, (char *)text_work_cursor);
                while (*text_work_cursor) ++text_work_cursor;
                *text_work_cursor++ = ' ';
                break;
            case 7: case 8:
                if (!(token_allocation_flags & 0x10000000UL)) break;
                if (number->type == type_0041_q1000) {
                    *text_work_cursor++ = '\'';
                    *text_work_cursor++ = '\\';
                    _ltoa(number->value.integer, (char *)text_work_cursor, 8);
                    while (*text_work_cursor) ++text_work_cursor;
                    *text_work_cursor++ = '\'';
                } else {
                    _ltoa(number->value.integer, (char *)text_work_cursor, 10);
                    while (*text_work_cursor) ++text_work_cursor;
                    if (current_token_code == 8) *text_work_cursor++ = 'L';
                }
                *text_work_cursor++ = ' ';
                break;
            case 9: case 10:
                if (!(token_allocation_flags & 0x10000000UL)) break;
                if (number->type == type_4041_q1000) {
                    *text_work_cursor++ = '\'';
                    *text_work_cursor++ = '\\';
                    _ultoa(number->value.integer, (char *)text_work_cursor, 8);
                    while (*text_work_cursor) ++text_work_cursor;
                    *text_work_cursor++ = '\'';
                } else {
                    _ultoa(number->value.integer, (char *)text_work_cursor, 10);
                    while (*text_work_cursor) ++text_work_cursor;
                    /* Preserve the original BL test, not current_token_code. */
                    if (token == 10) *text_work_cursor++ = 'L';
                    /* Original defect: missing increment overwrites this U. */
                    *text_work_cursor = 'U';
                }
                *text_work_cursor++ = ' ';
                break;
            }
            break;
        case 0x80:
            ++parentheses;
            *text_work_cursor++ = '(';
            break;
        case 0x84:
            *text_work_cursor++ = ']';
            break;
        case 0x85:
            --braces;
            *text_work_cursor++ = '}';
            break;
        case 0x88:
            --parentheses;
            /* Fall through to the original comma/parenthesis termination. */
        case 0x40:
            if (parentheses < 0 || (!parentheses && token == 0x40)) goto finished;
            *text_work_cursor++ = token == 0x88 ? ')' : ',';
            break;
        case 0xa1:
            if (token_allocation_flags & 0x10000000UL)
                copy_qualified_argument_name(current_token_value.context);
            break;
        case 0xa3:
            text_work_cursor = copy_argument_type(text_work_cursor, current_token_value.expression->type);
            break;
        default:
            spelling = token_spellings[current_token_code];
            for (length = 0; spelling[length]; ++length) ;
            text_work_cursor = copy_bytes_forward(text_work_cursor, (unsigned char *)spelling, length);
            break;
        }
    }
finished:
    argument_text_mode = 0;
    preprocessor_stream = stdout;
    compiler_stack = saved_stack;
    if (text_work_cursor != start) {
        unget_token(token);
        *text_work_cursor++ = 0;
        token = 0xc3;
        current_token_value.string.length = (unsigned short)(text_work_cursor - start);
        current_token_value.string.text = (char *)start;
        cpp_parser_flags |= 0x00080000UL;
    }
    token_allocation_flags |= 0x20;
    return token;
}
/* Original 0x19540..0x1955e, stdcall8. Append the spelling while
   returning the address of its NUL, ready for the next text fragment. */
unsigned char *__stdcall copy_argument_spelling(unsigned char *destination, char *text)
{
    while ((*destination = (unsigned char)*text++) != 0) ++destination;
    return destination;
}
unsigned char *__stdcall copy_argument_type(unsigned char *destination, CType *type)
{
    __asm { int 3 }
    return 0;
}
void __stdcall copy_qualified_argument_name(QualifiedDeclarator *qualified)
{
    __asm { int 3 }
}

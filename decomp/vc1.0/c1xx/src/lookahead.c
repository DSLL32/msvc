/* Deferred C++ declaration/expression lookahead and token reinterpretation. */
#include "lookahead.h"
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

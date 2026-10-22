/* C++ grammar-token driver, original 0x1af10..0x1b6eb, with its 31-entry
   switch table at 0x1b6ec..0x1b768. The underlying scanner is separate. */
#include "lexer.h"

TokenReplayStorage token_replay; /* original 0x96b8c..0x9783c */
DeferredToken *deferred_token_end = deferred_tokens; /* original 0x9d7c4 */
DeferredToken *deferred_token_cursor = deferred_tokens; /* original 0x9d7c8 */
DeferredToken *pushed_token_top = pushed_tokens; /* original 0x9d7b8 */
DeferredToken *pushed_token_begin = pushed_tokens; /* original 0x9d7bc */
int pushed_token_count; /* original 0x9d7c0 */
unsigned char current_token_code = 1; /* original 0x9fc58: initial newline */
CppSymbol *type_specifier_symbol; /* original 0x94af8 */

#define CAPTURING_DECLARATION (token_allocation_flags & 0x20000000UL)
#define MARK_EXPRESSION_TOKEN() do { \
    if (!CAPTURING_DECLARATION) declaration_spec_flags[declaration_depth] |= 2; \
} while (0)

unsigned char read_parser_token(void)
{
    unsigned char token, storage[3];
    DeferredToken *saved_cursor;
    TypeDescription *description;
    QualifiedDeclarator *qualified;
restart:
    if ((token_allocation_flags & 0x00410000UL) || argument_text_mode)
        return scan_token();
    if (!CAPTURING_DECLARATION && current_class_context >= 0 &&
        (token_allocation_flags & 0x20)) {
        if (!(token_allocation_flags & 0x10000000UL)) {
            capture_declaration_tokens(0);
            token_allocation_flags |= 0x10000008UL;
            declaration_spec_flags[declaration_depth] &= ~0x10;
        }
        saved_cursor = deferred_token_cursor;
        if (!(declaration_spec_flags[declaration_depth] & 0x10)) {
            declaration_spec_flags[declaration_depth] |= 0x10;
            if (deferred_tokens_may_be_declaration())
                declaration_spec_flags[declaration_depth] |= 0x40;
            else declaration_spec_flags[declaration_depth] &= ~0x40;
            deferred_token_cursor = saved_cursor;
        }
        if (declaration_spec_flags[declaration_depth] & 0x40)
            return read_deferred_argument_text();
    }
    token = scan_token();
    storage[2] = declaration_storage[2];
    switch (token) {
    case TOK_CLASS:
    case TOK_ENUM:
    case TOK_STRUCT:
    case TOK_UNION:
    case TOK_SEGMENT:
    case 35: /* resolved type */
    case 36: /* storage class */
    case 37: /* storage modifier */
    case TOK_DECLSPEC:
        return read_type_specifier(token);
    case 2: /* identifier */
        token = resolve_typename_token(0);
        if (token == 34) return read_type_specifier(34);
        MARK_EXPRESSION_TOKEN();
        return token;
    case TOK_VIRTUAL:
        if (!(token_allocation_flags & 2)) return read_type_specifier(token);
        current_token_code = TOK_VIRTUAL;
        current_token_value.bytes[0] = TOK_VIRTUAL;
        return TOK_VIRTUAL;
    case TOK_CONST:
        if (!(token_allocation_flags & 4)) return read_type_specifier(token);
        return 174; /* conversion const */
    case TOK_VOLATILE:
        if (!(token_allocation_flags & 4)) return read_type_specifier(token);
        return 175; /* conversion volatile */
    case 40: /* completed type specifier */
        if ((declaration_spec_flags[declaration_depth] & 2) &&
            (token_allocation_flags & 0x10000000UL) && type_specifier_symbol) {
            current_token_value.name = type_specifier_symbol->name;
            current_token_code = 2;
            return 2;
        }
        token_allocation_flags |= 1;
        MARK_EXPRESSION_TOKEN();
        return token;
    case TOK_BASED:
        MARK_EXPRESSION_TOKEN();
        cpp_parser_flags |= 1;
        return (token_allocation_flags & 4) ? 176 : TOK_BASED;
    case OP_MULTIPLY:
        MARK_EXPRESSION_TOKEN();
        return (token_allocation_flags & 4) ? 171 : OP_MULTIPLY;
    case OP_BIT_AND:
        MARK_EXPRESSION_TOKEN();
        return (token_allocation_flags & 4) ? 172 : OP_BIT_AND;
    case TOK_MODIFIER:
        MARK_EXPRESSION_TOKEN();
        return (token_allocation_flags & 4) ? 173 : TOK_MODIFIER;
    case OP_COMPLEMENT:
        token_allocation_flags |= 1;
        /* The operand still starts an expression. */
    case OP_NOT:
    case OP_SUBTRACT:
    case OP_ADD:
    case TOK_SEGNAME:
    case 111: /* constant */
    case 125: /* increment operator */
    case TOK_NEW:
    case TOK_DELETE:
    case 153: /* operator identifier */
    case 154: /* conversion identifier */
    case TOK_THIS:
    case 163: /* simple-type-name call */
        MARK_EXPRESSION_TOKEN();
        return token;
    case 128: /* left parenthesis */
        if (!(token_allocation_flags & 0x200)) {
            if (!CAPTURING_DECLARATION) {
                if (++declaration_depth >= 60) fatal_error(13);
                declaration_spec_flags[declaration_depth] &= 0xa0;
                deferred_declarations[declaration_depth] = 0;
            }
            token_allocation_flags &= ~1;
        }
        return token;
    case TOK_RETURN:
        if (!CAPTURING_DECLARATION) declaration_spec_flags[declaration_depth] |= 3;
        return token;
    case 136: /* right parenthesis */
        token_allocation_flags &= ~1;
        if (!declaration_depth && !CAPTURING_DECLARATION && !(token_allocation_flags & 0x200)) {
            report_syntax_error(59, token_spellings[136]);
            goto restart;
        }
        return token;
    case TOK_SIZEOF:
        if (!CAPTURING_DECLARATION) {
            ++allow_incomplete_class;
            declaration_spec_flags[declaration_depth] |= 2;
        }
        return token;
    case 182: /* const/volatile pair */
        if (!(declaration_spec_flags[declaration_depth] & 2)) {
            storage[0] = declaration_storage[0];
            storage[1] = declaration_storage[1];
            MARK_EXPRESSION_TOKEN();
            description = copy_type_description(type_0064);
            description->attributes = current_token_value.modifier;
            current_token_value.declaration.type = intern_type(description);
            --type_description_depth;
            current_token_value.declaration.storage[0] = (storage[0] & ~7) | 0x30;
            current_token_value.declaration.storage[1] = storage[1];
            current_token_value.declaration.storage[2] = storage[2];
            current_token_code = 40;
            return 40;
        }
        return token;
    case TOK_NCTAG:
        token = scan_token();
        if (token != 2) {
            unget_token(token);
            current_token_value.name = 0;
        }
        return TOK_NCTAG;
    case 181: /* ambiguous type/expression candidate */
        unget_token(token);
        return resolve_parenthesis_candidate(!(declaration_depth &&
            !(declaration_spec_flags[declaration_depth-1] & 1) &&
            (declaration_depth < 1 || !(declaration_spec_flags[declaration_depth-1] & 4)) &&
            (token_allocation_flags & 0x00080000UL) && !(token_allocation_flags & 0x40)));
    case TOK_OPERATOR:
        token = read_operator_function_id();
        MARK_EXPRESSION_TOKEN();
        return token;
    case 160: /* scope operator */
        unget_token(160);
        token = resolve_identifier_token(0);
        if (token != 161) {
            MARK_EXPRESSION_TOKEN();
            return token;
        }
        /* Qualified lookup follows the same type-name check as token 161. */
    case 161:
        qualified = current_token_value.context;
        token = resolve_typename_token((CppSymbol *)qualified);
        if (token != 34) {
            MARK_EXPRESSION_TOKEN();
            if (token_allocation_flags & 0x10000000UL)
                restore_qualified_lookup(qualified->resolution->binding);
            return token;
        }
        unwind_qualified_scopes(1);
        return read_type_specifier(34);
    }
    return token;
}

/* Original 0x1b770..0x1b888. A captured grammar token retains its raw lexical
   class and value. Only forward deferred replay restores the type-specifier
   symbol; the pushback path deliberately leaves that global unchanged. */
unsigned char scan_token(void)
{
    DeferredToken *record;
    for (;;) {
        if (pushed_token_count < 1 && !(token_allocation_flags & 0x10000000UL))
            return read_lexical_token();
        if (!(token_allocation_flags & 0x10000000UL)) {
            record = --pushed_token_top;
            --pushed_token_count;
            if (!pushed_token_count && !(token_allocation_flags & 0x10000000UL)) {
                pushed_token_begin = pushed_tokens;
                pushed_token_top = pushed_tokens;
                pushed_token_count = 0;
            }
            goto have_token;
        }
        if (deferred_token_cursor != deferred_token_end)
            break;
        if (!(token_allocation_flags & 0x80000008UL)) {
            fatal_error(504);
            break;
        }
        token_allocation_flags &= ~0x10000000UL;
        deferred_token_end = deferred_tokens;
        deferred_token_cursor = deferred_tokens;
    }
    record = deferred_token_cursor++;
    type_specifier_symbol = record->type_specifier;
have_token:
    current_token_code = record->lexical_token;
    current_token_value = record->value;
    return record->parser_token;
}

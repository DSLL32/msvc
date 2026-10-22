/* C++ grammar-token driver, original 0x1af10..0x1b6eb, with its 31-entry
   switch table at 0x1b6ec..0x1b768. The underlying scanner is separate. */
#include "lexer.h"
#include "source-input.h"
#include "characters.h"

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

unsigned char pch_output_written; /* original 0x9fabc */

#define NEXT_CHARACTER_CLASS() character_class[(unsigned char)*source_text_cursor++]
#define EMIT_LEXICAL_TOKEN(code_) do { \
    current_token_code = (unsigned char)(code_); \
    current_token_value.bytes[0] = current_token_code; \
    return token_categories[current_token_code]; \
} while (0)
/* /E can write either to a FILE or to the frontend's paged text stack. These
   are macro expansions because the original scanner calls the two writers
   directly. The character argument is evaluated only on the selected path. */
#define OUTPUT_CHARACTER(character_) do { \
    if (preprocessor_stream) fputc((character_), preprocessor_stream); \
    else write_work_character((character_), compiler_stack); \
} while (0)
#define OUTPUT_BYTES(source_, bytes_) do { \
    if (preprocessor_stream) fwrite((source_), (bytes_), 1, preprocessor_stream); \
    else write_work_bytes((source_), (bytes_), 1, compiler_stack); \
} while (0)

/* Original 0x1b890..0x1ca50; 46-way class table 0x1ca50..0x1cb08.
   The token value's unused bytes deliberately survive punctuation tokens. */
unsigned char read_lexical_token(void)
{
    unsigned char class, lookahead, code;
    unsigned flags, bytes;
    int wide;
    MacroDefinition *macro;
    char *text;
    if (!source_text_cursor) fatal_error(4);
next_character:
    class = NEXT_CHARACTER_CLASS();
dispatch:
    switch (class) {
    case CC_CARRIAGE_RETURN:
        goto next_character;
    case CC_SPACE:
        if (options.option_E) {
            do {
                OUTPUT_CHARACTER((unsigned char)source_text_cursor[-1]);
            } while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE);
            --source_text_cursor;
        } else {
            while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE)
                ;
            --source_text_cursor;
        }
        goto next_character;
    case CC_SLASH:
        if (skip_source_comment()) goto next_character;
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_DIVIDE_ASSIGN);
        code = OP_DIVIDE;
        goto finish_operator;
    case CC_STAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_MULTIPLY_ASSIGN);
        if (lookahead == CC_SLASH && !options.option_E) compiler_warning(1, 138);
        code = OP_MULTIPLY;
        goto finish_operator;
    case CC_BANG:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_NOT_EQUAL);
        code = OP_NOT;
        goto finish_operator;
    case CC_PERCENT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_REMAINDER_ASSIGN);
        code = OP_REMAINDER;
        goto finish_operator;
    case CC_EQUAL:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_EQUAL);
        code = OP_ASSIGN;
        goto finish_operator;
    case CC_CARET:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_XOR_ASSIGN);
        code = OP_BIT_XOR;
        goto finish_operator;
    case CC_BAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_OR_ASSIGN);
        if (lookahead == CC_BAR) EMIT_LEXICAL_TOKEN(OP_LOGICAL_OR);
        code = OP_BIT_OR;
        goto finish_operator;
    case CC_AMPERSAND:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_AND_ASSIGN);
        if (lookahead == CC_AMPERSAND) EMIT_LEXICAL_TOKEN(OP_LOGICAL_AND);
        code = OP_BIT_AND;
        goto finish_operator;
    case CC_PLUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_ADD_ASSIGN);
        if (lookahead == CC_PLUS) EMIT_LEXICAL_TOKEN(OP_POST_INCREMENT);
        code = OP_ADD;
        goto finish_operator;
    case CC_MINUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_SUBTRACT_ASSIGN);
        if (lookahead == CC_MINUS) EMIT_LEXICAL_TOKEN(OP_POST_DECREMENT);
        if (lookahead == CC_GREATER) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_STAR) EMIT_LEXICAL_TOKEN(OP_MEMBER_POINTER);
            class = CC_ARROW;
            code = OP_ARROW;
        } else code = OP_SUBTRACT;
        goto finish_operator;
    case CC_LESS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_LESS_EQUAL);
        if (lookahead == CC_LESS) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_SHIFT_LEFT_ASSIGN);
            class = CC_LEFT_SHIFT;
            code = OP_SHIFT_LEFT;
        } else code = OP_LESS;
        goto finish_operator;
    case CC_GREATER:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_GREATER_EQUAL);
        if (lookahead == CC_GREATER) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_SHIFT_RIGHT_ASSIGN);
            class = CC_RIGHT_SHIFT;
            code = OP_SHIFT_RIGHT;
        } else code = OP_GREATER;
        goto finish_operator;
    case CC_LEFT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_SHIFT_LEFT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(OP_SHIFT_LEFT);
    case CC_RIGHT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(OP_SHIFT_RIGHT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(OP_SHIFT_RIGHT);
    case CC_ARROW:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_STAR) EMIT_LEXICAL_TOKEN(OP_MEMBER_POINTER);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(OP_ARROW);
    case CC_COLON:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_GREATER) EMIT_LEXICAL_TOKEN(104); /* :> */
        if (lookahead == CC_COLON) EMIT_LEXICAL_TOKEN(160); /* :: */
        code = 63;
        goto finish_operator;
    case CC_RIGHT_BRACE: EMIT_LEXICAL_TOKEN(133);
    case CC_RIGHT_BRACKET: EMIT_LEXICAL_TOKEN(132);
    case CC_COMMA: EMIT_LEXICAL_TOKEN(64);
    case CC_RIGHT_PAREN: EMIT_LEXICAL_TOKEN(136);
    case CC_LEFT_BRACE: EMIT_LEXICAL_TOKEN(127);
    case CC_LEFT_BRACKET: EMIT_LEXICAL_TOKEN(OP_SUBSCRIPT);
    case CC_LEFT_PAREN: EMIT_LEXICAL_TOKEN(OP_CALL);
    case CC_SEMICOLON: EMIT_LEXICAL_TOKEN(138);
    case CC_TILDE: EMIT_LEXICAL_TOKEN(OP_COMPLEMENT);
    case CC_ESCAPE_OR_END:
        switch ((unsigned char)source_text_cursor[-1]) {
        case '\\':
            if (!options.option_E) {
                if (!consume_line_escape()) report_syntax_error(17);
            } else {
                OUTPUT_CHARACTER('\\');
                OUTPUT_CHARACTER(next_source_character());
            }
            goto next_character;
        case '?':
            if (!(recover_trigraph() & 0x40)) goto next_character;
            EMIT_LEXICAL_TOKEN(95);
        }
        if (macro_expansion_active) {
            recover_special_character();
            goto next_character;
        }
        if (!fill_source_buffer() || close_source_file()) goto next_character;
        if (options.pch_create && !pch_output_written) {
            prepare_pch_output(options.source_file);
            finish_pch_output();
        }
        if (options.pch_create_file && !pch_output_written)
            report_syntax_error(857, options.pch_create_file);
        if (!(cpp_parser_flags & 0x200)) {
            cpp_parser_flags |= 0x200;
            finish_translation_symbols();
        }
        if (!emit_pending_runtime_functions()) EMIT_LEXICAL_TOKEN(119);
        browser_filename = 0;
        flush_scanned_declaration(0);
        if (!source_text_cursor) EMIT_LEXICAL_TOKEN(119);
        goto next_character;
    case CC_NEWLINE:
        if (token_allocation_flags & 0x10000) {
            --source_text_cursor;
            current_token_code = 1;
            return 1;
        }
        if (token_allocation_flags & 0x400000) {
            current_token_code = 1;
            return 1;
        }
        if (options.option_E) {
            OUTPUT_CHARACTER('\n');
            if (token_allocation_flags & 0x800000) {
                token_allocation_flags &= ~0x800000UL;
                if (!options.configuration.option_EP) {
                    emit_line_directive();
                    OUTPUT_CHARACTER('\n');
                }
            }
        }
        begin_source_line();
        goto next_character;
    case CC_HASH:
        if (!options.option_E) {
            report_syntax_error(14);
            skip_source_line();
        } else if (preprocessor_stream)
            fwrite("#", 1, 1, preprocessor_stream);
        else
            write_work_bytes("#", 1, 1, compiler_stack);
        goto next_character;
    case CC_MACRO_END:
        ++source_text_cursor;
        goto next_character;
    case CC_APOSTROPHE:
        wide = 0;
        goto character_literal;
    case CC_QUOTE:
        wide = 0;
        goto string_literal;
    case CC_WIDE_PREFIX:
wide_prefix:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_ESCAPE_OR_END) {
            flags = recover_special_character();
            if (flags & ~0x70U) goto wide_prefix;
            if (flags & 0x20) ++source_text_cursor;
        } else if (lookahead == CC_APOSTROPHE) {
            wide = 1;
            goto character_literal;
        } else if (lookahead == CC_QUOTE) {
            wide = 1;
            goto string_literal;
        }
        --source_text_cursor;
        source_text_cursor[-1] = 'L';
        /* An L not followed by a quote starts an ordinary identifier. */
    case CC_IDENTIFIER:
        if (token_allocation_flags & 0x400) {
            scan_source_identifier();
            code = (SYMBOL_FROM_NAME(interned_name)->flags & 1) ?
                KEYWORD_FROM_NAME(interned_name)->token : 2;
            if (code != 2 && !(cpp_parser_flags & 0x10000000UL)) {
                current_token_code = code;
                current_token_value.bytes[0] = code;
                return token_categories[code];
            }
            current_token_value.name = interned_name;
            current_token_code = 2;
            return 2;
        }
        if (macro_expansion_active > 0) {
            if (!scan_macro_identifier()) goto identifier_ready;
        } else scan_source_identifier();
        macro = lookup_macro();
        if (macro && !macro->flags && expand_macro(macro)) goto next_character;
identifier_ready:
        if (options.option_E) {
            for (text = interned_name; *text; ++text)
                OUTPUT_CHARACTER((signed char)*text);
            current_token_code = 147;
            return 147;
        }
        code = (SYMBOL_FROM_NAME(interned_name)->flags & 1) ?
            KEYWORD_FROM_NAME(interned_name)->token : 2;
        if (code == 2 || (cpp_parser_flags & 0x10000000UL)) {
            current_token_value.name = interned_name;
            current_token_code = 2;
            return 2;
        }
        current_token_code = code;
        current_token_value.bytes[0] = code;
        code = token_categories[code];
        if (code == 1) {
            compiler_warning(1, 226, interned_name);
            goto next_character;
        }
        return code;
    case CC_DOT:
dot_lookahead:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_ESCAPE_OR_END) {
            flags = recover_special_character();
            if (flags & ~0x70U) goto dot_lookahead;
            if (flags & 0x20) ++source_text_cursor;
        } else if (lookahead == CC_STAR) {
            EMIT_LEXICAL_TOKEN(157); /* .* */
        } else if (lookahead == CC_DOT) {
            if (match_source_character('.')) EMIT_LEXICAL_TOKEN(116); /* ... */
            source_text_cursor[-1] = '.';
        } else if (lookahead == CC_DIGIT) {
            token_text[0] = '.';
            token_text[1] = source_text_cursor[-1];
            if (!(token_allocation_flags & 0x8000) &&
                (options.option_E || (token_allocation_flags & 0x1000000))) {
                bytes = read_number_text(token_text + 2);
                if (options.option_E) OUTPUT_BYTES(token_text, bytes);
                current_token_code = 147;
                return token_categories[147];
            }
            current_token_code = scan_float_tail(token_text + 2);
            return token_categories[current_token_code];
        }
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(89); /* . */
    case CC_DIGIT:
        if (!(token_allocation_flags & 0x8000) &&
            (options.option_E || (token_allocation_flags & 0x1000000))) {
            token_text[0] = source_text_cursor[-1];
            bytes = read_number_text(token_text + 1);
            if (options.option_E) OUTPUT_BYTES(token_text, bytes);
            current_token_code = 147;
            return token_categories[147];
        }
        if ((token_allocation_flags & 0x400000) && !(token_allocation_flags & 0x10000))
            current_token_code = scan_assembler_number((unsigned char)source_text_cursor[-1]);
        else
            current_token_code = scan_number((unsigned char)source_text_cursor[-1]);
        return token_categories[current_token_code];
    default:
        if (!options.option_E)
            report_syntax_error(18, (unsigned char)source_text_cursor[-1]);
        else
            OUTPUT_CHARACTER((unsigned char)source_text_cursor[-1]);
        goto next_character;
    }
finish_operator:
    if (lookahead == CC_ESCAPE_OR_END) {
        flags = recover_special_character();
        if (flags & ~0x70U) goto dispatch;
        if (flags & 0x20) ++source_text_cursor;
    }
    --source_text_cursor;
    EMIT_LEXICAL_TOKEN(code);
character_literal:
    if (options.option_E) {
        if (wide) OUTPUT_CHARACTER('L');
        copy_quoted_literal('\'');
        goto next_character;
    }
    current_token_code = scan_character_literal(wide);
    return token_categories[current_token_code];
string_literal:
    if (options.option_E) {
        if (wide) OUTPUT_CHARACTER('L');
        copy_quoted_literal('"');
        goto next_character;
    }
    scan_string_literal(wide, '"');
    current_token_code = 3;
    return 3;
}

/* Original 0x25180..0x2530b, including its 12-way class table through 0x2533c. */
void begin_source_line(void)
{
    unsigned char class, character;
    ++source_line_number;
    for (;;) {
        class = NEXT_CHARACTER_CLASS();
        switch (class) {
        case CC_CARRIAGE_RETURN:
            continue;
        case CC_SLASH:
            if (skip_source_comment()) continue;
            goto line_ready;
        case CC_ESCAPE_OR_END:
            character = (unsigned char)source_text_cursor[-1];
            if (!character || character == 0x1a) {
                if (!fill_source_buffer()) continue;
                if (pch_mode && !include_depth) finish_pch_capture();
                return;
            }
            if (character == '?') {
                if (!(recover_trigraph() & ~0x40U)) goto line_ready;
                continue;
            }
            if (consume_line_escape()) continue;
            goto line_ready;
        case CC_NEWLINE:
            ++source_line_number;
            /* A blank line falls through to leading-whitespace output. */
        case CC_SPACE:
            if (options.option_E)
                OUTPUT_CHARACTER((unsigned char)source_text_cursor[-1]);
            else {
                while (character_flags[(unsigned char)*source_text_cursor++] & CF_SPACE)
                    ;
                --source_text_cursor;
            }
            continue;
        case CC_HASH:
            browser_file_cookie = 1;
            preprocess_directive();
            continue;
        default:
            goto line_ready;
        }
    }
line_ready:
    if (options.configuration.option_ide) report_ide_line();
    browser_file_cookie = 1;
    --source_text_cursor;
    if ((token_allocation_flags & 0x80000) && !(token_allocation_flags & 0x40))
        record_line_information(1);
    if (pch_mode && !include_depth) finish_pch_capture();
}

/* Original 0x1dba0..0x1dd1e. Fast path interns directly from the current input
   window; a terminator, trigraph or splice switches to the copying path. */
void scan_source_identifier(void)
{
    char *begin = --source_text_cursor;
    unsigned char character;
    unsigned hash = 0, length, i, remaining;
    PreprocessorSymbol **bucket, **previous, *symbol;
    character = (unsigned char)*source_text_cursor++;
    while (character_flags[character] & CF_IDENTIFIER) {
        hash = hash * 4 + character + (hash >> 7);
        character = (unsigned char)*source_text_cursor++;
    }
    --source_text_cursor;
    if (!character || character == '?' || character == '\\') {
        scan_identifier_continuation(begin, hash);
        return;
    }
    length = source_text_cursor - begin;
    bucket = &preprocessor_buckets[hash & 0x7ff];
    previous = bucket;
    for (symbol = *bucket; symbol; symbol = symbol->next) {
        for (i = 0; i < length && symbol->name[i] == begin[i]; ++i)
            ;
        if (i == length && !symbol->name[length]) {
            *previous = symbol->next;
            symbol->next = *bucket;
            *bucket = symbol;
            interned_name = symbol->name;
            return;
        }
        previous = &symbol->next;
    }
    remaining = macro_text_begin - macro_text_next + 8192;
    if (remaining < length + 12) initialize_macro_text();
    symbol = (PreprocessorSymbol *)macro_text_next;
    symbol->next = *bucket;
    *bucket = symbol;
    for (i = 0; i < length; ++i) symbol->name[i] = begin[i];
    symbol->name[length] = 0;
    interned_name = symbol->name;
    macro_text_next += length + 6;
}

/* Original 0x18d00..0x18d79. Deferred replay rewinds its existing record;
   ordinary lookahead stores a new record in the three-token pushback area. */
void __stdcall unget_token(unsigned char token)
{
    if (token_allocation_flags & 0x10000000UL) {
        --deferred_token_cursor;
        return;
    }
    ++pushed_token_count;
    pushed_token_top->parser_token = token;
    pushed_token_top->lexical_token = current_token_code;
    pushed_token_top->value = current_token_value;
    pushed_token_top->type_specifier = type_specifier_symbol;
    ++pushed_token_top;
    if (pushed_token_top >= pushed_tokens + 3)
        token_allocation_flags |= 0x40000000UL;
}

/* Original 0x1aae0..0x1ad28. Recognize type names without losing the
   qualified wrapper needed by deferred lookup and browser reporting. */
unsigned char __stdcall resolve_typename_token(CppSymbol *input)
{
    unsigned char token;
    int resolved_name = 0;
    CppSymbol *symbol, *found, *browser_symbol;
    QualifiedDeclarator *qualified = 0;
    QualifierResolution *resolution = 0;
    if (!input) {
        token = resolve_identifier_token(current_token_value.name);
        if (token != 2) {
            if (token != 161) return token;
            resolved_name = 1;
            input = current_token_value.symbol;
        }
    } else token = 161;
    if (!(cpp_parser_flags & 1)) {
        if (token_allocation_flags & 1) {
            token_allocation_flags &= ~1UL;
            return token;
        }
        if ((declaration_spec_flags[declaration_depth] & 2) ||
            (token_allocation_flags & 0x80)) return token;
    }
    if (token == 2) {
        if (!(((unsigned char *)current_token_value.name)[-1] & 2)) return token;
        input = find_outer_name(current_token_value.name);
    }
    if (!input) return token;
    symbol = input;
    if ((symbol->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)symbol;
        resolution = qualified->resolution;
        symbol = (CppSymbol *)resolution->binding->target;
    }
    if ((symbol->kind & 31) == 15)
        symbol = cpp_context_stack->items[((BaseClassLink *)symbol)->context_index];
    if (!(symbol->flags & 0x10)) return token;
    if (symbol->kind & 0x80) {
        found = find_tag_declaration(symbol);
        if (resolved_name) {
            qualified->resolution->binding->target = (ClassReference *)found;
            current_token_value.symbol = (CppSymbol *)qualified;
            token = 161;
        }
        return token;
    }
    if (resolved_name) {
        if (!options.configuration.extensions_enabled) resolve_deferred_declaration(qualified);
        else deferred_declarations[declaration_depth] = copy_qualified_declarator(qualified, 1);
    } else deferred_declarations[declaration_depth] = 0;
    if ((symbol->kind & 31) == 4 && (declaration_spec_flags[declaration_depth] & 1) &&
        !(cpp_parser_flags & 0x20)) return 34;
    if (browser_filename && (!declaration_mode || options.option_FR)) {
        browser_symbol = symbol;
        if ((symbol->kind & 31) == 18)
            browser_symbol = (CppSymbol *)((QualifiedDeclarator *)symbol)->resolution->binding->target;
        browser_type_reference(browser_symbol);
    }
    if (resolved_name && resolution && resolution->binding->next) unwind_qualified_scopes(1);
    current_token_value.symbol = qualified ? (CppSymbol *)qualified : symbol;
    if ((token_allocation_flags & 0x80000000UL) &&
        (token_allocation_flags & 0x10000000UL))
        deferred_token_cursor[-1].type_specifier = current_token_value.symbol;
    return 34;
}

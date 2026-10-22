/* Lexical token pushback and source dispatch. */
#include "lexer.h"
#include "source-input.h"
#include "characters.h"
#include "parser.h"

struct FrontendSymbol *last_lookup_symbol; /* original 0x604b8 */
typedef char LexicalToken_must_be_sixteen_bytes[(sizeof(LexicalToken) == 16) ? 1 : -1];
unsigned scanner_flags; /* original 0x6bb44 */
unsigned char current_token_code; /* original 0x6c008 */
TokenValue current_token_value; /* original 0x5b150..0x5b158 */
LexicalToken initial_token_stack[3]; /* original 0x64d48..0x64d78 */
LexicalToken *token_stack_top = initial_token_stack; /* original 0x6a7b8 */
LexicalToken *token_stack_read = initial_token_stack; /* original 0x6a7bc */
int token_stack_count; /* original 0x6a7c0 */

/* Original 0x199a0..0x19a3e. */
unsigned char scan_token(void)
{
    LexicalToken *token;
    if (token_stack_count <= 0 && !(scanner_flags & SCANNER_SUPPRESS_POP))
        return scan_source_token();
    if (!(scanner_flags & SCANNER_SUPPRESS_POP)) {
        token = --token_stack_top;
        if (!--token_stack_count) {
            token_stack_read = initial_token_stack;
            token_stack_top = initial_token_stack;
            token_stack_count = 0;
        }
    }
    /* The original suppress-pop branch reloads an uninitialized automatic
       pointer. Its callers must leave that mode clear on entry here. */
    current_token_code = token->code;
    current_token_value = token->value;
    return token->category;
}

unsigned parser_flags; /* original 0x6bb48 */
unsigned char pch_output_written; /* original 0x6be74 */
LexerTextStorage lexer_text_storage; /* original 0x5b158..0x5b1dc */

#define NEXT_CHARACTER_CLASS() \
    character_class[(unsigned char)*source_text_cursor++]
#define EMIT_LEXICAL_TOKEN(code_) do { \
    current_token_code = (unsigned char)(code_); \
    current_token_value.bytes[0] = current_token_code; \
    return token_categories[current_token_code]; \
} while (0)

/* Original 0x19a40..0x1a8b8; class-dispatch table 0x1a8b8..0x1a970. */
unsigned char scan_source_token(void)
{
    unsigned char class, lookahead, code;
    unsigned flags, bytes;
    int wide;
    MacroDefinition *macro;
    char *text;
    if (!source_text_cursor)
        fatal_error(4);
next_character:
    class = NEXT_CHARACTER_CLASS();
dispatch:
    switch (class) {
    case CC_CARRIAGE_RETURN:
        goto next_character;
    case CC_SPACE:
        if (options.option_E) {
            do {
                fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
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
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_DIVIDE_ASSIGN);
        code = TOK_DIVIDE;
        goto finish_operator;
    case CC_STAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_MULTIPLY_ASSIGN);
        if (lookahead == CC_SLASH && !options.option_E)
            compiler_warning(1, 138);
        code = TOK_MULTIPLY;
        goto finish_operator;
    case CC_BANG:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_NOT_EQUAL);
        code = TOK_LOGICAL_NOT;
        goto finish_operator;
    case CC_PERCENT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_REMAINDER_ASSIGN);
        code = TOK_REMAINDER;
        goto finish_operator;
    case CC_EQUAL:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_EQUAL);
        code = TOK_ASSIGN;
        goto finish_operator;
    case CC_CARET:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_XOR_ASSIGN);
        code = TOK_BIT_XOR;
        goto finish_operator;
    case CC_BAR:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_OR_ASSIGN);
        if (lookahead == CC_BAR) EMIT_LEXICAL_TOKEN(TOK_LOGICAL_OR);
        code = TOK_BIT_OR;
        goto finish_operator;
    case CC_AMPERSAND:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_AND_ASSIGN);
        if (lookahead == CC_AMPERSAND) EMIT_LEXICAL_TOKEN(TOK_LOGICAL_AND);
        code = TOK_BIT_AND;
        goto finish_operator;
    case CC_PLUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_PLUS_ASSIGN);
        if (lookahead == CC_PLUS) EMIT_LEXICAL_TOKEN(TOK_PREINCREMENT);
        code = TOK_PLUS;
        goto finish_operator;
    case CC_MINUS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_MINUS_ASSIGN);
        if (lookahead == CC_MINUS) EMIT_LEXICAL_TOKEN(TOK_PREDECREMENT);
        if (lookahead == CC_GREATER) EMIT_LEXICAL_TOKEN(TOK_ARROW);
        code = TOK_MINUS;
        goto finish_operator;
    case CC_LESS:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LESS_EQUAL);
        if (lookahead == CC_LESS) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT_ASSIGN);
            class = CC_LEFT_SHIFT;
            code = TOK_LEFT_SHIFT;
        } else code = TOK_LESS;
        goto finish_operator;
    case CC_GREATER:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_GREATER_EQUAL);
        if (lookahead == CC_GREATER) {
            lookahead = NEXT_CHARACTER_CLASS();
            if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT_ASSIGN);
            class = CC_RIGHT_SHIFT;
            code = TOK_RIGHT_SHIFT;
        } else code = TOK_GREATER;
        goto finish_operator;
    case CC_LEFT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_LEFT_SHIFT);
    case CC_RIGHT_SHIFT:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_EQUAL) EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT_ASSIGN);
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_RIGHT_SHIFT);
    case CC_COLON:
        lookahead = NEXT_CHARACTER_CLASS();
        if (lookahead == CC_GREATER) EMIT_LEXICAL_TOKEN(TOK_SEGMENT_OFFSET);
        code = TOK_COLON;
        goto finish_operator;
    case CC_RIGHT_BRACE: EMIT_LEXICAL_TOKEN(TOK_RIGHT_BRACE);
    case CC_RIGHT_BRACKET: EMIT_LEXICAL_TOKEN(TOK_RIGHT_BRACKET);
    case CC_COMMA: EMIT_LEXICAL_TOKEN(TOK_COMMA);
    case CC_RIGHT_PAREN: EMIT_LEXICAL_TOKEN(TOK_RIGHT_PAREN);
    case CC_LEFT_BRACE: EMIT_LEXICAL_TOKEN(TOK_LEFT_BRACE);
    case CC_LEFT_BRACKET: EMIT_LEXICAL_TOKEN(TOK_LEFT_BRACKET);
    case CC_LEFT_PAREN: EMIT_LEXICAL_TOKEN(TOK_LEFT_PAREN);
    case CC_SEMICOLON: EMIT_LEXICAL_TOKEN(TOK_SEMICOLON);
    case CC_TILDE: EMIT_LEXICAL_TOKEN(TOK_BIT_NOT);
    case CC_ESCAPE_OR_END:
        switch ((unsigned char)source_text_cursor[-1]) {
        case '\\':
            if (!options.option_E) {
                if (!consume_line_escape()) report_syntax_error(17);
            } else {
                fputc('\\', preprocessor_stream);
                fputc(next_source_character(), preprocessor_stream);
            }
            goto next_character;
        case '?':
            if (!(recover_trigraph() & 0x40)) goto next_character;
            EMIT_LEXICAL_TOKEN(TOK_QUESTION);
        }
        if (macro_expansion_active) {
            recover_special_character();
            goto next_character;
        }
        if (!fill_source_buffer()) goto next_character;
        if (close_source_file()) goto next_character;
        if (options.pch_create && !pch_output_written) {
            prepare_pch_output(options.source_file);
            finish_pch_output();
        }
        if (options.pch_create_file && !pch_output_written)
            report_syntax_error(857, options.pch_create_file);
        parser_flags |= 0x200;
        EMIT_LEXICAL_TOKEN(TOK_END_OF_FILE);
    case CC_NEWLINE:
        if (scanner_flags & 0x10000) {
            --source_text_cursor;
            current_token_code = TOK_NEWLINE;
            return TOK_NEWLINE;
        }
        if (scanner_flags & 0x400000) {
            current_token_code = TOK_NEWLINE;
            return TOK_NEWLINE;
        }
        if (options.option_E) {
            fputc('\n', preprocessor_stream);
            if (scanner_flags & 0x800000) {
                scanner_flags &= ~0x800000;
                if (!options.option_EP) {
                    emit_line_directive();
                    fputc('\n', preprocessor_stream);
                }
            }
        }
        begin_source_line();
        goto next_character;
    case CC_HASH:
        if (!options.option_E) {
            report_syntax_error(14);
            skip_source_line();
        } else
            fwrite("#", 1, 1, preprocessor_stream);
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
        /* A non-literal L remains an identifier's first character. */
    case CC_IDENTIFIER:
        if (macro_expansion_active > 0) {
            if (!scan_macro_identifier()) goto identifier_ready;
        } else
            scan_source_identifier();
        macro = lookup_macro();
        if (macro && !macro->disabled && expand_macro(macro))
            goto next_character;
    identifier_ready:
        if (options.option_E) {
            for (text = interned_name; *text; ++text)
                fputc((signed char)*text, preprocessor_stream);
            current_token_code = TOK_PP_OUTPUT;
            return TOK_PP_OUTPUT;
        }
        code = (SYMBOL_FROM_NAME(interned_name)->flags & 1) ?
            KEYWORD_FROM_NAME(interned_name)->token : TOK_IDENTIFIER;
        if (code == TOK_IDENTIFIER || (parser_flags & 0x10000000)) {
            current_token_value.identifier = interned_name;
            current_token_code = TOK_IDENTIFIER;
            return TOK_IDENTIFIER;
        }
        current_token_code = code;
        current_token_value.bytes[0] = code;
        code = token_categories[code];
        if (code == TOK_NEWLINE) {
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
        } else if (lookahead == CC_DOT) {
            if (match_source_character('.')) EMIT_LEXICAL_TOKEN(TOK_ELLIPSIS);
            source_text_cursor[-1] = '.';
        } else if (lookahead == CC_DIGIT) {
            token_text[0] = '.';
            token_text[1] = source_text_cursor[-1];
            if (!(scanner_flags & 0x8000) && (options.option_E || (scanner_flags & 0x1000000))) {
                bytes = read_number_text(token_text + 2);
                if (options.option_E) fwrite(token_text, bytes, 1, preprocessor_stream);
                current_token_code = TOK_PP_OUTPUT;
                return token_categories[TOK_PP_OUTPUT];
            }
            current_token_code = scan_float_tail(token_text + 2);
            return token_categories[current_token_code];
        }
        --source_text_cursor;
        EMIT_LEXICAL_TOKEN(TOK_MEMBER);
    case CC_DIGIT:
        if (!(scanner_flags & 0x8000) && (options.option_E || (scanner_flags & 0x1000000))) {
            token_text[0] = source_text_cursor[-1];
            bytes = read_number_text(token_text + 1);
            if (options.option_E) fwrite(token_text, bytes, 1, preprocessor_stream);
            current_token_code = TOK_PP_OUTPUT;
            return token_categories[TOK_PP_OUTPUT];
        }
        if ((scanner_flags & 0x400000) && !(scanner_flags & 0x10000))
            current_token_code = scan_assembler_number((unsigned char)source_text_cursor[-1]);
        else
            current_token_code = scan_number((unsigned char)source_text_cursor[-1]);
        return token_categories[current_token_code];
    default:
        if (!options.option_E)
            report_syntax_error(18, (unsigned char)source_text_cursor[-1]);
        else
            fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
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
        if (wide) fputc('L', preprocessor_stream);
        copy_quoted_literal('\'');
        goto next_character;
    }
    current_token_code = scan_character_literal(wide);
    return token_categories[current_token_code];
string_literal:
    if (options.option_E) {
        if (wide) fputc('L', preprocessor_stream);
        copy_quoted_literal('"');
        goto next_character;
    }
    scan_string_literal(wide, '"');
    current_token_code = TOK_STRING;
    return TOK_STRING;
}

/* Original 0x21bb0..0x21d18; class-dispatch table 0x21d18..0x21d48. */
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
            /* Blank lines are copied by /E along with other leading space. */
        case CC_SPACE:
            if (options.option_E)
                fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
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
    if (options.option_ide) report_ide_line();
    browser_file_cookie = 1;
    --source_text_cursor;
    if ((scanner_flags & 0x80000) && !(scanner_flags & 0x40))
        record_line_information(1);
    if (pch_mode && !include_depth) finish_pch_capture();
}

/* Original 0x18bd0..0x18c36. The caller must provide stack capacity. */
void __stdcall unget_token(unsigned char category)
{
    ++token_stack_count;
    token_stack_top->category = category;
    token_stack_top->code = current_token_code;
    token_stack_top->value = current_token_value;
    token_stack_top->lookup_symbol = last_lookup_symbol;
    ++token_stack_top;
    if (token_stack_top >= initial_token_stack + 3)
        scanner_flags |= SCANNER_LOOKAHEAD_FULL;
}

/* Original 0x19600..0x19992. Interpret lexical categories in the current
   declaration context before handing them to the LR grammar. */
unsigned char read_parser_token(void)
{
    unsigned char category;
    for (;;) {
        if ((scanner_flags & (0x400000 | 0x10000)) || options.option_E)
            return scan_token();
        category = scan_token();
        switch (category) {
        case TOK_RIGHT_PAREN:
            scanner_flags &= ~1;
            if (!declaration_depth && !(scanner_flags & (0x20000000 | 0x200))) {
                report_syntax_error(59, ")");
                continue;
            }
            return category;
        case TOK_IDENTIFIER:
            category = lookup_identifier_token(0);
            if (category == TOK_TYPEDEF_NAME) return resolve_identifier_token(category);
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 2;
            return category;
        case TOK_VOLATILE:
        case TOK_ENUM:
        case TOK_STRUCT:
        case TOK_UNION:
        case TOK_SEGMENT:
        case TOK_STORAGE_CLASS:
        case TOK_TYPE:
        case TOK_STORAGE_MODIFIER:
        case TOK_DECLSPEC:
        case TOK_CONST:
            return resolve_identifier_token(category);
        case TOK_BASED:
        case TOK_SEGNAME:
        case TOK_BIT_AND:
        case TOK_LOGICAL_NOT:
        case TOK_MINUS:
        case TOK_MULTIPLY:
        case TOK_PLUS:
        case TOK_CONSTANT:
        case TOK_INCREMENT_OPERATOR:
        case TOK_MODIFIER:
        case TOK_BIT_NOT:
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 2;
            return category;
        case TOK_LEFT_PAREN:
            if (scanner_flags & 0x200) return category;
            if (!(scanner_flags & 0x20000000)) {
                if (++declaration_depth >= 60) fatal_error(13);
                declaration_spec_flags[declaration_depth] &= ~0x5f;
                declaration_shapes[declaration_depth] = 0;
            }
            scanner_flags &= ~1;
            return category;
        case TOK_RETURN:
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 3;
            return category;
        case TOK_SIZEOF:
            ++unevaluated_expression_depth;
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 2;
            return category;
        case TOK_TYPE_NAME:
            if ((declaration_spec_flags[declaration_depth] & 2) &&
                (scanner_flags & 0x10000000) && last_lookup_symbol) {
                current_token_value.identifier = last_lookup_symbol->name;
                current_token_code = TOK_IDENTIFIER;
                return TOK_IDENTIFIER;
            }
            scanner_flags |= 1;
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 2;
            return category;
        default:
            return category;
        }
    }
}

/* Original 0x18f90..0x19530. Gather declaration specifiers, fold a named
   type into its qualifiers, and leave the first declarator token pushed back. */
unsigned char __stdcall resolve_identifier_token(unsigned char category)
{
    unsigned short flags = 0, attributes = 0, inherited_attributes;
    unsigned char tag_category;
    unsigned char storage[3];
    unsigned consumed = 0, i;
    unsigned code;
    CType *type = 0;
    TypeShape *shape;
    KeywordSpec *keyword;
    char *spelling;
    storage[0] = current_storage[0] & ~7;
    storage[1] = current_storage[1];
    storage[2] = current_storage[2];
    last_lookup_symbol = 0;
    if (category != TOK_CLASS_HEAD) parser_flags &= ~0x4000;
    for (;;) {
        switch (category) {
        case TOK_IDENTIFIER:
            if (flags) {
                scanner_flags |= 1;
                goto finish_specifiers;
            }
            category = lookup_identifier_token(0);
            if (category == TOK_TYPEDEF_NAME) goto named_type;
            if (!consumed) return category;
            goto finish_specifiers;
        case TOK_CONST:
        case TOK_VOLATILE:
            if (!(parser_flags & 0x20) &&
                ((declaration_spec_flags[declaration_depth] & 2) || (scanner_flags & 4))) {
                current_token_code = category;
                current_token_value.bytes[0] = category;
                return category;
            }
            for (i = 0; declaration_codes[i].token != current_token_code; ++i)
                ;
            attributes = merge_type_attributes(attributes, declaration_codes[i].value);
            break;
        case TOK_SEGMENT:
            type = merge_type_specifier(type, &flags, 0x802);
            break;
        case TOK_ENUM:
        case TOK_STRUCT:
        case TOK_UNION:
            if (flags) unget_token(category);
            tag_category = category;
            parser_flags |= 0x10000;
            for (i = 0; declaration_codes[i].token != category; ++i)
                ;
            category = read_tag_specifier(declaration_codes[i].value, &attributes);
            parser_flags &= ~0x10000;
            if (category == TOK_NEWLINE) {
                category = scan_token();
                return recover_parser_token(category, TOK_SEMICOLON, 1);
            }
            if (flags) {
                keyword = keyword_specs;
                while (keyword->name && keyword->token != tag_category) ++keyword;
                report_syntax_error(236, keyword->name, current_token_value.symbol->name);
                flags = 0;
            }
            type = merge_named_type(current_token_value.symbol, &flags, &attributes);
            if (category == TOK_CLASS_HEAD) goto fold_specifiers;
            if (category == TOK_ENUM_TAG) {
                scanner_flags |= 1;
                goto fold_specifiers;
            }
            break;
        case TOK_STORAGE_CLASS:
        case TOK_STORAGE_MODIFIER:
            if (parser_flags & 0x20) {
                if (current_token_code == TOK_SYSAPI) spelling = "__sysapi";
                else {
                    keyword = keyword_specs;
                    while (keyword->name && keyword->token != current_token_code) ++keyword;
                    spelling = keyword->name;
                }
                report_syntax_error(721, spelling);
                storage[0] |= 8;
            } else {
                for (i = 0; declaration_codes[i].token != current_token_code; ++i)
                    ;
                code = declaration_codes[i].value;
                if (category == TOK_STORAGE_CLASS) merge_storage_class(storage, code, 1, 0);
                else merge_storage_class(storage, 0, code, 0);
            }
            break;
        case TOK_TYPE:
            for (i = 0; declaration_codes[i].token != current_token_code; ++i)
                ;
            type = merge_type_specifier(type, &flags, declaration_codes[i].value);
            break;
        case TOK_CLASS_HEAD:
            type = (CType *)current_token_value.words.high;
            last_lookup_symbol = (FrontendSymbol *)type->field08;
            flags = type->flags;
            attributes = type->attributes;
            storage[0] = current_token_value.bytes[0];
            storage[1] = current_token_value.bytes[1];
            storage[2] = current_token_value.bytes[2];
            break;
        case TOK_DECLSPEC:
            parse_declspec(storage);
            break;
        case TOK_TYPEDEF_NAME:
named_type:
            type = merge_named_type(current_token_value.symbol, &flags, &attributes);
            break;
        default:
finish_specifiers:
            unget_token(category);
fold_specifiers:
            if (!flags) {
                storage[0] |= 0x10;
                flags = 0x64;
            }
            shape = 0;
            if (type) {
                shape = type->related;
                if (shape && !(shape->flags & 7)) shape = shape->related;
                inherited_attributes = shape ? shape->attributes : type->attributes;
                if (!((attributes ^ inherited_attributes) & 0x3000) || !(attributes & 0x3000))
                    goto specifiers_ready;
                type = copy_mutable_type(type);
                if (shape) {
                    if (attributes & inherited_attributes & 0x3000) {
                        if (options.z_a) compiler_warning(1, 114);
                        else report_syntax_error(289);
                    }
                    type = replace_outer_type_shape(type, shape->flags,
                        (unsigned short)(shape->attributes | (attributes & 0x3000)), &shape->field08);
                    attributes &= ~0x3000;
                }
            } else {
                type = &type_description_stack[++type_description_depth];
                type->flags = 0;
                type->attributes = 0;
                type->related = 0;
                type->field08 = 0;
                type->field0c = 0;
                type->flags = flags | 0x1000;
            }
            type->attributes |= attributes;
            type = finalize_type_description(type);
specifiers_ready:
            if (!(storage[0] & 7)) storage[0] |= 0x20;
            current_token_value.words.high = (unsigned)type;
            current_token_value.bytes[0] = storage[0];
            current_token_value.bytes[1] = storage[1];
            current_token_value.bytes[2] = storage[2];
            if (category == TOK_CLASS_HEAD || category == TOK_ENUM_TAG) {
                if (category == TOK_ENUM_TAG) scanner_flags |= 1;
                if (storage[1] & 4) report_syntax_error(391, last_lookup_symbol->name);
                return category;
            }
            if (attributes & 0x3000) current_token_value.bytes[0] &= ~0x10;
            current_token_code = TOK_TYPE;
            if (!(scanner_flags & 0x20000000)) declaration_spec_flags[declaration_depth] |= 2;
            return TOK_TYPE_NAME;
        }
        ++consumed;
        category = scan_token();
    }
}


/* Original 0x19530..0x195f2. A declaration may force an identifier to stay
   a name; otherwise typedef symbols acquire their grammar category. */
unsigned char __stdcall lookup_identifier_token(FrontendSymbol *explicit_symbol)
{
    FrontendSymbol *symbol;
    if (!(parser_flags & 1)) {
        if (scanner_flags & 1) {
            scanner_flags &= ~1;
            return TOK_IDENTIFIER;
        }
        if ((declaration_spec_flags[declaration_depth] & 2) || (scanner_flags & 0x80))
            return TOK_IDENTIFIER;
        symbol = find_identifier_symbol(current_token_value.identifier);
    } else symbol = explicit_symbol;
    if (!symbol || !(symbol->flags & 4)) return TOK_IDENTIFIER;
    if ((symbol->kind & 0x1f) == 4 && (declaration_spec_flags[declaration_depth] & 1) &&
        !(parser_flags & 0x20)) return TOK_TYPEDEF_NAME;
    if (browser_filename && (!parameter_nesting || options.option_FR))
        browser_symbol_reference(symbol);
    current_token_value.symbol = symbol;
    return TOK_TYPEDEF_NAME;
}

/* Original 0x21db0..0x21f95. Scan radix and U/L suffixes before selecting
   the original integer token kind; decimal fractions use a separate entry. */
unsigned char __stdcall scan_number(int first_character)
{
    unsigned char character = (unsigned char)first_character, token = TOK_INT_CONSTANT;
    unsigned char following;
    unsigned value;
    int radix;
    char *destination = token_text;
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            radix = 16;
            character = read_numeric_character();
            while (character_flags[character] & CF_HEX) {
                *destination++ = character;
                character = read_numeric_character();
            }
            if (destination == token_text) report_syntax_error(153);
            goto integer_suffix;
        }
        radix = 8;
        *destination++ = '0';
    } else radix = 10;
    while (character_flags[character] & CF_DECIMAL) {
        *destination++ = character;
        character = read_numeric_character();
        if (destination > token_text + 128) fatal_error(64);
    }
    if (character == '.' || (character & 0xdf) == 'E') {
        --source_text_cursor;
        return scan_float_tail(destination);
    }
integer_suffix:
    character &= 0xdf;
    if (character == 'L') {
        following = read_numeric_character() & 0xdf;
        if (following == 'U') token = TOK_ULONG_CONSTANT;
        else { token = TOK_LONG_CONSTANT; --source_text_cursor; }
    } else if (character == 'U') {
        following = read_numeric_character() & 0xdf;
        if (following == 'L') token = TOK_ULONG_CONSTANT;
        else { token = TOK_UINT_CONSTANT; --source_text_cursor; }
    } else --source_text_cursor;
    following = character_class[(unsigned char)*source_text_cursor];
    if (following == CC_IDENTIFIER || following == CC_WIDE_PREFIX)
        report_syntax_error(59, "bad suffix on number");
    *destination = 0;
    value = convert_integer_digits(token_text, destination - token_text, radix);
    switch (token) {
    case TOK_INT_CONSTANT:
        token = radix == 10 ? classify_decimal_integer(value) : classify_radix_integer(value);
        break;
    case TOK_LONG_CONSTANT: token = classify_long_integer(value); break;
    case TOK_UINT_CONSTANT: token = classify_unsigned_integer(value); break;
    }
    current_token_value.literal = make_constant_token(token, &value);
    return token;
}

/* Original 0x22ac0..0x22afc. Numeric scanning uses the full character
   reader except at an input-window sentinel. A recovered space ends it. */
unsigned char read_numeric_character(void)
{
    char *cursor = source_text_cursor++;
    if (!*cursor) {
        if (recover_special_character() == ' ') {
            ++source_text_cursor;
            return 0;
        }
        return (unsigned char)*source_text_cursor++;
    }
    --source_text_cursor;
    return next_source_character();
}


/* Original 0x22b00..0x22b69. Read through source-window boundaries,
   trigraph rewriting, and escaped newlines unless raw scanning is active. */
unsigned char next_source_character(void)
{
    unsigned char character;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        while (!character && macro_rescan_depth <= 0) {
            recover_special_character();
            character = (unsigned char)*source_text_cursor++;
        }
        if (parser_flags & 0x02000000) return character;
        if (character == '?' && (recover_trigraph() & ~0x40U)) continue;
        if (character == '\\' && consume_line_escape()) continue;
        return character;
    }
}

/* Original 0x22160..0x22266. Validate the 32-bit radix limit before
   accumulating digits with unsigned wrap, reporting invalid radix digits. */
unsigned __stdcall convert_integer_digits(char *digits, int length, int radix)
{
    unsigned value = 0;
    int digit, comparison, i;
    char *limit = "4294967295";
    if (length > 8) {
        while (*digits == '0') { ++digits; --length; }
        if (!length) return 0;
        if (length > 8) {
            switch (radix) {
            case 8:
                if (length > 10 && !(length == 11 && *digits <= '3')) goto overflow;
                break;
            case 10:
                if (length == 9) break;
                if (length != 10) goto overflow;
                comparison = 0;
                for (i = 0; i < 11; ++i)
                    if ((unsigned char)digits[i] != (unsigned char)limit[i]) {
                        comparison = (unsigned char)digits[i] - (unsigned char)limit[i];
                        break;
                    }
                if (comparison > 0) goto overflow;
                break;
            case 16: goto overflow;
            }
        }
    }
    do {
        digit = integer_digit_value((unsigned char)*digits);
        if (digit >= radix) report_syntax_error(41, (unsigned char)*digits, radix);
        ++digits;
        value += digit;
        if (!*digits) return value;
        value *= radix;
    } while (1);
overflow:
    report_syntax_error(177);
    return 0;
}

/* Original 0x222e0..0x22302. Decimal digits use their ordinary value;
   hexadecimal letters are folded without a locale-dependent CRT call. */
int __stdcall integer_digit_value(int character)
{
    if (character_flags[(unsigned char)character] & CF_DECIMAL) return character - '0';
    return (character & 0xdf) - ('A' - 10);
}

/* Original 0x22270..0x22285. Octal and hexadecimal literals select unsigned
   int when their high bit is set. Int and long are both 32 bits here. */
unsigned char __stdcall classify_radix_integer(unsigned value)
{
    return (value & 0x80000000U) ? TOK_UINT_CONSTANT : TOK_INT_CONSTANT;
}

/* Original 0x22920..0x229a3. A slash has already been consumed; report
   whether it starts a comment and preserve /C output outside directives. */
int skip_source_comment(void)
{
    if (match_source_character('*')) {
        scanner_flags |= 0x1000;
        if (options.option_C && !(scanner_flags & 0x10000)) copy_block_comment();
        else skip_block_comment();
        scanner_flags &= ~0x1000UL;
        return 1;
    }
    if ((options.z_a || options.z_E) && match_source_character('/')) {
        scanner_flags |= 0x1000;
        skip_line_comment();
        scanner_flags &= ~0x1000UL;
        return 1;
    }
    return 0;
}

/* Original 0x22640..0x22663. Only a matching character stays consumed. */
int __stdcall match_source_character(int character)
{
    if (next_source_character() == (unsigned char)character) return 1;
    --source_text_cursor;
    return 0;
}

/* Original 0x22870..0x22915. Track physical newlines while discarding
   a block comment, including buffer boundaries and escaped star tails. */
void skip_block_comment(void)
{
    unsigned char character;
next_character:
    character = (unsigned char)*source_text_cursor++;
    if (character == '*') {
following_star:
        character = (unsigned char)*source_text_cursor++;
        if (character == '/') return;
        if (character == '*') goto following_star;
        if (!character) { recover_special_character(); goto following_star; }
        if (character == '\\') {
            if (recover_special_character() & 0x10) goto next_character;
            goto following_star;
        }
        if (character_class[character] == CC_LEAD_BYTE) ++source_text_cursor;
    }
    if (character == '\n') {
        ++source_line_number;
        if (options.option_E) fputc('\n', preprocessor_stream);
    } else if (!character) recover_special_character();
    goto next_character;
}

/* Original 0x21290..0x212e7. Leave the terminating newline for the
   caller while handling comments, buffer boundaries, and lead-byte pairs. */
void skip_source_line(void)
{
    unsigned char character, class;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        switch (class) {
        case CC_SLASH: skip_source_comment(); break;
        case CC_ESCAPE_OR_END: recover_special_character(); break;
        case CC_NEWLINE: --source_text_cursor; return;
        case CC_LEAD_BYTE: ++source_text_cursor; break;
        }
    }
}

/* Original 0x1e310..0x1e448. Zero bytes introduce macro control markers
   inside replacements and physical buffer refills outside an expansion. */
unsigned recover_special_character(void)
{
    unsigned char character = (unsigned char)source_text_cursor[-1];
    MacroFrame *frame;
    if (character == '?') return recover_trigraph();
    if (character == '\\') {
        if (!macro_expansion_active && consume_line_escape()) return 1;
        return 0x10;
    }
    if (!macro_expansion_active) { refill_source_boundary(); return 1; }
    character = (unsigned char)*source_text_cursor++;
    switch (character) {
    case 'A':
        save_macro_argument(macro_frames[macro_expansion_active].output_start, macro_expansion_next);
        macro_expansion_next = macro_frames[macro_expansion_active].output_start;
        advance_macro_argument();
        return 2;
    case 'D':
        return pop_macro_frame() ? 8 : 4;
    case 'R':
        frame = &macro_frames[macro_expansion_active];
        source_text_cursor = frame->source_cursor;
        --frame->definition->disabled;
        if (macro_expansion_active == 1) {
            macro_expansion_start = macro_work_buffer;
            macro_expansion_next = macro_work_buffer + (macro_work_bytes >> 1);
        }
        if (!browser_macro_context && browser_filename) browser_macro_end();
        browser_macro_context = 0;
        --macro_expansion_active;
        return 0x20;
    }
    return 0;
}

/* Original 0x22290..0x222a5. Decimal constants that exceed signed 32-bit
   range take unsigned-long type; the remaining values retain int type. */
unsigned char __stdcall classify_decimal_integer(unsigned value)
{
    return value & 0x80000000UL ? TOK_ULONG_CONSTANT : TOK_INT_CONSTANT;
}

/* Original 0x1e170..0x1e307. Rewrite a recognized trigraph's final byte
   for rescanning, preserving question marks across physical buffer boundaries. */
unsigned recover_trigraph(void)
{
    unsigned char character, replacement;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        if (character == '?') break;
        if (character || macro_expansion_active) {
            --source_text_cursor;
            return 0x40;
        }
        refill_source_boundary();
        source_text_cursor[-1] = '?';
    }
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        switch (character) {
        case '!': replacement = '|'; break;
        case '\'': replacement = '^'; break;
        case '(': replacement = '['; break;
        case ')': replacement = ']'; break;
        case '-': replacement = '~'; break;
        case '/': replacement = '\\'; break;
        case '<': replacement = '{'; break;
        case '=': replacement = '#'; break;
        case '>': replacement = '}'; break;
        case 0:
            if (!macro_expansion_active) {
                refill_source_boundary();
                continue;
            }
            /* An expansion boundary is an unrecognized third character. */
        default:
            --source_text_cursor;
            source_text_cursor[-1] = '?';
            --source_text_cursor;
            source_text_cursor[-1] = '?';
            return 0x40;
        }
        source_text_cursor[-1] = replacement;
        --source_text_cursor;
        return 1;
    }
}

/* Original 0x22670..0x22698, tail dispatching to copy or discard text. */
void skip_line_comment(void)
{
    compiler_warning(4, 1);
    if (options.option_C && !(scanner_flags & 0x10000)) copy_line_comment();
    else discard_line_comment();
}

/* Original 0x212f0..0x21358. Leave the terminating newline unread;
   diagnose a backslash continuation and skip multibyte trail characters. */
void discard_line_comment(void)
{
    unsigned char character, class;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        switch (class) {
        case CC_ESCAPE_OR_END:
            if (recover_special_character() == 1 && character == '\\')
                compiler_warning(3, 10);
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        case CC_LEAD_BYTE:
            ++source_text_cursor;
            break;
        }
    }
}

/* Original 0x22a30..0x22abf. Join a physical line following a backslash,
   carrying it across input boundaries and preserving preprocess-only output. */
int consume_line_escape(void)
{
    unsigned char character;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        if (character == '\r') continue;
        if (!character) {
            recover_special_character();
            source_text_cursor[-1] = '\\';
            continue;
        }
        if (character != '\n') {
            --source_text_cursor;
            return 0;
        }
        ++source_line_number;
        if (options.option_E) {
            if (parser_flags & 0x08000000UL) fputc('\\', preprocessor_stream);
            fputc('\n', preprocessor_stream);
        }
        return 1;
    }
}

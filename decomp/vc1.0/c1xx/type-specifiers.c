/* C++ declaration type and tag interpretation. */
#include "lexer.h"

#define LEXICAL_DECLARATION_CODE(destination_, token_) do { \
    for (code_index = 0; code_index < declaration_code_count; ++code_index) \
        if (declaration_codes[code_index].token == (token_)) break; \
    (destination_) = code_index < declaration_code_count ? \
        declaration_codes[code_index].value : 0x9e; \
} while (0)

/* Original 0x1a2d0..0x1aadc. Gather declaration storage, qualifiers and base
   type, then distinguish a declaration from an expression/type-name prefix. */
unsigned char __stdcall read_type_specifier(unsigned char token)
{
    unsigned short flags = 0, attributes = 0, prior_attributes;
    CType *type = 0;
    TypeDescription *description;
    TypeShape *shape;
    StorageSpec storage;
    unsigned code_index, code;
    int consumed = 0;
    char *name, *constructor_name;
    QualifiedDeclarator *qualified;
    storage.flags = (declaration_storage[0] | (declaration_storage[1] << 8)) & ~7;
    storage.extended = declaration_storage[2];
    type_specifier_symbol = 0;
    for (;;) {
        switch (token) {
        case 36: /* storage class */
            if (cpp_parser_flags & 0x20) {
                report_syntax_error(721, token_spellings[current_token_code]);
                storage.flags |= 8;
            } else {
                LEXICAL_DECLARATION_CODE(code, current_token_code);
                add_storage_specifier(&storage, (unsigned char)code, 1, 0);
            }
            break;
        case TOK_VIRTUAL:
        case 37: /* storage modifier */
            if (cpp_parser_flags & 0x20) {
                report_syntax_error(721, token_spellings[current_token_code]);
                storage.flags |= 8;
            } else {
                LEXICAL_DECLARATION_CODE(code, current_token_code);
                add_storage_specifier(&storage, 0, (unsigned char)code, 0);
            }
            break;
        case TOK_SEGMENT:
            type = merge_type_specifier(type, &flags, 0x802);
            break;
        case TOK_CLASS:
        case TOK_ENUM:
        case TOK_STRUCT:
        case TOK_UNION:
            if (flags) unget_token(token);
            cpp_parser_flags |= 0x10000;
            LEXICAL_DECLARATION_CODE(code, token);
            token = read_tag_specifier((unsigned short)code, &attributes);
            cpp_parser_flags &= ~0x10000UL;
            if (token == 1) return synchronize_tokens((unsigned)scan_token(), 138, 1);
            if (flags) {
                report_syntax_error(236, token_spellings[current_token_code]);
                flags = 0;
            }
            type = apply_type_symbol(current_token_value.symbol, &flags, &attributes);
            if (token == 38) goto finish_specifiers;
            if (token == 39) {
                token_allocation_flags |= 1;
                goto finish_specifiers;
            }
            break;
        case 34: /* type name */
apply_type_name:
            type = apply_type_symbol(current_token_value.symbol, &flags, &attributes);
            storage.flags |= 0x80;
            break;
        case 35: /* fundamental type */
            LEXICAL_DECLARATION_CODE(code, current_token_code);
            type = merge_type_specifier(type, &flags, (unsigned short)code);
            break;
        case 38: /* already assembled class-head declaration */
            type = current_token_value.declaration.type;
            type_specifier_symbol = cpp_context_stack->items[type->field08];
            flags = type->flags;
            attributes = type->attributes;
            storage.flags = current_token_value.declaration.storage[0] |
                            (current_token_value.declaration.storage[1] << 8);
            storage.extended = current_token_value.declaration.storage[2];
            break;
        case TOK_CONST:
        case TOK_VOLATILE:
            if (!(cpp_parser_flags & 0x20) &&
                ((declaration_spec_flags[declaration_depth] & 2) || (token_allocation_flags & 4))) {
                current_token_code = token;
                current_token_value.bytes[0] = token;
                return token;
            }
            LEXICAL_DECLARATION_CODE(code, current_token_code);
            attributes = merge_type_attributes(attributes, (unsigned short)code);
            break;
        case 160: /* :: may start a qualified type name */
            unget_token(160);
            token = resolve_identifier_token(0);
            if (token != 161) goto unread_token;
            /* The qualified and unqualified forms share the following test. */
        case 161:
        case 2:
            name = current_token_value.name;
            if (!flags) {
                if (token == 2)
                    token = resolve_typename_token(0);
                else {
                    token = resolve_typename_token(current_token_value.symbol);
                    if (token == 34) unwind_qualified_scopes(1);
                }
                if (token == 34) goto apply_type_name;
                if (!consumed) return token;
                goto unread_token;
            }
            if (!declaration_depth && !declaration_mode && current_class_context >= 0 &&
                !(token_allocation_flags & 0x20000000UL)) {
                if (token == 2) {
                    if (current_class_name() != name) goto mark_declaration;
                    token = resolve_identifier_token(name);
                    if (token == 161) goto qualified_constructor;
                    if (token == 2 && current_class_name() == current_token_value.name) {
                        constructor_name = name;
                        goto constructor_type_error;
                    }
                    token_allocation_flags |= 1;
                    goto unread_token;
                }
                if (token == 161) {
qualified_constructor:
                    qualified = current_token_value.context;
                    if ((qualified->declaration_flags & 0x1000) &&
                        qualified->resolution->binding->target->name == current_class_name()) {
                        constructor_name = qualified->resolution->binding->target->name;
constructor_type_error:
                        report_syntax_error(380, constructor_name);
                        flags = 0;
                        type = apply_type_symbol(get_current_class_symbol(), &flags, &attributes);
                        break;
                    }
                }
            }
mark_declaration:
            token_allocation_flags |= 1;
            goto unread_token;
        case TOK_DECLSPEC:
            read_declspec_attributes(&storage);
            break;
        default:
            goto unread_token;
        }
        ++consumed;
        token = scan_token();
    }
unread_token:
    unget_token(token);
finish_specifiers:
    if (!flags) {
        storage.flags |= 0x10;
        qualified = current_token_value.context;
        flags = token == 161 && (qualified->declaration_flags & 0x3000) ? 0x80 : 100;
    }
    if (!type) {
        description = &type_description_stack[++type_description_depth];
        description->flags = 0;
        description->attributes = 0;
        description->related = 0;
        description->field08 = description->field0a = 0;
        description->flags = flags | 0x1000;
        type = (CType *)description;
        goto finish_qualified_type;
    }
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    prior_attributes = shape ? shape->attributes : type->attributes;
    if (((attributes ^ prior_attributes) & 0x3000) && (attributes & 0x3000)) {
        type = copy_unfinished_type(type);
        if (shape) {
            if (attributes & prior_attributes & 0x3000) {
                if (!options.configuration.z_a) report_syntax_error(289);
                else compiler_warning(1, 114);
            }
            type = replace_first_type_shape(type, shape->flags,
                (unsigned short)(shape->attributes | (attributes & 0x3000)), &shape->data);
            attributes &= ~0x3000;
        }
finish_qualified_type:
        type->attributes |= attributes;
        type = finish_type(type);
    }
    if (!(storage.flags & 7)) storage.flags |= 0x20;
    current_token_value.declaration.type = type;
    current_token_value.declaration.storage[0] = (unsigned char)storage.flags;
    current_token_value.declaration.storage[1] = (unsigned char)(storage.flags >> 8);
    current_token_value.declaration.storage[2] = storage.extended;
    if (token == 38 || token == 39) {
        if (token == 39) token_allocation_flags |= 1;
        if (storage.flags & 0x400) report_syntax_error(391, format_symbol_name(type_specifier_symbol));
        return token;
    }
    if ((attributes & 0x3000) && (storage.flags & 0x10)) {
        if (token_allocation_flags & 0x20000000UL) {
            current_token_code = 182;
            current_token_value.modifier = attributes;
            return 182;
        }
        current_token_value.declaration.storage[0] &= ~0x10;
    }
    if (!(storage.flags & 0x10)) {
        if (cpp_parser_flags & 1)
            cpp_parser_flags &= ~1;
        else if ((storage.flags & 7) != 7) {
            if (!(declaration_spec_flags[declaration_depth] & 8)) {
                if ((!(flags & 0x9c0) && !(flags & 0x200)) ||
                    (!(storage.flags & 7) && !(storage.flags & 0x700)))
                    return resolve_type_parenthesis();
            } else if (token_allocation_flags & 0x20)
                return resolve_type_parenthesis();
        }
    }
    current_token_code = token_categories[40];
    if (!(token_allocation_flags & 0x20000000UL))
        declaration_spec_flags[declaration_depth] |= 2;
    return 40;
}

int suppress_vtordisp; /* original 0x8c1f8 */

/* Original 0x19d70..0x1a1da. Read a class/struct/union/enum tag, including
   inheritance and Microsoft class-pointer/storage modifiers. */
unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes)
{
    unsigned char token, lookahead;
    unsigned short pointer_attributes = 0;
    unsigned code, code_index;
    StorageSpec storage;
    CppSymbol *symbol;
    QualifiedDeclarator *qualified;
    char *name;
    storage.flags = (declaration_storage[0] | (declaration_storage[1] << 8)) & ~7;
    storage.extended = declaration_storage[2];
    suppress_vtordisp = 0;
    cpp_parser_flags &= ~0x00e00000UL;
    for (;;) {
        token = scan_token();
        switch (token) {
        case 0:
            fatal_error(4);
            goto define_tag;
        case 2:
            name = current_token_value.name;
            lookahead = scan_token();
            unget_token(lookahead);
            if (lookahead == 63 || lookahead == 127) goto define_tag;
            if (lookahead != 160) {
                current_token_value.symbol = reference_tag_symbol(name, kind, insert_symbol);
                return 35;
            }
            token_allocation_flags |= 0x10;
            token = resolve_identifier_token(name);
            token_allocation_flags &= ~0x10;
            if (token == 161) goto qualified_tag;
            if (token == 2) {
                current_token_value.symbol = reference_tag_symbol(current_token_value.name,
                    kind, finalize_parameter_declaration);
                return 35;
            }
            unget_token(token);
            break;
        case 63: /* anonymous tag followed by base list */
        case 127: /* anonymous tag body */
            unget_token(token);
            name = 0;
            goto define_tag;
        case TOK_MODIFIER:
            if (pointer_attributes) compiler_warning(1, 37);
            LEXICAL_DECLARATION_CODE(code, current_token_code);
            pointer_attributes = (unsigned short)code;
            if ((pointer_attributes & 7) != 1 && (pointer_attributes & 7) != 2 &&
                !(pointer_attributes & 0x400)) {
                compiler_warning(1, 38, token_spellings[current_token_code]);
                pointer_attributes = 0;
            }
            break;
        case 160:
            unget_token(160);
            token_allocation_flags |= 0x10;
            token = resolve_identifier_token(0);
            token_allocation_flags &= ~0x10;
            if (token == 161) goto qualified_tag;
            unget_token(token);
            break;
        case 161:
qualified_tag:
            unwind_qualified_scopes(1);
            qualified = current_token_value.context;
            current_token_value.symbol = (CppSymbol *)qualified->resolution->binding->target;
            symbol = match_tag_symbol(current_token_value.symbol, kind);
            if (!symbol)
                report_syntax_error(649, current_token_value.symbol->name, type_kind_name(kind));
            current_token_value.symbol = symbol;
            return 35;
        case TOK_NOVTORDISP:
            suppress_vtordisp = 1;
            break;
        case 196: /* inheritance model */
            switch (current_token_code) {
            case TOK_SINGLE_INHERITANCE:
                cpp_parser_flags = (cpp_parser_flags & ~0x00c00000UL) | 0x00200000UL;
                continue;
            case TOK_MULTIPLE_INHERITANCE:
                cpp_parser_flags = (cpp_parser_flags & ~0x00a00000UL) | 0x00400000UL;
                continue;
            case TOK_VIRTUAL_INHERITANCE:
                cpp_parser_flags = (cpp_parser_flags & ~0x00800000UL) | 0x00600000UL;
                continue;
            }
            /* Other lexical forms retain the original automatic name value;
               the three model keywords take the cases above. */
            goto define_tag;
        case TOK_DECLSPEC:
            read_declspec_attributes(&storage);
            break;
        default:
            report_syntax_error(237, type_kind_name(kind), token_spellings[token]);
            unget_token(token);
            current_token_value.symbol = 0;
            return 1;
        }
    }
define_tag:
    symbol = define_tag_symbol(name, kind, *attributes);
    current_token_value.symbol = symbol;
    if (!symbol || (symbol->declaration_flags & 2)) return 1;
    if (kind == 0x200) {
        token_allocation_flags |= 1;
        return 39;
    }
    symbol->detail.class_info->pointer_attributes = pointer_attributes;
    if (storage.extended & 4) {
        symbol->detail.class_info->flags2 |= 4;
        symbol->detail.class_info->flags2 =
            (symbol->detail.class_info->flags2 & ~2) | ((storage.extended >> 2) & 2);
    } else if ((storage.flags & 0x700) || (storage.extended & 3)) {
        begin_type_diagnostic();
        compiler_warning(1, 38, type_diagnostic_text(format_storage_spec(0, storage)));
        end_type_diagnostic();
    }
    return 38;
}


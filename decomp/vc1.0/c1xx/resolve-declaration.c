/* Declaration resolution and insertion into C++ symbol scopes. */
#include "declarations.h"
#include "global-ir.h"
#include "cpp-context.h"
#include "ir.h"
#include "operators.h"
#include "preprocessor.h"

DeclarationType conversion_declaration; /* original 0x91394..0x9139c */
unsigned short current_declaration_scope; /* original 0x9fc3c */

#define STORAGE_KIND (storage.flags & 7)
#define CLASS_SYMBOL(type_) ((CppSymbol *)cpp_context_stack->items[(type_)->field08])

/* Original 0x51fc0..0x52694; dispatch data 0x52694..0x526a0. */
CppSymbol *__stdcall resolve_declaration(DeclarationType *context, CppSymbol *declaration)
{
    CType *base;
    StorageSpec storage, conversion;
    QualifiedDeclarator *qualifier, *qualified;
    CppSymbol *symbol, *target, *alias, *class_symbol;
    TypeShape *shape;
    unsigned kind, old_tag, new_tag;
    unsigned short context_flags, classification;
    short text;
    int existing_scope = 0;
    char *storage_text;

    if (!declaration || !context->type)
        return 0;
    base = context->type;
    context->storage[0] &= 0x7f;
    storage.flags = context->storage[0] | (context->storage[1] << 8);
    storage.extended = context->storage[2];
    if (STORAGE_KIND == 1)
        storage.flags |= 0x1000;
    qualifier = (QualifiedDeclarator *)declaration->next;
    if (qualifier && is_static_qualified_symbol((CppSymbol *)(*qualifier->resolution->binding)->target) &&
        (!(storage.flags & 0x20) || (storage.flags & 0x700) || (storage.extended & 7))) {
        if (options.configuration.z_a && STORAGE_KIND == 1)
            compiler_warning(1, 630, format_symbol_name(declaration), token_spellings[13]);
        else {
            begin_type_diagnostic();
            text = format_storage_spec(0, storage);
            storage_text = type_diagnostic_text(text);
            report_syntax_error(720, format_symbol_name(declaration), storage_text);
            end_type_diagnostic();
        }
        storage.flags = (declaration_storage[0] | (declaration_storage[1] << 8)) & ~7;
        storage.flags |= 8;
        storage.extended = declaration_storage[2];
        declaration->declaration_flags |= 2;
    }
    if (declaration->declaration_flags & 0x4000) {
        shape = declaration->type->related;
        if (!shape || (shape->flags & 0x77) != 4 ||
            (shape->related && (shape->related->flags & 0x77))) {
            report_syntax_error(586);
            declaration->cpp.parts.token = 1;
            declaration->declaration_flags &= ~0x4000;
        }
        if (!(token_allocation_flags & 0x2000000) && !(context->storage[0] & 0x10))
            report_syntax_error(549);
        conversion.flags = conversion_declaration.storage[0] |
                           (conversion_declaration.storage[1] << 8);
        conversion.extended = conversion_declaration.storage[2];
        if (storage.flags & 0x20) {
            storage.flags = (storage.flags & ~7) | (conversion.flags & 7);
            merge_storage_spec(&storage, conversion);
        } else if (((conversion.flags & 7) && ((conversion.flags ^ storage.flags) & 7)) ||
                   merge_storage_spec(&storage, conversion)) {
            report_syntax_error(71, declaration->name);
            storage.flags = conversion.flags | 8;
            storage.extended = conversion.extended;
            declaration->declaration_flags |= 2;
        }
        base = conversion_declaration.type;
    }
    kind = declaration->kind & 31;
    old_tag = kind == 2 || kind == 3;
    if (kind == 1)
        declaration->kind = 9;
    declaration->storage[0] = (unsigned char)storage.flags;
    declaration->storage[1] = (unsigned char)(storage.flags >> 8);
    declaration->storage[2] = storage.extended;
    normalize_symbol_type(base, declaration);
    context_flags = context->storage[0] | (context->storage[1] << 8);
    if ((context_flags & 0x10) && (context_flags & 0x20) &&
        !(declaration->declaration_flags & 0x3000) &&
        ((declaration->kind & 31) != 7 ||
         (!(declaration->declaration_flags & 0x4000) && function_return_type(declaration)->related)))
        report_syntax_error(501, format_symbol_name(declaration));
    if (declaration->storage[2] & 1) {
        if (!(declaration->storage[1] & 1) || (declaration->declaration_flags & 2)) {
            if (old_tag || (declaration->kind & 31) != 7)
                report_symbol_declaration_error(488, declaration);
        } else
            compiler_warning(1, 271, format_symbol_name(declaration));
    } else if (declaration_mode == 2 && function_symbol &&
               (function_symbol->storage[2] & 1) &&
               (declaration->storage[0] & 7) > 3 && (declaration->storage[0] & 7) < 7 &&
               ((is_unqualified_class_type(declaration->type) &&
                 !(CLASS_SYMBOL(declaration->type)->detail.class_info->flags2 & 1)) ||
                (declaration->attributes & 4)))
        report_syntax_error(489, format_symbol_name(declaration));
    if (declaration->storage[2] & 2) {
        kind = declaration->kind & 31;
        if ((kind == 9 || kind == 3) && (declaration->storage[0] & 7) &&
            (declaration->storage[0] & 7) < 4) {
            classification = classify_incomplete_type(declaration->type);
            if ((classification == 0x80 || classification == 2) &&
                !(CLASS_SYMBOL(declaration->type)->detail.class_info->flags2 & 1))
                report_symbol_declaration_error(483, declaration);
        } else {
            report_syntax_error(480, format_symbol_name(declaration));
            declaration->storage[2] &= ~2;
        }
        if (declaration->storage[2] & 4)
            report_syntax_error(492, format_symbol_name(declaration));
    }
    if (declaration->declaration_flags & 2) {
        kind = declaration->kind & 31;
        new_tag = kind == 2 || kind == 3;
        if (new_tag != old_tag && kind != 6 && kind != 7)
            return declaration;
    }
    if (qualifier) {
        qualified = lookup_qualified_declaration(qualifier, declaration);
        if (!qualified)
            return 0;
        target = (CppSymbol *)(*qualified->resolution->binding)->target;
        if ((declaration->kind & 31) == 7) {
            merge_declared_symbol(target, declaration);
            return (CppSymbol *)(*qualified->resolution->binding)->target;
        }
        if (!(target->attributes & 0x40) || (target->kind & 31) != 3) {
            if (token_allocation_flags & 0x400) {
                merge_declared_symbol(target, declaration);
                return (CppSymbol *)(*qualified->resolution->binding)->target;
            }
            return declaration;
        }
        alias = static_member_alias(target);
        declaration->name = alias->name;
        declaration->storage[0] = (declaration->storage[0] & ~5) | 2;
        declaration->attributes |= 0x2000;
    }
    if ((declaration->storage[0] & 7) == 7) {
        declaration->flags |= 0x10;
        if (declaration->name)
            SYMBOL_FROM_NAME(declaration->name)->flags |= 2;
        if (is_class_name_type(declaration->type)) {
            class_symbol = CLASS_SYMBOL(declaration->type);
            if (class_symbol->flags & 0x80) {
                if ((declaration->kind & 31) == 3)
                    SYMBOL_FROM_NAME(declaration->name)->flags |= 4;
                class_symbol->name = declaration->name;
                class_symbol->detail.class_info->flags2 |= 0x20;
                class_symbol->flags &= ~0x80;
            }
        }
    }
    if (old_tag)
        return declaration;
    symbol = lookup_declaration(declaration);
    if (symbol)
        existing_scope = 1;
    else if (declaration_mode >= 2)
        symbol = lookup_outer_declaration(declaration);
    else if (!options.configuration.z_a && (current_scope->flags & 0xff) == 1) {
        target = find_outer_name(declaration->name);
        if (target && (target->flags & 0x10))
            compiler_warning(1, 224, format_symbol_name(declaration));
        symbol = 0;
    }
    if (!symbol) {
        if (!declaration->index) {
            if (next_symbol_index >= 0x7fffffffU)
                fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_symbol_rollover();
            declaration->index = next_symbol_index++;
        }
        if (declaration_mode >= 2)
            record_local_declaration(declaration);
        if ((declaration->kind & 31) == 9)
            declaration->detail.scope_number = (!(token_allocation_flags & 0x40) &&
                (declaration->storage[0] & 7) != 1) ? current_declaration_scope : 0;
        if (context->storage[1] & 0x20)
            insert_scope_symbol(global_scope, declaration);
        else
            insert_symbol(declaration);
        symbol = declaration;
    } else {
        if (declaration->attributes & 0x80)
            symbol->attributes |= 0x80;
        if (existing_scope && (declaration->attributes & 0x2000))
            symbol->attributes |= 0x2000;
    }
    return symbol;
}
#undef STORAGE_KIND
#undef CLASS_SYMBOL

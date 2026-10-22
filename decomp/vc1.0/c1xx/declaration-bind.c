/* Final checks after declaration binding. */
#include "declarations.h"
#include "global-ir.h"
#include "ir.h"

/* Original 0x51c60..0x51fb2. Resolve a declaration, then apply the
   function/object checks which depend on the resulting shared symbol. */
CppSymbol *__stdcall apply_declaration_type(DeclarationType *context, CppSymbol *declaration)
{
    QualifiedDeclarator *qualifier = (QualifiedDeclarator *)declaration->next;
    ClassReference *reference;
    CppSymbol *symbol;
    TypeShape *shape;
    unsigned kind, storage;
    short text;
    int unwind = 0;
    if (qualifier) {
        if (!qualifier->resolution)
            return 0;
        reference = qualifier->resolution->binding->target;
        if (!is_static_data_member((CppSymbol *)reference))
            unwind = 1;
        else if (declaration_mode)
            report_syntax_error(655, declaration->name);
    }
    if ((cpp_parser_flags & 0x80) && !declaration_mode && !(context->storage[0] & 7))
        context->storage[0] = (context->storage[0] & ~0x26) | 1;
    symbol = resolve_declaration(context, declaration);
    if (unwind)
        unwind_qualified_scopes(1);
    if (!symbol) {
        begin_type_diagnostic();
        text = format_declarator(declaration, 0);
        report_syntax_error(244, type_diagnostic_text(text));
        end_type_diagnostic();
        declaration->declaration_flags |= 2;
        return 0;
    }
    if ((symbol->kind & 31) == 4)
        return symbol;
    if (browser_filename && (!declaration_mode || options.option_FR)) {
        if (!symbol->index) {
            if (next_symbol_index >= 0x7fffffffU)
                fatal_error(55);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_symbol_rollover();
            symbol->index = next_symbol_index++;
        }
        browser_declaration(symbol);
    }
    kind = symbol->kind & 31;
    if (kind != 6 && kind != 7) {
        if (((symbol->storage[0] & 7) != 1 || (symbol->attributes & 4) ||
             (!(symbol->storage[1] & 0x10) && (symbol->attributes & 0x500) != 0x500)) &&
            (!(symbol->storage[2] & 4) || (symbol->storage[2] & 8)))
            symbol->flags |= 4;
        if (qualifier) {
            reference = qualifier->resolution->binding->target;
            if (is_static_data_member((CppSymbol *)reference))
                return declare_qualified_member(symbol, qualifier, (CppSymbol *)reference);
            if (token_allocation_flags & 0x400)
                return symbol;
            report_syntax_error(350, format_qualified_name(qualifier->resolution->binding, 0));
        }
        if ((symbol->storage[2] & 4) && !(symbol->storage[2] & 8) &&
            ((symbol->flags & 4) || (symbol->attributes & 4)) &&
            (!(token_allocation_flags & 0x80000) || !(function_symbol->storage[2] & 4) ||
             (function_symbol->storage[2] & 8) || !(function_symbol->storage[1] & 1)))
            report_syntax_error(491, format_symbol_name(symbol), "data");
        storage = symbol->storage[0] & 7;
        if (storage && storage != 7 && storage != 1 && !(symbol->attributes & 4) &&
            (!(symbol->flags & 4) || !symbol_construction_class(symbol)) &&
            (symbol->kind & 31) == 9 && declaration == symbol) {
            if (symbol_has_array_shape(symbol))
                report_syntax_error(530, symbol->name);
            else if (symbol->attributes & 0x400) {
                shape = symbol->type->related->related;
                if (shape && (shape->flags & 0x40))
                    report_syntax_error(530, symbol->name);
            }
            if (symbol->storage[2] & 2)
                initialize_zero_object(symbol);
        }
        finalize_object_storage(symbol);
        global_expression_scope_active = 1;
        return symbol;
    }
    if (symbol->attributes & 4) {
        report_syntax_error(72, symbol->name);
        symbol->type = type_0064;
        symbol->kind = (symbol->declaration_flags & 0x400) ? 3 : 9;
    }
    if (symbol->attributes & 0x10)
        report_syntax_error(136, symbol->name);
    validate_special_function(symbol, 0);
    if (context->storage[2] & 1)
        report_syntax_error(488, format_symbol_name(symbol));
    return symbol;
}

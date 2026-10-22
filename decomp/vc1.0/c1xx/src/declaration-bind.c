/* Final checks after declaration binding. */
#include "declarations.h"
#include "global-ir.h"
#include "ir.h"
#include "work-list.h"
#include "cpp-context.h"
#include "function-definitions.h"

SegmentSymbol *default_data_segment; /* original 0x9f3e8 */
typedef char ClassInfoDestructorTraitsOffsetMustBe54[
    (offsetof(ClassInfo, trivial_special_members) == 54) ? 1 : -1];
void __stdcall check_global_object_definition(CppSymbol *symbol);
int __stdcall has_destructor_member_model(CType *type);
void __stdcall defer_object_definition(CppSymbol *symbol);

/* Original 0x51960..0x519d0. Global nontrivial objects require a usable
   destructor model; defer their definition when destruction is nontrivial. */
void __stdcall check_global_object_definition(CppSymbol *symbol)
{
    CppSymbol *tag;
    if (error_count || (symbol->declaration_flags & 2) || declaration_mode) return;
    if (!symbol_construction_class(symbol)) return;
    if (has_destructor_member_model(symbol->type)) {
        tag = cpp_context_stack->items[symbol->type->field08];
        if (!(tag->detail.class_info->trivial_special_members & 8))
            defer_object_definition(symbol);
        return;
    }
    report_syntax_error(262, symbol->name);
    symbol->declaration_flags |= 2;
}

/* Original 0x526c0..0x528fe. Retain the incoming storage class while
   validating qualifiers, registering cleanup and applying segment defaults. */
void __stdcall finalize_object_storage(CppSymbol *symbol)
{
    unsigned storage;
    TypeShape *shape;
    CppSymbol *tag;
    if (symbol->flags & 0x10) return;
    storage = symbol->storage[0] & 7;
    if (storage == 1 || storage == 7) return;
    if (storage >= 4 && storage <= 6) {
        shape = symbol->type->related;
        if (shape && !(shape->flags & 0x77)) {
            report_syntax_error(185, format_symbol_name(symbol));
            symbol->type = intern_type((TypeDescription *)strip_based_shape(symbol->type));
        }
        if (storage >= 5) {
            if (options.configuration.option_qc && normalized_Ol)
                symbol->references += 16;
            if (options.option_m) {
                write_ir_record(0x2a, symbol_reference(symbol));
                push_work_list(&scope_cleanup_stack, 1);
                scope_cleanup_stack->value.pointer = symbol;
                push_work_list(&scope_cleanup_stack, 1);
                scope_cleanup_stack->value.byte = declaration_mode;
            }
        }
        if (options.configuration.option_qc && normalized_Ol && !current_qc_context)
            analyze_inline_parameter(symbol);
    }
    if ((storage == 2 || storage == 3) && default_data_segment &&
        (symbol->attributes & 4)) {
        if (symbol->attributes & 2) {
            if (symbol->type->related->data.based_symbol == default_data_segment) return;
            report_syntax_error(193, symbol->name);
            return;
        }
        assign_symbol_segment(symbol, default_data_segment);
    }
    check_global_object_definition(symbol);
    if (!(symbol->attributes & 4) && !type_size(symbol->type) && storage != 1) {
        if (symbol->type->flags & 0x400) {
            tag = cpp_context_stack->items[symbol->type->field08];
            if ((tag->kind & 31) != 4)
                tag = cpp_context_stack->items[tag->type->field08];
            if (!(tag->flags & 4)) {
                report_syntax_error(79, symbol->name, tag->name);
                symbol->declaration_flags |= 2;
                return;
            }
        }
        report_syntax_error(133, symbol->name);
    }
}

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
            if (symbol_has_reference_shape(symbol))
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

/* Original 0x55e70..0x55ea7. A based qualifier may precede the actual
   declarator shape; the reference bit belongs to that following shape. */
int __stdcall symbol_has_reference_shape(CppSymbol *symbol)
{
    TypeShape *shape;
    if (!symbol) return 0;
    shape = symbol->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    return shape && (shape->flags & 0x40) ? 1 : 0;
}

/* Reference binding and temporary lifetime. */
#include "reference-initializers.h"
#include "constructor-initializers.h"
#include "unary-expressions.h"
#include "frontend-records.h"
#include "parser.h"
#include "operators.h"
#include "expression-preparation.h"
#include "call-expressions.h"
#include <recon.h>

/* Original 0x67100..0x671ec, stdcall 8. */
IrToken *__stdcall convert_reference_initializer(CType *type, IrToken *expression)
{
    int extension_binding = 0;
    int is_const;
    CType *referent = dereference_pointer_type(type);
    IrToken *result;

    if (!reference_requires_temporary(referent, expression, &extension_binding))
        return bind_reference_lvalue(referent, type, expression);
    is_const = (type_qualifiers(referent) & 0x1000) != 0;
    if (extension_binding && options.configuration.extensions_enabled) {
        if (!is_const)
            report_type_conversion(270, 2, 0x7e, type, expression->type);
        if (initializing_reference)
            expression = retain_reference_temporary(referent, type, expression);
        return bind_reference_lvalue(referent, type, expression);
    }
    result = try_user_defined_conversion(type, expression, 0);
    if (!result) {
        if (is_const)
            return initialize_reference_temporary(referent, type, expression);
        report_type_conversion(extension_binding ? 609 : 607, 0, 0x7e,
            expression->type, type);
    }
    return result;
}

/* Original 0x671f0..0x67227, stdcall 4. */
int __stdcall reference_source_is_array(CType *type)
{
    if (!type || !type->related) return 0;
    return (type->related->flags & 0x77) == 2;
}

/* Original 0x67230..0x6727a, stdcall 12. */
int __stdcall reference_requires_temporary(CType *referent, IrToken *expression,
    int *extension_binding)
{
    if (!reference_types_compatible(referent, expression->type)) return 1;
    if (reference_lvalue_bindable(expression, extension_binding)) return 0;
    if (reference_source_is_array(expression->type)) return 0;
    return 1;
}

/* Original 0x67350..0x6742a, stdcall 8. Source flags are saved before
   reference stripping, as in the original comparison. */
int __stdcall reference_types_compatible(CType *referent, CType *source)
{
    unsigned short target_flags = referent->flags;
    unsigned short source_flags = source->flags;
    CType *value = strip_reference_type(source);
    TypeShape *shape = referent->related;
    if (value->related != shape) return 0;
    if (shape)
        return !((source_flags ^ target_flags) & 0x2fdf) &&
            value->field08 == referent->field08;
    if ((value->attributes & 0x1000) && !(referent->attributes & 0x1000))
        return 0;
    if (!(referent->flags & 0x9c0) && !shape) {
        if ((source_flags & 0x600) && reference_tag_compatible(referent, value))
            return 1;
        return 0;
    }
    if (((value->flags & 0x9c0) || value->related) && source_flags == target_flags)
        return 1;
    return 0;
}

/* Original 0x62360..0x6237c, stdcall 4. */
CType *__stdcall strip_reference_type(CType *type)
{
    if (type && type->related && (type->related->flags & 0x40))
        type = dereference_pointer_type(type);
    return type;
}

/* Original 0x67430..0x67459, stdcall 8. The ancestry helper's
   arguments are source first, target second. */
int __stdcall reference_tag_compatible(CType *referent, CType *source)
{
    if (source->field08 != referent->field08 &&
        !type_has_base_relation(source, referent)) return 0;
    return 1;
}

/* Original 0x67280..0x672b9, stdcall 8. */
int __stdcall reference_lvalue_bindable(IrToken *expression, int *extension_binding)
{
    if (!modifiable_lvalue(expression, 0)) return 0;
    if (reference_lvalue_needs_extension(expression)) {
        *extension_binding = 1;
        return 0;
    }
    return 1;
}

/* Original 0x672c0..0x67347, stdcall 4. This exceptional lvalue
   form is an indirection of a call whose function shape carries bit 0x200. */
int __stdcall reference_lvalue_needs_extension(IrToken *expression)
{
    IrToken *call, *callee;
    TypeShape *shape;
    if (!expression || expression->kind != OP_DEREFERENCE) return 0;
    call = expression->value.pointer;
    if (!call || call->kind != 0x7a) return 0;
    callee = call->value.pointer;
    if (!callee || !callee->type) return 0;
    shape = callee->type->related;
    if (!shape || (shape->flags & 0x77) != 4) return 0;
    return (shape->flags & 0x200) ? 1 : 0;
}

/* Original 0x674c0..0x67546, stdcall 12. */
IrToken *__stdcall bind_reference_lvalue(CType *referent, CType *reference,
    IrToken *expression)
{
    IrToken *address, *result;
    TypeShape *shape;
    unsigned rank;
    if ((referent->flags & 0x400) && !referent->related) {
        address = try_class_pointer_initializer(reference, expression);
    } else {
        address = builtin_unary_expression(OP_BIT_AND, expression);
        shape = address->type->related;
        rank = rank_pointer_attributes(1,
            (unsigned char)reference->related->attributes,
            (unsigned char)shape->attributes, shape);
        if (rank & 0x2000) {
            report_type_conversion(440, 0, 0x7e, address->type, reference);
            return 0;
        }
        address = new_cast_expression(reference, address);
    }
    result = indirect_storage_expression(address);
    result->type = referent;
    return result;
}

IrToken *__stdcall build_scalar_reference_temporary(CType *referent,
    CType *reference, IrToken *expression);

/* Original0x67550..0x67593, stdcall12. Class temporaries use the
   ordinary operand conversion; scalar references need an addressable value. */
IrToken *__stdcall initialize_reference_temporary(CType *referent, CType *reference,
    IrToken *expression)
{
    IrToken *result;
    if (is_unqualified_class_type(referent))
        result = convert_operand_if_needed(1,referent,expression);
    else result = build_scalar_reference_temporary(referent,reference,expression);
    initializing_symbol = 0;
    initializing_reference = 0;
    return result;
}

void diagnose_member_reference_temporary(void);

/* Original0x675a0..0x67628, stdcall12. The reference-type formal is
   unused; transfer constructor storage to a temporary with its new lifetime. */
IrToken *__stdcall retain_reference_temporary(CType *referent, CType *reference,
    IrToken *expression)
{
    IrToken *temporary, *result;
    CppSymbol *symbol;
    unsigned char storage;
    if (!find_constructor_result_reference(expression->value.pointer, &temporary)) return 0;
    diagnose_member_reference_temporary();
    storage = member_initializer_active ? 4 : initializing_symbol->storage[0] & 7;
    symbol = create_initializer_temporary(storage, referent);
    result = replace_temporary_reference(expression, temporary->value.integer,
        symbol_reference(symbol));
    cancel_temporary_cleanup(temporary->value.integer);
    record_initialized_temporary(symbol, referent);
    return result;
}

/* Original0x67720..0x67754. Member initializers diagnose this
   temporary-lifetime extension using the active member's actual name. */
void diagnose_member_reference_temporary(void)
{
    if (member_initializer_active) {
        if (options.configuration.extensions_enabled)
            report_syntax_error(354, initializing_symbol->name);
        else compiler_warning(1, 354, initializing_symbol->name);
    }
}

/* Original0x3cc70..0x3cd26, stdcall8. Build an anonymous declaration
   with a private mutable type description and restore declaration context. */
CppSymbol *__stdcall create_initializer_temporary(unsigned char storage, CType *type)
{
    unsigned char saved_mode = declaration_mode;
    TypeDescription *description = copy_type_description(type);
    DeclarationType context;
    CppSymbol *symbol;
    description->attributes &= 0xeff8;
    if (storage != 3 && ((token_allocation_flags & 0x40) || !declaration_mode))
        storage = 3;
    if (storage == 3) declaration_mode = 0;
    symbol = new_declarator(make_anonymous_identifier());
    context.type = (CType *)description;
    context.storage[0] = (declaration_storage[0] & ~7) | (storage & 7);
    context.storage[1] = declaration_storage[1];
    context.storage[2] = declaration_storage[2];
    symbol = apply_declaration_type(&context, symbol);
    declaration_mode = saved_mode;
    --type_description_depth;
    return symbol;
}

/* Original0x3dd70..0x3de47, stdcall8. Retained nontrivial class
   temporaries own a symbol cleanup record; finish the active initialization. */
void __stdcall record_initialized_temporary(CppSymbol *symbol, CType *type)
{
    DeclarationFrontendRecord record = {0};
    CppSymbol *tag;
    record.header.kind = 13;
    if (ir_records_suppressed) return;
    if (declaration_mode && !allow_incomplete_class && type) {
        tag = cpp_context_stack->items[type->field08];
        if ((tag->kind & 31) == 4 && (tag->declaration_flags & 0x2000) &&
            !(tag->detail.class_info->trivial_special_members & 8)) {
            record.header.kind |= 0xc0;
            record.symbol = symbol;
            append_frontend_record(&record.header, sizeof(record));
        }
    }
    initializing_symbol = 0;
    initializing_reference = 0;
}

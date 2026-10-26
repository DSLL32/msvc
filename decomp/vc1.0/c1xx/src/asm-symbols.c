/* C symbols appearing in assembler operand expressions. */
#include "asm.h"
#include "global-ir.h"
#include "unary-expressions.h"
#include "ir.h"
#include "characters.h"
#include <recon.h>

extern CppSymbol *translation_sentinel, *translation_placeholder;

/* Original 0x7fe73..0x80071. A symbol can denote data, a label or a
   stack/register-resident object. Its operand descriptor retains the symbol
   while the expression carries the remaining displacement contribution. */
AsmExpression *__stdcall make_asm_symbol(char *name)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    CppSymbol *symbol = lookup_asm_symbol(name,1);
    unsigned kind = symbol->kind & 31, storage;
    short expression_kind;
    char *context;
    if (kind < 7 || kind > 10) {
        context = asm_error_context();
        report_syntax_error(420,name,context);
        expression_kind = 0;
    } else {
        asm_operand->address->address_flags &= 0xe8;
        asm_operand->address->address_flags |= 3;
        if (!symbol->type->related && (symbol->type->flags & 0x100))
            asm_operand->address->address_flags |= 0x20;
        ++symbol->references;
        if (!(asm_operand->flags & 1)) {
            if ((kind != 8 && kind != 7) ||
                ((asm_operand_class != 16 && asm_operand_class != 15 &&
                  asm_operand_class != 27 && asm_operand_class != 14) &&
                 symbol != translation_sentinel && symbol != translation_placeholder)) {
                asm_operand->address->address_flags &= 0xef;
                asm_operand->address->address_flags |= 0x10;
            }
            asm_operand->size = asm_size_encoding(asm_symbol_type_size(symbol));
        } else if (!asm_address_mode) {
            asm_operand->address->address_flags &= 0xef;
            asm_operand->address->address_flags |= 0x10;
        }
        storage = symbol->storage[0] & 7;
        if (storage > 3 && storage < 7) {
            if (asm_operand->address->symbol) compiler_warning(1,411,interned_name);
            asm_operand->address->address_flags &= 0xde;
            if (options.option_m && storage > 4 && storage < 7) {
                expression = make_asm_register(8);
                asm_operand->register_symbol = symbol;
                return expression;
            }
            expression = make_asm_address_register(6);
            expression->value.integer = 0;
            asm_operand->frame_symbol = symbol;
            return expression;
        }
        if (!asm_operand->address->symbol) asm_operand->address->symbol = symbol;
        expression_kind = 2;
    }
    if (pool->remaining < (int)sizeof(AsmExpression)) refill_arena(&compiler_arena,pool);
    expression = (AsmExpression *)pool->next;
    pool->remaining -= sizeof(AsmExpression);
    pool->next += sizeof(AsmExpression);
    expression->kind = expression_kind;
    expression->value.integer = 0;
    return expression;
}

/* Original 0x7dd7d..0x7de4f, stdcall8. Special interned spellings
   retain symbol identity; C++ overload sets must name one definite function. */
CppSymbol *__stdcall lookup_asm_symbol(char *name, int allow_label)
{
    CppSymbol *symbol;
    unsigned kind;
    char *context;
    if (name == name_dollar) return translation_sentinel;
    if (name == anonymous_symbol_name) {
        if (!(function_symbol->storage[2] & 1)) report_syntax_error(486);
        return translation_placeholder;
    }
    symbol = find_outer_name(name);
    if (!symbol) {
        if (!allow_label) {
            context = asm_error_context();
            report_syntax_error(406, name, context);
            symbol = new_symbol(name, declaration_mode);
        } else {
            symbol = find_label(name);
            if (!symbol) {
                symbol = new_label(name);
                register_label(symbol);
            }
        }
    }
    kind = symbol->kind & 31;
    if (kind == 16 || kind == 17) {
        if (has_single_function(symbol)) symbol = ((OverloadSet *)symbol)->functions;
        else {
            report_syntax_error(244, symbol->name);
            symbol = new_symbol(name, declaration_mode);
        }
    }
    return symbol;
}

/* Original 0x7d9f0..0x7da3d, stdcall4. Code labels and
   four-byte function symbols use the near-address sentinel. */
unsigned __stdcall asm_symbol_type_size(CppSymbol *symbol)
{
    CType *type;
    if ((symbol->kind & 31) == 7 && type_size(symbol->type) == 4) return 65535;
    if ((symbol->kind & 31) == 8) return 65535;
    type = symbol->type;
    if (symbol->attributes & 0x400)
        type = drop_first_type_shape(strip_based_shape(type));
    return type_size(type);
}

/* Original0x7da3e..0x7da77, stdcall4. LENGTH uses the outer array
   bound, stepping past a representation prefix when its shape flags permit. */
unsigned __stdcall asm_symbol_length(CppSymbol *symbol)
{
    TypeShape *shape;
    if ((symbol->kind & 31) == 8 || !(symbol->attributes & 0x400)) return 1;
    shape = symbol->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    return shape->data.word;
}

/* Original0x8014c..0x801c5, stdcall4. OFFSET selects an immediate
   symbol address, preserving diagnostic arguments and recovery placeholder. */
AsmExpression *__stdcall make_asm_offset(AsmExpression *value)
{
    char *context;
    if (!(asm_operand->address->address_flags & 1)) {
        if (value->kind == 3) {
            context = asm_error_context();
            report_syntax_error(404,interned_name,context);
        } else report_syntax_error(415);
        asm_operand->address->symbol = translation_sentinel;
        value->kind = 2;
    }
    asm_operand->address->address_flags &= 0xe8;
    asm_operand->address->address_flags |= 3;
    asm_operand->size = 2;
    return value;
}

IrToken *__stdcall new_symbol_reference(unsigned char operation, CppSymbol *symbol, CType *type);

/* Original0x3bb80..0x3bc0e, stdcall4. Define every case-insensitive
   matching unresolved spelling, or register a new assembler label. */
void __stdcall emit_asm_label(char *name)
{
    CppSymbol *label;
    int found = 0;
    for (label = label_symbols; label; label = label->next) {
        if (!label->detail.value && !compare_names_ignore_case(name,label->name)) {
            found = 1;
            label->attributes |= 0x1000;
            if (label->flags & 4) report_syntax_error(412,label->name);
            else emit_label(new_symbol_reference(2,label,0));
        }
    }
    if (!found) {
        label = new_label(name);
        label->attributes |= 0x1000;
        register_label(label);
        emit_label(new_symbol_reference(2,label,0));
    }
}

CppSymbol *__stdcall find_asm_member(CppSymbol *members, char *name);
CppSymbol *__stdcall find_asm_scope_member(char *name);

/* Original 0x7dc13..0x7dd61. A dotted assembler field retains the
   current aggregate tag; otherwise infer it from the real operand symbol.
   Scalar fields update operand size, nested aggregates retain their context. */
unsigned __stdcall asm_member_offset(char *name)
{
    CppSymbol *symbol;
    char *context;
    unsigned kind;
    if (!asm_member_context || !asm_member_context->auxiliary.scope_members) {
        symbol = asm_operand->address->symbol;
        if (symbol && (asm_operand->address->address_flags & 1))
            asm_member_context = cpp_context_stack->items[(short)symbol->type->field08];
        else if (asm_operand->frame_symbol)
            asm_member_context = cpp_context_stack->items[
                (short)asm_operand->frame_symbol->type->field08];
        else if (asm_operand->register_symbol)
            asm_member_context = cpp_context_stack->items[
                (short)asm_operand->register_symbol->type->field08];
        else
            asm_member_context = 0;
    }
    if (asm_member_context && asm_member_context->auxiliary.scope_members)
        symbol = find_asm_member(asm_member_context->auxiliary.scope_members, name);
    else
        symbol = find_asm_scope_member(name);
    if (symbol) {
        kind = symbol->kind & 31;
        if ((kind == 2 || kind == 3) && (symbol->storage[0] & 7) != 3) {
            asm_member_context = cpp_context_stack->items[(short)symbol->type->field08];
            if (!asm_member_context) {
                if (kind == 2)
                    compiler_warning(1, 401, symbol->name);
                asm_operand->size = asm_size_encoding(asm_symbol_type_size(symbol));
            }
            return symbol->detail.value;
        }
    }
    context = asm_error_context();
    report_syntax_error(411, name, context);
    return 0;
}

/* Original 0x7dae6..0x7db0b. Hidden alignment declarations cannot
   satisfy an assembler field name, even if their interned spelling matches. */
CppSymbol *__stdcall find_asm_member(CppSymbol *members, char *name)
{
    while (members) {
        if (members->name != name_alignment && members->name == name)
            break;
        members = members->next;
    }
    return members;
}

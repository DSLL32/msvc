/* C symbols appearing in assembler operand expressions. */
#include "asm.h"
#include "global-ir.h"
#include "unary-expressions.h"
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

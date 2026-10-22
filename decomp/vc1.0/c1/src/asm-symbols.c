/* C symbols appearing in assembler operand expressions. */
#include "asm.h"
#include "global-ir.h"
#include <recon.h>

extern FrontendSymbol *translation_sentinel, *translation_placeholder;

/* Original 0x50342..0x50540. A symbol can denote data, a label or a
   stack/register-resident object. Its operand descriptor retains the symbol
   while the expression carries the remaining displacement contribution. */
AsmExpression *__stdcall make_asm_symbol(char *name)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    AsmExpression *expression;
    FrontendSymbol *symbol = lookup_asm_symbol(name,1);
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

/* Original 0x4e290..0x4e323. '$' and __LOCAL_SIZE are persistent special
   symbols. Other unresolved names either recover a C declaration or create
   an assembler-visible label, according to the caller's lookup mode. */
FrontendSymbol *__stdcall lookup_asm_symbol(char *name, int allow_label)
{
    FrontendSymbol *symbol;
    char *context;
    if (name == name_dollar) return translation_sentinel;
    if (name == anonymous_symbol_name) {
        if (!(function_symbol->storage[2] & 1)) report_syntax_error(486);
        return translation_placeholder;
    }
    symbol = find_identifier_symbol(name);
    if (!symbol) {
        if (!allow_label) {
            context = asm_error_context();
            report_syntax_error(406,name,context);
            symbol = make_implicit_declaration(name,parameter_nesting);
        } else {
            symbol = find_label(name);
            if (!symbol) {
                symbol = declare_label(name);
                reference_label(symbol);
            }
        }
    }
    return symbol;
}

/* Original 0x4df51..0x4df9e. Code labels and four-byte function pointers
   use the assembler's near-address sentinel instead of an ordinary size. */
unsigned __stdcall asm_symbol_type_size(FrontendSymbol *symbol)
{
    CType *type;
    if ((symbol->kind & 31) == 7 && type_size(symbol->type) == 4) return 65535;
    if ((symbol->kind & 31) == 8) return 65535;
    type = symbol->type;
    if (symbol->attributes & 0x400)
        type = drop_first_type_shape(remove_type_prefix(type));
    return type_size(type);
}

/* Enum constants, their enclosing tag table and ordinary scope bindings. */
#include "enumerators.h"
#include "lexer.h"
#include "parameters.h"
#include "binary-expressions.h"
#include "cpp-context.h"
#include <recon.h>

/* Original 0x2edc0..0x2ef8e. The 28-byte enumeration record can be
   replaced by an existing scope declaration before its tag table and numeric
   value are populated. Keep those two identities in the same order. */
void __stdcall declare_enumerator(char *name, CType *context, unsigned value)
{
    EnumeratorSymbol *symbol;
    CppSymbol *existing;
    HashTable *enumerators;
    ArenaPool *pool;
    if (!context) return;
    if ((declaration_depth > 0 && !(token_allocation_flags & 0x80000)) ||
        !declaration_mode) symbol = allocate_permanent(sizeof(EnumeratorSymbol));
    else {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(EnumeratorSymbol)) refill_arena(&compiler_arena, pool);
        symbol = (EnumeratorSymbol *)pool->next;
        pool->remaining -= sizeof(EnumeratorSymbol);
        pool->next += sizeof(EnumeratorSymbol);
    }
    symbol->kind = 5;
    symbol->name = name;
    symbol->type = context;
    if (!(token_allocation_flags & 0x80000) && !pch_separate_debug &&
        !(current_enum_symbol->flags & 0x80)) symbol->flags |= 8;
    else symbol->flags &= ~8;
    existing = lookup_current_scope_declaration((CppSymbol *)symbol);
    if (existing) symbol = (EnumeratorSymbol *)existing;
    else if (is_current_class_name(symbol->name)) report_syntax_error(458, symbol->name);
    else insert_current_scope_symbol((CppSymbol *)symbol);
    enumerators = current_enum_symbol->auxiliary.enumerators;
    if (!enumerators) return;
    set_hash_value(enumerators, name, symbol);
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    symbol->index = next_symbol_index++;
    if (integer_constant_overflows(4, 0, 0, value))
        compiler_warning(options.configuration.extensions_enabled ? 3 : 1,
            341, format_symbol_name((CppSymbol *)symbol));
    symbol->value = value;
    enumerator_value = value;
    if (current_class_context >= 0)
        symbol->flags = (symbol->flags & ~3) |
            (class_contexts[current_class_context].flags & 3);
    if (browser_filename && (!declaration_mode || options.option_FR))
        browser_declaration((CppSymbol *)symbol);
}

/* Original 0x2ed90..0x2edb8. Warn at the signed-int maximum,
   retaining the unsigned increment used for enum value transport. */
unsigned __stdcall next_enumerator_value(unsigned value, char *name)
{
    if (value == (unsigned)maximum_signed_int_constant)
        compiler_warning(1, 340, name);
    return value + 1;
}

/* Original 0x2ddb0..0x2ddef. Close browser tag state for a visible
   enumeration, then return its already interned declaration context. */
CType *end_enum_specifier(void)
{
    CppSymbol *tag = cpp_context_stack->items[enumerator_context->field08];
    if (!(tag->flags & 0x80) && browser_filename &&
        (!declaration_mode || options.option_FR)) browser_end_tag();
    return enumerator_context;
}

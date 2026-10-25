#include "frontend.h"
#include "keywords.h"
#include <recon.h>

void RECON_THISCALL CompilerSymbolTable::Insert(CompilerIrSymbol *symbol)
{
    int local = level >= 1 && level < 128;
    symbol->scope_bits = (symbol->scope_bits & ~2) | (local ? 2 : 0);
    int marked = local || c1_bss.declaration_scope_active;
    symbol->scope_bits = (symbol->scope_bits & ~4) |
        ((marked && !c1_data.c9_il) ? 4 : 0);
    if ((symbol->scope_bits & 4) && !c1_bss.debug_module->Slot48())
        compiler_error(73, "p1inl.inl", 648);
    if (!buckets) {
        CompilerArena *arena = &c1_bss.arenas[2];
        CompilerSymbolBinding **memory = (CompilerSymbolBinding **)arena->cursor;
        arena->cursor += (bucket_mask + 1) * 4;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        buckets = memory;
        c1_data.translation_unit->PushScope(this);
    }
    Link(symbol);
}
void RECON_THISCALL CompilerTranslationUnit::PushScope(CompilerSymbolTable *scope)
{ __asm { int 3 } }

void RECON_THISCALL CompilerSymbolTable::Link(CompilerIrSymbol *symbol)
{
    CompilerSymbolBinding *binding = &symbol->binding;
    unsigned index = (binding->name->flags & bucket_mask) & 0xffff;
    binding->next = buckets[index];
    buckets[index] = binding;
}

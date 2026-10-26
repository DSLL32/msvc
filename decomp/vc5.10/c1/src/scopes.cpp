#include <new.h>
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
void RECON_THISCALL CompilerTranslationUnit::PushScope(CompilerScopeTable *scope)
{
#ifdef RECON_NATIVE_TEST
    if (depth != 0)
        recon_unsupported("prior nested scope-stack entries are not yet modeled");
#endif
    if (scopes[depth] == scope || !scope->HasBuckets()) return;
    ++depth;
    if (depth >= 128) compiler_fatal(61);
    scopes[depth] = scope;
}

void RECON_THISCALL CompilerSymbolTable::Link(CompilerIrSymbol *symbol)
{
    CompilerSymbolBinding *binding = &symbol->binding;
    unsigned index = (binding->name->flags & bucket_mask) & 0xffff;
    binding->next = buckets[index];
    buckets[index] = binding;
}

void RECON_THISCALL CompilerParameterScope::Insert(CompilerIrSymbol *) { __asm { int 3 } }
void *RECON_THISCALL CompilerParameterScope::Find(CompilerNameEntry *) { __asm { int 3 } return 0; }
void *RECON_THISCALL CompilerParameterScope::Lookup(CompilerNameEntry *) { __asm { int 3 } return 0; }
void *RECON_THISCALL CompilerParameterScope::FindOrdinary(CompilerNameEntry *) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerParameterScope::FindNested(CompilerNameEntry *, void **) { __asm { int 3 } }
CompilerIrSymbol **RECON_THISCALL CompilerParameterScope::FindLink(CompilerIrSymbol *) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerParameterScope::Serialize(void) { __asm { int 3 } }
/* Source-only nonempty parameter chain; current scope construction supplies
 * no parameter value. The next link is the actual DWORD at+24. */
struct CompilerCheckedParameter {
    CompilerSymbolBinding binding;
    BYTE unmodeled_08[16];
    CompilerCheckedParameter *next;
};
struct CompilerCheckedParameters {
    BYTE unmodeled_00[8];
    CompilerCheckedParameter *first;
};
extern "C" void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
extern "C" void __cdecl write_global_reference(CompilerSymbolBinding *);
void RECON_THISCALL CompilerParameterScope::Check(void)
{
    CompilerCheckedParameter *parameter;
#ifdef RECON_NATIVE_TEST
    if (parameter_value)
        recon_unsupported("nonempty parameter-scope checking is not yet modeled");
#endif
    parameter = parameter_value ?
        ((CompilerCheckedParameters *)parameter_value)->first : NULL;
    if (c1_bss.parser_early_result) return;
    c1_bss.declaration_scope_active = level;
    write_global_record(13, NULL);
    while (parameter) {
        write_global_reference(&parameter->binding);
        parameter = parameter->next;
    }
}
void RECON_THISCALL CompilerParameterScope::Link(CompilerIrSymbol *) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::RegisterScope(CompilerScopeTable *scope)
{
    scope->next = current;
    current = scope;
    PushScope(scope);
}

void RECON_THISCALL CompilerTranslationUnit::UnregisterScope(void)
{
    PopScope();
    current = current->next;
}
void RECON_THISCALL CompilerTranslationUnit::PopScope(void)
{
    if (!current->HasBuckets()) return;
#ifdef RECON_NATIVE_TEST
    if (depth != 0)
        recon_unsupported("nested scope pop index is not yet modeled");
#endif
    if (scopes[depth] != current) {
        compiler_fatal(506);
        return;
    }
    scopes[depth] = NULL;
    --depth;
}

void RECON_THISCALL CompilerTranslationUnit::EnterBlockScope(void)
{
    CompilerArena *arena = &c1_bss.arenas[2];
    void *memory = arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    CompilerSymbolTable *scope = memory ?
        new(memory) CompilerSymbolTable(c1_bss.parameter_nesting, current) : NULL;
    RegisterScope(scope);
}

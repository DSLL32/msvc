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
    /* Initialize writes slot0. Every later current-depth slot is written by
     * this push before it can be read; Pop restores the preceding live slot. */
    if (depth >= 128) compiler_fatal(61);
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

void *RECON_THISCALL CompilerSymbolTable::Lookup(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = NULL;
    if (buckets) {
#ifdef RECON_NATIVE_TEST
        if (bucket_mask != 1023)
            recon_unsupported("tag lookup bucket extent is not yet modeled");
#endif
        unsigned index = (name->flags & bucket_mask) & 0xffff;
        for (CompilerSymbolBinding *binding = buckets[index]; binding; binding = binding->next) {
            if (binding->name == name) {
                CompilerIrSymbol *candidate = (CompilerIrSymbol *)((BYTE *)binding - 8);
                if (candidate->kind == 4) { symbol = candidate; break; }
            }
        }
    }
    if (!symbol) return level == 2 ? outer->Lookup(name) : NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = symbol;
    result->scope = this;
    return result;
}

void *RECON_THISCALL CompilerSymbolTable::FindOrdinary(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = NULL;
    if (bucket_mask != 1023 && bucket_mask != 15) {
#ifdef __linux__
        recon_unsupported("ordinary name bucket extent is not yet recovered");
#else
        __asm { int 3 }
#endif
    }
    unsigned index = (name->flags & bucket_mask) & 0xffff;
    for (CompilerSymbolBinding *binding = buckets[index]; binding; binding = binding->next) {
        if (binding->name == name) {
            CompilerIrSymbol *candidate = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (candidate->kind != 4) { symbol = candidate; break; }
        }
    }
    if (!symbol) return NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = symbol;
    result->scope = this;
    return result;
}

void RECON_THISCALL CompilerParameterScope::Insert(CompilerIrSymbol *symbol)
{
    symbol->scope_bits |= 2;
    int first = parameter_value == 0;
    Link(symbol);
    if (first) c1_data.translation_unit->PushScope(this);
}
void *RECON_THISCALL CompilerParameterScope::Find(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = parameter_value ?
        parameter_bindings->Find(name) : NULL;
    if (!symbol) return NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = symbol;
    result->scope = this;
    return result;
}
CompilerIrSymbol *RECON_THISCALL CompilerParameterBindings::Find(CompilerNameEntry *name)
{
    for (CompilerSymbolBinding *binding = first; binding; binding = binding->next) {
        if (binding->name == name) {
            CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (symbol->kind != 4) return symbol;
        }
    }
    return NULL;
}
void *RECON_THISCALL CompilerParameterScope::Lookup(CompilerNameEntry *) { __asm { int 3 } return 0; }
void *RECON_THISCALL CompilerParameterScope::FindOrdinary(CompilerNameEntry *name)
{
    CompilerIrSymbol *symbol = NULL;
    for (CompilerSymbolBinding *binding = parameter_bindings->first; binding; binding = binding->next) {
        if (binding->name == name) {
            CompilerIrSymbol *candidate = (CompilerIrSymbol *)((BYTE *)binding - 8);
            if (candidate->kind != 4) { symbol = candidate; break; }
        }
    }
    if (!symbol) return NULL;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerScopeLookup *result = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!result) return NULL;
    result->symbol = symbol;
    result->scope = this;
    return result;
}
void RECON_THISCALL CompilerParameterScope::FindNested(CompilerNameEntry *, void **) { __asm { int 3 } }
CompilerIrSymbol **RECON_THISCALL CompilerParameterScope::FindLink(CompilerIrSymbol *) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerParameterScope::Serialize(void) { __asm { int 3 } }
/* The scope retains the existing 12-byte parameter-binding owner. Check walks
 * its last binding backward through each symbol's proven +32 link. */
extern "C" void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
extern "C" void __cdecl write_global_reference(CompilerSymbolBinding *);
void RECON_THISCALL CompilerParameterScope::Check(void)
{
    CompilerSymbolBinding *parameter = parameter_bindings ? parameter_bindings->last : NULL;
    if (c1_bss.parser_early_result) return;
    c1_bss.declaration_scope_active = level;
    write_global_record(13, NULL);
    while (parameter) {
        write_global_reference(parameter);
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)parameter - 8);
        parameter = symbol->previous_parameter;
    }
}
/* These unvisited cells are excluded from the modeled/default contract.
 * Hold before their reads; the current constructor uses arena2 directly. */
static CompilerParameterBindings *unmodeled_parameter_binding_cache; /* RVA5f970 */
#define parameter_sentinel c1_data.parameter_sentinel /* RVA5e9ac, snapshottedNULL default. */
extern "C" CompilerIrSymbol *__cdecl create_parameter_sentinel(void);
void RECON_THISCALL CompilerParameterScope::Link(CompilerIrSymbol *symbol)
{
    CompilerParameterBindings *list = parameter_bindings;
    CompilerSymbolBinding *entry = &symbol->binding;
    if (!list) {
        unsigned arena_kind = c1_bss.parameter_nesting ? 2 : 0;
        if (!arena_kind) {
#ifdef __linux__
            recon_unsupported("parameter binding cache is not yet recovered");
#else
            __asm { int 3 }
#endif
            list = unmodeled_parameter_binding_cache;
            if (list) {
                unmodeled_parameter_binding_cache = list->free_next;
                list->arena_kind = 0;
                list->first = NULL;
                list->last = NULL;
            }
        }
        if (!list) {
            if (arena_kind) {
                CompilerArena *arena = &c1_bss.arenas[arena_kind];
                list = (CompilerParameterBindings *)arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                CompilerPool *pool = &c1_bss.pools[7];
                if (pool->remaining < 12) pool->Refill(12);
                list = (CompilerParameterBindings *)pool->cursor;
                pool->cursor += 12;
                pool->remaining -= 12;
            }
        }
        if (list) {
            list->arena_kind = arena_kind;
            list->first = NULL;
            list->last = NULL;
        }
        parameter_bindings = list;
    }
    if (entry) {
        entry->name = symbol->binding.name;
        entry->next = NULL;
        symbol->previous_parameter = list->last;
    }
    if (list->last) {
        if (!parameter_sentinel) parameter_sentinel = create_parameter_sentinel();
        CompilerIrSymbol *previous = (CompilerIrSymbol *)((BYTE *)list->last - 8);
        if (previous != parameter_sentinel) list->last->next = entry;
        else list->first = entry;
    } else list->first = entry;
    list->last = entry;
}
extern "C" CompilerIrSymbol *__cdecl lookup_cached_symbol_record(unsigned, CompilerNameEntry *, unsigned);
extern "C" CompilerIrSymbol *__cdecl create_parameter_sentinel(void)
{
    CompilerNameEntry *name = c1_data.builtin_names.formal_parameter;
    CompilerIrSymbol *symbol = lookup_cached_symbol_record(12, name, 0);
    if (!symbol) {
        if (c1_data.record_sizes.known.parameter_symbol_bytes != 52) {
#ifdef __linux__
            recon_unsupported("formal parameter symbol size is not yet recovered");
#else
            __asm { int 3 }
#endif
        }
        BYTE *memory = c1_bss.arenas[0].AllocateBytes(
            (c1_data.record_sizes.known.parameter_symbol_bytes + 3) & ~3U);
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(12, name, 0) : NULL;
    }
    symbol->intrinsic_flags = 4;
    if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
    symbol->record_id = (symbol->record_id & 0xff000000U) |
        (c1_data.next_symbol_record_id++ & 0xffffff);
    if ((symbol->reference_flags & 3) < 3)
        symbol->reference_flags = (symbol->reference_flags & ~3U) | ((symbol->reference_flags + 1) & 3);
    symbol->reference_flags = (symbol->reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2);
    symbol->value_type = c1_data.builtin_types.type_0080;
    return symbol;
}
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
    if (depth >= 128) compiler_fatal(506);
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

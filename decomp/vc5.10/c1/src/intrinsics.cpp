#include <string.h>
#include "frontend.h"
#include "keywords.h"
#pragma intrinsic(memset, memcpy, strlen)
#include <recon.h>

/* Encountered intrinsic declarations: the original uses a suffix-pointer
 * table, prefix ranges and twelve-byte metadata rows. Its unused WORD+2 is
 * omitted; these named fields are the actual consumers of the other five. */
struct CompilerIntrinsic {
    const char *suffix;
    WORD opcode, return_flags, shape_kind, traits, availability;
};
static const CompilerIntrinsic intrinsic_specs[63] = {
    { "strset", 0xa3, 0x41, 0x1, 0x2, 0x10 },
    { "setjmp", 0xdb, 0x63, 0x0, 0x81, 0x0 },
    { "rotr", 0xa0, 0x4063, 0x0, 0x2, 0x10 },
    { "rotl", 0x9f, 0x4063, 0x0, 0x2, 0x10 },
    { "outpw", 0xb0, 0x4042, 0x0, 0x2, 0x10 },
    { "outpd", 0xe1, 0x4044, 0x0, 0x2, 0x10 },
    { "outp", 0xa9, 0x63, 0x0, 0x2, 0x10 },
    { "lrotr", 0xa2, 0x4044, 0x0, 0x2, 0x10 },
    { "lrotl", 0xa1, 0x4044, 0x0, 0x2, 0x10 },
    { "inpw", 0xaf, 0x4042, 0x0, 0x1, 0x10 },
    { "inpd", 0xe0, 0x4044, 0x0, 0x1, 0x10 },
    { "inp", 0xa8, 0x63, 0x0, 0x1, 0x10 },
    { "exception_info", 0xda, 0x80, 0x1, 0x80, 0x0 },
    { "exception_code", 0xd9, 0x4044, 0x0, 0x80, 0x0 },
    { "enable", 0xb1, 0x80, 0x0, 0x0, 0x0 },
    { "disable", 0xb2, 0x80, 0x0, 0x0, 0x0 },
    { "alloca", 0xd8, 0x80, 0x1, 0x81, 0x0 },
    { "abnormal_termination", 0xd7, 0x63, 0x0, 0x80, 0x0 },
    { "_ull_rshift", 0xea, 0x404c, 0x0, 0x2, 0x20 },
    { "_ll_rshift", 0xeb, 0x4c, 0x0, 0x2, 0x20 },
    { "_ll_lshift", 0xe9, 0x404c, 0x0, 0x2, 0x20 },
    { "_emulu", 0xed, 0x404c, 0x0, 0x2, 0x20 },
    { "_emul", 0xec, 0x4c, 0x0, 0x2, 0x20 },
    { "ReturnAddress", 0xe5, 0x80, 0x1, 0x80, 0x0 },
    { "InterlockedIncrement", 0x11b, 0x44, 0x0, 0x1, 0x0 },
    { "InterlockedExchangeAdd", 0x13e, 0x44, 0x0, 0x2, 0x0 },
    { "InterlockedExchange", 0x11d, 0x44, 0x0, 0x2, 0x0 },
    { "InterlockedDecrement", 0x11c, 0x44, 0x0, 0x1, 0x0 },
    { "InterlockedCompareExchange", 0x13f, 0x80, 0x1, 0x3, 0x0 },
    { "InterlockedAddLargeStatistic", 0x14b, 0x44, 0x0, 0x2, 0x0 },
    { "tan2", 0x85, 0x107, 0x0, 0x2, 0x20 },
    { "tan", 0x83, 0x107, 0x0, 0x1, 0x20 },
    { "sin", 0x7f, 0x107, 0x0, 0x1, 0x20 },
    { "cos", 0x81, 0x107, 0x0, 0x1, 0x20 },
    { "bs", 0xf, 0x63, 0x0, 0x1, 0x10 },
    { "osh", 0x89, 0x107, 0x0, 0x1, 0x20 },
    { "os", 0x6f, 0x107, 0x0, 0x1, 0x20 },
    { "xp", 0x61, 0x107, 0x0, 0x1, 0x20 },
    { "mod", 0x18, 0x107, 0x0, 0x2, 0x20 },
    { "abs", 0x11, 0x107, 0x0, 0x1, 0x20 },
    { "npw", 0xaf, 0x4042, 0x0, 0x41, 0x10 },
    { "npd", 0xe0, 0x4044, 0x0, 0x41, 0x10 },
    { "np", 0xa8, 0x63, 0x0, 0x41, 0x10 },
    { "og10", 0x69, 0x107, 0x0, 0x1, 0x20 },
    { "og", 0x65, 0x107, 0x0, 0x1, 0x20 },
    { "abs", 0xf, 0x44, 0x0, 0x1, 0x10 },
    { "emset", 0xad, 0x80, 0x1, 0x3, 0x10 },
    { "emcpy", 0xac, 0x80, 0x1, 0x3, 0x10 },
    { "emcmp", 0xaa, 0x63, 0x0, 0x3, 0x10 },
    { "utpw", 0xb0, 0x4042, 0x0, 0x42, 0x10 },
    { "utpd", 0xe1, 0x4063, 0x0, 0x42, 0x10 },
    { "utp", 0xa9, 0x63, 0x0, 0x42, 0x10 },
    { "ow", 0xae, 0x107, 0x0, 0x2, 0x20 },
    { "trset", 0xa3, 0x41, 0x1, 0x42, 0x10 },
    { "trlen", 0xa7, 0x4063, 0x0, 0x1, 0x10 },
    { "trcpy", 0xa4, 0x41, 0x1, 0x2, 0x10 },
    { "trcmp", 0xa5, 0x63, 0x0, 0x2, 0x10 },
    { "trcat", 0xa6, 0x41, 0x1, 0x2, 0x10 },
    { "qrt", 0x5d, 0x107, 0x0, 0x1, 0x20 },
    { "inh", 0x87, 0x107, 0x0, 0x1, 0x20 },
    { "in", 0x6b, 0x107, 0x0, 0x1, 0x20 },
    { "anh", 0x8b, 0x107, 0x0, 0x1, 0x20 },
    { "an", 0x73, 0x107, 0x0, 0x1, 0x20 },
};
static const signed char intrinsic_prefix_ranges[29] = { 0, 30, 30, 35, 35, 37, 37, 38, 40, 40, 40, 43, 43, 43, 46, 49, 49, 52, 53, 53, 53, 61, 63, 63, 63, 63, 63, 63, 63 };
extern "C" {
int __cdecl intrinsic_enabled(unsigned index);
void __cdecl initialize_intrinsic_symbol(CompilerIrSymbol *, unsigned index);
CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
CompilerType *__cdecl make_function_type(unsigned, CompilerType *);
WORD __cdecl resolve_shape_attributes(unsigned, unsigned);
CompilerTypeShape *__cdecl make_type_shape(unsigned, unsigned, unsigned *);
}

extern "C" void __cdecl initialize_intermediate_output(void)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerType *description = (CompilerType *)arena->cursor;
    char name[1024];
    unsigned prefix, index;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    description->auxiliary = 0;
    /* Attributes and debug-cache bytes retain their allocation contents. */
    for (prefix = 0; prefix < 28; ++prefix) {
        for (index = intrinsic_prefix_ranges[prefix];
            index != (unsigned)intrinsic_prefix_ranges[prefix + 1]; ++index) {
            const CompilerIntrinsic *spec = &intrinsic_specs[index];
            if (c1_data.options.multithread_runtime && !spec->availability)
                continue;
            if (!intrinsic_enabled(index))
                continue;
            name[0] = (char)(prefix + '_');
            memcpy(name + 1, spec->suffix, strlen(spec->suffix) + 1);
            CompilerNameEntry *identifier = find_or_create_identifier(name);
            CompilerIrSymbol *symbol = NULL;
#ifdef RECON_NATIVE_TEST
            if (c1_data.intrinsic_symbol_cache_index != 9 ||
                c1_data.intrinsic_symbol_bytes != 52)
                recon_unsupported("intrinsic declarations: unmodeled symbol cache domain");
#endif
            /* Both producers share the same original head9 storage. */
            CompilerSymbolBinding *binding = c1_data.abstract_symbol_free;
            if (binding) {
                c1_data.abstract_symbol_free = binding->next;
                symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
                binding->name = NULL;
                binding->next = NULL;
                memset(symbol, 0, c1_data.intrinsic_symbol_bytes);
                binding->name = identifier;
                symbol->kind = 6;
            }
            if (!symbol) {
                BYTE *memory = c1_bss.arenas[0].AllocateBytes(
                    (c1_data.intrinsic_symbol_bytes + 3) & ~3U);
                symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(6, identifier, 0) : NULL;
            }
            if (c1_data.next_symbol_record_id >= 0x7fffffff)
                compiler_fatal(55);
            symbol->record_id = (symbol->record_id & 0xff000000) |
                (c1_data.next_symbol_record_id++ & 0xffffff);
            initialize_intrinsic_symbol(symbol, index);
            WORD type_flags = spec->return_flags;
            if ((type_flags & 0x2fcf) == 0x41 && c1_data.options.language_extensions &&
                c1_data.options.unsigned_char && !(type_flags & 0x8000))
                type_flags |= 0x4000;
            description->flags = type_flags;
            if (spec->shape_kind) {
                unsigned kind = spec->shape_kind & 7;
                description->shape = make_type_shape(kind, resolve_shape_attributes(kind, 0), NULL);
            } else
                description->shape = NULL;
            symbol->function_type = make_function_type(0x10, description);
            c1_data.translation_unit->root->Insert(symbol);
            if (c1_bss.browser_filename)
                c1_data.backend->Slot20(symbol);
            if (spec->traits & 0x80)
                symbol->declaration_flags |= 1;
        }
    }
}
extern "C" int __cdecl intrinsic_enabled(unsigned index) { __asm { int 3 } return 0; }
extern "C" void __cdecl initialize_intrinsic_symbol(CompilerIrSymbol *symbol, unsigned index)
{ __asm { int 3 } }
BYTE *RECON_THISCALL CompilerArena::AllocateBytes(unsigned bytes)
{ __asm { int 3 } return NULL; }

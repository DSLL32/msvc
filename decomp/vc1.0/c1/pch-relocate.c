/* Permanent-arena graph marking and reversible PCH pointer relocation. */
#include <stdlib.h>
#include "pch-relocate.h"
#include "source-input.h"
#include "pragma.h"
#include "debug-info.h"
#include "control-ir.h"
#include "lexer.h"
#include "keywords.h"

PchRelocationChain *pch_relocation_chains; /* original 0x5d5a8 */
int pch_relocation_capacity; /* original 0x5d5a4 */
unsigned pch_relocation_count; /* original 0x6b7d8 */
unsigned pch_reference; /* original 0x6ddbc */
unsigned char *pch_reference_base; /* original 0x6ddc0 */
unsigned short pch_reference_page; /* original 0x6ddc4 */

/* The original 105-cell root range contains unused cells as well as named
   roots. Null descriptors denote its untouched, permanently zero cells.
   Descriptor entries identify canonical storage; no root state is duplicated. */
extern FrontendSymbol *translation_sentinel, *translation_placeholder;
unsigned *pch_root_cells[105] = {
    0, /* root 0 */
    (unsigned *)&translation_sentinel, /* root 1 */
    (unsigned *)&translation_placeholder, /* root 2 */
    (unsigned *)&function_records, /* root 3 */
    (unsigned *)&last_function_records, /* root 4 */
    (unsigned *)&macro_buckets, /* root 5 */
    (unsigned *)&enumerator_context, /* root 6 */
    (unsigned *)&parameter_buckets, /* root 7 */
    (unsigned *)&permanent_next, /* root 8 */
    (unsigned *)&preprocessor_buckets, /* root 9 */
    (unsigned *)&type_shape_buckets, /* root 10 */
    (unsigned *)&lookup_scope, /* root 11 */
    (unsigned *)&packing_stack, /* root 12 */
    (unsigned *)&pch_saved_defines, /* root 13 */
    (unsigned *)&pch_saved_include_directories, /* root 14 */
    (unsigned *)&interned_name, /* root 15 */
    0, /* root 16 */
    0, /* root 17 */
    (unsigned *)&macro_text_begin, /* root 18 */
    (unsigned *)&macro_text_next, /* root 19 */
    (unsigned *)&name_alignment, /* root 20 */
    0, /* root 21 */
    (unsigned *)&name_cast, /* root 22 */
    (unsigned *)&name_defined, /* root 23 */
    (unsigned *)&name_dollar, /* root 24 */
    (unsigned *)&anonymous_symbol_name, /* root 25 */
    (unsigned *)&name_ellipsis, /* root 26 */
    (unsigned *)&name_formal, /* root 27 */
    (unsigned *)&name_static_guard, /* root 28 */
    (unsigned *)&name_main, /* root 29 */
    (unsigned *)&name_setjmp, /* root 30 */
    (unsigned *)&name_catch, /* root 31 */
    (unsigned *)&name_winmain, /* root 32 */
    (unsigned *)&name_dllmain, /* root 33 */
    (unsigned *)&name_unnamed, /* root 34 */
    (unsigned *)&pending_global_symbols, /* root 35 */
    (unsigned *)&function_return_type, /* root 36 */
    (unsigned *)&record_context_symbol, /* root 37 */
    0, /* root 38 */
    (unsigned *)&type_0041_q1000, /* root 39 */
    (unsigned *)&type_0064_q1000, /* root 40 */
    (unsigned *)&type_4041_q1000, /* root 41 */
    (unsigned *)&type_0108, /* root 42 */
    (unsigned *)&type_0064, /* root 43 */
    (unsigned *)&type_0044, /* root 44 */
    (unsigned *)&type_4044, /* root 45 */
    (unsigned *)&type_4041_q1, /* root 46 */
    (unsigned *)&type_0802, /* root 47 */
    (unsigned *)&type_0042, /* root 48 */
    (unsigned *)&type_4041, /* root 49 */
    (unsigned *)&type_4064_first, /* root 50 */
    (unsigned *)&type_4064_second, /* root 51 */
    (unsigned *)&type_0000, /* root 52 */
    (unsigned *)&type_0080, /* root 53 */
    (unsigned *)&type_implicit_int_function, /* root 54 */
    (unsigned *)&type_tagged_0080, /* root 55 */
    (unsigned *)&type_indirect_0080_2, /* root 56 */
    (unsigned *)&type_indirect_0080_1, /* root 57 */
    0, /* root 58 */
    0, /* root 59 */
    0, /* root 60 */
    0, /* root 61 */
    0, /* root 62 */
    0, /* root 63 */
    (unsigned *)&global_scope, /* root 64 */
    (unsigned *)&current_scope, /* root 65 */
    (unsigned *)&scope_directory, /* root 66 */
    (unsigned *)&function_shapes[0], /* root 67 */
    (unsigned *)&function_shapes[1], /* root 68 */
    (unsigned *)&function_shapes[2], /* root 69 */
    (unsigned *)&function_shapes[3], /* root 70 */
    (unsigned *)&function_shapes[4], /* root 71 */
    (unsigned *)&function_shapes[5], /* root 72 */
    (unsigned *)&function_shapes[6], /* root 73 */
    (unsigned *)&function_shapes[7], /* root 74 */
    (unsigned *)&function_shapes[8], /* root 75 */
    (unsigned *)&function_shapes[9], /* root 76 */
    (unsigned *)&function_shapes[10], /* root 77 */
    (unsigned *)&function_shapes[11], /* root 78 */
    (unsigned *)&function_shapes[12], /* root 79 */
    (unsigned *)&function_shapes[13], /* root 80 */
    (unsigned *)&function_shapes[14], /* root 81 */
    (unsigned *)&function_shapes[15], /* root 82 */
    (unsigned *)&function_shapes[16], /* root 83 */
    (unsigned *)&function_shapes[17], /* root 84 */
    (unsigned *)&function_shapes[18], /* root 85 */
    (unsigned *)&function_shapes[19], /* root 86 */
    (unsigned *)&function_shapes[20], /* root 87 */
    (unsigned *)&pragma_values, /* root 88 */
    (unsigned *)&void_parameters, /* root 89 */
    (unsigned *)&empty_parameters, /* root 90 */
    0, /* root 91 */
    (unsigned *)&default_bss_segment, /* root 92 */
    (unsigned *)&default_const_segment, /* root 93 */
    (unsigned *)&default_data_segment, /* root 94 */
    0, /* root 95 */
    (unsigned *)&default_code_segment, /* root 96 */
    0, /* root 97 */
    0, /* root 98 */
    0, /* root 99 */
    0, /* root 100 */
    0, /* root 101 */
    0, /* root 102 */
    0, /* root 103 */
    (unsigned *)&type_descriptor_buckets /* root 104 */
};

/* Original 0x446f0..0x44721. Mark pointer-bearing objects before replacing
   each permanent root with a page/offset reference and chaining page patches. */
void relocate_pch_state(void)
{
    unsigned i;
    mark_pch_graph();
    for (i = 0; i < 105; ++i) {
        pointer_to_pch_reference(pch_root_cells[i] ? (void *)*pch_root_cells[i] : 0);
        if (pch_root_cells[i]) *pch_root_cells[i] = pch_reference;
        pch_compiler_image.roots[i] = pch_reference;
    }
    initialize_pch_relocations();
    relocate_pch_pages();
    /* Gather the scalar suffix of the original serialized compiler state.
       Reserved cells have no executable writers; the format retains them. */
    pch_compiler_image.reserved_after_roots = 0;
    pch_compiler_image.relocation_count = pch_relocation_count;
    pch_compiler_image.available = permanent_available;
    pch_compiler_image.reserved_string_count = 0;
    pch_compiler_image.function_record_index = function_record_index;
    pch_compiler_image.reserved_record_count = 0;
    pch_compiler_image.active_plmf = active_plmf_pragma;
    pch_compiler_image.active_plmn = active_plmn_pragma;
    pch_compiler_image.next_symbol_index = next_symbol_index;
    pch_compiler_image.browser_symbol_limit = browser_symbol_limit;
    pch_compiler_image.reserved_one = 1;
    pch_compiler_image.reserved_zero = 0;
    pch_compiler_image.symbol_offset = pch_symbol_offset_adjustment;
    pch_compiler_image.next_debug_index = next_debug_type_index;
    pch_compiler_image.reserved_debug = 0;
    pch_compiler_image.pragma_record_number = pragma_record_number;
    pch_compiler_image.debug_type_bytes = debug_type_bytes;
    pch_compiler_image.reserved_debug_bytes = 0;
    for (i = 0; i < 800; ++i) pch_compiler_image.warning_levels[i] = warning_levels[i];
    pch_compiler_image.lowest_warning = lowest_warning_700;
    pch_compiler_image.highest_warning = highest_warning_700;
    pch_compiler_image.source_file_counter = source_file_counter;
    pch_compiler_image.scanner_flags = scanner_flags;
    pch_compiler_image.parser_flags = parser_flags;
}

/* These twelve native subsystem callbacks mark the permanent pointer graph. */
void (*pch_graph_markers[12])(void) = {
    mark_pch_scopes, mark_pch_types, mark_pch_shapes, mark_pch_macros,
    mark_pch_identifiers, mark_pch_function_records, mark_pch_scope_directory,
    mark_pch_parameters, mark_pch_pragma_values, mark_pch_defines,
    mark_pch_include_directories, mark_pch_packing
};

/* Original 0x43f50..0x43f65. */
void mark_pch_graph(void)
{
    unsigned index;
    for (index = 0; index < 12; ++index) pch_graph_markers[index]();
}

/* Original 0x43f70..0x43fcc. Mark global-scope links and each bucket's
   symbol chain, followed by pending global declarations. */
void mark_pch_scopes(void)
{
    SymbolScope *scope = global_scope;
    unsigned count, i;
    PCH_MARK(scope->next);
    PCH_MARK(scope->parent);
    PCH_MARK(scope->buckets);
    count = (unsigned)scope->bucket_mask + 1;
    for (i = 0; i < count; ++i) {
        PCH_MARK(scope->buckets[i]);
        mark_pch_symbol_chain(scope->buckets[i]);
    }
    mark_pch_symbol_chain(pending_global_symbols);
}

/* Original 0x44db0..0x44e35. Nonzero pointer cells are marked by byte
   offset in their containing permanent page, even when not word-aligned. */
int __stdcall mark_pch_pointer(void **slot)
{
    unsigned offset, page;
    if (!*slot) return 0;
    offset = (unsigned)slot - (unsigned)pch_reference_base;
    pch_reference = offset;
    if (offset > 0xeffc) pointer_to_pch_reference(slot);
    else pch_reference = (offset & 0xffff) | ((unsigned)pch_reference_page << 16);
    page = pch_reference >> 16;
    if (page == 0xffff) return 0;
    offset = pch_reference & 0xffff;
    permanent_pages[page].relocations->bits[offset >> 3] |= 1 << (offset & 7);
    return 1;
}

/* Original 0x445f0..0x44698. Cache the containing page while converting
   a pointer to a sixteen-bit page number and sixteen-bit byte offset. */
void __stdcall pointer_to_pch_reference(void *pointer)
{
    unsigned offset;
    if (!pointer) {
        pch_reference = 0;
        return;
    }
    pch_reference_page = 0;
    while (pch_reference_page < (unsigned short)permanent_page_count) {
        offset = (unsigned)pointer - (unsigned)permanent_pages[pch_reference_page].start;
        pch_reference = offset;
        if (offset < 0xeffc) {
            pch_reference = (offset & 0xffff) | ((unsigned)pch_reference_page << 16);
            pch_reference_base = permanent_pages[pch_reference_page].start;
            return;
        }
        ++pch_reference_page;
    }
    pch_reference = 0xffffffff;
    pch_reference_page = 0xffff;
    pch_reference_base = 0;
}

/* Original 0x43fd0..0x4407a; switch table 0x4407c..0x440ac. Each
   symbol variant contributes the pointer fields belonging to that variant. */
void __stdcall mark_pch_symbol_chain(FrontendSymbol *symbol)
{
    while (symbol) {
        PCH_MARK(symbol->next);
        PCH_MARK(symbol->name);
        switch (symbol->kind & 0x1f) {
        case 2: case 3: case 5: case 6: case 7: case 8: case 9:
        case 11: case 12:
            PCH_MARK(symbol->type);
            break;
        case 4:
            PCH_MARK(symbol->type);
            mark_pch_tag(symbol);
            break;
        case 10:
            PCH_MARK(symbol->type);
            PCH_MARK(symbol->detail.parameter_next);
            break;
        case 13:
            PCH_MARK(symbol->type);
            PCH_MARK(symbol->detail.text);
            PCH_MARK(symbol->associated.members);
            break;
        }
        symbol = symbol->next;
    }
}

/* Original 0x440b0..0x440ec. Enum tags own a generic value table;
   aggregate tags own a linear member declaration list. */
void __stdcall mark_pch_tag(FrontendSymbol *tag)
{
    if (!(tag->type->flags & 0x200)) {
        PCH_MARK(tag->associated.members);
        mark_pch_symbol_chain(tag->associated.members);
    } else {
        mark_pch_hash_table(tag->associated.enumerators, 0, 0);
        PCH_MARK(tag->associated.enumerators);
    }
}

/* Original 0x44520..0x44576. */
void __stdcall mark_pch_hash_table(HashTable *table,
    void (__stdcall *mark_key)(void *), void (__stdcall *mark_value)(void *))
{
    unsigned i;
    if (!table) return;
    PCH_MARK(table->buckets);
    for (i = 0; i < table->bucket_count; ++i) {
        PCH_MARK(table->buckets[i]);
        mark_pch_hash_entries(table->buckets[i], mark_key, mark_value);
    }
}

/* Original 0x44580..0x445cd. */
void __stdcall mark_pch_hash_entries(HashEntry *entry,
    void (__stdcall *mark_key)(void *), void (__stdcall *mark_value)(void *))
{
    while (entry) {
        PCH_MARK(entry->key);
        PCH_MARK(entry->value);
        PCH_MARK(entry->next);
        if (mark_key) mark_key(entry->key);
        if (mark_value) mark_value(entry->value);
        entry = entry->next;
    }
}

/* Original 0x440f0..0x4411b. */
void mark_pch_types(void)
{
    unsigned i;
    for (i = 0; i < 1024; ++i) {
        PCH_MARK(type_descriptor_buckets[i]);
        mark_pch_type_entries(type_descriptor_buckets[i]);
    }
}

/* Original 0x44120..0x44142. */
void __stdcall mark_pch_type_entries(TypeEntry *entry)
{
    while (entry) {
        PCH_MARK(entry->next);
        mark_pch_type(&entry->type);
        entry = entry->next;
    }
}

/* Original 0x44150..0x4416b. The third type word is pointer-valued
   only for the corresponding type variants; non-arena values remain literal. */
void __stdcall mark_pch_type(CType *type)
{
    PCH_MARK(type->related);
    PCH_MARK(type->field08);
}

/* Original 0x44170..0x4419b. */
void mark_pch_shapes(void)
{
    unsigned i;
    for (i = 0; i < 1024; ++i) {
        PCH_MARK(type_shape_buckets[i]);
        mark_pch_shape_entries(type_shape_buckets[i]);
    }
}

/* Original 0x441a0..0x441c2. */
void __stdcall mark_pch_shape_entries(TypeShapeEntry *entry)
{
    while (entry) {
        PCH_MARK(entry->next);
        mark_pch_shape(&entry->shape);
        entry = entry->next;
    }
}

/* Original 0x441d0..0x4422d. Pointer-bearing payloads depend on the
   shape kind and on the pointer model for pointer shapes. */
void __stdcall mark_pch_shape(TypeShape *shape)
{
    PCH_MARK(shape->related);
    switch (shape->flags & 7) {
    case 0:
    case 4:
        PCH_MARK(shape->field08);
        break;
    case 1:
        if ((shape->attributes & 7) >= 5) PCH_MARK(shape->field08);
        break;
    }
}

/* Original 0x443e0..0x4440b. */
void mark_pch_macros(void)
{
    unsigned i;
    for (i = 0; i < 512; ++i) {
        PCH_MARK(macro_buckets[i]);
        mark_pch_macro_entries(macro_buckets[i]);
    }
}

/* Original 0x44410..0x4443b. */
void __stdcall mark_pch_macro_entries(MacroDefinition *entry)
{
    while (entry) {
        PCH_MARK(entry->next);
        PCH_MARK(entry->name);
        PCH_MARK(entry->replacement);
        entry = entry->next;
    }
}

/* Original 0x44440..0x4446b. */
void mark_pch_identifiers(void)
{
    unsigned i;
    for (i = 0; i < 2048; ++i) {
        PCH_MARK(preprocessor_buckets[i]);
        mark_pch_identifier_entries(preprocessor_buckets[i]);
    }
}

/* Original 0x44470..0x44489. Interned identifier bytes follow their
   header inline; only the collision-chain link needs relocation. */
void __stdcall mark_pch_identifier_entries(PreprocessorSymbol *entry)
{
    while (entry) {
        PCH_MARK(entry->next);
        entry = entry->next;
    }
}

/* Original 0x3a650..0x3a690. The current block uses the inclusive
   directory index; older blocks contain twenty complete record slots. */
void mark_pch_function_records(void)
{
    FunctionRecordBlock *block = function_records;
    FunctionRecord *record;
    unsigned remaining;
    while (block) {
        PCH_MARK(block->next);
        remaining = (block == last_function_records ? function_record_index : 19) + 1;
        record = block->records;
        do {
            PCH_MARK(record->symbol);
            ++record;
        } while (--remaining);
        block = block->next;
    }
}

/* Original 0x43f10..0x43f41. */
void mark_pch_scope_directory(void)
{
    mark_pch_pointer_array((void **)scope_directory, 128);
    mark_pch_symbol_chain(translation_sentinel);
    mark_pch_symbol_chain(translation_placeholder);
}

/* Original 0x44490..0x444b3. */
void __stdcall mark_pch_pointer_array(void **slots, int count)
{
    if (!slots) return;
    while (count > 0) {
        mark_pch_pointer(slots++);
        --count;
    }
}

/* Original 0x44230..0x44283. */
void mark_pch_parameters(void)
{
    unsigned i;
    for (i = 0; i < 512; ++i) {
        PCH_MARK(parameter_buckets[i]);
        mark_pch_parameter_entries(parameter_buckets[i]);
    }
    if (void_parameters) mark_pch_parameter_info(void_parameters);
    if (empty_parameters) mark_pch_parameter_info(empty_parameters);
}

/* Original 0x44290..0x442b2. */
void __stdcall mark_pch_parameter_entries(ParameterEntry *entry)
{
    while (entry) {
        PCH_MARK(entry->next);
        mark_pch_parameter_info(&entry->info);
        entry = entry->next;
    }
}

/* Original 0x442f0..0x44314. */
void __stdcall mark_pch_parameter_info(ParameterInfo *parameters)
{
    int i;
    for (i = 0; i < (int)parameters->count; ++i) PCH_MARK(parameters->types[i]);
}

/* Original 0x442c0..0x442e7. */
void mark_pch_pragma_values(void)
{
    PragmaValue *value = pragma_values;
    while (value) {
        PCH_MARK(value->next);
        PCH_MARK(value->value);
        value = value->next;
    }
}

/* Original 0x445d0..0x445de. */
void mark_pch_defines(void)
{
    mark_pch_vector(pch_saved_defines, 0);
}

/* Original 0x444c0..0x44514. */
void __stdcall mark_pch_vector(Vector *vector, void (__stdcall *mark_item)(void *))
{
    int i;
    if (!vector) return;
    if (vector->capacity > 0) PCH_MARK(vector->items);
    for (i = 0; i < vector->count; ++i) {
        PCH_MARK(vector->items[i]);
        if (mark_item) mark_item(vector->items[i]);
    }
}

/* Original 0x445e0..0x445ee. */
void mark_pch_include_directories(void)
{
    mark_pch_vector(pch_saved_include_directories, 0);
}

/* Original 0x1e830..0x1e86c. Packing labels share the third entry word;
   only nonzero label cells need marking. The free list is not serialized. */
void mark_pch_packing(void)
{
    PackingEntry *entry;
    PCH_MARK(packing_stack->active);
    for (entry = top_packing(packing_stack); entry; entry = entry->next) {
        PCH_MARK(entry->next);
        if (entry->field08) PCH_MARK(entry->field08);
    }
}

/* Original 0x44730..0x447a6. Begin with one backward patch chain for
   each destination page, leaving room for an equal number of overflow rows. */
void initialize_pch_relocations(void)
{
    unsigned i;
    pch_relocation_capacity = permanent_page_count * 2;
    pch_relocation_chains = malloc(permanent_page_count * 16);
    if (!pch_relocation_chains) fatal_error(60);
    for (i = 0; i < (unsigned)pch_relocation_capacity; ++i) {
        pch_relocation_chains[i].head = 0;
        pch_relocation_chains[i].page = 0;
        pch_relocation_chains[i].reserved = 0;
    }
    pch_relocation_count = permanent_page_count;
    for (i = 0; i < pch_relocation_count; ++i) {
        pch_relocation_chains[i].head = 0;
        pch_relocation_chains[i].page = (unsigned short)i;
    }
}

/* Original 0x447b0..0x4499c. Marked cells are visited by ascending byte
   offset. Their target offsets and backward patch distances share one word. */
void relocate_pch_pages(void)
{
    int source_page;
    unsigned byte, bit, offset, target_page, location, distance, packed, i;
    unsigned *slot;
    BitSet *marks;
    PchRelocationChain *chain;
    for (source_page = 0; source_page < permanent_page_count; ++source_page) {
        marks = permanent_pages[source_page].relocations;
        for (byte = 0; byte <= (marks->bit_count >> 3); ++byte) {
            for (bit = 0; bit < 8; ++bit) {
                if (!(marks->bits[byte] & (1 << bit))) continue;
                slot = (unsigned *)(permanent_pages[source_page].start + byte * 8 + bit);
                offset = *slot - (unsigned)pch_reference_base;
                pch_reference = offset;
                if (offset <= 0xeffc)
                    pch_reference = (offset & 0xffff) | ((unsigned)pch_reference_page << 16);
                else pointer_to_pch_reference((void *)*slot);
                target_page = pch_reference >> 16;
                if (target_page == 0xffff) continue;
                chain = &pch_relocation_chains[target_page];
                location = ((unsigned)source_page << 16) |
                           (((unsigned)slot - (unsigned)permanent_pages[source_page].start) & 0xffff);
                packed = pch_reference & 0xffff;
                if (chain->head) {
                    distance = location - chain->head;
                    if ((int)distance >= 0x10000) {
                        if ((int)pch_relocation_count >= pch_relocation_capacity) {
                            pch_relocation_chains = realloc(pch_relocation_chains,
                                                            pch_relocation_capacity * 8 + 256);
                            if (!pch_relocation_chains) fatal_error(60);
                            for (i = pch_relocation_capacity; i < (unsigned)pch_relocation_capacity + 32; ++i) {
                                pch_relocation_chains[i].head = 0;
                                pch_relocation_chains[i].page = 0;
                                pch_relocation_chains[i].reserved = 0;
                            }
                            pch_relocation_capacity += 32;
                            chain = &pch_relocation_chains[target_page];
                        }
                        pch_relocation_chains[pch_relocation_count++] = *chain;
                    } else packed |= (distance & 0xffff) << 16;
                }
                *slot = packed;
                chain->head = location;
            }
        }
    }
}

/* Original 0x44c40..0x44cc7. The relocation chains precede full arena
   pages; the final page is trimmed at the permanent allocator's cursor. */
void write_pch_pages(void)
{
    int i = 0;
    fwrite(pch_relocation_chains, 1, pch_relocation_count * 8, pch_file);
    while (i < permanent_page_count - 1) {
        fwrite(permanent_pages[i].start, 1, 0xeffc, pch_file);
        ++i;
    }
    fwrite(permanent_pages[i].start, 1, 0xeffc - permanent_available, pch_file);
}

/* Original 0x44a00..0x44af2. Restore roots and walk each destination
   page's backward patch chains without needing a separate pointer per cell. */
void restore_pch_pointers(void)
{
    unsigned i, location, encoded, distance;
    unsigned *slot;
    unsigned char *destination;
    for (i = 0; i < 105; ++i) {
        if (pch_root_cells[i] && *pch_root_cells[i]) {
            encoded = *pch_root_cells[i];
            *pch_root_cells[i] = (unsigned)permanent_pages[encoded >> 16].start + (encoded & 0xffff);
        }
    }
    for (i = 0; i < pch_relocation_count; ++i) {
        location = pch_relocation_chains[i].head;
        destination = permanent_pages[pch_relocation_chains[i].page].start;
        if (!location) continue;
        for (;;) {
            slot = (unsigned *)(permanent_pages[location >> 16].start + (location & 0xffff));
            encoded = *slot;
            *slot = (unsigned)destination + (encoded & 0xffff);
            distance = encoded >> 16;
            if (!distance) break;
            location -= distance;
        }
    }
    free(pch_relocation_chains);
}

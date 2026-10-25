/* Permanent-arena PCH relocation. Serialization binds existing globals;
   it does not introduce a second live frontend state. */
#include "initialization.h"
#include "pch-relocation.h"
#include "function-finalization.h"
#include "function-definitions.h"
#include "symbol-output.h"
#include "cpp-context.h"
#include "operators.h"
#include "preprocessor.h"
#include "parser-state.h"
#include "debug-info.h"
#include "debug-types.h"
#include "global-ir.h"
#include "pragma.h"
#include <recon.h>

extern unsigned char *permanent_next;
extern unsigned permanent_available;
extern DeclarationParameterInfo *void_parameters;
extern InlineVector *generated_member_queue, *classes_pending_table_output;
extern CppSymbol *purecall_symbol, *alternate_purecall_symbol;
extern HashTable *constructor_closures, *virtual_adjustment_thunks;
extern HashTable *method_aliases, *member_access_declarations, *member_context_links;
extern SegmentSymbol *default_constant_segment, *default_data_segment, *default_bss_segment;
extern CppSymbol *initializer_segment_symbol;
extern char *initializer_segment_name;
extern TypeShape *function_shapes[21];
extern Vector *pch_saved_defines, *pch_saved_include_directories;
extern unsigned pch_operator_count;
extern int active_plmn_pragma, active_plmf_pragma;
extern int member_pointer_full_generality, member_pointer_generality_fixed;
extern int member_pointer_requested_model, anonymous_identifier_count;

/* Original0x9f1c0..0x9f414. Each cell names its sole current owner. The
   eleven slots with no individual xref, the pending binding/typedef maps and
   two far-function fallback pointers have no reconstructed live consumer yet.
   Their canonical cells remain in the image view: preserve actual values on
   load, without claiming they are unused or forcing them to zero. */
#define ROOT(object_) ((unsigned *)&(object_))
unsigned * const pch_root_cells[149] = {
    ROOT(generated_member_queue), /* 9f1c0 */
    ROOT(translation_sentinel), /* 9f1c4 */
    ROOT(translation_placeholder), /* 9f1c8 */
    ROOT(function_directory), /* 9f1cc */
    ROOT(function_directory_tail), /* 9f1d0 */
    ROOT(macro_buckets), /* 9f1d4 */
    ROOT(enumerator_context), /* 9f1d8 */
    ROOT(parameter_buckets), /* 9f1dc */
    ROOT(permanent_next), /* 9f1e0 */
    ROOT(preprocessor_buckets), /* 9f1e4 */
    ROOT(type_shape_buckets), /* 9f1e8 */
    ROOT(lookup_scope), /* 9f1ec */
    ROOT(translation_symbols), /* 9f1f0 */
    ROOT(pch_compiler_image.roots[13]), /* 9f1f4; no individual xref */
    ROOT(packing_stack), /* 9f1f8 */
    ROOT(pch_saved_defines), /* 9f1fc */
    ROOT(pch_saved_include_directories), /* 9f200 */
    ROOT(interned_name), /* 9f204 */
    ROOT(name_call_operator), /* 9f208 */
    ROOT(name_arrow_operator), /* 9f20c */
    ROOT(macro_text_begin), /* 9f210 */
    ROOT(macro_text_next), /* 9f214 */
    ROOT(name_alignment), /* 9f218 */
    ROOT(name_assignment), /* 9f21c */
    ROOT(name_cast), /* 9f220 */
    ROOT(name_defined), /* 9f224 */
    ROOT(name_dollar), /* 9f228 */
    ROOT(anonymous_symbol_name), /* 9f22c */
    ROOT(name_ellipsis), /* 9f230 */
    ROOT(name_formal), /* 9f234 */
    ROOT(name_static_guard), /* 9f238 */
    ROOT(name_main), /* 9f23c */
    ROOT(name_setjmp), /* 9f240 */
    ROOT(name_catch), /* 9f244 */
    ROOT(name_winmain), /* 9f248 */
    ROOT(name_dllmain), /* 9f24c */
    ROOT(name_unnamed), /* 9f250 */
    ROOT(segment_symbols), /* 9f254 */
    ROOT(current_return_type), /* 9f258 */
    ROOT(record_context_symbol), /* 9f25c */
    ROOT(static_member_name_bindings), /* 9f260 */
    ROOT(type_0041_q1000), /* 9f264 */
    ROOT(type_0064_q1000), /* 9f268 */
    ROOT(type_4041_q1000), /* 9f26c */
    ROOT(type_0108), /* 9f270 */
    ROOT(type_0064), /* 9f274 */
    ROOT(type_0044), /* 9f278 */
    ROOT(type_4044), /* 9f27c */
    ROOT(type_4041_q1), /* 9f280 */
    ROOT(type_0802), /* 9f284 */
    ROOT(type_0042), /* 9f288 */
    ROOT(type_4041), /* 9f28c */
    ROOT(type_4064_first), /* 9f290 */
    ROOT(type_4064_second), /* 9f294 */
    ROOT(type_0000), /* 9f298 */
    ROOT(type_0080), /* 9f29c */
    ROOT(type_implicit_int_function), /* 9f2a0 */
    ROOT(type_tagged_0080), /* 9f2a4 */
    ROOT(type_indirect_0080_2), /* 9f2a8 */
    ROOT(type_indirect_0080_1), /* 9f2ac */
    ROOT(purecall_symbol), /* 9f2b0 */
    ROOT(alternate_purecall_symbol), /* 9f2b4 */
    ROOT(pch_compiler_image.roots[62]), /* 9f2b8; no individual xref */
    ROOT(pch_compiler_image.roots[63]), /* 9f2bc; no individual xref */
    ROOT(pch_compiler_image.roots[64]), /* 9f2c0; no individual xref */
    ROOT(pch_compiler_image.roots[65]), /* 9f2c4; no individual xref */
    ROOT(global_scope), /* 9f2c8 */
    ROOT(current_scope), /* 9f2cc */
    ROOT(scope_directory), /* 9f2d0 */
    ROOT(function_shapes[0]), /* 9f2d4 */
    ROOT(function_shapes[1]), /* 9f2d8 */
    ROOT(function_shapes[2]), /* 9f2dc */
    ROOT(function_shapes[3]), /* 9f2e0 */
    ROOT(function_shapes[4]), /* 9f2e4 */
    ROOT(function_shapes[5]), /* 9f2e8 */
    ROOT(function_shapes[6]), /* 9f2ec */
    ROOT(function_shapes[7]), /* 9f2f0 */
    ROOT(function_shapes[8]), /* 9f2f4 */
    ROOT(function_shapes[9]), /* 9f2f8 */
    ROOT(function_shapes[10]), /* 9f2fc */
    ROOT(function_shapes[11]), /* 9f300 */
    ROOT(function_shapes[12]), /* 9f304 */
    ROOT(function_shapes[13]), /* 9f308 */
    ROOT(function_shapes[14]), /* 9f30c */
    ROOT(function_shapes[15]), /* 9f310 */
    ROOT(function_shapes[16]), /* 9f314 */
    ROOT(function_shapes[17]), /* 9f318 */
    ROOT(function_shapes[18]), /* 9f31c */
    ROOT(function_shapes[19]), /* 9f320 */
    ROOT(function_shapes[20]), /* 9f324 */
    ROOT(pragma_values), /* 9f328 */
    ROOT(cpp_context_stack), /* 9f32c */
    ROOT(virtual_adjustment_thunks), /* 9f330 */
    ROOT(method_aliases), /* 9f334 */
    ROOT(declaration_thunks), /* 9f338 */
    ROOT(deferred_type_tables), /* 9f33c */
    ROOT(declaration_table), /* 9f340 */
    ROOT(pch_compiler_image.roots[97]), /* 9f344; pending global_symbol_bindings */
    ROOT(pch_compiler_image.roots[98]), /* 9f348; pending typedef_aliases */
    ROOT(member_context_links), /* 9f34c */
    ROOT(initializer_segment_name), /* 9f350 */
    ROOT(name_ctor), /* 9f354 */
    ROOT(name_delete_dtor), /* 9f358 */
    ROOT(name_delete), /* 9f35c */
    ROOT(name_dtor), /* 9f360 */
    ROOT(name_flags), /* 9f364 */
    ROOT(name_init_vbases), /* 9f368 */
    ROOT(name_static_destructor), /* 9f36c */
    ROOT(name_new), /* 9f370 */
    ROOT(name_return_udt), /* 9f374 */
    ROOT(name_that), /* 9f378 */
    ROOT(name_this), /* 9f37c */
    ROOT(name_vbase_dtor), /* 9f380 */
    ROOT(name_vbptr), /* 9f384 */
    ROOT(name_vbtable), /* 9f388 */
    ROOT(name_vfptr), /* 9f38c */
    ROOT(name_vdispmap), /* 9f390 */
    ROOT(pch_compiler_image.roots[117]), /* 9f394; no individual xref */
    ROOT(pch_compiler_image.roots[118]), /* 9f398; no individual xref */
    ROOT(pch_compiler_image.roots[119]), /* 9f39c; no individual xref */
    ROOT(name_vector_delete_dtor), /* 9f3a0 */
    ROOT(name_vector_ctor), /* 9f3a4 */
    ROOT(name_vector_ctor_vb), /* 9f3a8 */
    ROOT(name_vector_dtor), /* 9f3ac */
    ROOT(name_vftable), /* 9f3b0 */
    ROOT(name_new_target), /* 9f3b4 */
    ROOT(name_atexit), /* 9f3b8 */
    ROOT(name_tlregdtor), /* 9f3bc */
    ROOT(void_parameters), /* 9f3c0 */
    ROOT(initializer_segment_symbol), /* 9f3c4 */
    ROOT(operator_zero), /* 9f3c8 */
    ROOT(empty_inline_vector), /* 9f3cc */
    ROOT(classes_pending_table_output), /* 9f3d0 */
    ROOT(static_member_aliases), /* 9f3d4 */
    ROOT(declaration_parameter_buckets), /* 9f3d8 */
    ROOT(pch_compiler_image.roots[135]), /* 9f3dc; pending default_far_data_segment */
    ROOT(default_bss_segment), /* 9f3e0 */
    ROOT(default_constant_segment), /* 9f3e4 */
    ROOT(default_data_segment), /* 9f3e8 */
    ROOT(pch_compiler_image.roots[139]), /* 9f3ec; pending default_far_code_segment */
    ROOT(default_code_segment), /* 9f3f0 */
    ROOT(name_default_ctor_closure), /* 9f3f4 */
    ROOT(constructor_closures), /* 9f3f8 */
    ROOT(member_access_declarations), /* 9f3fc */
    ROOT(name_most_derived), /* 9f400 */
    ROOT(pch_compiler_image.roots[145]), /* 9f404; no individual xref */
    ROOT(pch_compiler_image.roots[146]), /* 9f408; no individual xref */
    ROOT(pch_compiler_image.roots[147]), /* 9f40c; no individual xref */
    ROOT(type_descriptor_buckets), /* 9f410 */
};
#undef ROOT

unsigned pch_encoded_pointer; /* original0xa22fc: offset WORD, then page WORD */
unsigned pch_relocation_count; /* original0x9f418 */
void mark_pch_relocations(void);
void __stdcall encode_pch_pointer(void *pointer);
void encode_pch_operators(void);
void initialize_pch_relocations(void);
void encode_pch_pages(void);

/* Original0x6a830..0x6a866. Mark pointed-to fields before replacing the
   canonical roots with page/offset words, then build the on-disk page chains. */
void relocate_pch_state(void)
{
    unsigned i;
    mark_pch_relocations();
    for (i = 0; i < 149; ++i) {
        encode_pch_pointer((void *)*pch_root_cells[i]);
        *pch_root_cells[i] = pch_encoded_pointer;
        pch_compiler_image.roots[i] = pch_encoded_pointer;
    }
    encode_pch_operators();
    initialize_pch_relocations();
    encode_pch_pages();
    /* Marshal scalar state from its existing operational owners. Reserved
       cells retain their actual image-view state across save/restore. */
    pch_compiler_image.relocation_count = pch_relocation_count;
    pch_compiler_image.permanent_available = permanent_available;
    pch_compiler_image.operator_count = pch_operator_count;
    pch_compiler_image.function_directory_index = function_directory_index;
    pch_compiler_image.plmn_active = active_plmn_pragma;
    pch_compiler_image.plmf_active = active_plmf_pragma;
    pch_compiler_image.member_full_generality = member_pointer_full_generality;
    pch_compiler_image.member_generality_fixed = member_pointer_generality_fixed;
    pch_compiler_image.member_requested_model = member_pointer_requested_model;
    pch_compiler_image.next_symbol_index = next_symbol_index;
    pch_compiler_image.browser_symbol_limit = browser_symbol_limit;
    pch_compiler_image.anonymous_identifier_count = anonymous_identifier_count;
    pch_compiler_image.hidden_frame_bytes = hidden_frame_bytes;
    pch_compiler_image.next_debug_index = next_debug_type_index;
    pch_compiler_image.pragma_record_number = pragma_record_number;
    pch_compiler_image.debug_stream_offset = debug_stream_offset;
    pch_compiler_image.vtordisp = pragma_vtordisp;
    for (i = 0; i < 800; ++i) pch_compiler_image.warning_levels[i] = warning_levels[i];
    pch_compiler_image.lowest_warning = lowest_warning_700;
    pch_compiler_image.highest_warning = highest_warning_700;
    pch_compiler_image.source_file_counter = source_file_counter;
    pch_compiler_image.token_flags = token_allocation_flags;
    pch_compiler_image.parser_flags = cpp_parser_flags;
}

/* Original0xa2278..0xa22e8: the encountered28 zero-argument cdecl visitors.
   Table bounds and each RET0 contract come from the actual C1XX dispatcher
   and callback bodies; discovery of a visitor does not credit its body. */
void mark_pch_scopes(void);
void mark_pch_types(void);
void mark_pch_shapes(void);
void mark_pch_macros(void);
void mark_pch_identifiers(void);
void mark_pch_function_records(void);
void mark_pch_scope_directory(void);
void mark_pch_parameters(void);
void mark_pch_pragma_values(void);
void mark_pch_defines(void);
void mark_pch_include_directories(void);
void mark_pch_packing(void);
void mark_pch_translation_symbols(void);
void mark_pch_class_contexts(void);
void mark_pch_pending_tables(void);
void mark_pch_generated_members(void);
void mark_pch_declaration_table(void);
void mark_pch_virtual_thunks(void);
void mark_pch_declaration_thunks(void);
void mark_pch_deferred_types(void);
void mark_pch_member_contexts(void);
void mark_pch_typedef_aliases(void);
void mark_pch_static_member_aliases(void);
void mark_pch_static_member_names(void);
void mark_pch_global_bindings(void);
void mark_pch_method_aliases(void);
void mark_pch_constructor_closures(void);
void mark_pch_access_declarations(void);

void (* const pch_mark_callbacks[28])(void) = {
    mark_pch_scopes,
    mark_pch_types,
    mark_pch_shapes,
    mark_pch_macros,
    mark_pch_identifiers,
    mark_pch_function_records,
    mark_pch_scope_directory,
    mark_pch_parameters,
    mark_pch_pragma_values,
    mark_pch_defines,
    mark_pch_include_directories,
    mark_pch_packing,
    mark_pch_translation_symbols,
    mark_pch_class_contexts,
    mark_pch_pending_tables,
    mark_pch_generated_members,
    mark_pch_declaration_table,
    mark_pch_virtual_thunks,
    mark_pch_declaration_thunks,
    mark_pch_deferred_types,
    mark_pch_member_contexts,
    mark_pch_typedef_aliases,
    mark_pch_static_member_aliases,
    mark_pch_static_member_names,
    mark_pch_global_bindings,
    mark_pch_method_aliases,
    mark_pch_constructor_closures,
    mark_pch_access_declarations,
};

/* Original0x69d50..0x69d65. Each owning subsystem marks its live pointer
   fields before root and arena-word encoding alters their representations. */
void mark_pch_relocations(void)
{
    void (* const *mark)(void);
    for (mark = pch_mark_callbacks; mark < pch_mark_callbacks + 28; ++mark)
        (*mark)();
}

int __stdcall mark_pch_pointer(void **slot);
void __stdcall mark_pch_symbol_chain(CppSymbol *symbol);

/* Original0x69d70..0x69dcc. Mark the scope record's links and every bucket
   head before visiting each declaration chain and the segment declarations. */
void mark_pch_scopes(void)
{
    SymbolScope *scope = global_scope;
    unsigned count, i;
    mark_pch_pointer((void **)&scope->next);
    mark_pch_pointer((void **)&scope->parent);
    mark_pch_pointer((void **)&scope->buckets);
    count = (unsigned)scope->bucket_mask + 1;
    for (i = 0; i < count; ++i) {
        mark_pch_pointer((void **)&scope->buckets[i]);
        mark_pch_symbol_chain(scope->buckets[i]);
    }
    mark_pch_symbol_chain(segment_symbols);
}

void __stdcall mark_pch_tag(CppSymbol *tag);

/* Original0x69dd0..0x69e83;17-way switch0x69e84..0x69ec8. Visit only
   the pointer fields belonging to the symbol's actual compact record kind. */
void __stdcall mark_pch_symbol_chain(CppSymbol *symbol)
{
    while (symbol) {
        mark_pch_pointer((void **)&symbol->next);
        mark_pch_pointer((void **)&symbol->name);
        switch (symbol->kind & 31) {
        case 2: case 3: case 5: case 6: case 7: case 8:
        case 11: case 12: case 18:
            mark_pch_pointer((void **)&symbol->type);
            break;
        case 4:
            mark_pch_pointer((void **)&symbol->type);
            mark_pch_tag(symbol);
            break;
        case 9:
            mark_pch_pointer((void **)&symbol->type);
            if (symbol->storage[1] & 0x20)
                mark_pch_pointer((void **)&symbol->auxiliary);
            break;
        case 10:
            mark_pch_pointer((void **)&symbol->type);
            mark_pch_pointer((void **)&symbol->detail.parameter_next);
            break;
        case 13:
            mark_pch_pointer((void **)&symbol->type);
            mark_pch_pointer((void **)&symbol->detail.segment_class);
            /* Segments use this word for their associated initializer. */
            mark_pch_pointer((void **)&symbol->field28);
            break;
        case 16: case 17:
            mark_pch_pointer((void **)&((OverloadSet *)symbol)->functions);
            mark_pch_symbol_chain(((OverloadSet *)symbol)->functions);
            break;
        }
        symbol = symbol->next;
    }
}

typedef void (__stdcall *PchVisit)(void *item);
typedef struct PchMemberIndex {
    ClassMemberBucket *buckets;
    unsigned mask;
} PchMemberIndex;
void __stdcall mark_pch_vector(Vector *vector, PchVisit visit);
void __stdcall mark_pch_inline_vector(InlineVector *vector, PchVisit visit);
void __stdcall mark_pch_hash_table(HashTable *table, PchVisit key, PchVisit value);
void __stdcall mark_pch_virtual_table(VirtualTable *table);
void __stdcall mark_pch_virtual_group(VirtualTableGroup *group);
void __stdcall mark_pch_member_index(PchMemberIndex *index);

/* Original0x69ed0..0x69fba. Class information owns its table/conversion
   containers; enum tags instead own a generic enumerator hash table. */
void __stdcall mark_pch_tag(CppSymbol *tag)
{
    ClassInfo *info;
    mark_pch_pointer((void **)&tag->detail.class_info);
    info = tag->detail.class_info;
    mark_pch_pointer((void **)&info->virtual_tables);
    mark_pch_pointer((void **)&info->virtual_base_tables);
    mark_pch_pointer((void **)&info->virtual_table_groups);
    mark_pch_pointer((void **)&info->virtual_dispatch_pointer);
    mark_pch_pointer((void **)&info->conversion_types);
    mark_pch_pointer((void **)&info->ambiguous_conversion_types);
    mark_pch_vector(info->virtual_tables, (PchVisit)mark_pch_virtual_table);
    mark_pch_vector(info->virtual_base_tables, (PchVisit)mark_pch_virtual_table);
    mark_pch_vector(info->virtual_table_groups, (PchVisit)mark_pch_virtual_group);
    mark_pch_inline_vector(info->conversion_types, 0);
    mark_pch_inline_vector(info->ambiguous_conversion_types, 0);
    if (!(tag->type->flags & 0x200)) {
        mark_pch_pointer((void **)&tag->auxiliary.scope_members);
        mark_pch_symbol_chain(tag->auxiliary.scope_members);
        mark_pch_member_index((PchMemberIndex *)&info->member_index);
        mark_pch_pointer((void **)&info->member_index);
    } else {
        mark_pch_hash_table(tag->auxiliary.enumerators, 0, 0);
        mark_pch_pointer((void **)&tag->auxiliary.enumerators);
    }
}

/* Original0x6a540..0x6a594. Mark the vector's separate item allocation
   only when it has capacity, then visit each current element in order. */
void __stdcall mark_pch_vector(Vector *vector, PchVisit visit)
{
    int i;
    if (!vector) return;
    if (vector->capacity > 0) mark_pch_pointer((void **)&vector->items);
    for (i = 0; i < vector->count; ++i) {
        mark_pch_pointer(&vector->items[i]);
        if (visit) (*visit)(vector->items[i]);
    }
}

void __stdcall mark_pch_virtual_call(VirtualCallEntry *entry);
void __stdcall mark_pch_virtual_base(VirtualBaseEntry *entry);

/* Original0x7c250..0x7c2f3. Table metadata and slot arrays retain their
   owning fields; dispatch and virtual-base slots contain different pointers. */
void __stdcall mark_pch_virtual_table(VirtualTable *table)
{
    int i;
    VirtualEntry *entry;
    mark_pch_pointer((void **)&table->symbol);
    mark_pch_pointer((void **)&table->entries);
    mark_pch_pointer((void **)&table->pointer);
    mark_pch_pointer((void **)&table->inheritance_path);
    mark_pch_pointer((void **)&table->source_table);
    mark_pch_pointer((void **)&table->unshared_owner);
    mark_pch_pointer((void **)&table->shared_owners);
    for (i = 0; i < table->entry_count; ++i) {
        entry = &table->entries[i];
        if (table->kind == 0 || table->kind == 2)
            mark_pch_virtual_call(&entry->dispatch);
        else mark_pch_virtual_base(&entry->base);
    }
    mark_pch_vector(table->shared_owners, 0);
}

/* Original0x7c300..0x7c30d. Dispatch entries mark the function slot. */
void __stdcall mark_pch_virtual_call(VirtualCallEntry *entry)
{
    mark_pch_pointer((void **)&entry->function);
}

/* Original0x6a5a0..0x6a5db. Inline vectors own their item slots directly;
   no separate array pointer exists in this layout. */
void __stdcall mark_pch_inline_vector(InlineVector *vector, PchVisit visit)
{
    int i;
    if (!vector) return;
    for (i = 0; i < vector->count; ++i) {
        mark_pch_pointer(&vector->items[i]);
        if (visit) (*visit)(vector->items[i]);
    }
}

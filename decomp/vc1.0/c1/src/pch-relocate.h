#ifndef PCH_RELOCATE_H
#define PCH_RELOCATE_H
#include "pch.h"
#include "parser.h"
#include "preprocessor.h"
#include <recon.h>

typedef struct PermanentPage {
    unsigned char *start;
    BitSet *relocations;
} PermanentPage;
extern PermanentPage permanent_pages[512], *current_page;
extern unsigned char *permanent_next;
extern unsigned permanent_available;
extern Vector *pch_saved_defines, *pch_saved_include_directories;
/* An eight-byte chain row holds a page-relative patch location and the
   destination page. Multiple rows cover gaps larger than sixteen bits. */
typedef struct PchRelocationChain {
    unsigned head;
    unsigned short page, reserved;
} PchRelocationChain;
extern PchRelocationChain *pch_relocation_chains;
extern int pch_relocation_capacity;
extern unsigned pch_relocation_count;
extern unsigned pch_reference;
extern unsigned char *pch_reference_base;
extern unsigned short pch_reference_page;
typedef struct PchReservedState {
    unsigned after_roots, string_count, record_count, one, zero;
    unsigned short debug_padding;
    unsigned debug_bytes;
} PchReservedState;
extern PchReservedState pch_reserved_state;
extern unsigned *pch_root_cells[105];
extern void (*pch_graph_markers[12])(void);
void mark_pch_graph(void);
void __stdcall pointer_to_pch_reference(void *pointer);
void initialize_pch_relocations(void);
void relocate_pch_pages(void);
int __stdcall mark_pch_pointer(void **slot);
void __stdcall mark_pch_ir_node(IrToken *node);
Vector *__stdcall encode_pch_vector(Vector *vector);
Vector *__stdcall decode_pch_vector(Vector *vector);
void __stdcall mark_pch_hash_entries(struct HashEntry *entry,
    void (__stdcall *mark_key)(void *), void (__stdcall *mark_value)(void *));
void __stdcall mark_pch_hash_table(struct HashTable *table,
    void (__stdcall *mark_key)(void *), void (__stdcall *mark_value)(void *));
void __stdcall mark_pch_tag(FrontendSymbol *tag);
void __stdcall mark_pch_symbol_chain(FrontendSymbol *symbol);
#define PCH_MARK(field_) mark_pch_pointer((void **)&(field_))
void mark_pch_scopes(void);
void __stdcall mark_pch_type(CType *type);
void __stdcall mark_pch_type_entries(TypeEntry *entry);
void mark_pch_types(void);
void __stdcall mark_pch_shape(TypeShape *shape);
void __stdcall mark_pch_shape_entries(TypeShapeEntry *entry);
void mark_pch_shapes(void);
void __stdcall mark_pch_macro_entries(MacroDefinition *entry);
void mark_pch_macros(void);
void __stdcall mark_pch_identifier_entries(PreprocessorSymbol *entry);
void mark_pch_identifiers(void);
void mark_pch_function_records(void);
void __stdcall mark_pch_pointer_array(void **slots, int count,
    const char *unused_scope_name, const char *unused_item_name);
void mark_pch_scope_directory(void);
void __stdcall mark_pch_parameter_entries(ParameterEntry *entry);
void __stdcall mark_pch_parameter_info(ParameterInfo *parameters);
void mark_pch_parameters(void);
void mark_pch_pragma_values(void);
void __stdcall mark_pch_vector(Vector *vector, void (__stdcall *mark_item)(void *));
void mark_pch_defines(void);
void mark_pch_include_directories(void);
void mark_pch_packing(void);
typedef char PermanentPage_size[sizeof(PermanentPage) == 8 ? 1 : -1];
typedef char PchRelocationChain_size[sizeof(PchRelocationChain) == 8 ? 1 : -1];
typedef char PchRelocationChain_page_offset[offsetof(PchRelocationChain, page) == 4 ? 1 : -1];
#endif

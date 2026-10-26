#ifndef CV510_PACK_H
#define CV510_PACK_H
#include "image.h"
#include <recon.h>
extern ModuleInfo *current_module_info;
extern unsigned public_scope_depth;
extern int minimum_pack;
extern unsigned module_type_low, current_module_flags;
extern unsigned line_entry_index, symbol_entry_index, type_entry_index;
typedef struct LegacyTypeSymbol {
    struct LegacyTypeSymbol *next;
    unsigned short length, kind;
    unsigned index;
    unsigned char name[1];
} LegacyTypeSymbol;
extern LegacyTypeSymbol *pending_legacy_symbols, *last_legacy_symbol;
extern unsigned current_type_count, has_precompiled;
extern unsigned total_symbol_bytes, total_line_bytes;
extern unsigned short module_symbol_count, module_reference_count, module_procedure_count;
extern unsigned short current_type_index;
extern unsigned char *current_module_buffer;
extern unsigned char *module_symbol_records;
void initialize_single_module(CodeViewDirectoryEntry *entry, ModuleInfo *info);
void read_module_types(CodeViewDirectoryEntry *entry, int precompiled);
extern unsigned module_type_entries, module_type_list_used, module_precompiled_limit;
extern unsigned type_record_capacity, source_type_input_bytes, current_type_server;
extern unsigned type_module_count;
ModuleInfo *find_precompiled_module(const unsigned char *name);
void initialize_module_type_storage(void);
void use_precompiled_types(void *leaf);
void register_type_server(const unsigned char *leaf);
void __fastcall append_module_type(unsigned char *data, unsigned local,
    unsigned precompiled, unsigned mapped_index, unsigned skip);
void read_new_module_types(unsigned size, int precompiled);
void read_legacy_module_types(void *types, unsigned size);
void reset_type_name_cache(void);
void prepare_precompiled_types(ModuleInfo *info);
void read_module_symbols(unsigned index, CodeViewDirectoryEntry *entry);
const char *module_name(ModuleInfo *info);
unsigned widen_subsection(unsigned char **buffer, unsigned size, unsigned *capacity);
void read_module_local_symbols(unsigned index, CodeViewDirectoryEntry *entry);
void read_module_source_lines(unsigned index, CodeViewDirectoryEntry *entry);
void pack_symbols(void *symbols, CodeViewDirectoryEntry *entry);
unsigned __fastcall map_legacy_index(unsigned index);
unsigned __fastcall map_type_index(unsigned index);
typedef unsigned short (*SymbolKeyProc)(const unsigned char *, unsigned *, unsigned short *, unsigned short *, unsigned *, unsigned);
unsigned short numeric_leaf_size(const unsigned char *record);
unsigned short symbol_record_key(const unsigned char *record, unsigned *hash, unsigned short *name, unsigned short *segment, unsigned *offset, unsigned flags);
typedef struct PublicSymbol {
    struct PublicSymbol *next;
    unsigned hash, offset;
    unsigned short name_offset, segment;
    unsigned char record[1];
} PublicSymbol;
PublicSymbol *allocate_public_block(unsigned size);
extern unsigned public_symbol_bytes, public_symbol_count;
int insert_public_symbol(unsigned char *record, SymbolKeyProc key);
int __fastcall insert_global_symbol(unsigned char *record, SymbolKeyProc key);
void decode_legacy_lines(void *lines, unsigned size);
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info);
extern unsigned short symbol_segment_count;
extern unsigned short symbol_segments[255];
void scan_module_symbols(void *lines, unsigned size, unsigned short *symbols, unsigned short *references);
void add_symbol_segment(unsigned short segment, unsigned short *symbols);
int has_symbol_segment(unsigned short segment);
void read_type_database(const char *path);
void build_type_server_directory(char *path);
unsigned char *module_file_name(unsigned short index);
extern void *type_pdb, *type_pdb_types;
extern int type_pdb_close_status;
extern char type_database_path[260], current_type_server_path[260];
extern int type_database_error;
extern unsigned type_database_count;
typedef void (*SymbolRewriteProc)(void);
typedef struct SymbolRewriteEntry {
    unsigned short kind, reserved;
    SymbolRewriteProc rewrite;
} SymbolRewriteEntry;
/* The default dispatch row shares its first DWORD with the next global index.
 * Preserve that observed storage identity, including its NULL callback. */
typedef union SymbolRewriterState {
    SymbolRewriteEntry entries[39];
    struct {
        SymbolRewriteEntry predefined[38];
        unsigned next_global_type;
        SymbolRewriteProc fallback;
    } state;
} SymbolRewriterState;
extern SymbolRewriterState symbol_rewriters;
extern unsigned char *symbol_input, *symbol_output;
extern unsigned discard_symbol_depth, packed_symbol_bytes;
void rewrite_module_symbols(void *lines, CodeViewDirectoryEntry *entry, ModuleInfo *info, unsigned short *symbols, unsigned short *references);
unsigned short write_segment_record(unsigned char *buffer, unsigned short segment);
int link_symbol_scopes(unsigned char *buffer, unsigned size);
void record_symbol_references(unsigned char *buffer, unsigned size, unsigned short module);
void record_symbol_reference(const unsigned char *record, PublicSymbol **buckets,
    unsigned short module, unsigned offset, unsigned short kind);
void enter_symbol_scope(void);
void leave_symbol_scope(void);
void rewrite_symbol_type4(void);
void rewrite_bp_relative_symbol(void);
void rewrite_global_data_symbol(void);
void rewrite_register16_symbol(void);
void rewrite_data16_symbol(void);
void rewrite_global_data16_symbol(void);
void rewrite_type_name_symbol(void);
void rewrite_object_name_symbol(void);
void rewrite_procedure16_symbol(void);
void rewrite_procedure32_symbol(void);
void rewrite_mips_procedure_symbol(void);
int module_contains_address(ModuleInfo *info, unsigned short segment, unsigned offset);
void rewrite_module_global_types(void *types, CodeViewDirectoryEntry *entry, ModuleInfo *info);
#endif

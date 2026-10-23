#ifndef CV510_PACK_H
#define CV510_PACK_H
#include "image.h"
#include <recon.h>
extern ModuleInfo *current_module_info;
extern unsigned public_scope_depth;
extern int minimum_pack;
extern unsigned module_type_low, module_symbol_flags, current_module_flags;
extern unsigned line_entry_index, symbol_entry_index, type_entry_index;
extern unsigned current_type_count, has_precompiled, pending_module_bytes;
extern unsigned total_symbol_bytes, total_line_bytes, strip_module_lines;
extern unsigned short module_symbol_count, module_reference_count, module_procedure_count;
extern unsigned short current_type_index, current_module_has_types;
extern unsigned char *current_module_buffer;
void initialize_single_module(CodeViewDirectoryEntry *entry, ModuleInfo *info);
void read_module_types(CodeViewDirectoryEntry *entry, int precompiled);
void prepare_precompiled_types(ModuleInfo *info);
void read_module_symbols(unsigned index, CodeViewDirectoryEntry *entry);
const char *module_name(ModuleInfo *info);
unsigned widen_subsection(unsigned char **buffer, unsigned size, unsigned *capacity);
void read_module_lines(unsigned index, CodeViewDirectoryEntry *entry);
void read_module_global_types(unsigned index, CodeViewDirectoryEntry *entry);
void pack_symbols(void *symbols, CodeViewDirectoryEntry *entry);
unsigned __fastcall map_legacy_index(unsigned index);
unsigned __fastcall map_type_index(unsigned index);
typedef unsigned short (*SymbolKeyProc)(const unsigned char *, unsigned *, unsigned short *, unsigned short *, unsigned *, unsigned);
unsigned short symbol_record_key(const unsigned char *record, unsigned *hash, unsigned short *name, unsigned short *segment, unsigned *offset, unsigned flags);
typedef struct PublicSymbol {
    struct PublicSymbol *next;
    unsigned hash, offset;
    unsigned short name_offset, segment;
    unsigned char record[1];
} PublicSymbol;
PublicSymbol *allocate_public_block(unsigned size);
int insert_public_symbol(unsigned char *record, SymbolKeyProc key);
void decode_legacy_lines(void *lines, unsigned size);
void rewrite_legacy_lines(void *lines, CodeViewDirectoryEntry *entry, void *module_buffer, ModuleInfo *info);
void decode_new_lines(void *lines, unsigned size, unsigned short *symbols, unsigned short *references);
void rewrite_new_lines(void *lines, CodeViewDirectoryEntry *entry, ModuleInfo *info, unsigned short *symbols, unsigned short *references);
void rewrite_module_global_types(void *types, CodeViewDirectoryEntry *entry, ModuleInfo *info);
#endif

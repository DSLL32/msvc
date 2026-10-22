#ifndef LINK_PASSES_H
#define LINK_PASSES_H
#include "object.h"
#include "options.h"
#include "comdat.h"
#include <time.h>
typedef struct SectionDefault { char *name; unsigned long flags; } SectionDefault;
typedef struct Fixup { unsigned long type,offset,rva; } Fixup;
typedef struct FixupBlock { struct FixupBlock *next; Fixup records[341]; } FixupBlock;
typedef struct BaseFixup BaseFixup;
typedef struct ModuleDebugInfo ModuleDebugInfo;
extern SectionDefault readonly_section_defaults,bss_section_defaults,debug_section_defaults;
extern SectionDefault idata_directory_defaults,reloc_section_defaults;
extern Module *output_module;
extern Contribution *bss_contribution;
extern Section *base_relocation_section,*debug_section;
extern ComdatNode *comdat_nodes;
extern ComdatRoot *comdat_roots;
extern External *entry_symbol;
extern unsigned long undefined_symbols;
extern unsigned long bss_size,symbol_count,weak_symbol_count,string_table_size,local_symbol_count;
extern unsigned long mac_far_bss_offset;
extern Contribution *mac_far_bss;
extern unsigned long symbol_record_size,pe_header_offset;
extern int output_file;
char *replace_extension(const char *,const char *);
void apply_section_merges(Image *);
void sort_externals(SymbolTable *);
void read_absolute_symbols(const char *,External **,SymbolTable *);
void resolve_entry_symbol(SymbolTable *);
void resolve_local_imports(Image *,Contribution **,ExternalLink **);
void emit_externals(SymbolTable *,unsigned char defined,unsigned char emit,unsigned char name_order,ImageOptions *);
unsigned char contribution_discarded(Contribution *);
unsigned write_coff_symbol(int,IMAGE_SYMBOL *);
void initialize_mac_resources(void);
void initialize_mac_exports(void);
unsigned short count_output_sections(Image *);
External *find_external(SymbolTable *,unsigned char,const char *);
void move_section_last(Section *,SectionList *);
unsigned long size_mac_jump_table(SymbolTable *);
void initialize_mac_section_numbers(unsigned short);
void crt_tzset(void);
void order_sections(Image *);
void resolve_comdat_edges(SymbolTable *,ComdatNode *);
void visit_comdat_roots(ComdatRoot *,SymbolTable *);
void print_comdat_graph(ComdatNode *);
void release_comdat_storage(void);
void set_mac_export_mode(unsigned char);
void build_mac_resources(Image *);
void finalize_mac_debug(void);
void initialize_mac_loader(void);
void layout_sections(Image *,unsigned long,unsigned long *,unsigned long *,unsigned long *,unsigned long *,unsigned long *);
unsigned long size_mac_data_fixups(Image *,unsigned long);
void assign_section_numbers(SectionList *);
unsigned write_file(int,const void *,unsigned);
void pad_contribution(Contribution *);
void write_section_headers(unsigned long,unsigned long *,unsigned long *,Image *);
void make_section_header(Section *,IMAGE_SECTION_HEADER *);
void write_section_header(int,IMAGE_SECTION_HEADER *);
void assign_external_addresses(Contribution *,ExternalLink **,IMAGE_OPTIONAL_HEADER *,ImageOptions *);
unsigned long count_modules(Library *);
void finalize_mac_fixups(void);
void write_mac_debug(Image *);
void write_mac_resources(void);
void write_mac_swapper(void);
void dump_symbols(SymbolTable *);
void pass_two(Image *);
void emit_rom_relocations(void);
void emit_base_relocations(IMAGE_OPTIONAL_HEADER *);
unsigned long count_externals(SymbolTable *);
unsigned char emit_symbol_class(unsigned char,ImageOptions *);
void emit_string_table(SymbolTable *);
void emit_codeview(Image *);
int compare_fpo_records(const void *,const void *);
void finish_pe_image(Image *,void *);
void write_coff_header(int,IMAGE_FILE_HEADER *);
void write_optional_header(int,IMAGE_OPTIONAL_HEADER *,unsigned short);
void update_checksum(int);
#endif

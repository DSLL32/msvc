#ifndef LINK_OBJECT_H
#define LINK_OBJECT_H
#include "pass-one.h"
#include <recon.h>
typedef struct MacSymbolReference {
    unsigned long flags;
    Section *section;
} MacSymbolReference;
typedef char MacSymbolReferenceIs8[sizeof(MacSymbolReference)==8?1:-1];
extern MacSymbolReference *mac_symbol_references;
typedef struct MacRelocationRef { Contribution *contribution; unsigned long offset; } MacRelocationRef;
typedef struct MacFixupList {
    MacRelocationRef *records;
    unsigned long capacity,count,used;
    struct MacFixupList *last,*next;
} MacFixupList;
typedef char MacFixupListIs24[sizeof(MacFixupList)==24?1:-1];
extern MacFixupList *mac_section_code_fixups,*mac_section_data_fixups;
PIMAGE_RELOCATION allocate_coff_relocations(unsigned long);
unsigned read_coff_relocations(int,PIMAGE_RELOCATION,unsigned long);
void release_coff_relocations(PIMAGE_RELOCATION);
void add_relocation_edge(SymbolTable *,Contribution *,PIMAGE_SYMBOL,PIMAGE_RELOCATION);
void validate_mac_code_relocation(unsigned short);
void append_mac_fixup(MacFixupList *,Contribution *,unsigned long);
void record_mac_symbol_reference(unsigned long,Section *,unsigned long);
void record_mac_data_reference(Contribution *,unsigned long,unsigned long);
unsigned read_coff_symbol(int,PIMAGE_SYMBOL);
PIMAGE_SYMBOL next_coff_symbol(PIMAGE_SYMBOL *,unsigned char);
void process_mac_static_symbol(Contribution *,PIMAGE_SYMBOL,unsigned long);
void record_mac_external(External *,unsigned long);
void format_input_reference(char *,const char *,const char *);
void prepend_external(ExternalLink **,External *);
void record_undefined_reference(External *,Module *);
void process_weak_external(SymbolTable *,PIMAGE_SYMBOL *,PIMAGE_SYMBOL,External *,Module *,unsigned char *,unsigned char,unsigned char *,unsigned short);
void define_external(External *,Contribution *,unsigned long,short,unsigned short,unsigned short,SymbolTable *);
void mark_import_contributions(Module *);
void process_object_section(Image *,Module *,unsigned char,unsigned short,PIMAGE_SYMBOL);
void filter_debug_contribution(Contribution *,ImageOptions *);
unsigned long align_contribution(unsigned long,unsigned long);
void initialize_mac_object_symbols(unsigned long);
void finish_mac_object_symbols(void);
void process_mac_object_symbols(SymbolTable *,PIMAGE_SYMBOL,char *);
void process_object_symbols(Image *,Module *,unsigned char *,unsigned char *,PIMAGE_SYMBOL,unsigned short);
void release_coff_strings(char *);
void release_coff_symbols(PIMAGE_SYMBOL);
unsigned char skip_coff_auxiliaries(PIMAGE_SYMBOL *,unsigned char);
unsigned char select_comdat(SymbolTable *,Contribution *,PIMAGE_SYMBOL,unsigned short,char **);
void attach_comdat_node(Module *,Contribution *,const char *);
void classify_contribution(Contribution *,ImageOptions *);
void read_contribution_relocations(Image *,Contribution *,PIMAGE_SYMBOL);
char *format_module_name(Module *,char *);
void process_directives(char *,Image *,const char *,unsigned char);
#endif

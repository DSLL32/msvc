#ifndef LINK_PASS_TWO_H
#define LINK_PASS_TWO_H
#include "link-pass.h"
typedef struct FunctionSymbolMap FunctionSymbolMap;
FunctionSymbolMap *allocate_function_symbol_maps(unsigned short);
void release_function_symbol_maps(FunctionSymbolMap *,unsigned short);
void update_debug_symbol(Image *,PIMAGE_SYMBOL);
External *update_external_symbol(Image *,PIMAGE_SYMBOL,Module *);
void record_map_symbol(const char *,Contribution *,unsigned long,unsigned char);
void update_static_symbol(Image *,PIMAGE_SYMBOL,Module *);
void update_file_symbol(Image *,PIMAGE_SYMBOL,unsigned char *);
void update_section_symbol(Image *,PIMAGE_SYMBOL,Module *);
unsigned char is_section_symbol(Image *,PIMAGE_SYMBOL);
void update_function_auxiliary(PIMAGE_SYMBOL,short,PIMAGE_AUX_SYMBOL,unsigned long *,unsigned long *,FunctionSymbolMap *,unsigned long,Image *);
void update_block_auxiliary(PIMAGE_SYMBOL,PIMAGE_AUX_SYMBOL,unsigned long *,unsigned long *,unsigned long *,unsigned long);
unsigned write_coff_auxiliary(int,PIMAGE_AUX_SYMBOL);
void process_symbol_auxiliaries(Image *,PIMAGE_SYMBOL,short,PIMAGE_SYMBOL *,unsigned char,unsigned char,FunctionSymbolMap *,unsigned long *);
void process_output_symbol(Image *,Module *,PIMAGE_SYMBOL,PIMAGE_SYMBOL *,FunctionSymbolMap *,unsigned long *);
void write_mac_loader_prologue(Contribution *);
void *mapped_output_pointer(int,unsigned long,unsigned);
void prepare_relocations(void);
void record_debug_fixup(unsigned short,unsigned long,unsigned long);
void record_map_relocation(unsigned long);
void write_contribution(Image *,Contribution *,PIMAGE_SYMBOL);
unsigned read_coff_lines(int,PIMAGE_LINENUMBER,unsigned long);
unsigned write_coff_lines(int,PIMAGE_LINENUMBER,unsigned long,ImageOptions *);
unsigned char lookup_function_symbol(FunctionSymbolMap *,unsigned short,unsigned long,unsigned long *,unsigned long *);
void process_contribution_debug(Image *,Contribution *,unsigned short,FunctionSymbolMap *);
External *find_mac_contribution_external(Contribution *);
void process_module_output(Image *,Module *);
#endif

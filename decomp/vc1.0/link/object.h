#ifndef LINK_OBJECT_H
#define LINK_OBJECT_H
#include "pass-one.h"
void process_object_section(Image *,Module *,unsigned char,unsigned short,PIMAGE_SYMBOL);
void filter_debug_contribution(Contribution *,ImageOptions *);
unsigned long align_contribution(unsigned long,unsigned long);
void initialize_mac_object_symbols(unsigned long);
void finish_mac_object_symbols(void);
void process_mac_object_symbols(SymbolTable *,PIMAGE_SYMBOL,char *);
void process_object_symbols(Image *,Module *,unsigned char *,unsigned char *,PIMAGE_SYMBOL,unsigned short);
void release_coff_strings(char *);
void release_coff_symbols(PIMAGE_SYMBOL);
unsigned char select_comdat(SymbolTable *,Contribution *,PIMAGE_SYMBOL,unsigned short,char **);
void attach_comdat_node(Module *,Contribution *,const char *);
void classify_contribution(Contribution *,ImageOptions *);
void read_contribution_relocations(Image *,Contribution *,PIMAGE_SYMBOL);
char *format_module_name(Module *,char *);
void process_directives(char *,Image *,const char *,unsigned char);
#endif

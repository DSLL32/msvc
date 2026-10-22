/* Output operations not yet entered by accepted frontiers. */
#include "link-pass.h"
char * replace_extension(const char *name,const char *extension) { __asm { int 3 } return 0; }
void sort_externals(SymbolTable *table) { __asm { int 3 } }
void read_absolute_symbols(const char *file,External **sorted,SymbolTable *table) { __asm { int 3 } }
void resolve_entry_symbol(SymbolTable *table) { __asm { int 3 } }
void initialize_mac_resources(void) { __asm { int 3 } }
void initialize_mac_exports(void) { __asm { int 3 } }
unsigned long size_mac_jump_table(SymbolTable *table) { __asm { int 3 } return 0; }
void initialize_mac_section_numbers(unsigned short count) { __asm { int 3 } }
void print_comdat_graph(ComdatNode *nodes) { __asm { int 3 } }
void set_mac_export_mode(unsigned char mode) { __asm { int 3 } }
void build_mac_resources(Image *image) { __asm { int 3 } }
void finalize_mac_debug(void) { __asm { int 3 } }
void initialize_mac_loader(void) { __asm { int 3 } }
unsigned long size_mac_data_fixups(Image *image,unsigned long file) { __asm { int 3 } return 0; }
void assign_section_numbers(SectionList *sections) { __asm { int 3 } }
unsigned write_file(int file,const void *data,unsigned size) { __asm { int 3 } return 0; }
void pad_contribution(Contribution *contribution) { __asm { int 3 } }
void write_section_headers(unsigned long mask,unsigned long *state,unsigned long *rva,Image *image) { __asm { int 3 } }
void make_section_header(Section *section,IMAGE_SECTION_HEADER *header) { __asm { int 3 } }
void write_section_header(int file,IMAGE_SECTION_HEADER *header) { __asm { int 3 } }
void assign_external_addresses(Contribution *storage,ExternalLink **aliases,IMAGE_OPTIONAL_HEADER *header,ImageOptions *options) { __asm { int 3 } }
unsigned long count_modules(Library *libraries) { __asm { int 3 } return 0; }
void finalize_mac_fixups(void) { __asm { int 3 } }
void write_mac_debug(Image *image) { __asm { int 3 } }
void write_mac_resources(void) { __asm { int 3 } }
void write_mac_swapper(void) { __asm { int 3 } }
void dump_symbols(SymbolTable *table) { __asm { int 3 } }
void pass_two(Image *image) { __asm { int 3 } }
void emit_rom_relocations(void) { __asm { int 3 } }
void emit_base_relocations(IMAGE_OPTIONAL_HEADER *header) { __asm { int 3 } }
void emit_string_table(SymbolTable *table) { __asm { int 3 } }
void emit_codeview(Image *image) { __asm { int 3 } }
int compare_fpo_records(const void *left,const void *right) { __asm { int 3 } return 0; }
void finish_pe_image(Image *image,void *zeroes) { __asm { int 3 } }
void write_coff_header(int file,IMAGE_FILE_HEADER *header) { __asm { int 3 } }
void write_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,unsigned short size) { __asm { int 3 } }
void update_checksum(int file) { __asm { int 3 } }

External **get_externals_by_address(SymbolTable *table) { __asm { int 3 } return 0; }
unsigned write_coff_symbol(int file,IMAGE_SYMBOL *symbol) { __asm { int 3 } return 0; }
char *undecorate_symbol_name(const char *name,unsigned char decorated) { __asm { int 3 } return 0; }








void end_comdat_node_enumerator(ComdatNodeEnumerator *scan) { __asm { int 3 }  }



void end_comdat_edge_enumerator(ComdatEdgeEnumerator *scan) { __asm { int 3 }  }



void end_comdat_root_enumerator(ComdatRootEnumerator *scan) { __asm { int 3 }  }

void build_mac_section_fixups(Section *section,Image *image) { __asm { int 3 } }
unsigned long size_mac_section_fixups(Section *section,unsigned long file) { __asm { int 3 } return 0; }

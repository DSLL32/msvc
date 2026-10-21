/* Output operations not yet entered by accepted frontiers. */
#include "link-pass.h"
char * replace_extension(const char *name,const char *extension) { __asm { int 3 } return 0; }
void allocate_common_symbols(Image *image) { __asm { int 3 } }
void set_external_defined(External *external,unsigned char defined,SymbolTable *table) { __asm { int 3 } }
void sort_externals(SymbolTable *table) { __asm { int 3 } }
void read_absolute_symbols(const char *file,External **sorted,SymbolTable *table) { __asm { int 3 } }
void resolve_entry_symbol(SymbolTable *table) { __asm { int 3 } }
void collect_symbol_layout(Image *image,unsigned long *first,unsigned long *last) { __asm { int 3 } }
void emit_externals(SymbolTable *table,unsigned char output,unsigned char all,unsigned char define,ImageOptions *options) { __asm { int 3 } }
Section * find_section_by_name(const char *name,SectionList *sections) { __asm { int 3 } return 0; }
void initialize_mac_resources(void) { __asm { int 3 } }
void initialize_mac_exports(void) { __asm { int 3 } }
unsigned short count_output_sections(Image *image) { __asm { int 3 } return 0; }
External * find_external(SymbolTable *table,unsigned char short_name,const char *name) { __asm { int 3 } return 0; }
void move_section_last(Section *section,SectionList *sections) { __asm { int 3 } }
unsigned long size_mac_jump_table(SymbolTable *table) { __asm { int 3 } return 0; }
void initialize_mac_section_numbers(unsigned short count) { __asm { int 3 } }
void crt_tzset(void) { __asm { int 3 } }
void order_sections(Image *image) { __asm { int 3 } }
void resolve_comdat_edges(SymbolTable *table,ComdatNode *nodes) { __asm { int 3 } }
void visit_comdat_roots(ComdatRoot *roots,SymbolTable *table) { __asm { int 3 } }
void print_comdat_graph(ComdatNode *nodes) { __asm { int 3 } }
void release_comdat_storage(void) { __asm { int 3 } }
void set_mac_export_mode(unsigned char mode) { __asm { int 3 } }
void build_mac_resources(Image *image) { __asm { int 3 } }
void finalize_mac_debug(void) { __asm { int 3 } }
void initialize_mac_loader(void) { __asm { int 3 } }
void layout_sections(Image *image,unsigned long mask,unsigned long *rva,unsigned long *file,unsigned long *fpo,unsigned long *lines,unsigned long *state) { __asm { int 3 } }
unsigned long size_mac_data_fixups(Image *image,unsigned long file) { __asm { int 3 } return 0; }
void assign_section_numbers(SectionList *sections) { __asm { int 3 } }
unsigned write_file(int file,const void *data,unsigned size) { __asm { int 3 } return 0; }
void pad_contribution(Contribution *contribution) { __asm { int 3 } }
void write_section_headers(unsigned long mask,unsigned long *state,unsigned long *rva,Image *image) { __asm { int 3 } }
void make_section_header(Section *section,IMAGE_SECTION_HEADER *header) { __asm { int 3 } }
void write_section_header(int file,IMAGE_SECTION_HEADER *header) { __asm { int 3 } }
void assign_external_addresses(unsigned long first,unsigned long *last,IMAGE_OPTIONAL_HEADER *header,ImageOptions *options) { __asm { int 3 } }
unsigned long count_modules(Library *libraries) { __asm { int 3 } return 0; }
void finalize_mac_fixups(void) { __asm { int 3 } }
void write_mac_debug(Image *image) { __asm { int 3 } }
void write_mac_resources(void) { __asm { int 3 } }
void write_mac_swapper(void) { __asm { int 3 } }
void dump_symbols(SymbolTable *table) { __asm { int 3 } }
void pass_two(Image *image) { __asm { int 3 } }
void emit_rom_relocations(void) { __asm { int 3 } }
void emit_base_relocations(IMAGE_OPTIONAL_HEADER *header) { __asm { int 3 } }
unsigned long count_externals(SymbolTable *table) { __asm { int 3 } return 0; }
unsigned char emit_symbol_class(unsigned short storage_class,ImageOptions *options) { __asm { int 3 } return 0; }
void emit_string_table(SymbolTable *table) { __asm { int 3 } }
void emit_codeview(Image *image) { __asm { int 3 } }
int compare_fpo_records(const void *left,const void *right) { __asm { int 3 } return 0; }
void finish_pe_image(Image *image,void *zeroes) { __asm { int 3 } }
void write_coff_header(int file,IMAGE_FILE_HEADER *header) { __asm { int 3 } }
void write_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,unsigned short size) { __asm { int 3 } }
void update_checksum(int file) { __asm { int 3 } }

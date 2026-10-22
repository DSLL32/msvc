#ifndef LINK_IMPORT_LIBRARY_H
#define LINK_IMPORT_LIBRARY_H
#include "library.h"
/* Private target description, extended as import-member fields are recovered. */
typedef struct ImportMachine {
    unsigned short machine;
    unsigned char helper_thunk;
} ImportMachine;
extern ImportMachine import_i386,import_i860,import_mips;
extern SymbolTable *export_symbol_table;
extern Image *export_image;
extern External *exports_table_symbol;
extern char *null_thunk_name,*export_object_name;
extern unsigned long dll_basename_length;
extern ExternalLink *import_aliases;
extern unsigned long *import_member_offsets;
extern unsigned char *export_ordinals;
extern unsigned char import_only_export,export_error;
extern unsigned short import_member_base;
extern unsigned long first_export_ordinal,last_export_ordinal;
void reject_output_input_collision(const char *,Argument *);
void read_definition_file(Image *,const char *);
void copy_argument_node(ArgumentList *,char *,char *);
void clear_argument_list(ArgumentList *);
char *take_export_token(char *,const char *,const char *,char *);
extern Library *export_library;
extern Module *export_module;
extern Contribution *export_contribution;
extern SectionList export_sections;
extern unsigned long export_forwarder_bytes,export_alias_bytes;
void record_export_ordinal(unsigned long);
void register_export(SymbolTable *,char *,char *,char *,int,unsigned long,const char *,unsigned char,Image *,IMAGE_OPTIONAL_HEADER *);
void parse_export(char *,Image *,unsigned char,const char *);
void find_export_matches(SymbolTable *,const char *,unsigned short *,char **,char **);
void merge_export_definitions(SymbolTable *,SymbolTable *,unsigned char,Library *);
void invalidate_symbol_sorts(SymbolTable *);
unsigned long count_export_symbols(SymbolTable *,unsigned long *);
ExternalLink *create_import_aliases(SymbolTable *);
void mark_explicit_export_ordinals(SymbolTable *,unsigned char *);
extern unsigned char new_archive_index,import_member_long_name;
extern unsigned long second_index_offset;
extern char *import_member_name;
PIMAGE_ARCHIVE_MEMBER_HEADER build_archive_header(const char *,unsigned char,unsigned long);
void write_import_member_names(SymbolTable *,unsigned short);
void write_import_archive_indexes(unsigned long,SymbolTable *);
void write_import_descriptor(const ImportMachine *,Image *);
void write_null_import_descriptor(Image *);
void write_null_thunk_member(Image *);
void write_helper_thunk_member(const ImportMachine *,Image *);
void write_export_object(Image *,unsigned long,unsigned long,const ImportMachine *,unsigned char);
void write_import_members(Image *,const ImportMachine *);
void finish_import_archive_indexes(SymbolTable *);
#endif

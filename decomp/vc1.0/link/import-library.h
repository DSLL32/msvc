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
extern unsigned long dll_basename_length,import_name_bytes;
extern unsigned long *import_member_offsets;
extern unsigned char *export_ordinals;
extern unsigned char import_only_export,export_error;
extern unsigned short import_member_base;
extern unsigned long first_export_ordinal,last_export_ordinal;
void reject_output_input_collision(const char *,Argument *);
void read_definition_file(Image *,const char *);
void copy_argument_node(ArgumentList *,char *,char *);
void clear_argument_list(ArgumentList *);
void parse_export(const char *,Image *,unsigned char,const char *);
void merge_export_definitions(SymbolTable *,SymbolTable *,unsigned char,Library *);
void prepare_export_order(SymbolTable *);
unsigned long count_export_symbols(SymbolTable *,unsigned long *);
unsigned long count_export_name_bytes(SymbolTable *);
void assign_export_ordinals(SymbolTable *,unsigned char *);
void write_import_archive_indexes(unsigned long,SymbolTable *);
void write_import_descriptor(const ImportMachine *,Image *);
void write_null_import_descriptor(Image *);
void write_null_thunk_member(Image *);
void write_helper_thunk_member(const ImportMachine *,Image *);
void write_export_object(Image *,unsigned long,unsigned long,const ImportMachine *,unsigned char);
void write_import_members(Image *,const ImportMachine *);
void finish_import_archive_indexes(SymbolTable *);
#endif

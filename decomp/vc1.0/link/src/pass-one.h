#ifndef LINK_PASS_ONE_H
#define LINK_PASS_ONE_H
#include "image.h"
#include "arguments.h"
void pass_one(Image *);
void process_input_argument(Argument *,Image *);
void recognize_input_file(Argument *,Library *,Image *,Module **);
Module *read_object_module(Argument *,unsigned short,Image *,Library *);
void reproduce_input(const char *,unsigned char);
void check_explicit_modules(Image *,Library *);
void resolve_weak_externals(SymbolTable *,unsigned long);
void infer_subsystem(Image *);
void search_libraries(Image *);
Library *load_next_default_library(LibraryList *);
void search_library_members(Image *,Library *,unsigned char *,unsigned char *);
unsigned char create_import_library(Image *,const char *,const char *);
void warn_unused_libraries(Library *);
void order_import_contributions(Image *);
void apply_order_file(Image *);
void free_pointer(void **);
void begin_library(Library *,Image *);
void convert_resource_input(Argument *,unsigned short);
void convert_object(Argument *,unsigned short *,Image *);
void convert_omf(Argument *,unsigned short *,ImageOptions *);
long tell_file(int);
void process_object(Image *,unsigned char *,unsigned char *,Module *,unsigned char,unsigned short);
#endif

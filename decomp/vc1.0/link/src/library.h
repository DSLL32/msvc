#ifndef LINK_LIBRARY_H
#define LINK_LIBRARY_H
#include "link-pass.h"
#include "archive.h"
extern char *library_listing_name,*extract_member;
extern Library *first_input_library;
extern unsigned char remove_members_requested;
void process_library_options(Image *);
int build_import_library(int,char **,Image *);
void convert_omf_object(Argument *,unsigned char,ImageOptions *);
void convert_omf_inputs(unsigned char,ImageOptions *);
void validate_library_inputs(Image *);
void release_library_list(LibraryList *);
unsigned char remove_library_member(Image *,const char *);
int write_library(unsigned char,Image *);
#endif

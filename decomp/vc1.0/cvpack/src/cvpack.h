#ifndef CVPACK_H
#define CVPACK_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <recon.h>

extern int show_logo, banner_pending, cvnew, feedback, pcode;
extern unsigned char mapped_io, is_pe_image;
extern int input_file, already_packed, checksum_required;
extern unsigned short module_count, statistics_enabled;
extern int verbose, strip_symbols, minimum_types;
extern char *debug_filename;
extern char executable_name[260];
extern void *directory_data, *module_data, *type_data, *symbol_data, *global_type_data;

void parse_options(int, char **);
void print_banner(void);
void fatal_error(int, const char *, const char *);
void warning(int, const char *, const char *);
void files_initialize(int, int, int, int, int, unsigned char,
                      void (*)(int, const char *, const char *));
int open_file(const char *, int, ...);
int close_file(int, unsigned char);
void initialize_type_tables(void);
void read_image_debug(void);
int pack_module(unsigned short);
unsigned short strip_debug(int);
void finalize_types(void);
void build_source_file_index(void);
void free_type_tables(void);
int update_image_checksum(const char *, unsigned long, unsigned char);

#endif

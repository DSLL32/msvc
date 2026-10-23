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
extern unsigned char mapped_io, debug_present;
extern int input_file, already_packed, checksum_required;
extern unsigned short module_count, statistics_enabled;
extern char executable_name[260];
extern void *directory_data, *module_data, *type_data, *symbol_data, *line_data;

void parse_options(int, char **);
void print_banner(void);
void fatal_error(int, const char *, const char *);
void warning(int, const char *, const char *);
void files_initialize(int, int, int, int, int, unsigned char,
                      void (*)(int, const char *, const char *));
int open_file(const char *, int, ...);
int close_file(int, unsigned char);
void read_debug_directory(void);
void compact_types(void);
void pack_module(unsigned short);
void strip_debug(int);
void write_debug_directory(void);
void print_statistics(void);
void free_type_tables(void);
int update_image_checksum(const char *, unsigned long, unsigned char);

#endif

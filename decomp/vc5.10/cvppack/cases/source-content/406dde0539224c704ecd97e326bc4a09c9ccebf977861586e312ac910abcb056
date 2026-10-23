#ifndef VC510_CVPACK_H
#define VC510_CVPACK_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <recon.h>

/* The native driver is outside the reconstructed target graph. */
#ifdef RECON_NATIVE_TEST
#define main cvpack_main
#endif
int main(int argc, char **argv, char **environment);
typedef PIMAGE_NT_HEADERS (WINAPI *CheckSumMappedFileProc)(void *, DWORD, DWORD *, DWORD *);
int initialize_messages(const char *message_file, const char *program, int use_resources);
void parse_options(int argc, char **argv);
void files_initialize(int pages, int files, int page_shift, int file_pages, int file_shift, int mapped);
void print_banner(void);
void select_input_name(char *name);
void initialize_type_tables(void);
void read_image_debug(void);
void initialize_module_pack(void);
void pack_module(unsigned int module);
void write_debug_directory(void);
void free_type_tables(void);
void print_statistics(void);
void flush_packed_file(void);
unsigned int get_file_size(int file);
void *map_file_region(int file, unsigned int offset, unsigned int size);
void close_file(int file, int release);
void warning(unsigned int message, const char *first, const char *second);
void fatal_error(unsigned int message, const char *first, const char *second);
char *message_text(unsigned int message);

extern int input_file, show_logo, banner_pending, checksum_required;
extern unsigned short module_count, statistics_enabled;
extern void *debug_directory, *line_data, *symbol_data, *type_data, *module_data;
#endif

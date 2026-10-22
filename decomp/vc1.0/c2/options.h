#ifndef OPTIONS_H
#define OPTIONS_H
#include "c2.h"
typedef struct Option {
    char *pattern;
    void *destination;
    unsigned char result, action;
    unsigned short reserved;
} Option;
typedef struct Suboption { int letter, action; int *destination; } Suboption;
extern Option compiler_options[];
extern char *source_filename, *assembly_filename, *debug_filename, *listing_filename;
extern char *object_filename, *unknown_option, *assembly_options, *error_filename, *language_options;
extern int assembly_output, assembly_code, listing_output, assembly_source;
extern int assembly_cleanup_pending;
extern int byte_block_second, byte_block_first, identifier_limit;
char *next_argument();
int __fastcall match_option(Option *options, char *argument, char *(__cdecl *next)(void), int copy);
void __cdecl compiler_report(unsigned code, ...);
void __cdecl compiler_error(unsigned code, ...);
void __fastcall consume_string(char *string);
#endif

#ifndef COFF_OUTPUT_H
#define COFF_OUTPUT_H
#include "c2.h"
#include "segments.h"
typedef struct CoffConfiguration {
    unsigned short version, machine;
    unsigned flags;
    char *source;
    FILE *file;
    void *(__fastcall *allocate)(unsigned);
    void (__fastcall *error)(unsigned, char *, unsigned);
    unsigned (__cdecl *timestamp)(void);
    unsigned char debug;
    unsigned char reserved[3];
} CoffConfiguration;
extern FILE *object_file;
extern Segment *directive_segment;
unsigned coff_timestamp(void);
void *__fastcall coff_allocate(unsigned bytes);
void __fastcall coff_error(unsigned code, char *source, unsigned line);
int __fastcall coff_initialize(CoffConfiguration *configuration);
void write_module_directives(void);
void __fastcall coff_write_directive(char *prefix, char *text, int quote);
void __fastcall coff_write_comment(char *text);
extern int coff_precompiled_types;
extern char compiler_identification[];
void __fastcall coff_write_data(void *data, unsigned bytes);
void __fastcall coff_write_byte(unsigned char value);
#endif

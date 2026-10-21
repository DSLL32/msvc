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
    void (__fastcall *error)(unsigned, unsigned, void *);
    unsigned (__cdecl *timestamp)(void);
    unsigned char debug;
    unsigned char reserved[3];
} CoffConfiguration;
extern FILE *object_file;
extern Segment *directive_segment;
unsigned coff_timestamp(void);
void *__fastcall coff_allocate(unsigned bytes);
void __fastcall coff_error(unsigned code, unsigned detail, void *context);
int __fastcall coff_initialize(CoffConfiguration *configuration);
void write_module_directives(void);
#endif

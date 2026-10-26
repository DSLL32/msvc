#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H
#include "c2.h"
#include <recon.h>
void close_diagnostic_descriptors(void);
extern FILE *diagnostic_file;
extern char diagnostic_text[384];
typedef struct DiagnosticOffset {
    unsigned number;
    long position;
} DiagnosticOffset;
enum { DIAGNOSTIC_OFFSET_CAPACITY = 20 };
extern DiagnosticOffset diagnostic_offsets[DIAGNOSTIC_OFFSET_CAPACITY];
char *__fastcall diagnostic_format(unsigned number);
long __fastcall diagnostic_cached_offset(unsigned number);
char *__fastcall read_diagnostic_message(unsigned number, char *buffer, unsigned capacity);
#endif

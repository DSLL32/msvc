#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H
#include "frontend.h"
void report_syntax_error_arguments(unsigned code, char *arguments);
#include <recon.h>
typedef struct DiagnosticMessagePosition {
    int code;
    long offset;
} DiagnosticMessagePosition;
extern DiagnosticMessagePosition diagnostic_message_positions[20];
void format_text_arguments(char *destination, char *format, char *arguments);
extern FILE *diagnostic_message_file;
extern unsigned fatal_error_depth;
extern char diagnostic_message_buffer[256];
char *__stdcall parse_diagnostic_code(char *text, int *code);
void __stdcall remember_diagnostic_position(long offset, unsigned code);
char *__stdcall unquote_diagnostic_message(char *text);
long __stdcall diagnostic_message_offset(unsigned code);
char *__stdcall read_diagnostic_message(unsigned code, char *buffer, unsigned bytes);
#endif

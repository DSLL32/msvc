#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H
#include <stdarg.h>
#include <stdio.h>
#include "frontend.h"
#include <recon.h>
int __stdcall warning_setting(unsigned code);
extern FILE *diagnostic_message_file;
extern char diagnostic_message_buffer[256];
long __stdcall diagnostic_message_offset(unsigned code);
char *__stdcall read_diagnostic_message(unsigned code, char *buffer, unsigned bytes);
char *__stdcall diagnostic_format(unsigned code);
void emit_diagnostic(unsigned category, unsigned code, char *format, va_list arguments);
#endif

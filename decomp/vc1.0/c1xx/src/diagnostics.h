#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H
#include <stdarg.h>
#include "frontend.h"
#include <recon.h>
int __stdcall warning_setting(unsigned code);
char *__stdcall diagnostic_format(unsigned code);
void emit_diagnostic(unsigned category, unsigned code, char *format, va_list arguments);
#endif

#ifndef RC_FORMAT_H
#define RC_FORMAT_H
#include <stdio.h>
#include <stdarg.h>
#include <recon.h>
void __cdecl write_char(int, FILE *, int *);
void __cdecl write_multi_char(int, int, FILE *, int *);
void __cdecl write_string(char *, int, FILE *, int *);
int __cdecl get_int_arg(va_list *);
short __cdecl get_short_arg(va_list *);
#endif

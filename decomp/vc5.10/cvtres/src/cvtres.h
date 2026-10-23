#ifndef CVTRES_H
#define CVTRES_H
#include <windows.h>
#include <stdio.h>
#include <recon.h>

extern unsigned short target_machine;
extern unsigned short resource_machine_kind;
extern char *input_name, *output_name;
extern int verbose, windows_ce, windows_ce_emulation, windows_ce_noconvert;
extern int writable_resources;

void __stdcall usage(int status);
char *__stdcall load_message(unsigned int id);
void error(unsigned short code, ...);
void warning(unsigned short code, ...);
long __stdcall seek_file(FILE *file, long offset, int origin);
int __stdcall convert_resources(FILE *input, FILE *output, unsigned long size,
                                int writable, unsigned long timestamp);
#endif

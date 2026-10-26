#ifndef RCDLL_RESOURCE_OUTPUT_H
#define RCDLL_RESOURCE_OUTPUT_H
#include <stdio.h>
#include "rcdll.h"
#include <recon.h>
void __stdcall rc_write_apstudio_header(FILE *,DWORD,WORD);
char *__stdcall rc_format_resource_ordinal(WORD,char *,int);
unsigned __stdcall rc_write_apstudio_string(FILE *,const WCHAR *);
void __stdcall rc_write_apstudio_symbol(const WCHAR *,const WCHAR *,const void *,WORD,BYTE);
#endif

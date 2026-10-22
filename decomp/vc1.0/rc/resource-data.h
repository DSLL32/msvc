#ifndef RC_RESOURCE_DATA_H
#define RC_RESOURCE_DATA_H
#include "resource.h"
extern unsigned char *resource_data;
extern unsigned int resource_data_size;
extern FILE *resource_data_file;
void __stdcall resource_data_error(char *, char *);
void __stdcall put_byte(BYTE);
void __stdcall put_dword(DWORD);
void __stdcall put_wide_string(WCHAR *);
void __stdcall concatenate_wide_string(WCHAR *);
void __stdcall put_bytes(void *, WORD);
void __cdecl align_resource_data(void);
FILE * __stdcall resource_stream(FILE *);
BYTE * __cdecl resource_data_end(void);
WORD __cdecl resource_data_length(void);
#endif

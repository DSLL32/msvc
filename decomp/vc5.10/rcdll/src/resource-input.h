#ifndef RCDLL_RESOURCE_INPUT_H
#define RCDLL_RESOURCE_INPUT_H
#include <stdio.h>
#include "rcdll.h"
#include <recon.h>
extern DWORD rc_resource_status,rc_resource_file_index,rc_resource_line;
extern FILE *rc_resource_input_file;
extern WCHAR rc_resource_character;
unsigned __cdecl rc_next_resource_character(void);
unsigned __cdecl rc_read_resource_character(void);
#endif

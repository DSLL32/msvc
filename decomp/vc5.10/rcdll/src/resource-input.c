#include "command.h"
#include "resource-input.h"
#include <recon.h>

/* Distinct DWORD stores at original RVAs 25d3c, 27f78, 27f7c and 27f84.
 * The first field's role beyond initialization remains unobserved. */
DWORD rc_resource_status,rc_resource_file_index,rc_resource_line;
FILE *rc_resource_input_file;

int __stdcall rc_initialize_input(FILE *file)
{
    rc_resource_status=0;
    rc_resource_file_index=0;
    rc_resource_line=1;
    rc_resource_input_file=file;
    rc_next_resource_character();
    return 1;
}

/* Separate WORD at original RVA27f8c, filled by the underlying reader. */
WCHAR rc_resource_character;

unsigned __cdecl rc_next_resource_character(void)
{
    if(rc_read_resource_character()!=127 && rc_resource_character==L';'){
        while(rc_read_resource_character()!=127){
            if(rc_resource_character==L'\n')break;
        }
    }
    return rc_resource_character;
}

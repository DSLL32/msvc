#include <windows.h>
#include "omf.h"
#include <recon.h>

/* Actual distinct name and kind tables: original47ceb0..47eeb0 and
 * 476240..478240. The checksum BYTE at47eeb0 is a separate following owner. */
char *omf_names[2048];
DWORD omf_name_kinds[2048];
void __stdcall read_omf_lnames(DWORD kind)
{
    int index;
    char *name;
    while(omf_record_length>1) {
        omf_name_count=(WORD)(omf_name_count+1);
        if((SHORT)omf_name_count>=2048)omf_error("Too many names in LNAMES record");
        index=(SHORT)omf_name_count;
        name=read_omf_string(0);
        omf_names[index]=name;
        omf_name_kinds[index]=kind;
    }
}

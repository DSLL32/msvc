#include <string.h>
#include <stdio.h>
#include "command.h"
#include "resource-output.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

/* Encountered APSTUDIO scratch streams. These exact locations lie in the
 * original loader-zeroed .data tail; filename storage uses SDK MAX_PATH.
 * Only the false APSTUDIO branch has been executed by the current case. */
char rc_apstudio_path1[MAX_PATH],rc_apstudio_path2[MAX_PATH];
char rc_apstudio_path3[MAX_PATH],rc_apstudio_path4[MAX_PATH];
FILE *rc_apstudio_stream1,*rc_apstudio_stream2;
FILE *rc_apstudio_stream3,*rc_apstudio_stream4;
DWORD rc_apstudio_size1,rc_apstudio_size2,rc_apstudio_size3,rc_apstudio_size4;
DWORD rc_apstudio_header_size;

int __stdcall rc_initialize_resource(void)
{
    char *temporary;
    if(rc_apstudio){
#ifdef RECON_NATIVE_TEST
        /* Temp-file services and APSTUDIO header contents are not observed. */
        __asm int 3
#endif
        temporary=_tempnam(NULL,"RCX1");
        if(!temporary)temporary=tmpnam(NULL);
        strcpy(rc_apstudio_path1,temporary);
        temporary=_tempnam(NULL,"RCX2");
        if(!temporary)temporary=tmpnam(NULL);
        strcpy(rc_apstudio_path2,temporary);
        temporary=_tempnam(NULL,"RCX3");
        if(!temporary)temporary=tmpnam(NULL);
        strcpy(rc_apstudio_path3,temporary);
        temporary=_tempnam(NULL,"RCX4");
        if(!temporary)temporary=tmpnam(NULL);
        strcpy(rc_apstudio_path4,temporary);
        rc_apstudio_stream2=fopen(rc_apstudio_path2,"w+b");
        if(!rc_apstudio_stream2)return 0;
        rc_apstudio_stream1=fopen(rc_apstudio_path1,"w+b");
        if(!rc_apstudio_stream1)return 0;
        rc_apstudio_stream3=fopen(rc_apstudio_path3,"w+b");
        if(!rc_apstudio_stream3)return 0;
        rc_apstudio_stream4=fopen(rc_apstudio_path4,"w+b");
        if(!rc_apstudio_stream4)return 0;
        rc_apstudio_header_size=36;
        rc_write_apstudio_header(rc_apstudio_stream1,rc_apstudio_size1,200);
        rc_write_apstudio_header(rc_apstudio_stream2,rc_apstudio_size2,201);
        rc_write_apstudio_header(rc_apstudio_stream3,rc_apstudio_size3,202);
        rc_write_apstudio_header(rc_apstudio_stream4,rc_apstudio_size4,2);
    }
    return 1;
}

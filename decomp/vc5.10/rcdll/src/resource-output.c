#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "command.h"
#include "resource-output.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy,strlen)
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

/* APSTUDIO metadata has separate binary and textual side streams. The rich
 * case uses the early return; the native unobserved service path stays held. */
void __stdcall rc_write_resource_metadata(RcResource *resource,RcResourceType *type,int phase)
{
    WORD ordinal=0xffff,type_id;
    DWORD reserved=0;
    char text[36];
    if(!rc_apstudio)return;
#ifdef RECON_NATIVE_TEST
    __asm int 3
#endif
    if(!resource){
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,L"$",2);
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,&ordinal,2);
    }else if(phase){
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->size,4);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->data_offset,4);
        type_id=type->id;
        if(type_id){
            if(type_id==15)type_id=0;
            else if(type_id==18)type_id=5;
            rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&ordinal,2);
            rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&type_id,2);
        }else rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,type->name,(wcslen(type->name)+1)*2);
        if(resource->id){
            rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&ordinal,2);
            rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->id,2);
        }else rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,resource->name,(wcslen(resource->name)+1)*2);
        rc_apstudio_size4+=rc_align_output(rc_apstudio_stream4);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&reserved,4);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->flags,2);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->language,2);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->version,4);
        rc_apstudio_size4+=rc_write_bytes(rc_apstudio_stream4,&resource->characteristics,4);
    }else{
        type_id=type->id;
        if(!type_id)rc_apstudio_size3+=rc_write_apstudio_string(rc_apstudio_stream3,type->name);
        else{
            if(type_id==15)type_id=4;
            else if(type_id==18)type_id=5;
            rc_format_resource_ordinal(type_id,text,10);
            rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,text,strlen(text)+1);
        }
        if(!resource->id)rc_apstudio_size3+=rc_write_apstudio_string(rc_apstudio_stream3,resource->name);
        else{
            rc_format_resource_ordinal(resource->id,text,10);
            rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,text,strlen(text)+1);
        }
        rc_apstudio_size3+=rc_write_apstudio_string(rc_apstudio_stream3,resource->prefix);
        rc_apstudio_size3+=rc_write_apstudio_string(rc_apstudio_stream3,resource->prefix+248);
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,resource->prefix+508,2);
    }
}

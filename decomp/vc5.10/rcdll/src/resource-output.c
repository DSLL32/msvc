#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
 * APSTUDIO names are internal open/remove identities, not output data. */
char rc_apstudio_path1[MAX_PATH],rc_apstudio_path2[MAX_PATH];
char rc_apstudio_path3[MAX_PATH],rc_apstudio_path4[MAX_PATH];
FILE *rc_apstudio_stream1,*rc_apstudio_stream2;
FILE *rc_apstudio_stream3,*rc_apstudio_stream4;
DWORD rc_apstudio_size1,rc_apstudio_size2,rc_apstudio_size3,rc_apstudio_size4;
DWORD rc_apstudio_header_size;

char *__stdcall rc_format_resource_ordinal(WORD id,char *text,int radix)
{
    return _itoa(id==0xffff?-1:(int)id,text,radix);
}

unsigned __stdcall rc_write_apstudio_string(FILE *file,const WCHAR *text)
{
    char buffer[MAX_PATH];
    unsigned bytes;
    bytes=WideCharToMultiByte(rc_codepage,0,text,wcslen(text)+1,
                              buffer,MAX_PATH,NULL,NULL);
    return rc_write_bytes(file,buffer,bytes);
}

void __stdcall rc_write_apstudio_symbol(const WCHAR *name,const WCHAR *text,
                                        const void *unused,WORD line,BYTE kind)
{
    RcResource resource;
    RcResourceType type;
    if(!rc_apstudio)return;
    if(*name==L'$'&&*text){
        /* Only these three fields are consumed by the source-record writer. */
        resource.language=(WORD)rc_default_language;
        resource.id=0xffff;type.id=0xffff;
        rc_write_resource_source(&resource,&type,text);
    }else{
        rc_apstudio_size1+=rc_write_apstudio_string(rc_apstudio_stream1,name);
        rc_apstudio_size1+=rc_write_apstudio_string(rc_apstudio_stream1,text);
        rc_apstudio_size1+=rc_write_bytes(rc_apstudio_stream1,&line,2);
        rc_apstudio_size1+=rc_write_bytes(rc_apstudio_stream1,&kind,1);
    }
}

void __stdcall rc_write_apstudio_header(FILE *file,DWORD size,WORD id)
{
    DWORD value;
    WORD ordinal=0xffff,flags=0x30;
    rc_write_bytes(file,&size,4);
    rc_write_bytes(file,&rc_apstudio_header_size,4);
    rc_write_bytes(file,L"HWB",8);
    rc_write_bytes(file,&ordinal,2);
    rc_write_bytes(file,&id,2);
    rc_align_output(file);
    value=0;rc_write_bytes(file,&value,4);
    rc_write_bytes(file,&flags,2);
    rc_write_bytes(file,&rc_default_language,2);
    value=2;rc_write_bytes(file,&value,4);
    rc_write_bytes(file,&rc_resource_characteristics,4);
}

int __stdcall rc_initialize_resource(void)
{
    char *temporary;
    if(rc_apstudio){
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

int __stdcall rc_flush_resources(FILE *file)
{
    RcResource resource;
    RcResourceType *type;
    DWORD position;
    if(!rc_apstudio)return 1;
    if(file){
        rc_write_apstudio_symbol(L"",L"",L"",0,0);
        rc_seek(rc_apstudio_stream1,0,SEEK_SET);
        rc_write_bytes(rc_apstudio_stream1,&rc_apstudio_size1,4);
        rc_seek(rc_apstudio_stream2,0,SEEK_SET);
        rc_write_bytes(rc_apstudio_stream2,&rc_apstudio_size2,4);
        rc_write_resource_metadata(NULL,NULL,0);
        rc_seek(rc_apstudio_stream3,0,SEEK_SET);
        rc_write_bytes(rc_apstudio_stream3,&rc_apstudio_size3,4);
        type=rc_find_resource_type(L"HWB",0);
        resource.language=(WORD)rc_default_language;
        resource.version=rc_resource_version;
        resource.characteristics=rc_resource_characteristics;
        resource.flags=0x30;resource.name=NULL;resource.next=NULL;
        rc_seek(rc_apstudio_stream1,0,SEEK_SET);
        rc_align_output(file);
        resource.data_offset=rc_seek(file,0,SEEK_END)+rc_apstudio_header_size;
        resource.id=200;resource.size=rc_apstudio_size1;
        rc_write_resource_metadata(&resource,type,1);
        rc_copy_file(rc_apstudio_stream1,file);
        rc_seek(rc_apstudio_stream2,0,SEEK_SET);
        rc_align_output(file);
        resource.data_offset=rc_seek(file,0,SEEK_END)+rc_apstudio_header_size;
        resource.id=201;resource.size=rc_apstudio_size2;
        rc_write_resource_metadata(&resource,type,1);
        rc_copy_file(rc_apstudio_stream2,file);
        rc_seek(rc_apstudio_stream3,0,SEEK_SET);
        rc_align_output(file);
        resource.data_offset=rc_seek(file,0,SEEK_END)+rc_apstudio_header_size;
        resource.id=202;resource.size=rc_apstudio_size3;
        rc_write_resource_metadata(&resource,type,1);
        rc_copy_file(rc_apstudio_stream3,file);
        rc_align_output(file);
        position=rc_seek(file,0,SEEK_CUR);
        rc_seek(rc_apstudio_stream4,0,SEEK_SET);
        rc_write_bytes(rc_apstudio_stream4,&rc_apstudio_size4,4);
        rc_seek(rc_apstudio_stream4,0,SEEK_SET);
        rc_copy_file(rc_apstudio_stream4,file);
        rc_seek(file,rc_hwb_resource_offset,SEEK_SET);
        rc_write_bytes(file,&position,4);
        rc_seek(file,0,SEEK_END);
    }
    if(rc_apstudio_stream2){fclose(rc_apstudio_stream2);remove(rc_apstudio_path2);}
    if(rc_apstudio_stream3){fclose(rc_apstudio_stream3);remove(rc_apstudio_path3);}
    if(rc_apstudio_stream1){fclose(rc_apstudio_stream1);remove(rc_apstudio_path1);}
    if(rc_apstudio_stream4){fclose(rc_apstudio_stream4);remove(rc_apstudio_path4);}
    return 1;
}

void __stdcall rc_write_resource_source(RcResource *resource,RcResourceType *type,
                                       const WCHAR *filename)
{
    WORD ordinal=0xffff,type_id;
    if(!rc_apstudio)return;
    type_id=type->id;
    if(!type_id)
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,type->name,
                                        (wcslen(type->name)+1)*2);
    else {
        if(type_id==15)type_id=4;
        else if(type_id==18)type_id=5;
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,&ordinal,2);
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,&type_id,2);
    }
    if(!resource->id)
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,resource->name,
                                        (wcslen(resource->name)+1)*2);
    else {
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,&ordinal,2);
        rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,&resource->id,2);
    }
    rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,&resource->language,2);
    rc_apstudio_size2+=rc_write_bytes(rc_apstudio_stream2,filename,
                                    (wcslen(filename)+1)*2);
}

/* APSTUDIO metadata has separate binary and textual side streams. */
void __stdcall rc_write_resource_metadata(RcResource *resource,RcResourceType *type,int phase)
{
    WORD ordinal=0xffff,type_id;
    DWORD reserved=0;
    char text[36];
    if(!rc_apstudio)return;
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

WORD __stdcall rc_write_menu_item(RcMenuItem *item)
{
    int offset=rc_resource_buffer_used;
    WORD value;
    extern int rc_mac;
    value=rc_mac?(WORD)rc_mac_word(item->flags):item->flags;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    if(!(item->flags&0x10)){
        value=rc_mac?(WORD)rc_mac_word(item->id):item->id;
        *(WORD *)rc_reserve_resource_bytes(2)=value;
    }
    rc_write_resource_string(item->caption,1);
    return (WORD)offset;
}

void __stdcall rc_write_resource_string(const WCHAR *text,int flags)
{
    WORD value;
    extern int rc_mac;
    if(rc_mac)rc_write_mac_string(text,flags,0);
    else{
        do{
            value=rc_mac?(WORD)rc_mac_word(*text):*text;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
        }while(*text++);
    }
}

void __stdcall rc_write_apstudio_name(const WCHAR *text)
{
    WORD ordinal=0xffff;
    if(!rc_apstudio)return;
    if(!text){
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,L"$",2);
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,&ordinal,2);
    }else{
        rc_apstudio_size3+=rc_write_apstudio_string(rc_apstudio_stream3,text);
        rc_apstudio_size3+=rc_write_bytes(rc_apstudio_stream3,text+509,2);
    }
}

void __stdcall rc_mark_last_menu_item(WORD offset)
{
    unsigned position=offset;
    extern int rc_mac;
    if(rc_mac)++position;
#ifdef RECON_NATIVE_TEST
    /* Only already-written menu bytes belong to the supported live prefix. */
    if(!rc_resource_buffer||rc_resource_buffer_used<0||
       rc_resource_buffer_used>rc_resource_buffer_size||
       position>=(unsigned)rc_resource_buffer_used){ __asm int 3 }
#endif
    rc_resource_buffer[position]|=0x80;
}

WORD __stdcall rc_write_extended_menu_item(RcExtendedMenuItem *item)
{
    DWORD value;
    WORD flags,offset;
    extern int rc_mac;
    value=rc_mac?rc_mac_long(item->type):item->type;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    value=rc_mac?rc_mac_long(item->state):item->state;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    value=rc_mac?rc_mac_long(item->id):item->id;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    offset=(WORD)rc_resource_buffer_used;
    flags=rc_mac?(WORD)rc_mac_word(item->flags):item->flags;
    *(WORD *)rc_reserve_resource_bytes(2)=flags;
    rc_write_resource_string(item->caption,1);
    rc_pad_resource_buffer();
    if(item->flags&1){
        value=rc_mac?rc_mac_long(item->help):item->help;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    return offset;
}

void __stdcall rc_patch_control_data_size(WORD size,int enabled)
{
    if(enabled){
#ifdef RECON_NATIVE_TEST
        if(!rc_resource_buffer||rc_resource_buffer_used<2||
           rc_resource_buffer_used>rc_resource_buffer_size||
           rc_control_data_offset>(unsigned)rc_resource_buffer_used-2){ __asm int 3 }
#endif
        *(WORD *)(rc_resource_buffer+rc_control_data_offset)=size;
    }
}

/* RVA 25584 is a separate DWORD in the original loader-zeroed data tail.
 * Each accelerator records its own offset before any bytes are appended. */
DWORD rc_accelerator_offset;

void __stdcall rc_append_resource_bytes(const void *bytes,WORD count)
{
    const BYTE *source=bytes;
    while(count--)*(BYTE *)rc_reserve_resource_bytes(1)=*source++;
}

WORD __cdecl rc_resource_buffer_position(void)
{
    return (WORD)rc_resource_buffer_used;
}

WORD __stdcall rc_resource_buffer_word(DWORD offset)
{
#ifdef RECON_NATIVE_TEST
    if(!rc_resource_buffer||rc_resource_buffer_used<2||
       rc_resource_buffer_used>rc_resource_buffer_size||(offset&1)||
       offset>(DWORD)(rc_resource_buffer_used-2)){ __asm int 3 }
#endif
    return *(WORD *)(rc_resource_buffer+offset);
}

void __stdcall rc_set_resource_buffer_word(DWORD offset,WORD value)
{
#ifdef RECON_NATIVE_TEST
    if(!rc_resource_buffer||rc_resource_buffer_used<2||
       rc_resource_buffer_used>rc_resource_buffer_size||(offset&1)||
       offset>(DWORD)(rc_resource_buffer_used-2)){ __asm int 3 }
#endif
    *(WORD *)(rc_resource_buffer+offset)=value;
}

void __cdecl rc_begin_accelerator(void)
{
    rc_accelerator_offset=rc_resource_buffer_used;
}

void __cdecl rc_mark_last_accelerator(void)
{
    DWORD position=rc_accelerator_offset;
    extern int rc_mac;
#ifdef RECON_NATIVE_TEST
    /* The empty-table and unwritten-prefix cases have no supported byte. */
    if(!rc_resource_buffer||rc_resource_buffer_used<0||
       rc_resource_buffer_used>rc_resource_buffer_size||
       position>=(DWORD)rc_resource_buffer_used||
       (rc_mac&&position+1>=(DWORD)rc_resource_buffer_used)){ __asm int 3 }
#endif
    if(rc_mac)++position;
    rc_resource_buffer[position]|=0x80;
}

void __stdcall rc_write_resource(RcResourceType *type,RcResource *resource)
{
    extern int rc_mac;
    extern FILE *rc_output_file;
    if(!rc_mac)rc_align_output(rc_output_file);
    rc_append_resource(type,resource,rc_resource_buffer,rc_resource_buffer_used,-1);
}

unsigned __stdcall rc_align_output(FILE *file)
{
    DWORD zero=0;
    unsigned padding=(unsigned)rc_seek(file,0,SEEK_CUR)&3;
    if(!padding)return 0;
    padding=4-padding;
    rc_write_bytes(file,&zero,padding);
    return padding;
}

void __stdcall rc_append_resource(RcResourceType *type,RcResource *resource,
                                  const void *data,int bytes,int file_bytes)
{
    RcResource *last=type->resources;
    if(last){
        while(last->next)last=last->next;
        last->next=resource;
    }else type->resources=resource;
    rc_write_resource_record(type,resource,data,bytes,file_bytes);
    ++type->count;
    ++rc_resource_count;
    rc_write_resource_metadata(resource,type,1);
}

/* The actual 23-pointer verbose label table. Its DIALOGEX/DLGINCLUDE order
 * and NULL holes are retained; these are display labels, not type IDs. */
static const WCHAR *const rc_resource_type_labels[23]={
    NULL,L"CURSOR",L"BITMAP",L"ICON",L"MENU",L"DIALOG",L"STRING",
    L"FONTDIR",L"FONT",L"ACCELERATOR",L"RCDATA",L"MESSAGETABLE",
    L"GROUP_CURSOR",NULL,L"GROUP_ICON",NULL,L"VERSION",L"DIALOGEX",
    L"DLGINCLUDE",L"PLUGPLAY",L"VXD",L"ANICURSOR",L"ANIICON"
};

void __stdcall rc_write_resource_record(RcResourceType *type,RcResource *resource,
                                        const void *data,int bytes,int file_bytes)
{
    DWORD header_size=24,name_bytes=0,zero=0,converted;
    const WCHAR *type_name;
    extern int rc_verbose,rc_mac;
    extern FILE *rc_output_file;
    if(!resource->size)resource->size=rc_resource_data_size();
    if(!type->id){
        header_size+=(wcslen(type->name)+1)*2;
        name_bytes=(wcslen(type->name)+1)*2;
    }else header_size+=4;
    if(!resource->id){
        header_size+=(wcslen(resource->name)+1)*2;
        name_bytes+=(wcslen(resource->name)+1)*2;
    }else header_size+=4;
    if(name_bytes&3)header_size+=2;
    if(rc_verbose){
#ifdef RECON_NATIVE_TEST
        /* No verbose formatting service contract has been encountered. */
        __asm int 3
#endif
        if(!type->id||type->id<=22){
            type_name=type->id?rc_resource_type_labels[type->id]:type->name;
            if(!resource->id)
                wsprintfA(rc_error_text,"\nWriting %ws:%ws,\tlang:0x%x,\tsize %d",
                          type_name,resource->name,resource->language,resource->size);
            else wsprintfA(rc_error_text,"\nWriting %ws:%d,\tlang:0x%x,\tsize %d",
                           type_name,resource->id,resource->language,resource->size);
        }else{
            if(!resource->id)
                wsprintfA(rc_error_text,"\nWriting %d:%ws,\tlang:0x%x,\tsize %d",
                          type->id,resource->name,resource->language,resource->size);
            else wsprintfA(rc_error_text,"\nWriting %d:%d,\tlang:0x%x,\tsize %d",
                           type->id,resource->id,resource->language,resource->size);
        }
        printf(rc_error_text);
    }
    if(rc_mac){
        resource->data_offset=(DWORD)rc_seek(rc_output_file,0,SEEK_CUR)-256;
        converted=rc_mac_long(resource->size);
        rc_write_bytes(rc_output_file,&converted,4);
    }else{
        rc_write_bytes(rc_output_file,&resource->size,4);
        rc_write_bytes(rc_output_file,&header_size,4);
        rc_write_resource_identifier(type->id,type->name);
        rc_write_resource_identifier(resource->id,resource->name);
        rc_align_output(rc_output_file);
        rc_write_bytes(rc_output_file,&zero,4);
        rc_write_bytes(rc_output_file,&resource->flags,2);
        rc_write_bytes(rc_output_file,&resource->language,2);
        rc_write_bytes(rc_output_file,&resource->version,4);
        rc_write_bytes(rc_output_file,&resource->characteristics,4);
        resource->data_offset=(DWORD)rc_seek(rc_output_file,0,SEEK_CUR);
    }
    rc_write_resource_payload(rc_output_file,data,bytes,file_bytes);
}

DWORD __cdecl rc_resource_data_size(void)
{
    long position,end;
    DWORD size;
    if(!rc_resource_data_file)return (DWORD)rc_resource_buffer_used;
    position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
    end=rc_seek(rc_resource_data_file,0,SEEK_END);
    size=(DWORD)end+(DWORD)rc_resource_buffer_used-(DWORD)position;
    rc_seek(rc_resource_data_file,position,SEEK_SET);
    return size;
}

unsigned __stdcall rc_write_resource_identifier(WORD id,const WCHAR *name)
{
    WORD ordinal=0xffff;
    extern FILE *rc_output_file;
    if(id){
        rc_write_bytes(rc_output_file,&ordinal,2);
        return rc_write_bytes(rc_output_file,&id,2);
    }
    return rc_write_bytes(rc_output_file,name,(wcslen(name)+1)*2);
}

int __stdcall rc_write_resource_payload(FILE *file,const void *data,int bytes,int file_bytes)
{
    if(bytes>0)rc_write_bytes(file,data,bytes);
    if(rc_resource_data_file){
        if(file_bytes==-1){
            rc_copy_file(rc_resource_data_file,file);
            fclose(rc_resource_data_file);
        }else rc_copy_file_bytes(rc_resource_data_file,file,(unsigned)file_bytes);
    }
    return bytes;
}

/* RVA 28fdc is assigned from the live output cursor before count consumers. */
DWORD rc_dialog_count_offset;
extern int rc_mac;

void __stdcall rc_write_dialog_header(RcDialog *dialog,int extended)
{
    WORD word;
    DWORD value;
    if(extended){
        word=rc_mac?(WORD)rc_mac_word(1):1;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        word=rc_mac?(WORD)rc_mac_word(0xffff):0xffff;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        value=rc_mac?rc_mac_long(dialog->help_id):dialog->help_id;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        value=rc_mac?rc_mac_long(dialog->exstyle):dialog->exstyle;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    value=rc_mac?rc_mac_long(dialog->style):dialog->style;
    *(DWORD *)rc_reserve_resource_bytes(4)=value;
    if(!extended){
        value=rc_mac?rc_mac_long(dialog->exstyle):dialog->exstyle;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    rc_dialog_count_offset=rc_resource_buffer_used;
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(dialog->x):dialog->x;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(dialog->y):dialog->y;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(dialog->width):dialog->width;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(dialog->height):dialog->height;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    if(dialog->menu_is_ordinal){
        rc_write_ordinal_marker();
        word=rc_mac?(WORD)rc_mac_word(rc_decimal_resource_id(dialog->menu)):
            (WORD)rc_decimal_resource_id(dialog->menu);
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }else rc_write_resource_string(dialog->menu,0);
    if(dialog->class_is_ordinal){
        rc_write_ordinal_marker();
        word=rc_mac?(WORD)rc_mac_word(rc_decimal_resource_id(dialog->class_name)):
            (WORD)rc_decimal_resource_id(dialog->class_name);
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }else rc_write_resource_string(dialog->class_name,0);
    rc_write_resource_string(dialog->caption,1);
    if(dialog->point_size){
        word=rc_mac?(WORD)rc_mac_word(dialog->point_size):dialog->point_size;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        if(extended){
            word=rc_mac?(WORD)rc_mac_word(dialog->weight):dialog->weight;
            *(WORD *)rc_reserve_resource_bytes(2)=word;
            *(BYTE *)rc_reserve_resource_bytes(1)=dialog->italic;
            *(BYTE *)rc_reserve_resource_bytes(1)=dialog->charset;
        }
        rc_write_resource_string(dialog->font,0);
    }
}

void __cdecl rc_write_ordinal_marker(void)
{
    WORD value=rc_mac?(WORD)rc_mac_word(0xffff):0xffff;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
}

/* RVA 28ffc: output-relative location of an extended control's size WORD. */
DWORD rc_control_data_offset;

void __stdcall rc_write_control(RcControl *control,int extended)
{
    DWORD value;
    WORD word;
    rc_pad_resource_buffer();
    if(extended){
        value=rc_mac?rc_mac_long(control->help_id):control->help_id;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        value=rc_mac?rc_mac_long(control->exstyle):control->exstyle;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        value=rc_mac?rc_mac_long(control->style):control->style;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }else {
        value=rc_mac?rc_mac_long(control->style):control->style;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        value=rc_mac?rc_mac_long(control->exstyle):control->exstyle;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }
    word=rc_mac?(WORD)rc_mac_word(control->x):control->x;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(control->y):control->y;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(control->width):control->width;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(control->height):control->height;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    if(extended){
        value=rc_mac?rc_mac_long(control->id):control->id;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
    }else {
        word=rc_mac?(WORD)rc_mac_word((WORD)control->id):(WORD)control->id;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }
    if(control->class_name[0]==0xffff){
        word=rc_mac?(WORD)rc_mac_word(0xffff):0xffff;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        word=rc_mac?(WORD)rc_mac_word(control->class_name[1]):control->class_name[1];
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }else rc_write_resource_string(control->class_name,0);
    if(control->text_is_ordinal){
        rc_write_ordinal_marker();
        word=rc_mac?(WORD)rc_mac_word(rc_decimal_resource_id(control->text)):
            (WORD)rc_decimal_resource_id(control->text);
        *(WORD *)rc_reserve_resource_bytes(2)=word;
    }else rc_write_resource_string(control->text,1);
    if(extended)rc_control_data_offset=rc_resource_buffer_used;
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    rc_increment_dialog_count();
}

void __cdecl rc_pad_resource_buffer(void)
{
    WORD count=(WORD)(rc_resource_buffer_used%4);
    /* The original adds the remainder, rather than four minus it. Its current
     * WCHAR records have even lengths, for which this pads to a DWORD. */
    while(count){
        *(BYTE *)rc_reserve_resource_bytes(1)=0;
        --count;
    }
}

void __cdecl rc_increment_dialog_count(void)
{
#ifdef RECON_NATIVE_TEST
    if(!rc_resource_buffer||rc_resource_buffer_used<2||
       rc_resource_buffer_used>rc_resource_buffer_size||
       (rc_dialog_count_offset&1)||
       rc_dialog_count_offset>(DWORD)(rc_resource_buffer_used-2)){ __asm int 3 }
#endif
    ++*(WORD *)(rc_resource_buffer+rc_dialog_count_offset);
}

void __cdecl rc_convert_dialog_count(void)
{
    WORD *count;
    if(!rc_resource_buffer||rc_resource_buffer_used<2||
       rc_resource_buffer_used>rc_resource_buffer_size||
       (rc_dialog_count_offset&1)||
       rc_dialog_count_offset>(DWORD)(rc_resource_buffer_used-2)){ __asm int 3 }
    count=(WORD *)(rc_resource_buffer+rc_dialog_count_offset);
    *count=(WORD)rc_mac_word(*count);
}

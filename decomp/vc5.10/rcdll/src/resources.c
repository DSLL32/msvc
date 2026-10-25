#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "command.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(memcpy)
#endif

extern FILE *rc_output_file;
extern int rc_mac,rc_verbose;
extern unsigned rc_default_language;
extern char rc_input_path[260];
void *rc_string_tables;
extern DWORD rc_resource_recording;

/* New fields are in the verified loader-zero .data tail. The default language
 * is the existing command DWORD at 14db8; this parser accesses its low WORD. */
WCHAR rc_resource_name[257];
WORD rc_font_resource_count,rc_next_resource_id;
DWORD rc_resource_version,rc_resource_characteristics;
int rc_explicit_font_directory;
void *rc_dialog_template;
DWORD rc_hwb_resource_offset;
int rc_resource_buffer_size,rc_resource_buffer_used;
/* DWORD/PTR at RVA28fd8, separate from the preprocessed input FILE.
 * Resource read/seek paths use it as a FILE; this helper clears it. */
FILE *rc_resource_data_file;
BYTE *rc_resource_buffer;

/* These records describe only the actual fields constructed at the two
 * indirect call sites. The callback remains opaque at the public RC boundary;
 * non-NULL callbacks are unavailable in the supported native cases. */
typedef struct RcResourceRequest {
    DWORD size;
    WCHAR *type_name;
    WORD type_id,unknown10;
    WCHAR *name;
    WORD id,flags,language,unknown22;
    DWORD version,characteristics;
} RcResourceRequest;
typedef struct RcResourceResponse {
    HANDLE heap;
    HWND window;
    RC_DIAGNOSTIC diagnostic;
    DWORD line;
    char *format;
} RcResourceResponse;
typedef int (WINAPI *RcResourceCallback)(void *,void *,void *);

int __stdcall rc_parse_resources(void)
{
    DWORD zero=0,ordinal=0xffff,header_size=32;
    RcResource *resource;
    RcResourceType *type;
    WORD value,saved_type;
    unsigned id;
    char *name;
    int nesting,count;
    RcResourceRequest request;
    RcResourceResponse response;
    char format[512];

    rc_next_resource_id=0;
    rc_font_resource_count=0;
    rc_initialize_resource_types();
    if(!rc_mac){
        rc_write_bytes(rc_output_file,&zero,4);
        rc_write_bytes(rc_output_file,&header_size,4);
        rc_write_bytes(rc_output_file,&ordinal,2);
        rc_write_bytes(rc_output_file,&zero,2);
        rc_write_bytes(rc_output_file,&ordinal,2);
        rc_write_bytes(rc_output_file,&zero,2);
        rc_write_bytes(rc_output_file,&zero,4);
        rc_write_bytes(rc_output_file,&zero,2);
        rc_write_bytes(rc_output_file,&zero,2);
        rc_write_bytes(rc_output_file,&zero,4);
        rc_write_bytes(rc_output_file,&zero,4);
    }
    rc_initialize_resource_buffer();
    if(rc_apstudio){
        name=rc_input_path;
        rc_reset_resource_buffer();
        resource=rc_allocate(sizeof(RcResource));
        resource->language=(WORD)rc_default_language;
        resource->version=rc_resource_version;
        resource->characteristics=rc_resource_characteristics;
        resource->size=4;
        resource->flags=0;
        resource->name=NULL;
        resource->id=1;
        *(DWORD *)rc_reserve_resource_bytes(4)=rc_mac?rc_mac_long(0):0;
        while(*name){
            *(BYTE *)rc_reserve_resource_bytes(1)=*name++;
            ++resource->size;
        }
        *(BYTE *)rc_reserve_resource_bytes(1)=0;
        ++resource->size;
        type=rc_find_resource_type(L"HWB",0);
        rc_write_resource(type,resource);
        rc_hwb_resource_offset=resource->data_offset;
    }
    rc_resource_token.text[0]=0;
    rc_resource_token.unknown1038=0;
    if(!rc_read_resource_name())goto finish;
    for(;;){
        if(!wcscmp(rc_resource_name,L"LANGUAGE")){
            *(WORD *)&rc_default_language=(WORD)rc_read_resource_language();
            goto next_resource;
        }
        if(!wcscmp(rc_resource_name,L"VERSION")){
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2139);
            rc_resource_version=rc_resource_token.unknown0;
            goto next_resource;
        }
        if(!wcscmp(rc_resource_name,L"CHARACTERISTICS")){
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2140);
            rc_resource_characteristics=rc_resource_token.unknown0;
            goto next_resource;
        }
        if(rc_verbose)printf(".");
        resource=rc_allocate(sizeof(RcResource));
        resource->language=(WORD)rc_default_language;
        resource->version=rc_resource_version;
        resource->characteristics=rc_resource_characteristics;
        if(rc_resource_token.text[0])
            memcpy(resource->prefix,(BYTE *)&rc_resource_token+
                   offsetof(RcResourceToken,text),sizeof(resource->prefix));
        else resource->prefix[0]=0;
        if(!rc_resource_token.value){
            if(wcslen(rc_resource_name)>256){
                rc_format(rc_error_text,2048,rc_message(4206),
                          rc_resource_filename,rc_resource_token.line);
                rc_fatal(rc_error_text);
                rc_resource_name[256]=0;
                rc_resource_token.value=255;
            }
            resource->name=rc_duplicate_resource_name(rc_resource_name);
        }else resource->id=rc_resource_token.value;
        if(resource->name&&!wcscmp(resource->name,L"STRINGTABLE"))
            goto string_table;
        if(!rc_read_resource_name())break;
        if(!rc_resource_token.value){
            if(wcslen(rc_resource_name)>256){
                rc_format(rc_error_text,2048,rc_message(4207),
                          rc_resource_filename,rc_resource_token.line);
                rc_fatal(rc_error_text);
                rc_resource_name[256]=0;
                rc_resource_token.value=255;
            }
            if(!wcscmp(rc_resource_name,L"STRINGTABLE")){
                rc_resource_error(2255);
                resource->name=rc_duplicate_resource_name(rc_resource_name);
                goto string_table;
            }
            type=rc_find_resource_type(rc_resource_name,0);
        }else type=rc_find_resource_type(NULL,rc_resource_token.value);
        if(!type)return rc_resource_status==0;
        rc_scan_resource_token(0);
        id=type->id;
        if(id==1||id==8||(id>=3&&id<=5)||id==15||id==17||id==18)
            resource->flags=0x1030;
        else if(id==12||id==14)resource->flags=0x1010;
        else resource->flags=0x30;
        while(rc_read_resource_attributes(resource));
        rc_write_resource_metadata(resource,type,0);
        goto dispatch;
string_table:
        rc_scan_resource_token(0);
        resource->flags=0x1030;
        while(rc_read_resource_attributes(resource));
        type=NULL;
dispatch:
        if(!type){
            if(resource->name&&!wcscmp(resource->name,L"STRINGTABLE")){
                if(!rc_parse_string_table(resource))break;
                goto next_resource;
            }
            rc_resource_error(2141);
            goto next_resource;
        }
        rc_reset_resource_buffer();
        resource->size=0;
        switch(type->id){
        case 1:case 3:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(!resource->size)break;
            if(rc_is_riff_resource(resource->size))goto riff;
            resource->size=rc_convert_icon_file(resource->size);
            goto write_resource;
        case 2:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(!resource->size)break;
            resource->size=rc_convert_bitmap_file();
            goto write_resource;
        case 4:
            value=rc_mac?(WORD)rc_mac_word(0):0;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            value=rc_mac?(WORD)rc_mac_word(0):0;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            rc_parse_menu_template(0,resource);
            goto write_resource;
        case 5:
            rc_dialog_template=rc_allocate(0x826);
            rc_parse_dialog(resource,rc_dialog_template,0);
            goto write_dialog;
        case 7:
            rc_write_resource_source(resource,type,rc_resource_name);
            rc_explicit_font_directory=1;
            /* FALL THROUGH */
        case 11:
            resource->size=rc_read_resource_file();
            if(resource->size)goto write_resource;
            break;
        case 8:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(resource->name)rc_resource_error(2143);
            if(rc_parse_font_resource(resource)){
                ++rc_font_resource_count;
                goto write_resource;
            }
            break;
        case 9:
            rc_parse_accelerators(resource);
            goto write_resource;
        case 12:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(!resource->size)break;
            if(rc_is_riff_resource(resource->size))goto riff;
            if(rc_mac)rc_mac_group_cursor(type,resource);
            else rc_convert_group_resource(type,resource,3);
            break;
        case 14:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(!resource->size)break;
            if(rc_is_riff_resource(resource->size))goto riff;
            if(rc_mac)rc_mac_group_icon(type,resource);
            else rc_convert_group_resource(type,resource,1);
            break;
        case 15:
            value=rc_mac?(WORD)rc_mac_word(1):1;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            value=rc_mac?(WORD)rc_mac_word(4):4;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            rc_parse_menu(0,resource);
            type=rc_find_resource_type(L"MENU",0);
            goto write_resource;
        case 16:
            rc_parse_version_resource();
            goto write_resource;
        case 17:
            rc_parse_dialog_include(resource);
            goto write_resource;
        case 18:
            rc_dialog_template=rc_allocate(0x826);
            rc_parse_dialog(resource,rc_dialog_template,1);
            type=rc_find_resource_type(L"DIALOG",0);
write_dialog:
            rc_write_resource(type,resource);
            rc_release(rc_dialog_template);
            break;
        case 21:case 22:
riff:
            saved_type=type->id;
            if(saved_type==3||saved_type==14)type->id=22;
            else if(saved_type==1||saved_type==12)type->id=21;
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(resource->size){
                resource->size=rc_copy_riff_resource(resource->size);
                rc_write_resource(type,resource);
            }
            type->id=saved_type;
            break;
        case 241:
            rc_parse_toolbar_resource(resource);
            goto write_resource;
        default:
            if(rc_resource_token.kind!=13){
                resource->size=rc_read_resource_file();
                if(resource->size)
                    rc_write_resource_source(resource,type,rc_resource_name);
                goto write_resource;
            }
            rc_resource_recording=0;
            if(rc_resource){
#ifdef RECON_NATIVE_TEST
                /* Only NULL resource callbacks have a supported host contract. */
                __asm int 3
#endif
                request.type_name=type->name;
                request.type_id=type->id;
                request.name=resource->name;
                request.id=resource->id;
                request.flags=resource->flags;
                request.language=resource->language;
                request.version=resource->version;
                request.size=0;
                request.characteristics=resource->characteristics;
                rc_resource_recording=((RcResourceCallback)rc_resource)(&request,NULL,NULL);
            }
            if(!rc_resource_recording){
                rc_parse_raw_resource(resource);
                goto write_resource;
            }
            response.heap=rc_heap;
            response.window=rc_window;
            response.diagnostic=rc_diagnostic;
            response.line=rc_resource_token.line;
            wsprintfA(format,"%s(%%d) : %%s",rc_resource_filename);
            response.format=format;
            nesting=1;
            do{
                count=rc_resource_buffer_used;
                rc_scan_resource_token(0);
                if(rc_resource_token.kind==13)++nesting;
                else if(rc_resource_token.kind==14)--nesting;
            }while(nesting>0);
            rc_resource_recording=0;
            if(count>0&&!((RcResourceCallback)rc_resource)(&count,
                        rc_resource_buffer,&response))rc_emit("");
            rc_resource_buffer_size=count;
            rc_resource_buffer_used=count;
            resource->size=count;
write_resource:
            rc_write_resource(type,resource);
        }
        rc_write_resource_metadata(NULL,NULL,0);
next_resource:
        if(rc_resource_token.kind==127)break;
        rc_resource_token.text[0]=0;
        rc_resource_token.unknown1038=0;
        if(!rc_read_resource_name())break;
    }
finish:
    if(!rc_explicit_font_directory&&rc_font_resource_count)
        rc_flush_font_directory();
    if(rc_string_tables)rc_write_string_table(rc_string_tables);
    if(rc_mac)rc_write_mac_resource_file();
    rc_release_resource_buffer();
    if(rc_verbose)printf("\n");
    return rc_resource_status==0;
}

void __cdecl rc_initialize_resource_types(void)
{
    rc_find_resource_type(L"CURSOR",12);
    rc_find_resource_type(L"ICON",14);
    rc_find_resource_type(L"BITMAP",2);
    rc_find_resource_type(L"MENU",4);
    rc_find_resource_type(L"DIALOG",5);
    rc_find_resource_type(L"STRINGTABLE",6);
    rc_find_resource_type(L"FONTDIR",7);
    rc_find_resource_type(L"FONT",8);
    rc_find_resource_type(L"ACCELERATORS",9);
    rc_find_resource_type(L"RCDATA",10);
    rc_find_resource_type(L"MESSAGETABLE",11);
    rc_find_resource_type(L"VERSIONINFO",16);
    rc_find_resource_type(L"DLGINCLUDE",17);
    rc_find_resource_type(L"MENUEX",15);
    rc_find_resource_type(L"DIALOGEX",18);
    rc_find_resource_type(L"PLUGPLAY",19);
    rc_find_resource_type(L"VXD",20);
    rc_find_resource_type(L"DLGINIT",240);
    rc_find_resource_type(L"TOOLBAR",241);
    rc_find_resource_type(L"ANIICON",22);
    rc_find_resource_type(L"ANICURSOR",21);
    rc_find_resource_type(L"HTML",23);
}

/* Resource-type list head at RVA 25d44, independently loader-zero. Nodes are
 * the genuine 20-byte zeroed allocations, not image data. */
RcResourceType *rc_resource_types;

RcResourceType *__stdcall rc_find_resource_type(const WCHAR *name,unsigned id)
{
    RcResourceType *entry=rc_resource_types,*last=NULL;
    while(entry){
        if(name&&entry->name&&!wcscmp(name,entry->name))return entry;
        if(!name&&id&&entry->id==(WORD)id)return entry;
        if(!entry->next){last=entry;break;}
        entry=entry->next;
    }
    entry=rc_allocate(sizeof(RcResourceType));
    if(last)last->next=entry;
    else rc_resource_types=entry;
    name=rc_duplicate_resource_name(name);
    entry->unknown18=0;
    entry->unknown4=0;
    entry->name=(WCHAR *)name;
    entry->id=(WORD)id;
    return entry;
}

WCHAR *__stdcall rc_duplicate_resource_name(const WCHAR *name)
{
    WCHAR *copy=NULL;
    if(name){
        copy=rc_allocate((wcslen(name)+1)*sizeof(WCHAR));
        wcscpy(copy,name);
    }
    return copy;
}

void __cdecl rc_initialize_resource_buffer(void)
{
    rc_resource_buffer_size=128000;
    rc_resource_buffer=rc_allocate(rc_resource_buffer_size);
}

int __cdecl rc_read_resource_name(void)
{
    WCHAR *text;
    unsigned value,character;
    int not_decimal=0,remaining;
    if(!rc_collect_resource_name())return 0;
    _wcsupr(rc_resource_name);
    for(text=rc_resource_name;*text;++text)
        if(!iswctype(*text,4))not_decimal=1;
    if(rc_resource_name[0]==L'0'&&rc_resource_name[1]==L'X'){
        value=0;remaining=4;text=rc_resource_name+2;
        while(iswctype(*text,128)){
            character=*text++;
            value=(value<<4)+(character>=L'A'?character-55:character-L'0');
            if(!--remaining)break;
        }
        rc_resource_token.value=(WORD)value;
    }else if(not_decimal)rc_resource_token.value=0;
    else rc_resource_token.value=(WORD)rc_decimal_resource_id(rc_resource_name);
    return 1;
}

WCHAR *__cdecl rc_collect_resource_name(void)
{
    WCHAR *cursor;
    while(rc_resource_character!=0xe001&&rc_resource_character!=0xe000&&
          rc_resource_character<=32)rc_next_resource_character();
    if(rc_resource_character==127){
        rc_resource_token.kind=127;
        return NULL;
    }
    if(rc_resource_character==0xe001)rc_read_resource_marker(1,0xe001);
    rc_resource_token.text[0]=0;
    rc_resource_name[0]=rc_resource_character;
    cursor=rc_resource_name+1;
    while(rc_read_resource_character()!=127){
        if(rc_resource_character!=0xe001&&rc_resource_character!=0xe000&&
           rc_resource_character<=32)break;
#ifdef RECON_NATIVE_TEST
        /* The encountered name workspace has 257 WCHARs. Longer original
         * input/storage is unclaimed; reserve a defined terminating word. */
        if(cursor>=rc_resource_name+256)__asm int 3
#endif
        *cursor++=rc_resource_character;
    }
    *cursor=0;
    rc_next_resource_character();
    if(rc_resource_character==127)rc_resource_token.kind=127;
    if(rc_resource_character==0xe001){
        rc_read_resource_marker(1,0xe001);
        rc_resource_token.unknown1038=rc_resource_token.value;
    }
    return rc_resource_name;
}

WORD __cdecl rc_read_resource_language(void)
{
    DWORD primary;
    unsigned error;
    rc_scan_resource_token(0);
    if(rc_resource_token.kind!=20){error=2144;goto invalid;}
    if(rc_resource_token.unknown12){error=2145;goto invalid;}
    /* The original loads all four packed bytes, including kind, then the
     * actual caller consumes only the returned low WORD. */
    primary=rc_resource_token.packed_value;
    rc_scan_resource_token(0);
    if(rc_resource_token.kind!=15){error=2146;goto invalid;}
    rc_scan_resource_token(0);
    if(rc_resource_token.kind!=20){error=2147;goto invalid;}
    if(rc_resource_token.unknown12){error=2148;goto invalid;}
    return (WORD)(((DWORD)rc_resource_token.value<<10)|primary);
invalid:
    rc_resource_error(error);
    return 0;
}

unsigned __stdcall rc_decimal_resource_id(const WCHAR *text)
{
    unsigned value=0;
    while(*text){
        value=value*10+(signed char)*text-L'0';
        ++text;
    }
    return value;
}

int __stdcall rc_read_resource_attributes(RcResource *resource)
{
    if(rc_resource_token.kind==20)return 0;
    switch(rc_resource_token.value){
    case 247:
        rc_scan_resource_token(0);
        if(rc_resource_token.kind!=18)rc_resource_error(2136);
        rc_scan_resource_value(0x8000);
        rc_parse_resource_style(&resource->unknown8,1);
        break;
    case 0xfff0: resource->flags&=0xefef;break;
    case 0xfff1: resource->flags|=0x10;break;
    case 0xfff2: resource->flags|=0x1030;break;
    case 0xfff3: resource->flags&=0xffbf;break;
    case 0xfff4: resource->flags|=0x40;break;
    case 0xfff5: resource->flags|=0x20;break;
    case 0xfff6: resource->flags&=0xefdf;break;
    default:return 0;
    }
    rc_scan_resource_token(0);
    return 1;
}

void __cdecl rc_reset_resource_buffer(void)
{
    rc_resource_buffer_used=0;
    rc_resource_data_file=NULL;
}

void *__stdcall rc_reserve_resource_bytes(WORD bytes)
{
    BYTE *buffer,*result;
#ifdef RECON_NATIVE_TEST
    /* The live buffer is produced by the resource allocator. Invalid cursor
     * states have no proved ownership; do not form a pointer from them. */
    if(!rc_resource_buffer||rc_resource_buffer_size<0||
       rc_resource_buffer_used<0||rc_resource_buffer_used>rc_resource_buffer_size)
        __asm int 3
#endif
    if(rc_resource_buffer_used>rc_resource_buffer_size-bytes){
#ifdef RECON_NATIVE_TEST
        if(rc_resource_buffer_size>0x7ffefff7)__asm int 3
#endif
        buffer=HeapReAlloc(rc_heap,9,rc_resource_buffer-8,
                           rc_resource_buffer_size+65544);
        if(!buffer)rc_resource_data_fatal(2168);
        rc_resource_buffer_size+=65536;
        rc_resource_buffer=buffer+8;
    }
    result=rc_resource_buffer+rc_resource_buffer_used;
    rc_resource_buffer_used+=bytes;
    return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>
#include "command.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(memcpy,memset,strcmp,strcpy,strlen)
#endif

extern FILE *rc_output_file;
extern int rc_mac,rc_verbose;
extern unsigned rc_default_language;
extern char rc_input_path[260];
/* RVA28fd4, distinct from the block-list head at28fd0. Both are in the
 * original loader-zero tail; the first string-table call resets the head. */
RcResource *rc_last_string_resource;
RcStringBlock *rc_string_blocks;
extern DWORD rc_resource_recording;

/* New fields are in the verified loader-zero .data tail. The default language
 * is the existing command DWORD at 14db8; this parser accesses its low WORD. */
WCHAR rc_resource_name[257];
WORD rc_font_resource_count,rc_resource_count;
DWORD rc_resource_version,rc_resource_characteristics;
int rc_explicit_font_directory;
RcDialog *rc_dialog_template;
DWORD rc_hwb_resource_offset;
int rc_resource_buffer_size,rc_resource_buffer_used;
/* DWORD/PTR at RVA28fd8, separate from the preprocessed input FILE.
 * Resource read/seek paths use it as a FILE; this helper clears it. */
FILE *rc_resource_data_file;
BYTE *rc_resource_buffer;
/* The actual icon/cursor ID WORD at RVA15248 has pristine initialized value1.
 * It is distinct from the parser/appender resource count at RVA25a24. */
WORD rc_next_image_id=1;

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

    rc_resource_count=0;
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
            rc_dialog_template=rc_allocate(sizeof(*rc_dialog_template));
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
            else rc_convert_group_resource(type,resource,1);
            break;
        case 14:
            rc_write_resource_source(resource,type,rc_resource_name);
            resource->size=rc_read_resource_file();
            if(!resource->size)break;
            if(rc_is_riff_resource(resource->size))goto riff;
            if(rc_mac)rc_mac_group_icon(type,resource);
            else rc_convert_group_resource(type,resource,3);
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
            rc_dialog_template=rc_allocate(sizeof(*rc_dialog_template));
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
    if(rc_last_string_resource)rc_write_string_table(rc_last_string_resource);
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
    entry->count=0;
    entry->resources=NULL;
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

void __cdecl rc_release_resource_buffer(void)
{
    rc_resource_buffer_size=0;
    rc_release(rc_resource_buffer);
}

FILE *__stdcall rc_resource_file(FILE *file)
{
    if(file)rc_resource_data_file=file;
    return rc_resource_data_file;
}

int __stdcall rc_is_riff_resource(long unused_size)
{
    DWORD header[2];
    unsigned received=rc_read_bytes(rc_resource_data_file,header,8);
    rc_seek(rc_resource_data_file,0,SEEK_SET);
    return received==8&&header[0]==0x46464952;
}

void __stdcall rc_convert_group_resource(RcResourceType *type,RcResource *resource,
                                        unsigned kind)
{
    RcImageDirectory directory;
    RcImageFileEntry entry;
    RcBitmapInfo bitmap;
    RcImageGroupEntry group;
    RcResource *image;
    RcResourceType *image_type;
    WORD hotspot[2],id;
    unsigned index,prefix;
    long position;
#ifdef RECON_NATIVE_TEST
    if(rc_read_bytes(rc_resource_data_file,&directory,6)!=6){ __asm int 3 }
#else
    rc_read_bytes(rc_resource_data_file,&directory,6);
#endif
    if(directory.reserved||(directory.kind!=1&&directory.kind!=2))
        rc_resource_file_fatal(2175,rc_resource_name);
    rc_append_resource_bytes(&directory,6);
    for(index=0;index<directory.count;++index){
#ifdef RECON_NATIVE_TEST
        if(rc_read_bytes(rc_resource_data_file,&entry,16)!=16){ __asm int 3 }
#else
        rc_read_bytes(rc_resource_data_file,&entry,16);
#endif
        position=rc_seek(rc_resource_data_file,0,SEEK_CUR);
        rc_seek(rc_resource_data_file,entry.offset,SEEK_SET);
#ifdef RECON_NATIVE_TEST
        if(rc_read_bytes(rc_resource_data_file,&bitmap,40)!=40){ __asm int 3 }
#else
        rc_read_bytes(rc_resource_data_file,&bitmap,40);
#endif
        if(bitmap.size!=40)rc_resource_file_fatal(2176,rc_resource_name);
        group.bit_count=bitmap.bit_count;group.planes=bitmap.planes;
        rc_seek(rc_resource_data_file,entry.offset,SEEK_SET);
        prefix=0;
        switch(directory.kind){
        case 1:
            group.dimensions.icon.width=entry.width;
            group.dimensions.icon.height=entry.height;
            group.dimensions.icon.color_count=entry.color_count;
            group.dimensions.icon.reserved=entry.reserved;
            break;
        case 2:
            hotspot[0]=entry.planes_or_x;hotspot[1]=entry.bits_or_y;
            group.dimensions.cursor.width=(WORD)bitmap.width;
            group.dimensions.cursor.height=(WORD)bitmap.height;
            prefix=4;
            break;
        }
        group.size=entry.size+prefix;
        image=rc_allocate(sizeof(*image));
        image->language=(WORD)rc_default_language;
        image->version=rc_resource_version;
        image->characteristics=rc_resource_characteristics;
        image->name=NULL;
        image->id=rc_next_image_id++;
        image->flags=resource->flags;
        image->size=entry.size+prefix;
        image_type=rc_find_resource_type(NULL,kind);
        rc_append_resource_bytes(&group,12);
        id=rc_mac?(WORD)rc_mac_word(image->id):image->id;
        *(WORD *)rc_reserve_resource_bytes(2)=id;
        rc_align_output(rc_output_file);
        rc_append_resource(image_type,image,hotspot,prefix,entry.size);
        rc_seek(rc_resource_data_file,position,SEEK_SET);
    }
    resource->size=directory.count*14+6;
    if(!(resource->flags&0x40))resource->flags=0x1030;
    fclose(rc_resource_data_file);rc_resource_data_file=NULL;
    rc_write_resource(type,resource);
}

long __cdecl rc_convert_bitmap_file(void)
{
    FILE *file=rc_resource_file(NULL);
    RcBitmapFileHeader file_header;
    RcBitmapHeader header;
    DWORD header_size,palette_size,bits,offset,result;
    void *palette;
    if(!file)return 0;
#ifdef RECON_NATIVE_TEST
    if(rc_read_bytes(file,&file_header,14)!=14){ __asm int 3 }
#else
    rc_read_bytes(file,&file_header,14);
#endif
    if(file_header.type!=0x4d42)rc_resource_file_fatal(2170,rc_resource_name);
#ifdef RECON_NATIVE_TEST
    if(rc_read_bytes(file,&header,12)!=12){ __asm int 3 }
#else
    rc_read_bytes(file,&header,12);
#endif
    header_size=header.info.size;
    if(header_size>=40){
#ifdef RECON_NATIVE_TEST
        /* Larger headers exceed the original local header object's extent. */
        if(header_size!=40){ __asm int 3 }
        if(rc_read_bytes(file,(BYTE *)&header+12,header_size-12)!=header_size-12){ __asm int 3 }
#else
        rc_read_bytes(file,(BYTE *)&header+12,header_size-12);
#endif
        bits=header.info.planes*header.info.bit_count;
        if(header.info.compression==3){
            if(header.info.bit_count<=8||header.info.bit_count==24)
                rc_resource_file_fatal(2170,rc_resource_name);
            palette_size=12;
        }else {
            palette_size=header.info.colors_used*4;
            if(!palette_size&&header.info.bit_count<=8)
                palette_size=(1u<<(bits&31))*4;
        }
        if(rc_mac){
            header.info.size=rc_mac_long(header.info.size);
            header.info.width=rc_mac_long(header.info.width);
            header.info.height=rc_mac_long(header.info.height);
            header.info.planes=(WORD)rc_mac_word(header.info.planes);
            header.info.bit_count=(WORD)rc_mac_word(header.info.bit_count);
            header.info.compression=rc_mac_long(header.info.compression);
            header.info.image_size=rc_mac_long(header.info.image_size);
            header.info.x_pixels=rc_mac_long(header.info.x_pixels);
            header.info.y_pixels=rc_mac_long(header.info.y_pixels);
            header.info.colors_used=rc_mac_long(header.info.colors_used);
            header.info.colors_important=rc_mac_long(header.info.colors_important);
        }
    }else if(header_size==12){
        bits=header.core.planes*header.core.bit_count;
        palette_size=bits==24?0:(1u<<(bits&31))*3;
        if(rc_mac){
            header.core.size=rc_mac_long(12);
            header.core.width=(WORD)rc_mac_word(header.core.width);
            header.core.height=(WORD)rc_mac_word(header.core.height);
            header.core.planes=(WORD)rc_mac_word(header.core.planes);
            header.core.bit_count=(WORD)rc_mac_word(header.core.bit_count);
        }
    }else {
        rc_resource_data_fatal(2171);
    }
    rc_append_resource_bytes(&header,(WORD)header_size);
    if(palette_size){
#ifdef RECON_NATIVE_TEST
        if(palette_size>0xffff){ __asm int 3 }
#endif
        palette=rc_allocate(palette_size);
#ifdef RECON_NATIVE_TEST
        if(rc_read_bytes(file,palette,palette_size)!=palette_size){ __asm int 3 }
#else
        rc_read_bytes(file,palette,palette_size);
#endif
        rc_append_resource_bytes(palette,(WORD)palette_size);
        rc_release(palette);
    }
    offset=file_header.pixel_offset;
    result=(DWORD)rc_seek(file,0,SEEK_END)+header_size+palette_size-offset;
    rc_seek(file,offset,SEEK_SET);
    return result;
}

WORD __stdcall rc_parse_raw_resource(RcResource *resource)
{
    unsigned count=0,bytes,left;
    WCHAR *text;
    BYTE *buffer,*cursor;
    WORD word;
    DWORD value;
    rc_read_resource_preamble(resource,2134);
    while(rc_resource_token.kind!=14){
        if(rc_resource_token.kind==23)
            rc_resource_token.kind=rc_resource_token.unknown19;
        switch(rc_resource_token.kind){
        case 23:
#ifdef RECON_NATIVE_TEST
            if(rc_resource_token.value>256){ __asm int 3 }
#endif
            text=rc_resource_name;
            while(rc_resource_token.value--){
                word=rc_mac?(WORD)rc_mac_word(*text++):*text++;
                *(WORD *)rc_reserve_resource_bytes(2)=word;
                count+=2;
            }
            break;
        case 21:
#ifdef RECON_NATIVE_TEST
            if(rc_resource_token.value>256){ __asm int 3 }
#endif
            bytes=WideCharToMultiByte(rc_codepage,0,rc_resource_name,
                       rc_resource_token.value,NULL,0,NULL,NULL);
            buffer=rc_allocate(bytes);
            WideCharToMultiByte(rc_codepage,0,rc_resource_name,
                       rc_resource_token.value,(char *)buffer,bytes,NULL,NULL);
            cursor=buffer;left=bytes;count+=bytes;
            while(left--)*(BYTE *)rc_reserve_resource_bytes(1)=*cursor++;
            rc_release(buffer);
            break;
        case 20:
            if(rc_resource_token.unknown12){
                value=rc_mac?rc_mac_long(rc_resource_token.unknown0):rc_resource_token.unknown0;
                *(DWORD *)rc_reserve_resource_bytes(4)=value;
                count+=4;
            }else {
                word=rc_mac?(WORD)rc_mac_word(rc_resource_token.value):rc_resource_token.value;
                *(WORD *)rc_reserve_resource_bytes(2)=word;
                count+=2;
            }
            break;
        default:
            rc_resource_error(2164);
            return 0;
        }
        rc_scan_menu_commas();
    }
    return (WORD)count;
}

void __stdcall rc_parse_dialog_include(RcResource *unused_resource)
{
    char *text;
    int bytes,index;
    if(rc_resource_token.kind!=23){
        rc_resource_error(2165);
        return;
    }
    bytes=WideCharToMultiByte(0,0,rc_resource_name,-1,NULL,0,NULL,NULL);
    text=rc_allocate(bytes);
    WideCharToMultiByte(0,0,rc_resource_name,-1,text,bytes,NULL,NULL);
    for(index=0;index<bytes;++index)
        *(BYTE *)rc_reserve_resource_bytes(1)=(BYTE)text[index];
    rc_release(text);
}

int __cdecl rc_parse_version_resource(void)
{
    WORD offset=rc_write_fixed_version();
    WORD length=rc_resource_buffer_word(offset);
    length=(WORD)(length+rc_parse_version_children());
    rc_set_resource_buffer_word(offset,length);
    return 1;
}

WORD __cdecl rc_write_fixed_version(void)
{
    RcFixedVersion fixed;
    memset(&fixed,0,sizeof(fixed));
    fixed.date_ms=0;fixed.date_ls=0;
    fixed.signature=0xfeef04bd;fixed.structure_version=0x10000;
    while(rc_resource_token.kind!=13){
        switch(rc_resource_token.kind){
        case 120:rc_read_version_quad(&fixed.file_version);break;
        case 121:rc_read_version_quad(&fixed.product_version);break;
        case 122:rc_read_version_number(&fixed.flags_mask);break;
        case 123:rc_read_version_number(&fixed.flags);break;
        case 124:rc_read_version_number(&fixed.os);break;
        case 125:rc_read_version_number(&fixed.type);break;
        case 126:rc_read_version_number(&fixed.subtype);break;
        default:rc_resource_error(2167);break;
        }
    }
    return rc_write_version_block(L"VS_VERSION_INFO",&fixed,sizeof(fixed));
}

void __stdcall rc_read_version_quad(RcVersionNumber *number)
{
    rc_scan_resource_token(1);
    if(rc_resource_token.kind!=20||rc_resource_token.unknown12)rc_resource_error(2127);
    number->ms=(DWORD)rc_resource_token.value<<16;
    rc_scan_resource_token(1);
    if(rc_resource_token.kind==15){
        rc_scan_resource_token(1);
        if(rc_resource_token.kind!=20||rc_resource_token.unknown12)rc_resource_error(2127);
        *(WORD *)&number->ms=rc_resource_token.value;
        rc_scan_resource_token(1);
        if(rc_resource_token.kind==15){
            rc_scan_resource_token(1);
            if(rc_resource_token.kind!=20||rc_resource_token.unknown12)rc_resource_error(2127);
            number->ls=(DWORD)rc_resource_token.value<<16;
            rc_scan_resource_token(1);
            if(rc_resource_token.kind==15){
                rc_scan_resource_token(1);
                if(rc_resource_token.kind!=20||rc_resource_token.unknown12)rc_resource_error(2127);
                *(WORD *)&number->ls=rc_resource_token.value;
                rc_scan_resource_token(1);
            }
            return;
        }
    }
    number->ls=0;
}

void __stdcall rc_read_version_number(DWORD *number)
{
    rc_scan_resource_token(1);
    if(rc_resource_token.kind!=20)rc_resource_error(2128);
    *number=rc_resource_token.unknown0;
    rc_scan_resource_token(1);
}

WORD __stdcall rc_write_version_block(const WCHAR *name,const void *data,unsigned bytes)
{
    DWORD zero=0;
    WORD padding,start,offset,word;
    padding=(WORD)((rc_resource_buffer_position()+3)&0xfffc);
    padding=(WORD)(padding-rc_resource_buffer_position());
    if(padding)rc_append_resource_bytes(&zero,padding);
    start=rc_resource_buffer_position();
    offset=rc_resource_buffer_position();
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(bytes):(WORD)bytes;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    rc_write_resource_string(name,1);
    if((WORD)bytes){
        padding=(WORD)((rc_resource_buffer_position()+3)&0xfffc);
        padding=(WORD)(padding-rc_resource_buffer_position());
        if(padding)rc_append_resource_bytes(&zero,padding);
        rc_append_resource_bytes(data,(WORD)bytes);
    }
    rc_set_resource_buffer_word(offset,(WORD)(rc_resource_buffer_position()-start));
    return offset;
}

WORD __cdecl rc_parse_version_children(void)
{
    WORD start=rc_resource_buffer_position(),kind,offset,length;
    if(rc_resource_token.kind!=13)rc_resource_error(2129);
    rc_scan_resource_token(1);
    while(rc_resource_token.kind!=14){
        if(rc_resource_token.kind==113||rc_resource_token.kind==114){
            kind=rc_resource_token.kind;
            rc_scan_resource_token(1);
            if(rc_resource_token.kind!=23)rc_resource_error(2131);
            offset=rc_write_version_value(rc_resource_name);
            if(kind==114){
                length=rc_resource_buffer_word(offset);
                length=(WORD)(length+rc_parse_version_children());
                rc_set_resource_buffer_word(offset,length);
                rc_scan_resource_token(1);
            }
        }else rc_resource_error(2132);
    }
    return (WORD)(rc_resource_buffer_position()-start);
}

WORD __stdcall rc_write_version_value(const WCHAR *name)
{
    DWORD zero=0;
    WORD padding,start,offset,length_offset,type_offset,word;
    padding=(WORD)((rc_resource_buffer_position()+3)&0xfffc);
    padding=(WORD)(padding-rc_resource_buffer_position());
    if(padding)rc_append_resource_bytes(&zero,padding);
    start=rc_resource_buffer_position();
    offset=rc_resource_buffer_position();
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    length_offset=rc_resource_buffer_position();
    word=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    type_offset=rc_resource_buffer_position();
    word=rc_mac?(WORD)rc_mac_word(1):1;
    *(WORD *)rc_reserve_resource_bytes(2)=word;
    rc_write_resource_string(name,1);
    rc_set_resource_buffer_word(length_offset,rc_write_version_data(type_offset));
    rc_set_resource_buffer_word(offset,(WORD)(rc_resource_buffer_position()-start));
    return offset;
}

WORD __stdcall rc_write_version_data(DWORD type_offset)
{
    DWORD zero=0,count=0,value;
    WORD first=0,kind,padding,word;
    for(;;){
        kind=rc_scan_menu_commas();
        if(!first){
            first=1;
            padding=(WORD)((rc_resource_buffer_position()+3)&0xfffc);
            padding=(WORD)(padding-rc_resource_buffer_position());
            if(padding)rc_append_resource_bytes(&zero,padding);
        }
        switch(kind){
        case 23:
            if(!rc_resource_name[0])continue;
#ifdef RECON_NATIVE_TEST
            if(!rc_resource_token.value||rc_resource_token.value>256){ __asm int 3 }
#endif
            while(!rc_resource_name[rc_resource_token.value-1])
                --rc_resource_token.value;
            /* Retain the actual full packed DWORD in this arithmetic; only
             * the low WORD is returned for the serialized value length. */
            count+=rc_resource_token.packed_value+1;
            if(rc_menu_had_comma)rc_write_resource_string(rc_resource_name,1);
            else {
                rc_concatenate_resource_string(rc_resource_name,1);
                count+=0xffff;
            }
            break;
        case 20:
            rc_set_resource_buffer_word(type_offset,0);
            if(rc_resource_token.unknown12){
                value=rc_mac?rc_mac_long(rc_resource_token.unknown0):rc_resource_token.unknown0;
                *(DWORD *)rc_reserve_resource_bytes(4)=value;
                count+=4;
            }else {
                word=rc_mac?(WORD)rc_mac_word(rc_resource_token.value):rc_resource_token.value;
                *(WORD *)rc_reserve_resource_bytes(2)=word;
                count+=2;
            }
            break;
        case 13:case 14:case 113:case 114:
            return (WORD)count;
        default:
            rc_resource_error(2133);
            return 0;
        }
    }
}

void __stdcall rc_write_string_table(RcResource *unused_resource)
{
    RcStringBlock *block;
    RcResource *resource;
    RcResourceType *type;
    WCHAR *text,*cursor,*prefix,empty[510];
    unsigned slot,length,index;
    DWORD bytes;
    WORD word;
    for(block=rc_string_blocks;block;block=block->next){
        bytes=0;
        rc_reset_resource_buffer();
        if(rc_mac){
            word=(WORD)rc_mac_word(16);
            *(WORD *)rc_reserve_resource_bytes(2)=word;
        }
        for(slot=0;slot<16;++slot){
            text=block->strings[slot];length=0;
            if(rc_mac){
                rc_write_mac_string(text,1,1);
                continue;
            }
            if(text){
                cursor=text;
                for(;;){
#ifdef RECON_NATIVE_TEST
                    /* The real parser publishes value+2 initialized WCHARs
                     * with two terminal NULs, within its value<=255 scope.
                     * Arbitrary replacements of those live owners are unknown. */
                    if(length>255){ __asm int 3 }
#endif
                    if(!cursor[0]&&!cursor[1])break;
                    ++length;++cursor;
                }
                if(rc_null_terminate_strings)++length;
            }
            bytes+=length*2+2;
            *(WORD *)rc_reserve_resource_bytes(2)=(WORD)length;
            for(index=0;index<length;++index){
                word=rc_mac?(WORD)rc_mac_word(*text++):*text++;
                *(WORD *)rc_reserve_resource_bytes(2)=word;
            }
        }
        resource=rc_allocate(sizeof(*resource));
        resource->language=block->language;
        resource->version=block->version;
        resource->characteristics=block->characteristics;
        type=rc_find_resource_type(NULL,6);
        resource->size=bytes;
        resource->flags=block->flags;
        resource->id=(WORD)(block->block+1);
        rc_write_resource(type,resource);
        memset(empty,0,sizeof(empty));
        rc_write_resource_metadata(resource,type,0);
        for(slot=0;slot<16;++slot){
            prefix=block->prefixes[slot];
#ifdef RECON_NATIVE_TEST
            if(prefix&&!block->strings[slot]){ __asm int 3 }
#endif
            if(!prefix||!block->strings[slot][0])prefix=empty;
            rc_write_apstudio_name(prefix);
        }
        rc_write_resource_metadata(NULL,NULL,0);
    }
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

int __stdcall rc_parse_menu_template(int nested,RcResource *resource)
{
    unsigned last=0;
    int any=0;
    if(!nested)rc_read_resource_preamble(resource,2121);
    else{
        if(rc_resource_token.kind!=13)rc_resource_error(2121);
        rc_scan_resource_token(1);
    }
    while(rc_resource_token.kind!=14){
        switch(rc_resource_token.kind){
        case 78:
            any=1;
            last=rc_parse_menu_item(1);
            rc_parse_menu_template(1,resource);
            break;
        case 71:
            any=1;
            last=rc_parse_menu_item(0);
            break;
        default:
            rc_resource_error(2122);
            break;
        }
    }
    if(!any)rc_resource_error(2124);
    if(nested)rc_scan_resource_token(1);
    rc_mark_last_menu_item((WORD)last);
    return 1;
}

int __stdcall rc_parse_menu(int nested,RcResource *resource)
{
    DWORD help=0,value;
    WORD last=0;
    int any=0;
    if(!nested){
        rc_parse_extended_menu_fields(2,&help);
        value=rc_mac?rc_mac_long(help):help;
        *(DWORD *)rc_reserve_resource_bytes(4)=value;
        rc_read_resource_preamble(resource,2121);
    }else{
        if(rc_resource_token.kind!=13)rc_resource_error(2121);
        rc_scan_resource_token(1);
    }
    while(rc_resource_token.kind!=14){
        switch(rc_resource_token.kind){
        case 78:
            any=1;
            last=rc_parse_extended_menu_item(1);
            rc_parse_menu(1,resource);
            break;
        case 71:
            any=1;
            last=rc_parse_extended_menu_item(0);
            break;
        default:rc_resource_error_name(2245,rc_resource_name);break;
        }
    }
    if(!any)rc_resource_error_name(2246,rc_resource_name);
    if(nested)rc_scan_resource_token(0x8000);
    rc_mark_last_menu_item(last);
    return 1;
}

/* MENUEX's three finite field grammars. A high bit marks an optional field;
 * low bits select a WORD, DWORD or caption. The header accepts any end token.
 * Original header end-count is the encountered loader-zero WORD at RVA27f98;
 * no surrounding storage or mutable alias is modeled here. */
static const struct {
    WORD ends[4];
    BYTE count,fields[5];
} rc_extended_menu_grammar[]={
    {{1,13,0,0},5,{2,0x81,0x81,0x81,0x81}},
    {{3,78,71,14},4,{2,0x81,0x81,0x81,0}},
    {{0,0,0,0},1,{0x81,0,0,0,0}}
};

void __cdecl rc_parse_extended_menu_fields(unsigned grammar,...)
{
    const WORD *ends;
    const BYTE *fields;
    unsigned count,index;
    WORD status;
    void *value;
    va_list arguments;
    if(grammar>=3){ __asm int 3 }
    ends=rc_extended_menu_grammar[grammar].ends;
    fields=rc_extended_menu_grammar[grammar].fields;
    count=rc_extended_menu_grammar[grammar].count;
    va_start(arguments,grammar);
    for(index=0;index<count;++index){
        value=va_arg(arguments,void *);
        switch(fields[index]&0x7f){
        case 0:status=rc_read_extended_menu_number(ends,0,value);break;
        case 1:status=rc_read_extended_menu_number(ends,1,value);break;
        case 2:status=rc_read_extended_menu_caption(ends,value);break;
        }
        if(!(status&1)&&!(fields[index]&0x80))
            rc_resource_error_name(2236,rc_resource_name);
        if(status&2){
            while(++index<count)
                if(!(fields[index]&0x80))rc_resource_error_name(2236,rc_resource_name);
            va_end(arguments);
            return;
        }
        rc_scan_resource_token(0x8000);
        rc_write_apstudio_name(rc_resource_token.text);
    }
    va_end(arguments);
    if(!rc_menu_token_in_set(ends))rc_resource_error_name(2235,rc_resource_name);
}

WORD __stdcall rc_read_extended_menu_number(const WORD *ends,int wide,void *value)
{
    DWORD number=0;
    WORD status=rc_parse_resource_style_expression(&number,0)!=0;
    if(rc_menu_token_in_set(ends))status|=2;
    else if(rc_resource_token.kind!=15)rc_resource_error_name(2242,rc_resource_name);
    if(wide)*(DWORD *)value=number;
    else *(WORD *)value=(WORD)number;
    return status;
}

int __stdcall rc_menu_token_in_set(const WORD *tokens)
{
    WORD count=*tokens;
    if(!count)return 1;
    do{
        if(tokens[count]==rc_resource_token.kind)return 1;
    }while(--count);
    return 0;
}

WORD __stdcall rc_parse_extended_menu_item(int popup)
{
    RcExtendedMenuItem item;
    item.type=0;item.state=0;item.id=0;item.help=0;
    item.flags=popup!=0;item.caption[0]=0;
    rc_scan_resource_token(0x8000);
    if(rc_resource_token.kind==20&&rc_resource_token.value==0x800){
        if(popup)rc_resource_error_name(2244,rc_resource_name);
        item.type=0x800;item.state=0;item.id=0;
        rc_scan_resource_token(0x8000);
        if(!rc_menu_token_in_set(rc_extended_menu_grammar[1].ends))
            rc_resource_error_name(2235,rc_resource_name);
    }else if(popup)
        rc_parse_extended_menu_fields(0,item.caption,&item.id,&item.type,
            &item.state,&item.help);
    else rc_parse_extended_menu_fields(1,item.caption,&item.id,&item.type,&item.state);
    return rc_write_extended_menu_item(&item);
}

WORD __stdcall rc_read_extended_menu_caption(const WORD *ends,WCHAR *text)
{
    WCHAR *begin=text;
    WORD have=0;
    int plus=0,ended=rc_menu_token_in_set(ends);
    while(!ended&&rc_resource_token.kind!=15){
        /* The caller owns 257 WCHARs. Concatenation beyond that encountered
         * object remains unsupported in both builds, before the copier writes. */
        if((rc_resource_token.kind==21||rc_resource_token.kind==23)&&
           (unsigned)(text-begin)+(rc_resource_token.value>256?255:
               rc_resource_token.value)>256){ __asm int 3 }
        if(rc_copy_menu_caption(text)){
            text+=wcslen(text);
            if(plus)plus=0;
            else if(have)rc_resource_error_name(2243,rc_resource_name);
            have=1;
        }else if(rc_resource_token.kind==101&&!plus&&have)plus=1;
        else rc_resource_error_name(2243,rc_resource_name);
        rc_scan_resource_token(0x8000);
        ended=rc_menu_token_in_set(ends);
    }
    if(plus)rc_resource_error_name(2238,rc_resource_name);
    if(ended)have|=2;
    return have;
}

int __stdcall rc_parse_toolbar_resource(RcResource *resource)
{
    WORD width,height,value;
    int any=0;
    value=rc_mac?(WORD)rc_mac_word(1):1;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    rc_read_toolbar_dimensions(&width,&height);
    value=rc_mac?(WORD)rc_mac_word(width):width;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    value=rc_mac?(WORD)rc_mac_word(height):height;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    /* Toolbar and dialog counts share the original DWORD at RVA28fdc. */
    rc_dialog_count_offset=rc_resource_buffer_used;
    value=rc_mac?(WORD)rc_mac_word(0):0;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    rc_read_resource_preamble(resource,2251);
    while(rc_resource_token.kind!=14){
        switch(rc_resource_token.kind){
        case 127:
            rc_resource_error(2252);
            rc_fatal("");
            break;
        case 95:
            any=1;
            rc_scan_resource_token(1);
            if(rc_resource_token.kind!=20)rc_resource_error(2250);
            rc_write_apstudio_name(rc_resource_token.text);
            value=rc_read_toolbar_number();
            if(rc_mac)value=(WORD)rc_mac_word(value);
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            break;
        case 72:
            any=1;
            rc_scan_resource_token(0x8000);
            value=rc_mac?(WORD)rc_mac_word(0):0;
            *(WORD *)rc_reserve_resource_bytes(2)=value;
            break;
        default:
            rc_resource_error(2253);
            rc_scan_resource_token(0x8000);
            continue;
        }
        rc_increment_dialog_count();
    }
    if(!any)rc_resource_error(2254);
    if(rc_mac)rc_convert_dialog_count();
    return 1;
}

void __stdcall rc_read_toolbar_dimensions(WORD *width,WORD *height)
{
    *width=rc_read_toolbar_number();
    if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
    *height=rc_read_toolbar_number();
}

WORD __cdecl rc_read_toolbar_number(void)
{
    WORD value;
    if(!rc_parse_resource_style(&value,3))rc_resource_error(2250);
    return value;
}

void __stdcall rc_parse_dialog_class(RcDialog *dialog)
{
    rc_scan_resource_token(1);
    if(!rc_copy_menu_caption(dialog->class_name)){
        if(rc_resource_token.kind==20){
            rc_format_wide_resource_number(rc_resource_token.value,
                dialog->class_name,10);
            dialog->class_is_ordinal=1;
        }else rc_resource_error(2120);
    }
    rc_scan_resource_token(1);
}

void __stdcall rc_read_resource_preamble(RcResource *resource,unsigned message)
{
    while(rc_resource_token.kind!=13){
        switch(rc_resource_token.kind){
        case 132:
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2139);
            resource->version=rc_resource_token.unknown0;
            break;
        case 131:
            resource->language=rc_read_resource_language();
            break;
        case 130:
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2140);
            resource->characteristics=rc_resource_token.unknown0;
            break;
        default:
            rc_resource_error(message);
            break;
        }
        rc_scan_resource_token(0);
    }
    if(rc_resource_token.kind!=13)rc_resource_error(message);
    rc_scan_resource_token(1);
}

WORD __stdcall rc_parse_menu_item(int popup)
{
    RcMenuItem item;
    item.unknown518=(BYTE)popup;
    rc_scan_resource_token(1);
    if(rc_copy_menu_caption(item.caption)){
        item.flags=0x10;
        rc_scan_menu_commas();
        if(!popup){
            item.flags=0;
            rc_write_apstudio_name(rc_resource_token.text);
            if(!rc_parse_resource_style(&item.id,3))rc_resource_error(2125);
            if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
        }
        while(rc_parse_menu_attribute(rc_resource_token.kind,&item))
            rc_scan_menu_commas();
    }else if(rc_resource_token.kind==72){
        item.caption[0]=0;
        item.id=0;
        item.flags=0;
        rc_scan_menu_commas();
    }else rc_resource_error(2126);
    return rc_write_menu_item(&item);
}

int __stdcall rc_copy_menu_caption(WCHAR *text)
{
    if(rc_resource_token.kind!=21&&rc_resource_token.kind!=23)return 0;
    if(rc_resource_token.value>256){
        rc_format(rc_error_text,2048,rc_message(4208),
                  rc_resource_filename,rc_resource_token.line);
        rc_emit(rc_error_text);
        rc_resource_name[256]=0;
        rc_resource_token.value=255;
    }
    memcpy(text,rc_resource_name,(rc_resource_token.value+1)*sizeof(WCHAR));
    return 1;
}

/* DWORD at RVA29014 is written before this scanner's first consumer. */
int rc_menu_had_comma;
WORD __cdecl rc_scan_menu_commas(void)
{
    rc_menu_had_comma=0;
    rc_scan_resource_token(1);
    while(rc_resource_token.kind==15){
        rc_scan_resource_token(1);
        rc_menu_had_comma=1;
    }
    return rc_resource_token.kind;
}

int __stdcall rc_parse_menu_attribute(unsigned kind,RcMenuItem *item)
{
    if(kind==50)item->flags|=0x100;
    else if(kind==67)item->flags|=4;
    else if(kind==73)item->flags|=8;
    else if(kind==74)item->flags|=1;
    else if(kind==75)item->flags|=2;
    else if(kind==76)item->flags|=0x20;
    else if(kind==77)item->flags|=0x40;
    else if(kind==79)item->flags|=0x4000;
    else return 0;
    return 1;
}

int __stdcall rc_parse_resource_style(void *style,unsigned flags)
{
    DWORD value=0;
    int result;
    if(!(flags&1))value=flags&2?*(WORD *)style:*(DWORD *)style;
    result=rc_parse_resource_style_expression(&value,0);
    if(flags&2)*(WORD *)style=(WORD)value;
    else *(DWORD *)style=value;
    return result;
}

int __stdcall rc_parse_resource_style_expression(DWORD *value,int parenthesized)
{
    WORD operation;
    BYTE kind;
    if(!rc_apply_resource_style_term(value,12))return 0;
    for(;;){
        if(rc_resource_token.kind==20){
            if((LONG)rc_resource_token.unknown0<0){
                *value+=rc_resource_token.unknown0;
                rc_scan_resource_token(0x8000);
                continue;
            }
            if(!rc_resource_token.unknown0&&rc_resource_name[0]==L'-'&&
               !rc_resource_name[1])rc_resource_token.kind=102;
        }
        kind=rc_resource_token.kind;
        if(kind==2){
            if(parenthesized)rc_scan_resource_token(0x8000);
            else rc_resource_error_name(2241,rc_resource_name);
            return 1;
        }
        if(kind==12||kind==17||(kind>100&&kind<=102)||kind==104){
            operation=kind;
            rc_scan_resource_token(0x8000);
            if(!rc_apply_resource_style_term(value,operation))
                rc_resource_error_name(2237,rc_resource_name);
            continue;
        }
        if(parenthesized)rc_resource_error_name(2241,rc_resource_name);
        return 1;
    }
}

int __stdcall rc_apply_resource_style_term(DWORD *value,WORD operation)
{
    DWORD operand=0;
    int grouped=0;
    BYTE kind=rc_resource_token.kind;
    if(kind==1){
        rc_scan_resource_token(0x8000);
        if(!rc_parse_resource_style_expression(&operand,1))return 0;
        grouped=1;
    }else if(kind==16||kind==104){
        if(kind==104)operation=17;
        rc_scan_resource_token(0x8000);
        operand=~rc_resource_token.unknown0;
    }else if(kind==20)operand=rc_resource_token.unknown0;
    else if(kind==101){
        rc_scan_resource_token(0x8000);
        operand=rc_resource_token.unknown0;
    }else if(kind==102){
        rc_scan_resource_token(0x8000);
        operand=0-rc_resource_token.unknown0;
    }else return 0;
    if(!grouped){
        if(rc_resource_token.kind!=20)rc_resource_error_name(2237,rc_resource_name);
        rc_scan_resource_token(0x8000);
    }
    if(operation==12)*value|=operand;
    else if(operation==17)*value&=operand;
    else if(operation==101)*value+=operand;
    else if(operation==102)*value-=operand;
    return 1;
}

int __stdcall rc_parse_dialog(RcResource *resource,RcDialog *dialog,int extended)
{
    dialog->style=0x80880000;
    dialog->exstyle=resource->unknown8;
    dialog->menu[0]=0;
    dialog->caption[0]=0;
    dialog->class_name[0]=0;
    dialog->menu_is_ordinal=0;
    dialog->class_is_ordinal=0;
    dialog->point_size=0;
    rc_read_dialog_rectangle(&dialog->x,&dialog->y,&dialog->width,&dialog->height);
    if(!rc_parse_dialog_attributes(resource,dialog,extended))return 0;
    if(dialog->point_size)dialog->style|=0x40;
    else dialog->style&=~0x40;
    rc_write_dialog_header(dialog,extended);
    if(rc_resource_token.kind!=13)rc_resource_error(2112);
    rc_parse_dialog_controls(extended);
    if(rc_mac)rc_convert_dialog_count();
    if(rc_resource_token.kind!=14)rc_resource_error(2113);
    return 1;
}

void __stdcall rc_read_dialog_rectangle(WORD *x,WORD *y,WORD *width,WORD *height)
{
    *x=rc_read_dialog_coordinate();
    if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
    *y=rc_read_dialog_coordinate();
    if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
    *width=rc_read_dialog_coordinate();
    if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
    *height=rc_read_dialog_coordinate();
}

WORD __cdecl rc_read_dialog_coordinate(void)
{
    WORD value;
    if(!rc_parse_resource_style(&value,3))rc_resource_error(2109);
    return value;
}

int __stdcall rc_parse_dialog_attributes(RcResource *resource,RcDialog *dialog,int extended)
{
    if(extended&&rc_resource_token.kind==15){
        rc_scan_resource_token(0x8000);
        rc_parse_resource_style(&dialog->help_id,1);
    }
    while(rc_resource_token.kind!=13){
        switch(rc_resource_token.kind){
        case 70:rc_parse_dialog_menu(dialog);break;
        case 94:rc_parse_dialog_font(dialog,extended);break;
        case 100:rc_parse_dialog_class(dialog);break;
        case 130:
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2140);
            resource->characteristics=rc_resource_token.unknown0;
            rc_scan_resource_token(0);
            break;
        case 131:
            resource->language=rc_read_resource_language();
            rc_scan_resource_token(0);
            break;
        case 132:
            rc_scan_resource_token(0);
            if(rc_resource_token.kind!=20)rc_resource_error(2139);
            resource->version=rc_resource_token.unknown0;
            rc_scan_resource_token(0);
            break;
        case 133:
            dialog->style=(dialog->style&0xc00000)==0xc00000?0xc00000:0;
            rc_scan_resource_value(0x8000);
            rc_parse_resource_style(&dialog->style,0);
            break;
        case 135:rc_parse_dialog_caption(dialog);break;
        case 247:
            rc_scan_resource_value(0x8000);
            rc_parse_resource_style(&dialog->exstyle,0);
            break;
        default:rc_resource_error(2112);return 0;
        }
    }
    return 1;
}

void __stdcall rc_parse_dialog_caption(RcDialog *dialog)
{
    rc_scan_resource_token(1);
    if(rc_copy_menu_caption(dialog->caption))dialog->style|=0xc00000;
    else rc_resource_error(2119);
    rc_scan_resource_token(1);
}

void __stdcall rc_parse_dialog_font(RcDialog *dialog,int extended)
{
    WORD value;
    int i;
    rc_scan_resource_token(1);
    if(!rc_parse_resource_style(&dialog->point_size,3))rc_resource_error(2117);
    if(rc_resource_token.kind==15)rc_scan_menu_commas();
    if(!rc_copy_menu_caption(dialog->font))rc_resource_error(2118);
    if(_wcsicmp(dialog->font,L"System")&&rc_special_file[0]&&rc_special_type_count>0){
        for(i=0;i<rc_special_type_count;++i){
            if(!_wcsicmp(rc_special_types[i],dialog->font)){
                rc_resource_notice_strings(4510,dialog->font,rc_special_file,0);
                wcscpy(dialog->font,rc_special_file);
            }
        }
    }
    rc_scan_resource_token(1);
    dialog->charset=1;
    if(extended&&rc_resource_token.kind==15){
        rc_scan_resource_token(0x8000);
        if(rc_parse_resource_style(&value,3))dialog->weight=value;
        if(rc_resource_token.kind==15){
            rc_scan_resource_token(0x8000);
            if(rc_resource_token.kind==20){
                dialog->italic=rc_resource_token.value!=0;
                rc_scan_resource_token(0x8000);
                if(rc_resource_token.kind==15){
                    rc_scan_resource_token(0x8000);
                    if(rc_parse_resource_style(&value,3))dialog->charset=(BYTE)value;
                }
            }
        }
    }
}

void __stdcall rc_parse_dialog_menu(RcDialog *dialog)
{
    WCHAR first;
    if(rc_collect_resource_name()){
        rc_resource_token.kind=23;
        rc_copy_menu_caption(dialog->menu);
        first=dialog->menu[0];
        if(first>=L'0'&&first<=L'9')dialog->menu_is_ordinal=1;
        rc_scan_resource_token(1);
    }
}

typedef struct RcControlKind {
    WORD kind,reserved2;
    DWORD style;
    BYTE class_ordinal,has_text;
    WORD reserved10;
} RcControlKind;
typedef char RcControlKindLayout[
    sizeof(RcControlKind)==12 && offsetof(RcControlKind,style)==4 &&
    offsetof(RcControlKind,class_ordinal)==8 && offsetof(RcControlKind,has_text)==9 ? 1 : -1];
/* Exact 22 contextual descriptors at RVA15290..15398. */
static const RcControlKind rc_control_kinds[]={
    { 51,0,0x00000007,0x80,1,0 }, /* GROUPBOX */
    { 42,0,0x00010000,0x80,1,0 }, /* PUSHBUTTON */
    { 43,0,0x00010001,0x80,1,0 }, /* DEFPUSHBUTTON */
    { 41,0,0x00010002,0x80,1,0 }, /* CHECKBOX */
    { 40,0,0x00000004,0x80,1,0 }, /* RADIOBUTTON */
    { 46,0,0x00010006,0x80,1,0 }, /* AUTO3STATE */
    { 44,0,0x00010003,0x80,1,0 }, /* AUTOCHECKBOX */
    { 48,0,0x00000009,0x80,1,0 }, /* AUTORADIOBUTTON */
    { 52,0,0x0001000a,0x80,1,0 }, /* PUSHBOX */
    { 45,0,0x00010005,0x80,1,0 }, /* STATE3 */
    { 47,0,0x00010008,0x80,1,0 }, /* USERBUTTON */
    { 61,0,0x00020000,0x82,1,0 }, /* LTEXT */
    { 62,0,0x00020002,0x82,1,0 }, /* RTEXT */
    { 63,0,0x00020001,0x82,1,0 }, /* CTEXT */
    { 66,0,0x00000003,0x82,1,0 }, /* ICON */
    { 57,0,0x00810000,0x00,0,0 }, /* BEDIT */
    { 58,0,0x00810000,0x00,0,0 }, /* HEDIT */
    { 59,0,0x00810000,0x00,0,0 }, /* IEDIT */
    { 60,0,0x00810000,0x81,0,0 }, /* EDITTEXT */
    { 90,0,0x00800001,0x83,0,0 }, /* LISTBOX */
    { 91,0,0x00000000,0x85,0,0 }, /* COMBOBOX */
    { 93,0,0x00000000,0x84,0,0 }, /* SCROLLBAR */
};
/* Each invocation establishes the count/capacity and allocated list before use. */
int rc_dialog_id_count,rc_dialog_id_capacity;
DWORD *rc_dialog_ids;

int __stdcall rc_parse_dialog_controls(int extended)
{
    RcControl control;
    unsigned i;
    BYTE kind;
    rc_dialog_id_count=0;
    rc_dialog_id_capacity=100;
    rc_dialog_ids=rc_allocate(400);
    if(!rc_dialog_ids)return 0;
    rc_scan_resource_token(1);
    control.id=0;
    while(rc_resource_token.kind!=14){
        kind=rc_resource_token.kind;
        control.help_id=0;control.exstyle=0;control.style=0x50000000;
        control.text[0]=0;control.text_is_ordinal=0;
        if(kind==134)rc_parse_generic_control(&control,extended);
        else {
            for(i=0;i<22;++i)if(rc_control_kinds[i].kind==kind)break;
            if(i==22){rc_resource_error(2111);return 0;}
            control.style=rc_control_kinds[i].style|0x50000000;
            if(rc_mac&&(kind==42||kind==43||kind==41||kind==46||
                        kind==44||kind==52||kind==45||kind==47))
                control.style&=0xfffeffff;
            if(rc_control_kinds[i].class_ordinal){
                control.class_name[0]=0xffff;
                control.class_name[1]=rc_control_kinds[i].class_ordinal;
            }else {
#ifdef RECON_NATIVE_TEST
                /* No uninitialized class spelling is a supported native input. */
                if(!rc_copy_menu_caption(control.class_name)){ __asm int 3 }
#else
                rc_copy_menu_caption(control.class_name);
#endif
            }
            if(rc_control_kinds[i].has_text)rc_read_control_text(&control);
            rc_read_control_id(&control,extended);
            rc_read_dialog_rectangle(&control.x,&control.y,&control.width,&control.height);
            if(rc_resource_token.kind==15){
                rc_scan_resource_token(0x8000);
                rc_parse_resource_style(&control.style,0);
            }
        }
        if(rc_resource_token.kind==15){
            rc_scan_resource_token(0x8000);
            rc_parse_resource_style(&control.exstyle,0);
            if(extended&&rc_resource_token.kind==15){
                rc_scan_resource_token(0x8000);
                rc_parse_resource_style(&control.help_id,1);
            }
        }
        rc_write_control(&control,extended);
        if(extended&&rc_resource_token.kind==13){
            rc_patch_control_data_size(rc_parse_raw_resource(NULL),extended);
            rc_scan_resource_token(0x8000);
        }
    }
    rc_release(rc_dialog_ids);
    return 1;
}

void __stdcall rc_read_control_text(RcControl *control)
{
    rc_scan_resource_value(0x8000);
    if(rc_copy_menu_caption(control->text)){
        control->text_is_ordinal=0;
        rc_resource_token.text[0]=0;
        rc_resource_token.unknown1038=0;
    }else if(rc_resource_token.kind==20){
        rc_format_wide_resource_number(rc_resource_token.value,control->text,10);
        control->text_is_ordinal=1;
        rc_write_apstudio_name(rc_resource_token.text);
    }else rc_resource_error(2115);
}

void __stdcall rc_read_control_id(RcControl *control,int extended)
{
    DWORD *ids;
    int i;
    rc_scan_menu_commas();
    rc_write_apstudio_name(rc_resource_token.text);
    if(rc_parse_resource_style(&control->id,extended?1:3)){
        if((extended||control->id!=0xffff)&&control->id!=0xffffffff){
#ifdef RECON_NATIVE_TEST
            if(rc_dialog_id_count<0||rc_dialog_id_capacity<rc_dialog_id_count||
               !rc_dialog_ids){ __asm int 3 }
#endif
            for(i=0;i<rc_dialog_id_count;++i){
                if(control->id==rc_dialog_ids[i]){
                    rc_format(rc_error_text,2048,rc_message(2182),rc_resource_filename,
                              rc_resource_token.line,control->id);
                    rc_emit(rc_error_text);
                    break;
                }
            }
            if(rc_dialog_id_count==rc_dialog_id_capacity){
#ifdef RECON_NATIVE_TEST
                if(rc_dialog_id_capacity>0x1fffff99){ __asm int 3 }
#endif
                rc_dialog_id_capacity+=100;
                ids=rc_allocate(rc_dialog_id_capacity*sizeof(DWORD));
                memcpy(ids,rc_dialog_ids,rc_dialog_id_count*sizeof(DWORD));
                rc_release(rc_dialog_ids);
                rc_dialog_ids=ids;
            }
            rc_dialog_ids[rc_dialog_id_count++]=control->id;
        }
    }else rc_resource_error(2116);
    if(rc_resource_token.kind==15)rc_scan_menu_commas();
}

typedef struct RcControlClass {
    BYTE ordinal,reserved1;
    WORD kind;
    const WCHAR *name;
} RcControlClass;
typedef char RcControlClassLayout[
    sizeof(RcControlClass)==8 && offsetof(RcControlClass,kind)==2 &&
    offsetof(RcControlClass,name)==4 ? 1 : -1];
/* Case-insensitive aliases, searched from the final row back to the first. */
static const RcControlClass rc_control_classes[]={
    {0x80,0,95,L"button"},
    {0x81,0,64,L"edit"},
    {0x82,0,65,L"static"},
    {0x83,0,90,L"listbox"},
    {0x84,0,93,L"scrollbar"},
    {0x85,0,91,L"combobox"}
};

void __stdcall rc_parse_generic_control(RcControl *control,int extended)
{
    int i;
    rc_read_control_text(control);
    rc_read_control_id(control,extended);
    if(rc_resource_token.kind==20){
        control->class_name[0]=(WORD)(signed char)rc_resource_token.value;
        control->class_name[1]=0;
    }else {
        if(rc_resource_token.kind==23){
            for(i=5;i>=0;--i)
                if(!_wcsicmp(rc_resource_name,rc_control_classes[i].name))break;
            if(i<0)rc_copy_menu_caption(control->class_name);
        }else {
            for(i=5;i>=0;--i)
                if(rc_resource_token.kind==rc_control_classes[i].kind)break;
            if(i<0){rc_resource_error(2114);i=0;}
        }
        if(i>=0){
            control->class_name[0]=0xffff;
            control->class_name[1]=rc_control_classes[i].ordinal;
        }
    }
    rc_scan_resource_value(0x8000);
    rc_parse_resource_style(&control->style,0);
    rc_scan_menu_commas();
    rc_read_dialog_rectangle(&control->x,&control->y,&control->width,&control->height);
}

WCHAR *__stdcall rc_format_wide_resource_number(unsigned value,WCHAR *text,unsigned radix)
{
    unsigned remaining=value,digits=0;
    WCHAR *cursor;
#ifdef RECON_NATIVE_TEST
    /* Current callers provide a live 257-WCHAR field and radix ten. */
    if(!text||radix!=10){ __asm int 3 }
#endif
    while(remaining){++digits;remaining/=radix;}
    text[digits]=0;
    cursor=text+digits;
    while(value){
        *--cursor=(WCHAR)(L'0'+(signed char)(value%radix));
        value/=radix;
    }
    return cursor;
}

RcResource * __stdcall rc_parse_string_table(RcResource *resource)
{
    RcStringBlock *block,*previous;
    DWORD id;
    WCHAR *prefix;
    BYTE found;
    unsigned slot;
    rc_read_resource_preamble(resource,2105);
    if(!rc_last_string_resource){previous=NULL;rc_string_blocks=NULL;}
#ifdef RECON_NATIVE_TEST
    else if(!rc_string_blocks){ __asm int 3 }
#endif
    do {
        found=0;
        block=rc_string_blocks;
        if(rc_resource_token.kind!=20)rc_resource_error(2149);
        id=rc_resource_token.packed_value;
        prefix=rc_allocate_copy(1020,(BYTE *)&rc_resource_token+
                                    offsetof(RcResourceToken,text));
        if(!rc_parse_resource_style(&id,3))rc_resource_error(2110);
        if(rc_resource_token.kind==15)rc_scan_resource_token(0x8000);
        if(rc_resource_token.kind!=23)rc_resource_error(2150);
#ifdef RECON_NATIVE_TEST
        /* This consumer needs both the terminator and one extra initialized
         * NUL within the existing 257-WCHAR name workspace. */
        if(rc_resource_token.value>255){ __asm int 3 }
#endif
        rc_resource_name[rc_resource_token.value+1]=0;
        slot=(WORD)id%16;
        while(block){
            if(block->language==resource->language&&block->block==((WORD)id>>4)){
                found=1;
                if(!block->strings[slot]){
                    block->strings[slot]=rc_allocate_copy(rc_resource_token.value*2+4,
                                                         rc_resource_name);
                    block->prefixes[slot]=prefix;
                }else {
                    rc_format(rc_error_text,2048,rc_message(2151),rc_resource_filename,
                              rc_resource_token.line,(unsigned)(WORD)id,
                              block->strings[slot],rc_resource_name);
                    rc_report_resource_error(2151);
                }
            }
            previous=block;
            block=block->next;
            if(found)break;
        }
        if(!found){
            block=rc_allocate_copy(sizeof(RcStringBlock),NULL);
            block->block=(WORD)id>>4;
            block->flags=resource->flags;
            block->language=resource->language;
            block->version=resource->version;
            block->characteristics=resource->characteristics;
            block->strings[slot]=rc_allocate_copy(rc_resource_token.value*2+4,
                                                 rc_resource_name);
            block->prefixes[slot]=prefix;
            if(previous)previous->next=block;
            if(!rc_string_blocks)rc_string_blocks=block;
        }
        rc_scan_resource_token(1);
    }while(rc_resource_token.kind!=14);
    rc_last_string_resource=resource;
    return resource;
}

void *__stdcall rc_allocate_copy(unsigned bytes,const void *source)
{
    BYTE *result,*out;
    const BYTE *in=source;
    unsigned count=bytes;
#ifdef RECON_NATIVE_TEST
    if(bytes>0x7ffffff7){ __asm int 3 }
#endif
    result=rc_allocate(bytes);
    if(!result)rc_fatal("RC : fatal error RW1025: Out of heap memory");
    if(source){
        out=result;
        while(count--)*out++=*in++;
    }else memset(result,0,bytes);
    return result;
}

int __stdcall rc_parse_accelerators(RcResource *resource)
{
    WORD flags,key,command,word;
    BYTE key_kind;
    int count=0,mode_given;
    DWORD value;
    rc_read_resource_preamble(resource,2106);
    while(rc_resource_token.kind!=14){
        mode_given=0;
        key_kind=rc_resource_token.kind;
        if(key_kind==20)key=rc_resource_token.value;
        else if(key_kind==23){
            if(rc_resource_name[0]==L'^'){
                if(wcslen(rc_resource_name)!=2)rc_resource_error(2152);
                if(rc_resource_name[1]==L'^')key=L'^';
                else {
                    key=(WORD)towupper(rc_resource_name[1]);
                    if(!iswctype(key,0x103))rc_resource_error(2154);
                    key-=64;
                }
            }else if(wcslen(rc_resource_name)==2)
                key=(WORD)((rc_resource_name[0]<<8)+rc_resource_name[1]);
            else if(wcslen(rc_resource_name)==1)key=rc_resource_name[0];
            else rc_resource_error(2155);
        }else rc_resource_error(2156);
        rc_scan_resource_token(1);
        if(rc_resource_token.kind!=15)rc_resource_error(2157);
        rc_scan_resource_token(1);
        if(rc_resource_token.kind!=20)rc_resource_error(2107);
        command=rc_resource_token.value;
        rc_write_apstudio_name(rc_resource_token.text);
        if(!rc_parse_resource_style(&command,3))rc_resource_error(2107);
        flags=0;
        while(rc_resource_token.kind==15){
            rc_scan_resource_token(1);
            switch(rc_resource_token.kind){
            case 103:flags|=2;break;
            case 106:flags|=4;break;
            case 110:flags|=16;break;
            case 111:mode_given=1;break;
            case 112:flags|=1;mode_given=1;break;
            case 134:flags|=8;break;
            default:rc_resource_error(2159);break;
            }
            rc_scan_resource_token(1);
        }
        if(key_kind==20&&!mode_given)rc_resource_error(2163);
        if(!(flags&1)&&(flags&12)){
            rc_format(rc_error_text,2048,rc_message(4203),rc_resource_filename,
                      rc_resource_token.line);
            rc_emit(rc_error_text);
        }
        if((flags&1)&&key_kind==23){
            key=(WORD)towupper(key);
            if(!iswctype(key,0x107)){
                rc_format(rc_error_text,2048,rc_message(4204),rc_resource_filename,
                          rc_resource_token.line);
                rc_emit(rc_error_text);
            }
        }
        rc_begin_accelerator();
        word=rc_mac?(WORD)rc_mac_word(flags):flags;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        word=rc_mac?(WORD)rc_mac_word(key):key;
        *(WORD *)rc_reserve_resource_bytes(2)=word;
        if(rc_mac){
            value=rc_mac_long(command);
            *(DWORD *)rc_reserve_resource_bytes(4)=value;
        }else {
            *(WORD *)rc_reserve_resource_bytes(2)=command;
            word=rc_mac?(WORD)rc_mac_word(0):0;
            *(WORD *)rc_reserve_resource_bytes(2)=word;
        }
        ++count;
    }
    rc_mark_last_accelerator();
    return count*5;
}

long __cdecl rc_read_resource_file(void)
{
    char filename[MAX_PATH],path[MAX_PATH];
    FILE *file;
    long size;
    WideCharToMultiByte(GetACP(),0,rc_resource_name,-1,filename,
                        MAX_PATH,NULL,NULL);
    rc_search_resource_path(filename,"INCLUDE",path);
    if(path[0]){
        file=fopen(path,"rb");
        if(file){
            size=rc_seek(file,0,SEEK_END);
            rc_seek(file,0,SEEK_SET);
            rc_resource_file(file);
            return size;
        }
    }
    rc_resource_error_name(2135,filename);
    return 0;
}

void __stdcall rc_search_resource_path(const char *filename,const char *variable,
                                      char *path)
{
    const char *paths;
    char *end;
    unsigned length;
    FILE *file;
    paths=!strcmp(variable,"INCLUDE")?rc_include_paths:getenv(variable);
#ifdef RECON_NATIVE_TEST
    if(!paths||!filename[0]||strlen(filename)>=MAX_PATH){ __asm int 3 }
#endif
    if(filename[0]=='\\'||filename[0]=='/'||filename[1]==':'){
        strcpy(path,filename);
        return;
    }
    do {
        length=0;
        while(paths[length]&&paths[length]!=';'){
#ifdef RECON_NATIVE_TEST
            if(length>=MAX_PATH-1){ __asm int 3 }
#endif
            path[length]=paths[length];
            ++length;
        }
        path[length]=0;
        paths+=length;
        if(*paths)++paths;
        end=path+strlen(path);
#ifdef RECON_NATIVE_TEST
        /* The original examines end[-1]; an empty path component is unknown. */
        if(end==path||length+strlen(filename)+2>MAX_PATH){ __asm int 3 }
#endif
        if(end[-1]!='\\'&&end[-1]!='/')*end++='\\';
        strcpy(end,filename);
        file=fopen(path,"rb");
        if(file){fclose(file);break;}
    }while(*path&&*paths);
}

/* Validate the RIFF/ACON chunk walk, then leave the entire file at its start
 * for the ordinary resource-payload copier. No chunk data is rewritten. */
long __stdcall rc_copy_riff_resource(long size)
{
    struct { DWORD tag,size; } chunk;
    long remaining;
    unsigned received=rc_read_bytes(rc_resource_data_file,&chunk,8);
    if(received!=8){ __asm int 3 }
    remaining=size-received;
    if(chunk.tag!=0x46464952)
        rc_resource_file_fatal(2173,rc_resource_name);
    received=rc_read_bytes(rc_resource_data_file,&chunk,4);
    if(received!=4){ __asm int 3 }
    remaining-=received;
    if(chunk.tag!=0x4e4f4341)
        rc_resource_file_fatal(2173,rc_resource_name);
    while((received=rc_read_bytes(rc_resource_data_file,&chunk,8))!=0){
        if(received!=8){ __asm int 3 }
        remaining-=8+chunk.size;
        rc_seek(rc_resource_data_file,chunk.size,SEEK_CUR);
    }
    if(remaining)rc_resource_file_fatal(2173,rc_resource_name);
    rc_seek(rc_resource_data_file,0,SEEK_SET);
    return size;
}

void __stdcall rc_resource_notice_strings(unsigned code,const void *first,
                                         const void *second,DWORD extra)
{
    rc_format(rc_error_text,2048,rc_message(code),rc_resource_filename,
              rc_resource_token.line,first,second,extra);
    rc_emit_notice(rc_error_text);
}

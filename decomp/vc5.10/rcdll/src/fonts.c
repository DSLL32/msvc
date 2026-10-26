#include <stdio.h>
#include <string.h>
#include "command.h"
#include "resources.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(memcpy)
#endif

/* The actual reader uses this 148-byte expanded header directly from disk.
 * Its DWORDs at136/140 are not the packed FNTv3 offsets101/105: in a packed
 * v3 file they are reserved. Preserve that original interpretation. Only the
 * named offsets are interpreted here; the preceding metrics are transferred.
 */
typedef struct RcFontHeader {
    BYTE metrics[136];
    DWORD device_offset,face_offset,reserved;
} RcFontHeader;
typedef char RcFontHeaderLayout[
    sizeof(RcFontHeader)==148 && offsetof(RcFontHeader,device_offset)==136 &&
    offsetof(RcFontHeader,face_offset)==140 ? 1 : -1];

/* Distinct loader-zero pointer owners: head25478 and tail22c30. The count
 * stays at25990 and is advanced by the resource dispatcher after success. */
RcFontDirectoryEntry *rc_font_directory_head,*rc_font_directory_tail;

int __stdcall rc_parse_font_resource(RcResource *resource)
{
    struct {
        RcFontHeader header;
        char names[64];
    } font;
    FILE *file=rc_resource_file(NULL);
    RcFontDirectoryEntry *entry,*at;
    char *name=font.names;
    short size;

    if(!file)return 0;
    rc_seek(file,0,SEEK_SET);
    /* Short reads and names beyond the observed local storage are unsupported;
     * the original would consume undefined bytes or overwrite its saved FILE. */
    if(rc_read_bytes(file,&font.header,148)!=148){ __asm int 3 }
    if(font.header.device_offset){
        rc_seek(file,font.header.device_offset,SEEK_SET);
        do {
            if(name==font.names+sizeof(font.names)){ __asm int 3 }
            if(rc_read_bytes(file,name,1)!=1){ __asm int 3 }
        }while(*name++);
    }else *name++=0;
    rc_seek(file,font.header.face_offset,SEEK_SET);
    do {
        if(name==font.names+sizeof(font.names)){ __asm int 3 }
        if(rc_read_bytes(file,name,1)!=1){ __asm int 3 }
    }while(*name++);
    size=(short)(148+(name-font.names));
    entry=rc_allocate(size+8);
    entry->size=(WORD)size;
    entry->id=resource->id;
    entry->next=NULL;
    memcpy(entry->data,&font,size);
    if(!rc_font_resource_count){
        rc_font_directory_tail=entry;
        rc_font_directory_head=entry;
    }else{
        for(at=rc_font_directory_head;at;at=at->next)
            if(entry->id==at->id){
                rc_format(rc_error_text,2048,rc_message(2181),
                          rc_resource_filename,rc_resource_token.line,
                          (unsigned)entry->id);
                rc_emit(rc_error_text);
                rc_release(entry);
                return 0;
            }
        rc_font_directory_tail->next=entry;
        rc_font_directory_tail=entry;
    }
    rc_seek(file,0,SEEK_SET);
    return 1;
}

void __cdecl rc_flush_font_directory(void)
{
    RcResource *resource=rc_allocate(sizeof(*resource));
    RcResourceType *type;
    RcFontDirectoryEntry *entry;
    WORD value;
    extern int rc_mac;
    resource->language=(WORD)rc_default_language;
    resource->version=rc_resource_version;
    resource->characteristics=rc_resource_characteristics;
    resource->name=rc_duplicate_resource_name(L"FONTDIR");
    type=rc_find_resource_type(NULL,7);
    rc_reset_resource_buffer();
    value=rc_mac?(WORD)rc_mac_word(rc_font_resource_count):rc_font_resource_count;
    *(WORD *)rc_reserve_resource_bytes(2)=value;
    for(entry=rc_font_directory_head;entry;entry=entry->next){
        value=rc_mac?(WORD)rc_mac_word(entry->id):entry->id;
        *(WORD *)rc_reserve_resource_bytes(2)=value;
        rc_append_resource_bytes(entry->data,entry->size);
    }
    resource->flags=0x50;
    rc_write_resource(type,resource);
}

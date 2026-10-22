/* Macintosh runtime fixup sorting and compressed output. */
#include <stdlib.h>
#include "object.h"
#include "link-pass.h"
#include <recon.h>

extern MacFixupList mac_data_references;
extern unsigned long mac_data_base;
unsigned char *mac_fixup_buffer;
void sort_mac_fixup_list(MacFixupList *);
unsigned long write_mac_fixup_list(MacFixupList *,unsigned char);

/* Original [00039bc0,00039c61). */
unsigned long size_mac_data_fixups(Image *image,unsigned long file)
{
    MacFixupList *block=&mac_data_references;
    MacRelocationRef *record=block->records;
    unsigned long index,in_block=0;
    if (mac_data_references.count) {
        for (index=0;index<mac_data_references.count;++index) {
            if (in_block==512) {
                block=block->next;
                in_block=0;
                record=block->records;
            }
            record->offset+=record->contribution->rva;
            if (image->options.eliminate_unreferenced && contribution_discarded(record->contribution))
                record->offset=0xffffffff;
            ++record;
            ++in_block;
        }
        sort_mac_fixup_list(&mac_data_references);
    }
    seek_file(output_file,file,0);
    return write_mac_fixup_list(&mac_data_references,0);
}
void sort_mac_fixup_list(MacFixupList *list) { __asm { int 3 } }
/* Original [00037800,00037a35).  Fixups are unsigned halfword deltas;
 * the first zero offset has a distinct two-byte representation. */
unsigned long write_mac_fixup_list(MacFixupList *list,unsigned char code)
{
    unsigned char *cursor;
    unsigned short total=0,pending=0,index;
    unsigned long delta,base;
    if (!mac_fixup_buffer) mac_fixup_buffer=malloc(2056);
    cursor=mac_fixup_buffer;
    base=code?40:mac_data_base;
    if (list->count && list->records[0].offset!=0xffffffff) {
        delta=code?list->records[0].offset+base:list->records[0].offset-base;
        if (delta&1) fatal_message(program_name,4,"Run-time fixup on odd address");
        if (!delta) {
            *cursor++=0x80; *cursor++=0;
            total=2;
        } else if (delta<255) {
            *cursor++=(unsigned char)(delta>>1);
            total=1;
        } else if (delta<65535) {
            delta=(delta>>1)|0x8000;
            *cursor++=(unsigned char)(delta>>8); *cursor++=(unsigned char)delta;
            total=2;
        } else {
            *cursor++=0;
            delta=(delta>>1)|0x80000000;
            *cursor++=(unsigned char)(delta>>24); *cursor++=(unsigned char)(delta>>16);
            *cursor++=(unsigned char)(delta>>8); *cursor++=(unsigned char)delta;
            total=5;
        }
        pending=total;
    }
    for (index=1;index<list->count;++index) {
        if (list->records[index].offset==0xffffffff) break;
        delta=list->records[index].offset-list->records[index-1].offset;
        if (delta&1) fatal_message(program_name,4,"Run-time fixup on odd address");
        if (delta<255) {
            *cursor++=(unsigned char)(delta>>1);
            ++total; ++pending;
        } else if (delta<65535) {
            delta=(delta>>1)|0x8000;
            *cursor++=(unsigned char)(delta>>8); *cursor++=(unsigned char)delta;
            total+=2; pending+=2;
        } else {
            *cursor++=0;
            delta=(delta>>1)|0x80000000;
            *cursor++=(unsigned char)(delta>>24); *cursor++=(unsigned char)(delta>>16);
            *cursor++=(unsigned char)(delta>>8); *cursor++=(unsigned char)delta;
            total+=5; pending+=5;
        }
        if (pending>=2048) {
            write_file(output_file,mac_fixup_buffer,pending);
            pending=0;
            cursor=mac_fixup_buffer;
        }
    }
    cursor[0]=0; cursor[1]=0;
    pending+=2;
    write_file(output_file,mac_fixup_buffer,pending);
    free(list->records);
    return (unsigned long)total+2;
}

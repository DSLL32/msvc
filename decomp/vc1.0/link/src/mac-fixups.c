/* Macintosh runtime fixup sorting and compressed output. */
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "link-pass.h"
#include "mac.h"
#include <recon.h>

#pragma intrinsic(memcpy)

extern MacFixupList mac_data_references;
extern unsigned long mac_data_base;
unsigned char *mac_fixup_buffer;
void sort_mac_fixup_list(MacFixupList *);
unsigned long write_mac_fixup_list(MacFixupList *,unsigned char);
void sort_mac_section_fixups(unsigned short);
int compare_mac_fixup_offsets(const void *,const void *);

/* Original [000376e0,000377fc).  Emit both streams at the aligned end
 * and retain their big-endian offsets in the forty-byte resource header. */
unsigned long size_mac_section_fixups(Section *section,unsigned long file)
{
    unsigned short resource=(unsigned short)section->mac_resource_id;
    unsigned long data,code,value;
    seek_file(output_file,file,0);
    data=write_mac_fixup_list(&mac_section_data_fixups[resource],1);
    code=write_mac_fixup_list(&mac_section_code_fixups[resource],1);
    seek_file(output_file,section->file_offset+20,0);
    value=MAC_BIG_LONG(section->raw_size);
    write_file(output_file,&value,4);
    seek_file(output_file,4,1);
    value=MAC_BIG_LONG(section->raw_size+data);
    write_file(output_file,&value,4);
    return data+code;
}

/* Original [00037550,0003764b). */
void build_mac_section_fixups(Section *section,Image *image)
{
    unsigned short resource=(unsigned short)section->mac_resource_id;
    MacFixupList *block=&mac_section_data_fixups[resource];
    MacRelocationRef *record=block->records;
    Contribution *contribution;
    unsigned long remaining=block->count,in_block=0;
    while (remaining--) {
        if (in_block==512) {
            block=block->next;
            in_block=0;
            record=block->records;
        }
        contribution=record->contribution;
        record->offset+=contribution->rva-contribution->group->section->groups->rva;
        if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
            record->offset=0xffffffff;
        ++record;
        ++in_block;
    }
    block=&mac_section_code_fixups[resource];
    record=block->records;
    remaining=block->count;
    in_block=0;
    while (remaining--) {
        if (in_block==512) {
            block=block->next;
            in_block=0;
            record=block->records;
        }
        contribution=record->contribution;
        record->offset+=contribution->rva-contribution->group->section->groups->rva;
        if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
            record->offset=0xffffffff;
        ++record;
        ++in_block;
    }
    sort_mac_section_fixups(resource);
}
/* Original [00037650,000376d4). */
void sort_mac_section_fixups(unsigned short resource)
{
    MacFixupList *list=&mac_section_data_fixups[resource];
    if (list->capacity) {
        if (list->capacity<512)
            qsort(list->records,list->count,sizeof(MacRelocationRef),compare_mac_fixup_offsets);
        else sort_mac_fixup_list(list);
    }
    list=&mac_section_code_fixups[resource];
    if (list->capacity) {
        if (list->capacity<512)
            qsort(list->records,list->count,sizeof(MacRelocationRef),compare_mac_fixup_offsets);
        else sort_mac_fixup_list(list);
    }
}
/* Original [000370a0,000370c3), unsigned offset ordering. */
int compare_mac_fixup_offsets(const void *left,const void *right)
{
    unsigned long a=((const MacRelocationRef *)left)->offset;
    unsigned long b=((const MacRelocationRef *)right)->offset;
    return a>b?1:(a<b?-1:0);
}

/* Original [00037180,00037299).  Head capacity/count span the chain;
 * used is local to each512-record allocation, and last names the tail. */
void append_mac_fixup(MacFixupList *list,Contribution *contribution,unsigned long offset)
{
    MacRelocationRef *record;
    MacFixupList *tail;
    if (!list->capacity) {
        list->records=malloc(4096);
        if (!list->records) fatal_message(NULL,5,"");
        list->capacity=512;
        list->count=0;
        list->used=0;
        list->last=list;
        list->next=NULL;
    } else if (list->count==list->capacity) {
        list->capacity+=512;
        list->last->next=malloc(sizeof(MacFixupList));
        if (!list->last->next) fatal_message(NULL,5,"");
        tail=list->last->next;
        list->last=tail;
        tail->records=malloc(4096);
        if (!tail->records) fatal_message(NULL,5,"");
        tail->used=0;
        tail->next=NULL;
    }
    if (!list->next) record=&list->records[list->used++];
    else record=&list->last->records[list->last->used++];
    record->offset=offset;
    record->contribution=contribution;
    ++list->count;
}

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
/* Original [00039c70,00039d1a).  Consolidation frees record allocations,
 * preserving the original chain nodes/capacity fields. */
void sort_mac_fixup_list(MacFixupList *list)
{
    MacRelocationRef *records=calloc(list->count,sizeof(MacRelocationRef));
    MacRelocationRef *destination=records;
    MacFixupList *block;
    unsigned long remaining=list->count,count;
    if (!records) fatal_message(NULL,5,"");
    for (block=list;block;block=block->next) {
        count=block->next?512:remaining;
        memcpy(destination,block->records,count*sizeof(MacRelocationRef));
        remaining-=count;
        destination+=count;
        free(block->records);
    }
    list->records=records;
    qsort(records,list->count,sizeof(MacRelocationRef),compare_mac_fixup_offsets);
}
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

/* PowerPC branch islands contain serialized instructions, never host code. */
#include <windows.h>
#include <stddef.h>
#include "full-link-functions.h"
#include "sections.h"
#include "memory.h"
#include <recon.h>

typedef struct PpcBranchIsland {
    struct PpcBranchIsland *next;
    Contribution *contribution;
    DWORD rva,used,capacity;
    DWORD *targets;
} PpcBranchIsland;
typedef char PpcBranchIslandLayout[(sizeof(PpcBranchIsland)==24 &&
    offsetof(PpcBranchIsland,targets)==20)?1:-1];
/* Original4946fc head and4673e4 address of its final next-link slot. */
static PpcBranchIsland *islands;
static PpcBranchIsland **island_tail=&islands;

extern "C" void __stdcall add_ppc_branch_island(Contribution *last,DWORD rva,
    DWORD count)
{
    PpcBranchIsland *island;
    if(!count)return;
    island=(PpcBranchIsland *)allocate_memory(sizeof(PpcBranchIsland));
    island->contribution=last;
    island->rva=rva;
    island->used=0;
    island->capacity=count;
    island->targets=(DWORD *)allocate_memory(count*4);
    island->next=NULL;
    *island_tail=island;
    island_tail=&island->next;
}

extern "C" {
extern BYTE *next_base_relocation;
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
}
extern "C" DWORD __stdcall find_ppc_branch_target(LinkContext *context,DWORD site,
    DWORD absolute,DWORD section,DWORD value)
{
    PpcBranchIsland *island=islands;
    LinkBaseRelocation *record;
    DWORD rva,target;
    while(island && island->rva<site)island=island->next;
    rva=island->rva+island->used*16;
    island->targets[island->used]=value;
    /* Actual physical DWORD slots retain their complete transport; only
     * the low BYTE flag and low WORD section enter these consumers. */
    if(!(BYTE)absolute) {
        target=value-context->optional_header.ImageBase;
        if(context->options.debug_formats&8) {
            record_debug_fixup(16,0,rva,target);
            record_debug_fixup(18,0,rva,0);
            record_debug_fixup(17,0,rva+4,target);
        }
        if(!context->options.fixed_image) {
            record=(LinkBaseRelocation *)next_base_relocation;
            record->type=4;
            record->address=rva;
            record->section=(SHORT)section;
            record->value=target;
            next_base_relocation+=12;
        }
        if(!context->options.fixed_image) {
            record=(LinkBaseRelocation *)next_base_relocation;
            record->type=2;
            record->address=rva+4;
            record->section=(SHORT)section;
            record->value=0;
            next_base_relocation+=12;
        }
    }
    ++island->used;
    return rva;
}

extern "C" {
extern int output_file;
extern BYTE file_access_statistics;
}
extern "C" void __cdecl finish_ppc_branches(void)
{
    PpcBranchIsland *island;
    FileRecord *record;
    DWORD error,i,target,instructions[4];
    while(islands) {
        island=islands;
        islands=island->next;
        error=0;
        seek_file(output_file,island->contribution->file_offset+island->rva-
            island->contribution->rva,FILE_BEGIN,&error);
        for(i=0;i<island->used;++i) {
            target=island->targets[i];
            /* Serialized LIS r11 / ADDI r11,r11 / MTCTR r11 / BCTR. */
            instructions[0]=0x3d600000|(((target>>16)+((target&0x8000)?1:0))&0xffff);
            instructions[1]=0x396b0000|(target&0xffff);
            instructions[2]=0x7d6903a6;
            instructions[3]=0x4e800420;
            if(file_access_statistics)record_file_access();
            record=file_slots[output_file];
            if(record->flags&8)mapped_write(record,instructions,16);
            else buffered_write(record,instructions,16);
        }
        release_memory(island->targets);
        release_memory(island);
    }
    island_tail=&islands;
}

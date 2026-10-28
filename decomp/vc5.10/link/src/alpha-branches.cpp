/* Alpha branch islands are serialized instruction stubs, never host code. */
#include <windows.h>
#include <stddef.h>
#include <string.h>
#include "full-link-functions.h"
#include "sections.h"
#include "memory.h"
#include <recon.h>

typedef struct AlphaBranchIsland {
    Contribution *contribution;
    DWORD next_rva,remaining,total;
    DWORD *targets;
} AlphaBranchIsland;
typedef char AlphaBranchIslandLayout[(sizeof(AlphaBranchIsland)==20 &&
    offsetof(AlphaBranchIsland,targets)==16)?1:-1];
/* Original 4674c8/cc/d0: capacity, owned array and used record count. */
static DWORD island_capacity,island_count;
static AlphaBranchIsland *islands;

extern "C" void __stdcall add_alpha_branch_island(Contribution *last,DWORD rva,
    DWORD count)
{
    AlphaBranchIsland *island;
    if(!count)return;
    if(!islands) {
        islands=(AlphaBranchIsland *)allocate_memory(4*sizeof(AlphaBranchIsland));
        island_capacity+=4;
    }
    if(island_count>=island_capacity) {
        island_capacity+=4;
        islands=(AlphaBranchIsland *)reallocate_memory(islands,
            island_capacity*sizeof(AlphaBranchIsland));
    }
    island=islands+island_count;
    /* Only targets recorded by later fixups are read by the final writer. */
    island->targets=(DWORD *)allocate_memory(count*4);
    island->contribution=last;
    island->next_rva=rva;
    island->remaining=count;
    island->total=count;
    ++island_count;
}

extern "C" {
extern DWORD gp_base,gp_limit;
}
void __cdecl update_gp_bounds(void)
{
    ContributionGroup *group;
    if(!small_data_section->merge_target) {
        gp_base=small_data_section->rva;
        gp_limit=small_data_section->rva+small_data_section->raw_size;
        return;
    }
    group=find_contribution_group(small_data_section->merge_target,".sdata");
    if(group) {
        gp_base=group->rva;
        while(group->next && !strncmp(".sdata$",group->next->name,7))group=group->next;
        gp_limit=group->rva+group->size;
    }
}

extern "C" {
extern BYTE *next_base_relocation;
void __stdcall record_debug_fixup(WORD,WORD,DWORD,DWORD);
}
extern "C" DWORD __stdcall find_alpha_branch_target(LinkContext *context,DWORD site,
    DWORD absolute,DWORD section,DWORD value)
{
    AlphaBranchIsland *island=NULL;
    LinkBaseRelocation *record;
    DWORD i,used,target,rva;
    LONG displacement;
    for(i=0;i<island_count;++i) {
        displacement=(LONG)(islands[i].next_rva-site);
        if(islands[i].remaining && displacement> -0x400000 && displacement<0x400000) {
            island=islands+i;
            break;
        }
    }
    if(!island)return 0;
    used=island->total-island->remaining;
    for(i=0;i<used;++i) {
        if(island->targets[i]==value) {
            rva=island->next_rva+i*16-used*16;
            displacement=(LONG)(rva-site);
            if(displacement> -0x400000 && displacement<0x400000)return rva;
        }
    }
    rva=island->next_rva;
    /* Both formals retain their full physical DWORD transport. Only the
     * original low BYTE absolute flag and low WORD section are consumed. */
    if(!(BYTE)absolute) {
        target=value-context->optional_header.ImageBase;
        if(context->options.debug_formats&8) {
            record_debug_fixup(9,0,rva,target);
            record_debug_fixup(13,0,rva,4);
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
    island->targets[island->total-island->remaining]=value;
    --island->remaining;
    island->next_rva+=16;
    return rva;
}

extern "C" {
extern int output_file;
extern BYTE file_access_statistics;
}
extern "C" void __cdecl finish_alpha_branches(void)
{
    DWORD i,j,used,position,error,target,instructions[4];
    FileRecord *record;
    for(i=0;i<island_count;++i) {
        AlphaBranchIsland *island=islands+i;
        used=island->total-island->remaining;
        position=island->contribution->file_offset+island->contribution->raw_size-
            island->total*16;
        for(j=0;j<used;++j) {
            target=island->targets[j];
            /* Output-only Alpha LDAH r28 / LDA r28 / JMP through r28,
             * followed by the original zero padding DWORD. */
            instructions[0]=0x279f0000;
            instructions[1]=0x239c0000;
            instructions[2]=0x6bfc0000;
            instructions[3]=0;
            *(WORD *)(instructions+0)=(WORD)((target>>16)+((target&0x8000)?1:0));
            *(WORD *)(instructions+1)=(WORD)target;
            error=0;seek_file(output_file,position,FILE_BEGIN,&error);
            if(file_access_statistics)record_file_access();
            record=file_slots[output_file];
            if(record->flags&8)mapped_write(record,instructions,16);
            else buffered_write(record,instructions,16);
            position+=16;
        }
        release_memory(island->targets);
    }
    /* The original consumes this one-shot table without resetting its
     * pointer, capacity or used count after freeing it. */
    release_memory(islands);
}

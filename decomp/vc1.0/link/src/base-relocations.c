/* PE base relocation blocks emitted from the target's typed fixup records. */
#include "link-pass.h"
#include <recon.h>

/* Group fixups by4KB page and serialize IMAGE_BASE_RELOCATION blocks. */
extern const unsigned char output_zero_padding[16];
extern char *output_name;
void emit_base_relocations(IMAGE_OPTIONAL_HEADER *header)
{
    BaseFixup *first,*scan,*record;
    IMAGE_BASE_RELOCATION block;
    unsigned long total=0,padding,tail;
    unsigned short encoded;
    unsigned char last;
    void *zeroes;
    seek_file(output_file,base_relocation_section->file_offset,SEEK_SET);
    qsort(base_fixups,base_fixup_next-base_fixups,sizeof(*base_fixups),compare_base_fixups);
    first=base_fixups;
    for (scan=base_fixups;scan!=base_fixup_next;++scan) {
        if (scan->type) { first=scan; break; }
    }
    /* With only ABSOLUTE records the original retains the allocation base. */
    block.VirtualAddress=first->rva&0xfffff000UL;
    do {
        scan=first;
        while (scan!=base_fixup_next && (scan->rva&0xfffff000UL)==block.VirtualAddress)
            ++scan;
        last=scan==base_fixup_next;
        block.SizeOfBlock=8+(unsigned long)(scan-first)*2;
        for (record=first;record!=scan;++record)
            if (record->type==IMAGE_REL_BASED_HIGHADJ)
                block.SizeOfBlock+=last?4:2;
        /* The final-page HIGHADJ reservation is four bytes in the original. */
        padding=block.SizeOfBlock&3;
        if (padding) padding=4-padding;
        block.SizeOfBlock+=padding;
        write_file(output_file,&block,sizeof(block));
        for (record=first;record!=scan;++record) {
            encoded=(unsigned short)((record->rva&0xfff)|(record->type<<12));
            write_file(output_file,&encoded,sizeof(encoded));
            if (record->type==IMAGE_REL_BASED_HIGHADJ) {
                encoded=(unsigned short)record->value.adjustment;
                write_file(output_file,&encoded,sizeof(encoded));
            }
        }
        if (padding) write_file(output_file,output_zero_padding,padding);
        base_relocation_section->file_offset+=block.SizeOfBlock;
        total+=block.SizeOfBlock;
        if (last) break;
        first=scan;
        block.VirtualAddress=first->rva&0xfffff000UL;
    } while (1);
    free(base_fixups);
    header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size=total;
    if (total>=base_relocation_section->raw_size)
        fatal_message(output_name,0x21,total-base_relocation_section->raw_size);
    tail=base_relocation_section->raw_size-total;
    zeroes=calloc(tail,1);
    if (!zeroes) out_of_memory();
    write_file(output_file,zeroes,tail);
    free(zeroes);
    base_relocation_section->file_offset+=tail;
    base_relocation_section->file_end=base_relocation_section->file_offset;
}


int compare_base_fixups(const void *left,const void *right)
{
    return (int)(((const BaseFixup *)left)->rva-((const BaseFixup *)right)->rva);
}


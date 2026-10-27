/* The legacy dumper consumes the same public LE/VxD header as the writer. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dumper.h"
#include "input.h"
#include "vxd.h"
#include "full-link-functions.h"
#include <recon.h>
extern "C" {
DWORD dumped_le_header_offset;
VxdImageHeader dumped_le_header;
extern BYTE file_access_statistics;
extern FILE *map_output;
extern const char *default_phase;
void __stdcall dump_le_object_header(DWORD index,VxdObjectRecord *object)
{
    static const char *access_names[8]={NULL,"Read Only","Write Only","Read Write","Execute Only","Execute Read","Execute Write","Execute Read Write"};
    static const struct {DWORD bit;const char *name;} flag_names[]={{0x8,"Resource"},{0x10,"Discardable"},{0x20,"Shared"},{0x40,"Has preload pages"},{0x80,"Has invalid pages"},{0x200,"Resident"},{0x1000,"16:16 alias"},{0x2000,"32-bit"},{0x4000,"Conforming"},{0x8000,"Message"}};
    unsigned access=object->flags&7,i;
    fprintf(map_output,"\nOBJECT HEADER #%X\n%8lX virtual size\n%8lX virtual address\n%8lX flags\n",index,object->virtual_size,object->base,object->flags);
    if(access)fprintf(map_output,"         %s\n",access_names[access]);
    for(i=0;i<sizeof(flag_names)/sizeof(flag_names[0]);++i)
        if(object->flags&flag_names[i].bit)fprintf(map_output,"         %s\n",flag_names[i].name);
    fprintf(map_output,"%8lX map index\n%8lX map size\n%8lX reserved\n",object->page_map_index,object->page_count,object->reserved);
}
void __stdcall dump_le_pages(DWORD index,VxdObjectRecord *object,const BYTE *pages)
{
    DWORD logical,number,offset;
    const char *kind;
    const BYTE *page=pages+4*(object->page_map_index-1);
    fprintf(map_output,"\nOBJECT PAGE MAP #%X\n\n",index);
    fprintf(map_output,"    Logical  Physical File     Flags\n    Page     Page     Offset   Flags\n    -------- -------- -------- --------\n");
    for(logical=1;logical<=object->page_count;++logical,page+=4) {
        offset=0;
        switch(page[3]) {
        case 0: kind="Valid";offset=dumped_le_header.data_pages;break;
        case 1: kind="Iterated";break;
        case 2: kind="Invalid";break;
        case 3: kind="Zeroed";break;
        case 4: kind="Range";break;
        default: kind="Unknown";break;
        }
        number=((DWORD)page[0]<<16)|((DWORD)page[1]<<8)|page[2];
        if(offset && number)offset+=(number-1)*dumped_le_header.page_size;
        fprintf(map_output,"    %08lX %08lX %08lX %s\n",logical,number,offset,kind);
    }
}
void __cdecl link_unresolved_error(const char *,unsigned,...);
DWORD __stdcall read_le_object_data(VxdObjectRecord *object,const BYTE *pages,BYTE **result)
{
    DWORD size=object->virtual_size,page_size=dumped_le_header.page_size;
    DWORD allocation,index,number,offset,bytes,error;
    BYTE *cursor;
    const BYTE *page;
    FileRecord *record;
    *result=NULL;
    if(!size || !object->page_count)return 0;
    allocation=(size+page_size-1)&~(page_size-1);
#ifdef __linux__
    /* Reject malformed spans before native pointer arithmetic or page writes. */
    if(!page_size || (page_size&(page_size-1)) || size>0xffffffffUL-(page_size-1) ||
        !object->page_map_index || object->page_map_index>dumped_le_header.module_pages ||
        object->page_count>dumped_le_header.module_pages-object->page_map_index+1 ||
        object->page_count>allocation/page_size)__asm { int 3 }
#endif
    cursor=(BYTE *)zero_allocate(allocation);
    *result=cursor;
    page=pages+4*(object->page_map_index-1);
    for(index=0;index<object->page_count;++index,page+=4,cursor+=page_size) {
        number=((DWORD)page[0]<<16)|((DWORD)page[1]<<8)|page[2];
        if(number && !page[3]) {
            offset=dumped_le_header.data_pages+(number-1)*page_size;
            error=0;
            if(seek_file(current_input_file,offset,FILE_BEGIN,&error)==-1)
                link_unresolved_error(NULL,1106,offset);
            bytes=number==dumped_le_header.module_pages?dumped_le_header.last_page_size:page_size;
#ifdef __linux__
            if(bytes>page_size)__asm { int 3 }
#endif
            if(file_access_statistics)record_file_access();
            record=file_slots[current_input_file];
            if(record->flags&8)mapped_read(record,cursor,bytes);
            else buffered_read(record,cursor,bytes);
        }
    }
    return size;
}
void __stdcall disassemble_section(DumperSectionState *,void *,DWORD,DWORD,DWORD,DWORD,
    const void *,DWORD,FILE *);
void __stdcall dump_raw_data(DWORD,const void *,DWORD);
#pragma intrinsic(memset)
void __cdecl dump_le_objects(void)
{
    VxdObjectRecord *objects,*object;
    BYTE *pages,*data;
    DWORD error=0,bytes,index,address;
    FileRecord *record;
    BYTE disassembly,raw_data;
    union {DWORD raw;BYTE low;} mode;
    DumperSectionState state;
#define LE_READ(destination,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[current_input_file]; \
    if(record->flags&8)mapped_read(record,(destination),(length)); \
    else buffered_read(record,(destination),(length)); \
} while(0)
    default_phase="DumpLeSections";
    objects=(VxdObjectRecord *)allocate_memory(dumped_le_header.object_count*sizeof(*objects));
    seek_file(current_input_file,dumped_le_header.object_table+dumped_le_header_offset,
        FILE_BEGIN,&error);
    LE_READ(objects,dumped_le_header.object_count*sizeof(*objects));
    pages=(BYTE *)allocate_memory(dumped_le_header.module_pages*4);
    error=0;seek_file(current_input_file,dumped_le_header.page_map+dumped_le_header_offset,
        FILE_BEGIN,&error);
    LE_READ(pages,dumped_le_header.module_pages*4);
    for(index=1;index<=dumped_le_header.object_count;++index) {
        object=objects+index-1;
        if(dumper_context->options.dump_headers) {
            dump_le_object_header(index,object);
            if(object->page_count)dump_le_pages(index,object,pages);
        }
        disassembly=dumper_context->options.dump_disassembly && (object->flags&4);
        raw_data=dumper_context->options.dump_raw_data;
        if(disassembly || raw_data) {
            bytes=read_le_object_data(object,pages,&data);
            if(bytes) {
                address=object->base;
#ifdef __linux__
                /* Original mode has only its BYTE initialized before DWORD
                 * transport. No upper-bit equivalence or consumption claim. */
                mode.raw=0;
#endif
                mode.low=0;
                if(object->flags&0x1000) {address=index<<16;mode.low=1;}
                if(disassembly) {
                    fputc('\n',map_output);
                    memset(&state,0,sizeof(state));
                    disassemble_section(&state,NULL,IMAGE_FILE_MACHINE_I386,mode.raw,index,
                        address,data,bytes,map_output);
                }
                if(raw_data) {
                    fprintf(map_output,"\nRAW DATA #%hX\n",index);
                    dump_raw_data(address,data,bytes);
                }
            }
            release_memory(data);
        }
    }
    release_memory(pages);release_memory(objects);
#undef LE_READ
}

void __stdcall dump_le_file(const char *filename)
{
    DWORD error=0;
    FileRecord *record;
    VxdImageHeader *header=&dumped_le_header;
    const char *kind;
    dumped_le_header_offset=(DWORD)seek_file(current_input_file,-4,FILE_CURRENT,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[current_input_file];
    if(record->flags&8)mapped_read(record,header,sizeof(*header));
    else buffered_read(record,header,sizeof(*header));
    if(header->os==4 && (header->flags&0x8000) &&
        ((header->flags&0x38000)==0x28000 || (header->flags&0x38000)==0x38000))kind="VXD";
    else kind="LE Executable";
    fprintf(map_output,"\nFile Type: %s\n",kind);
    if(dumper_context->options.dump_headers) {
        fprintf(map_output,"\n%8hX magic number\n%8hX byte order\n%8hX word order\n%8lX executable format level\n%8hX CPU type (**)\n%8hX operating system (**)\n%8lX module version\n%8lX module flags\n%8lX number of memory pages\n%8lX object number of entry point\n%8lX offset of entry point\n%8lX object number of stack\n%8lX offset of stack\n%8lX memory page size\n",
            (DWORD)header->signature,(DWORD)header->byte_order,(DWORD)header->word_order,
            header->format_level,(DWORD)header->cpu,(DWORD)header->os,header->module_version,
            header->flags,header->module_pages,header->entry_object,header->entry_offset,
            header->stack_object,header->stack_offset,header->page_size);
        fprintf(map_output,"%8lX bytes on last page\n",header->last_page_size);
        fprintf(map_output,"%8lX fixup section size\n%8lX fixup section checksum\n%8lX loader section size\n%8lX loader section checksum\n%8lX object table\n%8lX object table entries\n%8lX object map\n%8lX iterated data map\n%8lX resource table\n%8lX resource table entries\n%8lX resident names table\n%8lX entry table\n%8lX module directives table\n%8lX module directives entries\n%8lX fixup page table\n%8lX fixup record table\n%8lX imported modules name table\n%8lX imported modules\n%8lX imported procedures name table\n%8lX page checksum table\n%8lX enumerated data pages\n%8lX preload page count\n%8lX non-resident name table\n%8lX non-resident name table size\n%8lX non-resident name checksum\n%8lX automatic data object\n%8lX debug information\n%8lX debug information size\n%8lX preload instance page count\n%8lX demand instance page count\n%8lX extra heap allocation\n",header->fixup_size,header->fixup_checksum,
            header->loader_size,header->loader_checksum,header->object_table,header->object_count,
            header->page_map,header->iterated_pages,header->resource_table,header->resource_count,
            header->resident_names,header->entry_table,header->directives,header->directive_count,
            header->fixup_page_table,header->fixup_records,header->import_modules,
            header->import_module_count,header->import_procedures,header->page_checksums,
            header->data_pages,header->preload_pages,header->nonresident_names,header->nonresident_size,
            header->nonresident_checksum,header->automatic_data_object,header->debug_offset,
            header->debug_size,header->preload_instance_pages,header->demand_instance_pages,
            header->heap_size);
        if(header->os==4 && (header->flags&0x8000))
            fprintf(map_output,"%8lX offset of Windows resources\n%8lX size of Windows resources\n%8hX device id\n%8hX DDK version\n",header->windows_resources,header->windows_resource_size,
                (DWORD)header->device_id,(DWORD)header->ddk_version);
    }
    dump_le_objects();
}
}

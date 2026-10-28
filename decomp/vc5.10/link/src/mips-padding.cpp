/* R4000 code-page hazard analysis concerns emitted MIPS bytes only. */
#include <windows.h>
#include <string.h>
#include "input.h"
#include "files.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>
#pragma intrinsic(memcpy)
extern "C" {
extern OutputModule *output_module;
extern BYTE file_access_statistics;
void __cdecl link_contribution_fatal(Contribution *,unsigned,...);
BYTE __stdcall mips_memory_register_dependency(const BYTE *contents,DWORD left,DWORD right)
{
    DWORD instruction,opcode,target;
    memcpy(&instruction,contents,4);
    opcode=instruction>>26;
    switch(opcode) {
    case 32: case 33: case 34: case 35: case 36: case 37: case 38:
    case 48: case 55: case 56:
        target=(instruction>>16)&31;
        return target==left || target==right;
    default:return 0;
    }
}
BYTE __stdcall mips_divide_follows_integer_memory(const BYTE *contents)
{
    DWORD instruction,function;
    memcpy(&instruction,contents,4);
    if(instruction>>26)return 0;
    function=instruction&63;
    if(function!=26 && function!=27)return 0;
    memcpy(&instruction,contents-4,4);
    switch(instruction>>26) {
    case 32: case 33: case 34: case 35: case 36: case 37: case 38:
    case 40: case 41: case 42: case 43: case 46: case 48:
    case 55: case 56: case 63:return 1;
    default:return 0;
    }
}
BYTE __stdcall mips_page_boundary_hazard(const BYTE *contents,DWORD distance)
{
    DWORD instruction,opcode,left,right,function;
    const BYTE *previous;
    if(distance<=4)return 0;
    memcpy(&instruction,contents+distance-4,4);
    opcode=instruction>>26;
    switch(opcode) {
    case 0:
        function=instruction&63;
        if(function==26 || function==27)return 1; /* DIV/DIVU at page end. */
        if(function!=8 && function!=9)return 0; /* JR/JALR. */
        right=0;
        break;
    case 1:
        right=(instruction>>16)&31;
        if(right>3 && (right<16 || right>19))return 0;
        right=0;
        break;
    case 2: case 3: /* Immediate J/JAL does not consume a source GPR. */
        if(distance>8)return mips_divide_follows_integer_memory(contents+distance-8);
        return 0;
    case 4: case 5: case 6: case 7:
    case 20: case 21: case 22: case 23:
        right=(instruction>>16)&31;
        break;
    default:return 0;
    }
    left=(instruction>>21)&31;
    previous=contents+distance-8;
    if(mips_memory_register_dependency(previous,left,right))return 1;
    if(distance>8) {
        if(mips_memory_register_dependency(contents+distance-12,left,right))return 1;
        if(mips_divide_follows_integer_memory(previous))return 1;
    }
    return 0;
}
BYTE __stdcall find_mips_code_padding(DWORD start,const BYTE *contents,DWORD bytes,
    DWORD page_size,DWORD *padding)
{
    DWORD candidate=start,distance;
    for(;;) {
        distance=page_size-(candidate&(page_size-1));
        while(distance<bytes) {
            if(mips_page_boundary_hazard(contents,distance))break;
            distance+=page_size;
        }
        if(distance>=bytes) {
            *padding=candidate-start;
            return 1;
        }
        candidate+=4;
        if(candidate-start>=page_size) {
            *padding=distance;
            return 0;
        }
    }
}
}
extern "C" DWORD __stdcall mips_code_padding(Contribution *contribution)
{
    OutputModule *module;
    DWORD bytes,offset,error,padding;
    int file;
    BYTE *contents;
    BYTE mapped;
    FileRecord *record;
    if(!(((contribution->rva+contribution->raw_size)^contribution->rva)&0xfffff000))return 0;
    if(!(contribution->characteristics&IMAGE_SCN_CNT_CODE))return 0;
    module=contribution->module;
    if(module==output_module)return 0;
    file=open_module_file(module,0x8000,0);
    bytes=contribution->raw_size-contribution->padding;
    if(!(module->library->flags&16))
        offset=(module->flags&0x40)?
            *(DWORD *)((BYTE *)module->optional_data+12):(DWORD)module->identity;
    else offset=0;
    offset+=contribution->input_raw_offset;
    contents=(BYTE *)map_file_range(file,offset,bytes);
    mapped=contents!=NULL;
    if(!mapped) {
        contents=(BYTE *)allocate_memory(bytes);
        if(!(module->library->flags&16))
            offset=(module->flags&0x40)?
                *(DWORD *)((BYTE *)module->optional_data+12):(DWORD)module->identity;
        else offset=0;
        offset+=contribution->input_raw_offset;
        error=0;seek_file(file,offset,FILE_BEGIN,&error);
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags&8)mapped_read(record,contents,bytes);
        else buffered_read(record,contents,bytes);
    }
    if(!find_mips_code_padding(contribution->rva,contents,bytes,0x1000,&padding))
        link_contribution_fatal(contribution,1166,padding,contribution->rva);
    if(!mapped)release_memory(contents);
    close_argument_file(file,0);
    return padding;
}

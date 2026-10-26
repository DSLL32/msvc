/* COFF function/file records describe each contiguous group of line entries. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include <recon.h>
/* This line-tracking owner is unrelated to the scoped 12-byte byte buffer.
 * The factory leaves its first DWORD untouched; its eventual writer owns it. */
/* Five encountered scalar stores initialize this independent 16-byte owner.
 * Its field roles will be named when the actual producers/consumers use them. */
struct DebugLineStore {
    WORD reserved00,reserved02;
    DWORD reserved04,reserved08,reserved0c;
};
typedef char DebugLineStoreSize[(sizeof(DebugLineStore)==16)?1:-1];
struct DebugLineModule {
    void *reserved00,*reserved04;
    DebugLineStore *lines;
};
typedef char DebugLineModuleSize[(sizeof(DebugLineModule)==12 &&
    offsetof(DebugLineModule,lines)==8)?1:-1];
extern "C" {
extern BYTE pdb_enabled;
DebugLineStore *__cdecl create_line_store(void)
{
    DebugLineStore *store=(DebugLineStore *)allocate_memory(sizeof(DebugLineStore));
    store->reserved00=0;
    store->reserved02=0;
    store->reserved04=0;
    store->reserved08=0;
    store->reserved0c=0;
    return store;
}
DebugLineModule *__cdecl create_line_module(void)
{
    DebugLineModule *module=(DebugLineModule *)allocate_memory(sizeof(DebugLineModule));
    module->reserved04=NULL;
    module->lines=create_line_store();
    return module;
}
#pragma intrinsic(strcmp)
MapLineFile *__stdcall find_map_line_file(OutputModule *module,const char *name)
{
    MapLineFile *file=module->map_line_files;
    while(file && strcmp(name,file->name))file=file->next;
    return file;
}
void __stdcall append_map_line(MapLineFile *file,OutputSection *section,
    DWORD base_line,DWORD line,DWORD offset)
{
    MapLineRecord record;
    record.line=(WORD)((WORD)line==0x7fff?base_line:line+base_line);
#ifdef __linux__
    /* This body never writes the two alignment bytes in the original stack
     * record. They have no line/section/offset role; zero the native padding
     * without asserting the original bytes or any wider consumer equality. */
    record.reserved=0;
#endif
    record.section=section;
    record.offset=offset;
    append_bytes(&file->lines,&record,sizeof(record));
}
void *__stdcall zero_allocate(unsigned);
char *__stdcall duplicate_argument(const char *);
void __stdcall record_map_lines(Contribution *contribution,const char *name,
    DWORD base_line,IMAGE_LINENUMBER *lines,DWORD count,DWORD first_offset)
{
    OutputModule *module=contribution->module;
    MapLineFile *file=find_map_line_file(module,name);
    OutputSection *section;
    DWORD index,input_rva,offset;
    if(!file) {
        file=(MapLineFile *)zero_allocate(sizeof(MapLineFile));
        file->name=duplicate_argument(name);
        file->next=module->map_line_files;
        module->map_line_files=file;
    }
    section=contribution->group->section;
    append_map_line(file,section,base_line,lines[0].Linenumber,first_offset);
    for(index=1;index<count;++index) {
        module=contribution->module;
        input_rva=module->section_map[
            contribution-(Contribution *)module->contributions].virtual_address;
        offset=contribution->rva-input_rva-contribution->group->section->rva+
            lines[index].Type.VirtualAddress;
        append_map_line(file,section,base_line,lines[index].Linenumber,offset);
    }
}
void __stdcall write_pdb_debug_lines(const char *,DWORD,DWORD,DWORD,DWORD,DWORD,
                                   IMAGE_LINENUMBER *,DWORD)
{ __asm { int 3 } }
void __stdcall write_local_debug_lines(const char *,DWORD,DWORD,DWORD,
                                      IMAGE_LINENUMBER *,DWORD,Contribution *)
{ __asm { int 3 } }
void __cdecl link_contribution_warning(Contribution *,unsigned,...);
}
#pragma intrinsic(memcpy)
extern "C" void __stdcall adjust_debug_lines(IMAGE_LINENUMBER *lines,DWORD count,
    Contribution *contribution,IMAGE_SYMBOL *symbols,DWORD symbol_count,
    DWORD first_file,BYTE write_debug,BYTE write_map)
{
    OutputModule *module=contribution->module;
    DWORD line=0,section_symbol,file_symbol,next_file,function,block,scan;
    DWORD input_rva,delta,start,end,group_count,next_line,j;
    WORD name_bytes,base_line;
    char local_name[260],*name;
    IMAGE_AUX_SYMBOL *aux;
    OutputSection *section;
    if(!pdb_enabled && !module->debug_stream)module->debug_stream=create_line_module();
    if(lines[0].Linenumber) {
        /* Some producers describe section-relative lines without a leading
         * function marker. Find that section and its enclosing FILE record. */
        for(section_symbol=0;section_symbol<symbol_count;
            section_symbol+=symbols[section_symbol].NumberOfAuxSymbols+1) {
            IMAGE_SYMBOL *symbol=symbols+section_symbol;
            aux=(IMAGE_AUX_SYMBOL *)(symbol+1);
            if(symbol->StorageClass==IMAGE_SYM_CLASS_STATIC &&
               symbol->NumberOfAuxSymbols==1 && aux->Section.NumberOfLinenumbers &&
               module->section_map[(contribution-(Contribution *)module->contributions)].virtual_address+
               symbol->Value-contribution->rva==lines[0].Type.VirtualAddress)break;
        }
        if(section_symbol==symbol_count)goto invalid_lines;
        for(file_symbol=0;file_symbol<symbol_count &&
            symbols[file_symbol].StorageClass!=IMAGE_SYM_CLASS_FILE;
            file_symbol+=symbols[file_symbol].NumberOfAuxSymbols+1) {}
        if(file_symbol==symbol_count)goto invalid_lines;
        next_file=symbols[file_symbol].Value;
        while(next_file && file_symbol<symbol_count && next_file<=section_symbol) {
            file_symbol=next_file;
            next_file=symbols[file_symbol].Value;
        }
        if(file_symbol==symbol_count)goto invalid_lines;
        aux=(IMAGE_AUX_SYMBOL *)(symbols+section_symbol+1);
        group_count=aux->Section.NumberOfLinenumbers;
        if(group_count>count)goto invalid_lines;
        input_rva=module->section_map[
            contribution-(Contribution *)module->contributions].virtual_address;
        section=contribution->group->section;
        delta=contribution->rva-input_rva-section->rva;
        name_bytes=(WORD)(symbols[file_symbol].NumberOfAuxSymbols*18);
        name=name_bytes>=sizeof(local_name)?
            (char *)allocate_memory(name_bytes+1):local_name;
        memcpy(name,symbols+file_symbol+1,name_bytes);
        name[name_bytes]=0;
        if(write_debug) {
            end=delta+aux->Section.Length;
            if(pdb_enabled)write_pdb_debug_lines(name,section->index,delta,end,
                contribution->rva-input_rva-section->rva,0,lines,group_count*6);
            else write_local_debug_lines(name,delta,end,0,lines,group_count*6,contribution);
        }
        if(write_map)record_map_lines(contribution,name,0,lines,group_count,delta);
        if(name!=local_name)release_memory(name);
        return;
    }
    scan=0;
    file_symbol=first_file;
    while(line<count) {
        IMAGE_LINENUMBER *group=lines+line;
        if(!group->Linenumber) {
            function=group->Type.SymbolTableIndex;
            if(function<=first_file || function+1>=symbol_count ||
               symbols[function].NumberOfAuxSymbols<1)goto invalid_lines;
            section=contribution->group->section;
            start=symbols[function].Value-section->rva;
            block=((IMAGE_AUX_SYMBOL *)(symbols+function+1))->Sym.TagIndex;
            if(block<=function || block+1>=symbol_count ||
               symbols[block].StorageClass!=IMAGE_SYM_CLASS_FUNCTION ||
               symbols[block].NumberOfAuxSymbols<1)goto invalid_lines;
            file_symbol=first_file;
            for(;;) {
                if(file_symbol+1>=symbol_count ||
                   symbols[file_symbol].SectionNumber!=IMAGE_SYM_DEBUG ||
                   symbols[file_symbol].StorageClass!=IMAGE_SYM_CLASS_FILE ||
                   symbols[file_symbol].NumberOfAuxSymbols<1)goto invalid_lines;
                next_file=symbols[file_symbol].Value;
                if(next_file<=file_symbol || next_file>=function)break;
                file_symbol=next_file;
            }
            scan=block;
        } else {
            module=contribution->module;
            input_rva=module->section_map[
                contribution-(Contribution *)module->contributions].virtual_address;
            start=contribution->rva-input_rva-contribution->group->section->rva+
                group->Type.VirtualAddress;
        }
        scan+=symbols[scan].NumberOfAuxSymbols+1;
        if(scan>=symbol_count)goto invalid_lines;
        for(;;) {
            IMAGE_SYMBOL *symbol=symbols+scan;
            if(symbol->StorageClass==IMAGE_SYM_CLASS_FUNCTION &&
               !strncmp((const char *)symbol->N.ShortName,".lf",3))break;
            if(symbol->StorageClass==IMAGE_SYM_CLASS_FILE)file_symbol=scan;
            scan+=symbol->NumberOfAuxSymbols+1;
            if(scan>=symbol_count)goto invalid_lines;
        }
        group_count=symbols[scan].Value;
        if(!group_count)continue;
        for(j=1;j<group_count;++j)
            if(!group[j].Linenumber)group_count=j;
        next_line=line+group_count;
        if(next_line>count)goto invalid_lines;
        aux=(IMAGE_AUX_SYMBOL *)(symbols+function+1);
        if(aux->Sym.Misc.TotalSize) {
            name_bytes=(WORD)(symbols[file_symbol].NumberOfAuxSymbols*18);
            name=name_bytes>=sizeof(local_name)?
                (char *)allocate_memory(name_bytes+1):local_name;
            memcpy(name,symbols+file_symbol+1,name_bytes);
            name[name_bytes]=0;
            /* The map recorder consumes this as a WORD line number. The
             * PDB writer still has an unqualified full-DWORD transport; its
             * named pending boundary gives no PDB-enabled equivalence. */
            base_line=((IMAGE_AUX_SYMBOL *)(symbols+block+1))->Sym.Misc.LnSz.Linenumber;
            if(write_debug) {
                if(next_line<count && lines[next_line].Linenumber) {
                    module=contribution->module;
                    input_rva=module->section_map[
                        contribution-(Contribution *)module->contributions].virtual_address;
                    end=contribution->rva-input_rva-contribution->group->section->rva+
                        lines[next_line].Type.VirtualAddress-1;
                } else end=start+aux->Sym.Misc.TotalSize-1;
                section=contribution->group->section;
                module=contribution->module;
                input_rva=module->section_map[
                    contribution-(Contribution *)module->contributions].virtual_address;
                delta=contribution->rva-input_rva-section->rva;
                if(pdb_enabled)write_pdb_debug_lines(name,section->index,start,end,delta,
                    base_line,group,group_count*6);
                else write_local_debug_lines(name,start,end,base_line,group,group_count*6,contribution);
            }
            if(write_map)record_map_lines(contribution,name,base_line,group,group_count,start);
            if(name!=local_name)release_memory(name);
        }
        line=next_line;
    }
    return;
invalid_lines:
    link_contribution_warning(contribution,4200);
}

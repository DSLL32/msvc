/* COFF function/file records describe each contiguous group of line entries. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include "pass-one.h"
#include <recon.h>
/* This line-tracking owner is unrelated to the scoped 12-byte byte buffer.
 * The factory leaves its first DWORD untouched; its eventual writer owns it. */
struct DebugLinePool {
    DebugLinePool *next;
    BYTE *data;
    DWORD used;
    BYTE storage[1012];
};
typedef char DebugLinePoolSize[(sizeof(DebugLinePool)==1024 &&
    offsetof(DebugLinePool,storage)==12)?1:-1];
struct DebugLineEntry {
    DebugLineEntry *next;
    DWORD offset;
    WORD line,reserved;
};
typedef char DebugLineEntrySize[(sizeof(DebugLineEntry)==12 &&
    offsetof(DebugLineEntry,line)==8)?1:-1];
struct DebugLineSection {
    DebugLineSection *next;
    WORD section,line_count;
    DebugLineEntry *lines;
    DWORD serialized_size,start,end;
};
typedef char DebugLineSectionSize[(sizeof(DebugLineSection)==24 &&
    offsetof(DebugLineSection,start)==16)?1:-1];
struct DebugLineFile {
    DebugLineFile *next;
    WORD section_count,name_length;
    DWORD serialized_size;
    char *name;
    DebugLineSection *sections;
};
typedef char DebugLineFileSize[(sizeof(DebugLineFile)==20 &&
    offsetof(DebugLineFile,name)==12 && offsetof(DebugLineFile,sections)==16)?1:-1];
struct DebugLineStore {
    WORD file_count,segment_count;
    DebugLineFile *files;
    DebugLinePool *pool;
    struct DebugLineSegment *segments;
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
extern BYTE incremental_reuse;
unsigned __stdcall count_debug_modules(void *first_library)
{
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    unsigned count=0;
    initialize_library_enumerator(&libraries,(OutputLibrary *)first_library);
    while(next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        while(next_module(&modules))
            if(!incremental_reuse || (modules.current->flags&1))++count;
    }
    return count;
}
DebugLineStore *__cdecl create_line_store(void)
{
    DebugLineStore *store=(DebugLineStore *)allocate_memory(sizeof(DebugLineStore));
    store->file_count=0;
    store->segment_count=0;
    store->files=NULL;
    store->pool=NULL;
    store->segments=NULL;
    return store;
}
DebugLineModule *__cdecl create_line_module(void)
{
    DebugLineModule *module=(DebugLineModule *)allocate_memory(sizeof(DebugLineModule));
    module->reserved04=NULL;
    module->lines=create_line_store();
    return module;
}
#pragma intrinsic(strcmp,strlen,strcpy)
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
DebugLinePool *current_debug_line_pool;
DebugLinePool *__cdecl create_debug_line_pool(void)
{
    DebugLinePool *pool=(DebugLinePool *)allocate_memory(sizeof(DebugLinePool));
    pool->next=NULL;
    pool->data=pool->storage;
    pool->used=0;
    return pool;
}
void __stdcall select_debug_line_pool(DebugLineStore *store)
{
    DebugLinePool *pool=store->pool;
    if(!pool)store->pool=pool=create_debug_line_pool();
    current_debug_line_pool=pool;
    while(pool->next) {
        pool=pool->next;
        current_debug_line_pool=pool;
    }
}
void *__stdcall allocate_debug_line_record(unsigned bytes)
{
    DWORD rounded=(bytes+3)&~3U;
    DebugLinePool *pool=current_debug_line_pool;
    BYTE *result;
    if(pool->used+rounded>sizeof(pool->storage)) {
        pool=create_debug_line_pool();
        current_debug_line_pool->next=pool;
        current_debug_line_pool=pool;
    }
    result=pool->data+pool->used;
    pool->used+=rounded;
    return result;
}
char *__stdcall copy_debug_line_name(const char *name,DWORD length)
{
    char *copy=(char *)allocate_debug_line_record(length+2);
    strcpy(copy,name);
    return copy;
}
DebugLineFile *__stdcall find_debug_line_file(DebugLineStore *store,const char *name)
{
    DebugLineFile **link=&store->files,*file;
    DWORD length;
    while(*link) {
        if(!strcmp(name,(*link)->name))return *link;
        link=&(*link)->next;
    }
    *link=file=(DebugLineFile *)allocate_debug_line_record(sizeof(DebugLineFile));
    ++store->file_count;
    file->section_count=0;
    length=strlen(name);
    file->name_length=(WORD)length;
    file->name=copy_debug_line_name(name,length);
    file->sections=NULL;
    file->next=NULL;
    return file;
}
DebugLineSection *__stdcall find_debug_line_section(DebugLineFile *file,WORD section,
    DWORD start,DWORD end)
{
    DebugLineSection **link=&file->sections,*group;
    while(*link) {
        group=*link;
        if(group->section==section && group->end<start && group->end+4>=start) {
            group->end=end;
            return group;
        }
        link=&group->next;
    }
    *link=group=(DebugLineSection *)allocate_debug_line_record(sizeof(DebugLineSection));
    ++file->section_count;
    group->start=start;
    group->section=section;
    group->lines=NULL;
    group->line_count=0;
    group->end=end;
    group->next=NULL;
    return group;
}
void __stdcall append_debug_line(DebugLineSection *section,DWORD base_line,
    DWORD line,DWORD offset)
{
    DebugLineEntry *entry=(DebugLineEntry *)allocate_debug_line_record(sizeof(DebugLineEntry));
    entry->offset=offset;
    entry->next=section->lines;
    /* Both physical DWORD inputs are consumed only through their low WORDs. */
    entry->line=(WORD)((WORD)line==0x7fff?(WORD)base_line:
        (WORD)base_line+(WORD)line);
    ++section->line_count;
    section->lines=entry;
}
void __stdcall write_local_debug_lines(const char *name,DWORD start,DWORD end,
    DWORD base_line,IMAGE_LINENUMBER *lines,DWORD bytes,Contribution *contribution)
{
    DebugLineStore *store=((DebugLineModule *)contribution->module->debug_stream)->lines;
    WORD section=contribution->group->section->index;
    DWORD count=bytes/sizeof(*lines),index,delta,first;
    DebugLineFile *file;
    DebugLineSection *group;
    OutputModule *module;
    select_debug_line_pool(store);
    file=find_debug_line_file(store,name);
    module=contribution->module;
    delta=contribution->rva-module->section_map[
        contribution-(Contribution *)module->contributions].virtual_address-
        contribution->group->section->rva;
    first=lines[0].Linenumber?lines[0].Type.VirtualAddress+delta:start;
    group=find_debug_line_section(file,section,first,end);
    append_debug_line(group,base_line,lines[0].Linenumber,first);
    for(index=1;index<count;++index)
        append_debug_line(group,base_line,lines[index].Linenumber,
            lines[index].Type.VirtualAddress+delta);
}
struct DebugLineSegment {
    DebugLineSegment *next;
    WORD section,reserved06;
    DWORD reserved08,reserved0c,start,end;
};
typedef char DebugLineSegmentSize[(sizeof(DebugLineSegment)==24 &&
    offsetof(DebugLineSegment,start)==16)?1:-1];
DebugLineSegment *__cdecl create_debug_line_segment(void)
{
    DebugLineSegment *segment=(DebugLineSegment *)allocate_debug_line_record(sizeof(DebugLineSegment));
    segment->section=0xffff;
    segment->next=NULL;
    segment->start=0xffffffff;
    segment->end=0;
    segment->reserved0c=0;
    return segment;
}
void __stdcall build_debug_line_segments(DebugLineStore *store)
{
    DebugLineFile *file;
    DebugLineSection *group;
    select_debug_line_pool(store);
    if(store->segments)return;
    store->segments=create_debug_line_segment();
    store->segment_count=0;
    for(file=store->files;file;file=file->next) {
        for(group=file->sections;group;group=group->next) {
            DebugLineSegment *segment=store->segments,*previous=NULL;
            BYTE merged=0;
            while(segment) {
                if(segment->section==group->section) {
                    if(group->start<segment->start && group->start+4>=segment->start) {
                        segment->start=group->start;
                        merged=1;
                        break;
                    }
                    if(group->end>segment->end && group->end-4<=segment->end) {
                        segment->end=group->end;
                        merged=1;
                        break;
                    }
                }
                previous=segment;
                segment=segment->next;
            }
            if(!merged) {
                ++store->segment_count;
                if(previous->section==0xffff)segment=previous;
                else previous->next=segment=create_debug_line_segment();
                segment->start=group->start;
                segment->section=group->section;
                segment->end=group->end;
            }
        }
    }
}
DWORD __stdcall debug_line_file_size(DebugLineFile *file)
{
    DebugLineSection *section;
    DWORD bytes=file->name_length+file->section_count*12+5;
    bytes+=(-(DWORD)(file->name_length+1))&3;
    for(section=file->sections;section;section=section->next) {
        DWORD size=section->line_count*6+4;
        if(section->line_count&1)size+=2;
        section->serialized_size=size;
        bytes+=size;
    }
    file->serialized_size=bytes;
    return bytes;
}
DWORD __stdcall debug_stream_size(DebugLineModule *module)
{
    DebugLineStore *store;
    DebugLineFile *file;
    DWORD bytes;
    if(!module)return 0;
    store=module->lines;
    if(!store->files)return 0;
    build_debug_line_segments(store);
    bytes=4+store->file_count*4+store->segment_count*10;
    if(store->segment_count&1)bytes+=2;
    for(file=store->files;file;file=file->next)bytes+=debug_line_file_size(file);
    return bytes;
}
void __stdcall serialize_debug_line_section(DebugLineSection *section,BYTE *buffer)
{
    DebugLineEntry *entry;
    BYTE *offsets=buffer+4+section->line_count*4;
    BYTE *lines=offsets+section->line_count*2;
    *(WORD *)buffer=section->section;
    *(WORD *)(buffer+2)=section->line_count;
    for(entry=section->lines;entry;entry=entry->next) {
        offsets-=4;
        *(DWORD *)offsets=entry->offset;
    }
    for(entry=section->lines;entry;entry=entry->next) {
        lines-=2;
        *(WORD *)lines=entry->line;
    }
    if(section->line_count&1)*(WORD *)(buffer+4+section->line_count*6)=0;
}
#pragma intrinsic(memcpy,memset)
void __stdcall serialize_debug_line_file(DebugLineFile *file,BYTE *buffer,DWORD base_offset)
{
    BYTE *cursor=buffer+4;
    DebugLineSection *section;
    DWORD padding=(-(DWORD)(file->name_length+1))&3;
    DWORD offset=base_offset+5+file->section_count*12+file->name_length+padding;
    *(WORD *)buffer=file->section_count;
    *(WORD *)(buffer+2)=0;
    for(section=file->sections;section;section=section->next) {
        *(DWORD *)cursor=offset;
        cursor+=4;
        offset+=section->serialized_size;
    }
    for(section=file->sections;section;section=section->next) {
        *(DWORD *)cursor=section->start;
        *(DWORD *)(cursor+4)=section->end;
        cursor+=8;
    }
    /* The stored length is one byte, but the copy consumes the full WORD. */
    *cursor++=(BYTE)file->name_length;
    memcpy(cursor,file->name,file->name_length);
    cursor+=file->name_length;
    memset(cursor,0,padding);
    cursor+=padding;
    for(section=file->sections;section;section=section->next) {
        serialize_debug_line_section(section,cursor);
        cursor+=section->serialized_size;
    }
}
void __stdcall copy_debug_stream(DebugLineModule *module,void *buffer,DWORD bytes)
{
    DebugLineStore *store=module->lines;
    BYTE *base=(BYTE *)buffer,*cursor=base+4;
    DebugLineFile *file;
    DebugLineSegment *segment;
    DWORD offset=4+store->file_count*4+store->segment_count*10;
    *(WORD *)base=store->file_count;
    *(WORD *)(base+2)=store->segment_count;
    if(store->segment_count&1)offset+=2;
    for(file=store->files;file;file=file->next) {
        *(DWORD *)cursor=offset;
        cursor+=4;
        offset+=debug_line_file_size(file);
    }
    for(segment=store->segments;segment;segment=segment->next) {
        *(DWORD *)cursor=segment->start;
        *(DWORD *)(cursor+4)=segment->end;
        cursor+=8;
    }
    for(segment=store->segments;segment;segment=segment->next) {
        *(WORD *)cursor=segment->section;
        cursor+=2;
    }
    /* The original advances over this alignment slot without writing it. */
    if(store->segment_count&1)cursor+=2;
    for(file=store->files;file;file=file->next) {
        serialize_debug_line_file(file,cursor,cursor-base);
        cursor+=file->serialized_size;
    }
}
void __stdcall release_debug_line_pools(DebugLineStore *store)
{
    DebugLinePool *pool=store->pool;
    while(pool) {
        DebugLinePool *next=pool->next;
        release_memory(pool);
        pool=next;
    }
}
void __stdcall destroy_debug_line_store(DebugLineStore *store)
{
    release_debug_line_pools(store);
    store->files=NULL;
    release_memory(store);
}
void __stdcall destroy_debug_line_module(DebugLineModule *module)
{
    if(module) {
        destroy_debug_line_store(module->lines);
        release_memory(module);
    }
}
void __stdcall release_debug_stream(DebugLineModule *module)
{
    destroy_debug_line_module(module);
}
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

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "link-driver.h"
#include "link.h"
#include "full-link-functions.h"
#include "files.h"
#include "vxd.h"
#include "memory.h"
#include "symbols.h"
#include "debug-output.h"
#include <recon.h>

#pragma intrinsic(memset,memcpy,strlen,strcpy)
#undef toupper
extern "C" BYTE file_access_statistics;
/* These are the actual scalar defaults of the encountered LE header owner. */
VxdImageHeader vxd_header={
    0x454c,0,0,0,2,4,0,0x8000,0, /* Signature, CPU/OS, flags, page count. */
    0,0,0,0,                    /* Entry and stack. */
    0,0,0,0,                    /* Pages and fixup size/checksum. */
    0,0,0,0,0,                  /* Loader and object tables. */
    0,0,0,0,0,                  /* Iterated pages, resources, names, entries. */
    0,0,0,0,                    /* Directives and fixup tables. */
    0,0,0,0,                    /* Imports and page checksums. */
    0,0,0,0,                    /* Data pages and nonresident names. */
    0,0,0,0,                    /* Automatic data and debug records. */
    0,0,0,{0},0,0,0,0x400       /* Instance pages, heap, resources, DDK. */
};

void RECON_THISCALL LinkContext::VxdWriteSection(int file,void *section,void *header)
{
    OutputSection *output=(OutputSection *)section;
    VxdObjectRecord object;
    FileRecord *record;
    DWORD pages,first_page,index,error=0,saved;
    BYTE page[4];
    if(!vxd_page_count)vxd_first_data=output->file_offset;
    object.virtual_size=output->virtual_size;
    object.base=0;
    object.flags=0;
    if(output->characteristics & IMAGE_SCN_MEM_READ)object.flags|=1;
    if(output->characteristics & IMAGE_SCN_MEM_WRITE)object.flags|=2;
    if(output->characteristics & 0x10000000)object.flags|=0x20;
    if(output->characteristics & IMAGE_SCN_MEM_EXECUTE)object.flags|=4;
    object.flags|=(output->characteristics & 0x20000) ? 0x1000 : 0x2000;
    if(output->characteristics & 0x40000)object.flags|=0x200;
    if(output->vxd_flag10)object.flags|=0x10;
    if(output->vxd_flag40)object.flags|=0x40;
    if(output->vxd_flag8000)object.flags|=0x8000;
    if(output->vxd_flag4000)object.flags|=0x4000;
    object.page_map_index=vxd_page_count+1;
    pages=(output->virtual_size+optional_header.SectionAlignment-1)/
        optional_header.SectionAlignment;
    object.page_count=pages;
    memset(object.name,0,sizeof(object.name));
    strncpy(object.name,output->name,sizeof(object.name));
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_write(record,&object,sizeof(object));
    else buffered_write(record,&object,sizeof(object));
    vxd_header.last_page_size=output->virtual_size-
        (pages-1)*optional_header.SectionAlignment;
    if(output->vxd_flag40)vxd_header.preload_pages+=pages;
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    saved=(record->flags & 8) ? record->file_size : ((FileBufferHeader *)record->base)->cursor;
    seek_file(file,vxd_page_map_cursor,FILE_BEGIN,&error);
    first_page=(output->file_offset-vxd_first_data)/optional_header.SectionAlignment+1;
    pages=(output->raw_size+optional_header.SectionAlignment-1)/
        optional_header.SectionAlignment;
    page[3]=0;
    for(index=0;index<pages;++index,++first_page) {
        page[0]=(BYTE)(first_page>>16);
        page[1]=(BYTE)(first_page>>8);
        page[2]=(BYTE)first_page;
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags & 8)mapped_write(record,page,sizeof(page));
        else buffered_write(record,page,sizeof(page));
    }
    vxd_page_map_cursor+=pages*4;
    vxd_page_count+=pages;
    error=0;
    seek_file(file,saved,FILE_BEGIN,&error);
}

extern "C" {
extern const char *vxd_module_name,*output_file_name;
}
void RECON_THISCALL LinkContext::WriteVxdModuleTable(int file)
{
    char name[260];
    BYTE length;
    WORD index;
    DWORD error=0;
    FileRecord *record;
    if(*vxd_module_name) {
#ifdef __linux__
        if(strlen(vxd_module_name)>=sizeof(name))
            recon_unsupported("VxD module-name local extent");
#endif
        name[0]=0;
        strcpy(name,vxd_module_name);
    } else _splitpath(output_file_name,NULL,NULL,name,NULL);
    length=(BYTE)strlen(name);
    for(index=0;index<length;++index)name[index]=(char)(toupper)((signed char)name[index]);
    name[index++]=0;
    name[index++]=0;
    name[index++]=0;
    vxd_resident_names=vxd_page_map_cursor;
    seek_file(file,vxd_page_map_cursor,FILE_BEGIN,&error);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_write(record,&length,1);
    else buffered_write(record,&length,1);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags & 8)mapped_write(record,name,index);
    else buffered_write(record,name,index);
    vxd_page_map_cursor+=index+1;
}

extern "C" {
extern InputArguments export_arguments;
void __cdecl link_unresolved_error(const char *,unsigned,...);
}
#pragma pack(push,1)
struct VxdEntryBundle { BYTE count,type; WORD object; };
struct VxdEntry { BYTE flags; DWORD offset; BYTE reserved; };
#pragma pack(pop)
typedef char VxdEntrySizes[(sizeof(VxdEntryBundle)==4 && sizeof(VxdEntry)==6)?1:-1];

/* The provisional method name denotes the LE entry bundles, not a second
 * independently stored export list. The real export_arguments owner is reused. */
void RECON_THISCALL LinkContext::WriteVxdNames(int file)
{
    WORD *objects=(WORD *)allocate_memory(export_arguments.count*2);
    StringNode **entries=(StringNode **)allocate_memory(export_arguments.count*4);
    InputArgument *argument;
    StringNode *symbol;
    Contribution *contribution;
    OutputSection *section;
    WORD i,j;
    DWORD missing=0,error=0;
    char *copy,*comma;
    BYTE created;
    FileRecord *record;
    VxdEntryBundle bundle;
    VxdEntry entry;
    vxd_entry_table=vxd_page_map_cursor;
    seek_file(file,vxd_page_map_cursor,FILE_BEGIN,&error);
    argument=export_arguments.first;
    for(i=0;i<export_arguments.count;++i) {
        copy=(char *)allocate_memory(strlen(argument->name)+1);
        strcpy(copy,argument->name);
        comma=strchr(copy,',');
        if(comma)*comma=0;
        created=0;
        symbol=symbols->LookupSymbol((BYTE)strlen(copy)<=8,copy,&created);
        if(created) {
            objects[i]=0xffff;
            link_unresolved_error(NULL,2001,copy);
            ++missing;
        } else {
            contribution=symbol->contribution;
            while(contribution && (contribution->characteristics & 0x4000))
                contribution=contribution->associated_contribution;
            objects[i]=contribution->group->section->index;
            entries[i]=symbol;
        }
        release_memory(copy);
        argument=argument->next;
    }
    if((WORD)missing)link_fatal(output_file_name,1120,(WORD)missing);
#define WRITE_VXD(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[file]; \
    if(record->flags & 8)mapped_write(record,(data),(bytes)); \
    else buffered_write(record,(data),(bytes)); \
} while(0)
    for(i=0;i<export_arguments.count;++i) {
        if(objects[i]==0xffff)continue;
        bundle.count=1;
        bundle.type=3;
        bundle.object=objects[i];
        for(j=i+1;j<export_arguments.count;++j) {
            if(objects[j]==objects[i]) { objects[j]=0xffff; ++bundle.count; }
        }
        WRITE_VXD(&bundle,sizeof(bundle));
        for(j=0;j<export_arguments.count;++j) {
            symbol=entries[j];
            contribution=symbol->contribution;
            while(contribution && (contribution->characteristics & 0x4000))
                contribution=contribution->associated_contribution;
            section=contribution->group->section;
            if(section->index!=objects[i])continue;
            entry.flags=3;
            entry.offset=symbol->symbol.Value-section->rva+contribution->rva;
            entry.reserved=0;
            WRITE_VXD(&entry,sizeof(entry));
        }
        objects[i]=0xffff;
    }
#undef WRITE_VXD
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    vxd_page_map_cursor=(record->flags & 8) ? record->file_size :
        ((FileBufferHeader *)record->base)->cursor;
    release_memory(objects);
    release_memory(entries);
}

extern "C" {
extern BYTE *base_relocations,*next_base_relocation;
extern int output_file;
extern BYTE pdb_enabled;
OutputSection *__stdcall find_section_by_index(SHORT,SectionList *);
/* Named pending boundaries: no successful lookup/restart implementation yet. */
VxdFixup *__stdcall find_vxd_fixup(VxdPageFixups *page,BYTE target,LONG offset,BYTE type)
{
    VxdFixup *fixup;
    for(fixup=page->first;fixup;fixup=fixup->next) {
        if(fixup->target_offset==offset && fixup->target_object==target &&
           fixup->type==type && fixup->count!=255)return fixup;
    }
    return NULL;
}
DWORD __stdcall relink_with_header_size(DWORD bytes)
{ __asm { int 3 } return 0; }
}

void RECON_THISCALL LinkContext::WriteVxdRelocations(void)
{
    VxdPageFixups *pages;
    LinkBaseRelocation *relocation=(LinkBaseRelocation *)base_relocations;
    VxdFixup *fixup,*next;
    VxdPageFixups *page;
    OutputSection *section;
    FileRecord *record;
    BYTE previous_section=0,repeat=0,source_object,target_object,type;
    DWORD previous_page,section_page,page_start,site,bytes=0,error=0,saved;
    DWORD page_number,page_index,i,offset,used;
    LONG target,page_relative,page_global;
    BYTE *buffer,*cursor;
    WORD source;
#ifdef __linux__
    if(vxd_page_count>=65535)recon_unsupported("VxD fixup WORD page iteration");
#endif
    pages=(VxdPageFixups *)zero_allocate(vxd_page_count*8);
    while((BYTE *)relocation<next_base_relocation) {
        site=relocation->address&0xffffff;
        source_object=(BYTE)(relocation->address>>24);
        target_object=(BYTE)(relocation->value>>24);
        target=(LONG)(relocation->value<<8)>>8;
        type=relocation->type==8 ? 8 : 7;
        if(previous_section!=source_object) {
            previous_section=source_object;
            section=find_section_by_index(source_object,&sections);
            section_page=(section->file_offset-vxd_first_data)/optional_header.SectionAlignment;
            previous_page=0xffff;
        }
#ifdef __linux__
        /* The real producers provide positive section indexes. No section
         * base has been established on the initial zero-index path. */
        if(!previous_section)recon_unsupported("VxD fixup missing source object");
#endif
        page_number=site/optional_header.SectionAlignment+(repeat ? 1 : 0);
        page_relative=(SHORT)page_number;
        if((previous_page&0xffff)!=(DWORD)page_relative) {
            previous_page=page_number;
            page_global=(section_page&0xffff)+page_relative;
#ifdef __linux__
            if(page_global<0 || (DWORD)page_global>=vxd_page_count)
                recon_unsupported("VxD fixup page extent");
#endif
            page=pages+page_global;
            page_start=page_relative*optional_header.SectionAlignment;
        }
        fixup=find_vxd_fixup(page,target_object,target,type);
        if(!fixup) {
            fixup=(VxdFixup *)allocate_memory(sizeof(VxdFixup));
            fixup->next=page->first;
            page->first=fixup;
            fixup->type=type;
            fixup->target_object=target_object;
            fixup->target_offset=target;
            fixup->count=0;
            fixup->capacity=16;
            fixup->sources=(SHORT *)allocate_memory(32);
            bytes+=(DWORD)fixup->target_offset<0x10000 ? 9 : 11;
        } else {
            if(fixup->count==1)--bytes;
            bytes+=2;
            if(fixup->count>=fixup->capacity) {
                fixup->capacity+=16;
                fixup->sources=(SHORT *)reallocate_memory(fixup->sources,fixup->capacity*2);
            }
        }
        fixup->sources[fixup->count++]=(SHORT)(site-page_start);
        repeat=(page_start+optional_header.SectionAlignment<site+4);
        if(!repeat)++relocation;
    }
    buffer=(BYTE *)allocate_memory(bytes);
    cursor=buffer;
#define WRITE_VXD(data,length) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[output_file]; \
    if(record->flags & 8)mapped_write(record,(data),(length)); \
    else buffered_write(record,(data),(length)); \
} while(0)
    if(file_access_statistics)record_file_access();
    record=file_slots[output_file];
    saved=(record->flags & 8) ? record->file_size : ((FileBufferHeader *)record->base)->cursor;
    error=0;
    seek_file(output_file,vxd_page_map_cursor,FILE_BEGIN,&error);
    vxd_fixup_pages=vxd_page_map_cursor;
    for(i=1;(WORD)i<=vxd_page_count;++i) {
        offset=cursor-buffer;
        pages[i-1].offset=offset;
        WRITE_VXD(&offset,4);
        for(fixup=pages[i-1].first;fixup;fixup=fixup->next) {
            BYTE wide=(DWORD)fixup->target_offset>=0x10000;
            if(fixup->count>1)fixup->type|=0x20;
            *cursor++=fixup->type;
            *cursor++=wide ? 0x10 : 0;
            if(fixup->count==1) {
                memcpy(cursor,fixup->sources,2); cursor+=2;
            } else *cursor++=fixup->count;
            *cursor++=fixup->target_object;
            if(wide) { memcpy(cursor,&fixup->target_offset,4);cursor+=4; }
            else { source=(WORD)fixup->target_offset;memcpy(cursor,&source,2);cursor+=2; }
            if(fixup->count>1) {
                for(page_index=0;page_index<fixup->count;++page_index) {
                    memcpy(cursor,fixup->sources+page_index,2);cursor+=2;
                }
            }
        }
    }
    used=cursor-buffer;
    WRITE_VXD(&used,4);
    if(file_access_statistics)record_file_access();
    record=file_slots[output_file];
    vxd_fixup_records=(record->flags & 8) ? record->file_size :
        ((FileBufferHeader *)record->base)->cursor;
    if(vxd_fixup_records+used>optional_header.BaseOfCode) {
        release_memory(buffer);
        if(pdb_enabled)close_pdb();
        ExitProcess(relink_with_header_size(vxd_fixup_records+used));
    }
    WRITE_VXD(buffer,used);
    if(file_access_statistics)record_file_access();
    record=file_slots[output_file];
    vxd_page_map_cursor=(record->flags & 8) ? record->file_size :
        ((FileBufferHeader *)record->base)->cursor;
    error=0;
    seek_file(output_file,saved,FILE_BEGIN,&error);
    release_memory(buffer);
    for(i=1;(WORD)i<=vxd_page_count;++i) {
        for(fixup=pages[i-1].first;fixup;fixup=next) {
            next=fixup->next;
            release_memory(fixup->sources);
            release_memory(fixup);
        }
    }
    release_memory(pages);
#undef WRITE_VXD
}

extern "C" {
extern ByteBuffer image_comments;
extern StringNode *entry_symbol;
extern Contribution *debug_directory,*codeview_debug;
extern const char *pdb_file_name;
}
/* This consumer reads the 80-byte ordinal-1 payload. It does not establish
 * that the chosen input exports an operational VxD device descriptor. */
struct VxdDdbPrefix { DWORD next; WORD sdk_version,device_id; BYTE reserved08[72]; };
typedef char VxdDdbReadSize[(sizeof(VxdDdbPrefix)==80)?1:-1];

void RECON_THISCALL LinkContext::VxdWriteHeaders(int file)
{
    SectionEnumerator enumerator;
    OutputSection *last=NULL,*section;
    Contribution *contribution;
    StringNode *symbol,*device=NULL;
    InputArgument *argument;
    FileRecord *record;
    DWORD error=0,nonresident_start,position,address,missing=0;
    WORD offset,length,index=0,ordinal=0;
    BYTE count,created,zero=0;
    char *copy,*comma;
    long specified_ordinal;
    IMAGE_DEBUG_DIRECTORY directory;
    VxdDdbPrefix ddb;
    initialize_section_enumerator(&enumerator,&sections);
    while(next_output_section(&enumerator)) {
        section=enumerator.current;
        if(!last || (SHORT)section->index>(SHORT)last->index)last=section;
    }
    nonresident_start=(last->reserved08+3)&~3UL;
    seek_file(file,nonresident_start,FILE_BEGIN,&error);
#define HEADER_WRITE(fd,data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[(fd)]; \
    if(record->flags & 8)mapped_write(record,(data),(bytes)); \
    else buffered_write(record,(data),(bytes)); \
} while(0)
#define HEADER_TELL(fd,destination) do { \
    if(file_access_statistics)record_file_access(); \
    record=file_slots[(fd)]; \
    (destination)=(record->flags & 8) ? record->file_size : \
        ((FileBufferHeader *)record->base)->cursor; \
} while(0)
    for(offset=0;offset<image_comments.used;offset=(WORD)(offset+length+1)) {
        length=(WORD)strlen((char *)image_comments.data+offset);
        count=length>=255 ? 255 : (BYTE)length;
        HEADER_WRITE(file,&count,1);
        HEADER_WRITE(file,image_comments.data+offset,count);
        HEADER_WRITE(file,&ordinal,2);
        ++ordinal;
    }
    argument=export_arguments.first;
    for(index=0;index<export_arguments.count;++index,argument=argument->next) {
        copy=(char *)allocate_memory(strlen(argument->name)+1);
        strcpy(copy,argument->name);
        comma=strchr(copy,',');
        if(comma)*comma=0;
        count=(BYTE)strlen(copy);
        created=0;
        symbol=symbols->Lookup((unsigned char *)copy,&created);
        if(!device && comma && comma[1]=='@') {
#ifdef __linux__
            if(sscanf(comma+2,"%li",&specified_ordinal)!=1)
                recon_unsupported("VxD export ordinal conversion");
#else
            sscanf(comma+2,"%li",&specified_ordinal);
#endif
            if(specified_ordinal==1)device=symbol;
        }
        if(created)++missing;
        else {
            HEADER_WRITE(file,&count,1);
            HEADER_WRITE(file,copy,count);
            HEADER_WRITE(file,&ordinal,2);
            ++ordinal;
        }
        release_memory(copy);
    }
    HEADER_WRITE(file,&zero,1);
    if((WORD)missing)link_fatal(output_file_name,1120,(WORD)missing);
    vxd_header.nonresident_names=nonresident_start;
    HEADER_TELL(file,position);
    vxd_header.nonresident_size=position-nonresident_start;
    if(options.debug_level && (options.debug_formats & 2)) {
        HEADER_TELL(file,debug_directory->file_offset);
        codeview_debug->file_offset=debug_directory->raw_size+debug_directory->file_offset;
        directory.Characteristics=0;
        directory.TimeDateStamp=file_header.TimeDateStamp;
        directory.MajorVersion=directory.MinorVersion=0;
        directory.Type=2;
        directory.SizeOfData=codeview_debug->raw_size;
        directory.AddressOfRawData=0;
        directory.PointerToRawData=codeview_debug->file_offset;
        HEADER_WRITE(output_file,&directory,sizeof(directory));
        HEADER_WRITE(output_file,&pdb_codeview_header,sizeof(pdb_codeview_header));
        HEADER_WRITE(output_file,pdb_file_name,strlen(pdb_file_name)+1);
        release_memory((void *)pdb_file_name);
        vxd_header.debug_offset=debug_directory->file_offset;
        vxd_header.debug_size=debug_directory->raw_size;
    }
    vxd_header.flags|=dynamic_vxd ? 0x38000 : 0x28000;
    vxd_header.module_pages=vxd_page_count;
    if(entry_symbol) {
        contribution=entry_symbol->contribution;
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        address=entry_symbol->symbol.Value+contribution->rva;
        section=find_section_by_index(entry_symbol->symbol.SectionNumber,&sections);
        vxd_header.entry_object=(LONG)(SHORT)section->index;
        vxd_header.entry_offset=address-section->rva;
    }
    vxd_header.page_size=optional_header.SectionAlignment;
    vxd_header.object_table=196;
    vxd_header.object_count=file_header.NumberOfSections;
    vxd_header.page_map=vxd_header_bytes-pe_header_offset;
    vxd_header.resident_names=vxd_resident_names-pe_header_offset;
    vxd_header.entry_table=vxd_entry_table-pe_header_offset;
    vxd_header.fixup_page_table=vxd_fixup_pages-pe_header_offset;
    vxd_header.fixup_records=vxd_fixup_records-pe_header_offset;
    vxd_header.import_modules=vxd_page_map_cursor-pe_header_offset;
    vxd_header.import_procedures=vxd_page_map_cursor-pe_header_offset;
    vxd_header.fixup_size=vxd_header.import_modules-vxd_header.fixup_page_table;
    vxd_header.loader_size=vxd_header.fixup_page_table-196;
    vxd_header.data_pages=vxd_first_data;
    if(device) {
        contribution=device->contribution;
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        error=0;
        seek_file(file,contribution->file_offset+device->symbol.Value,FILE_BEGIN,&error);
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags & 8)mapped_read(record,&ddb,sizeof(ddb));
        else buffered_read(record,&ddb,sizeof(ddb));
        vxd_header.device_id=ddb.device_id;
        vxd_header.ddk_version=ddb.sdk_version;
    }
    error=0;
    seek_file(file,pe_header_offset,FILE_BEGIN,&error);
    HEADER_WRITE(file,&vxd_header,sizeof(vxd_header));
#undef HEADER_TELL
#undef HEADER_WRITE
}

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "link-driver.h"
#include "full-link-functions.h"
#include "memory.h"
#include "sections.h"
#include "debug-output.h"
#include <recon.h>

extern "C" {
extern int output_file;
extern BYTE file_access_statistics,pdb_enabled,mapped_debug;
extern Contribution *coff_debug,*misc_debug,*fixup_debug,*codeview_debug,*debug_directory;
extern DWORD coff_symbol_offset,emitted_symbol_count,codeview_offset;
extern const char *output_file_name,*pdb_file_name,*default_phase;
/* Original4672e0 contains NB10,0,0,0. Signature/age producers address fields
 * +8/+12 of this same whole-write owner. */
PdbCodeViewHeader pdb_codeview_header={0x3031424e,0,0,0};
DWORD codeview_directory_offset;
typedef struct DebugFixupBlock {
    struct DebugFixupBlock *next;
    BYTE records[341*12];
} DebugFixupBlock;
DebugFixupBlock *first_debug_fixup_block,*last_debug_fixup_block;
DWORD debug_fixup_full_blocks,debug_fixup_tail_count;
void __stdcall write_symbol_strings(int,StringTable *);
int __stdcall set_output_file_length(int,LONG);
void __stdcall emit_codeview(LinkContext *);
void __stdcall pdb_emit_fpo(DWORD *,DWORD);
int __cdecl compare_debug_fixups(const void *,const void *);
void __stdcall architecture_directory(DWORD *,DWORD *);
void __stdcall build_section_header(OutputSection *,IMAGE_SECTION_HEADER *);
}
/* Repeated file-cache operations are inline in this original owner. */
typedef struct BufferedDebugPosition { DWORD reserved[3],position; } BufferedDebugPosition;
#define DEBUG_WRITE(data,bytes) do { \
    int fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    FileRecord *file=file_slots[fd]; \
    if(file->flags & 8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#define DEBUG_READ(data,bytes) do { \
    int fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    FileRecord *file=file_slots[fd]; \
    if(file->flags & 8)mapped_read(file,(data),(bytes)); \
    else buffered_read(file,(data),(bytes)); \
} while(0)
#define DEBUG_TELL(destination) do { \
    int fd=output_file; \
    if(file_access_statistics)record_file_access(); \
    FileRecord *file=file_slots[fd]; \
    destination=(file->flags & 8) ? file->file_size : \
        ((BufferedDebugPosition *)file->base)->position; \
} while(0)
#define DEBUG_SEEK(position) do { error=0;seek_file(output_file,(position),FILE_BEGIN,&error); } while(0)
#define ROUND_DEBUG(value) (((value)+3)&~3U)
#define DEBUG_ENTRY(kind,bytes,address,position) do { \
    directory.Type=(kind);directory.SizeOfData=(bytes); \
    directory.AddressOfRawData=(address);directory.PointerToRawData=(position); \
    DEBUG_WRITE(&directory,sizeof(directory)); \
} while(0)

#pragma intrinsic(memset,strcpy,strlen)
void RECON_THISCALL LinkContext::EmitDebugDirectories(DWORD start,DWORD original_offset,
    DWORD lines,DWORD end,DWORD line_offset)
{
    DWORD error,base,cursor,position;
    BYTE has_fixups;
    IMAGE_DEBUG_DIRECTORY directory;
    if(!options.debug_level || is_vxd==1)return;
    base=debug_section->file_offset;
    cursor=base;
    if(options.debug_formats & 1) {
        DEBUG_SEEK(end);
        write_symbol_strings(output_file,symbols);
        DEBUG_TELL(position);
        coff_debug->raw_size=position-base;
        cursor=ROUND_DEBUG(position);
    }
    if(options.debug_formats & 16) {
        BYTE *data=(BYTE *)allocate_memory(misc_debug->raw_size);
        if(!options.misc_rdata) {
            misc_debug->file_offset=cursor;
            misc_debug->rva=debug_section->rva+cursor-base;
        }
        *(DWORD *)data=1;
        *(DWORD *)(data+4)=misc_debug->raw_size;
        data[8]=0;
        /* Original leaves the three reserved bytes9..11 unwritten. Preserve
         * Win32 behavior; native output uses zero for those undefined bytes. */
#ifdef __linux__
        memset(data+9,0,3);
#endif
        memset(data+12,0,misc_debug->raw_size-12);
        strcpy((char *)data+12,output_file_name);
        DEBUG_SEEK(misc_debug->file_offset);
        DEBUG_WRITE(data,misc_debug->raw_size);
        release_memory(data);
        if(!options.misc_rdata) { DEBUG_TELL(position);cursor=ROUND_DEBUG(position); }
    }
    if((options.debug_formats & 4) && debug_fixup_group->size) {
        DWORD aligned=(cursor+debug_fixup_group->alignment)&~(DWORD)debug_fixup_group->alignment;
        if(debug_fixup_group->file_offset!=aligned) {
            if(!pdb_enabled) {
                void *data=allocate_memory(debug_fixup_group->size);
                DEBUG_SEEK(debug_fixup_group->file_offset);
                DEBUG_READ(data,debug_fixup_group->size);
                debug_fixup_group->file_offset=aligned;
                debug_fixup_group->rva=debug_section->rva+aligned-base;
                DEBUG_SEEK(debug_fixup_group->file_offset);
                DEBUG_WRITE(data,debug_fixup_group->size);
                release_memory(data);
            } else {
                debug_fixup_group->file_offset=aligned;
                debug_fixup_group->rva=debug_section->rva+aligned-base;
            }
        }
        cursor=debug_fixup_group->file_offset+debug_fixup_group->size;
        if(pdb_enabled)cursor+=pdb_module_count*16-debug_fixup_group->size;
        cursor=ROUND_DEBUG(cursor);
    }
    has_fixups=(BYTE)((options.debug_formats>>3)&1);
    if(has_fixups) {
        fixup_debug->file_offset=cursor;
        fixup_debug->rva=debug_section->rva+cursor-base;
        fixup_debug->raw_size=(debug_fixup_tail_count+debug_fixup_full_blocks*341)*12;
        DEBUG_SEEK(fixup_debug->file_offset);
        DebugFixupBlock *block=first_debug_fixup_block;
        while(block) {
            DWORD count=block==last_debug_fixup_block ? debug_fixup_tail_count : 341;
            DEBUG_WRITE(block->records,count*12);
            DebugFixupBlock *next=block->next;
            release_memory(block);
            block=next;
        }
        DEBUG_TELL(position);
        cursor=ROUND_DEBUG(position);
    }
    if(!(options.debug_formats & 2))set_output_file_length(output_file,cursor);
    else {
        codeview_debug->file_offset=cursor;
        codeview_debug->rva=debug_section->rva+cursor-base;
        codeview_offset=cursor;
        if(!pdb_enabled) {
            default_phase="EmitCodeView";
            DEBUG_SEEK(debug_section->raw_size);
            emit_codeview(this);
        } else {
            DEBUG_SEEK(codeview_debug->file_offset);
            DEBUG_WRITE(&pdb_codeview_header,sizeof(pdb_codeview_header));
            DEBUG_WRITE(pdb_file_name,strlen(pdb_file_name)+1);
            release_memory((void *)pdb_file_name);
        }
        DEBUG_TELL(cursor);
    }
    debug_section->raw_size=cursor-debug_section->file_offset;
    debug_section->virtual_size=debug_section->raw_size;
    debug_section->reserved08=cursor;
    debug_section->initialized_file_size=((debug_section->raw_size-1)/
        optional_header.SectionAlignment+1)*optional_header.SectionAlignment;
    if(mapped_debug) {
        IMAGE_SECTION_HEADER header;
        build_section_header(debug_section,&header);
        DEBUG_SEEK(start);
        (this->*image_write_section)(output_file,debug_section,&header);
    }
    if((options.debug_formats & 4) && debug_fixup_group->size)
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=sizeof(directory);
    if(has_fixups)optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=sizeof(directory);
    if(options.debug_formats & 16)optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=sizeof(directory);
    if(options.debug_formats & 1)optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=sizeof(directory);
    if(options.debug_formats & 2)optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size+=sizeof(directory);
    if(optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress=debug_directory->rva;
    directory.Characteristics=0;
    directory.TimeDateStamp=file_header.TimeDateStamp;
    directory.MajorVersion=directory.MinorVersion=0;
    DEBUG_SEEK(debug_directory->file_offset);
    if(options.debug_formats & 1)
        DEBUG_ENTRY(1,coff_debug->raw_size,mapped_debug ? coff_debug->rva : 0,coff_debug->file_offset);
    if(options.debug_formats & 16)
        DEBUG_ENTRY(4,misc_debug->raw_size,(options.misc_rdata || mapped_debug) ?
            misc_debug->rva : 0,misc_debug->file_offset);
    if((options.debug_formats & 4) && debug_fixup_group->size) {
        directory.Type=3;
        directory.SizeOfData=debug_fixup_group->size;
        directory.AddressOfRawData=mapped_debug ? debug_fixup_group->rva : 0;
        directory.PointerToRawData=debug_fixup_group->file_offset;
        DEBUG_TELL(fpo_directory_offset);
        DEBUG_WRITE(&directory,sizeof(directory));
        DEBUG_TELL(position);
        if(!pdb_enabled) {
            DEBUG_SEEK(directory.PointerToRawData);
            void *data=allocate_memory(directory.SizeOfData+10);
            DEBUG_READ(data,directory.SizeOfData);
            qsort(data,directory.SizeOfData>>4,16,compare_debug_fixups);
            DEBUG_SEEK(directory.PointerToRawData);
            DEBUG_WRITE(data,directory.SizeOfData);
            release_memory(data);
        } else {
            pdb_emit_fpo(&pdb_fpo_count,directory.PointerToRawData);
            directory.SizeOfData=pdb_fpo_count*16;
            DEBUG_SEEK(fpo_directory_offset);
            DEBUG_WRITE(&directory,sizeof(directory));
        }
        DEBUG_SEEK(position);
    }
    if(has_fixups)
        DEBUG_ENTRY(6,fixup_debug->raw_size,mapped_debug ? fixup_debug->rva : 0,fixup_debug->file_offset);
    if(options.debug_formats & 2)
        DEBUG_ENTRY(2,debug_section->reserved08-codeview_debug->file_offset,
            mapped_debug ? codeview_debug->rva : 0,codeview_debug->file_offset);
    if(options.rom) {
        memset(&directory,0,sizeof(directory));
        DEBUG_WRITE(&directory,sizeof(directory));
    }
    if(options.debug_formats & 1) {
        IMAGE_COFF_SYMBOLS_HEADER header;
        header.NumberOfSymbols=emitted_symbol_count;
        header.LvaToFirstSymbol=coff_symbol_offset-original_offset;
        header.NumberOfLinenumbers=lines;
        header.LvaToFirstLinenumber=line_offset-original_offset;
        header.RvaToFirstByteOfCode=optional_header.BaseOfCode;
        header.RvaToLastByteOfCode=optional_header.BaseOfCode+optional_header.SizeOfCode;
        header.RvaToFirstByteOfData=optional_header.BaseOfData;
        header.RvaToLastByteOfData=optional_header.BaseOfCode+
            optional_header.SizeOfInitializedData+optional_header.SizeOfUninitializedData;
        DEBUG_SEEK(coff_debug->file_offset);
        DEBUG_WRITE(&header,sizeof(header));
    }
    if(!pdb_enabled && (options.debug_formats & 2)) {
        DWORD signature=0x3530424e,offset=codeview_directory_offset-codeview_offset;
        DEBUG_SEEK(codeview_debug->file_offset);
        DEBUG_WRITE(&signature,4);
        DEBUG_WRITE(&offset,4);
    }
}
#undef DEBUG_ENTRY
#undef ROUND_DEBUG
#undef DEBUG_SEEK
#undef DEBUG_TELL
#undef DEBUG_READ
#undef DEBUG_WRITE
extern "C" {
void __stdcall write_symbol_strings(int fd,StringTable *table)
{
    const char *saved=default_phase;
    DWORD size=0;
    void *data=&size;
    default_phase="WriteStringTable";
    if(table->hash.strings.data) {
        size=table->hash.strings.used;
        *(DWORD *)table->hash.strings.data=size;
        data=table->hash.strings.data;
    }
    DWORD bytes=table->hash.strings.data ? size : 4;
    if(file_access_statistics)record_file_access();
    FileRecord *file=file_slots[fd];
    if(file->flags&8)mapped_write(file,data,bytes);
    else buffered_write(file,data,bytes);
    default_phase=saved;
}
void __stdcall emit_codeview(LinkContext *) { __asm { int 3 } }
void __stdcall pdb_emit_fpo(DWORD *,DWORD) { __asm { int 3 } }
int __cdecl compare_debug_fixups(const void *left,const void *right)
{
    return (int)(*(const DWORD *)left-*(const DWORD *)right);
}
}

extern "C" {
extern BYTE incremental_enabled,mac_target;
extern StringNode *entry_symbol,*gp_symbol;
extern Contribution *incremental_code;
extern OutputSection *relocation_section;
extern ContributionGroup *mac_procedure_group;
}
void RECON_THISCALL LinkContext::FinishImage(void)
{
    ContributionGroup *group;
    StringNode *symbol;
    Contribution *contribution;
    const char *name;
    if(is_vxd)return;
    if(entry_symbol) {
        if(incremental_enabled && entry_symbol->reserved28)
            optional_header.AddressOfEntryPoint=incremental_code->rva+entry_symbol->reserved28;
        else optional_header.AddressOfEntryPoint=entry_symbol->final_value;
    }
    if(options.rom && file_header.Machine!=IMAGE_FILE_MACHINE_I386)return;
    if(export_group->size) {
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress=export_group->rva;
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size=export_group->size;
    }
    group=find_contribution_group(import_descriptor_section,".idata$2");
    if(group) {
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress=group->rva;
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size=group->size+20;
    }
    group=find_contribution_group(import_thunk_section,".idata$5");
    if(group) {
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress=group->rva;
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size=group->size;
    }
    group=find_contribution_group(delay_import_section,".didat$2");
    if(group) {
        optional_header.DataDirectory[13 /* delay imports */].VirtualAddress=group->rva;
        optional_header.DataDirectory[13 /* delay imports */].Size=group->size+32;
    }
    if(architecture_section && architecture_section->raw_size)
        architecture_directory(&optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].VirtualAddress,
            &optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size);
    if((mac_target && mac_procedure_group) || procedure_section->virtual_size) {
        if(mac_target) {
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size=
                mac_module_count ? mac_procedure_count*20 : mac_procedure_group->size;
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress=mac_procedure_group->rva;
        } else if(auxiliary->emitted_procedure_count) {
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size=
                auxiliary->procedure_record_size*auxiliary->emitted_procedure_count;
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress=procedure_section->rva;
        }
    }
    if(relocation_section && relocation_section->virtual_size)
        optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress=relocation_section->rva;
    if(gp_symbol)optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress=gp_symbol->final_value;
    name=(file_header.Machine==IMAGE_FILE_MACHINE_I386 || file_header.Machine==0x1a2 ||
          file_header.Machine==0x1a6) ? "__tls_used" : "_tls_used";
    symbol=symbols->FindSymbol(name);
    if(symbol && (symbol->flag_byte & 1) && (contribution=symbol->contribution)!=NULL) {
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        if(contribution) {
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size=24;
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress=symbol->final_value;
        }
    }
    name=(file_header.Machine==IMAGE_FILE_MACHINE_I386 || file_header.Machine==0x1a2 ||
          file_header.Machine==0x1a6) ? "__load_config_used" : "_load_config_used";
    symbol=symbols->FindSymbol(name);
    if(symbol && (symbol->flag_byte & 1) && (contribution=symbol->contribution)!=NULL) {
        while(contribution && (contribution->characteristics & 0x4000))
            contribution=contribution->associated_contribution;
        if(contribution) {
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size=64;
            optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress=symbol->final_value;
        }
    }
}
extern "C" void __stdcall architecture_directory(DWORD *,DWORD *) { __asm { int 3 } }

extern "C" {
void __stdcall write_file_header(int,IMAGE_FILE_HEADER *,IMAGE_FILE_HEADER *);
void __stdcall write_optional_header(int,const IMAGE_OPTIONAL_HEADER *,WORD);
}
/* Public ROM optional-header layout. This view writes the same56-byte object;
 * it is distinct from the PE32 optional header held in LinkContext. */
typedef struct LinkRomHeader {
    WORD Magic;
    BYTE MajorLinkerVersion,MinorLinkerVersion;
    DWORD SizeOfCode,SizeOfInitializedData,SizeOfUninitializedData;
    DWORD AddressOfEntryPoint,BaseOfCode,BaseOfData,BaseOfBss,GprMask;
    DWORD CprMask[4],GpValue;
} LinkRomHeader;
typedef char LinkRomHeaderSize[(sizeof(LinkRomHeader)==56)?1:-1];
void RECON_THISCALL LinkContext::PeWriteHeaders(int file)
{
    DWORD error=0;
    FileRecord *record;
    if(options.rom) {
        LinkRomHeader header;
        seek_file(file,0,FILE_BEGIN,&error);
        write_file_header(file,&file_header,NULL);
        header.Magic=0x107;
        header.MajorLinkerVersion=optional_header.MajorLinkerVersion;
        header.MinorLinkerVersion=optional_header.MinorLinkerVersion;
        header.SizeOfCode=optional_header.SizeOfCode;
        header.SizeOfInitializedData=optional_header.SizeOfInitializedData;
        header.SizeOfUninitializedData=optional_header.SizeOfUninitializedData;
        header.AddressOfEntryPoint=optional_header.AddressOfEntryPoint;
        header.BaseOfCode=optional_header.BaseOfCode;
        header.BaseOfData=optional_header.BaseOfData;
        header.BaseOfBss=reserved1e8;
        header.GprMask=0;
        header.CprMask[0]=header.CprMask[1]=header.CprMask[2]=header.CprMask[3]=0;
        header.GpValue=gp_symbol ? gp_symbol->final_value+optional_header.BaseOfData : 0;
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags & 8)mapped_write(record,&header,file_header.SizeOfOptionalHeader);
        else buffered_write(record,&header,file_header.SizeOfOptionalHeader);
    } else {
        WORD maximum_sections;
        DWORD signature=IMAGE_NT_SIGNATURE;
        switch(file_header.Machine) {
        case 0x14c:case 0x166:case 0x168:case 0x1f0:maximum_sections=96;break;
        case 0x184:maximum_sections=198;break;
        case 0x1a2:case 0x1a6:case 0x1c0:maximum_sections=19;break;
        default:maximum_sections=0xffff;break;
        }
        if(file_header.NumberOfSections>maximum_sections)
            link_warning(output_file_name,4072,(unsigned)file_header.NumberOfSections,(unsigned)maximum_sections);
        if(!(optional_header.Subsystem>=1 && optional_header.Subsystem<=3 &&
             (optional_header.MajorSubsystemVersion>4 ||
              (optional_header.MajorSubsystemVersion==4 && optional_header.MinorSubsystemVersion))) &&
           optional_header.SizeOfImage>0x10000000)
            link_warning(output_file_name,4084,optional_header.SizeOfImage,0x10000000);
        seek_file(file,pe_header_offset,FILE_BEGIN,&error);
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags & 8)mapped_write(record,&signature,sizeof(signature));
        else buffered_write(record,&signature,sizeof(signature));
        write_file_header(file,&file_header,NULL);
        /* Complete original callee tests AX then zero-extends it before any
         * size use; the physical stack argument remains one DWORD slot. */
        write_optional_header(file,&optional_header,file_header.SizeOfOptionalHeader);
    }
}

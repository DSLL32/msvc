#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "dumper.h"
#include "pef.h"
#include "input.h"
#include "sections.h"
#include <recon.h>

extern "C" {
extern FILE *map_output;
extern DWORD dump_file_type;
#pragma intrinsic(memcpy,memset,strlen)
DWORD __stdcall dump_pef_container_header(const PefContainerHeader *image)
{
    PefContainerHeader header;
    memcpy(&header,image,sizeof(header));
    swap_byte_order(&header.magic1,2);
    swap_byte_order(&header.magic2,2);
    swap_byte_order(&header.container_id,4);
    swap_byte_order(&header.architecture,4);
    swap_byte_order(&header.version,4);
    swap_byte_order(&header.timestamp,4);
    swap_byte_order(&header.old_definition_version,4);
    swap_byte_order(&header.old_implementation_version,4);
    swap_byte_order(&header.current_version,4);
    swap_byte_order(&header.section_count,2);
    swap_byte_order(&header.loadable_sections,2);
    swap_byte_order(&header.memory_address,4);
    if(dumper_context->options.dump_headers)
        fprintf(map_output,"\n Container Header\n\n magic1, magic2   = 0x%04X%04X\n"
            " containerID      = 0x%08lX\n architectureID   = 0x%08lX\n"
            " versionNumber    = 0x%08lX\n timeStamp        = 0x%08lX\n"
            " oldDefVersion    = %u\n oldImpVersion    = %u\n currentVersion   = %u\n"
            " nbrOfSections    = %u\n loadableSections = %u\n memoryAddress    = 0x%08lX\n",
            header.magic1,header.magic2,header.container_id,header.architecture,
            header.version,header.timestamp,header.old_definition_version,
            header.old_implementation_version,header.current_version,
            header.section_count,header.loadable_sections,header.memory_address);
    return header.section_count;
}
void __stdcall dump_pef_loader_header(const PefLoaderHeader *header)
{
    fprintf(map_output,"\n Loader Header\n\n entryPointSection = %d\n"
        " entryPointOffset  = %08lX\n initPointSection  = %d\n initPointOffset   = %08lX\n"
        " termPointSection  = %d\n termPointOffset   = %08lX\n numImportFiles    = %08lX\n"
        " numImportSyms     = %08lX\n numSections       = %d\n relocationsOffset = %08lX\n"
        " stringsOffset     = %08lX\n hashSlotTable     = %08lX\n hashSlotTabSize   = %d (%d)\n"
        " numExportedSyms   = %08lX\n",
        header->main_section,header->main_offset,header->init_section,header->init_offset,
        header->term_section,header->term_offset,header->library_count,header->import_count,
        header->relocation_section_count,header->relocation_offset,header->strings_offset,
        header->hash_offset,header->hash_power,1U<<(header->hash_power&31),header->export_count);
}
void __stdcall dump_pef_import_libraries(const PefLoaderHeader *header,
    const PefImportLibrary *records,const char *strings)
{
    DWORD index,name_offset;
    PefImportLibrary library;
    const BYTE *reserved;
    if(!header->library_count)return;
    fprintf(map_output,"\n Loader Import Container ID Table\n\n");
    for(index=0;index<header->library_count;++index) {
        memcpy(&library,records+index,sizeof(library));
        name_offset=byte_swap_dword(library.name_offset);
        reserved=(const BYTE *)&library.reserved_word;
        fprintf(map_output," [%2u] containerName    = 0x%08lX (%u) \"%s\"\n"
            "      oldImpVersion    = %lu\n      currentVersion   = %lu\n"
            "      nbrOfImports     = %lu\n      firstImport      = %lu\n"
            "      importFlags      = 0x%02X\n      reserved         = 0x%02X%02X%02X\n\n",
            index,name_offset,name_offset,strings+name_offset,
            byte_swap_dword(library.old_version),byte_swap_dword(library.current_version),
            byte_swap_dword(library.import_count),byte_swap_dword(library.first_import),
            library.flags,library.reserved_byte,reserved[0],reserved[1]);
    }
}
void __stdcall dump_pef_import_symbols(const PefLoaderHeader *header,
    const DWORD *records,const char *strings)
{
    DWORD index,symbol,name_offset;
    unsigned symbol_class;
    const char *kind;
    if(!header->import_count)return;
    fprintf(map_output," Loader Import Symbol Table\n\n");
    for(index=0;index<header->import_count;++index) {
        symbol=records[index];
        swap_byte_order(&symbol,4);
        symbol_class=(BYTE)(symbol>>24);
        switch(symbol_class) {
        case 0:kind="Code   ";break;
        case 1:kind="Data   ";break;
        case 2:kind="TVector";break;
        case 3:kind="TOC    ";break;
        case 4:kind="Glue   ";break;
        default:kind="Unknown";break;
        }
        name_offset=symbol&0x00ffffff;
        fprintf(map_output," [%4lu] %02X %06X  %s  %s\n",index,
            symbol_class,name_offset,kind,strings+name_offset);
    }
}
void __stdcall dump_pef_relocation_headers(const PefLoaderHeader *header,
    const PefRelocationHeader *records)
{
    DWORD index;
    if(!header->relocation_section_count)return;
    fprintf(map_output,"\n Loader Relocation Headers\n\n");
    for(index=0;index<header->relocation_section_count;++index) {
        const PefRelocationHeader *record=records+index;
        fprintf(map_output," sectionNumber    = %u\n reserved         = 0x%04X\n"
            " nbrOfRelocs      = %u\n firstRelocInstr  = 0x%08lX (%u)\n",
            byte_swap_word(record->section),byte_swap_word(record->reserved),
            byte_swap_dword(record->instruction_count),
            byte_swap_dword(record->instruction_offset),
            byte_swap_dword(record->instruction_offset));
    }
}
void __stdcall dump_pef_relocations(const PefLoaderHeader *header,
    const PefRelocationHeader *records)
{
    DWORD section_index,index,address=0,slot=0;
    WORD instruction,extension;
    DWORD opcode,subcode,count,delta,repeat,prior,sum,j;
    PefRelocationHeader section;
    const WORD *instructions=(const WORD *)(records+header->relocation_section_count);
#ifdef __linux__
    /* Original repeat history is unwritten until each instruction establishes
     * its slot. Do not expose those bytes on a future premature repeat. */
    DWORD history[16]={0};
    BYTE history_defined[16]={0};
#else
    DWORD history[16];
#endif
    if(!header->relocation_section_count)return;
    fprintf(map_output,"\n Loader Relocation Instructions\n\n");
    for(section_index=0;section_index<header->relocation_section_count;++section_index) {
        memcpy(&section,records+section_index,sizeof(section));
        swap_byte_order(&section.section,2);
        swap_byte_order(&section.instruction_count,4);
        swap_byte_order(&section.instruction_offset,4);
        fprintf(map_output," Relocations for Section %u\n\n",section.section);
        for(index=0;index<section.instruction_count;++index) {
            instruction=byte_swap_word(*instructions++);
            opcode=instruction&0x8000?instruction>>12:
                instruction&0xc000?instruction>>13:instruction>>14;
            switch(opcode) {
            case 0:
                delta=(instruction>>6)<<2;
                count=instruction&0x3f;
                address+=delta;
                history[slot]=delta;
                fprintf(map_output," (%08X) %04X      DDAT  delta=%u,n=%u\n",
                    address,instruction,delta,count);
                delta+=count*4;
                address+=count*4;
                history[slot]=delta;
                break;
            case 2:
                fprintf(map_output," (%08X) %04X      ",address,instruction);
                subcode=(instruction>>9)&15;
                count=(instruction&0x1ff)+1;
                switch(subcode) {
                case 0:fprintf(map_output,"CODE  cnt=%u\n",count);delta=count*4;break;
                case 1:fprintf(map_output,"DATA  cnt=%u\n",count);delta=count*4;break;
                case 2:fprintf(map_output,"DESC  cnt=%u\n",count);delta=count*12;break;
                case 3:fprintf(map_output,"DSC2  cnt=%u\n",count);delta=count*8;break;
                case 4:fprintf(map_output,"VTBL  cnt=%u\n",count);delta=count*8;break;
                case 5:fprintf(map_output,"SYMR  cnt=%u\n",count);delta=count*4;break;
                default:fprintf(map_output,"????  cnt=%u\n",count);delta=0;break;
                }
                address+=delta;
                history[slot]=delta;
                break;
            case 3:
                fprintf(map_output," (%08X) %04X      ",address,instruction);
                subcode=(instruction>>9)&15;
                count=instruction&0x1ff;
                switch(subcode) {
                case 0:fprintf(map_output,"SYMB  idx=%u\n",count);delta=4;break;
                case 1:fprintf(map_output,"CDIS  sct=%u\n",count);delta=0;break;
                case 2:fprintf(map_output,"DTIS  sct=%u\n",count);delta=0;break;
                case 3:fprintf(map_output,"SECN  sct=%u\n",count);delta=4;break;
                default:fprintf(map_output,"????  %u\n",count);delta=0;break;
                }
                address+=delta;
                history[slot]=delta;
                break;
            case 8:
                delta=(instruction&0xfff)+1;
                fprintf(map_output," (%08X) %04X      DELT  delta=%u\n",
                    address,instruction,delta);
                address+=delta;
                history[slot]=delta;
                break;
            case 9:
                prior=((instruction>>8)&15)+1;
                repeat=(instruction&255)+1;
                fprintf(map_output," (%08X) %04X      RPT   i=%u,rpt=%u\n",
                    address,instruction,prior,repeat);
                sum=0;
                for(j=0;j<prior;++j) {
                    DWORD previous=(slot-prior+16+j)%16;
#ifdef __linux__
                    if(!history_defined[previous])recon_unsupported("PEF relocation unwritten repeat history");
#endif
                    sum+=history[previous];
                }
                address+=sum*repeat;
                history[slot]=0;
                break;
            case 10:
                extension=byte_swap_word(*instructions++);
                ++index;
                fprintf(map_output," (%08X) %04X %04X ",address,instruction,extension);
                count=((instruction&0x3ff)<<16)+extension;
                subcode=(instruction>>10)&3;
                if(subcode==0) {
                    fprintf(map_output,"LABS  offset=0x%X\n",count);
                    address=count;history[slot]=0;
                } else if(subcode==1) {
                    fprintf(map_output,"LSYM  idx=0x%X\n",count);
                    address+=4;history[slot]=4;
                } else {
                    fprintf(map_output,"????  %u\n",count);
                    history[slot]=0;
                }
#ifdef __linux__
                history_defined[slot]=1;
#endif
                slot=(slot+1)%16;
                history[slot]=0;
                break;
            case 11:
                extension=byte_swap_word(*instructions++);
                ++index;
                fprintf(map_output," (%08X) %04X %04X ",address,instruction,extension);
                count=((instruction&0x3f)<<16)+extension;
                subcode=(instruction>>10)&3;
                prior=(instruction>>6)&15;
                if(subcode==0) {
                    ++prior;
                    fprintf(map_output,"LRPT  i=%u,rpt=%u\n",prior,count);
                    sum=0;
                    for(j=0;j<prior;++j) {
                        DWORD previous=(slot-prior+16+j)%16;
#ifdef __linux__
                        if(!history_defined[previous])recon_unsupported("PEF relocation unwritten long repeat history");
#endif
                        sum+=history[previous];
                    }
                    address+=sum*count;
                } else if(subcode==1) {
                    switch(prior) {
                    case 0:fprintf(map_output,"LSEC  LSECN,sct=%u\n",count);address+=4*count;break;
                    case 1:fprintf(map_output,"LSEC  LCDIS,sct=%u\n",count);break;
                    case 2:fprintf(map_output,"LSEC  LDTIS,sct=%u\n",count);break;
                    default:fprintf(map_output,"????? %u\n",count);break;
                    }
                } else fprintf(map_output,"????  %u,%u\n",prior,count);
                history[slot]=0;
                break;
            default:
                fprintf(map_output," (%08X) %04X      ????",address,instruction);
                /* Unknown opcodes leave this history slot unchanged. */
                break;
            }
#ifdef __linux__
            if(opcode==0 || opcode==2 || opcode==3 || opcode>=8 && opcode<=11)
                history_defined[slot]=1;
#endif
            slot=(slot+1)%16;
        }
    }
}
void __stdcall dump_mppc_relocations(const void *,const PefLoaderHeader *,const PefRelocationHeader *)
{ __asm { int 3 } }
void __stdcall dump_pef_hash_slots(const PefLoaderHeader *header,const void *loader)
{
    const DWORD *slots;
    DWORD value;
    LONG index;
    if(!header->hash_power)return;
    fprintf(map_output,"\n   Hash Slot Table:\n\n");
    slots=(const DWORD *)((const BYTE *)loader+header->hash_offset);
    for(index=0;index<(LONG)(1U<<(header->hash_power&31));++index) {
        value=slots[index];
        swap_byte_order(&value,4);
        fprintf(map_output,"    HashSlot %3d: chain count %3d index %3d\n",
            index,value>>18,value&0x3ffff);
    }
}
/* Actual46735c owner: retained across files, initialized once, and reduced by
 * the encountered export names. This is not a per-container reset. */
DWORD pef_first_export_name=0xffffffffUL;
void __stdcall dump_pef_exports(const PefLoaderHeader *header,const void *loader,
    const char *strings)
{
    const DWORD *hashes;
    const PefExportSymbol *exports;
    DWORD index,hash,name_offset;
    PefExportSymbol symbol;
    if(!header->export_count)return;
    fprintf(map_output,"\n   Exported Symbols:\n\n");
    hashes=(const DWORD *)((const BYTE *)loader+header->hash_offset)+
        (1U<<(header->hash_power&31));
    exports=(const PefExportSymbol *)(hashes+header->export_count);
    for(index=0;index<header->export_count;++index) {
        hash=hashes[index];
        memcpy(&symbol,exports+index,sizeof(symbol));
        swap_byte_order(&hash,4);
        swap_byte_order(&symbol.class_and_name,4);
        swap_byte_order(&symbol.value,4);
        swap_byte_order(&symbol.section,2);
        name_offset=symbol.class_and_name&0xffffff;
        if(pef_first_export_name>=name_offset)pef_first_export_name=name_offset;
        fprintf(map_output,"    Exp %3d: sec %d offset 0x%08lX hash 0x%X class %d \"%.*s\"\n",
            index,symbol.section,symbol.value,hash,(BYTE)(symbol.class_and_name>>24),
            hash>>16,strings+name_offset);
    }
}
void __stdcall dump_pef_loader_strings(const PefLoaderHeader *header,
    const void *loader,const char *strings)
{
    DWORD size=header->hash_offset-header->strings_offset;
    DWORD offset=0,remaining=header->export_count,index,name,hash,length;
    const DWORD *hashes;
    const PefExportSymbol *exports;
    const char *text;
    if(!size)return;
    fprintf(map_output,"\n Loader String Table\n\n");
    while(offset<size) {
        text=strings+offset;
        if(!*text)break;
        if(remaining && (dumper_context->options.dump_exports?
            offset==pef_first_export_name:strlen(text)>64)) {
            hashes=(const DWORD *)((const BYTE *)loader+header->hash_offset)+
                (1U<<(header->hash_power&31));
            exports=(const PefExportSymbol *)(hashes+header->export_count);
            for(index=remaining;index;--index) {
                name=exports[index-1].class_and_name;
                swap_byte_order(&name,4);
                name&=0xffffff;
                hash=hashes[index-1];
                swap_byte_order(&hash,4);
                length=hash>>16;
                fprintf(map_output," %08lX: \"%.*s\"\n",name,length,strings+name);
                /* The original advances by name lengths, without their NULs.
                 * It then resumes the ordinary string walk at that offset. */
                offset+=length;
                --remaining;
            }
        } else {
            fprintf(map_output," %08lX: \"%s\"\n",offset,text);
            offset+=strlen(text)+1;
        }
    }
}

void __stdcall dump_mppc_loader(const void *loader)
{
    PefLoaderHeader header;
    const PefImportLibrary *libraries;
    const DWORD *imports;
    const PefRelocationHeader *relocations;
    const char *strings;
    memcpy(&header,loader,sizeof(header));
    swap_byte_order(&header.main_section,4);
    swap_byte_order(&header.main_offset,4);
    swap_byte_order(&header.init_section,4);
    swap_byte_order(&header.init_offset,4);
    swap_byte_order(&header.term_section,4);
    swap_byte_order(&header.term_offset,4);
    swap_byte_order(&header.library_count,4);
    swap_byte_order(&header.import_count,4);
    swap_byte_order(&header.relocation_section_count,4);
    swap_byte_order(&header.relocation_offset,4);
    swap_byte_order(&header.strings_offset,4);
    swap_byte_order(&header.hash_offset,4);
    swap_byte_order(&header.hash_power,4);
    swap_byte_order(&header.export_count,4);
    if(dumper_context->options.dump_headers)dump_pef_loader_header(&header);
    strings=(const char *)loader+header.strings_offset;
    libraries=(const PefImportLibrary *)((const BYTE *)loader+sizeof(header));
    imports=(const DWORD *)(libraries+header.library_count);
    if(dumper_context->options.dump_imports) {
        dump_pef_import_libraries(&header,libraries,strings);
        dump_pef_import_symbols(&header,imports,strings);
    }
    relocations=(const PefRelocationHeader *)(imports+header.import_count);
    if(dumper_context->options.dump_base_relocations) {
        dump_pef_relocation_headers(&header,relocations);
        if(dump_file_type==8)dump_pef_relocations(&header,relocations);
        else dump_mppc_relocations(loader,&header,relocations);
    }
    dump_pef_hash_slots(&header,loader);
    if(dumper_context->options.dump_exports)dump_pef_exports(&header,loader,strings);
    if(dumper_context->options.dump_imports || dumper_context->options.dump_exports)
        dump_pef_loader_strings(&header,loader,strings);
}
void __stdcall dump_raw_data(DWORD,const void *,DWORD);
void __stdcall disassemble_section(DumperSectionState *,void *,DWORD,DWORD,DWORD,DWORD,
    const void *,DWORD,FILE *);
void __stdcall dump_pef_section(DWORD index,const PefContainerHeader *image,
    const char *names)
{
    PefSectionHeader section;
    const char *name,*kind,*share;
    OutputSection *summary;
    DumperSectionState state;
    memcpy(&section,(const BYTE *)(image+1)+index*sizeof(section),sizeof(section));
    swap_byte_order(&section.name_offset,4);
    swap_byte_order(&section.address,4);
    swap_byte_order(&section.total_size,4);
    swap_byte_order(&section.initialized_size,4);
    swap_byte_order(&section.raw_size,4);
    swap_byte_order(&section.file_offset,4);
    if(section.name_offset==-1)name="<unnamed>";
    else {
        name=names+section.name_offset;
        if(dumper_context->options.dump_summary) {
            summary=create_output_section(NULL,name,0,&dumper_context->sections,
                &dumper_context->optional_header);
            summary->raw_size+=section.total_size?section.total_size:section.raw_size;
        }
    }
    if(dumper_context->options.dump_headers) {
        switch(section.kind) {
        case 0:kind="code";break;
        case 1:kind="data";break;
        case 2:kind="pidata";break;
        case 3:kind="constant";break;
        case 4:kind="loader";break;
        case 5:kind="debug";break;
        default:kind="unknown";break;
        }
        share=section.share_kind==1?"context share":
            section.share_kind==4?"global share":"unknown";
        fprintf(map_output,"\n Section Header %u\n\n sectionName      = 0x%08lX (%d) \"%s\"\n"
            " sectionAddress   = 0x%08lX\n execSize         = 0x%08lX (%lu)\n"
            " initSize         = 0x%08lX (%lu)\n rawSize          = 0x%08lX (%lu)\n"
            " containerOffset  = 0x%08lX (%lu)\n regionKind       = 0x%02X       (%s)\n"
            " shareKind        = 0x%02X       (%s)\n alignment        = 0x%02X       (%lu-byte bndry)\n"
            " reserved         = 0x%02X\n",
            index,section.name_offset,section.name_offset,name,section.address,
            section.total_size,section.total_size,section.initialized_size,section.initialized_size,
            section.raw_size,section.raw_size,section.file_offset,section.file_offset,
            section.kind,kind,section.share_kind,share,section.alignment,
            1UL<<(section.alignment&31),section.reserved);
    }
    if(section.kind==4)dump_mppc_loader((const BYTE *)image+section.file_offset);
    if(section.raw_size) {
        if(dumper_context->options.dump_disassembly && section.kind==0) {
            fputc('\n',map_output);
            memset(&state,0,sizeof(state));
            disassemble_section(&state,NULL,0x601,0,index,section.address,
                (const BYTE *)image+section.file_offset,section.raw_size,map_output);
        }
        if(dumper_context->options.dump_raw_data && (section.kind==0 || section.kind==1)) {
            fprintf(map_output,"\nRAW DATA #%hX\n",index);
            dump_raw_data(section.address,(const BYTE *)image+section.file_offset,section.raw_size);
        }
    }
}

void __stdcall dump_pef_file(void *mapped)
{
    const PefContainerHeader *header=(const PefContainerHeader *)mapped;
    DWORD count=dump_pef_container_header(header),section;
    const char *names=(const char *)(header+1)+count*sizeof(PefSectionHeader);
    for(section=0;section<count;++section)
        dump_pef_section(section,header,names);
}
}

/* Built-in COFF Binary File Dumper: ordinary shared input/cache machinery. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <share.h>
#include <string.h>
#include <time.h>
#include "link.h"
#include "arguments.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include "dumper.h"
#include <recon.h>

extern "C" {
LinkContext *dumper_context;
BYTE dump_width_warning;
extern FILE *map_output;
extern int current_input_file;
extern BYTE file_access_statistics;
extern DWORD dump_file_type;
extern DWORD archive_member_offset;
void *__stdcall allocate_link_memory(unsigned,unsigned);
void __cdecl close_files(void);
void __cdecl cleanup_pdb(void);
void __cdecl dumper_usage(void) { __asm { int 3 } }
BOOL __stdcall read_separate_dumper_debug(FILE *,DWORD,IMAGE_DEBUG_DIRECTORY *) { __asm { int 3 } return FALSE; }
FILE *__stdcall find_separate_dumper_debug(const char *,DWORD,char *,IMAGE_DEBUG_DIRECTORY *) { __asm { int 3 } return NULL; }
PDB *__stdcall find_embedded_dumper_pdb(const char *,const char *,DWORD,DWORD) { __asm { int 3 } return NULL; }
#pragma intrinsic(strcpy,strlen)
BOOL __stdcall load_dumper_debug(const char *name,LONG *error,char *error_text,char *debug_name,
    DWORD *data_offset,DWORD *data_bytes,DumperDatabase **database)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY debug;
    BYTE codeview[0x114];
    char separate[MAX_PATH],directory[256],search_path[MAX_PATH],drive[4];
    const char *origin=name;
    DWORD debug_rva,debug_bytes,count,i,signature,age;
    FILE *file,*replacement;
    if(debug_name)*debug_name=0;
    if(data_offset)*data_offset=0;
    if(data_bytes)*data_bytes=0;
    *database=NULL;
    file=_fsopen(name,"rb",_SH_DENYWR);
    if(!file)return FALSE;
    if(fread(&dos,sizeof(dos),1,file)!=1)goto failed;
    if(dos.e_magic==0x4944) {
        if(fseek(file,0,SEEK_SET))goto failed;
        if(!read_separate_dumper_debug(file,*(DWORD *)((BYTE *)&dos+8),&debug))goto failed;
#ifdef __linux__
        if(strlen(name)>=sizeof(separate)) { __asm { int 3 } }
#endif
        strcpy(separate,name);
        origin=separate;
    } else if(dos.e_magic==IMAGE_DOS_SIGNATURE) {
        if(!dos.e_lfanew || fseek(file,dos.e_lfanew,SEEK_SET) || fread(&nt,sizeof(nt),1,file)!=1 ||
            nt.Signature!=IMAGE_NT_SIGNATURE)goto failed;
        if(nt.FileHeader.Characteristics&IMAGE_FILE_DEBUG_STRIPPED) {
            replacement=find_separate_dumper_debug(name,nt.FileHeader.TimeDateStamp,separate,&debug);
            if(!replacement)goto failed;
            fclose(file);
            file=replacement;
            origin=separate;
        } else {
            if(nt.OptionalHeader.NumberOfRvaAndSizes<6)goto failed;
            debug_rva=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            debug_bytes=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
            count=debug_bytes/sizeof(IMAGE_DEBUG_DIRECTORY);
            if(!debug_rva || !count)goto failed;
            if(fseek(file,dos.e_lfanew+24+nt.FileHeader.SizeOfOptionalHeader,SEEK_SET))goto failed;
            for(i=0;i<nt.FileHeader.NumberOfSections;++i) {
                if(fread(&section,sizeof(section),1,file)!=1) {
#ifdef __linux__
                    /* The original can continue with a short section read.
                     * Its unwritten tail is outside this valid-input domain. */
                    __asm { int 3 }
#endif
                    break;
                }
                if(section.VirtualAddress<=debug_rva &&
                    section.VirtualAddress+section.SizeOfRawData>=debug_rva+debug_bytes)break;
            }
            if(i==nt.FileHeader.NumberOfSections)goto failed;
            if(fseek(file,section.PointerToRawData+debug_rva-section.VirtualAddress,SEEK_SET))goto failed;
            for(i=0;i<count;++i) {
                if(fread(&debug,sizeof(debug),1,file)!=1)goto failed;
                if(debug.Type==IMAGE_DEBUG_TYPE_CODEVIEW)break;
            }
            if(i==count)goto failed;
        }
    } else goto failed;
    if(origin!=name) {
        if(debug_name)strcpy(debug_name,separate);
        if(debug.Type!=IMAGE_DEBUG_TYPE_CODEVIEW)goto failed;
    }
    if(data_offset)*data_offset=debug.PointerToRawData;
    if(data_bytes)*data_bytes=debug.SizeOfData;
    if(debug.SizeOfData<0x12 || debug.SizeOfData>sizeof(codeview))goto failed;
    if(fseek(file,debug.PointerToRawData,SEEK_SET) ||
        fread(codeview,1,debug.SizeOfData,file)!=debug.SizeOfData ||
        *(DWORD *)codeview!=0x3031424e)goto failed;
    fclose(file);
#ifdef __linux__
    if(!memchr(codeview+16,0,debug.SizeOfData-16)) { __asm { int 3 } }
#endif
    _splitpath(origin,drive,directory,NULL,NULL);
    if(!*directory)strcpy(directory,".");
    _makepath(search_path,drive,directory,NULL,NULL);
    signature=*(DWORD *)(codeview+8);
    age=*(DWORD *)(codeview+12);
    if(PDB::OpenValidate((char *)codeview+16,search_path,"r",signature,age,error,error_text,database))return TRUE;
    if(origin==name) {
        *database=find_embedded_dumper_pdb((char *)codeview+16,name,signature,age);
        if(*database) {*data_offset=0;return TRUE;}
    }
    return FALSE;
failed:
    fclose(file);
    return FALSE;
}
int __cdecl compare_dumper_symbol_value(const void *left,const void *right)
{
    DWORD a=(*(const IMAGE_SYMBOL * const *)left)->Value;
    DWORD b=(*(const IMAGE_SYMBOL * const *)right)->Value;
    if(a<b)return -1;
    return a>b?1:0;
}
void __stdcall build_dumper_symbol_index(DumperSectionState *state,DWORD section,DWORD image_base)
{
    IMAGE_SYMBOL *symbol;
    DWORD i,count;
    state->symbol_count=0;
    state->symbol_base=dump_file_type==1?0:image_base;
    if(state->public_symbols || !dump_symbols) {
        state->symbol_index=NULL;
        return;
    }
    count=dumper_context->file_header.NumberOfSymbols;
    state->symbol_index=(IMAGE_SYMBOL **)allocate_memory(count*sizeof(IMAGE_SYMBOL *));
    for(i=0;i<count;i+=symbol->NumberOfAuxSymbols+1) {
        symbol=dump_symbols+i;
        if(dump_file_type!=2 && symbol->SectionNumber!=(int)(WORD)section)continue;
        switch(symbol->StorageClass) {
        case IMAGE_SYM_CLASS_STATIC:
            if(symbol->NumberOfAuxSymbols && (symbol->Type&0x30)!=0x20)break;
            /* Static functions and static symbols without auxiliaries share
             * the ordinary external/label indexing path. */
        case IMAGE_SYM_CLASS_EXTERNAL:
        case IMAGE_SYM_CLASS_LABEL:
        case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
            state->symbol_index[state->symbol_count++]=symbol;
            break;
        }
    }
    qsort(state->symbol_index,state->symbol_count,sizeof(IMAGE_SYMBOL *),compare_dumper_symbol_value);
    state->symbol_index=(IMAGE_SYMBOL **)realloc(state->symbol_index,state->symbol_count*sizeof(IMAGE_SYMBOL *));
}
void __stdcall release_dumper_symbol_index(DumperSectionState *state) { release_memory(state->symbol_index); }
extern const char *default_phase;
DWORD dump_archive_export_count;
DWORD dump_archive_export_capacity;
DumperArchiveExport *dump_archive_exports;
void __stdcall dump_archive_linker_members(OutputLibrary *library);
#pragma intrinsic(memcmp)
void __stdcall dump_archive_header(OutputLibrary *library,IMAGE_ARCHIVE_MEMBER_HEADER header,DWORD offset)
{
    time_t timestamp;
    WORD mode;
    SHORT user,group;
    DWORD size;
    const char *text;
    default_phase="DumpMemberHeader";
    sscanf((const char *)header.Date,"%ld",&timestamp);
    sscanf((const char *)header.Mode,"%ho",&mode);
    sscanf((const char *)header.Size,"%ld",(long *)&size);
    fprintf(map_output,"\nArchive member name at %lX: %.16s",offset,header.Name);
    if(library && header.Name[0]=='/') {
        text=library->MemberName(&header);
        if(!text)text="member corrupt";
        fputs(text,map_output);
    }
    fputc('\n',map_output);
    fprintf(map_output,"%8lX time/date",timestamp);
    text=ctime(&timestamp);
    if(text)fprintf(map_output," %s",text);
    else fputc('\n',map_output);
    if(!memcmp(header.UserID,"      ",6))fputs("        ",map_output);
    else {
        sscanf((const char *)header.UserID,"%hd",&user);
        fprintf(map_output,"%8hX",user);
    }
    fputs(" uid\n",map_output);
    if(!memcmp(header.GroupID,"      ",6))fputs("        ",map_output);
    else {
        sscanf((const char *)header.GroupID,"%hd",&group);
        fprintf(map_output,"%8hX",group);
    }
    fputs(" gid\n",map_output);
    fprintf(map_output,"%8ho mode\n%8lX size\n",mode,size);
    if(memcmp(header.EndHeader,"`\n",2))fputs("in",map_output);
    fputs("correct header end\n",map_output);
}
void __stdcall dump_archive_members(OutputLibrary *library,const char *name);
char *__stdcall display_symbol_name(const char *,DWORD);
void __cdecl dump_archive_export_names(void)
{
    DumperArchiveExport *entry,*end;
    char *display;
    if(!dumper_context->options.dump_exports || !dump_archive_export_count)return;
    entry=dump_archive_exports;
    end=entry+dump_archive_export_count;
    fprintf(map_output,"\n     Exports\n\n       ordinal    name\n\n");
    for(;entry<end;++entry) {
        display=display_symbol_name(entry->name,0);
        if(entry->has_ordinal)fprintf(map_output,"         %5u",entry->ordinal);
        else fprintf(map_output,"              ");
        fprintf(map_output,"    %s\n",display);
        if(display!=entry->name)free(display);
    }
}
void __stdcall dump_archive(const char *name)
{
    OutputLibrary *library;
    default_phase="DumpArchive";
    fputs("\nFile Type: LIBRARY\n",map_output);
    dumper_context->file_header.Machine=0;
    dump_archive_export_count=0;
    library=create_library(name,NULL,&dumper_context->pointer_list);
    dump_archive_linker_members(library);
    dump_archive_members(library,name);
    dump_archive_export_names();
}
void __stdcall dump_separate_debug_file(const char *name) { __asm { int 3 } }

/* The original recognizes NE, but only emits its unsupported-format warning. */
void __stdcall dump_ne_file(const char *name) { link_warning(NULL,4095,name); }
void __stdcall dump_unknown_file(const char *name) { __asm { int 3 } }
void __stdcall dump_pef_file(void *image) { __asm { int 3 } }
BYTE __stdcall is_supported_coff_file(const char *name,const IMAGE_FILE_HEADER *header)
{
    /* The dumper accepts the same encountered machine numbers as the input
     * reader, including the period target-only ARM/SH/0x200/0x601 entries. */
    switch(header->Machine) {
    case 0: case 0x14c: case 0x162: case 0x166: case 0x168:
    case 0x184: case 0x1a2: case 0x1a6: case 0x1c0:
    case 0x1f0: case 0x200: case 0x601:
        return 1;
    default:
        link_warning(name,4048);
        return 0;
    }
}
#pragma intrinsic(strcmp)
int __cdecl compare_output_section_names(const void *left,const void *right)
{
    const OutputSection *a=*(const OutputSection * const *)left;
    const OutputSection *b=*(const OutputSection * const *)right;
    return strcmp(a->name,b->name);
}
void __stdcall sort_output_sections(SectionList *,int (__cdecl *)(const void *,const void *));
void __stdcall sort_output_sections_by_name(SectionList *sections)
{
    sort_output_sections(sections,compare_output_section_names);
}
}

extern "C" {
extern const char *output_file_name;
extern BYTE let_exceptions_escape;
extern InputArguments section_attributes;
}
#pragma intrinsic(strcmp)
extern "C" void __cdecl configure_dumper_options(void)
{
    InputArgument *argument=option_arguments.first;
    WORD index,position;
    BYTE raw_enabled=1;
    const char *text;
    for(index=0;index<option_arguments.count;++index,argument=argument->next) {
        text=argument->name;
        if(!strcmp(text,"?"))dumper_usage();
        if(!_stricmp(text,"all")) {
            dumper_context->options.dump_raw_data=raw_enabled;
            dumper_context->options.dump_headers=1;
            dumper_context->options.dump_relocations=1;
            dumper_context->options.dump_base_relocations=1;
            dumper_context->options.dump_line_numbers=1;
            dumper_context->options.dump_symbols=1;
            dumper_context->options.dump_imports=1;
            dumper_context->options.dump_exports=1;
            dumper_context->options.dump_summary=1;
            dumper_context->options.dump_architecture=1;
            dumper_context->options.dump_archive_members=1;
            dumper_context->options.dump_fpo=1;
            dumper_context->options.dump_pdata=1;
            dumper_context->options.dump_omap_from=1;
            dumper_context->options.dump_omap_to=1;
            dumper_context->options.dump_fixups=1;
            dumper_context->options.dump_directives=1;
            dumper_context->options.dump_linker_members=3;
        } else if(!_stricmp(text,"arch"))dumper_context->options.dump_architecture=1;
        else if(!_stricmp(text,"archivemembers"))dumper_context->options.dump_archive_members=1;
        else if(!_stricmp(text,"convert"))dumper_context->options.dump_convert=1;
        else if(!_stricmp(text,"dependents"))dumper_context->options.dump_dependents=1;
        else if(!_stricmp(text,"directives"))dumper_context->options.dump_directives=1;
        else if(!_stricmp(text,"disasm"))dumper_context->options.dump_disassembly=1;
        else if(!_stricmp(text,"exports"))dumper_context->options.dump_exports=1;
        else if(!_stricmp(text,"fixup"))dumper_context->options.dump_fixups=1;
        else if(!_stricmp(text,"fpo"))dumper_context->options.dump_fpo=1;
        else if(!_stricmp(text,"headers"))dumper_context->options.dump_headers=1;
        else if(!_stricmp(text,"imports"))dumper_context->options.dump_imports=1;
        else if(!_stricmp(text,"linenumbers"))dumper_context->options.dump_line_numbers=1;
        else if(!_strnicmp(text,"linkermember",12)) {
            if(!text[12])dumper_context->options.dump_linker_members|=3;
            else {
                if(text[12]!=':' || text[13]<'1' || text[13]>'2' || text[14])
                    link_fatal(NULL,1117,text);
                dumper_context->options.dump_linker_members|=(WORD)((signed char)text[13]-'0');
            }
        } else if(!_stricmp(text,"map"))dumper_context->options.dump_map=1;
        else if(!_stricmp(text,"omapf"))dumper_context->options.dump_omap_from=1;
        else if(!_stricmp(text,"omapt"))dumper_context->options.dump_omap_to=1;
        else if(!_strnicmp(text,"out:",4)) {
            if(text[4]) {
                output_file_name=text+4;
                map_output=fopen(output_file_name,"wt");
                if(!map_output)link_fatal(NULL,1104,output_file_name);
            }
        } else if(!_stricmp(text,"pdata"))dumper_context->options.dump_pdata=1;
        else if(!_strnicmp(text,"rawdata",7)) {
            dumper_context->options.dump_raw_data=1;
            if(text[7]==':') {
                position=8;
                if(!_strnicmp(text+8,"bytes",5)) {
                    dumper_context->options.dump_raw_format=0;position=13;
                } else if(!_strnicmp(text+8,"shorts",6)) {
                    dumper_context->options.dump_raw_format=1;position=14;
                } else if(!_strnicmp(text+8,"longs",5)) {
                    dumper_context->options.dump_raw_format=2;position=13;
                } else if(!_strnicmp(text+8,"none",4)) {
                    raw_enabled=0;dumper_context->options.dump_raw_data=0;position=12;
                }
                if(text[position]==',') {
                    dumper_context->options.dump_raw_count=0;
                    /* The actual %li call writes a DWORD starting at this
                     * WORD, also replacing the low WORD of raw_format. */
                    sscanf(text+position+1,"%li",(long *)&dumper_context->options.dump_raw_count);
                    if(!dumper_context->options.dump_raw_count)dump_width_warning=1;
                } else if(text[position])link_fatal(NULL,1117,text);
            } else if(text[7])link_fatal(NULL,1117,text);
        } else if(!_stricmp(text,"relocations")) {
            dumper_context->options.dump_relocations=1;
            dumper_context->options.dump_base_relocations=1;
        } else if(!_strnicmp(text,"section:",8)) {
            if(text[8])append_argument(&section_attributes,text+8);
        } else if(!_stricmp(text,"summary"))dumper_context->options.dump_summary=1;
        else if(!_strnicmp(text,"symbols",7))dumper_context->options.dump_symbols=1;
        else if(!_stricmp(text,"xoff"))let_exceptions_escape=1;
        else link_warning(NULL,4044,text);
    }
}

extern "C" int __stdcall run_dumper(int argc,char **argv)
{
    InputArgument *argument;
    WORD index;
    unsigned width;
    SectionEnumerator sections;
    OutputSection *section;
    if(argc<2)dumper_usage();
    dumper_context=(LinkContext *)allocate_link_memory(1,sizeof(LinkContext));
    dumper_context->Initialize(0);
    parse_command_line(argc,argv,NULL,NULL);
    configure_dumper_options();
    if(logo_pending)print_logo();
    if(dump_width_warning) {
        width=dumper_context->options.dump_raw_format==0 ? 16 :
            dumper_context->options.dump_raw_format==1 ? 8 : 4;
        link_warning(NULL,4038,width);
    }
    argument=link_files.first;
    for(index=0;index<link_files.count;++index,argument=argument->next) {
        if(index)fputc('\f',map_output);
        dump_file(argument->name);
    }
    if(dumper_context->options.dump_summary) {
        fputs("\n  Summary\n\n",map_output);
        sort_output_sections_by_name(&dumper_context->sections);
        initialize_section_enumerator(&sections,&dumper_context->sections);
        while(next_output_section(&sections)) {
            section=sections.current;
            fprintf(map_output,"    %8lX %s\n",section->raw_size,section->name);
        }
        end_section_enumerator(&sections);
    }
    fclose(map_output);
    close_files();
    cleanup_pdb();
    return 0;
}

extern "C" {
char *dump_string_cursor;
DWORD dump_string_bytes;
char *__stdcall read_module_string_table(const char *,DWORD,DWORD *);
void __stdcall read_dumper_strings(const char *name)
{
    DWORD bytes,offset;
    if(!dump_strings_owned) {
        offset=archive_member_offset+dumper_context->file_header.PointerToSymbolTable+
            dumper_context->file_header.NumberOfSymbols*18;
#ifdef __linux__
        /* The original forwards its reused argument slot as the size output.
         * A zero offset leaves that DWORD unwritten in ReadStringTable; the
         * current symbol-reader caller proves a nonzero table offset. */
        bytes=0;
        if(!offset) { __asm { int 3 } }
#endif
        dump_string_table=read_module_string_table(name,offset,&bytes);
        dump_string_cursor=dump_string_table;
        dump_strings_owned=1;
        dump_string_bytes=bytes;
    }
}
IMAGE_SYMBOL *__stdcall read_module_symbols(DWORD,DWORD,BYTE);
}
void RECON_THISCALL CoffDumper::ReadSymbolTable(IMAGE_SYMBOL **symbols)
{
    IMAGE_FILE_HEADER *header=&context->file_header;
    if(header->PointerToSymbolTable && header->NumberOfSymbols) {
        default_phase="ReadStringTable";
        read_dumper_strings(name);
        default_phase="ReadSymbolTable";
        *symbols=read_module_symbols(archive_member_offset+header->PointerToSymbolTable,
            header->NumberOfSymbols,0);
    } else *symbols=NULL;
}
extern "C" {
IMAGE_SYMBOL *dump_symbols;
IMAGE_SECTION_HEADER *dump_section_headers;
char *dump_string_table;
BYTE dump_strings_owned;
void __stdcall read_dumper_optional_header(int file,IMAGE_OPTIONAL_HEADER *header,DWORD raw_size)
{
    DWORD size=(WORD)raw_size;
    FileRecord *record;
    if(!size)return;
    if(size>sizeof(*header))size=sizeof(*header);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags&8)mapped_read(record,header,size);
    else buffered_read(record,header,size);
}
const char *__stdcall dumper_machine_name(DWORD machine)
{
    switch((WORD)machine) {
    case 0x14c:return "i386";
    case 0x162:return "R3000";
    case 0x166:return "R4000";
    case 0x168:return "R10000";
    case 0x184:return "Alpha AXP";
    case 0x1a2:return "SH3";
    case 0x1a6:return "SH4";
    case 0x1c0:return "ARM";
    case 0x1f0:return "PPC";
    case 0x200:return "ia64";
    case 0x601:return "MPPC";
    default:return "Unknown";
    }
}
/* The ROM view occupies the same actual optional-header bytes. */
typedef struct DumperRomHeader {
    WORD Magic; BYTE MajorLinkerVersion,MinorLinkerVersion;
    DWORD SizeOfCode,SizeOfInitializedData,SizeOfUninitializedData;
    DWORD AddressOfEntryPoint,BaseOfCode,BaseOfData,BaseOfBss;
    DWORD GprMask,CprMask[4],GpValue;
} DumperRomHeader;
void __stdcall prepare_dumper_file(DWORD archive)
{
    static const char *subsystems[]={"Unknown","Native","Windows GUI",
        "Windows CUI","Posix CUI","Native Windows","Windows CE GUI"};
    static const char *directories[]={"Export","Import","Resource","Exception",
        "Certificates","Base Relocation","Debug","Architecture","Special",
        "Thread Storage","Load Configuration","Bound Import",
        "Import Address Table","Delay Import"};
    IMAGE_FILE_HEADER *header=&dumper_context->file_header;
    IMAGE_OPTIONAL_HEADER *optional=&dumper_context->optional_header;
    DumperRomHeader *rom=(DumperRomHeader *)optional;
    const char *text;
    char linker_version[32],os_version[32],image_version[32],subsystem_version[32];
    DWORD bits,bit,index,count;
    default_phase="DumpHeaders";
    if(header->SizeOfOptionalHeader)
        read_dumper_optional_header(current_input_file,optional,header->SizeOfOptionalHeader);
    if(!dump_file_type)
        dump_file_type=header->SizeOfOptionalHeader && optional->Magic==0x107 ? 3:1;
    if(!(BYTE)archive) {
        if(dump_file_type==1)fputs("\nFile Type: COFF OBJECT\n",map_output);
        else if(dump_file_type==2)fputs((header->Characteristics&0x2000)?
            "\nFile Type: DLL\n":"\nFile Type: EXECUTABLE IMAGE\n",map_output);
        else if(dump_file_type==3)fputs("\nFile Type: ROM IMAGE\n",map_output);
        else fputs("\nFile Type: UNKNOWN\n",map_output);
    }
    if(!dumper_context->options.dump_headers)return;
    fprintf(map_output,"\nFILE HEADER VALUES\n%8hX machine (%s)\n%8hX number of sections\n%8lX time date stamp",
        header->Machine,dumper_machine_name(header->Machine),header->NumberOfSections,header->TimeDateStamp);
    text=ctime((time_t *)&header->TimeDateStamp);
    if(text)fprintf(map_output," %s",text);else fputc('\n',map_output);
    fprintf(map_output,"%8lX file pointer to symbol table\n%8lX number of symbols\n%8hX size of optional header\n%8hX characteristics\n",
        header->PointerToSymbolTable,header->NumberOfSymbols,header->SizeOfOptionalHeader,header->Characteristics);
    for(bits=header->Characteristics,bit=0;bits;bits>>=1,++bit) {
        if(!(bits&1))continue;
        switch(1UL<<bit) {
        case 1:text="Relocations stripped";break;
        case 2:text="Executable";break;
        case 4:text="Line numbers stripped";break;
        case 8:text="Symbols stripped";break;
        case 0x10:text="Aggressively trim working set";break;
        case 0x20:text="Application can handle large (>2GB) addresses";break;
        case 0x80:text="Bytes reversed";break;
        case 0x100:text="32 bit word machine";break;
        case 0x200:text="Debug information stripped";break;
        case 0x400:text="CD - run from swapfile";break;
        case 0x800:text="Net - run from swapfile";break;
        case 0x1000:text="System";break;
        case 0x2000:text="DLL";break;
        case 0x4000:text="Uniprocessor Only";break;
        case 0x8000:text="";break;
        default:text="RESERVED - UNKNOWN";break;
        }
        if(*text)fprintf(map_output,"            %s\n",text);
    }
    if(!header->SizeOfOptionalHeader)return;
    sprintf(linker_version,"%u.%02u",optional->MajorLinkerVersion,optional->MinorLinkerVersion);
    fprintf(map_output,"\nOPTIONAL HEADER VALUES\n%8hX magic #\n%8s linker version\n%8lX size of code\n%8lX size of initialized data\n%8lX size of uninitialized data\n%8lX address of entry point\n%8lX base of code\n%8lX base of data\n",
        optional->Magic,linker_version,optional->SizeOfCode,optional->SizeOfInitializedData,
        optional->SizeOfUninitializedData,optional->AddressOfEntryPoint,optional->BaseOfCode,optional->BaseOfData);
    if(optional->Magic==0x10b) {
        switch(optional->Subsystem) {
        case 1:index=1;break;case 2:index=2;break;case 3:index=3;break;
        case 7:index=4;break;case 8:index=5;break;case 9:index=6;break;
        default:index=0;break;
        }
        sprintf(os_version,"%hu.%02hu",optional->MajorOperatingSystemVersion,optional->MinorOperatingSystemVersion);
        sprintf(image_version,"%hu.%02hu",optional->MajorImageVersion,optional->MinorImageVersion);
        sprintf(subsystem_version,"%hu.%02hu",optional->MajorSubsystemVersion,optional->MinorSubsystemVersion);
        fprintf(map_output,"%8lX image base\n%8lX section alignment\n%8lX file alignment\n%8s operating system version\n%8s image version\n%8s subsystem version\n%8lX Win32 version\n%8lX size of image\n%8lX size of headers\n%8lX checksum\n%8hX subsystem (%s)\n%8hX DLL characteristics\n",
            optional->ImageBase,optional->SectionAlignment,optional->FileAlignment,
            os_version,image_version,subsystem_version,optional->Win32VersionValue,
            optional->SizeOfImage,optional->SizeOfHeaders,optional->CheckSum,
            optional->Subsystem,subsystems[index],optional->DllCharacteristics);
        for(bits=optional->DllCharacteristics,bit=0;bits;bits>>=1,++bit) {
            if(!(bits&1))continue;
            text=(1UL<<bit)==0x1000?"Wx86 Thunk DLL":
                (1UL<<bit)==0x2000?"WDM Driver":"RESERVED - UNKNOWN";
            fprintf(map_output,"            %s\n",text);
        }
        fprintf(map_output,"%8lX size of stack reserve\n%8lX size of stack commit\n%8lX size of heap reserve\n%8lX size of heap commit\n%8lX loader flags\n%8lX number of directories\n",
            optional->SizeOfStackReserve,optional->SizeOfStackCommit,optional->SizeOfHeapReserve,
            optional->SizeOfHeapCommit,optional->LoaderFlags,optional->NumberOfRvaAndSizes);
        count=optional->NumberOfRvaAndSizes<16?optional->NumberOfRvaAndSizes:16;
        for(WORD directory=0;directory<count;++directory)
            fprintf(map_output,"%8lX [%8lX] address [size] of %s Directory\n%",
                optional->DataDirectory[directory].VirtualAddress,optional->DataDirectory[directory].Size,
                directory<14?directories[directory]:"Reserved");
    } else if(optional->Magic==0x107) {
        /* Actual formatting consumes only the low WORD of the transported
         * GpValue DWORD; the other ROM fields are printed full-width. */
        fprintf(map_output,"         ----- rom -----\n%8lX base of bss\n%8lX gpr mask\n         cpr mask\n         %08lX %08lX %08lX %08lX\n%8hX gp value\n",
            rom->BaseOfBss,rom->GprMask,rom->CprMask[0],rom->CprMask[1],rom->CprMask[2],rom->CprMask[3],rom->GpValue);
    }
    fputc('\n',map_output);
}


IMAGE_SYMBOL *__stdcall next_coff_symbol(IMAGE_SYMBOL **);
void __cdecl dump_coff_map(void)
{
    IMAGE_SYMBOL *cursor=dump_symbols,*symbol;
    DWORD index=0,auxiliary,length;
    const char *name;
    char short_name[9];
    short_name[8]=0;
    default_phase="DumpSymbolMap";
    fputs("\nSYMBOL MAP\n",map_output);
    while(index<dumper_context->file_header.NumberOfSymbols) {
        ++index;
        symbol=next_coff_symbol(&cursor);
        auxiliary=symbol->NumberOfAuxSymbols;
        index+=auxiliary;
        while(auxiliary--)next_coff_symbol(&cursor);
        if(symbol->SectionNumber<=0 || symbol->StorageClass!=IMAGE_SYM_CLASS_EXTERNAL)continue;
        if(!symbol->N.Name.Short)name=dump_string_table+symbol->N.Name.Long;
        else name=strncpy(short_name,(const char *)symbol->N.ShortName,8);
        if(*name=='_')++name;
        fprintf(map_output,"%08lX",symbol->Value);
        if(index+1<dumper_context->file_header.NumberOfSymbols &&
            cursor->SectionNumber==symbol->SectionNumber)length=cursor->Value-symbol->Value;
        else {
            IMAGE_SECTION_HEADER *section=dump_section_headers+symbol->SectionNumber-1;
            length=section->VirtualAddress+section->SizeOfRawData-symbol->Value;
        }
        fprintf(map_output," , %8lu",length);
        fprintf(map_output,", %s\n",name);
    }
}
void __stdcall dump_relocation_symbol(FILE *,const char *,IMAGE_SYMBOL *);
#pragma intrinsic(strcpy,strcat,strlen,memset)
void __stdcall format_dumper_symbol(IMAGE_SYMBOL *symbol,char *text)
{
    const char *type,*storage;
    char *cursor;
    unsigned length,derived;
    int shift;
    switch(symbol->SectionNumber) {
    case IMAGE_SYM_DEBUG:strcpy(text,"DEBUG  ");break;
    case IMAGE_SYM_ABSOLUTE:strcpy(text,"ABS    ");break;
    case 0:strcpy(text,"UNDEF  ");break;
    default:
        sprintf(text,"SECT%hX ",symbol->SectionNumber);
        if(symbol->SectionNumber<16)strcat(text," ");
        break;
    }
    cursor=text+strlen(text);
    switch(symbol->Type&0xf) {
    case 0:type="notype";break;
    case 1:type="void";break;
    case 2:type="char";break;
    case 3:type="short";break;
    case 4:type="int";break;
    case 5:type="long";break;
    case 6:type="float";break;
    case 7:type="double";break;
    case 8:type="struct";break;
    case 9:type="union";break;
    case 10:type="enum";break;
    case 11:type="moe";break;
    case 12:type="byte";break;
    case 13:type="word";break;
    case 14:type="uint";break;
    case 15:type="dword";break;
    default:type="????";break;
    }
    strcpy(cursor,type);
    strcat(cursor," ");
    length=strlen(cursor);
    cursor+=length;
    for(shift=14;shift>=4;shift-=2) {
        derived=(symbol->Type>>shift)&3;
        if(derived==1) { length+=sprintf(cursor,"*");++cursor; }
        if(derived==3) { length+=sprintf(cursor,"[]");cursor+=2; }
        if(derived==2) { length+=sprintf(cursor,"()");cursor+=2; }
    }
    if(length<12) { memset(cursor,' ',12-length);cursor+=12-length; }
    *cursor++=' ';
    switch(symbol->StorageClass) {
    case 0:storage="NoClass";break;
    case 1:storage="AutoVar";break;
    case IMAGE_SYM_CLASS_EXTERNAL:storage="External";break;
    case IMAGE_SYM_CLASS_STATIC:storage="Static";break;
    case 4:storage="RegisterVar";break;
    case 5:storage="ExternalDef";break;
    case IMAGE_SYM_CLASS_LABEL:storage="Label";break;
    case IMAGE_SYM_CLASS_UNDEFINED_LABEL:storage="UndefinedLabel";break;
    case 8:storage="MemberOfStruct";break;
    case 9:storage="FunctionArg";break;
    case IMAGE_SYM_CLASS_STRUCT_TAG:storage="StructTag";break;
    case 11:storage="MemberOfUnion";break;
    case IMAGE_SYM_CLASS_UNION_TAG:storage="UnionTag";break;
    case 13:storage="TypeDefinition";break;
    case IMAGE_SYM_CLASS_UNDEFINED_STATIC:storage="UndefinedStatic";break;
    case IMAGE_SYM_CLASS_ENUM_TAG:storage="EnumTag";break;
    case 16:storage="MemberOfEnum";break;
    case 17:storage="RegisterParam";break;
    case 18:storage="BitField";break;
    case IMAGE_SYM_CLASS_FAR_EXTERNAL:storage="Far External";break;
    case IMAGE_SYM_CLASS_BLOCK:
        storage=symbol->N.ShortName[1]=='b'?"BeginBlock":
            symbol->N.ShortName[1]=='e'?"EndBlock":".bb or.eb";break;
    case IMAGE_SYM_CLASS_FUNCTION:
        storage=symbol->N.ShortName[1]=='b'?"BeginFunction":
            symbol->N.ShortName[1]=='e'?"EndFunction":".bf or.ef";break;
    case IMAGE_SYM_CLASS_END_OF_STRUCT:storage="EndOfStruct";break;
    case IMAGE_SYM_CLASS_FILE:storage="Filename";break;
    case IMAGE_SYM_CLASS_SECTION:storage="Section";break;
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:storage="WeakExternal";break;
    case 0xff:storage="EndOfFunction";break;
    default:
        sprintf(cursor,"0x%hx ",(unsigned)symbol->StorageClass);
        cursor+=strlen(cursor);
        storage="UNKNOWN SYMBOL CLASS";break;
    }
    strcpy(cursor,storage);
}
void __stdcall dump_coff_symbol(IMAGE_SYMBOL *symbol)
{
    char description[256];
    fprintf(map_output,"%08lX ",symbol->Value);
    description[0]=0;
    format_dumper_symbol(symbol,description);
    fprintf(map_output,"%-32s | ",description);
    dump_relocation_symbol(map_output,"%s\n",symbol);
}
void __stdcall dump_aux_symbol(IMAGE_SYMBOL *symbol,DWORD raw_index,IMAGE_AUX_SYMBOL *aux)
{
    const char *selection,*search;
    unsigned i;
    WORD index=(WORD)raw_index;
    BYTE *raw=(BYTE *)aux;
    fputs("    ",map_output);
    switch(symbol->StorageClass) {
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        fprintf(map_output,"Default index %8lX",aux->Sym.TagIndex);
        switch(aux->Sym.Misc.TotalSize) {
        case 1:search=" No library search\n";break;
        case 2:search=" library search\n";break;
        case 3:search=" Alias record\n";break;
        default:search=" Unknown\n";break;
        }
        fputs(search,map_output);
        return;
    case IMAGE_SYM_CLASS_STATIC:
        if((!symbol->Value || dump_file_type!=1) && symbol->SectionNumber>0 &&
            !symbol->Type && (symbol->N.ShortName[0]=='.' || aux->Section.Length)) {
            if(index==1) { fprintf(map_output,"Relocation CRC %08lX\n",aux->Section.Length);return; }
            if(!index) {
                fprintf(map_output,"Section length % 4lX, #relocs % 4hX, #linenums % 4hX",
                    aux->Section.Length,(unsigned)aux->Section.NumberOfRelocations,
                    (unsigned)aux->Section.NumberOfLinenumbers);
                fprintf(map_output,", checksum %8lX",aux->Section.CheckSum);
                if(dump_section_headers[symbol->SectionNumber-1].Characteristics&IMAGE_SCN_LNK_COMDAT) {
                    fprintf(map_output,", selection % 4hX",(unsigned)aux->Section.Selection);
                    switch(aux->Section.Selection) {
                    case 1:selection="no duplicates";break;
                    case 2:selection="any";break;
                    case 3:selection="same size";break;
                    case 4:selection="exact match";break;
                    case 5:selection="associative";break;
                    case 6:selection="largest";break;
                    case 7:selection="newest";break;
                    default:selection="unknown";break;
                    }
                    if(aux->Section.Selection==5)
                        fprintf(map_output," (pick %s Section %hx)",selection,(int)aux->Section.Number);
                    else fprintf(map_output," (pick %s)",selection);
                }
                fputc('\n',map_output);
                return;
            }
        }
        /* Other static auxiliaries use the ordinary symbol record view. */
    case IMAGE_SYM_CLASS_EXTERNAL:
        fprintf(map_output,"tag index %08lX size %08lX lines %08lX next function %08lX\n",
            aux->Sym.TagIndex,aux->Sym.Misc.TotalSize,
            aux->Sym.FcnAry.Function.PointerToLinenumber,aux->Sym.FcnAry.Function.PointerToNextFunction);
        return;
    case IMAGE_SYM_CLASS_STRUCT_TAG:
    case IMAGE_SYM_CLASS_UNION_TAG:
    case IMAGE_SYM_CLASS_ENUM_TAG:
    case IMAGE_SYM_CLASS_END_OF_STRUCT:
        fprintf(map_output,"tag index %08lX size %08lX\n",aux->Sym.TagIndex,aux->Sym.Misc.TotalSize);
        return;
    case IMAGE_SYM_CLASS_BLOCK:
    case IMAGE_SYM_CLASS_FUNCTION:
        fprintf(map_output,"line# %04hx",(unsigned)aux->Sym.Misc.LnSz.Linenumber);
        if(!strncmp((const char *)symbol,".b",2))
            fprintf(map_output," end %08lX",aux->Sym.FcnAry.Function.PointerToNextFunction);
        fputc('\n',map_output);
        return;
    case IMAGE_SYM_CLASS_FILE:
        fprintf(map_output,"%-18.18s\n",aux);
        return;
    default:
        if((symbol->Type&0x30)==0x30) {
            fputs("Array Bounds ",map_output);
            for(i=0;i<4;++i)if(aux->Sym.FcnAry.Array.Dimension[i])
                fprintf(map_output,"[%04X]",(unsigned)aux->Sym.FcnAry.Array.Dimension[i]);
        } else for(i=0;i<sizeof(IMAGE_AUX_SYMBOL);++i) {
            fprintf(map_output,"%1X",(unsigned)raw[i]>>4);
            fprintf(map_output,"%1X ",(unsigned)raw[i]&0xf);
        }
        fputc('\n',map_output);
        return;
    }
}
void __cdecl dump_coff_symbols(void)
{
    IMAGE_SYMBOL *symbol=dump_symbols,*current;
    DWORD index=0,i;
    BYTE count;
    default_phase="DumpCoffSymbols";
    if(!dumper_context->file_header.PointerToSymbolTable ||
        !dumper_context->file_header.NumberOfSymbols)return;
    fputs("\nCOFF SYMBOL TABLE\n",map_output);
    while(index<dumper_context->file_header.NumberOfSymbols) {
        current=symbol++;
        fprintf(map_output,"%03lX ",index++);
        dump_coff_symbol(current);
        count=current->NumberOfAuxSymbols;
        if(count) {
            if(!strncmp((const char *)current,".file",5)) {
                fprintf(map_output,"    %s\n",symbol);
                symbol+=count;
            } else for(i=0;i<count;++i)dump_aux_symbol(current,i,(IMAGE_AUX_SYMBOL *)symbol++);
            index+=count;
        }
    }
    fprintf(map_output,"\nString Table Size = 0x%lX bytes\n",dump_string_bytes);
}
void __stdcall release_module_symbols(void *);
void __stdcall release_module_strings(void *);
}
#pragma intrinsic(memset)
void RECON_THISCALL CoffDumper::Dump(DWORD archive,DWORD header_bytes)
{
    DumperSectionState directories;
    memset(&directories,0,sizeof(directories));
    prepare_dumper_file(archive);
    ReadSymbolTable(&dump_symbols);
    dump_sections(&directories,0,header_bytes,archive);
    if(dumper_context->options.dump_map)dump_coff_map();
    if(dumper_context->options.dump_symbols)dump_coff_symbols();
    if(dump_symbols) {
        release_module_symbols(dump_symbols);
        dump_symbols=NULL;
    }
    if(dump_section_headers) {
        release_memory(dump_section_headers);
        dump_section_headers=NULL;
    }
    if(dump_strings_owned) {
        release_module_strings(dump_string_table);
        dump_strings_owned=0;
    }
}

/* Archive/object/PE dump driver and its mode-private scratch state. */
#include "dump.h"
#pragma intrinsic(strlen)
Image *dump_image;
unsigned char *dump_buffer;
unsigned short dump_buffer_size=0xfc00;
unsigned char dump_invalid_columns;
extern FILE *map_stream;

/* Original [0002fee0,00030210). */
int dumper_main(int argc,char **argv)
{
    unsigned short index,width;
    Argument *argument;
    SectionEnumerator sections;
    if (argc<2) {
        setvbuf(stdout,NULL,0,1);
        if (banner_pending) print_banner();
        printf("usage: DUMPBIN [options] [files]\n\n");
        printf("   options:\n\n");
        printf("      /ALL\n");
        printf("      /ARCHIVEMEMBERS\n");
        printf("      /DISASM\n");
        printf("      /EXPORTS\n");
        printf("      /FPO\n");
        printf("      /HEADERS\n");
        printf("      /IMPORTS\n");
        printf("      /LINENUMBERS\n");
        printf("      /LINKERMEMBER[:{1|2}]\n");
        printf("      /OUT:filename\n");
        printf("      /RAWDATA[:{NONE|BYTES|SHORTS|LONGS}[,#]]\n");
        printf("      /RELOCATIONS\n");
        printf("      /SECTION:name\n");
        printf("      /SUMMARY\n");
        printf("      /SYMBOLS\n");
        printf("      /UNDECORATE\n");
        return 3;
    }
    create_image(&dump_image);
    collect_arguments(argc,argv,NULL);
    process_dump_options();
    if (banner_pending) print_banner();
    if (dump_invalid_columns) {
        width=4;
        if (dump_image->options.dump.raw_format==DUMP_BYTES) width=16;
        else if (dump_image->options.dump.raw_format==DUMP_SHORTS) width=8;
        warning_message(program_name,0x6d,width,"");
    }
    if (dump_image->options.dump.raw_data) {
        dump_buffer=malloc(dump_buffer_size);
        if (!dump_buffer) {
            dump_buffer_size=0x200;
            dump_buffer=malloc(dump_buffer_size);
            if (!dump_buffer) out_of_memory();
        }
    }
    if (library_files.count) convert_omf_inputs(1,&dump_image->options);
    argument=library_files.first;
    for (index=0;index<library_files.count;++index,argument=argument->next)
        dump_file(argument->text,argument->original,0);
    argument=other_inputs.first;
    for (index=0;index<other_inputs.count;++index,argument=argument->next)
        dump_file(argument->text,argument->original,1);
    if (dump_image->options.dump.display.listing.summary) {
        fprintf(map_stream,"\n     Summary\n");
        initialize_section_enumerator(&sections,&dump_image->sections);
        while (next_section(&sections))
            fprintf(map_stream,"    %8.8s % 8lX\n",sections.current->name,sections.current->raw_size);
    }
    if (dump_buffer) free(dump_buffer);
    close_all_files();
    remove_temporary_inputs();
    fclose(map_stream);
    return 0;
}

/* Original [00030210,000307c0). Dump switch abbreviations are prefix matches. */
void process_dump_options(void)
{
    Argument *argument=switches.first;
    char *text,*value;
    unsigned short index,used;
    unsigned long length;
    unsigned char raw_enabled=1,recognized;
    DumpOptions *options=&dump_image->options.dump;
    for (index=0;index<switches.count;++index,argument=argument->next) {
        text=argument->text;
        if (!_strnicmp(text,"out:",4)) {
            if (text[4]) {
                map_filename=text+4;
                map_stream=fopen(map_filename,"wt");
                if (!map_stream) fatal_message(program_name,7,map_filename);
            }
            continue;
        }
        if (!_strnicmp(text,"rawdata",7)) {
            options->raw_data=1;
            if (text[7]==':' && text[8]) {
                used=8;
                value=text+8;
                if (!_strnicmp(value,"bytes",5)) { used=13; options->raw_format=DUMP_BYTES; }
                else if (!_strnicmp(value,"shorts",6)) { used=14; options->raw_format=DUMP_SHORTS; }
                else if (!_strnicmp(value,"longs",5)) { used=13; options->raw_format=DUMP_LONGS; }
                else if (!_strnicmp(value,"none",4)) { used=12; options->raw_data=0; raw_enabled=0; }
                if (text[used]==',') {
                    /* Native %li writes the WORD count and both following flags. */
                    sscanf(text+used+1,"%li",(long *)&options->line.columns);
                    if (!options->line.columns) dump_invalid_columns=1;
                } else if (text[used]) fatal_message(program_name,0x14,text);
            } else if (text[7]) fatal_message(program_name,0x14,text);
            continue;
        }
        if (!_strnicmp(text,"section:",8)) {
            if (text[8]) append_argument(&section_overrides,text+8);
            continue;
        }
        length=strlen(text);
        recognized=0;
        if (!_strnicmp(text,"all",length)) {
            options->raw_data=raw_enabled;
            options->headers=options->relocations=options->base_relocations=1;
            options->line_numbers=options->symbols=options->imports=options->exports=1;
            options->display.listing.summary=options->display.listing.archive_members=1;
            options->linker_members=1;
            recognized=1;
        }
        if (!_strnicmp(text,"rawdata",length)) { options->raw_data=1; recognized=1; }
        if (!_strnicmp(text,"headers",length)) { options->headers=1; recognized=1; }
        if (!_strnicmp(text,"relocations",length)) { options->relocations=options->base_relocations=1; recognized=1; }
        if (!_strnicmp(text,"linenumbers",length)) { options->line_numbers=1; recognized=1; }
        if (!_strnicmp(text,"map",length)) { options->display.listing.map=1; recognized=1; }
        if (!_strnicmp(text,"symbols",length)) { options->symbols=1; recognized=1; }
        if (!_strnicmp(text,"fpo",length)) { options->display.listing.fpo=1; recognized=1; }
        if (!_strnicmp(text,"imports",length)) { options->imports=1; recognized=1; }
        if (!_strnicmp(text,"exports",length)) { options->exports=1; recognized=1; }
        if (!_strnicmp(text,"summary",length)) { options->display.listing.summary=1; recognized=1; }
        if (!_strnicmp(text,"archivemembers",length)) { options->display.listing.archive_members=1; recognized=1; }
        if (!_strnicmp(text,"linkermember:1",length)) { ++options->linker_members; recognized=1; }
        if (!_strnicmp(text,"linkermember:2",length)) { options->linker_members+=2; recognized=1; }
        if (!_strnicmp(text,"undecorate",length)) { options->symbols=options->undecorate=1; recognized=1; }
        if (!_strnicmp(text,"warnings",length)) { options->line.warnings=1; recognized=1; }
        if (!_strnicmp(text,"disasm",length)) {
            options->line.disassemble=1;
            initialize_disassembler(fatal_message,warning_message);
            recognized=1;
        }
        if (!recognized) warning_message(program_name,0x73,text,"");
    }
}

unsigned long dump_file_size;
unsigned short dump_file_count;
PIMAGE_SYMBOL dump_symbol_table;
PIMAGE_SECTION_HEADER dump_section_headers;
extern char *coff_strings;
extern unsigned long pe_header_offset;

/* Original [000307c0,00030ba0). Inspect one object/PE or every archive member. */
void dump_file(const char *name,const char *source,unsigned char archive)
{
    Library *library;
    PIMAGE_ARCHIVE_MEMBER_HEADER member;
    unsigned long archive_size,signature;
    unsigned short magic;
    IMAGE_DOS_HEADER dos;
    DumpOptions *options=&dump_image->options.dump;
    archive_member_size=archive_data_offset=0;
    input_file=open_file(source,0x8000,0);
    dump_file_size=file_size(input_file);
    if (dump_file_size<20) fatal_message(name,0x27,"");
    if (dump_file_count++) fputc(12,map_stream);
    fprintf(options->display.listing.map?stderr:map_stream,"\nDump of file %s\n",name);
    if (archive) {
        archive_size=file_size(input_file);
        fprintf(map_stream,"%s","!<arch>\n");
        fprintf(map_stream,"\nFile Type: LIBRARY\n");
        archive_data_offset=8;
        dump_image->file.Machine=0;
        library=create_library(name,0,1,&dump_image->libraries);
        read_archive_index(library,options->linker_members,dump_image);
    }
    do {
        if (archive) {
            member=read_archive_header();
            if (options->display.listing.archive_members)
                dump_archive_header(library,*member,tell_file(input_file)-60);
        }
        read_file(input_file,&magic,2);
        seek_file(input_file,-2,SEEK_CUR);
        pe_header_offset=0;
        if (magic==IMAGE_DOS_SIGNATURE) {
            read_file(input_file,&dos,64);
            if ((unsigned long)dos.e_lfanew+4>=dump_file_size) {
                fprintf(map_stream,"\n*** Invalid PE signature offset 0x%x ***\n",dos.e_lfanew);
                goto next_member;
            }
            seek_file(input_file,dos.e_lfanew,SEEK_SET);
            read_file(input_file,&signature,4);
            if (options->headers) {
                if (signature!=IMAGE_NT_SIGNATURE) {
                    fprintf(map_stream,"\nPE signature not found\n");
                    goto next_member;
                }
                fprintf(map_stream,"\nNT signature found\n");
            }
            pe_header_offset=tell_file(input_file);
        }
        abort_phase="DumpHeaders";
        dump_headers(name,archive);
        dump_string_table(name);
        if (dump_image->file.PointerToSymbolTable && dump_image->file.NumberOfSymbols)
            dump_symbol_table=read_coff_symbols(dump_image->file.PointerToSymbolTable+
                archive_data_offset,dump_image->file.NumberOfSymbols);
        else dump_symbol_table=NULL;
        abort_phase="DumpSections";
        dump_sections();
        if (dump_image->file.PointerToSymbolTable && dump_image->file.NumberOfSymbols &&
            (options->display.listing.map || options->symbols || options->display.listing.fpo)) {
            if (options->display.listing.map) {
                abort_phase="DumpSymbolMap";
                dump_symbol_map();
            }
            if (options->symbols) {
                abort_phase="DumpSymbols";
                dump_coff_symbols();
            }
        }
        if (dump_symbol_table) {
            release_coff_symbols(dump_symbol_table);
            dump_symbol_table=NULL;
        }
        if (dump_section_headers) {
            free(dump_section_headers);
            dump_section_headers=NULL;
        }
        release_coff_strings(coff_strings);
        coff_strings=NULL;
next_member:
        ;
    } while (archive && archive_member_size+archive_data_offset+1<archive_size);
    close_file(input_file,1);
    release_library_list(&dump_image->libraries);
}

/* Original [00030ba0,00030d50). Archive metadata, including native end-marker text. */
void dump_archive_header(Library *library,IMAGE_ARCHIVE_MEMBER_HEADER header,unsigned long offset)
{
    long date,size;
    short uid,gid,mode;
    char *name,*time;
    sscanf((char *)header.Date,"%ld",&date);
    sscanf((char *)header.UserID,"%hd",&uid);
    sscanf((char *)header.GroupID,"%hd",&gid);
    sscanf((char *)header.Mode,"%ho",&mode);
    sscanf((char *)header.Size,"%ld",&size);
    if (dump_file) {
        fprintf(map_stream,"\nArchive member name at %lX: %.16s",offset,header.Name);
        if (library && header.Name[0]=='/') {
            name=get_archive_member_name(library,&header);
            fprintf(map_stream,"%s",name?name:"member corrupt");
        }
        fputc('\n',map_stream);
        fprintf(map_stream,"% 8lX time/date",date);
        time=ctime((time_t *)&date);
        if (time) fprintf(map_stream," %s",time);
        else fputc('\n',map_stream);
        fprintf(map_stream,"% 8hX uid\n% 8hX gid\n% 8hX mode\n% 8lX size\n",uid,gid,mode,size);
        if (strncmp((char *)header.EndHeader,"`\n",2)) fprintf(map_stream,"in");
        fprintf(map_stream,"correct header end\n");
    }
}

typedef struct DumpRomHeader {
    unsigned short magic;
    unsigned char linker_major,linker_minor;
    unsigned long code_size,data_size,bss_size,entry,code_base,data_base,bss_base;
    unsigned long gpr_mask,cpr_mask[4],gp_value;
} DumpRomHeader;
extern unsigned long section_header_offset;
unsigned char valid_coff_machine(IMAGE_FILE_HEADER *);

/* Original [00030d50,00031420). COFF and optional-header values. */
void dump_headers(const char *name,unsigned char archive)
{
    static const char *machine_names[]={"Unknown","i860","i386","r3000","r4000","M68K"};
    static const char *subsystem_names[]={"Unknown","Native","Windows GUI","Windows CUI","OS/2 CUI","Posix CUI"};
    static const char *directory_names[]={"Export","Import","Resource","Exception","Security",
        "Base Relocation","Debug","Description","Special","Thread Storage",NULL};
    IMAGE_FILE_HEADER *file=&dump_image->file;
    IMAGE_OPTIONAL_HEADER *optional=&dump_image->optional;
    DumpRomHeader *rom=(DumpRomHeader *)optional;
    unsigned short machine,subsystem,flags,bit,length,spaces,index;
    const char *description;
    char *date,version[32];
    read_coff_header(input_file,file);
    if (!valid_coff_machine(file)) fatal_message(name,0x27,"");
    if (file->SizeOfOptionalHeader)
        read_optional_header(input_file,optional,file->SizeOfOptionalHeader);
    if (file->Characteristics&IMAGE_FILE_DLL) fprintf(map_stream,"\nFile Type: DLL\n");
    else if (file->Characteristics&IMAGE_FILE_EXECUTABLE_IMAGE)
        fprintf(map_stream,"\nFile Type: EXECUTABLE IMAGE\n");
    else if (!archive) {
        if (!file->SizeOfOptionalHeader) fprintf(map_stream,"\nFile Type: COFF OBJECT\n");
        else fprintf(map_stream,"\nFile Type: UNKNOWN\n");
    }
    section_header_offset=file->SizeOfOptionalHeader+pe_header_offset+20;
    if (!dump_image->options.dump.headers) return;
    switch (file->Machine) {
    case 0x14c: machine=2; break;
    case 0x14d: machine=1; break;
    case 0x162: machine=3; break;
    case 0x166: machine=4; break;
    case 0x268: machine=5; break;
    default: machine=0; break;
    }
    fprintf(map_stream,"\nFILE HEADER VALUES\n% 8hX machine (%s)\n% 8hX number of sections\n% 8lX time date stamp",
        file->Machine,machine_names[machine],file->NumberOfSections,file->TimeDateStamp);
    date=ctime((time_t *)&file->TimeDateStamp);
    if (date) fprintf(map_stream," %s",date);
    else fputc('\n',map_stream);
    fprintf(map_stream,"% 8lX file pointer to symbol table\n% 8lX number of symbols\n% 8hX size of optional header\n% 8hX characteristics\n",
        file->PointerToSymbolTable,file->NumberOfSymbols,file->SizeOfOptionalHeader,file->Characteristics);
    bit=0;
    for (flags=file->Characteristics;flags;flags>>=1,++bit) {
        if (!(flags&1)) continue;
        switch (1UL<<bit) {
        case 1: description="Relocations stripped"; break;
        case 2: description="Executable"; break;
        case 4: description="Line numbers stripped"; break;
        case 8: description="Local symbols stripped"; break;
        case 0x10: description="RESERVED - Minimal object"; break;
        case 0x20: description="RESERVED - Update object"; break;
        case 0x40: description="16 bit word machine"; break;
        case 0x80: description="Bytes reversed"; break;
        case 0x100: description="32 bit word machine"; break;
        case 0x400: description="RESERVED - Patch"; break;
        case 0x1000: description="System"; break;
        case 0x2000: description="DLL"; break;
        case 0x8000: description=""; break;
        default: description="RESERVED - UNKNOWN"; break;
        }
        if (*description) fprintf(map_stream,"            %s\n",description);
    }
    if (file->SizeOfOptionalHeader) {
        fprintf(map_stream,"\nOPTIONAL HEADER VALUES\n% 8hX magic #\n",optional->Magic);
        length=(unsigned short)sprintf(version,"%d.%d",optional->MajorLinkerVersion,optional->MinorLinkerVersion);
        if (length>8) length=8;
        for (spaces=8-length;spaces;--spaces) fputc(' ',map_stream);
        fprintf(map_stream,"%s linker version\n% 8lX size of code\n% 8lX size of initialized data\n% 8lX size of uninitialized data\n% 8lX address of entry point\n% 8lX base of code\n% 8lX base of data\n",
            version,optional->SizeOfCode,optional->SizeOfInitializedData,optional->SizeOfUninitializedData,
            optional->AddressOfEntryPoint,optional->BaseOfCode,optional->BaseOfData);
    }
    if (file->SizeOfOptionalHeader==56)
        fprintf(map_stream,"         ----- rom -----\n% 8lX base of bss\n% 8lX gpr mask\n         cpr mask\n         %08lX %08lX %08lX %08lX\n% 8hX gp value\n",
            rom->bss_base,rom->gpr_mask,rom->cpr_mask[0],rom->cpr_mask[1],rom->cpr_mask[2],rom->cpr_mask[3],rom->gp_value);
    if (file->SizeOfOptionalHeader!=224) return;
    switch (optional->Subsystem) {
    case 1: subsystem=1; break;
    case 2: subsystem=2; break;
    case 3: subsystem=3; break;
    case 5: subsystem=4; break;
    case 7: subsystem=5; break;
    default: subsystem=0; break;
    }
    fprintf(map_stream,"         ----- new -----\n% 8lX image base\n% 8lX section alignment\n% 8lX file alignment\n% 8hX subsystem (%s)\n",
        optional->ImageBase,optional->SectionAlignment,optional->FileAlignment,
        optional->Subsystem,subsystem_names[subsystem]);
    length=(unsigned short)sprintf(version,"%hX.%hX",optional->MajorOperatingSystemVersion,optional->MinorOperatingSystemVersion);
    if (length>8) length=8;
    for (spaces=8-length;spaces;--spaces) fputc(' ',map_stream);
    fprintf(map_stream,"%s operating system version\n",version);
    length=(unsigned short)sprintf(version,"%hX.%hX",optional->MajorImageVersion,optional->MinorImageVersion);
    if (length>8) length=8;
    for (spaces=8-length;spaces;--spaces) fputc(' ',map_stream);
    fprintf(map_stream,"%s image version\n",version);
    length=(unsigned short)sprintf(version,"%hX.%hX",optional->MajorSubsystemVersion,optional->MinorSubsystemVersion);
    if (length>8) length=8;
    for (spaces=8-length;spaces;--spaces) fputc(' ',map_stream);
    fprintf(map_stream,"%s subsystem version\n% 8lX size of image\n% 8lX size of headers\n% 8lX checksum\n",
        version,optional->SizeOfImage,optional->SizeOfHeaders,optional->CheckSum);
    fprintf(map_stream,"% 8lX size of stack reserve\n% 8lX size of stack commit\n% 8lX size of heap reserve\n% 8lX size of heap commit\n%",
        optional->SizeOfStackReserve,optional->SizeOfStackCommit,optional->SizeOfHeapReserve,optional->SizeOfHeapCommit);
    for (index=0;index<16 && directory_names[index];++index)
        fprintf(map_stream,"% 8lX [% 8lx] address [size] of %s Directory\n%",
            optional->DataDirectory[index].VirtualAddress,optional->DataDirectory[index].Size,directory_names[index]);
    fputc('\n',map_stream);
}

DumpStringTable dump_string_view;
typedef char dump_string_view_layout[(sizeof(DumpStringTable)==8)?1:-1];

/* Original [00032f70,00032fc0). Publish the current member's COFF string table. */
void dump_string_table(const char *name)
{
    unsigned long size;
    coff_strings=read_coff_strings(name,dump_image->file.PointerToSymbolTable+
        dump_image->file.NumberOfSymbols*18+archive_data_offset,&size);
    dump_string_view.data=coff_strings;
    dump_string_view.size=size;
}

void disassemble_section(IMAGE_SECTION_HEADER *section,unsigned short index,PIMAGE_SYMBOL symbols,unsigned long count,unsigned char image,DumpStringTable *strings,int file,FILE *stream);
void dump_fpo(IMAGE_SECTION_HEADER *section);
void dump_debug_directory(IMAGE_SECTION_HEADER *section);
void dump_imports(IMAGE_SECTION_HEADER *section);
void dump_exports(IMAGE_SECTION_HEADER *section);
unsigned char valid_dump_range(unsigned long offset,unsigned long size);
void dump_base_relocation_block(IMAGE_BASE_RELOCATION *block,unsigned short *fixups);
void dump_relocations(unsigned long count,IMAGE_SECTION_HEADER *section);
void dump_lines(unsigned long count);
#include <ctype.h>
/* This shared row is mutated for each sixteen-byte ASCII preview. */
char dump_ascii[]="12345678|12345678";

/* Original [00031420,00032410), including its two trailing dispatch tables. */
void dump_sections(void)
{
    IMAGE_SECTION_HEADER header;
    IMAGE_BASE_RELOCATION relocation;
    IMAGE_OPTIONAL_HEADER *optional=&dump_image->optional;
    DumpOptions *options=&dump_image->options.dump;
    Argument *argument;
    Section *summary;
    PIMAGE_SYMBOL symbol;
    unsigned short index,other,selected,permissions,column,columns,chunk_size,ascii_index;
    unsigned long flags,bit,symbol_index,name_offset,remaining,offset,position,bytes;
    unsigned char found,custom_columns=0,character;
    unsigned char *data,*ascii,*end;
    unsigned short *fixups;
    RawDumpFormat format;
    const char *description,*symbol_name;
    int printed,spaces;
    dump_section_headers=malloc(dump_image->file.NumberOfSections*40);
    if (!dump_section_headers) out_of_memory();
    seek_file(input_file,section_header_offset+archive_data_offset,SEEK_SET);
    read_file(input_file,dump_section_headers,dump_image->file.NumberOfSections*40);
    section_header_offset+=dump_image->file.NumberOfSections*40;
    argument=section_overrides.first;
    for (other=0;other<section_overrides.count;++other,argument=argument->next) {
        found=0;
        for (index=1;index<=dump_image->file.NumberOfSections;++index) {
            header=dump_section_headers[index-1];
            if (!strncmp((char *)header.Name,argument->text,8)) { found=1; break; }
        }
        if (!found) warning_message(program_name,0x6e,argument->text,"");
    }
    for (index=1;index<=dump_image->file.NumberOfSections;++index) {
        header=dump_section_headers[index-1];
        if (section_overrides.count) {
            argument=section_overrides.first;
            for (selected=0;selected<section_overrides.count;++selected,argument=argument->next)
                if (!strncmp(argument->text,(char *)header.Name,8)) break;
            if (selected>=section_overrides.count) continue;
        }
        if (options->display.listing.summary) {
            summary=create_section((char *)header.Name,header.Characteristics,
                &dump_image->sections,optional);
            summary->raw_size+=header.SizeOfRawData;
        }
        if (options->headers || section_overrides.count) {
            fprintf(map_stream,"\nSECTION HEADER #%hX\n% 8.8s name",index,header.Name);
            if (header.Name[0]=='/' && sscanf((char *)header.Name+1,"%lu",&name_offset)==1)
                fprintf(map_stream," (%s)",coff_strings+name_offset);
            fprintf(map_stream,"\n");
            fprintf(map_stream,"% 8lX %s\n% 8lX virtual address\n% 8lX size of raw data\n% 8lX file pointer to raw data\n% 8lX file pointer to relocation table\n",
                header.Misc.VirtualSize,dump_image->file.SizeOfOptionalHeader?"virtual size":"physical address",
                header.VirtualAddress,header.SizeOfRawData,header.PointerToRawData,header.PointerToRelocations);
            fprintf(map_stream,"% 8lX file pointer to line numbers\n% 8hX number of relocations\n% 8hX number of line numbers\n% 8lX flags\n",
                header.PointerToLinenumbers,header.NumberOfRelocations,header.NumberOfLinenumbers,header.Characteristics);
            permissions=0;
            flags=header.Characteristics;
            if (dump_image->file.Machine!=0x268) flags&=0xff8fffffUL;
            for (bit=0;flags;flags>>=1,++bit) {
                if (!(flags&1)) continue;
                switch (1UL<<bit) {
                case 0: description="Regular"; break;
                case 1: description="Dummy"; break;
                case 2: description="No Load"; break;
                case 4: description="Grouped"; break;
                case 8: description="No Pad"; break;
                case 0x10: description="Copy"; break;
                case 0x20: description="Code"; break;
                case 0x40: description="Initialized Data"; break;
                case 0x80: description="Uninitialized Data"; break;
                case 0x100: description="Other"; break;
                case 0x200: description="Info"; break;
                case 0x400: description="Overlay"; break;
                case 0x800: description="Remove"; break;
                case 0x1000: description="Communal"; break;
                case 0x80000: description="Protected"; break;
                case 0x100000: description="Far Data"; break;
                case 0x200000: description="Sys Heap"; break;
                case 0x400000: description="Purgeable"; break;
                case 0x800000: description="Locked"; break;
                case 0x1000000: description="Preload"; break;
                case 0x2000000: description="Discardable"; break;
                case 0x4000000: description="Not Cached"; break;
                case 0x8000000: description="Not Paged"; break;
                case 0x10000000: description="Shared"; break;
                case 0x20000000: description=""; permissions|=1; break;
                case 0x40000000: description=""; permissions|=2; break;
                case 0x80000000: description=""; permissions|=4; break;
                default: description="RESERVED - UNKNOWN"; break;
                }
                if (!*description) continue;
                fprintf(map_stream,"         %s",description);
                if ((1UL<<bit)==0x1000 && dump_symbol_table) {
                    found=0;
                    for (symbol_index=0;symbol_index<dump_image->file.NumberOfSymbols;++symbol_index) {
                        symbol=&dump_symbol_table[symbol_index];
                        if (symbol->SectionNumber!=index) continue;
                        if (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
                            (symbol->StorageClass==IMAGE_SYM_CLASS_STATIC &&
                             (symbol->NumberOfAuxSymbols!=1 || (symbol->Type&0x30)==0x20))) {
                            found=1;
                            break;
                        }
                        symbol_index+=symbol->NumberOfAuxSymbols;
                    }
                    if (found) {
                        if (symbol->N.Name.Short)
                            symbol_name=strncpy(short_symbol_name,(char *)symbol,8);
                        else symbol_name=dump_string_view.data+symbol->N.Name.Long;
                        fprintf(map_stream,"; sym= %s",symbol_name);
                    } else fprintf(map_stream," (no symbol)");
                }
                fputc('\n',map_stream);
            }
            if (dump_image->file.Machine!=0x268) {
                switch (header.Characteristics&0x700000) {
                case 0x100000: description="1 byte align"; break;
                case 0x200000: description="2 byte align"; break;
                case 0x300000: description="4 byte align"; break;
                case 0x400000: description="8 byte align"; break;
                case 0x500000: description="16 byte align"; break;
                case 0x600000: description="32 byte align"; break;
                case 0x700000: description="64 byte align"; break;
                default: description="(no align specified)"; break;
                }
                fprintf(map_stream,"         %s\n",description);
            }
            if (permissions) {
                switch (permissions) {
                case 1: description="Execute Only"; break;
                case 2: description="Read Only"; break;
                case 3: description="Execute Read"; break;
                case 4: description="Write Only"; break;
                case 5: description="Execute Write"; break;
                case 6: description="Read Write"; break;
                case 7: description="Execute Read Write"; break;
                default: description="Unknown Memory Flags"; break;
                }
                fprintf(map_stream,"         %s\n",description);
            }
        }
        if (options->line.disassemble && (header.Characteristics&0x1e0)==0x20 && header.SizeOfRawData) {
            if (dump_image->file.Machine==0x14c)
                disassemble_section(&header,index,dump_symbol_table,dump_image->file.NumberOfSymbols,
                    (unsigned char)(dump_image->file.SizeOfOptionalHeader!=0),&dump_string_view,input_file,map_stream);
            else printf("Warning: disassembly not supported for this target machine\n");
        }
        if (options->raw_data && (header.Characteristics&0x1e0)!=0x80 && header.SizeOfRawData) {
            seek_file(input_file,header.PointerToRawData+archive_data_offset,SEEK_SET);
            fprintf(map_stream,"\nRAW DATA #%hX\n",index);
            remaining=header.SizeOfRawData;
            chunk_size=dump_buffer_size;
            offset=0;
            format=options->raw_format;
            columns=options->line.columns;
            if (!columns) {
                columns=4;
                if (format==DUMP_BYTES) columns=16;
                else if (format==DUMP_SHORTS) columns=8;
            } else custom_columns=1;
            while (remaining) {
                if (remaining<chunk_size) chunk_size=(unsigned short)remaining;
                read_file(input_file,dump_buffer,chunk_size);
                remaining-=chunk_size;
                data=ascii=dump_buffer;
                end=dump_buffer+chunk_size;
                while (data<end) {
                    printed=fprintf(map_stream,"%08lX  ",offset);
                    if (!custom_columns) {
                        for (ascii_index=0;ascii_index<17;++ascii_index) {
                            if (ascii_index==8) continue;
                            if (ascii<end) {
                                character=*ascii++;
                                dump_ascii[ascii_index]=(char)(isprint(character)?character:'.');
                            } else dump_ascii[ascii_index]=' ';
                        }
                    }
                    for (column=columns;column;--column) {
                        if (!custom_columns && (columns>>1)==column)
                            printed+=fprintf(map_stream,"| ");
                        if (data>=end) break;
                        switch (format) {
                        case DUMP_BYTES:
                            printed+=fprintf(map_stream,"%02X ",*data++);
                            ++offset;
                            break;
                        case DUMP_SHORTS:
                            printed+=fprintf(map_stream,"%04hX ",*(unsigned short *)data);
                            data+=2; offset+=2;
                            if (end<data+2) format=DUMP_BYTES;
                            break;
                        case DUMP_LONGS:
                            printed+=fprintf(map_stream,"%08lX ",*(unsigned long *)data);
                            data+=4; offset+=4;
                            if (data+4>end) {
                                format=DUMP_SHORTS;
                                if (data+2>end) format=DUMP_BYTES;
                            }
                            break;
                        }
                    }
                    if (custom_columns) fputc('\n',map_stream);
                    else {
                        for (spaces=61-printed;spaces;--spaces) fputc(' ',map_stream);
                        fprintf(map_stream,"%s\n",dump_ascii);
                    }
                }
            }
        }
        if (options->display.listing.fpo && !strncmp(".debug$F",(char *)header.Name,8)) dump_fpo(&header);
        position=optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        if (dump_image->file.SizeOfOptionalHeader && header.VirtualAddress<=position &&
            position<header.VirtualAddress+header.SizeOfRawData) {
            if (options->display.listing.fpo) dump_fpo(&header);
            if (options->headers) dump_debug_directory(&header);
        }
        position=optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        if (options->imports && position && header.VirtualAddress<=position &&
            position<header.VirtualAddress+header.SizeOfRawData) dump_imports(&header);
        position=optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        if (options->exports && position && header.VirtualAddress<=position &&
            position<header.VirtualAddress+header.SizeOfRawData) dump_exports(&header);
        position=optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        if (options->base_relocations && position && header.VirtualAddress<=position &&
            position<header.VirtualAddress+header.SizeOfRawData) {
            fprintf(map_stream,"\nBASE RELOCATIONS\n");
            position=header.PointerToRawData-header.VirtualAddress+position;
            if (valid_dump_range(position,0)) {
                seek_file(input_file,position,SEEK_SET);
                bytes=optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
                while (bytes) {
                    read_file(input_file,&relocation,8);
                    if (!relocation.SizeOfBlock) break;
                    fixups=calloc(relocation.SizeOfBlock-8,4);
                    if (!fixups) out_of_memory();
                    read_file(input_file,fixups,relocation.SizeOfBlock-8);
                    dump_base_relocation_block(&relocation,fixups);
                    bytes-=relocation.SizeOfBlock;
                    free(fixups);
                }
            } else warning_message(program_name,0x65,position,"BASERELOCATIONS");
        }
        if (options->relocations && header.NumberOfRelocations) {
            fprintf(map_stream,"\nRELOCATIONS #%hX\n",index);
            position=header.PointerToRelocations+archive_data_offset;
            if (valid_dump_range(position,header.NumberOfRelocations*10)) {
                seek_file(input_file,position,SEEK_SET);
                dump_relocations(header.NumberOfRelocations,&header);
            } else warning_message(program_name,0x65,position,"RELOCATIONS");
        }
        if (options->line_numbers && header.NumberOfLinenumbers) {
            fprintf(map_stream,"\nLINENUMBERS #%hX\n",index);
            position=header.PointerToLinenumbers+archive_data_offset;
            if (valid_dump_range(position,header.NumberOfLinenumbers*6)) {
                seek_file(input_file,position,SEEK_SET);
                dump_lines(header.NumberOfLinenumbers);
            } else warning_message(program_name,0x65,position,"LINENUMBERS");
        }
    }
}

/* Original [000344a0,000344c0). Validate a dump read against the current input. */
unsigned char valid_dump_range(unsigned long offset,unsigned long size)
{
    return (unsigned char)(offset+size<=dump_file_size);
}

/* Original [00032410,00032ce0). COFF relocation records and machine-specific names. */
void dump_relocations(unsigned long count,IMAGE_SECTION_HEADER *section)
{
    PIMAGE_RELOCATION records=calloc(count,10),record;
    const char *description;
    if (!records) out_of_memory();
    read_coff_relocations(input_file,records,count);
    for (record=records;count;--count,++record) {
        fprintf(map_stream,"% 8lX virtual address, % 8lX ",record->VirtualAddress,record->SymbolTableIndex);
        fprintf(map_stream,"symbol table index, ");
        switch (dump_image->file.Machine) {
        case 0x14c:
            switch (record->Type) {
            case 0: description="ABS"; break;
            case 1: description="DIR16"; break;
            case 2: description="REL16"; break;
            case 6: description="DIR32"; break;
            case 7: description="DIR32NB"; break;
            case 9: description="SEG12"; break;
            case 10: description="SECT#"; break;
            case 11: description="SECREL"; break;
            case 0x14: description="REL32"; break;
            default: fprintf(map_stream,"0x%hx ",record->Type); description="UNKNOWN i386 RELOCATION"; break;
            }
            break;
        case 0x14d:
            switch (record->Type) {
            case 0: description="ABS"; break;
            case 6: description="DIR32"; break;
            case 7: description="DIR32NB"; break;
            case 10: description="SECT#"; break;
            case 11: description="SECREL"; break;
            case 0x1c: description="PAIR"; break;
            case 0x1e: description="HIGH"; break;
            case 0x1f: description="LOW0"; break;
            case 0x20: description="LOW1"; break;
            case 0x21: description="LOW2"; break;
            case 0x22: description="LOW3"; break;
            case 0x23: description="LOW4"; break;
            case 0x24: description="SPLIT0"; break;
            case 0x25: description="SPLIT1"; break;
            case 0x26: description="SPLIT2"; break;
            case 0x27: description="HIGHADJ"; break;
            case 0x28: description="BRADDR"; break;
            default: fprintf(map_stream,"0x%hx ",record->Type); description="UNKNOWN i860 RELOCATION"; break;
            }
            break;
        case 0x162: case 0x166:
            switch (record->Type) {
            case 0: description="ABS"; break;
            case 1: description="REFHALF"; break;
            case 2: description="REFWORD"; break;
            case 3: description="JMPADDR"; break;
            case 4: description="REFHI"; break;
            case 5: description="REFLO"; break;
            case 6: description="GPREL"; break;
            case 7: description="LITERAL"; break;
            case 10: description="SECT#"; break;
            case 11: description="SECREL"; break;
            case 0x22: description="REFWORDNB"; break;
            case 0x25: description="PAIR"; break;
            default: fprintf(map_stream,"0x%hx ",record->Type); description="UNKNOWN MIPS RELOCATION"; break;
            }
            break;
        case 0x268:
            switch (record->Type) {
            case 0: description="DTOD16"; break;
            case 1: description="DTOC16"; break;
            case 2: description="DTOD32"; break;
            case 3: description="DTOC32"; break;
            case 4: description="DTOABSD32"; break;
            case 5: description="DTOABSC32"; break;
            case 6: description="CTOD16"; break;
            case 7: description="CTOC16"; break;
            case 8: description="CTOT16"; break;
            case 9: description="CTOD32"; break;
            case 10: description="CTOABSD32"; break;
            case 11: description="CTOABSC32"; break;
            case 12: description="CTOABST32"; break;
            case 0x13: description="MACPROF32"; break;
            case 0x14: description="PCODETOC32"; break;
            case 0x15: description="CTOCS16"; break;
            case 0x16: description="CTOABSCS32"; break;
            case 0x17: description="CV"; break;
            case 0x19: description="DTOU16"; break;
            case 0x1a: description="DTOU32"; break;
            case 0x1b: description="DTOABSU32"; break;
            case 0x1c: description="CTOU16"; break;
            case 0x1d: description="CTOABSU32"; break;
            case 0x1e: description="DIFF8"; break;
            case 0x1f: description="DIFF16"; break;
            case 0x20: description="DIFF32"; break;
            case 0x23: description="CSECTABLEB16"; break;
            case 0x24: description="CSECTABLEW16"; break;
            case 0x25: description="CSECTABLEL16"; break;
            case 0x26: description="CSECTABLEBABS32"; break;
            case 0x27: description="CSECTABLEWABS32"; break;
            case 0x28: description="CSECTABLELABS32"; break;
            case 0x29: description="DUPCON16"; break;
            case 0x2a: description="DUPCONABS32"; break;
            case 0x2d: description="PCODESN16"; break;
            case 0x2e: description="PCODETOD24"; break;
            case 0x2f: description="PCODETOT24"; break;
            case 0x30: description="PCODETOCS24"; break;
            default: fprintf(map_stream,"0x%hx ",record->Type); description="UNKNOWN M68K RELOCATION"; break;
            }
            break;
        default: fprintf(map_stream,"0x%hx ",record->Type); description=" "; break;
        }
        fprintf(map_stream,"%s\n",description);
        if (section->VirtualAddress+section->SizeOfRawData<record->VirtualAddress+4 &&
            dump_image->options.dump.line.warnings)
            fprintf(map_stream,"Warning Relocations beyond Raw Data\n");
    }
    free(records);
}

void print_dump_symbol_name(FILE *,const char *,PIMAGE_SYMBOL);

/* Original [00032e20,00032f70). COFF line rows and function-name annotations. */
void dump_lines(unsigned long count)
{
    PIMAGE_LINENUMBER records=calloc(count,6),line;
    PIMAGE_SYMBOL symbol;
    unsigned short columns=5;
    if (!records) out_of_memory();
    read_coff_lines(input_file,records,count);
    for (line=records;count;--count,++line) {
        if (!columns) { columns=5; fputc('\n',map_stream); }
        if (!line->Linenumber && columns!=5) fputc('\n',map_stream);
        fprintf(map_stream,"%8lX %4hX  ",line->Type.VirtualAddress,line->Linenumber);
        if (!line->Linenumber) {
            if (!dump_symbol_table) fprintf(map_stream,"\n");
            else {
                symbol=NULL;
                if (line->Type.SymbolTableIndex<dump_image->file.NumberOfSymbols)
                    symbol=&dump_symbol_table[line->Type.SymbolTableIndex];
                if (symbol &&
                    (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
                     (symbol->StorageClass==IMAGE_SYM_CLASS_STATIC && (symbol->Type&0x30)==0x20)) &&
                    symbol->NumberOfAuxSymbols==1)
                    print_dump_symbol_name(map_stream,"sym=  %s\n",symbol);
                else fprintf(map_stream,"(error: invalid symbol index)\n");
            }
            columns=5;
        }
        --columns;
    }
    fputc('\n',map_stream);
    free(records);
}

/* Original [000337a0,00033840). Print a sanitized or undecorated COFF name. */
void print_dump_symbol_name(FILE *stream,const char *format,PIMAGE_SYMBOL symbol)
{
    char short_name[9],*name,*display;
    unsigned short index;
    unsigned char character;
    if (symbol->N.Name.Short) {
        for (index=0;index<8;++index) {
            character=((unsigned char *)symbol)[index];
            short_name[index]=(char)((character>0x1f && character<0x7f)?character:0);
        }
        short_name[8]=0;
        name=short_name;
    } else name=coff_strings+symbol->N.Name.Long;
    display=name;
    if (dump_image->options.dump.undecorate) display=undecorate_symbol_name(name,0);
    fprintf(stream,format,display);
    if (name!=display) free(display);
}

void print_dump_symbol(PIMAGE_SYMBOL);
void print_dump_auxiliary(PIMAGE_SYMBOL,PIMAGE_AUX_SYMBOL);

/* Original [00033110,000332e0). COFF symbol records and their auxiliary entries. */
void dump_coff_symbols(void)
{
    PIMAGE_SYMBOL cursor=dump_symbol_table,symbol;
    IMAGE_SYMBOL saved;
    unsigned long index=0;
    unsigned short auxiliary_count,remaining;
    short part;
    char *filenames;
    fprintf(map_stream,"\nSYMBOL TABLE\n");
    while (index<dump_image->file.NumberOfSymbols) {
        fprintf(map_stream,"%03lX ",index++);
        symbol=next_coff_symbol(&cursor,0);
        print_dump_symbol(symbol);
        auxiliary_count=symbol->NumberOfAuxSymbols;
        if (!strncmp((char *)symbol,".file",5)) {
            filenames=calloc(1,(short)auxiliary_count*18+1);
            if (!filenames) out_of_memory();
            if (auxiliary_count) {
                index+=auxiliary_count;
                for (part=0;part<(short)auxiliary_count;++part)
                    ((PIMAGE_SYMBOL)filenames)[part]=*next_coff_symbol(&cursor,1);
            }
            fprintf(map_stream,"    %s\n",filenames);
            for (part=1;part<(short)auxiliary_count;++part)
                fprintf(map_stream,"    (%.18s)\n",filenames+part*18);
            free(filenames);
        } else if (auxiliary_count) {
            saved=*symbol;
            index+=auxiliary_count;
            remaining=auxiliary_count;
            do {
                symbol=next_coff_symbol(&cursor,1);
                print_dump_auxiliary(&saved,(PIMAGE_AUX_SYMBOL)symbol);
            } while (--remaining);
        }
    }
    fprintf(map_stream,"\nSTRING TABLE size = 0x%lx bytes\n",dump_string_view.size);
}

/* Original [000332e0,000337a0). One COFF symbol's value, name, type and class. */
void print_dump_symbol(PIMAGE_SYMBOL symbol)
{
    static const char *base_types[]={"notype","void","char","short","int","long","float","double",
        "struct","union","enum","moe","uchar","ushort","uint","ulong"};
    static const char *classes[]={"NoClass","AutoVar","External","Static","RegisterVar","ExternalDef",
        "Label","UndefinedLabel","MemberOfStruct","FunctionArg","StructTag","MemberOfUnion","UnionTag",
        "TypeDefinition","UndefinedStatic","EnumTag","MemberOfEnum","RegisterParam","BitField"};
    const char *description;
    unsigned long printed,depth,spaces;
    unsigned short modifier;
    fprintf(map_stream,"%08lX ",symbol->Value);
    print_dump_symbol_name(map_stream,"%-32s ",symbol);
    if (symbol->SectionNumber>0) fprintf(map_stream,"SECT%hX ",symbol->SectionNumber);
    else {
        switch (symbol->SectionNumber) {
        case -2: description="DEBUG"; break;
        case -1: description="ABS  "; break;
        case 0: description="UNDEF"; break;
        default: fprintf(map_stream,"0x%hx ",symbol->SectionNumber); description="?????"; break;
        }
        fprintf(map_stream,"%s ",description);
    }
    printed=fprintf(map_stream,"%s ",base_types[symbol->Type&15]);
    for (depth=0;depth<6;++depth) {
        modifier=(unsigned short)((symbol->Type>>((7-depth)*2))&3);
        if (modifier==1) printed+=fprintf(map_stream,"*");
        if (modifier==3) printed+=fprintf(map_stream,"[]");
        if (modifier==2) printed+=fprintf(map_stream,"()");
    }
    if (printed<12) for (spaces=12-printed;spaces;--spaces) fputc(' ',map_stream);
    fputc(' ',map_stream);
    if (symbol->StorageClass<19) description=classes[symbol->StorageClass];
    else {
        switch (symbol->StorageClass) {
        case 0x44: description="Far External"; break;
        case 100:
            if (((char *)symbol)[1]=='b') description="BeginBlock";
            else if (((char *)symbol)[1]=='e') description="EndBlock";
            else description=".bb or.eb";
            break;
        case 101:
            if (((char *)symbol)[1]=='b') description="BeginFunction";
            else if (((char *)symbol)[1]=='e') description="EndFunction";
            else description=".bf or.ef";
            break;
        case 102: description="EndOfStruct"; break;
        case 103: description="Filename"; break;
        case 104: description="Section"; break;
        case 105: description="WeakExternal"; break;
        case 255: description="EndOfFunction"; break;
        default: fprintf(map_stream,"0x%hx ",symbol->StorageClass); description="UNKNOWN SYMBOL CLASS"; break;
        }
    }
    fprintf(map_stream,"%s\n",description);
}

typedef struct DumpWeakAuxiliary {
    unsigned long default_index,search;
} DumpWeakAuxiliary;

/* Original [00033840,00033c20). Interpret an auxiliary using its owning symbol. */
void print_dump_auxiliary(PIMAGE_SYMBOL symbol,PIMAGE_AUX_SYMBOL auxiliary)
{
    const char *selection;
    DumpWeakAuxiliary *weak=(DumpWeakAuxiliary *)auxiliary;
    unsigned short index;
    unsigned char *raw;
    fprintf(map_stream,"    ");
    switch (symbol->StorageClass) {
    case IMAGE_SYM_CLASS_EXTERNAL:
        fprintf(map_stream,"tag index %08lx size %08lx lines %08lx next function %08lx\n",
            auxiliary->Sym.TagIndex,auxiliary->Sym.Misc.TotalSize,
            auxiliary->Sym.FcnAry.Function.PointerToLinenumber,
            auxiliary->Sym.FcnAry.Function.PointerToNextFunction);
        return;
    case IMAGE_SYM_CLASS_STATIC:
        if (((char *)symbol)[0]=='.' || (!(symbol->Type&15) && auxiliary->Section.Length)) {
            fprintf(map_stream,"Section length % 4lX, #relocs % 4hX, #linenums % 4hX",
                auxiliary->Section.Length,auxiliary->Section.NumberOfRelocations,
                auxiliary->Section.NumberOfLinenumbers);
            if (symbol->SectionNumber>0 &&
                (dump_section_headers[symbol->SectionNumber-1].Characteristics&0x1000)) {
                fprintf(map_stream,", checksum % 8lX, selection % 4hX",
                    auxiliary->Section.CheckSum,auxiliary->Section.Selection);
                switch (auxiliary->Section.Selection) {
                case 1: selection="no duplicates"; break;
                case 2: selection="any"; break;
                case 3: selection="same size"; break;
                case 4: selection="exact match"; break;
                case 5: selection="associative"; break;
                default: selection="unknown"; break;
                }
                if (auxiliary->Section.Selection==5)
                    fprintf(map_stream," (pick %s Section %hx)",selection,auxiliary->Section.Number);
                else fprintf(map_stream," (pick %s)",selection);
            }
            fputc('\n',map_stream);
            return;
        }
        break;
    case IMAGE_SYM_CLASS_STRUCT_TAG:
    case IMAGE_SYM_CLASS_UNION_TAG:
    case IMAGE_SYM_CLASS_ENUM_TAG:
    case IMAGE_SYM_CLASS_END_OF_STRUCT:
        fprintf(map_stream,"tag index %08lx size %08lx\n",auxiliary->Sym.TagIndex,auxiliary->Sym.Misc.TotalSize);
        return;
    case IMAGE_SYM_CLASS_BLOCK:
    case IMAGE_SYM_CLASS_FUNCTION:
        fprintf(map_stream,"line# %04hx",auxiliary->Sym.Misc.LnSz.Linenumber);
        if (!strncmp((char *)symbol,".b",2))
            fprintf(map_stream," end %08lx",auxiliary->Sym.FcnAry.Function.PointerToNextFunction);
        fputc('\n',map_stream);
        return;
    case IMAGE_SYM_CLASS_FILE:
        fprintf(map_stream,"%-18.18s\n",auxiliary);
        return;
    case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        fprintf(map_stream,"Default index % 8lx",weak->default_index);
        switch (weak->search) {
        case 1: fprintf(map_stream," No library search\n"); break;
        case 2: fprintf(map_stream," library search\n"); break;
        case 3: fprintf(map_stream," Alias record\n"); break;
        default: fprintf(map_stream," Unknown\n"); break;
        }
        return;
    }
    if ((symbol->Type&0x30)==0x30) {
        fprintf(map_stream,"Array Bounds ");
        for (index=0;index<4;++index)
            if (auxiliary->Sym.FcnAry.Array.Dimension[index])
                fprintf(map_stream,"[%04x]",auxiliary->Sym.FcnAry.Array.Dimension[index]);
    } else {
        raw=(unsigned char *)auxiliary;
        for (index=0;index<18;++index) {
            fprintf(map_stream,"%1x",raw[index]>>4);
            fprintf(map_stream,"%1x ",raw[index]&15);
        }
    }
    fputc('\n',map_stream);
}

/* Original [00034730,00034850). Print the image's debug-directory descriptors. */
void dump_debug_directory(IMAGE_SECTION_HEADER *section)
{
    unsigned long rva=dump_image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    unsigned long count;
    IMAGE_DEBUG_DIRECTORY directory;
    if (!rva) return;
    seek_file(input_file,section->PointerToRawData-section->VirtualAddress+rva+archive_data_offset,SEEK_SET);
    count=dump_image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size/28;
    fprintf(map_stream,"\n\nDebug Directories(%d)\n",count);
    fprintf(map_stream,"\tType       Size     Address  Pointer\n\n");
    while (count--) {
        read_file(input_file,&directory,28);
        switch (directory.Type) {
        case 1: fprintf(map_stream,"\tcoff   "); break;
        case 2: fprintf(map_stream,"\tcv     "); break;
        case 3: fprintf(map_stream,"\tfpo    "); break;
        case 6: fprintf(map_stream,"\tfixup  "); break;
        default: fprintf(map_stream,"\t???    "); break;
        }
        fprintf(map_stream,"%8x    %8x %8x\n",directory.SizeOfData,directory.AddressOfRawData,directory.PointerToRawData);
    }
}

typedef struct DumpImportDescriptor {
    unsigned long lookup_rva,timestamp,forwarder,name_rva,address_rva;
} DumpImportDescriptor;
typedef char dump_import_descriptor_layout[(sizeof(DumpImportDescriptor)==20)?1:-1];

/* Original [00033c20,00033fe0). Import descriptors and named/ordinal thunks. */
void dump_imports(IMAGE_SECTION_HEADER *section)
{
    /* The native formatter shares this indentation object between its rows. */
    static const char indent[]="            ";
    DumpImportDescriptor descriptor;
    IMAGE_SECTION_HEADER *names;
    unsigned long descriptor_offset,lookup_offset,bound_offset,thunk,bound_value;
    unsigned short index,name_section,hint;
    unsigned char character;
    char *date;
    fprintf(map_stream,"\n         Section contains the following Imports\n");
    descriptor_offset=dump_image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress-
        section->VirtualAddress+section->PointerToRawData;
    if (!valid_dump_range(descriptor_offset,0)) {
        warning_message(program_name,0x65,descriptor_offset,"IMPORTS");
        return;
    }
    for (;;) {
        seek_file(input_file,descriptor_offset,SEEK_SET);
        read_file(input_file,&descriptor,20);
        if (!descriptor.lookup_rva && !descriptor.name_rva && !descriptor.address_rva) break;
        for (index=0;index<dump_image->file.NumberOfSections;++index) {
            names=&dump_section_headers[index];
            if (names->VirtualAddress<=descriptor.name_rva &&
                descriptor.name_rva<names->VirtualAddress+names->SizeOfRawData) {
                name_section=index;
                break;
            }
        }
        names=&dump_section_headers[name_section];
        seek_file(input_file,names->PointerToRawData-names->VirtualAddress+descriptor.name_rva,SEEK_SET);
        fprintf(map_stream,"\n%s",indent);
        for (;;) {
            read_file(input_file,&character,1);
            if (!character) break;
            fputc(character,map_stream);
        }
        fprintf(map_stream,"\n\n%s% 8lX characteristics\n",indent,descriptor.lookup_rva);
        fprintf(map_stream,"%s% 8lX time date stamp",indent,descriptor.timestamp);
        date=ctime((time_t *)&descriptor.timestamp);
        if (date) fprintf(map_stream," %s",date);
        else fputc('\n',map_stream);
        if (descriptor.forwarder!=0xffffffffUL)
            fprintf(map_stream,"%s% 8lX index of first forwarder reference\n",indent,descriptor.forwarder);
        lookup_offset=descriptor.address_rva+section->PointerToRawData-section->VirtualAddress;
        if (descriptor.lookup_rva && descriptor.timestamp) {
            bound_offset=lookup_offset;
            lookup_offset=descriptor.lookup_rva+section->PointerToRawData-section->VirtualAddress;
        } else bound_offset=0;
        for (;;) {
            seek_file(input_file,lookup_offset,SEEK_SET);
            read_file(input_file,&thunk,4);
            lookup_offset+=4;
            if (bound_offset) {
                seek_file(input_file,bound_offset,SEEK_SET);
                bound_offset+=4;
                read_file(input_file,&bound_value,4);
            }
            if (!thunk) break;
            fprintf(map_stream,"               ");
            if (bound_offset) fprintf(map_stream,"% 8X  ",bound_value);
            if (thunk&0x80000000UL) fprintf(map_stream,"Ordinal % 8lX\n",thunk);
            else {
                seek_file(input_file,names->PointerToRawData-names->VirtualAddress+thunk,SEEK_SET);
                read_file(input_file,&hint,2);
                fprintf(map_stream,"% 4hX   ",hint);
                for (;;) {
                    read_file(input_file,&character,1);
                    if (!character) break;
                    fputc(character,map_stream);
                }
                fputc('\n',map_stream);
            }
        }
        descriptor_offset+=20;
    }
}

typedef struct DumpExportDirectory {
    unsigned long characteristics,timestamp;
    unsigned short major,minor;
    unsigned long name_rva,base,function_count,name_count,functions_rva,names_rva,ordinals_rva;
} DumpExportDirectory;
typedef char dump_export_directory_layout[(sizeof(DumpExportDirectory)==40)?1:-1];

/* Original [00033fe0,000344a0). Preserve the native name-order/EAT-order pairing. */
void dump_exports(IMAGE_SECTION_HEADER *section)
{
    static const char indent[]="            ";
    DumpExportDirectory directory;
    unsigned long offset,index,*functions,*names,export_rva,export_size;
    unsigned short *ordinals,length,spaces;
    unsigned char character;
    char *date,version[32];
    export_rva=dump_image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    export_size=dump_image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    offset=export_rva-section->VirtualAddress+section->PointerToRawData;
    if (!valid_dump_range(offset,0)) {
        warning_message(program_name,0x65,offset,"EXPORTS");
        return;
    }
    seek_file(input_file,offset,SEEK_SET);
    read_file(input_file,&directory,40);
    seek_file(input_file,section->PointerToRawData-section->VirtualAddress+directory.name_rva,SEEK_SET);
    fprintf(map_stream,"\n         Section contains the following Exports for ");
    for (;;) {
        read_file(input_file,&character,1);
        if (!character) break;
        fputc(character,map_stream);
    }
    fprintf(map_stream,"\n\n%s% 8lX characteristics\n",indent,directory.characteristics);
    fprintf(map_stream,"%s% 8lX time date stamp",indent,directory.timestamp);
    date=ctime((time_t *)&directory.timestamp);
    if (date) fprintf(map_stream," %s",date);
    else fputc('\n',map_stream);
    length=(unsigned short)sprintf(version,"%hX.%hX",directory.major,directory.minor);
    if (length>8) length=8;
    for (spaces=8-length;spaces;--spaces) fputc(' ',map_stream);
    fprintf(map_stream,"%s%s version\n",indent,version);
    fprintf(map_stream,"%s% 8lX base\n",indent,directory.base);
    fprintf(map_stream,"%s% 8lX # functions\n",indent,directory.function_count);
    fprintf(map_stream,"%s% 8lX # names\n",indent,directory.name_count);
    fprintf(map_stream,"\n%sordinal hint   name\n\n",indent);
    functions=calloc(directory.function_count,4);
    if (!functions) out_of_memory();
    names=calloc(directory.name_count,4);
    if (!names) out_of_memory();
    ordinals=calloc(directory.name_count,2);
    if (!ordinals) out_of_memory();
    seek_file(input_file,section->PointerToRawData-section->VirtualAddress+directory.functions_rva,SEEK_SET);
    read_file(input_file,functions,directory.function_count*4);
    seek_file(input_file,section->PointerToRawData-section->VirtualAddress+directory.names_rva,SEEK_SET);
    read_file(input_file,names,directory.name_count*4);
    seek_file(input_file,section->PointerToRawData-section->VirtualAddress+directory.ordinals_rva,SEEK_SET);
    read_file(input_file,ordinals,directory.name_count*2);
    for (index=0;index<directory.name_count;++index) {
        seek_file(input_file,names[index]-section->VirtualAddress+section->PointerToRawData,SEEK_SET);
        fprintf(map_stream,"%s   % 4hX % 4hX   ",indent,
            (unsigned short)(ordinals[index]+directory.base),(unsigned short)index);
        for (;;) {
            read_file(input_file,&character,1);
            if (!character) break;
            fputc(character,map_stream);
        }
        if (export_rva<functions[index] && functions[index]<export_rva+export_size) {
            fprintf(map_stream," (forwarded to ");
            seek_file(input_file,functions[index]-section->VirtualAddress+section->PointerToRawData,SEEK_SET);
            for (;;) {
                read_file(input_file,&character,1);
                if (!character) break;
                fputc(character,map_stream);
            }
            fprintf(map_stream,")\n");
        } else fprintf(map_stream,"  (%08x)\n",functions[index]);
    }
    free(functions);
    free(names);
    free(ordinals);
}

/* Original [00032ce0,00032e20). Decode one base-relocation block. */
void dump_base_relocation_block(IMAGE_BASE_RELOCATION *block,unsigned short *fixups)
{
    static const char *names[]={"ABS","HIGH","LOW","HIGHLOW","HIGHADJ","JMPADDR","BRADDR","SPLIT"};
    unsigned long count=(block->SizeOfBlock-8)>>1;
    unsigned short entry,type;
    const char *description;
    fprintf(map_stream,"% 8lX virtual address, % 8lX SizeOfBlock\n",block->VirtualAddress,block->SizeOfBlock);
    while (count--) {
        entry=*fixups++;
        type=(unsigned short)(entry>>12);
        if (type<8) description=names[type];
        else {
            fprintf(map_stream,"0x%hx ",type);
            description="UNKNOWN BASED RELOCATION";
        }
        fprintf(map_stream,"% 8hX %s",entry&0xfff,description);
        /* The native HIGHADJ path consumes this WORD without another count decrement. */
        if (type==4) fprintf(map_stream," (%04hx)\n",*fixups++);
        else fprintf(map_stream,"\n");
    }
}

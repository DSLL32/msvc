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

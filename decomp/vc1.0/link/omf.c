/* Embedded OMF-to-COFF converter and its LINK input adapter. */
#include "omf.h"
#include <io.h>
jmp_buf omf_abort;
char omf_source_filename[260];
int omf_argument_index,omf_option_character,omf_debug_mode;
char *omf_option_argument,*omf_program_name;
int omf_option_offset=1;
unsigned short omf_external_count;
unsigned short omf_symbol_count;
unsigned short omf_group_count;
unsigned short omf_segment_count;
unsigned short omf_name_count;
unsigned short omf_module_count;
int omf_extensions_seen;
short omf_record_remaining;
unsigned short omf_record_wide;
unsigned short omf_record_type;
unsigned short omf_data_record_type;
unsigned short omf_data_segment;
unsigned long omf_data_offset;
int omf_pass;
unsigned char omf_modend_seen;
unsigned long omf_line_count;
unsigned long omf_symbol_fixup_bytes;
unsigned short omf_bss_section;
unsigned short omf_data_section;
unsigned short omf_text_section;
unsigned short omf_exestr_count;
unsigned short omf_defined_segment_count;
unsigned long omf_exestr_bytes;
unsigned short omf_type_segment_count;
unsigned short omf_symbol_segment_count;
unsigned short omf_first_type_segment;
unsigned short omf_first_symbol_segment;
unsigned char omf_record_checksum;
PIMAGE_LINENUMBER omf_line_data;
unsigned char * omf_symbol_fixup_data;
unsigned long omf_symbol_indices[4096];
unsigned long omf_external_indices[4096];
FILE *omf_input;
char *omf_names[2048];
int omf_local_names[2048];
unsigned long omf_coff_symbol_count;
PIMAGE_SYMBOL omf_coff_symbols;
unsigned long omf_coff_symbol_capacity;
unsigned long omf_coff_string_size;
char * omf_coff_strings;
unsigned long omf_coff_string_capacity;
char *omf_first_module_name;
OmfSymbol **omf_symbol_buckets;
OmfRelocationSymbol **omf_relocation_buckets;

/* Original [00020b30,00020ca0). */
void convert_omf_object(Argument *input,unsigned char link,ImageOptions *options)
{
    char *arguments[4],prefix[]="tmp",extension[260];
    int file;
    arguments[0]="cvtomf";
    arguments[1]=(options->debug_types&2)?"-co":"-o";
    arguments[2]=_tempnam("\\",prefix);
    if (!arguments[2]) fatal_message(program_name,7,"TEMPFILE");
    file=open_file(arguments[2],0x8302,0x180);
    close_file(file,1);
    arguments[3]=input->text;
    if (!setjmp(omf_abort)) {
        omf_main(4,arguments);
        input->original=duplicate_string(arguments[2]);
        if (link && omf_source_filename[0]) {
            _splitpath(omf_source_filename,NULL,NULL,NULL,extension);
            if (!_strcmpi(extension,".c") || !_strcmpi(extension,".cxx") ||
                !_strcmpi(extension,".cpp"))
                warning_message(input->text,0x68,"","");
        }
    } else fatal_message(input->text,0x1a,"");
}

/* Embedded converter command driver [00048010,00048150). */
int omf_main(int argc,char **argv)
{
    int option;
    char *output=NULL;
    omf_argument_index=1;
    omf_option_character=0;
    omf_option_argument=NULL;
    omf_program_name=argv[0];
    while ((option=omf_get_option(argc,argv,"?cgo:"))!=-1) {
        switch (option) {
        case '?':
            fprintf(stdout,"%s\n","version 1.3\nusage: cvtomf [-?] [-c] [-g] [-o output] file file file ...");
            exit(0);
            break;
        case 'c': omf_debug_mode=1; break;
        case 'g': omf_debug_mode=2; break;
        case 'o':
            if (output) omf_warning("Multiple -o options specified");
            output=omf_option_argument;
            break;
        }
    }
    if (argc-omf_argument_index>1 && output)
        omf_error("Cannot specify output file for more than one input");
    if (argc<=omf_argument_index) omf_error("No input file specified");
    do {
        if (omf_open_input(argv[omf_argument_index])) {
            omf_convert_pass(1);
            omf_write_coff(output?output:argv[omf_argument_index]);
            omf_close_input();
        }
        ++omf_argument_index;
    } while (omf_argument_index<argc);
    return 0;
}

/* Converter's getopt-style parser [00048220,000483d0). */
int omf_get_option(int argc,char **argv,const char *options)
{
    char *word,*match;
    int option;
    if (omf_option_offset==1) {
        if (argc<=omf_argument_index) return -1;
        word=argv[omf_argument_index];
        if (word[0]!='-' || !word[1]) return -1;
        if (word[1]=='-' && !word[2]) {
            ++omf_argument_index;
            return -1;
        }
    }
    word=argv[omf_argument_index];
    option=omf_option_character=(signed char)word[omf_option_offset];
    match=option==':'?NULL:strchr(options,option);
    if (!match) {
        fprintf(stdout,": illegal option -- %c\n",option);
        exit(1);
        ++omf_option_offset;
        if (!word[omf_option_offset]) {
            ++omf_argument_index;
            omf_option_offset=1;
        }
        return '?';
    }
    if (match[1]==':') {
        if (word[omf_option_offset+1]) {
            omf_option_argument=word+omf_option_offset+1;
            ++omf_argument_index;
            return option;
        }
        ++omf_argument_index;
        if (argc<=omf_argument_index) {
            fprintf(stdout,": option requires an argument -- %c\n",option);
            omf_option_offset=1;
            return '?';
        }
        omf_option_argument=argv[omf_argument_index++];
        omf_option_offset=1;
        return option;
    }
    ++omf_option_offset;
    if (!word[omf_option_offset]) {
        omf_option_offset=1;
        ++omf_argument_index;
    }
    omf_option_argument=NULL;
    return option;
}

/* Original [000453c0,00045460). */
int omf_open_input(const char *path)
{
    unsigned short machine;
    omf_input=fopen(path,"rb");
    if (!omf_input) omf_error("Cannot open \"%s\" for reading",path);
    if (fread(&machine,2,1,omf_input)!=1) omf_error("Cannot read \"%s\"",path);
    if (machine==IMAGE_FILE_MACHINE_I386) {
        fclose(omf_input);
        return 0;
    }
    rewind(omf_input);
    return 1;
}

/* First converter record pass [00047520,000479f0), including native dispatch data. */
void omf_convert_pass(int pass)
{
    unsigned index,type;
    omf_external_count=omf_symbol_count=omf_group_count=omf_segment_count=0;
    omf_name_count=omf_module_count=0;
    omf_extensions_seen=0;
    omf_record_remaining=omf_record_wide=omf_record_type=0;
    omf_data_record_type=omf_data_segment=0;
    omf_data_offset=0;
    omf_source_filename[0]=0;
    omf_pass=pass;
    omf_modend_seen=0;
    omf_initialize_symbols();
    omf_line_data=malloc(512);
    if (!omf_line_data)
        omf_error("Bad return from malloc: not enough memory to process LINNUM records");
    omf_line_count=0;
    if (omf_debug_mode==2) {
        omf_symbol_fixup_data=malloc(512);
        if (!omf_symbol_fixup_data)
            omf_error("Bad return from malloc: not enough memory to process $$SYMBOLS FIXUP records");
        omf_symbol_fixup_bytes=0;
    }
    for (index=0;index<4096;++index) omf_symbol_indices[index]=0;
    for (index=0;index<4096;++index) omf_external_indices[index]=0;
    omf_bss_section=omf_data_section=omf_text_section=0;
    omf_exestr_count=omf_defined_segment_count=0;
    omf_exestr_bytes=0;
    omf_type_segment_count=omf_symbol_segment_count=0;
    omf_first_type_segment=omf_first_symbol_segment=0;
    rewind(omf_input);
    for (;;) {
        omf_record_type=(unsigned short)getc(omf_input);
        if (omf_record_type==0xffff) {
            if (!omf_modend_seen) omf_warning("Missing MODEND record");
            omf_finish_symbols();
            if (omf_debug_mode==2) {
                omf_convert_debug_types();
                omf_convert_debug_symbols();
                omf_finish_debug_types();
            } else omf_convert_symbol_types();
            omf_release_line_data();
            omf_pass=2;
            omf_second_pass();
            return;
        }
        if (omf_modend_seen) omf_warning("OMF records past MODEND record");
        omf_record_wide=omf_record_type&1;
        omf_record_remaining=omf_read_word();
        type=omf_record_type&0xfe;
        switch (type) {
        case 0x80: omf_record_theadr(); break;
        case 0x82: omf_record_lheadr(); break;
        case 0x88: omf_record_comment(); break;
        case 0x8a: omf_record_modend(); break;
        case 0x8c: omf_record_externals(0,2); break;
        case 0x90: omf_record_publics(2); break;
        case 0x94: omf_record_lines(); break;
        case 0x96: omf_record_names(0); break;
        case 0x98: omf_record_segments(); break;
        case 0x9a: omf_record_groups(); break;
        case 0x9c:
            omf_save_fixup_record(ftell(omf_input)-3);
            omf_record_fixups();
            break;
        case 0xa0: omf_record_data(); break;
        case 0xa2: omf_record_iterated_data(); break;
        case 0xb0: omf_record_common(2); break;
        case 0xb2: omf_record_backpatch(); break;
        case 0xb4: omf_record_externals(0,3); break;
        case 0xb6: omf_record_local_publics(3); break;
        case 0xb8: omf_record_common(3); break;
        case 0xbc: omf_record_externals(1,2); break;
        case 0xc2: omf_record_comdat(); break;
        case 0xc4: omf_record_symbol_lines(); break;
        case 0xc8: omf_record_named_backpatch(); break;
        case 0xca: omf_record_names(-1); break;
        default: omf_error("Unknown or bad record type %x",type); break;
        }
        omf_skip_record();
        omf_record_checksum=omf_read_byte();
    }
}

/* Original [00047490,00047520). */
void omf_initialize_symbols(void)
{
    unsigned index;
    if (!omf_symbol_buckets) {
        omf_symbol_buckets=malloc(211*sizeof(*omf_symbol_buckets));
        if (!omf_symbol_buckets)
            omf_error("Bad return from malloc: not enough memory");
    }
    for (index=0;index<211;++index) omf_symbol_buckets[index]=NULL;
    if (!omf_relocation_buckets) {
        omf_relocation_buckets=malloc(211*sizeof(*omf_relocation_buckets));
        if (!omf_relocation_buckets)
            omf_error("Bad return from malloc: not enough memory");
    }
    for (index=0;index<211;++index) omf_relocation_buckets[index]=NULL;
}

/* Original [00046f80,00046fc0). */
unsigned short omf_read_word(void)
{
    unsigned char bytes[2];
    omf_read_block(bytes,2);
    return (unsigned short)(bytes[0]|((unsigned short)bytes[1]<<8));
}

/* Original [00047180,000471c0). */
void omf_read_block(void *buffer,unsigned count)
{
    if (fread(buffer,1,count,omf_input)!=count)
        omf_error("Bad read of object file");
    omf_record_remaining=(short)(omf_record_remaining-count);
}

/* Translator header and COFF FILE/auxiliary records [00046bf0,00046d50). */
void omf_record_theadr(void)
{
    char *name,*cursor,*destination,*left,*right;
    unsigned short length,auxiliary_count;
    unsigned index;
    IMAGE_AUX_SYMBOL auxiliary;
    ++omf_module_count;
    name=omf_read_name(0);
    if (!omf_first_module_name) {
        omf_first_module_name=name;
        cursor=name;
        destination=omf_source_filename;
        do { *destination++=*cursor; } while (*cursor++);
    }
    if ((short)omf_module_count>1) {
        if (omf_debug_mode==2) {
            left=name; right=omf_first_module_name;
            while (*left && *left==*right) { ++left; ++right; }
            if (*left!=*right)
                for (cursor=name;*cursor;++cursor) ;
        }
        return;
    }
    for (cursor=name;*cursor;++cursor) ;
    length=(unsigned short)(cursor-name);
    auxiliary_count=(unsigned short)(length/18+(length%18!=0));
    omf_append_symbol(".file",0,-2,IMAGE_SYM_CLASS_FILE,0,(unsigned char)auxiliary_count);
    while (*name) {
        for (index=0;index<18;++index) auxiliary.File.Name[index]=0;
        for (index=0;index<18 && *name;++index)
            auxiliary.File.Name[index]=*name++;
        omf_append_auxiliary(&auxiliary);
    }
}

/* Counted OMF name reader [00047120,00047180). */
char *omf_read_name(int option)
{
    unsigned char length;
    char *name;
    (void)option;
    omf_read_block(&length,1);
    name=malloc((unsigned)length+1);
    if (!name) omf_error("Bad return from malloc: out of space");
    omf_read_block(name,length);
    name[length]=0;
    return name;
}

#pragma intrinsic(strlen,memcpy)
/* Append one native18-byte COFF symbol [00048dc0,00048ff0). */
unsigned long omf_append_symbol(const char *name,unsigned long value,short section,
                               unsigned char storage,unsigned short type,
                               unsigned char auxiliary)
{
    IMAGE_SYMBOL symbol;
    unsigned long length,index;
    char *strings;
    PIMAGE_SYMBOL symbols;
    if (!omf_coff_symbol_count) {
        omf_coff_symbols=calloc(0x20000,1);
        if (!omf_coff_symbols) out_of_memory();
        omf_coff_symbol_capacity=0x20000;
    }
    length=strlen(name);
    if ((long)length<=8) strncpy((char *)symbol.N.ShortName,name,8);
    else {
        if (!omf_coff_string_size) {
            omf_coff_strings=calloc(0x20000,1);
            if (!omf_coff_strings) out_of_memory();
            omf_coff_string_size=4;
            omf_coff_string_capacity=0x20000;
        }
        ++length;
        symbol.N.Name.Short=0;
        symbol.N.Name.Long=omf_coff_string_size;
        if (omf_coff_string_size+length>omf_coff_string_capacity) {
            strings=calloc(omf_coff_string_capacity+length+0x20000,1);
            if (!strings) out_of_memory();
            memcpy(strings,omf_coff_strings,omf_coff_string_capacity);
            free(omf_coff_strings);
            omf_coff_strings=strings;
            omf_coff_string_capacity+=length+0x20000;
        }
        memcpy(omf_coff_strings+omf_coff_string_size,name,length);
        omf_coff_string_size+=length;
    }
    symbol.Value=value;
    symbol.SectionNumber=section;
    symbol.Type=type;
    symbol.StorageClass=storage;
    symbol.NumberOfAuxSymbols=auxiliary;
    if ((omf_coff_symbol_count+1)*18>omf_coff_symbol_capacity) {
        symbols=calloc(omf_coff_symbol_capacity+0x20000,1);
        if (!symbols) out_of_memory();
        memcpy(symbols,omf_coff_symbols,omf_coff_symbol_capacity);
        free(omf_coff_symbols);
        omf_coff_symbols=symbols;
        omf_coff_symbol_capacity+=0x20000;
    }
    index=omf_coff_symbol_count;
    omf_coff_symbols[index]=symbol;
    ++omf_coff_symbol_count;
    return index;
}
#pragma function(strlen,memcpy)

#pragma intrinsic(memcpy)
/* Original [00048ff0,000490b0). */
unsigned long omf_append_auxiliary(const IMAGE_AUX_SYMBOL *auxiliary)
{
    PIMAGE_SYMBOL symbols;
    unsigned long index;
    if ((omf_coff_symbol_count+1)*18>omf_coff_symbol_capacity) {
        symbols=calloc(omf_coff_symbol_capacity+0x20000,1);
        if (!symbols) out_of_memory();
        memcpy(symbols,omf_coff_symbols,omf_coff_symbol_capacity);
        free(omf_coff_symbols);
        omf_coff_symbols=symbols;
        omf_coff_symbol_capacity+=0x20000;
    }
    index=omf_coff_symbol_count;
    *(IMAGE_AUX_SYMBOL *)&omf_coff_symbols[index]=*auxiliary;
    ++omf_coff_symbol_count;
    return index;
}
#pragma function(memcpy)

/* Original [00046f20,00046f60). */
void omf_skip_record(void)
{
    if (omf_record_remaining>1) {
        if (fseek(omf_input,(long)omf_record_remaining-1,SEEK_CUR))
            omf_error("Bad seek on object file");
        omf_record_remaining=1;
    }
}

/* Original [00046f60,00046f80). */
unsigned char omf_read_byte(void)
{
    unsigned char value;
    omf_read_block(&value,1);
    return value;
}

/* LNAMES/LLNAMES records [00045f70,00045fe0). */
void omf_record_names(int local)
{
    while (omf_record_remaining>1) {
        ++omf_name_count;
        if ((short)omf_name_count>=2048)
            omf_error("Too many names in LNAMES record");
        omf_names[(short)omf_name_count]=omf_read_name(0);
        omf_local_names[(short)omf_name_count]=local;
    }
}

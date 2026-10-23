/* Embedded OMF-to-COFF converter and its LINK input adapter. */
#include "omf.h"
#include <io.h>
#include <time.h>
#include <recon.h>
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
unsigned short omf_type_segments[OMF_MAX_SEGMENTS];
unsigned short omf_symbol_segments[OMF_MAX_SEGMENTS];
unsigned char omf_record_checksum;
OmfLine *omf_line_data;
unsigned long omf_line_base;
unsigned char * omf_symbol_fixup_data;
unsigned long omf_symbol_indices[4096];
unsigned long omf_external_indices[4096];
FILE *omf_input;
OmfLineList omf_line_lists[OMF_MAX_SEGMENTS];
unsigned long omf_coff_line_count;
OmfRelocationList omf_relocation_lists[OMF_MAX_SEGMENTS];
unsigned short omf_file_relocation_seen;
unsigned long omf_coff_relocation_count;
unsigned long omf_thread_symbols[4];
unsigned char omf_thread_segments[4];
OmfFixupRecord *omf_fixup_head,*omf_fixup_tail;
const unsigned long omf_alignment_bytes[8]={0,1,2,16,4096,4,0,0};
int omf_last_comdat_segment;
unsigned short omf_coff_machine;
char *omf_exestr[256];
unsigned long omf_group_symbol_indices[OMF_MAX_SEGMENTS];
unsigned short omf_coff_section_count;
unsigned long omf_coff_raw_bytes;
IMAGE_SECTION_HEADER omf_coff_sections[OMF_MAX_SEGMENTS];
unsigned char * omf_coff_section_data[OMF_MAX_SEGMENTS];
unsigned long omf_coff_section_capacity[OMF_MAX_SEGMENTS];
OmfSegment omf_segments[OMF_MAX_SEGMENTS];
unsigned long omf_segment_symbol_indices[OMF_MAX_SEGMENTS];
unsigned long omf_section_aux_indices[OMF_MAX_SEGMENTS];
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
    omf_type_segments[0]=omf_symbol_segments[0]=0;
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
            omf_write_lines();
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

#pragma intrinsic(strcmp,strlen,memset)
/* Segment definitions [000461f0,000466a0). */
void omf_record_segments(void)
{
    unsigned char attributes,hidden=0;
    unsigned short name_index,class_index,section;
    unsigned long length,flags=0;
    int text=0,data=0,bss=0;
    char *name,*class_name,*class_end;
    OmfSegment *segment;
    IMAGE_AUX_SYMBOL auxiliary;
    ++omf_segment_count;
    if ((short)omf_segment_count>=OMF_MAX_SEGMENTS)
        omf_error("Too many SEGDEF/COMDAT records");
    attributes=omf_read_byte();
    if (!(attributes&0xe0)) {
        omf_read_word();
        omf_read_word();
    }
    length=omf_record_wide?omf_read_dword():omf_read_word();
    name_index=omf_read_index();
    name=omf_names[(short)name_index];
    ++omf_defined_segment_count;
    segment=&omf_segments[(short)omf_defined_segment_count];
    segment->name_index=name_index;
    class_index=omf_read_index();
    class_name=omf_names[(short)class_index];
    segment->class_index=class_index;
    if (!strcmp(name,"$$SYMBOLS") && !strcmp(class_name,"DEBSYM")) {
        if (!omf_debug_mode || omf_debug_mode==2) {
            omf_symbol_segments[(short)omf_symbol_segment_count++]=omf_defined_segment_count;
            hidden=1;
        } else if (omf_debug_mode==1) {
            name=".debug$S";
            flags=0x42000048;
        }
    } else if (!strcmp(name,"$$TYPES") && !strcmp(class_name,"DEBTYP")) {
        if (!omf_debug_mode || omf_debug_mode==2) {
            omf_type_segments[(short)omf_type_segment_count++]=omf_defined_segment_count;
            hidden=1;
        } else if (omf_debug_mode==1) {
            name=omf_extensions_seen?".debug$P":".debug$T";
            flags=0x42000048;
        }
    }
    segment->overlay_index=omf_read_index();
    if (attributes&2) omf_error("Bad segment definition");
    if (!(attributes&1)) omf_warning("16-bit segment");
    if (hidden) return;
    class_end=class_name+strlen(class_name);
    if (attributes&0xe0) {
        if (!strcmp(class_end-4,"CODE")) {
            flags=0x20;
            text=omf_defined_segment_count;
            if (!strcmp(name,"_TEXT")) name=omf_names[(short)name_index]=".text";
            if (!strcmp(name,"init")) flags=0x2000020;
        } else if (!strcmp(class_end-4,"DATA")) {
            flags=0x40;
            data=omf_defined_segment_count;
            if (!strcmp(name,"_DATA")) name=omf_names[(short)name_index]=".data";
        } else if (!strcmp(class_end-5,"CONST")) {
            flags=0x40000040;
            data=omf_defined_segment_count;
            if (!strcmp(name,"CONST")) name=omf_names[(short)name_index]=".rdata";
        } else if (!strcmp(class_end-3,"BSS")) {
            flags=0x80;
            bss=omf_defined_segment_count;
            if (!strcmp(name,"_BSS")) name=omf_names[(short)name_index]=".bss";
        }
    }
    if (!strcmp(name,".debug$F")) flags=0x42000048;
    else if (!flags) flags=0x40;
    if (strlen(name)>8) name[8]=0;
    segment->characteristics=flags;
    segment->alignment=omf_alignment_bytes[attributes>>5];
    section=omf_create_section(name,segment->alignment,0,length,flags);
    segment->coff_section=section;
    if (text) omf_text_section=section;
    if (data) omf_data_section=section;
    if (bss) omf_bss_section=section;
    omf_segment_symbol_indices[(short)omf_segment_count]=
        omf_append_symbol(name,0,(short)section,IMAGE_SYM_CLASS_STATIC,0,1);
    memset(&auxiliary,0,18);
    auxiliary.Section.Length=length;
    omf_section_aux_indices[(short)section-1]=omf_append_auxiliary(&auxiliary);
}
#pragma function(strcmp,strlen,memset)

/* Original [00046fc0,00047000). */
unsigned long omf_read_dword(void)
{
    unsigned char bytes[4];
    omf_read_block(bytes,4);
    return (unsigned long)bytes[0]|((unsigned long)bytes[1]<<8)|
           ((unsigned long)bytes[2]<<16)|((unsigned long)bytes[3]<<24);
}

/* Original [00047000,00047060). */
unsigned short omf_read_index(void)
{
    unsigned char first,second;
    omf_read_block(&first,1);
    if (!(first&0x80)) return first;
    omf_read_block(&second,1);
    return (unsigned short)(((unsigned short)(first&0x7f)<<8)|second);
}

/* Original [00048990,00048aa0). */
unsigned short omf_create_section(const char *name,unsigned long alignment,
                                 unsigned long address,unsigned long length,
                                 unsigned long flags)
{
    unsigned short index=omf_coff_section_count;
    unsigned long alignment_flags;
    PIMAGE_SECTION_HEADER header;
    ++omf_coff_section_count;
    if (omf_coff_section_count>OMF_MAX_SEGMENTS)
        omf_error("Too many COFF sections");
    omf_coff_section_data[index]=calloc(0x20000,1);
    if (!omf_coff_section_data[index]) out_of_memory();
    omf_coff_section_capacity[index]=0x20000;
    header=&omf_coff_sections[index];
    strncpy((char *)header->Name,name,8);
    header->Misc.VirtualSize=0;
    header->VirtualAddress=address;
    header->SizeOfRawData=length;
    header->NumberOfRelocations=0;
    header->NumberOfLinenumbers=0;
    switch (alignment) {
    case 1: alignment_flags=0x100000; break;
    case 2: alignment_flags=0x200000; break;
    case 4: alignment_flags=0x300000; break;
    case 8: alignment_flags=0x400000; break;
    case 16:
    default: alignment_flags=0x500000; break;
    }
    header->Characteristics=flags|alignment_flags;
    omf_coff_raw_bytes+=length;
    return omf_coff_section_count;
}

/* GRPDEF [00045bf0,00045c80). */
void omf_record_groups(void)
{
    char *name=omf_names[(short)omf_read_index()];
    unsigned short segment,section=0xffff;
    while (omf_record_remaining>1) {
        omf_read_byte();
        segment=omf_read_index();
        section=omf_segments[(short)segment].coff_section;
    }
    ++omf_group_count;
    if ((short)omf_group_count>=OMF_MAX_SEGMENTS)
        omf_error("Too many groups");
    omf_group_symbol_indices[(short)omf_group_count]=
        omf_append_symbol(name,0,(short)section,IMAGE_SYM_CLASS_STATIC,0,0);
}

/* PUBDEF records [00046130,000461f0). */
void omf_record_publics(unsigned short storage)
{
    unsigned short segment,type;
    short section;
    unsigned long value;
    char *name;
    (void)storage;
    omf_read_index();
    segment=omf_read_index();
    if (!segment) {
        section=-1;
        omf_read_word();
    } else section=(short)omf_segments[(short)segment].coff_section;
    while (omf_record_remaining>1) {
        name=omf_read_name(1);
        value=omf_record_wide?omf_read_dword():omf_read_word();
        type=omf_read_index();
        ++omf_symbol_count;
        if (omf_symbol_count>=4096) omf_error("Too many externals");
        omf_insert_symbol(name,value,3,section,omf_symbol_count,type);
    }
}

#pragma intrinsic(memset)
/* Original [000471f0,00047270). */
void omf_insert_symbol(char *name,unsigned long value,unsigned short kind,
                       short section,unsigned short ordinal,unsigned short type)
{
    OmfSymbol *symbol;
    unsigned bucket;
    symbol=malloc(sizeof(*symbol));
    if (!symbol) omf_error("Bad return from malloc; not enough memory");
    memset(symbol,0,sizeof(*symbol));
    symbol->name=name;
    symbol->value=value;
    symbol->kind=kind;
    symbol->section=section;
    symbol->type=type;
    symbol->ordinal=ordinal;
    bucket=omf_hash_name(name);
    symbol->next=omf_symbol_buckets[bucket];
    omf_symbol_buckets[bucket]=symbol;
}
#pragma function(memset)

/* Original [000471c0,000471f0). */
unsigned omf_hash_name(const char *name)
{
    unsigned hash=0;
    while (*name) hash=hash*3+(unsigned char)*name++;
    return hash%211;
}

#pragma intrinsic(strcmp)
/* EXTDEF/LEXTDEF/CEXTDEF [000456d0,00045820). */
void omf_record_externals(unsigned char named,unsigned short storage)
{
    unsigned short name_index,type,kind;
    short section;
    char *name;
    OmfSymbol *existing;
    while (omf_record_remaining>1) {
        if (named) {
            name_index=omf_read_index();
            name=omf_names[(short)name_index];
        } else name=omf_read_name(storage==3?0:1);
        type=omf_read_index();
        if (named && omf_local_names[(short)name_index]) continue;
        ++omf_symbol_count;
        if (omf_symbol_count>=4096) omf_error("Too many externals");
        ++omf_external_count;
        omf_external_indices[(short)omf_external_count]=omf_symbol_count;
        if (!strcmp(name,"_acrtused") || !strcmp(name,"__acrtused")) continue;
        kind=storage==3?2:1;
        existing=omf_lookup_symbol(name);
        section=existing?existing->section:0;
        omf_insert_symbol(name,0,kind,section,omf_symbol_count,type);
    }
}
#pragma function(strcmp)

#pragma intrinsic(strcmp)
/* Original [00047270,000472d0). */
OmfSymbol *omf_lookup_symbol(const char *name)
{
    OmfSymbol *symbol=omf_symbol_buckets[omf_hash_name(name)];
    while (symbol) {
        if (!strcmp(symbol->name,name)) break;
        symbol=symbol->next;
    }
    return symbol;
}
#pragma function(strcmp)

#pragma intrinsic(memset)
/* COMDAT/COMDAT32 [000466a0,00046a10). */
void omf_record_comdat(void)
{
    unsigned char data[4096],flags,attributes,alignment_code,checksum;
    unsigned short base,name_index,section;
    unsigned long alignment,symbol;
    int length;
    OmfSegment *segment,*source;
    IMAGE_AUX_SYMBOL auxiliary;
    char *name;
    flags=omf_read_byte();
    attributes=omf_read_byte();
    alignment_code=omf_read_byte();
    omf_data_record_type=0xa0;
    omf_data_offset=omf_record_wide?omf_read_dword():omf_read_word();
    omf_read_index();
    omf_read_index();
    base=omf_read_index();
    if (!base) omf_error("Section not defined in COMDAT");
    name_index=omf_read_index();
    name=omf_names[(short)name_index];
    length=omf_record_remaining-1;
    if (length>4096) omf_error("Bad data record; too large");
    memset(data,0,length);
    omf_read_block(data,length);
    if (!(flags&1)) {
        ++omf_segment_count;
        if (omf_segment_count>=256) omf_error("Too many SEGDEF/COMDAT records");
        ++omf_defined_segment_count;
        omf_data_segment=omf_defined_segment_count;
        segment=&omf_segments[(short)omf_data_segment];
        source=&omf_segments[(short)base];
        *segment=*source;
        alignment=alignment_code?omf_alignment_bytes[alignment_code]:segment->alignment;
        section=omf_create_section(omf_names[(short)source->name_index],alignment,
                                  0,length,source->characteristics|0x1000);
        omf_last_comdat_segment=(short)omf_data_segment;
        segment->coff_section=section;
        omf_write_section_data(section,omf_data_offset,data,length);
        if (flags&2) omf_error("COMDAT uses iterated data");
        symbol=omf_append_symbol(omf_names[(short)source->name_index],0,section,3,0,1);
        omf_segment_symbol_indices[(short)omf_segment_count]=symbol;
        memset(&auxiliary,0,sizeof(auxiliary));
        auxiliary.Section.Length=length;
        if (fread(&checksum,1,1,omf_input)!=1) omf_error("Bad read of object file");
        if (fseek(omf_input,-1,SEEK_CUR)) omf_error("Bad seek on object file");
        auxiliary.Section.CheckSum=checksum;
        switch (attributes) {
        case 0: auxiliary.Section.Selection=1; break;
        case 0x10: auxiliary.Section.Selection=2; break;
        case 0x20: auxiliary.Section.Selection=3; break;
        case 0x30: auxiliary.Section.Selection=4; break;
        default: auxiliary.Section.Selection=0; break;
        }
        omf_section_aux_indices[(short)section-1]=omf_append_auxiliary(&auxiliary);
        symbol=omf_append_symbol(name,0,section,(unsigned char)((flags&4)?3:2),0,0);
        if (omf_local_names[(short)name_index]==-1)
            omf_local_names[(short)name_index]=symbol;
    } else {
        omf_data_segment=(short)omf_last_comdat_segment;
        omf_extend_section(omf_segments[omf_last_comdat_segment].coff_section,
                           omf_data_offset,data,length);
    }
}
#pragma function(memset)

/* Original [00048ba0,00048c60). The native count is a byte count copied
 * from the first comment pointer, despite the caller passing comment count. */
#pragma intrinsic(memcpy)
void omf_write_comment_data(short section,unsigned long offset,
                             char *const *comments,short count)
{
    int index=(short)(section-1);
    unsigned long capacity=omf_coff_section_capacity[index];
    unsigned char *replacement;
    if (offset+(long)count>capacity) {
        replacement=calloc(capacity+(long)count+0x20000,1);
        if (!replacement) out_of_memory();
        memcpy(replacement,omf_coff_section_data[index],capacity);
        free(omf_coff_section_data[index]);
        omf_coff_section_data[index]=replacement;
        omf_coff_section_capacity[index]+=(long)count+0x20000;
    }
    memcpy(omf_coff_section_data[index]+offset,*comments,(long)count);
}
#pragma function(memcpy)

/* Original [00046df0,00046f20). Repeat a leaf or replay nested input blocks. */
unsigned long omf_expand_iterated_block(unsigned long offset)
{
    unsigned char data[2048];
    unsigned long repeat=omf_record_wide?omf_read_dword():omf_read_word();
    unsigned short blocks=omf_read_word(),index;
    unsigned char length;
    long position;
    short remaining;
    if (blocks) {
        position=ftell(omf_input);
        remaining=omf_record_remaining;
        while ((long)repeat-- > 0) {
            omf_record_remaining=remaining;
            for (index=0;index<blocks;++index)
                offset=omf_expand_iterated_block(offset);
            if (repeat && fseek(omf_input,position,SEEK_SET))
                omf_error("Cannot expand iterated data");
        }
    } else {
        length=omf_read_byte();
        omf_read_block(data,length);
        while ((long)repeat-- > 0) {
            omf_write_section_data(omf_segments[(short)omf_data_segment].coff_section,
                                   offset,data,length);
            offset+=length;
        }
    }
    return offset;
}

/* LIDATA/LIDATA32 [00045e00,00045e60). */
void omf_record_iterated_data(void)
{
    omf_data_record_type=0xa2;
    omf_data_segment=omf_read_index();
    omf_data_offset=omf_record_wide?omf_read_dword():omf_read_word();
    while (omf_record_remaining>1)
        omf_data_offset=omf_expand_iterated_block(omf_data_offset);
}

#pragma intrinsic(memcpy)
/* Original [00048ae0,00048ba0). */
void omf_write_section_data(short section,unsigned long offset,const void *data,
                            unsigned long length)
{
    int index=section-1;
    unsigned char *replacement;
    unsigned long capacity=omf_coff_section_capacity[index];
    if (offset+length>capacity) {
        replacement=calloc(capacity+length+0x20000,1);
        if (!replacement) out_of_memory();
        memcpy(replacement,omf_coff_section_data[index],capacity);
        free(omf_coff_section_data[index]);
        omf_coff_section_data[index]=replacement;
        omf_coff_section_capacity[index]+=length+0x20000;
    }
    memcpy(omf_coff_section_data[index]+offset,data,length);
}
#pragma function(memcpy)

/* COMENT [00045550,000456d0). */
void omf_record_comment(void)
{
    unsigned char record_class;
    unsigned short weak,fallback,bucket;
    int length;
    char *comment;
    OmfSymbol *symbol;
    omf_read_byte();
    record_class=omf_read_byte();
    switch (record_class) {
    case 0xa0:
        omf_extensions_seen=1;
        if (omf_read_byte()==6) omf_coff_machine=0x268;
        break;
    case 0xa4:
        if ((short)omf_exestr_count>=256) {
            omf_warning("More than %d comments: skipping exestr",256);
            break;
        }
        length=omf_record_remaining;
        comment=malloc(length);
        if (!comment) {
            omf_warning("Malloc failed, skipping comment record");
            break;
        }
        omf_read_block(comment,length-1);
        comment[length-1]=0;
        omf_exestr[(short)omf_exestr_count]=comment;
        omf_exestr_bytes+=length;
        ++omf_exestr_count;
        break;
    case 0xa8:
    case 0xa9:
        while (omf_record_remaining>1) {
            weak=(unsigned short)omf_external_indices[(short)omf_read_index()];
            fallback=(unsigned short)omf_external_indices[(short)omf_read_index()];
            for (bucket=0;bucket<211;++bucket) {
                symbol=omf_symbol_buckets[bucket];
                while (symbol && symbol->ordinal!=weak) symbol=symbol->next;
                if (symbol && !symbol->section) {
                    symbol->weak_default=fallback;
                    symbol->kind=record_class==0xa8?6:7;
                }
            }
        }
        break;
    }
}

#pragma intrinsic(memset)
/* LEDATA/LEDATA32 [00045c80,00045df0). */
void omf_record_data(void)
{
    unsigned char data[4096];
    int length,index;
    omf_data_record_type=0xa0;
    omf_data_segment=omf_read_index();
    omf_data_offset=omf_record_wide?omf_read_dword():omf_read_word();
    length=omf_record_remaining-1;
    if (length>4096) omf_error("Bad data record; too large");
    memset(data,0,length);
    omf_read_block(data,length);
    for (index=0;index<(short)omf_type_segment_count;++index) {
        if (omf_type_segments[index]!=omf_data_segment) continue;
        if (!omf_debug_mode) return;
        if (omf_debug_mode==2) {
            omf_write_debug_type_data(omf_data_offset,data,length);
            return;
        }
    }
    for (index=0;index<(short)omf_symbol_segment_count;++index) {
        if (omf_symbol_segments[index]!=omf_data_segment) continue;
        if (!omf_debug_mode) return;
        if (omf_debug_mode==2) {
            omf_write_debug_symbol_data(omf_data_offset,data,length);
            return;
        }
    }
    omf_write_section_data(omf_segments[(short)omf_data_segment].coff_section,
                           omf_data_offset,data,length);
}
#pragma function(memset)

/* Original [00047be0,00047c50). */
void omf_save_fixup_record(long position)
{
    OmfFixupRecord *record=malloc(sizeof(*record));
    if (!record) omf_error("save_fixup: Out of memory");
    record->position=position;
    record->data_offset=omf_data_offset;
    record->segment=omf_data_segment;
    record->data_record_type=omf_data_record_type;
    record->next=NULL;
    if (!omf_fixup_tail) omf_fixup_head=record;
    else omf_fixup_tail->next=record;
    omf_fixup_tail=record;
}

/* FIXUPP/FIXUPP32 [00045820,00045bf0), including native switch data. */
void omf_record_fixups(void)
{
    IMAGE_RELOCATION relocations[1024];
    unsigned long displacements[1024],symbol,resolved;
    unsigned char header,fields[2],segment_target;
    IMAGE_RELOCATION *relocation;
    int index,count=0;
    for (index=0;index<(short)omf_symbol_segment_count;++index) {
        if (omf_symbol_segments[index]!=omf_data_segment) continue;
        if (omf_pass==1 && omf_debug_mode==2) omf_save_debug_fixups();
        return;
    }
    while (omf_record_remaining>1) {
        omf_read_block(&header,1);
        if (header&0x80) {
            omf_read_block(fields,2);
            if (count>=1024) omf_error("Too many relocation entries");
            if (omf_data_record_type!=0xa0 && omf_data_record_type!=0xa2)
                omf_error("Bad relocatable reference");
            relocation=&relocations[count];
            relocation->VirtualAddress=((unsigned long)(header&3)<<8|fields[0])+omf_data_offset;
            if (!(fields[1]&0x80)) omf_read_fixup_target((fields[1]&0x70)>>4);
            if (!(fields[1]&8)) {
                symbol=omf_read_fixup_target(fields[1]&3);
                segment_target=(fields[1]&3)==0;
            } else {
                symbol=omf_thread_symbols[fields[1]&3];
                segment_target=omf_thread_segments[fields[1]&3];
            }
            relocation->SymbolTableIndex=symbol;
            switch ((header&0x3c)>>2) {
            case 0: relocation->Type=(header&0x40)?8:0x16; break;
            case 1:
            case 5: relocation->Type=(header&0x40)?1:2; break;
            case 2:
            case 3: omf_error("Segment reference in fixup record");
            default: omf_error("Bad relocation type"); break;
            case 4:
                if (!(header&0x40)) omf_error("Bad relocation type");
                relocation->Type=10;
                break;
            case 9:
            case 13:
            case 11: relocation->Type=(header&0x40)?6:20; break;
            case 14: relocation->Type=7; break;
            }
            displacements[count]=0;
            if (!(fields[1]&4)) {
                displacements[count]=omf_record_wide?omf_read_dword():omf_read_word();
                if (segment_target && displacements[count]) {
                    if (omf_pass==1)
                        omf_insert_relocation_symbol(symbol,displacements[count]);
                    else {
                        resolved=omf_lookup_relocation_symbol(symbol,displacements[count]);
                        if (resolved!=symbol) {
                            relocation->SymbolTableIndex=resolved;
                            displacements[count]=0;
                        }
                    }
                }
            }
            ++count;
        } else if (!(header&0x40)) {
            index=header&3;
            omf_thread_symbols[index]=omf_read_fixup_target((header&0x1c)>>2);
            omf_thread_segments[index]=(header&0x1c)==0;
        } else omf_read_fixup_target((header&0x1c)>>2);
    }
    if (omf_pass!=1) {
        while (count>0) {
            --count;
            relocation=&relocations[count];
            omf_emit_relocation(omf_segments[(short)omf_data_segment].coff_section,
                relocation->VirtualAddress,relocation->SymbolTableIndex,
                relocation->Type,displacements[count]);
        }
    }
}

/* Original [00046d50,00046df0). */
unsigned long omf_read_fixup_target(unsigned method)
{
    short index;
    unsigned long symbol;
    switch (method) {
    case 0:
        index=(short)omf_read_index();
        return omf_segment_symbol_indices[index];
    case 1:
        index=(short)omf_read_index();
        return omf_group_symbol_indices[index];
    case 2:
        index=(short)omf_read_index();
        symbol=omf_local_names[index];
        if (!symbol) symbol=omf_symbol_indices[omf_external_indices[index]];
        return symbol;
    case 4:
    case 5: return (unsigned long)-1;
    default: omf_error("Bad method in FIXUP record");
    }
    return 0;
}

/* MODEND/MODEND32 [000460b0,00046130). */
void omf_record_modend(void)
{
    unsigned short section;
    short index;
    omf_modend_seen=1;
    if (omf_exestr_count) {
        section=omf_create_section(".comment",1,0,omf_exestr_bytes,0x840);
        omf_write_comment_data(section,0,omf_exestr,omf_exestr_count);
        for (index=0;index<(short)omf_exestr_count;++index) {
            free(omf_exestr[index]);
            omf_exestr[index]=NULL;
        }
    }
}

/* LINSYM16/32 [00045f20,00045f67). The native converter consumes these
 * COMDAT-relative line pairs but does not add them to its LINNUM output lists. */
void omf_record_symbol_lines(void)
{
    omf_read_byte();
    omf_read_index();
    while(omf_record_remaining>1) {
        omf_read_word();
        if(omf_record_wide) omf_read_dword();
        else omf_read_word();
    }
}

/* Original [00048150,000481a6). Converter warnings use stdout, including
 * the historical buffered putc/_flsbuf path for the terminating newline. */
void omf_warning(const char *format,...)
{
    va_list arguments;
    fprintf(stdout,"%s() : warning : ",omf_program_name);
    va_start(arguments,format);
    vfprintf(stdout,format,arguments);
    va_end(arguments);
    putc('\n',stdout);
}

/* Original [00048aa0,00048ad1). Continuation extends the declared raw span
 * by its payload length; the explicit offset is passed unchanged to the writer. */
void omf_extend_section(short section,unsigned long offset,
                        const void *data,unsigned long length)
{
    omf_coff_sections[section-1].SizeOfRawData+=length;
    omf_coff_raw_bytes+=length;
    omf_write_section_data(section,offset,data,length);
}

/* Original [00047350,00047386), including two internal alignment bytes.
 * Pass1 inserts every displaced target before this pass2 lookup; the original
 * traversal intentionally has no missing-key/null-node fallback. */
unsigned long omf_lookup_relocation_symbol(unsigned long symbol,unsigned long offset)
{
    OmfRelocationSymbol *entry=omf_relocation_buckets[(symbol+offset)%211];
    while(entry->symbol!=symbol || entry->offset!=offset) entry=entry->next;
    return entry->resolved;
}

/* Original [000472d0,00047341). The resolved slot is assigned later by
 * omf_finish_symbols; inserting or comparing an entry never reads it. */
void omf_insert_relocation_symbol(unsigned long symbol,unsigned long offset)
{
    unsigned bucket=(symbol+offset)%211;
    OmfRelocationSymbol *entry;
    for(entry=omf_relocation_buckets[bucket];entry;entry=entry->next)
        if(entry->symbol==symbol && entry->offset==offset) return;
    entry=malloc(sizeof(*entry));
    if(!entry) omf_error("Bad return from malloc; not enough memory");
    entry->symbol=symbol;
    entry->offset=offset;
    entry->next=omf_relocation_buckets[bucket];
    omf_relocation_buckets[bucket]=entry;
}

/* Resolve section-relative displacement symbols [00047390,00047430). */
void omf_finish_symbols(void)
{
    unsigned bucket,serial=0;
    OmfRelocationSymbol *symbol;
    PIMAGE_SYMBOL target;
    short section;
    char name[8];
    for (bucket=0;bucket<211;++bucket) {
        for (symbol=omf_relocation_buckets[bucket];symbol;symbol=symbol->next) {
            target=&omf_coff_symbols[symbol->symbol];
            section=target->SectionNumber;
            if (section>0) {
                sprintf(name,"$$R%04X",++serial);
                symbol->resolved=omf_append_symbol(name,target->Value+symbol->offset,
                                                   section,3,0,0);
            } else symbol->resolved=symbol->symbol;
        }
    }
}

#pragma intrinsic(memset)
/* Emit ordinary and weak/lazy symbols [00047d40,00048010). */
void omf_convert_symbol_types(void)
{
    unsigned short bucket,type=0,auxiliary_count=0,index;
    unsigned char storage;
    OmfSymbol *symbol,*next;
    IMAGE_AUX_SYMBOL auxiliary;
    omf_line_base=0;
    for (bucket=0;bucket<211;++bucket) {
        for (symbol=omf_symbol_buckets[bucket];symbol;symbol=symbol->next) {
            if (omf_symbol_indices[symbol->ordinal] || symbol->kind==6 || symbol->kind==7)
                continue;
            if (omf_debug_mode==2)
                auxiliary_count=omf_debug_type_auxiliary(symbol->type,&type,&auxiliary);
            if (symbol->kind==1) {
                auxiliary_count=0;
                type=0;
                storage=2;
            } else storage=(symbol->kind==2 || symbol->kind==4)?3:2;
            if (omf_debug_mode==1 && omf_line_count && !omf_line_base &&
                (unsigned short)symbol->section==omf_line_section) auxiliary_count=1;
            omf_symbol_indices[symbol->ordinal]=omf_append_symbol(symbol->name,
                symbol->value,symbol->section,storage,type,(unsigned char)auxiliary_count);
            if (auxiliary_count) {
                if (omf_debug_mode==2) omf_append_auxiliary(&auxiliary);
                else if (omf_debug_mode==1) {
                    omf_line_base=omf_line_symbol_index;
                    omf_append_function_auxiliary(omf_line_count,omf_line_section,
                        omf_line_data[0].offset,omf_line_data[0].line,
                        omf_line_data[omf_line_count-1].offset-omf_line_data[0].offset,
                        omf_line_data[omf_line_count-1].line);
                    for (index=1;index<omf_line_count;++index) {
                        auxiliary_count=0;
                        omf_line_data[index].line-=omf_line_data[0].line;
                        omf_line_data[0].line=0;
                        omf_line_data[0].offset=omf_line_base-1;
                    }
                }
            }
        }
    }
    memset(&auxiliary,0,sizeof(auxiliary));
    for (bucket=0;bucket<211;++bucket) {
        symbol=omf_symbol_buckets[bucket];
        while (symbol) {
            if (!omf_symbol_indices[symbol->ordinal]) {
                omf_symbol_indices[symbol->ordinal]=omf_append_symbol(symbol->name,
                    symbol->value,symbol->section,105,0,1);
                auxiliary.Sym.TagIndex=omf_symbol_indices[symbol->weak_default];
                auxiliary.Sym.Misc.TotalSize=symbol->kind==6?1:2;
                omf_append_auxiliary(&auxiliary);
            }
            next=symbol->next;
            free(symbol);
            symbol=next;
        }
    }
}
#pragma function(memset)

/* Serialize collected LINNUM records [000492c0,00049300). */
void omf_write_lines(void)
{
    OmfLine *line=omf_line_data,*end=line+omf_line_count;
    while (line<end) {
        omf_append_line(omf_line_section,line->offset,line->line);
        ++line;
    }
}

/* Revisit saved FIXUPP records [00047c50,00047d40). */
void omf_second_pass(void)
{
    OmfFixupRecord *record=omf_fixup_head,*next;
    unsigned short type;
    while (record) {
        fseek(omf_input,record->position,SEEK_SET);
        omf_record_type=(unsigned short)getc(omf_input);
        type=omf_record_type&0xfe;
        if (type!=0x9c && type!=0x9d) omf_error("proc_fixups: not a fixup record");
        omf_record_wide=omf_record_type&1;
        omf_record_remaining=omf_read_word();
        omf_data_segment=record->segment;
        omf_data_record_type=record->data_record_type;
        omf_data_offset=record->data_offset;
        omf_record_fixups();
        next=record->next;
        free(record);
        record=next;
    }
    omf_fixup_tail=omf_fixup_head=NULL;
}

/* Append a COFF relocation and apply its addend [00048c60,00048d80). */
void omf_emit_relocation(short section,unsigned long address,unsigned long symbol,
                          unsigned short type,unsigned long displacement)
{
    unsigned index;
    unsigned char *data,*name=omf_coff_symbols[symbol].N.ShortName;
    for (index=0;index<6 && name[index]==".file"[index];++index) ;
    if (index==6) {
        omf_file_relocation_seen=1;
        return;
    }
    ++omf_coff_relocation_count;
    ++omf_coff_sections[section-1].NumberOfRelocations;
    omf_queue_relocation(&omf_relocation_lists[section-1],address,symbol,type);
    data=omf_coff_section_data[section-1]+address;
    switch (type) {
    case 1:
    case 2: *(unsigned short *)data+=(unsigned short)displacement; break;
    case 6:
    case 20: *(unsigned long *)data+=displacement; break;
    case 7: break;
    case 8:
    case 10:
    case 22: *data+=(unsigned char)displacement; break;
    default: omf_error("Bad COFF relocation type"); break;
    }
}

/* Original [000483d0,00048430). */
void omf_queue_relocation(OmfRelocationList *list,unsigned long address,
                          unsigned long symbol,unsigned short type)
{
    OmfRelocation *relocation=malloc(sizeof(*relocation));
    if (!relocation) out_of_memory();
    relocation->address=address;
    relocation->symbol=symbol;
    relocation->type=type;
    relocation->next=NULL;
    if (!list->head) list->head=relocation;
    else list->tail->next=relocation;
    ++list->count;
    list->tail=relocation;
}

/* Serialize the converted COFF object [00048480,00048990). */
void omf_write_coff(const char *path)
{
    FILE *output;
    IMAGE_FILE_HEADER file_header;
    PIMAGE_SECTION_HEADER section;
    PIMAGE_AUX_SYMBOL auxiliary;
    OmfRelocation *relocation,*next_relocation;
    OmfCoffLine *line,*next_line;
    unsigned long raw_position,relocation_position,line_position,symbol;
    unsigned short index,count,saved_count;
    omf_file_relocation_seen=0;
    output=fopen(path,"wb");
    if (!output) omf_error("Cannot open \"%s\" for writing",path);
    file_header.Machine=omf_coff_machine?omf_coff_machine:IMAGE_FILE_MACHINE_I386;
    file_header.NumberOfSections=omf_coff_section_count;
    file_header.TimeDateStamp=time(NULL);
    file_header.PointerToSymbolTable=20+omf_coff_section_count*40+
        omf_coff_raw_bytes+omf_coff_relocation_count*10+omf_coff_line_count*6;
    file_header.NumberOfSymbols=omf_coff_symbol_count;
    file_header.SizeOfOptionalHeader=0;
    file_header.Characteristics=0;
    fwrite(&file_header,20,1,output);
    raw_position=20+omf_coff_section_count*40;
    relocation_position=raw_position+omf_coff_raw_bytes;
    line_position=relocation_position+omf_coff_relocation_count*10;
    for (index=0;index<omf_coff_section_count;++index) {
        section=&omf_coff_sections[index];
        section->PointerToRawData=(!(section->Characteristics&0x80) &&
                                   section->SizeOfRawData)?raw_position:0;
        section->PointerToRelocations=section->NumberOfRelocations?relocation_position:0;
        section->PointerToLinenumbers=section->NumberOfLinenumbers?line_position:0;
        if (omf_line_base) {
            auxiliary=(PIMAGE_AUX_SYMBOL)&omf_coff_symbols[omf_line_base];
            auxiliary->Sym.FcnAry.Function.PointerToLinenumber=line_position;
            omf_line_base=0;
        }
        fwrite(section,40,1,output);
        raw_position+=section->SizeOfRawData;
        relocation_position+=section->NumberOfRelocations*10;
        line_position+=section->NumberOfLinenumbers*6;
    }
    for (index=0;index<omf_coff_section_count;++index) {
        section=&omf_coff_sections[index];
        if (section->SizeOfRawData)
            fwrite(omf_coff_section_data[index],1,section->SizeOfRawData,output);
        free(omf_coff_section_data[index]);
        omf_coff_section_data[index]=NULL;
    }
    for (index=0;index<omf_coff_section_count;++index) {
        section=&omf_coff_sections[index];
        if (section->Characteristics==0x200 || !section->NumberOfRelocations) continue;
        relocation=omf_relocation_lists[index].head;
        for (count=0;count<section->NumberOfRelocations;++count) {
            fwrite(relocation,10,1,output);
            next_relocation=relocation->next;
            free(relocation);
            relocation=next_relocation;
        }
        omf_relocation_lists[index].head=NULL;
    }
    for (index=0;index<omf_coff_section_count;++index) {
        section=&omf_coff_sections[index];
        if (!(section->Characteristics&0x20)) continue;
        saved_count=0;
        count=section->NumberOfLinenumbers;
        if (count) {
            line=omf_line_lists[index].head;
            saved_count=count;
            while (count) {
                fwrite(line,6,1,output);
                next_line=line->next;
                free(line);
                line=next_line;
                --count;
            }
            omf_line_lists[index].head=NULL;
        }
        section->NumberOfLinenumbers=saved_count;
    }
    for (index=0;index<omf_coff_section_count;++index) {
        if (omf_section_aux_indices[index] && omf_coff_symbols) {
            auxiliary=(PIMAGE_AUX_SYMBOL)&omf_coff_symbols[omf_section_aux_indices[index]];
            auxiliary->Section.NumberOfRelocations=omf_coff_sections[index].NumberOfRelocations;
            auxiliary->Section.NumberOfLinenumbers=omf_coff_sections[index].NumberOfLinenumbers;
        }
        omf_section_aux_indices[index]=0;
    }
    fseek(output,file_header.PointerToSymbolTable,SEEK_SET);
    if (omf_coff_symbol_count) {
        for (symbol=0;symbol<omf_coff_symbol_count;++symbol)
            fwrite(&omf_coff_symbols[symbol],18,1,output);
        fwrite(&omf_coff_string_size,4,1,output);
        if (omf_coff_string_size) {
            omf_coff_string_size-=4;
            fwrite(omf_coff_strings+4,1,omf_coff_string_size,output);
            free(omf_coff_strings);
        }
    }
    free(omf_coff_symbols);
    omf_coff_symbols=NULL;
    fclose(output);
    omf_coff_symbol_count=omf_coff_line_count=omf_coff_relocation_count=0;
    omf_coff_string_size=omf_coff_raw_bytes=0;
    omf_coff_section_count=0;
}

/* Original [00045460,00045470). */
void omf_close_input(void)
{
    fclose(omf_input);
}

/* COMDEF/LCOMDEF common allocation records [00045470,00045550). */
void omf_record_common(unsigned short storage)
{
    char *name;
    unsigned short type,index;
    unsigned char data_type;
    unsigned long length;
    while (omf_record_remaining>1) {
        name=omf_read_name(1);
        type=omf_read_index();
        data_type=omf_read_byte();
        length=omf_read_numeric_leaf();
        if (data_type==0x61) length*=omf_read_numeric_leaf();
        ++omf_symbol_count;
        if (omf_symbol_count>=4096) omf_error("Too many externals");
        index=omf_symbol_count;
        ++omf_external_count;
        omf_external_indices[(short)omf_external_count]=index;
        if (omf_debug_mode==2)
            omf_insert_symbol(name,length,5,0,index,type);
        else omf_symbol_indices[index]=omf_append_symbol(name,length,0,
                                                       (unsigned char)storage,0,0);
    }
}

/* Communal numeric leaf [00047060,00047120). */
unsigned long omf_read_numeric_leaf(void)
{
    unsigned char bytes[4];
    unsigned long value;
    omf_read_block(bytes,1);
    value=bytes[0];
    switch (value) {
    case 0x81:
        omf_read_block(bytes,2);
        return (unsigned long)bytes[0]|((unsigned long)bytes[1]<<8);
    case 0x84:
        omf_read_block(bytes,3);
        return (unsigned long)bytes[0]|((unsigned long)bytes[1]<<8)|
               ((unsigned long)bytes[2]<<16);
    case 0x88:
        omf_read_block(bytes,4);
        return (unsigned long)bytes[0]|((unsigned long)bytes[1]<<8)|
               ((unsigned long)bytes[2]<<16)|((unsigned long)bytes[3]<<24);
    }
    return value;
}

/* LPUBDEF records [00046020,000460b0). */
void omf_record_local_publics(unsigned short storage)
{
    unsigned short segment,type;
    short section;
    unsigned long value;
    char *name;
    (void)storage;
    omf_read_index();
    segment=omf_read_index();
    if (!segment) {
        section=-1;
        omf_read_word();
    } else section=(short)omf_segments[(short)segment].coff_section;
    while (omf_record_remaining>1) {
        name=omf_read_name(0);
        value=omf_record_wide?omf_read_dword():omf_read_word();
        type=omf_read_index();
        omf_update_symbol(name,value,4,section,type);
    }
}

/* Update a local definition without advancing its ordinal [00047430,00047490). */
void omf_update_symbol(char *name,unsigned long value,unsigned short kind,
                       short section,unsigned short type)
{
    OmfSymbol *symbol=omf_lookup_symbol(name);
    if (symbol) {
        symbol->value=value;
        symbol->kind=kind;
        symbol->section=section;
        symbol->type=type;
    } else omf_insert_symbol(name,value,kind,section,omf_symbol_count,type);
}

/* BAKPAT records [00046ae0,00046bb0). */
void omf_record_backpatch(void)
{
    unsigned short segment=omf_read_index();
    unsigned char type=omf_read_byte();
    unsigned long offset,addend;
    unsigned char *location;
    while (omf_record_remaining>1) {
        offset=omf_record_wide?omf_read_dword():omf_read_word();
        addend=omf_record_wide?omf_read_dword():omf_read_word();
        location=omf_coff_section_data[(short)omf_segments[(short)segment].coff_section-1]+offset;
        switch (type) {
        case 0: *location=(unsigned char)(*location*2+addend); break;
        case 1: *(unsigned short *)location=(unsigned short)(*(unsigned short *)location*2+addend); break;
        case 2: *(unsigned long *)location=*(unsigned long *)location*2+addend; break;
        default: omf_error("Ilegal LocTyp in BAKPAT record"); break;
        }
    }
}

/* NBKPAT applies to the current COMDAT [00046a10,00046ae0). */
void omf_record_named_backpatch(void)
{
    unsigned char type=omf_read_byte();
    unsigned long offset,addend;
    unsigned char *location;
    omf_read_index();
    while (omf_record_remaining>1) {
        offset=omf_record_wide?omf_read_dword():omf_read_word();
        addend=omf_record_wide?omf_read_dword():omf_read_word();
        location=omf_coff_section_data[(short)omf_segments[(short)omf_data_segment].coff_section-1]+offset;
        switch (type) {
        case 0: *location=(unsigned char)(*location*2+addend); break;
        case 1: *(unsigned short *)location=(unsigned short)(*(unsigned short *)location*2+addend); break;
        case 2: *(unsigned long *)location=*(unsigned long *)location*2+addend; break;
        default: omf_error("Ilegal LocTyp in NBKPAT record"); break;
        }
    }
}

/* Collect LINNUM offsets/lines [00045e60,00045f20). */
void omf_record_lines(void)
{
    unsigned short line;
    unsigned long offset;
    omf_read_index();
    omf_read_index();
    while (omf_record_remaining>1) {
        line=omf_read_word();
        offset=omf_record_wide?omf_read_dword():omf_read_word();
        omf_line_data[omf_line_count].offset=offset;
        omf_line_data[omf_line_count].line=line;
        ++omf_line_count;
        if (!(omf_line_count&63)) {
            omf_line_data=realloc(omf_line_data,omf_line_count*8+512);
            if (!omf_line_data) omf_error("linnum: Out of space");
        }
    }
}

#pragma intrinsic(memset,memcpy)
/* COFF function/begin/line/end auxiliaries [000490b0,000492c0). */
void omf_append_function_auxiliary(unsigned long count,unsigned short section,
    unsigned long start,unsigned short first_line,unsigned long length,
    unsigned short last_line)
{
    IMAGE_AUX_SYMBOL auxiliary;
    IMAGE_SYMBOL symbol;
    (void)start;
    omf_coff_symbols[omf_line_symbol_index-1].Type=0x20;
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.TagIndex=omf_line_symbol_index+1;
    auxiliary.Sym.Misc.LnSz.Linenumber=first_line;
    auxiliary.Sym.Misc.TotalSize=length+1;
    omf_append_auxiliary(&auxiliary);

    memset(&symbol,0,sizeof(symbol));
    memcpy(symbol.N.ShortName,".bf",4);
    symbol.SectionNumber=section;
    symbol.StorageClass=101;
    symbol.NumberOfAuxSymbols=1;
    omf_append_auxiliary((const IMAGE_AUX_SYMBOL *)&symbol);
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.Misc.LnSz.Linenumber=first_line;
    omf_append_auxiliary(&auxiliary);

    memset(&symbol,0,sizeof(symbol));
    memcpy(symbol.N.ShortName,".lf",4);
    symbol.Value=count;
    symbol.SectionNumber=section;
    symbol.StorageClass=101;
    omf_append_auxiliary((const IMAGE_AUX_SYMBOL *)&symbol);

    memset(&symbol,0,sizeof(symbol));
    memcpy(symbol.N.ShortName,".ef",4);
    symbol.SectionNumber=section;
    symbol.StorageClass=101;
    symbol.NumberOfAuxSymbols=1;
    omf_append_auxiliary((const IMAGE_AUX_SYMBOL *)&symbol);
    memset(&auxiliary,0,sizeof(auxiliary));
    auxiliary.Sym.Misc.LnSz.Linenumber=last_line;
    omf_append_auxiliary(&auxiliary);
}
#pragma function(memset,memcpy)

/* Collect a COFF line entry [00048d80,00048dc0). */
void omf_append_line(short section,unsigned long offset,unsigned short line)
{
    ++omf_coff_line_count;
    --section;
    ++omf_coff_sections[section].NumberOfLinenumbers;
    omf_queue_line(&omf_line_lists[section],offset,line);
}

/* Append a line node to its original12-byte list [00048430,00048480). */
void omf_queue_line(OmfLineList *list,unsigned long offset,unsigned short line)
{
    OmfCoffLine *entry=malloc(sizeof(*entry));
    if (!entry) out_of_memory();
    entry->offset=offset;
    entry->line=line;
    entry->next=NULL;
    if (!list->head) list->head=entry;
    else list->tail->next=entry;
    ++list->count;
    list->tail=entry;
}

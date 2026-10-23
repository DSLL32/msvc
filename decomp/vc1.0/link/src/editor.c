/* Image editing, with mode helpers retaining their native graph vertices. */
#include "editor.h"
#include <recon.h>
Image *edit_image;
PIMAGE_SECTION_HEADER edit_section_headers;
unsigned char edit_optional_changed;
extern FILE *map_stream;
extern unsigned long pe_header_offset,section_header_offset;
unsigned char valid_coff_machine(IMAGE_FILE_HEADER *);

/* Original [000278a0,00027ff0). */
int editor_main(int argc,char **argv)
{
    Argument *argument,*input;
    ParsedSwitch *option;
    unsigned short index,value,used,file,section,magic;
    unsigned long signature;
    unsigned char base_given;
    char **vector,*key,*text;
    IMAGE_DOS_HEADER dos;
    if (argc<2) {
        setvbuf(stdout,NULL,0,1);
        if (banner_pending) print_banner();
        printf("usage: EDITBIN [options] [files]\n\n",program_name);
        printf("   options:\n\n");
        printf("      /BIND[:PATH=path]\n");
        printf("      /REBASE[:[BASE=address][,BASEFILE][,DOWN]]\n");
        printf("      /RELEASE\n");
        printf("      /SECTION:name[=newname][,[[!]{dekprsw}][a{1248pts}]]\n\n");
        printf("      /STACK:reserve[,commit]\n");
        return 3;
    }
    create_image(&edit_image);
    collect_arguments(argc,argv,NULL);
    if (banner_pending) print_banner();
    close_all_files();
    remove_temporary_inputs();
    input=other_inputs.first;
    for (index=0;index<other_inputs.count;++index,input=input->next)
        warning_message(program_name,0x76,input->original,"");
    if (library_files.count) convert_omf_inputs(1,&edit_image->options);
    argument=switches.first;
    for (index=0;index<switches.count;++index,argument=argument->next) {
        option=argument->parsed=parse_switch(argument->text);
        if (!_strcmpi(option->name,"rebase")) {
            vector=malloc((unsigned short)(option->count+library_files.count+8)*4);
            if (!vector) out_of_memory();
            vector[0]="rebase";
            base_given=0;
            used=1;
            for (value=0;value<option->count;++value) {
                key=option->values[value].key;
                text=option->values[value].value;
                if (key && !_strcmpi(key,"base")) {
                    base_given=1;
                    vector[used++]="-b";
                    vector[used++]=text;
                } else if (!_strcmpi(text,"basefile")) {
                    vector[used++]="-c";
                    vector[used++]="coffbase.txt";
                } else if (!_strcmpi(text,"down")) vector[used++]="-d";
                else fatal_message(program_name,0x14,argument->text);
            }
            if (!base_given) { vector[used++]="-b"; vector[used++]="0x10000"; }
            input=library_files.first;
            for (file=0;file<library_files.count;++file,input=input->next) {
                prepare_edit_input(input);
                vector[used++]=input->text;
            }
            vector[used++]=NULL;
            rebase_main((int)used-1,vector,NULL);
            free(vector);
        } else if (!_strcmpi(option->name,"bind")) {
            vector=malloc((unsigned short)(option->count+library_files.count+5)*4);
            if (!vector) out_of_memory();
            vector[0]="bind";
            vector[1]="-U";
            used=2;
            for (value=0;value<option->count;++value) {
                key=option->values[value].key;
                text=option->values[value].value;
                if (key && !_strcmpi(key,"path")) {
                    vector[used++]="-p";
                    vector[used++]=text;
                } else fatal_message(program_name,0x14,argument->text);
            }
            input=library_files.first;
            for (file=0;file<library_files.count;++file,input=input->next) {
                prepare_edit_input(input);
                vector[used++]=input->text;
            }
            vector[used++]=NULL;
            bind_main((int)used-1,vector,NULL);
            free(vector);
        }
    }
    input=library_files.first;
    for (file=0;file<library_files.count;++file,input=input->next) {
        prepare_edit_input(input);
        output_file=open_file(input->text,0x8002,0);
        read_file(output_file,&magic,2);
        seek_file(output_file,-2,SEEK_CUR);
        pe_header_offset=0;
        if (magic==IMAGE_DOS_SIGNATURE) {
            read_file(output_file,&dos,64);
            seek_file(output_file,dos.e_lfanew,SEEK_SET);
            read_file(output_file,&signature,4);
            if (edit_image->options.dump.headers && signature!=IMAGE_NT_SIGNATURE) {
                fprintf(map_stream,"\nPE signature not found\n");
                goto close_input;
            }
        }
        pe_header_offset=tell_file(output_file);
        read_coff_header(output_file,&edit_image->file);
        if (!valid_coff_machine(&edit_image->file)) {
            warning_message(program_name,0x77,input->original,"");
            continue;
        }
        if (edit_image->file.SizeOfOptionalHeader)
            read_optional_header(output_file,&edit_image->optional,edit_image->file.SizeOfOptionalHeader);
        section_header_offset=edit_image->file.SizeOfOptionalHeader+pe_header_offset+20;
        seek_file(output_file,section_header_offset+archive_data_offset,SEEK_SET);
        printf("\n");
        edit_section_headers=malloc((edit_image->file.NumberOfSections+1)*40);
        if (!edit_section_headers) out_of_memory();
        for (section=1;section<=edit_image->file.NumberOfSections;++section)
            read_section_header(output_file,&edit_section_headers[section]);
        apply_edit_options();
        if (edit_optional_changed && edit_image->file.SizeOfOptionalHeader) {
            seek_file(output_file,pe_header_offset+20,SEEK_SET);
            write_optional_header(output_file,&edit_image->optional,edit_image->file.SizeOfOptionalHeader);
        }
        free(edit_section_headers);
        if (edit_image->file.SizeOfOptionalHeader && edit_image->optional.CheckSum)
            update_checksum(output_file);
close_input:
        close_file(output_file,1);
    }
    return 0;
}

#pragma intrinsic(strlen)
/* Original [00028660,00028750). Ensure writable effective input, copying when converted. */
void prepare_edit_input(Argument *argument)
{
    const unsigned char *left,*right;
    char *command;
    close_file(open_file(argument->text,0x8002,0x180),1);
    if (!argument->original) return;
    left=(const unsigned char *)argument->original;
    right=(const unsigned char *)argument->text;
    while (*left && *left==*right) { ++left; ++right; }
    if (*left==*right) return;
    command=allocate_checked(strlen(argument->original)+strlen(argument->text)+20);
    close_file(open_file(argument->original,0x8002,0x180),1);
    sprintf(command,"copy %s %s >nul",argument->original,argument->text);
    if (system(command)) fatal_message(argument->text,0x33,argument->original);
    free(command);
}

void parse_reserve_commit(const char *,unsigned long *,unsigned long *);
void edit_section(char *);

/* Original [00027ff0,00028180). Apply the per-file options to its loaded headers. */
void apply_edit_options(void)
{
    Argument *argument=switches.first;
    ParsedSwitch *option;
    unsigned short index;
    if (!switches.count) warning_message(program_name,0x70,"","");
    for (index=0;index<switches.count;++index,argument=argument->next) {
        option=argument->parsed=parse_switch(argument->text);
        if (!_strcmpi(option->name,"rebase") || !_strcmpi(option->name,"bind")) continue;
        if (!_strcmpi(argument->text,"release")) {
            if (edit_image->file.SizeOfOptionalHeader) edit_image->optional.CheckSum=1;
        } else if (!_strnicmp(argument->text,"stack:",6)) {
            parse_reserve_commit(argument->text+6,&edit_image->optional.SizeOfStackReserve,
                &edit_image->optional.SizeOfStackCommit);
            edit_optional_changed=1;
        } else if (!_strnicmp(argument->text,"heap:",5)) {
            parse_reserve_commit(argument->text+5,&edit_image->optional.SizeOfHeapReserve,
                &edit_image->optional.SizeOfHeapCommit);
            edit_optional_changed=1;
        } else if (!_strnicmp(argument->text,"section:",8)) edit_section(argument->text+8);
        else warning_message(program_name,0x73,argument->text,"");
    }
}

/* Original [00019f30,0001a050). Reserve[,commit], preserving unspecified values. */
void parse_reserve_commit(const char *text,unsigned long *reserve,unsigned long *commit)
{
    unsigned long first,second;
    unsigned short fields;
    unsigned mask;
    char remainder[256];
    if (!*text) mask=0;
    else if (*text==',') {
        sscanf(text,",%li%s",&second,remainder);
        mask=2;
    } else {
        fields=(unsigned short)sscanf(text,"%li,%li%s",&first,&second,remainder);
        if (fields==1) mask=1;
        else if (fields==2) mask=3;
        else mask=0;
    }
    if (!mask) { fatal_message(program_name,0x14,text); return; }
    if (mask&1) {
        if (first&3) first=(first&~3UL)+4;
        *reserve=first;
    }
    if (mask&2) {
        if (second&3) second=(second&~3UL)+4;
        *commit=second;
    }
}

#pragma intrinsic(memcpy,memset)
/* Original code [00028180,00028630), switch table [00028630,00028658). */
void edit_section(char *text)
{
    char old_name[9],new_name[8],*cursor=text,*start;
    unsigned long length,set_flags=0,clear_flags=0,*mask=&set_flags,alignment;
    unsigned short index;
    unsigned char found=0;
    PIMAGE_SECTION_HEADER section;
    while (*cursor && *cursor!='=' && *cursor!=',') ++cursor;
    length=cursor-text;
    if (!length || length>8) fatal_message(program_name,0x28,text);
    strncpy(old_name,text,length);
    memset(old_name+length,0,9-length);
    if (*cursor=='=') {
        start=++cursor;
        while (*cursor && *cursor!='=' && *cursor!=',') ++cursor;
        length=cursor-start;
        if (!length || length>8) fatal_message(program_name,0x28,text);
        strncpy(new_name,start,length);
        if (length<8) memset(new_name+length,0,8-length);
    } else new_name[0]=0;
    if (*cursor==',') ++cursor;
    else if (*cursor) fatal_message(program_name,0x28,text);
    for (;*cursor;++cursor) {
        switch ((unsigned char)*cursor) {
        case 'c': *mask|=0x20; break;
        case 'd': *mask|=0x02000000; break;
        case 'e': *mask|=0x20000000; break;
        case 'i': *mask|=0x40; break;
        case 'm': *mask|=0x800; break;
        case 'o': *mask|=0x200; break;
        case 'r': *mask|=0x40000000; break;
        case 's': *mask|=0x10000000; break;
        case 'u': *mask|=0x80; break;
        case 'w': *mask|=0x80000000; break;
        case 'k':
            if (mask==&set_flags) clear_flags|=0x04000000;
            else set_flags|=0x04000000;
            break;
        case 'p':
            if (mask==&set_flags) clear_flags|=0x08000000;
            else set_flags|=0x08000000;
            break;
        case 'a':
            ++cursor;
            if (*cursor=='x') *mask|=8;
            else if (mask==&clear_flags) {
                /* Original AND retains only previously requested alignment
                 * removals; negated alignment does not invent a clear mask. */
                *mask&=0x00700000;
            } else {
                clear_flags|=0x00700000;
                *mask&=~0x00700000UL;
                switch ((unsigned char)*cursor) {
                default:
                    fatal_message(program_name,0x28,text);
                    /* The native fatal helper exits; retain its fallthrough. */
                case '1': alignment=0x00100000; break;
                case '2': alignment=0x00200000; break;
                case '4': alignment=0x00300000; break;
                case '8': alignment=0x00400000; break;
                case 'p': alignment=0x00500000; break;
                case 't': alignment=0x00600000; break;
                case 's': alignment=0x00700000; break;
                }
                *mask|=alignment;
            }
            break;
        default:
            fatal_message(program_name,0x28,text);
        case '!':
        case 'n':
            if (!cursor[1]) fatal_message(program_name,0x28,text);
            mask=&clear_flags;
            continue;
        }
        mask=&set_flags;
    }
    for (index=1;index<=edit_image->file.NumberOfSections;++index) {
        section=&edit_section_headers[index];
        if (strncmp(old_name,(char *)section->Name,8)) continue;
        found=1;
        if (new_name[0]) memcpy(section->Name,new_name,8);
        section->Characteristics&=~clear_flags;
        section->Characteristics|=set_flags;
        seek_file(output_file,edit_image->file.SizeOfOptionalHeader+index*40+
            pe_header_offset+archive_data_offset-20,SEEK_SET);
        write_section_header(output_file,section);
    }
    if (!found) warning_message(program_name,0x6e,old_name,"");
}

/* Command-line switches and COFF .drectve options. */
#include "options.h"
#include "dos-stub.h"
#include "pass-one.h"
#include <io.h>
#pragma intrinsic(strlen,strcpy,strcat)
ArgumentList switches,section_merges,section_overrides,exports;
TextBuffer image_comments;
char *option_delimiters=" \t";
IncludedExternal *included_externals;
unsigned char base_pair,no_entry,verbose_link,mapped_debug,split_image,nt_beta1;
unsigned char old_library_format;
unsigned short warning_level=1;
char *map_filename,*def_filename="",*absolute_symbols_file="",*import_library_name;
/* run_cvpack distinguishes this default object by address. */
char cvpack_default[]="cvpack.exe";
char *cvpack_program=cvpack_default;
char *order_filename;
extern unsigned char banner_pending,reproducible,reference_option_explicit;
extern unsigned long debug_options,output_file_size;
extern char *entry_point,*output_name,*repro_directory;
extern FILE *repro_stream;
extern char *dos_stub_template;
extern unsigned long dos_stub_size;
void link_usage(void);

void process_options(Image *image,unsigned char directives,unsigned char library_member,const char *context)
{
    Argument *argument=switches.first;
    ParsedSwitch *option;
    unsigned short number,used,value_index;
    char saved[268],filename[_MAX_FNAME],extension[_MAX_EXT],path[_MAX_PATH];
    char drive[_MAX_DRIVE],directory[_MAX_DIR],line[261],display[256];
    char *text,*value,*token,*delimiter,*file,*destination;
    unsigned long first,second,major,minor,size,aligned;
    unsigned char specified,created,flags;
    IncludedExternal *include;
    External *external;
    FILE *stream;
    if (debug_enabled(21)) reproducible=1;
    for (number=0;number<switches.count;++number,argument=argument->next) {
        option=argument->parsed=parse_switch(argument->text);
        text=argument->text;
        strcpy(saved,text);
        used=0;
        if (text[0]=='?' && !text[1]) link_usage();
        if (!_strcmpi(text,"batch")) {}
        else if (!_strcmpi(option->name,"comment")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            value=option->values[0].value;
            append_text(&image_comments,value,strlen(value)+1);
            used=1;
        } else if (!_strcmpi(text,"nopack")) image->options.no_pack=1;
        else if (!_strcmpi(text,"nologo")) banner_pending=0;
        else if (!_strcmpi(text,"strict")) warning_message(program_name,0x63,text,"");
        else if (!_strcmpi(option->name,"export")) {
            if (text[6]!=':') goto bad_option;
            append_argument_node(&exports,text+7,(char *)(directives?context:NULL));
            goto next;
        } else if (!_strcmpi(option->name,"defaultlib")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            if (!image->options.no_default_libraries) {
                for (used=0;used<option->count;++used)
                    add_default_library(option->values[used].value,&image->libraries);
            }
            ++used;
        } else if (!_strcmpi(option->name,"opt")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            for (used=0;used<option->count;++used) {
                value=option->values[used].value;
                if (!_strcmpi(value,"ref")) image->options.eliminate_unreferenced=1;
                else if (!_strcmpi(value,"noref")) image->options.eliminate_unreferenced=0;
                else goto bad_option;
                reference_option_explicit=1;
            }
        } else if (!_strcmpi(option->name,"nodefaultlib") || !_strcmpi(option->name,"nod")) {
            goto exclude_library;
        } else if (!_strcmpi(option->name,"out")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            used=1;
            if (!(image->options.output_specified && directives)) {
                output_name=option->values[0].value;
                image->options.output_specified=1;
                if (repro_directory) {
                    _splitpath(output_name,NULL,NULL,filename,extension);
                    sprintf(saved,"out:.\\%s%s",filename,extension);
                }
            }
        } else if (!_strcmpi(option->name,"release")) image->options.checksum=1;
        else if (!_strcmpi(option->name,"base")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            if (image->options.base_specified && directives) { used=option->count; goto option_done; }
            value=option->values[0].value;
            if (*value=='@') {
                if (option->count<=1 || !*option->values[1].value) goto missing_value;
                file=find_library_file(value+1,".txt",&search_paths);
                if (repro_directory) {
                    _splitpath(file,NULL,NULL,filename,extension);
                    sprintf(saved,"base:@.\\%s%s,%s",filename,extension,option->values[1].value);
                    reproduce_input(file,0);
                }
                stream=fopen(file,"rt");
                if (!stream) fatal_message(program_name,7,file);
                specified=0;
                while (fgets(line,261,stream)) {
                    line[strlen(line)-1]=0;
                    delimiter=strchr(line,';');
                    if (delimiter) *delimiter=0;
                    token=strtok(line,option_delimiters);
                    if (token && !_strcmpi(token,option->values[1].value)) {
                        token=strtok(NULL,option_delimiters);
                        if (!token || sscanf(token,"%li",&first)!=1) fatal_message(file,0xe,token);
                        specified=1;
                        image->optional.ImageBase=first;
                        token=strtok(NULL,option_delimiters);
                        if (!token || sscanf(token,"%li",&output_file_size)!=1) fatal_message(file,0xe,token);
                    }
                    if (specified) break;
                }
                fclose(stream);
                if (!specified) fatal_message(file,0x13,option->values[1].value);
                free(file);
                used=2;
            } else {
                sscanf(text+5,"%li,%li",&first,&second);
                if (option->count==1) {
                    if (!read_switch_number(option,0,&image->optional.ImageBase)) goto bad_number;
                    used=1;
                } else if (option->count==2) {
                    split_image=1;
                    image->file.SizeOfOptionalHeader=224;
                    image->optional.ImageBase=0;
                    if (!read_switch_number(option,0,&image->optional.BaseOfCode) ||
                        !read_switch_number(option,1,&image->optional.BaseOfData)) goto bad_number;
                    used=2; base_pair=1;
                } else fatal_message(program_name,0xe,text+5);
            }
            image->options.base_specified=1;
        } else if (!_strnicmp(text,"Brepro",6)) reproducible=1;
        else if (!_strcmpi(option->name,"cvpack")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            cvpack_program=option->values[0].value; used=1;
        } else if (!_strcmpi(option->name,"debug")) {
            specified=0;
            for (used=0;used<option->count;++used) {
                value=option->values[used].value;
                if (!_strcmpi(value,"mapped")) mapped_debug=1;
                else if (!_strcmpi(value,"notmapped")) mapped_debug=0;
                else if (!_strcmpi(value,"full")) { specified=1; image->options.debug_level=3; }
                else if (!_strcmpi(value,"partial")) { specified=1; image->options.debug_level=2; }
                else if (!_strcmpi(value,"minimal")) { specified=1; image->options.debug_level=1; }
                else if (!_strcmpi(value,"none")) { specified=1; image->options.debug_level=0; }
                else goto bad_option;
            }
            if (!specified) image->options.debug_level=3;
        } else if (!_strcmpi(option->name,"merge")) {
            append_argument_node(&section_merges,text,argument->original);
            used=option->count;
        } else if (!_strcmpi(option->name,"ntbeta1")) nt_beta1=1;
        else if (!_strcmpi(option->name,"debugtype")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            for (used=0;used<option->count;++used) {
                value=option->values[used].value;
                if (!_strcmpi(value,"coff")) debug_options |= 1;
                else if (!_strcmpi(value,"cv") || !_strcmpi(value,"cvnew")) debug_options |= 2;
                else if (!_strcmpi(value,"both")) debug_options |= 3;
                else if (!_strcmpi(value,"fpo")) debug_options |= 8;
                else if (!_strcmpi(value,"fixup")) debug_options |= 0x10;
                else goto bad_option;
            }
        } else if (!_strcmpi(option->name,"entry")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            value=option->values[0].value;
            destination=entry_point=malloc(strlen(value)+2);
            if (*value!='?') *destination++='_';
            strcpy(destination,value);
            used=1;
        } else if (!_strcmpi(text,"force")) image->options.force=1;
        else if (!_strcmpi(text,"fixed")) {
            image->file.Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;
            image->options.fixed=1;
        } else if (!_strnicmp(text,"absolutesfrom:",14)) {
            if (!text[14]) goto bad_option;
            absolute_symbols_file=text+14;
            goto next;
        } else if (!_strnicmp(text,"map",3)) {
            image->options.map_format=2;
            if (text[3]) {
                if (text[3]!=':' || !text[4]) goto bad_option;
                map_filename=text+4;
                if (repro_directory) {
                    _splitpath(map_filename,NULL,NULL,filename,extension);
                    sprintf(saved,"map:.\\%s%s",filename,extension);
                }
            }
            goto next;
        } else if (!_strnicmp(text,"dll",3)) {
            image->file.Characteristics |= IMAGE_FILE_DLL;
            if (text[3]==':' && text[4]) {
                if (_strcmpi(text+4,"system")) goto bad_option;
                image->file.Characteristics |= IMAGE_FILE_SYSTEM;
            } else if (text[3]) goto bad_option;
            goto next;
        } else if (!_strcmpi(text,"noentry")) no_entry=1;
        else if (!_strnicmp(text,"implib:",7)) { import_library_name=text+7; goto next; }
        else if (!_strnicmp(text,"def:",4)) { if (text[4]) def_filename=text+4; goto next; }
        else if (!_strnicmp(text,"include:",8)) {
            value=text+8;
            external=lookup_external(image->symbols,(unsigned short)(strlen(value)<=8),value,&created,&flags);
            include=allocate_checked(sizeof(*include));
            include->external=external; include->next=included_externals; included_externals=include;
            goto next;
        } else if (!_strcmpi(text,"oldlib")) old_library_format=1;
        else if (!_strnicmp(text,"version:",8)) {
            value=text+8;
            if (!_strnicmp(value,"liborder=before,",16)) {
                value+=16;
                do {
                    delimiter=strchr(value,',');
                    if (delimiter) *delimiter=0;
                    if (find_library(value,image->libraries.first,1)) warning_message(program_name,0x80,context,value);
                    if (delimiter) { *delimiter=','; value=delimiter+1; }
                } while (delimiter);
                goto next;
            }
            major=minor=0;
            delimiter=strchr(value,'.');
            if (delimiter && (sscanf(delimiter+1,"%li",&minor)!=1 || minor>0xffff)) goto bad_version;
            if (sscanf(value,"%li",&major)!=1 || major>0xffff) goto bad_version;
            image->optional.MajorImageVersion=(unsigned short)major;
            image->optional.MinorImageVersion=(unsigned short)minor;
            goto next;
        } else if (!_strnicmp(text,"subsystem:",10)) {
            value=text+10;
            if (!_strnicmp(value,"native",6)) { image->optional.Subsystem=1; value+=6; }
            else if (!_strnicmp(value,"windows",7)) { image->optional.Subsystem=2; value+=7; }
            else if (!_strnicmp(value,"console",7)) { image->optional.Subsystem=3; value+=7; }
            else if (!_strnicmp(value,"posix",5)) { image->optional.Subsystem=7; value+=5; }
            else if (*value!=',') warning_message(program_name,0x52,value,"");
            if (*value!=',') goto next;
            ++value;
            major=minor=0;
            delimiter=strchr(value,'.');
            if (delimiter && (sscanf(delimiter+1,"%li",&minor)!=1 || minor>0xffff)) goto bad_version;
            if (sscanf(value,"%li",&major)!=1 || major>0xffff) goto bad_version;
            image->optional.MajorSubsystemVersion=(unsigned short)major;
            image->optional.MinorSubsystemVersion=(unsigned short)minor;
            goto next;
        } else if (!_strcmpi(option->name,"stack")) {
            if (!option->count) goto missing_value;
            used=2;
            if (!(image->options.stack_specified && directives)) {
                if (*option->values[0].value && !read_switch_number(option,0,&image->optional.SizeOfStackReserve)) goto bad_number;
                if (option->count>=2 && *option->values[1].value &&
                    !read_switch_number(option,1,&image->optional.SizeOfStackCommit)) goto bad_number;
                image->options.stack_specified=1;
                image->optional.SizeOfStackCommit=(image->optional.SizeOfStackCommit+3)&~3UL;
                image->optional.SizeOfStackReserve=(image->optional.SizeOfStackReserve+3)&~3UL;
            }
        } else if (!_strcmpi(option->name,"heap")) {
            if (!option->count) goto missing_value;
            used=2;
            if (!(image->options.heap_specified && directives)) {
                if (*option->values[0].value && !read_switch_number(option,0,&image->optional.SizeOfHeapReserve)) goto bad_number;
                if (option->count>=2 && option->values[1].value[1] &&
                    !read_switch_number(option,1,&image->optional.SizeOfHeapCommit)) goto bad_number;
                image->options.heap_specified=1;
                image->optional.SizeOfHeapCommit=(image->optional.SizeOfHeapCommit+3)&~3UL;
                image->optional.SizeOfHeapReserve=(image->optional.SizeOfHeapReserve+3)&~3UL;
                if (image->optional.SizeOfHeapReserve<image->optional.SizeOfHeapCommit)
                    image->optional.SizeOfHeapReserve=image->optional.SizeOfHeapCommit;
            }
        } else if (!_strcmpi(option->name,"machine")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            used=1; value=option->values[0].value;
            if (!_strcmpi(value,"i386") || !_strcmpi(value,"ix86")) image->file.Machine=0x14c;
            else if (!_strcmpi(value,"i860")) image->file.Machine=0x14d;
            else if (!_strcmpi(value,"M68K")) image->file.Machine=0x268;
            else if (!_strcmpi(value,"mips")) image->file.Machine=0x162;
            else warning_message(program_name,0x53,text+8,"iX86, M68K, or mips");
        } else if (!_strcmpi(option->name,"align")) {
            if (!option->count || !*option->values[0].value) goto missing_value;
            used=1;
            if (!read_switch_number(option,0,&image->optional.SectionAlignment)) goto bad_number;
            image->optional.SectionAlignment=(image->optional.SectionAlignment+511)&~511UL;
        } else if (!_strnicmp(text,"gpsize:",7)) {
            if (sscanf(text+7,"%li",&image->options.global_pointer_size)!=1) goto bad_option;
            goto next;
        } else if (!_strnicmp(text,"section:",8)) {
            if (!text[8]) goto bad_option;
            append_argument(&section_overrides,text+8); goto next;
        } else if (!_strcmpi(text,"verbose")) verbose_link=1;
        else if (!_strnicmp(text,"rom",3)) { image->options.rom=1; image->options.portable_image=0; }
        else if (!_strnicmp(text,"stub:",5)) {
            if (!text[5]) goto bad_option;
            stream=fopen(text+5,"rb");
            if (!stream) {
                _splitpath(text+5,drive,directory,filename,extension);
                if (!*drive && !*directory) {
                    _splitpath(_pgmptr,drive,directory,NULL,NULL);
                    _makepath(path,drive,directory,filename,extension);
                    stream=fopen(path,"rb");
                }
                if (!stream) { fatal_message(program_name,7,text+5); goto next; }
            }
            size=crt_filelength(stream->_file);
            if (size<64) fatal_message(program_name,0x23,text+5);
            aligned=(size+7)&~7UL;
            dos_stub_template=malloc(aligned+4);
            if (!dos_stub_template) out_of_memory();
            if (crt_fread(dos_stub_template,1,size,stream)!=size) fatal_message(program_name,0xa,text+5);
            fclose(stream);
            if (dos_stub_template[0]!='M' || dos_stub_template[1]!='Z') fatal_message(program_name,0x23,text+5);
            if (((IMAGE_DOS_HEADER *)dos_stub_template)->e_lfarlc<64) warning_message(text+5,0x83,"","");
            dos_stub_template[aligned]='P'; dos_stub_template[aligned+1]='E';
            dos_stub_template[aligned+2]=dos_stub_template[aligned+3]=0;
            ((IMAGE_DOS_HEADER *)dos_stub_template)->e_lfanew=aligned;
            dos_stub_size=aligned+4;
            goto next;
        } else if (!_strnicmp(text,"order:@",7)) {
            image->options.order_specified=1;
            order_filename=text+7;
            initialize_order(fatal_message,warning_message);
            if (repro_directory) {
                reproduce_input(order_filename,0);
                _splitpath(order_filename,NULL,NULL,filename,extension);
                sprintf(saved,"order:@.\\%s%s",filename,extension);
            }
            goto next;
        } else if (!_strnicmp(text,"warn",4)) {
            if (!text[4]) warning_level=2;
            else if (text[4]==':' && text[5]>='0' && text[5]<='3' && !text[6]) warning_level=text[5]-'0';
            else goto bad_option;
            goto next;
        } else { warning_message(program_name,0x73,text,""); goto next; }
option_done:
        if (option->count>used) warning_message(program_name,0x7f,text,"");
next:
        if (!directives && repro_directory) fprintf(repro_stream,"-%s\n",saved);
        continue;
exclude_library:
        if (directives) { warning_message(program_name,0x60,context,""); used=1; }
        else if (image->options.no_default_libraries) used=1;
        else if (option->count) { used=1; exclude_default_library(option->values[0].value,&image->libraries); }
        else { image->options.no_default_libraries=1; exclude_all_default_libraries(NULL,&image->libraries); }
        goto option_done;
bad_version:
        warning_message(program_name,0x51,value,""); goto next;
bad_number:
        fatal_message(program_name,0x32,text); goto next;
missing_value:
        fatal_message(program_name,0x31,text); goto next;
bad_option:
        fatal_message(program_name,0x14,text); goto next;
    }
}

/* Original [0001c830,0001c8d0). Parse switches embedded in a .drectve section. */
void process_directives(char *text,Image *image,const char *display,unsigned char library_member)
{
    char *token;
    const char *delimiters=option_delimiters;
    switches.first=switches.last=NULL;
    switches.count=0;
    token=tokenize_response(text,delimiters,NULL);
    while (token) {
        if (*token=='/' || *token=='-') {
            if (token[1]=='?') ++token;
            ++token;
            append_argument(&switches,duplicate_string(token));
        }
        token=tokenize_response(NULL,delimiters,NULL);
    }
    process_options(image,1,library_member,display);
}

/* Original [00035890,00035910). Append each requested default library once. */
void add_default_library(const char *name,LibraryList *libraries)
{
    DefaultLibrary **link;
    if (libraries->exclude_defaults) return;
    link=&libraries->default_libraries;
    while (*link) {
        if (matches_default_library(name,*link)) return;
        link=&(*link)->next;
    }
    *link=allocate_checked(sizeof(**link));
    (*link)->next=NULL;
    (*link)->name=duplicate_string(name);
    (*link)->enabled=1;
    if (verbose_link) fprintf(stdout,"Processed -defaultlib:%s\n",name);
}

/* Original [00035910,000359c0). Default-library names ignore a final .lib. */
unsigned char matches_default_library(const char *name,DefaultLibrary *library)
{
    unsigned short length=(unsigned short)strlen(name);
    unsigned short other=(unsigned short)strlen(library->name);
    if (length>=4 && !_strcmpi(name+length-4,".lib")) length-=4;
    if (other>=4 && !_strcmpi(library->name+other-4,".lib")) other-=4;
    return length==other && !_strnicmp(name,library->name,length);
}

Library *load_next_default_library(LibraryList *libraries)
{
    DefaultLibrary *candidate;
    char drive[_MAX_DRIVE],directory[_MAX_DIR],filename[_MAX_FNAME];
    char extension[_MAX_EXT],path[_MAX_PATH];
    if (libraries->exclude_defaults) return NULL;
    for (candidate=libraries->default_libraries;candidate;candidate=candidate->next) {
        if (!candidate->enabled) continue;
        candidate->enabled=0;
        _splitpath(candidate->name,drive,directory,filename,extension);
        if (!*extension) strcpy(extension,".lib");
        _makepath(path,drive,directory,filename,extension);
        if (!find_library(path,libraries->first,(unsigned char)(!*drive && !*directory)))
            return create_library(find_library_file(candidate->name,".lib",&search_paths),0,0,libraries);
    }
    return NULL;
}

char *find_library_file(const char *name,const char *default_extension,ArgumentList *paths)
{
    char drive[_MAX_DRIVE],directory[_MAX_DIR],filename[_MAX_FNAME],extension[_MAX_EXT];
    char with_extension[_MAX_PATH],candidate[_MAX_PATH];
    Argument *path;
    unsigned short index,length;
    _splitpath(name,drive,directory,filename,extension);
    if (!*extension && default_extension) {
        _makepath(with_extension,drive,directory,filename,default_extension);
        name=with_extension;
        strcpy(extension,default_extension);
    }
    if (!_access(name,0)) return duplicate_string(name);
    if (*drive || *directory) return duplicate_string(name);
    path=paths->first;
    for (index=0;index<paths->count;++index) {
        strcpy(candidate,path->text);
        length=(unsigned short)strlen(candidate);
        candidate[length]='\\';
        candidate[(unsigned short)(length+1)]=0;
        strcat(candidate,filename);
        strcat(candidate,extension);
        if (!_access(candidate,0)) return duplicate_string(candidate);
        path=path->next;
    }
    return duplicate_string(name);
}

void apply_section_merges(Image *image)
{
    Argument *argument=section_merges.first;
    Section *source,*destination;
    char *separator;
    unsigned short index;
    for (index=0;index<section_merges.count;++index) {
        if (!_strnicmp(argument->text,"merge:",6)) {
            separator=strchr(argument->text+6,'=');
            if (!separator) fatal_message(program_name,0x14,argument->text);
            *separator++=0;
            source=find_section_by_name(argument->text+6,&image->sections);
            if (!source) warning_message(program_name,0x6e,source,"(-merge)");
            destination=create_section(separator,source->characteristics,&image->sections,&image->optional);
            merge_sections(source,destination);
        }
        argument=argument->next;
    }
}

/* Original [000357a0,000357d0). A switch may suppress several libraries. */
void exclude_default_library(const char *name,LibraryList *libraries)
{
    char *token=strtok((char *)name,option_delimiters);
    while (token) {
        exclude_all_default_libraries(token,libraries);
        token=strtok(NULL,option_delimiters);
    }
}

/* Original [000357d0,00035890). NULL disables the entire default list. */
void exclude_all_default_libraries(const char *name,LibraryList *libraries)
{
    DefaultLibrary **link;
    unsigned char found=0;
    if (libraries->exclude_defaults) return;
    if (!name) {
        if (verbose_link)
            fprintf(stdout,"Processed -nodefaultlib (suppressing all default libs)\n");
        libraries->exclude_defaults=1;
        return;
    }
    link=&libraries->default_libraries;
    while (*link) {
        if (matches_default_library(name,*link)) {
            found=1;
            (*link)->enabled=0;
        }
        link=&(*link)->next;
    }
    if (!found) {
        *link=allocate_checked(sizeof(**link));
        (*link)->next=NULL;
        (*link)->name=duplicate_string(name);
        (*link)->enabled=0;
    }
    if (verbose_link) fprintf(stdout,"Processed -nodefaultlib:%s\n",name);
}

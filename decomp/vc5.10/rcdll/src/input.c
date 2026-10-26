#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "command.h"
#include <recon.h>

#ifdef _MSC_VER
#pragma intrinsic(strcpy)
#endif

/* Current source identity is shared with resource diagnostics. */
WCHAR rc_filename_storage[257];
WCHAR *rc_filename=rc_filename_storage;
unsigned rc_preparing_line,rc_prepared_lines;

int __stdcall rc_prepare_input(char *path)
{
    FILE *input,*output;
    char escaped[520],*name;
    WCHAR directive[4098],line[0x1051],*text,*option;
    unsigned line_number=0,page,message;
    int first=1,encoding;

    MultiByteToWideChar(rc_codepage,1,path,-1,rc_filename,257);
    input=fopen(path,"rb");
    if(!input){
        rc_format(rc_error_text,2048,rc_message(1110),path);
        return 0;
    }
    output=fopen(rc_input_temporary,"w+b");
    if(!output){
        strcpy(rc_error_text,rc_message(2180));
        return 0;
    }
    name=escaped;
    while(*path){
        *name++=*path;
        if(IsDBCSLeadByteEx(rc_codepage,(BYTE)*path)) *name++=path[1];
        else if(*path=='\\') *name++='\\';
        path=CharNextA(path);
    }
    *name=0;
    wcscpy(directive,L"#line 1\"");
    MultiByteToWideChar(rc_codepage,1,escaped,-1,directive+8,4090);
    wcscat(directive,L"\"\r\n");
    rc_write_bytes(output,directive,wcslen(directive)*2);
    encoding=rc_detect_encoding(input);
    while(rc_read_wide_line(line,0x1051,encoding==2,input)){
        rc_preparing_line=line_number++;
        if(!(line_number&63)) rc_notify_progress(1,line_number);
        page=0;
        text=line;
        while(*text&&(iswctype(*text,8)||*text==0xfeff)) ++text;
        if(rc_prefix_equal(L"rcinclude",text)){
            option=rc_skip_white(text+9,1);
            rc_write_bytes(output,L"#include \"",20);
            rc_write_bytes(output,option,wcslen(option)*2);
            rc_write_bytes(output,L"\"\r\n",6);
            continue;
        }
        if(rc_prefix_equal(L"#pragma",text)){
            option=rc_skip_white(text+7,0);
            if(rc_prefix_equal(L"code_page",option)){
                option=rc_skip_white(option+9,0);
                if(*option!='('){
                    strcpy(rc_error_text,rc_message(4210));
                    rc_input_error(4210);
                    continue;
                }
                option=rc_skip_white(option+1,0);
                if(iswctype(*option,4)){
                    while(iswctype(*option,4)) page=page*10+*option++-'0';
                    option=rc_skip_white(option,0);
                }else if(rc_prefix_equal(L"default",option)){
                    option+=7;
                    page=rc_saved_codepage;
                    option=rc_skip_white(option,0);
                }
                if(!page){
                    wsprintfA(rc_error_text,"%s%ws",rc_message(4212),option);
                    rc_input_error(4212);
                    continue;
                }
                if(*option!=')'){
                    strcpy(rc_error_text,rc_message(4211));
                    rc_input_error(4211);
                    continue;
                }
                if(page==1200) message=4213;
                else if(!IsValidCodePage(page)) message=4214;
                else goto emit_line;
                strcpy(rc_error_text,rc_message(message));
                if(!rc_warnings) rc_input_fatal(message);
                else rc_input_warning(message);
                continue;
            }
        }
        if(!*text){first=1;continue;}
        page=rc_codepage;
        if(first){
            swprintf(directive,L"#line %d\r\n",line_number);
            rc_write_bytes(output,directive,wcslen(directive)*2);
            first=0;
        }
emit_line:
        rc_codepage=page;
        rc_write_bytes(output,text,wcslen(text)*2);
        rc_write_bytes(output,L"\r\n",4);
    }
    rc_preparing_line=0;
    rc_prepared_lines=line_number;
    rc_codepage=rc_saved_codepage;
    fclose(output);
    fclose(input);
    return 1;
}

unsigned __stdcall rc_write_bytes(FILE *file,const void *data,unsigned bytes)
{
    unsigned written=fwrite(data,1,bytes,file);
    if(written!=bytes){
        rc_fatal("RC : fatal error RW1022: I/O error writing file.");
        written=0;
    }
    return written;
}

int __stdcall rc_detect_encoding(FILE *file)
{
    WCHAR sample[126];
    unsigned flags=0xffff,bytes;
    long position=ftell(file);
    int encoding=1;

    if(position%2) fgetc(file);
    bytes=fread(sample,1,250,file);
    *(WCHAR *)((char *)sample+bytes)=0;
    if(rc_is_text_unicode(sample,bytes,&flags))
        encoding=3-((flags&0x80)!=0x80);
    fseek(file,position,SEEK_SET);
    return encoding;
}

int __stdcall rc_read_wide_line(WCHAR *line,unsigned capacity,int wide,FILE *file)
{
    unsigned remaining=capacity-1,spaces;
    int character=0;
    WCHAR *output=line;
    char *buffer,*next;

    *line=0;
    if(!wide){
        next=buffer=LocalAlloc(LMEM_ZEROINIT,capacity);
        while(remaining&&(character=fgetc(file))!=EOF&&character!='\n'){
            if(character=='\r') continue;
            if(character=='\t'){
                spaces=8-((next-buffer)&7);
                if((int)remaining<=(int)spaces) spaces=remaining;
                remaining-=spaces;
                while(spaces--) *next++=' ';
                character=0;
            }else{*next++=(char)character;--remaining;}
        }
        *next=0;
        MultiByteToWideChar(rc_codepage,1,buffer,-1,line,next-buffer+1);
        LocalFree(buffer);
    }else{
        while(remaining&&(character=fgetc(file))!=EOF){
            character=(BYTE)character|((BYTE)fgetc(file)<<8);
            if(character=='\n') break;
            if(character=='\r') continue;
            if(character=='\t'){
                spaces=8-((output-line)&7);
                if((int)remaining<=(int)spaces) spaces=remaining;
                remaining-=spaces;
                while(spaces--) *output++=' ';
                character=0;
            }else{*output++=(WCHAR)character;--remaining;}
        }
        *output=0;
    }
    return character!=EOF||*line!=0;
}

int __stdcall rc_prefix_equal(const WCHAR *prefix,const WCHAR *text)
{
    WCHAR left,right;
    while(*prefix){
        if(!*text) return 0;
        left=towupper(*prefix);
        right=towupper(*text);
        if(left!=right) return 0;
        ++prefix;
        ++text;
    }
    return -1;
}

void __stdcall rc_notify_progress(WPARAM kind,LPARAM value)
{
    if(rc_window&&SendMessageA(rc_window,0x420,kind,value))
        rc_fatal("\n");
}

unsigned __stdcall rc_read_bytes(FILE *file,void *buffer,unsigned bytes)
{
    unsigned received=fread(buffer,1,bytes,file);
    if(ferror(file)){
        rc_fatal(rc_message(1121));
        received=0;
    }
    return received;
}

long __stdcall rc_seek(FILE *file,long offset,int origin)
{
    long position;
    char *error="RC : fatal error RW1023: I/O error seeking in file";
    if(fseek(file,offset,origin))rc_fatal(error);
    position=ftell(file);
    if(position==-1)rc_fatal(error);
    return position;
}

unsigned __stdcall rc_copy_file_bytes(FILE *input,FILE *output,unsigned bytes)
{
    void *buffer=rc_allocate(0x4000);
    unsigned remaining=bytes,chunk=bytes;
    while(remaining){
        chunk=remaining>0x4000?0x4000:remaining;
        remaining-=chunk;
#ifdef RECON_NATIVE_TEST
        /* The original writes the requested size even after a short read.
         * Only fully initialized chunks are supported by the native case. */
        if(rc_read_bytes(input,buffer,chunk)!=chunk){ __asm int 3 }
#else
        rc_read_bytes(input,buffer,chunk);
#endif
        rc_write_bytes(output,buffer,chunk);
    }
    rc_release(buffer);
    return chunk;
}

int __stdcall rc_copy_file(FILE *input,FILE *output)
{
    void *buffer=rc_allocate(0x4000);
    unsigned received;
    while((received=fread(buffer,1,0x4000,input))!=0)
        rc_write_bytes(output,buffer,received);
    rc_release(buffer);
    return 1;
}


WCHAR *__stdcall rc_skip_white(WCHAR *text,int terminate)
{
    WCHAR *word;
    while(*text&&iswctype(*text,8))++text;
    if(!terminate)return text;
    word=text;
    while(*text&&!iswctype(*text,8))++text;
    *text=0;
    return word;
}

#include <stdio.h>
#include <string.h>
#include <wchar.h>
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

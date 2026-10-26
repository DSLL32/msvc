#include <string.h>
#include <stdio.h>
#include <io.h>
#include "command.h"
#include "resources.h"
#include <recon.h>
#ifdef _MSC_VER
#pragma intrinsic(strcmp)
#pragma intrinsic(strlen,strcpy)
#endif

int rc_last_line,rc_message_count;
/* Bounded contextual diagnostic storage at original15818 and15a18. The first
 * span is488 raw-zero bytes plus24 loader-zero bytes; the second is entirely
 * loader-zero. These512-byte models do not recover original C capacities. */
char rc_normalized_diagnostic[512],rc_diagnostic_text[512];

void __stdcall rc_report_input_message(DWORD category,unsigned code,char *text)
{
    char *cursor=rc_diagnostic_text,*kind,*end,*input,*output;
    char number[32];
    unsigned length,index;

    if((int)rc_preparing_line>0&&rc_filename){
        for(index=0;index<257&&rc_filename[index];++index);
        if(index>=257){ __asm int 3 }
        sprintf(cursor,"%ws(%d) : ",rc_filename,(int)rc_preparing_line);
        cursor+=strlen(cursor);
    }
    if(category){
        switch(category){
        case 1000:kind=rc_message(314);break;
        case 2000:kind=rc_message(313);break;
        case 4000:kind=rc_message(312);break;
        default:kind=(char *)category;break;
        }
        end=kind;
        while(*end&&*end!='\n'&&*end!='\r')++end;
        *end=0;
        length=strlen(kind);
        if(length>=sizeof(rc_diagnostic_text)-(cursor-rc_diagnostic_text)){
            __asm int 3
        }
        strcpy(cursor,kind);cursor+=length;
        sprintf(number," %s%d: ","RC",(int)code);
        length=strlen(number);
        if(length>=sizeof(rc_diagnostic_text)-(cursor-rc_diagnostic_text)){
            __asm int 3
        }
        strcpy(cursor,number);cursor+=length;
        length=strlen(text);
        if(length>=sizeof(rc_diagnostic_text)-(cursor-rc_diagnostic_text)){
            __asm int 3
        }
        strcpy(cursor,text);
    }
    input=rc_diagnostic_text;output=rc_normalized_diagnostic;
    if(*input){
        do{
            if(input[0]=='\\'&&input[1]=='\\')++input;
            *output++=*input++;
        }while(*input);
    }
    *output=0;
    if(rc_diagnostic)rc_diagnostic(0,0,rc_diagnostic_text);
    if(rc_window&&SendMessageA(rc_window,0x419,1,(LPARAM)rc_diagnostic_text))
        rc_fatal("\n");
}

void __stdcall rc_input_warning(unsigned code)
{
    rc_report_input_message(4000,code,rc_error_text);
}

void __stdcall rc_resource_file_warning(unsigned code,const WCHAR *name)
{
    rc_format(rc_error_text,2048,rc_message(code),rc_resource_filename,
              rc_resource_token.line,name);
    rc_emit_notice(rc_error_text);
}

void __stdcall rc_emit(char *message)
{
    if (rc_diagnostic) rc_diagnostic(0,0,message);
    if (rc_window && SendMessageA(rc_window,0x419,0,(LPARAM)message))
        rc_fatal("\n");
    if (rc_resource_token.line==(DWORD)rc_last_line) {
        ++rc_message_count;
        if (rc_message_count>4 && strcmp(message,"\n"))
            rc_fatal("\n");
    } else {
        rc_message_count=0;
        rc_last_line=rc_resource_token.line;
    }
}


/* Notices notify the same clients as errors but do not advance the repeated
 * diagnostic counters used by rc_emit. */
void __stdcall rc_emit_notice(char *message)
{
    if(rc_diagnostic)rc_diagnostic(0,0,message);
    if(rc_window&&SendMessageA(rc_window,0x419,0,(LPARAM)message))
        rc_fatal("\n");
}

/* Original27fac/27fb0/27fb4: distinct loader-zero diagnostic history. */
DWORD rc_last_resource_error_line,rc_last_resource_error_file;
unsigned rc_last_resource_error_code;

void __stdcall rc_resource_error(unsigned code)
{
    if(rc_result>0&&rc_last_resource_error_code==code&&
       rc_last_resource_error_line==rc_resource_token.line&&
       rc_last_resource_error_file==rc_resource_token.file_index)
        rc_fatal("\n");
    rc_emit("\n");
    rc_format(rc_error_text,2048,rc_message(code),rc_resource_filename,
              rc_resource_token.line);
    rc_emit(rc_error_text);
    ++rc_result;
    if(rc_result>25)rc_fatal("\n");
    rc_last_resource_error_line=rc_resource_token.line;
    rc_last_resource_error_file=rc_resource_token.file_index;
    rc_last_resource_error_code=code;
}

void __stdcall rc_fatal(char *message)
{
    if(message){
        rc_emit("\n");
        rc_emit(message);
        rc_emit("\n");
    }
    rc_cleanup();
    if(rc_output_path)remove(rc_output_path);
    HeapDestroy(rc_heap);
    ++rc_result;
    longjmp(rc_abort_context,rc_result);
}

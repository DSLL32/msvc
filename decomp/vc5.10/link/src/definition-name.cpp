/* NAME/LIBRARY declarations retain the original path and directive ordering. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <ctype.h>
#include "librarian.h"
#include "definition-parser.h"
#include "options.h"
#include <recon.h>
extern "C" {
extern const char *default_phase,*definition_file;
char *__stdcall next_directive_token(char *,BYTE *);
char *__stdcall duplicate_argument(const char *);
void __stdcall read_librarian_directive(const char *);
BYTE definition_name_seen;
}
#pragma intrinsic(strlen,strcpy,strcat)
extern "C" WORD __stdcall read_definition_name(LinkContext *image,BYTE library)
{
    static const char *const flags[]={
        "BASE","WINDOWAPI","WINDOWCOMPAT","NOTWINDOWCOMPAT","NEWFILES",
        "LONGNAMES","INITINSTANCE","DYNAMIC",NULL
    };
    LinkContext *active;
    BYTE quoted=0;
    char *name,*flag;
    char drive[3]="",directory[256]="",basename[256],extension[256];
    char directive[128],number[32];
    DWORD base;
    WORD index;
    default_phase="ParseDefNameOrLibrary";
    if(definition_name_seen) {
        link_warning(definition_file,4017,library?"LIBRARY":"NAME");
        return next_definition_token();
    }
    definition_name_seen=1;
    name=next_directive_token(definition_cursor,&quoted);
    if(!name)return next_definition_token();
    active=librarian_context->image;
    if(library)read_librarian_directive("/DLL");
    _splitpath(name,drive,directory,basename,extension);
    if(*drive || *directory)
        link_warning(definition_file,4093,library?"LIBRARY":"NAME");
    active->options.library_basename=duplicate_argument(basename);
    if(*extension)active->options.default_extension=duplicate_argument(extension);
    else if(active->is_vxd==1)active->options.default_extension=".VXD";
    else if(!library)active->options.default_extension=".EXE";
    strcpy(directive,"/OUT:");
    if(quoted)strcat(directive,"\"");
    strcat(directive,active->options.library_basename);
    strcat(directive,active->options.default_extension);
    if(quoted)strcat(directive,"\"");
    read_librarian_directive(directive);
    if(image->is_vxd==1 && !library) {
        strcpy(directive,"/NAME:");
        strcat(directive,active->options.library_basename);
        read_librarian_directive(directive);
    }
    if(active->file_header.Machine==0x601 && !*extension)active->options.default_extension="";
    flag=next_directive_token(NULL,&quoted);
    while(flag) {
        if(!_strnicmp(flag,"BASE=",5))index=0;
        else {
            for(index=0;flags[index];++index)
                if(!_stricmp(flag,flags[index]))break;
        }
        switch(index) {
        case 0:
            if(image->is_vxd==1)goto invalid;
            if(sscanf(flag+5,"%li",&base)!=1)link_fatal(definition_file,1118,"NAME");
            strcpy(directive,"/BASE:0x");
            strcat(directive,_ultoa(base,number,16));
            read_librarian_directive(directive);
            break;
        case 1:
            if(image->is_vxd==1)goto invalid;
            read_librarian_directive("/SUBSYSTEM:WINDOWS");
            break;
        case 2:
            if(image->is_vxd==1)goto invalid;
            read_librarian_directive("/SUBSYSTEM:CONSOLE");
            break;
        case 3:
            if(image->is_vxd==1)goto invalid;
            link_warning(definition_file,4017,flag);
            break;
        case 4:case 5:case 6:
            if(image->is_vxd==1)goto invalid;
            break;
        case 7:
            if(image->is_vxd!=1)goto invalid;
            read_librarian_directive("/EXETYPE:DYNAMIC");
            break;
        default:
        invalid:
            link_fatal(definition_file,1118,library?"LIBRARY":"NAME");
        }
        flag=next_directive_token(NULL,&quoted);
    }
    return next_definition_token();
}

extern "C" WORD __stdcall read_definition_description(char *text)
{
    char *value,*last;
    default_phase="ParseDefDescription";
    if(text && *text) {
        value=text;
        while(*value==' ' || *value=='\t')++value;
        if(*value=='\'' && *++value) {
            last=(char *)_mbsrchr((const unsigned char *)text,'\'');
            if(last)*last=0;
        }
        definition_text=duplicate_argument(value);
    }
    return next_definition_token();
}

#undef isdigit
extern "C" WORD __stdcall read_vxd_definition_export(char *text)
{
    char directive[128],separator=*text,*cursor=text;
    WORD token;
    unsigned length;
    default_phase="ParseVxdDefExport";
    if(!*text)return 0;
    while(*cursor) {
        if(*cursor==' ' || *cursor=='\t' || *cursor=='=') {
            separator=*cursor;
            *cursor=0;
        } else ++cursor;
    }
#ifdef __linux__
    /* The original reads the byte after this token's NUL. At the physical
     * line owner's last byte that would consume the following pointer owner. */
    if((DWORD)cursor<(DWORD)definition_line ||
       (DWORD)cursor+1>=(DWORD)definition_line+sizeof(definition_line)) { __asm { int 3 } }
#endif
    ++cursor;
    if(*cursor) {
        while(*cursor==' ' || *cursor=='\t')++cursor;
        if(separator==' ' || separator=='\t') {
            separator=*cursor;
            if(separator=='=' || separator=='@')++cursor;
        }
    }
    if(separator) {
        token=find_definition_keyword(text);
        if(token!=(WORD)-1)return token;
    }
    strcpy(directive,"/EXPORT:");
    strcat(directive,text);
    if(separator=='@') {
        strcat(directive,",@");
        length=strlen(directive);
        while(isdigit((signed char)*cursor))directive[length++]=*cursor++;
        directive[length]=0;
    }
    read_librarian_directive(directive);
    return 0;
}

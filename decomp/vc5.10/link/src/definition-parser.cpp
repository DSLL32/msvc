/* Translate a module-definition file's actual keyword stream into the same
 * librarian/linker directives. Keyword helpers remain their real App owners. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "librarian.h"
#include "definition-parser.h"
#include "options.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*definition_file;
const char *const definition_keywords[23]={
    "NAME","LIBRARY","DESCRIPTION","HEAPSIZE","STACKSIZE","PROTMODE",
    "CODE","DATA","SEGMENTS","EXPORTS","IMPORTS","STUB","OLD","APPLOADER",
    "EXETYPE","REALMODE","FUNCTIONS","INCLUDE","SECTIONS","OBJECTS","VERSION","VXD",NULL
};
FILE *definition_source;
char *definition_cursor,*definition_text;
DWORD definition_version;
void __stdcall read_librarian_directive(const char *);
}

#pragma intrinsic(strlen,strcpy,strcat)
extern "C" void __stdcall read_librarian_definition(LinkContext *image,const char *name)
{
    LinkContext *active=librarian_context->image;
    WORD token;
    char directive[128],number[32];
    char *remainder,*period;
    DWORD reserve,commit,major,minor;
    default_phase="ParseDefinitionFile";
    if(!*name)return;
    definition_source=fopen(name,"rt");
    if(!definition_source)link_fatal(NULL,1104,name);
    token=next_definition_token();
    while(token!=(WORD)-1) {
        remainder=definition_cursor;
        /* The lexer splits each keyword from its remaining physical line.
         * Advance the shared cursor past that first terminating NUL. */
        if(remainder) {
            while(*remainder++) {}
            definition_cursor=remainder;
        }
        switch(token) {
        case 0: case 1:
            token=read_definition_name(image,token==1);
            break;
        case 2:
            if(active->is_vxd!=1)goto ignored;
            token=read_definition_description(remainder);
            if(*definition_text) {
                if(strlen(definition_text)>=117)definition_text[117]=0;
                strcpy(directive,"/COMMENT:\"");
                strcat(directive,definition_text);
                strcat(directive,"\"");
                read_librarian_directive(directive);
            }
            break;
        case 3:
            reserve=commit=0;
            token=read_definition_size("HEAP",&reserve,&commit);
            if(reserve) {
                strcpy(directive,"/HEAP:0x");
                strcat(directive,_ultoa(reserve,number,16));
                if(commit) {
                    strcat(directive,",0x");
                    strcat(directive,_ultoa(commit,number,16));
                }
                read_librarian_directive(directive);
            }
            break;
        case 4:
            reserve=commit=0;
            token=read_definition_size("STACK",&reserve,&commit);
            if(reserve) {
                strcpy(directive,"/STACK:0x");
                strcat(directive,_ultoa(reserve,number,16));
                if(commit) {
                    strcat(directive,",0x");
                    strcat(directive,_ultoa(commit,number,16));
                }
                read_librarian_directive(directive);
            }
            /* The original repeats this call even when reserve is zero. It
             * then uses prior buffer contents, or unwritten bytes if no earlier
             * directive initialized the buffer; that domain is unqualified. */
            read_librarian_directive(directive);
            break;
        case 8: case 18: case 19:
            token=read_definition_sections(definition_keywords[token],remainder);
            break;
        case 9:
            token=read_definition_exports(image,remainder);
            break;
        case 11:
            if(active->is_vxd!=1) {
                link_warning(definition_file,4017,definition_keywords[11]);
                goto ignored;
            }
            token=read_definition_stub(remainder);
            break;
        case 14:
            if(active->is_vxd==1) {
                if(!_stricmp((const char *)_mbstok((unsigned char *)remainder,
                    (const unsigned char *)" \t"),"dev386"))goto ignored;
            } else if(!_stricmp(remainder,"windows"))goto ignored;
            goto unsupported;
        case 20:
            major=minor=0;
            period=strchr(remainder,'.');
            if(period && (sscanf(period+1,"%li",(LONG *)&minor)!=1 || minor>0x7fff))
                link_fatal(definition_file,1118,"VERSION");
            if(sscanf(definition_cursor,"%li",(LONG *)&major)!=1 || major>0x7fff)
                link_fatal(definition_file,1118,"VERSION");
            definition_version=(major<<16)+minor;
            if(definition_version) {
                strcpy(directive,"/VERSION:0x");
                strcat(directive,_ultoa(major,number,16));
                if(minor) {
                    strcat(directive,".0x");
                    strcat(directive,_ultoa(minor,number,16));
                }
                read_librarian_directive(directive);
            }
            goto ignored;
        case 21:
            if(active->is_vxd!=1) {
                link_warning(definition_file,4017,definition_keywords[21]);
                goto ignored;
            }
            token=read_definition_name(image,0);
            break;
        default:
unsupported:
            link_warning(definition_file,4017,definition_keywords[token]);
ignored:
            token=next_definition_token();
            break;
        }
    }
    fclose(definition_source);
}

/* The original 4803b8 byte buffer accumulates .drectve payload, without NULs.
 * This operation appends text; it does not immediately process the switches. */
extern "C" { ByteBuffer definition_directives; }
extern "C" void __stdcall read_librarian_directive(const char *text)
{
    append_bytes(&definition_directives," ",strlen(" "));
    append_bytes(&definition_directives,text,strlen(text));
}

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

/* HEAPSIZE/STACKSIZE share the existing line cursor and caller-owned DWORDs.
 * A leading comma assigns only commit; omitted outputs keep their incoming
 * values, as in the actual parser's zero-initialized call sites. */
extern "C" WORD __stdcall read_definition_size(const char *kind,DWORD *reserve,DWORD *commit)
{
    default_phase="ParseSizes";
    if(definition_cursor && *definition_cursor) {
        int expected,result;
        if(strchr(definition_cursor,',')) {
            if(*definition_cursor==',') {
                ++definition_cursor;
                expected=1;
                result=sscanf(definition_cursor,"%li",(LONG *)commit);
            } else {
                expected=2;
                result=sscanf(definition_cursor,"%li,%li",(LONG *)reserve,(LONG *)commit);
            }
        } else {
            expected=1;
            result=sscanf(definition_cursor,"%li",(LONG *)reserve);
        }
        if(result!=expected)link_fatal(definition_file,1118,kind);
        if(*commit>*reserve) {
            link_warning(definition_file,4015,kind,*reserve);
            *commit=*reserve;
        }
    }
    return next_definition_token();
}
static const char *const definition_section_attributes[]={
    "EXECUTEONLY",
    "EXECUTEREAD",
    "READONLY",
    "READWRITE",
    "SHARED",
    "NONSHARED",
    "CONFORMING",
    "NONCONFORMING",
    "DISCARDABLE",
    "NONDISCARDABLE",
    "NONE",
    "SINGLE",
    "MULTIPLE",
    "IOPL",
    "NOIOPL",
    "PRELOAD",
    "LOADONCALL",
    "MOVEABLE",
    "MOVABLE",
    "FIXED",
    "EXECUTE",
    "READ",
    "WRITE",
    "PURE",
    "IMPURE",
    "RESIDENT",
    NULL
};
extern "C" WORD __stdcall find_section_attribute(const char *name)
{
    WORD index;
    for(index=0;definition_section_attributes[index];++index)
        if(!_stricmp(name,definition_section_attributes[index]))return index;
    return (WORD)-1;
}
#pragma intrinsic(strcpy,strcat,strcmp)
extern "C" WORD __stdcall read_definition_sections(const char *kind,char *text)
{
    char directive[128],qualified[128];
    char *section_name;
    const char *class_name;
    char *attribute,*closing;
    WORD token;
    DWORD flags;
    BYTE has_class,non_discardable,iopl,conforming;
    LinkContext *image=librarian_context->image;
    default_phase="ParseDefSections";
    if(text)while(*text==' ' || *text=='\t')++text;
    else text=read_definition_line();
    definition_cursor=text;
    while(definition_cursor) {
        if((unsigned char)*definition_cursor==0x7f)return (WORD)-1;
        if(*definition_cursor) {
            section_name=(char *)_mbstok((unsigned char *)definition_cursor,
                (const unsigned char *)" \t");
            token=find_definition_keyword(section_name);
            if(token!=(WORD)-1)return token;
            if(*section_name=='\'' || *section_name=='"') {
                char quote=*section_name++;
                closing=strchr(section_name,quote);
                if(closing)*closing=0;
            }
            if(!valid_section_name((const unsigned char *)section_name))
                link_fatal(definition_file,1185,section_name);
            flags=0;
            has_class=non_discardable=iopl=conforming=0;
            attribute=(char *)_mbstok(NULL,(const unsigned char *)" \t");
            if(attribute && !_stricmp(attribute,"CLASS")) {
                if(image->is_vxd==1) {
                    class_name=(char *)_mbstok(NULL,(const unsigned char *)"' \t");
                    has_class=1;
                } else _mbstok(NULL,(const unsigned char *)" \t");
                attribute=(char *)_mbstok(NULL,(const unsigned char *)" \t");
            }
            while(attribute) {
                switch(find_section_attribute(attribute)) {
                case 0:case 20:flags|=0x20000000;break;
                case 1:flags|=0x60000000;break;
                case 2:case 21:flags|=0x40000000;break;
                case 3:flags|=0xc0000000;break;
                case 4:flags|=0x10000000;break;
                case 5:case 10:case 11:case 12:break;
                case 6:
                    if(image->is_vxd==1)conforming=1;
                    else link_warning(definition_file,4017,attribute);
                    break;
                case 8:flags|=0x02000000;break;
                case 9:if(image->is_vxd==1)non_discardable=1;break;
                case 13:
                    if(image->is_vxd==1)iopl=1;
                    else link_warning(definition_file,4017,attribute);
                    break;
                case 15:if(image->is_vxd==1)flags|=0x80000;break;
                case 22:flags|=0x80000000;break;
                case 25:if(image->is_vxd==1)flags|=0x40000;break;
                case 7:case 14:case 16:case 17:case 18:case 19:case 23:case 24:
                    link_warning(definition_file,4017,attribute);
                    break;
                default:link_fatal(definition_file,1118,kind);break;
                }
                attribute=(char *)_mbstok(NULL,(const unsigned char *)" \t");
            }
            if(flags || has_class || non_discardable) {
                strcpy(directive,"/SECTION:");
                if(image->is_vxd==1) {
                    if(has_class) { strcpy(qualified,class_name);strcat(qualified,"_vxd"); }
                    else strcpy(qualified,"vxd");
                    if(flags&0x80000)strcat(qualified,"p");
                    if(flags&0x20000000)strcat(qualified,"e");
                    if(flags&0x40000000)strcat(qualified,"r");
                    if(flags&0x80000000)strcat(qualified,"w");
                    if(flags&0x10000000)strcat(qualified,"s");
                    if(flags&0x02000000)strcat(qualified,"d");
                    if(non_discardable)strcat(qualified,"n");
                    if(iopl)strcat(qualified,"i");
                    if(conforming)strcat(qualified,"c");
                    if(flags&0x40000)strcat(qualified,"x");
                    strcat(directive,qualified);
                } else strcat(directive,section_name);
                strcat(directive,",");
                if(flags&0x02000000)strcat(directive,"D");
                if(flags&0x20000000)strcat(directive,"E");
                if(flags&0x40000000)strcat(directive,"R");
                if(flags&0x10000000)strcat(directive,"S");
                if(flags&0x80000000)strcat(directive,"W");
                if(image->is_vxd==1) {
                    if(flags&0x80000)strcat(directive,"L");
                    if(flags&0x40000)strcat(directive,"X");
                    if(iopl)strcat(directive,"I");
                    if(conforming)strcat(directive,"C");
                    if(non_discardable)strcat(directive,"!D");
                }
                read_librarian_directive(directive);
                if(image->is_vxd==1) {
                    strcpy(directive,"/MERGE:");
                    strcat(directive,section_name);
                    strcat(directive,"=");
                    strcat(directive,qualified);
                    read_librarian_directive(directive);
                    const char *alias=NULL;
                    if(!strcmp(section_name,"_TEXT"))alias=".text";
                    else if(!strcmp(section_name,"_DATA"))alias=".data";
                    else if(!strcmp(section_name,"_BSS"))alias=".bss";
                    else if(!strcmp(section_name,"CONST"))alias=".rdata";
                    if(alias) {
                        strcpy(directive,"/MERGE:");
                        strcat(directive,alias);
                        strcat(directive,"=");
                        strcat(directive,qualified);
                        read_librarian_directive(directive);
                    }
                }
            }
        }
        definition_cursor=read_definition_line();
    }
    return (WORD)-1;
}

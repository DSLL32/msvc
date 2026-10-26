/* Parse one ordinary DEF export, or return a newly encountered DEF keyword. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include "definition-parser.h"
#include "options.h"
#include <recon.h>
extern "C" {
extern const char *default_phase,*definition_file;
extern BYTE mac_target,librarian_undecorated_exports;
void __stdcall record_librarian_ordinal(DWORD);
void __stdcall define_librarian_export(const char *,const char *,DWORD,DWORD,DWORD,
    const char *,DWORD,LinkContext *,DWORD,DWORD);
}
extern "C" WORD __stdcall read_definition_export_entry(LinkContext *image,char *text,const char *file)
{
    char *name=text,*part;
    char separator;
    char *internal=NULL;
    DWORD ordinal=0,type=0;
    BYTE noname=0,private_export=0;
    WORD token;
    default_phase="ParseAnExport";
    if(!*text)return 0;
    while(*text && *text!=' ' && *text!='\t' && *text!='=')++text;
    separator=*text;
    if(separator)*text++=0;
    token=find_definition_keyword(name);
    if(token!=(WORD)-1)return token;
    part=(char *)_mbstok((unsigned char *)text,(const unsigned char *)" \t");
    if(separator!='=' && part && *part=='=') {
        ++part;
        if(!*part)part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
        separator='=';
    }
    if(separator=='=') {
        internal=part;
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
    }
    if(part && *part=='@') {
        ++part;
        if(!*part) {
            part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
            if(!part)link_fatal(file,1119,"");
        }
        if(sscanf(part,"%lu",&ordinal)!=1 || !ordinal || ordinal>65535)
            link_fatal(file,1119,part);
        record_librarian_ordinal(ordinal);
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
        if(part && !_stricmp(part,"NONAME")) {
            noname=1;
            part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
        } else if(part && !_stricmp(part,"RESIDENTNAME")) {
            link_warning(definition_file,4017,part);
            part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
        }
    }
    if(part && !_stricmp(part,"PRIVATE")) {
        private_export=1;
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
    }
    if(part && !_stricmp(part,"CONSTANT")) {
        link_warning(definition_file,4087);
        type=1;
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
    } else if(part && !_stricmp(part,"DATA")) {
        type=mac_target?0:2;
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
    }
    if(part && !_stricmp(part,"NODATA")) {
        link_warning(definition_file,4017,part);
        part=(char *)_mbstok(NULL,(const unsigned char *)" \t");
    }
    if(part)link_warning(file,4017,part);
    /* Physical call slots are DWORDs. Only the flag bytes are meaningful;
     * their actual callee reads them as BYTEs, not as pointer/register bits. */
    define_librarian_export(name,internal,noname,type,ordinal,file,0,image,
        librarian_undecorated_exports,private_export);
    return 0;
}

extern "C" { extern DWORD librarian_next_ordinal; DWORD librarian_last_ordinal; }
extern "C" void __stdcall record_librarian_ordinal(DWORD ordinal)
{
    if(!librarian_next_ordinal) {
        librarian_last_ordinal=ordinal;
        librarian_next_ordinal=ordinal;
        return;
    }
    if(ordinal<librarian_next_ordinal) {
        librarian_next_ordinal=ordinal;
        return;
    }
    if(ordinal>librarian_last_ordinal)librarian_last_ordinal=ordinal;
}

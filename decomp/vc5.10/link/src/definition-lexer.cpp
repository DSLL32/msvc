/* The real keyword-token wrapper; line storage and lookup remain separate owners. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include "definition-parser.h"
#include "options.h"
#include <recon.h>
extern "C" char *__cdecl read_definition_line(void);
extern "C" WORD __stdcall find_definition_keyword(const char *);
extern "C" { extern const char *definition_file,*default_phase; }
extern "C" WORD __cdecl next_definition_token(void)
{
    char *word;
    WORD token;
    definition_cursor=read_definition_line();
    while(definition_cursor) {
        if((unsigned char)*definition_cursor==0x7f)break;
        if(*definition_cursor) {
            word=(char *)_mbstok((unsigned char *)definition_cursor,
                (const unsigned char *)" \t");
            if(word) {
                token=find_definition_keyword(word);
                if(token!=(WORD)-1)return token;
                link_warning(definition_file,4017,word);
            }
        }
        definition_cursor=read_definition_line();
    }
    return (WORD)-1;
}

/* The original 4096-byte input owner is bounded by the following token pointer
 * at 4803a8. Only fgets-produced bytes are consumed; old tails stay untouched. */
extern "C" { char definition_line[4096]; }
#pragma intrinsic(strlen)
extern "C" char *__cdecl read_definition_line(void)
{
    char *comment,*text;
    int last;
    if(!fgets(definition_line,sizeof(definition_line),definition_source))return "\177";
    last=(int)strlen(definition_line)-1;
#ifdef __linux__
    /* The original unconditionally reads line[last-1], which can reach the
     * preceding physical FILE-pointer owner for strings shorter than 2 bytes.
     * That pointer-byte/layout-dependent domain is not qualified natively. */
    if(last<1) { __asm { int 3 } }
#endif
    if(definition_line[last]=='\n')definition_line[last]=0;
    if(definition_line[last-1]=='\r')definition_line[last-1]=0;
    comment=(char *)_mbschr((const unsigned char *)definition_line,';');
    if(comment)*comment=0;
    text=definition_line;
    while(_ismbcspace((signed char)*text))++text;
    return text;
}

/* Keyword spelling is case-sensitive, with one shared NULL-terminated table. */
#pragma intrinsic(strcmp)
extern "C" WORD __stdcall find_definition_keyword(const char *name)
{
    WORD index;
    for(index=0;definition_keywords[index];++index)
        if(!strcmp(name,definition_keywords[index]))return index;
    return (WORD)-1;
}

extern "C" WORD __stdcall read_definition_exports(LinkContext *image,char *text)
{
    WORD token;
    default_phase="ParseDefExports";
    if(text) {
        while(_ismbcspace((signed char)*text))++text;
    } else text=read_definition_line();
    definition_cursor=text;
    while(text && (unsigned char)*text!=0x7f) {
        if(image->is_vxd==1)token=read_vxd_definition_export(text);
        else token=read_definition_export_entry(image,text,definition_file);
        if(token)return token;
        definition_cursor=text=read_definition_line();
    }
    return (WORD)-1;
}

#include <wchar.h>
#include "directives.h"
#include <recon.h>

typedef struct RcDirectiveName {
    const WCHAR *suffix;
    unsigned char code;
} RcDirectiveName;

const RcDirectiveName rc_directive_names[]={
    {L"efine",PP_DEFINE},
    {L"rror",PP_ERROR},{L"ndif",PP_ENDIF},
    {L"lse",PP_ELSE},{L"lif",PP_ELIF},
    {L"nclude",PP_INCLUDE},{L"fndef",PP_IFNDEF},
    {L"fdef",PP_IFDEF},{L"f",PP_IF},{L"dent",PP_IDENT},
    {L"ine",PP_LINE},{L"ragma",PP_PRAGMA},{L"ndef",PP_UNDEF}
};
/* Group boundaries for '_', '`', a..z, followed by the end sentinel. */
const unsigned char rc_directive_groups[]={
    0,0,0,0,0,0,1,5,5,5,5,10,10,10,11,
    11,11,11,12,12,12,12,12,13,13,13,13,13,13
};

unsigned char __stdcall rc_directive_code(const WCHAR *word)
{
    const RcDirectiveName *name,*end;
    unsigned character=*word;

    if(character<L'_') return PP_UNKNOWN;
#ifdef RECON_NATIVE_TEST
    /* The original's unchecked high-character index can leave the identified
     * group table. Hold before reading unavailable adjacent data. */
    if(character>L'z'){
        __asm int 3
    }
#endif
    name=rc_directive_names+rc_directive_groups[character-L'_'];
    end=rc_directive_names+rc_directive_groups[character-L'_'+1];
    ++word;
    while(name!=end){
        if(!wcscmp(name->suffix,word)) return name->code;
        ++name;
    }
    return PP_UNKNOWN;
}

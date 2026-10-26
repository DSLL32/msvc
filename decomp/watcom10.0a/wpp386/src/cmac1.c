/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* bld/plusplus/c/cmac1.c initial commit6b9cb443. In1994 the special
 * entries are built here, without the later MacroSpecialAdd helper or
 * nested-macro carver. Names and order match the encountered six-row table. */
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "frontend.h"
#include "initstate.h"
#include "macro.h"
#include "scan.h"
#include "tokenbuf.h"
#include "pending.h"
#include <recon.h>

static struct special_macro_name {
    char *name;
    int value;
} SpcMacros[] = {
    { "__LINE__", 0 },
    { "__FILE__", 1 },
    { "__DATE__", 2 },
    { "__TIME__", 3 },
    { "__STDC__", 4 },
    { "__cplusplus", 5 },
    { 0, 0 }
};
/* Actual1994 scanner token-list owner at originalbd4bc. */
void *macro_expansion_state;
#pragma pack(1)
typedef struct macro_token {
    struct macro_token *next;
    unsigned char token;
    char data[1];
} MACRO_TOKEN;
typedef struct macro_arg { char *arg; } MACRO_ARG;
typedef struct token_list {
    struct token_list *next;
    unsigned length;
    char buf[1];
} TOKEN_LIST;
typedef struct nested_macro {
    struct nested_macro *next;
    MEPTR fmentry;
    MACRO_ARG *macro_parms;
    unsigned char rescanning, substituting_parms;
} NESTED_MACRO;
#pragma pack()
static NESTED_MACRO *nestedMacros;
static MACRO_TOKEN *internalTokenList;
static unsigned macroDepth;
unsigned char InitialMacroFlag;
/* Encountered scanner storage:512-byte token capacity, with the
 * released scanner's unrolled-read slack and overrun word. */
char Buffer[512+16+4];

void macroInit(INITFINI *entry)
{
    struct special_macro_name *mac;
    MEPTR macptr;
    unsigned required;

    (void)entry;
    DirectiveInit();
    macro_expansion_state = 0;
    nestedMacros = 0;
    InitialMacroFlag = 1;
    MacroStorageInit();
    for (mac = SpcMacros; mac->name; ++mac) {
        required = strlen(mac->name) + offsetof(MEDEFN, macro_name) + 1;
        macptr = (MEPTR)CMemAlloc(required);
        macptr->macro_defn = 0;
        macptr->macro_len = required;
        macptr->parm_count = mac->value;
        strcpy(macptr->macro_name, mac->name);
        MacroDefine(macptr, Buffer, 0);
    }
    TimeInit();
}

int SpecialMacro(MEPTR entry) { (void)entry; STUB_TRAP(); return 0; }
MACRO_TOKEN *buildAToken(char *text);
MACRO_ARG *collectParms(void);
MACRO_TOKEN *buildMTokenList(char *tokens, MACRO_ARG *parms);
MACRO_TOKEN *substituteParms(MACRO_TOKEN *head, MACRO_ARG *parms);
MACRO_TOKEN *glueTokens(MACRO_TOKEN *head);
MACRO_TOKEN *appendToken(MACRO_TOKEN *head, int token, char *text);
MACRO_TOKEN *macroExpansion(int rescanning)
{
    NESTED_MACRO *nested;
    MEPTR entry=fmentry;
    MACRO_ARG *parms;
    MACRO_TOKEN *head, *token;
    NESTED_MACRO *search;
    nested=(NESTED_MACRO *)CMemAlloc(14);
    nested->substituting_parms=0;
    nested->macro_parms=NULL;
    nested->fmentry=entry;
    nested->rescanning=(unsigned char)rescanning;
    if (!entry->macro_defn) {
        CurToken=SpecialMacro(entry);
        head=buildAToken(Buffer);
        head->token=(unsigned char)CurToken;
        nested->next=nestedMacros;
        nestedMacros=nested;
    } else {
        parms=collectParms();
        nested->next=nestedMacros;
        nested->macro_parms=parms;
        nestedMacros=nested;
        head=buildMTokenList((char *)entry+entry->macro_defn,parms);
        if (parms) {
            nested->substituting_parms=1;
            head=substituteParms(head,parms);
            nested->substituting_parms=0;
        }
        head=glueTokens(head);
        /* Old code performs this walk inline; later source split it into
         * markUnexpandableIds. Keep the actual call graph. */
        for (token=head; token; token=token->next) {
            if (token->token==0x36) {
                for (search=nestedMacros; search; search=search->next) {
                    if (!strcmp(search->fmentry->macro_name,token->data) &&
                        !search->substituting_parms) {
                        token->token=0xa9;
                        break;
                    }
                }
            }
        }
    }
    return appendToken(head,0,"Z-<end of macro>");
}
unsigned macroTextLen(char *text)
{
    unsigned length=0;
    while (*text++) ++length;
    return length;
}
MACRO_TOKEN *buildAToken(char *text)
{
    MACRO_TOKEN *token=(MACRO_TOKEN *)CMemAlloc(macroTextLen(text)+6);
    char *target=token->data;
    token->next=NULL;
    do { *target++=*text; } while (*text++);
    return token;
}
void __cdecl CErr(unsigned number, ...);
int nextMToken(void) { STUB_TRAP(); return 0; }
void saveParm(MEPTR entry, int count, MACRO_ARG *parms,
              TOKEN_LIST *tokens, int total)
{ (void)entry; (void)count; (void)parms; (void)tokens; (void)total; STUB_TRAP(); }
void *ScopeFunctionInProgress(void);
int CurrFunctionHasEllipsis(void) { STUB_TRAP(); return 0; }
/* The old fixed buffer predates the released TokenBuf objects. Spill a
 * genuine ring-owned chunk before reusing it, including for object macros. */
void saveTokenBuffer(TOKEN_LIST **head)
{
    unsigned size=MacroTokenBufferSize();
    TOKEN_LIST *tokens;
    if (size) {
        tokens=(TOKEN_LIST *)RingAlloc((void **)head,size+8);
        tokens->length=size;
        MacroTokenBufferGet(tokens->buf);
        MacroTokenBufferInit();
    }
}
MACRO_ARG *collectParms(void)
{
    TOKEN_LIST *saved=NULL, *tokens;
    MACRO_ARG *parms=NULL;
    MEPTR entry;
    unsigned char tok, previous;
    unsigned required;
    int count, bracket, total, size;

    saveTokenBuffer(&saved);
    entry=fmentry;
    if (entry->parm_count) {
        required=entry->parm_count-1;
        if (entry->parm_count>1)
            parms=(MACRO_ARG *)CMemAlloc(required*sizeof(*parms));
        count=0;
        do { tok=(unsigned char)nextMToken(); } while (tok==0x8f);
        bracket=0;
        tokens=NULL;
        total=0;
        for (;;) {
            previous=tok;
            do {
                tok=(unsigned char)nextMToken();
                if (tok!=0x8f) break;
            } while (!MacroTokenBufferSize());
            if (tok==1) {
                CErr(79,entry->macro_name);
                return NULL;
            }
            if (tok==0x94 && BadTokenInfo==67) {
                CErr1(67);
                tok=8;
            }
            if (tok==7) ++bracket;
            else if (tok==8) {
                if (!bracket) break;
                --bracket;
            } else if (tok==2 && !bracket) {
                MacroTokenBufferRemoveWhiteSpace();
                saveParm(entry,count,parms,tokens,total);
                ++count;
                tokens=NULL;
                total=0;
                continue;
            }
            size=1;
            switch (tok) {
            case 0x8f: if (previous==0x8f) size=0; break;
            case 0x37: size=5; /* encoded constant has type and value */
            case 0x36: case 0x38: case 0x94: case 0xa9:
                size+=strlen(Buffer)+1;
                break;
            }
            if (MacroTokenBufferSize()+size>=512) {
                total+=MacroTokenBufferSize();
                saveTokenBuffer(&tokens);
            }
            switch (tok) {
            case 0x8f:
                if (previous!=0x8f) MacroTokenBufferAddChar(tok);
                break;
            case 0x90:
                MacroTokenBufferAddChar(tok);
                MacroTokenBufferAddChar((unsigned char)Buffer[0]);
                if (Buffer[1]) MacroTokenBufferAddChar(0x8f);
                break;
            case 0x37:
                MacroTokenBufferAddChar(tok);
                MacroTokenBufferAddConstant();
                break;
            case 0x36: case 0x38: case 0x94: case 0xa3: case 0xa9:
                MacroTokenBufferAddChar(tok);
                MacroTokenBufferAddString(Buffer);
                break;
            default: MacroTokenBufferAddChar(tok); break;
            }
        }
        MacroTokenBufferRemoveWhiteSpace();
        if (MacroTokenBufferSize()+total) {
            saveParm(entry,count,parms,tokens,total);
            ++count;
        }
        if (count<(int)required) {
            CErr(55,entry->macro_name);
            do {
                MacroTokenBufferAddChar(0x8f);
                saveParm(entry,count,parms,NULL,1);
                ++count;
            } while (count<entry->parm_count);
        } else if (count>(int)required) {
            CErr(60,entry->macro_name);
        } else if (!strcmp(entry->macro_name,"va_start")) {
            if (ScopeFunctionInProgress() && !CurrFunctionHasEllipsis())
                CErr1(93);
        }
    }
    if (saved) {
        MacroTokenBufferRestore(saved->buf,saved->length);
        CMemFree(saved);
    } else MacroTokenBufferInit();
    return parms;
}
void buildString(int length, char *tokens)
{ (void)length; (void)tokens; STUB_TRAP(); }
MACRO_TOKEN *buildMTokenList(char *tokens, MACRO_ARG *parms)
{
    MACRO_TOKEN *head=NULL, *tail=NULL, *token;
    unsigned char kind, previous=0, type;
    char buf[2];
    buf[1]='\0';
    if (!tokens) return NULL;
    while (*tokens) {
        kind=(unsigned char)*tokens;
        switch (kind) {
        case 0x37:
            type=(unsigned char)tokens[1];
            tokens+=2;
            if (type<11 || type>13) tokens+=4;
            token=buildAToken(tokens);
            token->token=0x37;
            while (*tokens++);
            break;
        case 0x36: case 0x94: case 0xa9: case 0x38: case 0xa3:
            token=buildAToken(tokens+1);
            token->token=kind;
            ++tokens;
            while (*tokens++);
            break;
        case 0x8f:
            ++tokens;
            if (previous==0x92) continue;
            token=buildAToken(" ");
            token->token=0x8f;
            break;
        case 0x90:
            buf[0]=tokens[1];
            tokens+=2;
            token=buildAToken(buf);
            token->token=0x90;
            break;
        case 0x93:
            ++tokens;
            while ((unsigned char)*tokens==0x8f) ++tokens;
            ++tokens; /* encoded macro-parameter token */
            if (parms) buildString(0,parms[(unsigned char)*tokens].arg);
            else Buffer[0]='\0';
            ++tokens;
            token=buildAToken(Buffer);
            token->token=0x38;
            break;
        case 0x7f:
            buf[0]=tokens[1];
            tokens+=2;
            token=buildAToken(buf);
            token->token=kind;
            break;
        default:
            /* Only encountered token spellings have been defined. */
            if (kind>=164 || !Tokens[kind]) STUB_TRAP();
            token=buildAToken(Tokens[kind]);
            token->token=kind;
            ++tokens;
            break;
        }
        if (!head) head=token;
        if (tail) tail->next=token;
        if (token) {
            if (token->token!=0x8f) previous=token->token;
            tail=token;
            while (tail->next) tail=tail->next;
        }
    }
    return head;
}
MACRO_TOKEN *substituteParms(MACRO_TOKEN *head, MACRO_ARG *parms)
{ (void)head; (void)parms; STUB_TRAP(); return NULL; }
MACRO_TOKEN *glue2Tokens(MACRO_TOKEN *first, MACRO_TOKEN *second)
{ (void)first; (void)second; STUB_TRAP(); return NULL; }
MACRO_TOKEN *glueTokens(MACRO_TOKEN *head)
{
    MACRO_TOKEN *token=head, *previous=NULL, *next, *replacement, *last;
    while (token) {
        if (token->token!=0x8f) {
            next=token->next;
            if (!next) break;
            if (next->token==0x8f) next=next->next;
            if (!next) break;
            if (next->token==0x92) {
                next=next->next;
                replacement=glue2Tokens(token,next);
                if (previous) previous->next=replacement;
                else head=replacement;
                last=replacement;
                while (last->next) { previous=last; last=last->next; }
                if (next) last->next=next->next;
                do {
                    next=token->next;
                    CMemFree(token);
                    token=next;
                } while (token!=last->next);
                token=last;
                continue;
            }
        }
        previous=token;
        token=token->next;
    }
    return head;
}
MACRO_TOKEN *appendToken(MACRO_TOKEN *head, int kind, char *text)
{
    MACRO_TOKEN *token=buildAToken(text), *tail;
    token->token=(unsigned char)kind;
    if (!head) head=token;
    else {
        tail=head;
        while (tail->next) tail=tail->next;
        tail->next=token;
    }
    return head;
}
int idLookup(unsigned length);
MACRO_TOKEN *nestedMacroExpansion(void);
void deleteNestedMacro(void)
{
    NESTED_MACRO *nested=nestedMacros;
    MEPTR entry;
    MACRO_ARG *parms;
    int count;
    if (nested) {
        nestedMacros=nested->next;
        entry=nested->fmentry;
        parms=nested->macro_parms;
        CMemFree(nested);
        count=entry->parm_count-1;
        if (count>0 && parms) {
            do { --count; CMemFree(parms[count].arg); } while (count);
            CMemFree(parms);
        }
    }
}
int macroBeingExpanded(MEPTR entry)
{
    NESTED_MACRO *nested=nestedMacros;
    while (nested) {
        if (nested->fmentry==entry) return 1;
        if (!nested->rescanning) break;
        nested=nested->next;
    }
    return 0;
}
int isExpandable(MACRO_TOKEN *token, int macro_parm)
{
    int bracket;
    MEPTR entry=fmentry;
    if (!entry->macro_defn) return 1;
    if (!entry->parm_count) {
        if (macro_parm && macroBeingExpanded(entry)) return 0;
        return 1;
    }
    while (token && (token->token==0x8f || !token->token)) token=token->next;
    if (token) {
        if (token->token==7) {
            if (macroDepth==1 && !macro_parm) return 1;
            bracket=0;
            for (;;) {
                token=token->next;
                if (!token) break;
                if (token->token==7) ++bracket;
                else if (token->token==8) {
                    if (!bracket) return 1;
                    --bracket;
                }
            }
        }
    } else if (!macro_parm) {
        SkipAhead();
        if (CurrChar=='(') return 1;
        if (CompFlags.cpp_output) return 2;
    }
    return 0;
}
MACRO_TOKEN *expandNestedMacros(MACRO_TOKEN *head, int rescanning)
{
    MACRO_TOKEN *token=head, *previous=NULL, *list, *tail, *old_tokens;
    unsigned length;
    int result;
    ++macroDepth;
    while (token) {
        list=NULL;
        if (token->token==0x36) {
            length=0;
            do { Buffer[length]=token->data[length]; } while (token->data[length++]);
            --length;
            if (idLookup(length)==0x80) {
                if (rescanning) {
                    if (macroBeingExpanded(fmentry)) token->token=0xa9;
                    else {
                        tail=token;
                        while (tail->next) tail=tail->next;
                        tail->next=internalTokenList;
                        result=isExpandable(token->next,0);
                        switch (result) {
                        case 0:
                            token->token=0x80;
                            tail->next=NULL;
                            break;
                        case 1:
                            internalTokenList=token->next;
                            if (head==token) { head=NULL; previous=NULL; }
                            CMemFree(token);
                            list=macroExpansion(rescanning);
                            token=internalTokenList;
                            internalTokenList=NULL;
                            break;
                        case 2:
                            token->token=0xa9;
                            tail->next=NULL;
                            tail=buildAToken(" ");
                            tail->token=0x8f;
                            tail->next=token->next;
                            token->next=tail;
                            break;
                        }
                    }
                } else if (isExpandable(token->next,1)) {
                    old_tokens=internalTokenList;
                    internalTokenList=token->next;
                    if (head==token) { head=NULL; previous=NULL; }
                    CMemFree(token);
                    list=nestedMacroExpansion();
                    token=internalTokenList;
                    internalTokenList=old_tokens;
                } else { previous=token; token=token->next; }
            } else {
                token->token=0x91;
                previous=token;
                token=token->next;
            }
        } else if (!token->token) {
            list=token->next;
            if (token->data[0]=='Z') {
                rescanning=nestedMacros->rescanning;
                deleteNestedMacro();
                CMemFree(token);
                token=list;
            } else {
                if (list) internalTokenList=list;
                CMemFree(token);
                token=NULL;
            }
            list=NULL;
        } else { previous=token; token=token->next; }
        if (list) {
            if (previous) previous->next=list;
            else head=list;
            if (token) {
                tail=list;
                while (tail->next) tail=tail->next;
                tail->next=token;
            }
            token=previous ? previous->next : head;
        } else {
            if (previous) previous->next=token;
            else head=token;
        }
    }
    for (token=head; token; token=token->next)
        if (token->token==0x80) token->token=0x36;
    --macroDepth;
    return head;
}
MACRO_TOKEN *nestedMacroExpansion(void)
{
    return expandNestedMacros(macroExpansion(0),1);
}
void DoMacroExpansion(void)
{
    macroDepth=0;
    internalTokenList=nestedMacroExpansion();
    macro_expansion_state=internalTokenList ? "" : NULL;
}

int KwLookup(unsigned length);
void ReScanInit(char *text);
int ReScanToken(void);
void GetMacroToken(int expanding)
{
    MACRO_TOKEN *token;
    unsigned length;
    CurToken=0;
    for (;;) {
        token=internalTokenList;
        if (!token) {
            macro_expansion_state=NULL;
            break;
        }
        internalTokenList=token->next;
        CurToken=token->token;
        length=0;
        do { Buffer[length]=token->data[length]; } while (token->data[length++]);
        --length;
        CMemFree(token);
        switch (CurToken) {
        case 0xa9:
            if (!expanding) CurToken=KwLookup(length);
            break;
        case 0x36: case 0x91:
            if (MacroSearch()) CurToken=0x36;
            else CurToken=KwLookup(length);
            break;
        case 0x37:
            ReScanInit(Buffer);
            ReScanToken();
            break;
        case 0x38: case 0xa3:
            TokenSavedLen=length+1;
            break;
        case 0:
            if (Buffer[0]=='Z') deleteNestedMacro();
            continue;
        }
        break;
    }
}

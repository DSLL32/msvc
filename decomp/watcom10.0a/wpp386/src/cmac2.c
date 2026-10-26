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

/* Initial plusplus/c/cmac2.c at6b9cb443 supplies parser terminology.
 * The1994 body uses the earlier fixed token buffer and a RINGNAMECTL;
 * later MacroOffsetAddMem/location/PP-number scan helpers are absent. */
#include <string.h>
#include "compiler.h"
#include "frontend.h"
#include "macro.h"
#include "scan.h"
#include "cmdline.h"
#include "pending.h"
#include "vstk.h"
#include "srcfile.h"
#include "cgio.h"
#include "tokenbuf.h"
#include <recon.h>

static RINGNAMECTL parm_names={0,1};
void Expecting(char *name);
void MustRecog(int token);
void MacroTokenBufferInit(void);
void MacroTokenBufferAddString(const char *name);
unsigned findMacroParm(void);
MEPTR grabTokens(unsigned count, int mode);
void RingNameFree(RINGNAMECTL *control);

MEPTR MacroScan(int mode)
{
    int count;
    MEPTR entry;
    SrcFileGuardStateSig();
    NextToken();
    if (CurToken!=0x36) { Expecting("identifier"); return NULL; }
    MacroTokenBufferInit();
    MacroTokenBufferAddString(Buffer);
    RingNameInit(&parm_names);
    if (CurrChar=='(') {
        if (!(mode&1)) { BadCmdLine(68); return NULL; }
        NextToken();
        NextToken();
        count=0;
        for (;;) {
            if (CurToken==8) break;
            if (CurToken!=0x36) { Expecting("identifier"); return NULL; }
            if (findMacroParm()) {
                CErr2p(102,Buffer);
            } else {
                RingNameAllocStr(&parm_names,Buffer);
                ++count;
                MacroTokenBufferAddString(Buffer);
            }
            NextToken();
            if (CurToken==8) break;
            if (CurToken==0) { CErr1(78); break; }
            MustRecog(2);
        }
    } else {
        count=-1;
    }
    entry=grabTokens(count+1,(unsigned char)mode);
    RingNameFree(&parm_names);
    return entry;
}

void __cdecl CErr(unsigned number, ...);
void Expecting(char *name)
{
    char *actual;
    if (CurToken==0x90) { Buffer[1]=0; actual=Buffer; }
    else if (CurToken==0x94 || (CurToken>=0x36 && CurToken<=0x38)) actual=Buffer;
    else {
        if ((unsigned)CurToken>=164 || !Tokens[CurToken]) { STUB_TRAP(); return; }
        actual=Tokens[CurToken];
    }
    CErr(28,name,actual);
}
void MustRecog(int token)
{
    if (token==CurToken) { NextToken(); return; }
    if ((unsigned)token>=164 || !Tokens[token]) { STUB_TRAP(); return; }
    Expecting(Tokens[token]);
    if (CurToken!=1) NextToken();
}
unsigned findMacroParm(void)
{
    return RingNameLookupPosition(&parm_names,Buffer);
}
MEPTR grabTokens(unsigned count, int mode)
{
    struct {
        MEDEFN entry;
        char name_tail[512];
    } temporary;
    MEPTR entry, result;
    char *name;
    unsigned mlen, size, name_len, parm_index;
    int previous, previous_non_ws;
    entry=&temporary.entry;
    size=MacroTokenBufferSize();
    name=MacroTokenBufferStart();
    name_len=strlen(name); /* The1994 body retains this length query. */
    (void)name_len;
    /* Original temporary header leaves these unused bytes uninitialized;
     * they are replaced by macroAdd. Give them defined zero values here. */
    entry->next_macro=NULL;
    entry->macro_len=0;
    entry->macro_flags=0;
    entry->parm_count=(unsigned char)count;
    strcpy(entry->macro_name,name);
    mlen=size+10;
    entry->macro_defn=(unsigned short)mlen;
    MacroOverflow(mlen,0);
    macroCopy(entry,MacroOffset,10);
    macroCopy(name,MacroOffset+10,size);
    result=NULL;
    previous=previous_non_ws=0;
    if (CurToken!=0) {
        do { CurToken=ScanToken(0); } while (CurToken==0x8f);
        if (mode&2) {
            switch (CurToken) {
            case 0: break;
            case 0x10: CurToken=0x0f; break;
            case 0x0f: case 0x2f: NextToken(); break;
            case 0x31: CurToken=0x2f; break;
            default: BadCmdLine(68); return result;
            }
        }
        if (CurToken==0x31) { CErr1(89); NextToken(); }
    }
    for (;;) {
        MacroTokenBufferInit();
        if (CurToken==0) break;
        switch (CurToken) {
        case 0x2f:
            if (count) CurToken=0x93;
            MacroTokenBufferAddChar(CurToken);
            break;
        case 0x31:
            CurToken=0x92;
            MacroTokenBufferAddChar(CurToken);
            break;
        case 0x8f:
            if (previous!=CurToken) MacroTokenBufferAddChar(CurToken);
            break;
        case 0x35: case 0x36:
            parm_index=findMacroParm();
            if (parm_index) {
                CurToken=0x7f;
                MacroTokenBufferAddChar(CurToken);
                MacroTokenBufferAddChar(parm_index-1);
                break;
            }
            /* fall through: a non-parameter retains its spelling. */
        case 0x38: case 0x94: case 0xa3:
            MacroTokenBufferAddChar(CurToken);
            MacroTokenBufferAddString(Buffer);
            break;
        case 0x90:
            MacroTokenBufferAddChar(CurToken);
            MacroTokenBufferAddChar((unsigned char)Buffer[0]);
            if (Buffer[1]) MacroTokenBufferAddChar(0x8f);
            break;
        case 0x37:
            MacroTokenBufferAddChar(CurToken);
            MacroTokenBufferAddConstant();
            break;
        default:
            MacroTokenBufferAddChar(CurToken);
            break;
        }
        if (CurToken!=0x8f) {
            if (previous_non_ws==0x93 && CurToken!=0x7f) CErr1(52);
            previous_non_ws=CurToken;
        }
        previous=CurToken;
        CurToken=ScanToken(0);
        size=MacroTokenBufferSize();
        MacroOverflow(mlen+size,mlen);
        macroCopy(MacroTokenBufferStart(),MacroOffset+mlen,size);
        mlen+=size;
        if (!(mode&1)) break;
    }
    if (previous==0x8f) --mlen;
    MacroOffset[mlen++]='\0';
    if (previous_non_ws==0x31) CErr1(89);
    entry->macro_len=(unsigned short)mlen;
    result=macroAdd(entry,mlen);
    if (!(mode&1)) {
        while (CurToken==0x8f) CurToken=ScanToken(0);
        if (CurToken!=0) BadCmdLine(68);
    }
    return result;
}

unsigned NestLevel, SkipLevel;
#pragma pack(1)
struct cpp_info {
    CGIO_LOCATION locn;
    unsigned char cpp_type, processing;
};
#pragma pack()
static struct cpp_info *pp_stack;
static VSTK_CTL vstkPp;

void DirectiveInit(void)
{
    NestLevel = 0;
    SkipLevel = 0;
    pp_stack = 0;
    VstkOpen(&vstkPp, 14, 8);
}

/*1994 ChkControl owns the directive loop only; scanNewline supplies the
 * source-location and break-key check that later source moved here. */
void CDefine(void);
void CInclude(void);
void CIfDef(void);
void CIfNDef(void);
void CIf(void);
void CElif(void);
void CElse(void);
void CEndif(void);
void CUnDef(void);
void CLine(void);
void CError(void);
void CPragma(void);
void CIdent(void);
void CUnknown(void);
void CSkip(void);

/* Actual1994 linear table, same preprocessor roles as released cmac2.c.
 * The final directive pointer is a mutable search sentinel, set to Buffer. */
typedef struct {
    char *directive;
    void (*samelevel)(void), (*skiplevel)(void);
} PPCTRL;
static PPCTRL controlTable[]={
    { "define", CDefine, CSkip }, { "include", CInclude, CSkip },
    { "ifdef", CIfDef, CIfDef }, { "ifndef", CIfNDef, CIfNDef },
    { "if", CIf, CIf }, { "elif", CElif, CElif },
    { "else", CElse, CElse }, { "endif", CEndif, CEndif },
    { "undef", CUnDef, CSkip }, { "line", CLine, CSkip },
    { "error", CError, CSkip }, { "pragma", CPragma, CSkip },
    { "ident", CIdent, CSkip }, { NULL, CUnknown, CSkip }
};
void flush2EOL(void);
void preProcStmt(void)
{
    PPCTRL *control;
    NextChar();
    NextToken();
    if (CurToken==0x36) {
        controlTable[13].directive=Buffer;
        for (control=controlTable; strcmp(control->directive,Buffer); ++control) {}
        if (NestLevel==SkipLevel) control->samelevel();
        else control->skiplevel();
    } else if (CurToken) CUnknown();
    flush2EOL();
}
void flush2EOL(void)
{
    while (CurToken!=0 && CurToken!=1) NextToken();
}
char *SrcFileNameCurrent(void);
unsigned SrcFileLine(void);
void EmitLineNL(unsigned line, char *name);
void SkipAhead(void);
void ChkControl(void)
{
    int lines_skipped=0;
    for (;;) {
        if (CompFlags.cpp_output) PrtChar('\n');
        NextChar();
        SkipAhead();
        if (CurrChar==256) break;
        PPState=2;
        if (CurrChar==PreProcChar) preProcStmt();
        else if (NestLevel!=SkipLevel) { NextToken(); flush2EOL(); }
        if (NestLevel==SkipLevel || CurrChar==256) break;
        if (CurrChar=='\n') lines_skipped=1;
    }
    PPState=0;
    if (CompFlags.cpp_output && lines_skipped)
        EmitLineNL(SrcFileLine()+1,SrcFileNameCurrent());
}

void CDefine(void)
{
    MEPTR entry=MacroScan(1);
    if (entry) entry->macro_flags |= 4;
}
void CInclude(void)
{
    char name[84];
    unsigned char in_macro;
    SrcFileGuardStateSig();
    InitialMacroFlag=0;
    in_macro=macro_expansion_state!=NULL;
    PPState=1;
    NextToken();
    if (CurToken==0x38) OpenSrcFile(Buffer,0);
    else if (CurToken==0x1f) {
        if (!in_macro) PPState=2;
        name[0]=0;
        for (;;) {
            NextToken();
            if (CurToken==0x1b) { OpenSrcFile(name,1); break; }
            strncat(name,Buffer,80);
            if ((in_macro && !macro_expansion_state) || !CurToken || CurToken==1) {
                CErr1(54);
                break;
            }
        }
    } else CErr1(54);
    if (CurToken!=1) NextToken();
}
void CIfDef(void) { STUB_TRAP(); }
void SrcFileGuardPpIf(void);
void SrcFileGuardPpIfndef(char *name);
int macroDependsDefined(char *name);
void ppIfId(int value);
void IncLevel(int value);
void CIfNDef(void)
{
    SrcFileGuardPpIf();
    NextToken();
    if (CurToken!=0x36) {
        Expecting("identifier");
        IncLevel(0);
        return;
    }
    SrcFileGuardPpIfndef(Buffer);
    ppIfId(!macroDependsDefined(Buffer));
}
int SrcFileGuardedIf(void);
void ChkEOL(void);
void ppIf(int value)
{
    if (!value && SrcFileGuardedIf()) return;
    IncLevel(value);
    ChkEOL();
}
void ppIfId(int value)
{
    NextToken();
    ppIf(value);
}
int MacroExists(char *name);
int macroDependsDefined(char *name)
{
    if (NestLevel==SkipLevel) return MacroExists(name);
    return 0;
}
void *VstkPush(VSTK_CTL *stack);
void SrcFileGetTokenLocn(CGIO_LOCATION *location);
void IncLevel(int value)
{
    struct cpp_info *info=(struct cpp_info *)VstkPush(&vstkPp);
    pp_stack=info;
    SrcFileGetTokenLocn(&info->locn);
    info->cpp_type=3;
    info->processing=0;
    if (NestLevel==SkipLevel && value) {
        ++SkipLevel;
        info->processing=1;
    }
    ++NestLevel;
}
void ChkEOL(void)
{
    if (CurToken!=0 && CurToken!=1) Expecting("end of line");
    else if (CompFlags.cpp_output) PrtChar('\n');
}
long PpConstExpr(void);
void CIf(void)
{
    int value;
    SrcFileGuardPpIf();
    PPState=1;
    NextToken();
    if (NestLevel!=SkipLevel) IncLevel(0);
    else {
        value=PpConstExpr();
        ppIf(value);
    }
}

void CElif(void) { STUB_TRAP(); }
void SrcFileGuardPpElse(void);
void SrcFileGuardPpEndif(void);
void *VstkTop(VSTK_CTL *stack);
void *VstkPop(VSTK_CTL *stack);
void wantEOL(void)
{
    if (!CompFlags.extensions_enabled) {
        if (CurToken && CurToken!=1) {
            CErr1(627);
            flush2EOL();
        }
    } else ChkEOL();
}
void CElse(void)
{
    SrcFileGuardPpElse();
    if (!NestLevel || pp_stack->cpp_type==1) CErr1(25);
    else {
        if (NestLevel==SkipLevel) {
            --SkipLevel;
            pp_stack->processing=0;
        } else if (NestLevel==SkipLevel+1 && pp_stack->cpp_type==3) {
            SkipLevel=NestLevel;
            pp_stack->processing=1;
        }
        pp_stack->cpp_type=1;
    }
    NextToken();
    wantEOL();
}
void CEndif(void)
{
    if (!NestLevel) CErr1(26);
    else {
        --NestLevel;
        VstkPop(&vstkPp);
        pp_stack=(struct cpp_info *)VstkTop(&vstkPp);
        SrcFileGuardPpEndif();
    }
    if (NestLevel<SkipLevel) SkipLevel=NestLevel;
    NextToken();
    wantEOL();
}
void CUnDef(void) { STUB_TRAP(); }
void CLine(void) { STUB_TRAP(); }
void CError(void) { STUB_TRAP(); }
void CPragma(void) { STUB_TRAP(); }
void CIdent(void) { STUB_TRAP(); }
void CUnknown(void) { STUB_TRAP(); }
/* Exact empty RET facet shared with findMacroParm. */
void CSkip(void) {}

void *VstkTop(VSTK_CTL *stack);
void *VstkNext(VSTK_CTL *stack, void *entry);
unsigned IfDepthInSrcFile(void)
{
    SRCFILE current=SrcFileCurrent();
    struct cpp_info *info;
    unsigned depth=0;
    for (info=(struct cpp_info *)VstkTop(&vstkPp);
         info && info->locn.src_file==current;
         info=(struct cpp_info *)VstkNext(&vstkPp,info)) ++depth;
    return depth;
}

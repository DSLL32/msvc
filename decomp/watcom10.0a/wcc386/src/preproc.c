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
* Description:  Preprocessor directive dispatch and conditional control.
*
****************************************************************************/


/* cmac2.c directive-control loop. The earlier void variant processes one
 * control run; ScanNewline owns the outer newline loop and break check. */
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "expression.h"
#include "scanner.h"
#include "macros.h"
#include "options.h"
#include "files.h"
#include "preproc.h"
#include <recon.h>

void ChkControl(void)
{
    int lines_skipped;
    lines_skipped=0;
    for (;;) {
        if (CompFlags.cpp_output) PrtChar('\n');
        NextChar();
        SkipAhead();
        if (CurrChar==256) break;
        CompFlags.pre_processing=2;
        if (CurrChar==PreProcChar) {
            PreProcStmt();
        } else if (NestLevel!=SkipLevel) {
            PPNextToken();
            Flush2EOL();
        }
        if (NestLevel==SkipLevel) break;
        if (CurrChar=='\n') lines_skipped=1;
        if (CurrChar==256) break;
    }
    CompFlags.pre_processing=0;
    if (CompFlags.cpp_output && lines_skipped && SrcFile!=NULL)
        EmitPoundLine(SrcFile->src_line+1,SrcFile->src_name);
}

/* Earlier linear table: conditional directives handle the skipped level
 * themselves; the later perfect-hash table/CSkipIf does not apply here. */
struct preproc {
    char *directive;
    void (*samelevel)(void),(*skipfunc)(void);
};
static struct preproc PreProcTable[]={
    {"define",CDefine,CSkip},
    {"include",CInclude,CSkip},
    {"ifdef",CIfDef,CIfDef},
    {"ifndef",CIfNDef,CIfNDef},
    {"if",CIf,CIf},
    {"elif",CElif,CElif},
    {"else",CElse,CElse},
    {"endif",CEndif,CEndif},
    {"undef",CUndef,CSkip},
    {"line",CLine,CSkip},
    {"error",CError,CSkip},
    {"pragma",CPragma,CSkip},
    {"ident",CIdent,CSkip},
    {NULL,CUnknown,CSkip}
};

/* Static exact empty-C3 proof: the entry shares ChkControl's RET byte.
 * It is seeded/excluded explicitly; no observed frontier is claimed. */
void CSkip(void)
{
}

void PreProcStmt(void)
{
    struct preproc *pp;
    NextChar();
    PPNextToken();
    if (CurToken==T_ID) {
        for (pp=PreProcTable;pp->directive;++pp)
            if (strcmp(pp->directive,Buffer)==0) break;
        if (NestLevel==SkipLevel) pp->samelevel();
        else pp->skipfunc();
    } else if (CurToken!=T_NULL) {
        CUnknown();
    }
    Flush2EOL();
}

void CInclude(void)
{
    char in_macro;
    char buffer[82];
    if (PCH_FileName!=NULL && !CompFlags.make_precompiled_header &&
        CompFlags.ok_to_use_precompiled_hdr) CompFlags.use_precompiled_header=1;
    InitialMacroFlag=0;
    in_macro=MacroPtr!=NULL;
    CompFlags.pre_processing=1;
    PPNextToken();
    if (CurToken==T_STRING) {
        OpenSrcFile(Buffer,0);
    } else if (CurToken==0x1e) { /* T_LT */
        if (!in_macro) CompFlags.pre_processing=2;
        buffer[0]='\0';
        for (;;) {
            PPNextToken();
            if (CurToken==0x1a) { /* T_GT */
                OpenSrcFile(buffer,'<');
                break;
            }
            strncat(buffer,Buffer,80);
            if ((in_macro && MacroPtr==NULL) || CurToken==T_NULL || CurToken==1) {
                CErr1(0x419);
                break;
            }
        }
    } else {
        CErr1(0x419);
    }
    if (CurToken!=1) PPNextToken();
    CompFlags.use_precompiled_header=0;
}

void Flush2EOL(void)
{
    while (CurToken!=T_NULL && CurToken!=1) PPNextToken();
}

/* The older dispatch reaches this routine at skipped nesting levels too. */
void CIfNDef(void)
{
    MEPTR mentry;
    int value;
    PPNextToken();
    if (CurToken!=T_ID) {
        ExpectIdentifier();
        IncLevel(0);
        return;
    }
    value=0;
    if (NestLevel==SkipLevel) {
        mentry=MacroLookup();
        if (mentry!=NULL) mentry->macro_flags|=8; /* MACRO_REFERENCED */
        else value=1;
    }
    IncLevel(value);
    PPNextToken();
    ChkEOL();
}

void IncLevel(int value)
{
    struct cpp_info *cpp;
    cpp=(struct cpp_info *)CMemAlloc(sizeof(struct cpp_info));
    cpp->cpp_type=0; /* PRE_IF */
    cpp->processing=0;
    cpp->prev_cpp=CppStack;
    cpp->file_name=ErrFName;
    cpp->line_num=TokenLine;
    CppStack=cpp;
    if (NestLevel==SkipLevel && value) {
        ++SkipLevel;
        cpp->processing=1;
    }
    ++NestLevel;
}

void CDefine(void)
{
    struct macro_parm *mp,*prev_mp,*formal_parms;
    int parm_cnt,i,j;
    PPNextToken();
    if (CurToken!=T_ID) {
        ExpectIdentifier();
        return;
    }
    if (strcmp(Buffer,"defined")==0) {
        CErr1(0x44e);
        return;
    }
    i=0;
    while ((TokenBuf[i]=Buffer[i])!=0) ++i;
    ++i;
    formal_parms=NULL;
    parm_cnt=-1;
    if (CurrChar=='(') {
        PPNextToken();
        PPNextToken();
        parm_cnt=0;
        prev_mp=NULL;
        for (;;) {
            if (CurToken==7) break; /* T_RIGHT_PAREN */
            if (CurToken!=T_ID) {
                ExpectIdentifier();
                return;
            }
            ++parm_cnt;
            mp=(struct macro_parm *)CMemAlloc(sizeof(struct macro_parm));
            if (formal_parms==NULL) formal_parms=mp;
            else {
                if (FormalParm(formal_parms)) CErr2p(0x46b,Buffer);
                prev_mp->next_macro_parm=mp;
            }
            mp->parm=CStrSave(Buffer);
            prev_mp=mp;
            j=0;
            while ((TokenBuf[i++]=Buffer[j++])!=0) ;
            PPNextToken();
            if (CurToken==7) break;
            if (CurToken==T_NULL) {
                CErr1(0x440);
                break;
            }
            MustRecog(2); /* T_COMMA */
        }
    }
    GrabTokens(parm_cnt+1,formal_parms,i);
    /* This older body frees the nodes only, unlike the 2002 source. */
    while ((mp=formal_parms)!=NULL) {
        formal_parms=mp->next_macro_parm;
        CMemFree(mp);
    }
}

/* Earlier numeric tokens contain both their binary value and spelling;
 * the later released preprocessor uses preprocessing-number text instead. */
void GrabTokens(int parm_cnt,struct macro_parm *formal_parms,int defn_offset)
{
    MEPTR mentry;
    int i,j,prev_token,prev_non_ws_token;
    unsigned mlen;
    i=defn_offset;
    j=strlen(TokenBuf);
    mentry=(MEPTR)CMemAlloc(offsetof(MEDEFN,macro_name)+1+j);
    mentry->parm_count=parm_cnt;
    strcpy(mentry->macro_name,TokenBuf);
    mlen=offsetof(MEDEFN,macro_name)+i;
    mentry->macro_defn=mlen;
    MacroOverflow(mlen,0);
    MacroCopy((char *)mentry,MacroOffset,offsetof(MEDEFN,macro_name));
    MacroCopy(TokenBuf,MacroOffset+offsetof(MEDEFN,macro_name),i);
    prev_token=T_NULL;
    prev_non_ws_token=T_NULL;
    if (CurToken!=T_NULL) {
        do { CurToken=ScanToken(); } while (CurToken==T_WHITE_SPACE);
        if (CurToken==0x30) {
            CErr1(0x44f);
            PPNextToken();
        }
    }
    for (;;) {
        i=0;
        if (CurToken==T_STRING && CompFlags.wide_char_string) CurToken=T_LSTRING;
        TokenBuf[i++]=CurToken;
        if (CurToken==T_NULL) break;
        switch (CurToken) {
        case 0x2e: /* T_SHARP */
            if (parm_cnt!=0) {
                TokenBuf[i-1]=0x7b;
                CurToken=0x7b; /* T_MACRO_SHARP */
            }
            break;
        case 0x30: /* T_SHARP_SHARP */
            TokenBuf[i-1]=0x79;
            break;
        case T_WHITE_SPACE:
            if (prev_token==T_WHITE_SPACE) --i;
            break;
        case T_ID:
            j=FormalParm(formal_parms);
            if (j!=0) {
                CurToken=0x65;
                TokenBuf[i-1]=0x65; /* T_MACRO_PARM */
                TokenBuf[i++]=j-1;
            } else {
                j=0;
                while ((TokenBuf[i++]=Buffer[j++])!=0) ;
            }
            break;
        case T_BAD_CHAR:
            TokenBuf[i++]=Buffer[0];
            if (Buffer[1]!=0) TokenBuf[i++]=T_WHITE_SPACE;
            break;
        case T_CONSTANT:
            i=SaveConstantToken(i);
            /* fall through: this version also retains the spelling. */
        case T_STRING:
        case T_LSTRING:
            j=0;
            while ((TokenBuf[i++]=Buffer[j++])!=0) ;
            break;
        }
        if (CurToken!=T_WHITE_SPACE) {
            if (prev_non_ws_token==0x7b && CurToken!=0x65) {
                CErr1(0x417);
                prev_token=(unsigned char)TokenBuf[0];
                TokenBuf[0]=0x2e;
                MacroCopy(TokenBuf,MacroOffset+mlen-1,1);
                TokenBuf[0]=prev_token;
            }
            prev_non_ws_token=CurToken;
        }
        prev_token=CurToken;
        CurToken=ScanToken();
        MacroOverflow(mlen+i,mlen);
        MacroCopy(TokenBuf,MacroOffset+mlen,i);
        mlen+=i;
    }
    if (prev_token==T_WHITE_SPACE) --mlen;
    MacroOverflow(mlen+1,mlen);
    MacroOffset[mlen++]=T_NULL;
    if (prev_non_ws_token==0x30) CErr1(0x44f);
    mentry->macro_len=mlen;
    MacLkAdd(mentry,mlen,4); /* MACRO_USER_DEFINED */
    CMemFree(mentry);
    MacroSize+=mlen;
}

int FormalParm(struct macro_parm *formal_parms)
{
    int i;
    i=1;
    while (formal_parms!=NULL) {
        if (strcmp(formal_parms->parm,Buffer)==0) return i;
        ++i;
        formal_parms=formal_parms->next_macro_parm;
    }
    return 0;
}

void ChkEOL(void)
{
    if (CurToken!=T_NULL && CurToken!=1) ExpectMessage(0x7d8);
}

void CIf(void)
{
    CompFlags.pre_processing=1;
    PPNextToken();
    if (NestLevel!=SkipLevel) IncLevel(0);
    else {
        IncLevel(GetConstExpr());
        ChkEOL();
    }
}

int GetConstExpr(void)
{
    int value,useful_side_effect,meaningless_stmt;
    useful_side_effect=CompFlags.useful_side_effect;
    meaningless_stmt=CompFlags.meaningless_stmt;
    value=BoolConstExpr();
    CompFlags.useful_side_effect=useful_side_effect;
    CompFlags.meaningless_stmt=meaningless_stmt;
    return value;
}

void CElse(void)
{
    if (NestLevel==0 || CppStack->cpp_type==2) {
        CErr1(0x3ee);
    } else {
        if (NestLevel==SkipLevel) {
            --SkipLevel;
            CppStack->processing=0;
        } else if (NestLevel==SkipLevel+1 && CppStack->cpp_type==0) {
            SkipLevel=NestLevel;
            CppStack->processing=1;
        }
        CppStack->cpp_type=2;
    }
    PPNextToken();
    WantEOL();
}

void WantEOL(void)
{
    if (CompFlags.extensions_enabled) {
        /* The old compiler warns here even at skipped nesting levels. */
        if (CurToken!=T_NULL && CurToken!=1) {
            CWarn1(0x1d,0x79);
            Flush2EOL();
        }
    } else ChkEOL();
}

void CEndif(void)
{
    struct cpp_info *cpp;
    if (NestLevel==0) CErr1(0x3ef);
    else {
        cpp=CppStack;
        --NestLevel;
        CppStack=cpp->prev_cpp;
        CMemFree(cpp);
    }
    if (NestLevel<SkipLevel) SkipLevel=NestLevel;
    PPNextToken();
    WantEOL();
}

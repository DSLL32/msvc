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
* Description:  Macro argument collection and expansion.
*
****************************************************************************/


#include <string.h>
#include "compiler.h"
#include "memory.h"
#include "macros.h"
#include "scanner.h"
#include "symbols.h"
#include "stubs.h"
#include <recon.h>

struct macro_arg *CollectParms(void)
{
    MEPTR mentry;
    int j,bracket,tok,prev_tok,parm_cnt,total;
    struct macro_arg *macro_parms;
    struct tokens **token_tail,*token_head;
    macro_parms=NULL;
    mentry=NextMacro;
    if (mentry->parm_count!=0) {
        if (mentry->parm_count>1)
            macro_parms=(struct macro_arg *)CMemAlloc((mentry->parm_count-1)*sizeof(struct macro_arg));
        parm_cnt=0;
        do { tok=NextMToken(); } while (tok==T_WHITE_SPACE);
        bracket=0;
        token_tail=&token_head;
        token_head=NULL;
        total=0;
        MTokenLen=0;
        for (;;) {
            prev_tok=tok;
            do {
                tok=NextMToken();
                if (tok!=T_WHITE_SPACE) break;
            } while (MTokenLen==0);
            if (tok==1) {
                CErr(0x441,mentry->macro_name);
                return NULL;
            }
            if (tok==0x7c && BadTokenInfo==0x42d) {
                CErr1(0x42d);
                tok=7;
            }
            if (tok==6) ++bracket;
            else if (tok==7) {
                if (bracket==0) break;
                --bracket;
            } else if (tok==2 && bracket==0) {
                if (prev_tok==T_WHITE_SPACE) --MTokenLen;
                SaveParm(mentry,MTokenLen,parm_cnt,macro_parms,token_head,total);
                ++parm_cnt;
                token_tail=&token_head;
                token_head=NULL;
                total=0;
                MTokenLen=0;
                continue;
            }
            j=1;
            switch (tok) {
            case T_WHITE_SPACE:
                if (prev_tok==T_WHITE_SPACE) j=0;
                break;
            case T_CONSTANT:
                j=6; /* token, type byte, binary DWORD, then spelling */
                /* fall through */
            case T_ID:
            case 0x8a: /* T_UNEXPANDABLE_ID */
            case T_STRING:
            case 0x7c: /* T_BAD_TOKEN */
                j+=strlen(Buffer)+1;
                break;
            }
            if (MTokenLen+j>=BUF_SIZE) {
                *token_tail=(struct tokens *)CMemAlloc(sizeof(struct tokens)+MTokenLen);
                (*token_tail)->length=MTokenLen;
                (*token_tail)->next=NULL;
                memcpy((*token_tail)->buf,TokenBuf,MTokenLen);
                token_tail=&(*token_tail)->next;
                total+=MTokenLen;
                MTokenLen=0;
            }
            if (tok==T_STRING && CompFlags.wide_char_string) tok=T_LSTRING;
            TokenBuf[MTokenLen++]=tok;
            switch (tok) {
            case T_WHITE_SPACE:
                if (prev_tok==T_WHITE_SPACE) --MTokenLen;
                break;
            case T_BAD_CHAR:
                TokenBuf[MTokenLen++]=Buffer[0];
                if (Buffer[1]!=0) TokenBuf[MTokenLen++]=T_WHITE_SPACE;
                break;
            case T_CONSTANT:
                MTokenLen=SaveConstantToken(MTokenLen);
                /* fall through */
            case T_ID:
            case 0x8a:
            case T_LSTRING:
            case T_STRING:
            case 0x7c:
                j=0;
                while ((TokenBuf[MTokenLen++]=Buffer[j++])!=0) ;
                break;
            }
        }
        if (prev_tok==T_WHITE_SPACE) --MTokenLen;
        if (MTokenLen+total!=0) {
            SaveParm(mentry,MTokenLen,parm_cnt,macro_parms,token_head,total);
            ++parm_cnt;
        }
        if (parm_cnt<mentry->parm_count-1) CErr(0x41a,mentry->macro_name);
        else if (parm_cnt>mentry->parm_count-1) CErr(0x420,mentry->macro_name);
        else if (far_strcmp(mentry->macro_name,"va_start",9)==0) {
            if (SymLevel!=0 && !VarParm(CurFunc)) CErr1(0x45d);
        }
    }
    return macro_parms;
}

struct macro_token *BuildMTokenList(char *p,struct macro_arg *macro_parms)
{
    struct macro_token *mtok,*head,*tail;
    struct nested_macros *nested;
    char *p2;
    char buf[2];
    int prev_token;
    head=NULL;
    tail=NULL;
    nested=NestedMacros;
    buf[1]='\0';
    prev_token=T_NULL;
    if (p==NULL) return NULL;
    while (*p!=T_NULL) {
        switch ((unsigned char)*p) {
        case T_CONSTANT:
            /* Skip the type and binary DWORD; keep the spelling. */
            p+=6;
            mtok=BuildAToken(p);
            mtok->token=T_CONSTANT;
            while (*p++) ;
            break;
        case T_ID:
        case 0x8a:
        case 0x7c:
        case T_LSTRING:
        case T_STRING:
            mtok=BuildAToken(p+1);
            mtok->token=*p++;
            while (*p++) ;
            break;
        case T_WHITE_SPACE:
            ++p;
            if (prev_token==0x79) continue;
            mtok=BuildAToken(" ");
            mtok->token=T_WHITE_SPACE;
            break;
        case T_BAD_CHAR:
            ++p;
            buf[0]=*p++;
            mtok=BuildAToken(buf);
            mtok->token=T_BAD_CHAR;
            break;
        case 0x7b: /* T_MACRO_SHARP */
            ++p;
            while (*p==T_WHITE_SPACE) ++p;
            ++p; /* T_MACRO_PARM */
            if (macro_parms!=NULL) BuildString(0,macro_parms[(unsigned char)*p].arg);
            else Buffer[0]='\0';
            ++p;
            mtok=BuildAToken(Buffer);
            mtok->token=T_STRING;
            break;
        case 0x65: /* T_MACRO_PARM */
            ++p;
            buf[0]=*p++;
            p2=p;
            while (*p2==T_WHITE_SPACE) ++p2;
            nested->substituting_parms=1;
            if (macro_parms!=NULL)
                mtok=BuildMTokenList(macro_parms[(unsigned char)buf[0]].arg,NULL);
            else {
                mtok=BuildAToken("");
                mtok->token=T_WHITE_SPACE;
            }
            if (*p2!=0x79 && prev_token!=0x79 && mtok!=NULL) {
                mtok=AppendToken(mtok,T_NULL,"P-<end of parm>");
                mtok=ExpandNestedMacros(mtok,0);
            }
            nested->substituting_parms=0;
            break;
        default:
            /* Only recovered semantic token data may be consumed. */
            if ((unsigned char)*p>=101) STUB_TRAP();
            mtok=BuildAToken(Tokens[(unsigned char)*p]);
            mtok->token=*p++;
            break;
        }
        if (head==NULL) head=mtok;
        if (tail!=NULL) tail->next=mtok;
        if (mtok!=NULL) {
            if (mtok->token!=T_WHITE_SPACE) prev_token=mtok->token;
            tail=mtok;
            while (tail->next!=NULL) tail=tail->next;
        }
    }
    head=GlueTokens(head);
    return head;
}

struct macro_token *BuildAToken(char *text)
{
    struct macro_token *mtok;
    char *p;
    mtok=(struct macro_token *)CMemAlloc(sizeof(struct macro_token)+TokLength(text));
    mtok->next=NULL;
    p=mtok->data;
    while ((*p=*text)!=0) {
        ++p;
        ++text;
    }
    return mtok;
}

int TokLength(char *text)
{
    int len;
    len=0;
    while (*text++) ++len;
    return len;
}

/* The older pasting operation is factored through a two-token helper. */
struct macro_token *GlueTokens(struct macro_token *head)
{
    struct macro_token *mtok,*prev,*next,*new_list,*last,*remove;
    mtok=head;
    prev=NULL;
    while (mtok!=NULL) {
        if (mtok->token!=T_WHITE_SPACE) {
            next=mtok->next;
            if (next==NULL) break;
            if (next->token==T_WHITE_SPACE) next=next->next;
            if (next==NULL) break;
            if (next->token==0x79) {
                next=next->next;
                new_list=Glue2Tokens(mtok,next);
                if (prev==NULL) head=new_list;
                else prev->next=new_list;
                last=new_list;
                while (last->next!=NULL) {
                    prev=last;
                    last=last->next;
                }
                if (next!=NULL) last->next=next->next;
                while (mtok!=last->next) {
                    remove=mtok;
                    mtok=mtok->next;
                    CMemFree(remove);
                }
                mtok=last;
                continue;
            }
        }
        prev=mtok;
        mtok=mtok->next;
    }
    return head;
}

struct macro_token *AppendToken(struct macro_token *head,int token,char *text)
{
    struct macro_token *tail,*new_token;
    new_token=BuildAToken(text);
    new_token->token=token;
    if (head==NULL) head=new_token;
    else {
        tail=head;
        while (tail->next!=NULL) tail=tail->next;
        tail->next=new_token;
    }
    return head;
}

struct macro_token *ExpandNestedMacros( struct macro_token *head, int rescanning )
{
    struct macro_token *mtok;
    struct macro_token *toklist;
    struct macro_token *prev_tok;
    struct macro_token *old_tokenlist;
    int         i;

    mtok = head;
    ++MacroDepth;
    prev_tok = NULL;
    for(;;) {
        if( mtok == NULL ) break;
        toklist = NULL;
        if( mtok->token == T_ID ) {
            // if macro and not being expanded, then expand it
            // only tokens available for expansion are those in mtok list
            i = 0;
            while( Buffer[i] = mtok->data[i] ) i++;
            CalcHash( Buffer, i );
            if( IdLookup() == T_MACRO ) {
                if( rescanning ) {
                    if( MacroBeingExpanded( NextMacro ) ) {
                        mtok->token = 0x8a /* T_UNEXPANDABLE_ID */;
                    } else {
                        toklist = mtok;
                        while( toklist->next != NULL ) {
                            toklist = toklist->next;
                        }
                        toklist->next = TokenList;
                        i = Expandable( mtok->next, 0 );
                        switch( i ) {
                        case 0:         // macro is currently not expandable
                            mtok->token = T_MACRO;
                            toklist->next = NULL;
                            toklist = NULL;
                            break;
                        case 1:         // macro is expandable
                            TokenList = mtok->next;
                            if( head == mtok ) {
                                head = NULL;
                                prev_tok = NULL;
                            }
                            CMemFree( mtok );
                            toklist = MacroExpansion( rescanning );
                            mtok = TokenList;
                            TokenList = NULL;
                            break;
                        case 2:         // we skipped over some white space
                            mtok->token = 0x8a /* T_UNEXPANDABLE_ID */;
                            toklist->next = NULL;
                            toklist = BuildAToken( " " );
                            toklist->token = T_WHITE_SPACE;
                            toklist->next = mtok->next;
                            mtok->next = toklist;
                            toklist = NULL;
                            break;
                        }
                    }
                } else {        // expanding a macro parm
                    if( Expandable( mtok->next, 1 ) ) {
                        old_tokenlist = TokenList;
                        TokenList = mtok->next;
                        if( head == mtok ) {
                            head = NULL;
                            prev_tok = NULL;
                        }
                        CMemFree( mtok );
                        toklist = NestedMacroExpansion();
                        mtok = TokenList;
                        TokenList = old_tokenlist;
                    } else {
                        prev_tok = mtok;
                        mtok = mtok->next;      // advance onto next token
                    }
                }
            } else {
                mtok->token = 0x78 /* T_SAVED_ID */;       // avoid rechecking this ID
                prev_tok = mtok;
                mtok = mtok->next;      // advance onto next token
            }
        } else if( mtok->token == T_NULL ) {
            toklist = mtok->next;
            if( mtok->data[0] == 'Z' ) {        // end of a macro
                rescanning = NestedMacros->rescanning;
                DeleteNestedMacro();
                CMemFree( mtok );
                mtok = toklist;
            } else {                            // end of a macro parm
                if( toklist != NULL ) {
                    TokenList = toklist;
                }
                CMemFree( mtok );
                mtok = NULL;                    // will cause us to exit
            }
            toklist = NULL;
        } else {                        // advance onto next token
            prev_tok = mtok;
            mtok = mtok->next;
        }
        if( toklist != NULL ) {         // new tokens to insert
            if( prev_tok == NULL ) {
                head = toklist;
            } else {
                prev_tok->next = toklist;
            }
            if( mtok != NULL ) {
                while( toklist->next != NULL )  toklist = toklist->next;
                toklist->next = mtok;
            }
            if( prev_tok == NULL ) {
                mtok = head;
            } else {
                mtok = prev_tok->next;
            }
        } else {
            // either no change, or tokens were deleted
            if( prev_tok == NULL ) {
                head = mtok;
            } else {
                prev_tok->next = mtok;
            }
        }
    }
    for( mtok = head; mtok; mtok = mtok->next ) {       /* 02-nov-93 */
        // change a temporarily unexpandable ID into an ID because it
        // could become expandable in a later rescanning phase
        if( mtok->token == T_MACRO )  mtok->token = T_ID;
    }
    --MacroDepth;
    return( head );
}

void DeleteNestedMacro(void)
{
    MEPTR mentry;
    struct nested_macros *nested;
    struct macro_arg *macro_parms;
    int i;
    nested=NestedMacros;
    if (nested!=NULL) {
        NestedMacros=nested->next;
        macro_parms=nested->macro_parms;
        mentry=nested->mentry;
        CMemFree(nested);
        i=mentry->parm_count-1;
        if (i>0 && macro_parms!=NULL) {
            do {
                --i;
                CMemFree(macro_parms[i].arg);
            } while (i!=0);
            CMemFree(macro_parms);
        }
    }
}

void GetMacroToken(void)
{
    struct macro_token *mtok;
    unsigned i;
    CurToken=T_NULL;
    for (;;) {
        mtok=TokenList;
        if (mtok==NULL) {
            MacroPtr=NULL;
            break;
        }
        TokenList=mtok->next;
        CurToken=mtok->token;
        i=0;
        while ((Buffer[i]=mtok->data[i])!=0) ++i;
        CMemFree(mtok);
        switch (CurToken) {
        case 0x8a: /* T_UNEXPANDABLE_ID */
            if (!CompFlags.doing_macro_expansion) {
                CalcHash(Buffer,i);
                CurToken=KwLookup();
            }
            break;
        case T_ID:
        case 0x78: /* T_SAVED_ID */
            CalcHash(Buffer,i);
            if (MacroLookup()!=NULL) CurToken=T_ID;
            else CurToken=KwLookup();
            break;
        case T_CONSTANT:
            ReScanInit(Buffer);
            ReScanToken();
            break;
        case T_LSTRING:
            CurToken=T_STRING;
            CompFlags.wide_char_string=1;
            CLitLength=i+1;
            break;
        case T_STRING:
            CompFlags.wide_char_string=0;
            CLitLength=i+1;
            break;
        case T_NULL:
            if (Buffer[0]=='Z') DeleteNestedMacro();
            continue;
        }
        break;
    }
}

int NextMToken(void)
{
    CompFlags.doing_macro_expansion=1;
    GetMacroToken();
    if (CurToken==T_NULL) CurToken=ScanToken();
    CompFlags.doing_macro_expansion=0;
    return CurToken;
}

void SaveParm(MEPTR mentry,int length,int count,struct macro_arg *parms,
              struct tokens *list,int total)
{
    struct tokens *next;
    char *text;
    TokenBuf[length++]=T_NULL;
    if (count<mentry->parm_count-1) {
        text=CMemAlloc(total+length);
        parms[count].arg=text;
        if (text!=NULL) {
            total=0;
            while (list!=NULL) {
                memcpy(text+total,list->buf,list->length);
                total+=list->length;
                next=list->next;
                CMemFree(list);
                list=next;
            }
            memcpy(text+total,TokenBuf,length);
        }
    }
}

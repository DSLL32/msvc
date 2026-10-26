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


/* Adapted initial6b9cb443 rewrite.c. Actual1994 CF45be2..45c28:
 *18-byte REWRITE,265-byte token blocks and8-byte source-file handles. */
#include "compiler.h"
#include "carve.h"
#include "rewrite.h"
#include "srcfile.h"
#include "pending.h"
#include "scan.h"
#include "macro.h"
#include "types.h"
#include "ptree.h"
#include <recon.h>
#pragma pack(1)
typedef struct rewrite_tokens {
    struct rewrite_tokens *next;
    unsigned count;
    unsigned char free:1,reserved:7;
    unsigned char stream[256];
} REWRITE_TOKENS;
typedef struct srcfile_handle {
    struct srcfile_handle *next;
    SRCFILE srcfile;
} SRCFILE_HANDLE;
struct rewrite_package {
    REWRITE_TOKENS *list,*curr;
    SRCFILE_HANDLE *srcfiles_refd;
    unsigned char *token;
    unsigned char last_token;
    unsigned char busy:1,alternate:1,free:1,reserved:5;
};
#pragma pack()
typedef char RewriteSize[(sizeof(REWRITE)==18 && sizeof(REWRITE_TOKENS)==265)?1:-1];
static REWRITE *currRewrite;
static carve_t carveREWRITE,carveREWRITE_TOKENS,carveSRCFILE_HANDLE;
void rewriteInit(INITFINI *defn)
{
    (void)defn;
    currRewrite=0;
    carveREWRITE=CarveCreate(18,8);
    carveREWRITE_TOKENS=CarveCreate(265,8);
    carveSRCFILE_HANDLE=CarveCreate(8,8);
}

REWRITE_TOKENS *tokenAlloc(void)
{
    REWRITE_TOKENS *tokens=CarveAlloc(carveREWRITE_TOKENS);
    tokens->next=0;
    tokens->count=0;
    tokens->free=0;
    /* Seven unused allocator bits are uninitialized in the original. */
    tokens->reserved=0;
    return tokens;
}
void putByte(REWRITE *rewrite,unsigned char value)
{
    REWRITE_TOKENS *tokens=rewrite->curr;
    if (tokens->count==256) {
        tokens->next=tokenAlloc();
        tokens=tokens->next;
        rewrite->curr=tokens;
    }
    tokens->stream[tokens->count++]=value;
}
void putBinary(REWRITE *rewrite,unsigned char *bytes,unsigned length)
{
    while (length) {
        putByte(rewrite,*bytes);
        --length;
        ++bytes;
    }
}
unsigned newSrcFileHandle(REWRITE *rewrite,SRCFILE srcfile)
{
    unsigned index=0;
    SRCFILE_HANDLE *entry,**head=&rewrite->srcfiles_refd;
    while ((entry=*head)!=0) {
        if (entry->srcfile==srcfile) return index;
        head=&entry->next;
        ++index;
    }
    entry=CarveAlloc(carveSRCFILE_HANDLE);
    entry->next=0;
    entry->srcfile=srcfile;
    *head=entry;
    return index;
}
void putSrcFile(REWRITE *rewrite,CGIO_LOCATION *location)
{
    unsigned index=newSrcFileHandle(rewrite,location->src_file);
    putByte(rewrite,0);
    putBinary(rewrite,(unsigned char *)&index,sizeof(index));
    location->line=0;
    location->column=0;
}
REWRITE *newREWRITE(int end_token,CGIO_LOCATION *location)
{
    REWRITE *rewrite=CarveAlloc(carveREWRITE);
    REWRITE_TOKENS *tokens=tokenAlloc();
    rewrite->list=tokens;
    rewrite->curr=tokens;
    rewrite->srcfiles_refd=0;
    rewrite->token=tokens->stream;
    rewrite->last_token=end_token;
    rewrite->busy=0;
    rewrite->free=0;
    rewrite->alternate=0;
    /* Five unused bits are uninitialized in the original. */
    rewrite->reserved=0;
    location->src_file=SrcFileCurrent();
    putSrcFile(rewrite,location);
    return rewrite;
}
REWRITE *RewriteRecordInit(CGIO_LOCATION *location)
{
    return newREWRITE(1,location); /* old T_EOF */
}
void putString(REWRITE *rewrite,char *text)
{
    while (*text) putByte(rewrite,*text++);
    putByte(rewrite,0);
}
void putSrcLocn(REWRITE *rewrite,CGIO_LOCATION *location)
{
    SRCFILE current=SrcFileCurrent();
    unsigned absolute,delta;
    if (current!=location->src_file) {
        location->src_file=current;
        putSrcFile(rewrite,location);
    }
    delta=TokenLine-location->line;
    if (delta>127) {
        putByte(rewrite,0x80);
        absolute=TokenLine|0x80000000UL;
        putBinary(rewrite,(unsigned char *)&absolute,sizeof(absolute));
        location->line=TokenLine;
    } else if (delta) {
        putByte(rewrite,delta|0x80);
        location->line=TokenLine;
    }
    delta=TokenColumn-location->column;
    if (delta>127) {
        putByte(rewrite,0x80);
        absolute=TokenColumn;
        putBinary(rewrite,(unsigned char *)&absolute,sizeof(absolute));
    } else if (delta) putByte(rewrite,delta);
    else {
        putByte(rewrite,0x80);
        absolute=TokenColumn;
        putBinary(rewrite,(unsigned char *)&absolute,sizeof(absolute));
    }
    location->column=TokenColumn;
}
void saveToken(REWRITE *rewrite,CGIO_LOCATION *location)
{
    putSrcLocn(rewrite,location);
    switch (CurToken) {
    case 54: case 56: case 163: /* ID, string, wide string */
        putByte(rewrite,CurToken);
        putString(rewrite,Buffer);
        break;
    case 145: /* saved identifier */
        putByte(rewrite,54);
        putString(rewrite,SavedId);
        break;
    case 55: /* constant */
        putByte(rewrite,CurToken);
        putByte(rewrite,ConstType);
        /* Actual old format saves only float/double as text; integral data
         * is four bytes, before the released source's64-bit constants. */
        if (ConstType==TYP_FLOAT || ConstType==TYP_DOUBLE) putString(rewrite,Buffer);
        else putBinary(rewrite,(unsigned char *)&Constant,sizeof(Constant));
        break;
    default: putByte(rewrite,CurToken); break;
    }
}
void RewriteRecordToken(REWRITE *rewrite,CGIO_LOCATION *location)
{
    if (CurToken!=1) saveToken(rewrite,location);
}

void SrcFileGetTokenLocn(CGIO_LOCATION *location);
void NextToken(void);
void Expecting(char *name);
REWRITE *templateError(REWRITE *rewrite,CGIO_LOCATION *location)
{ (void)rewrite; (void)location; STUB_TRAP(); return 0; }
REWRITE *RewritePackageClassTemplate(REWRITE *rewrite,CGIO_LOCATION *location)
{
    unsigned brace_depth=0;
    int first_time=1;
    CGIO_LOCATION start;
    SrcFileGetTokenLocn(&start);
    for (;;) {
        if (CurToken==1) {
            templateError(rewrite,&start);
            break;
        }
        if (!first_time) saveToken(rewrite,location);
        first_time=0;
        switch (CurToken) {
        case 12: /* right brace */
            if (!brace_depth) return templateError(rewrite,&start);
            --brace_depth;
            if (!brace_depth) {
                NextToken();
                if (CurToken!=5) {
                    Expecting(Tokens[5]);
                    return templateError(rewrite,&start);
                }
                return rewrite;
            }
            break;
        case 11: ++brace_depth; break;
        }
        NextToken();
    }
    return 0;
}

void RewriteFree(REWRITE *rewrite)
{
    REWRITE_TOKENS *tokens,*next_tokens;
    SRCFILE_HANDLE *handle,*next_handle;
    if (!rewrite) return;
    if (!rewrite->alternate) {
        for (tokens=rewrite->list;tokens;tokens=next_tokens) {
            next_tokens=tokens->next;
            CarveFree(carveREWRITE_TOKENS,tokens);
        }
        for (handle=rewrite->srcfiles_refd;handle;handle=next_handle) {
            next_handle=handle->next;
            CarveFree(carveSRCFILE_HANDLE,handle);
        }
    }
    CarveFree(carveREWRITE,rewrite);
}

static CGIO_LOCATION *currLocn;
PTREE recoverToken(PTREE tree)
{
    switch (tree->op) {
    case 2: case 3: case 4: /* old constant-node forms */
        /* As in the original, scanner Buffer/ConstType still retain the
         * token; correct C++ does not need these recovery alternatives. */
        CurToken=55;
        break;
    case 6: /* PT_ID */
        CurToken=145;
        SavedId=tree->u.id.name;
        break;
    case 8: case 9: /* PT_UNARY / PT_BINARY */
        switch (tree->cgop) {
        case 40: CurToken=41; break; /* CO_INDIRECT / T_TIMES */
        case 55: CurToken=6; break;  /* CO_COLON_COLON */
        case 54: CurToken=60; break; /* CO_OPERATOR */
        case 11: CurToken=14; break; /* CO_TILDE */
        case 63: CurToken=74; break; /* CO_NEW */
        case 58: CurToken=109; break; /* CO_DELETE */
        default: return tree; /* old release body ignores storage/other nodes */
        }
        break;
    default: return tree;
    }
    saveToken(currRewrite,currLocn);
    return tree;
}
void captureMulti(REWRITE *rewrite,PTREE multi,CGIO_LOCATION *location)
{
    unsigned save_token;
    char *save_id;
    REWRITE *save_rewrite;
    CGIO_LOCATION *save_location;
    if (!multi) return;
    save_token=CurToken;
    save_id=SavedId;
    save_rewrite=currRewrite;
    save_location=currLocn;
    currLocn=location;
    currRewrite=rewrite;
    multi=PTreeTraversePostfix(multi,recoverToken);
    PTreeFreeSubtrees(multi);
    currRewrite=save_rewrite;
    CurToken=save_token;
    currLocn=save_location;
    SavedId=save_id;
    NextToken();
}
REWRITE *ParseGetRecordingInProgress(CGIO_LOCATION **location);
REWRITE *RewritePackageFunction(PTREE multi)
{
    CGIO_LOCATION location,*record_location;
    REWRITE *rewrite=ParseGetRecordingInProgress(&record_location);
    unsigned depth=1;
    int skip_first=0;
    if (!rewrite) {
        record_location=&location;
        rewrite=newREWRITE(12,&location);
    } else skip_first=1;
    captureMulti(rewrite,multi,record_location);
    while (CurToken!=1) {
        if (CurToken==11) ++depth;
        else if (CurToken==12) --depth;
        if (!skip_first) saveToken(rewrite,record_location);
        skip_first=0;
        if (!depth) break;
        NextToken();
    }
    return rewrite;
}

REWRITE *memInitError(REWRITE *rewrite,CGIO_LOCATION *start)
{
    CErr1(585);
    CErr2p(680,(const char *)start);
    RewriteFree(rewrite);
    return NULL;
}
REWRITE *RewritePackageMemInit(PTREE multi)
{
    REWRITE *rewrite;
    unsigned parens=0,braces=0;
    CGIO_LOCATION location,start;
    SrcFileGetTokenLocn(&start);
    rewrite=newREWRITE(12,&location);
    captureMulti(rewrite,multi,&location);
    for (;;) {
        if (CurToken==1) {
            memInitError(rewrite,&start);
            break;
        }
        saveToken(rewrite,&location);
        switch (CurToken) {
        case 7: ++parens; break;
        case 8:
            if (!parens) return memInitError(rewrite,&start);
            --parens;
            break;
        case 12:
            if (!braces) return memInitError(rewrite,&start);
            --braces;
            break;
        case 11:
            if (!parens) return rewrite;
            ++braces;
            break;
        }
        NextToken();
    }
    return NULL;
}

REWRITE *dupREWRITE(REWRITE *old_rewrite)
{
    REWRITE *rewrite=CarveAlloc(carveREWRITE);
    rewrite->list=old_rewrite->list;
    rewrite->curr=old_rewrite->curr;
    rewrite->srcfiles_refd=old_rewrite->srcfiles_refd;
    rewrite->token=old_rewrite->token;
    rewrite->last_token=old_rewrite->last_token;
    rewrite->busy=0;
    rewrite->alternate=1;
    rewrite->free=0;
    return rewrite;
}

REWRITE *RewriteRewind(REWRITE *rewrite)
{
    REWRITE *previous=currRewrite;
    if (rewrite->busy) rewrite=dupREWRITE(rewrite);
    currRewrite=rewrite;
    rewrite->busy=1;
    rewrite->curr=rewrite->list;
    rewrite->token=rewrite->curr->stream;
    RewriteToken();
    return previous;
}
SRCFILE accessSrcFile(REWRITE *rewrite,unsigned index)
{
    SRCFILE_HANDLE *handle;
    for (handle=rewrite->srcfiles_refd;handle;handle=handle->next) {
        if (!index) return handle->srcfile;
        --index;
    }
    return NULL;
}
unsigned char getByte(REWRITE *rewrite,REWRITE_TOKENS **block,unsigned char **stop)
{
    REWRITE_TOKENS *next;
    if (rewrite->token!=*stop) return *rewrite->token++;
    next=(*block)->next;
    if (!next) return rewrite->last_token;
    *block=next;
    rewrite->curr=next;
    *stop=next->stream+next->count;
    rewrite->token=next->stream;
    return *rewrite->token++;
}
unsigned getString(REWRITE *rewrite,REWRITE_TOKENS **block,unsigned char **stop,char *dest)
{
    unsigned length=0;
    unsigned char ch;
    for (;;) {
        ch=getByte(rewrite,block,stop);
        if (!ch) break;
        *dest++=ch;
        ++length;
    }
    *dest=0;
    return length;
}
void getBinary(REWRITE *rewrite,REWRITE_TOKENS **block,unsigned char **stop,
    unsigned char *dest,unsigned size)
{
    for (;size;--size) *dest++=getByte(rewrite,block,stop);
}
void set_srcFile(SRCFILE file);
void RewriteToken(void)
{
    REWRITE *rewrite=currRewrite;
    REWRITE_TOKENS *block=rewrite->curr;
    unsigned char *stop=block->stream+block->count;
    unsigned char code,token;
    unsigned index,absolute,length;
    for (;;) {
        code=getByte(rewrite,&block,&stop);
        if (code==0) {
            getBinary(rewrite,&block,&stop,(unsigned char *)&index,sizeof(index));
            set_srcFile(accessSrcFile(rewrite,index));
            TokenLine=0;
            TokenColumn=0;
        } else if (code==0x80) {
            getBinary(rewrite,&block,&stop,(unsigned char *)&absolute,sizeof(absolute));
            if (absolute&0x80000000UL) {
                TokenLine=absolute&0x7fffffffUL;
                TokenColumn=0;
            } else {
                TokenColumn=absolute;
                break;
            }
        } else if (code&0x80) TokenLine+=code&0x7f;
        else { TokenColumn+=code; break; }
    }
    token=getByte(rewrite,&block,&stop);
    switch (token) {
    case 54: /*1994 identifier: getString does not update TokenLen. */
        getString(rewrite,&block,&stop,Buffer);
        break;
    case 56: case 163:
        length=getString(rewrite,&block,&stop,Buffer);
        TokenLen=length+1;
        break;
    case 55:
        ConstType=getByte(rewrite,&block,&stop);
        if (ConstType==TYP_FLOAT || ConstType==TYP_DOUBLE)
            getString(rewrite,&block,&stop,Buffer);
        else getBinary(rewrite,&block,&stop,(unsigned char *)&Constant,sizeof(Constant));
        break;
    }
    CurToken=token;
}
void RewriteClose(REWRITE *rewrite)
{
    REWRITE *current=currRewrite;
    if (current && current->alternate) RewriteFree(current);
    currRewrite=rewrite;
}
REWRITE *RewritePackageToken(void)
{
    CGIO_LOCATION location;
    REWRITE *rewrite=newREWRITE(1,&location);
    if (CurToken!=1) {
        if (CurToken==145) {
            saveToken(rewrite,&location);
            NextToken();
        }
        saveToken(rewrite,&location);
    }
    return rewrite;
}
void LookPastName(void);
void RewriteRestoreToken(REWRITE *rewrite)
{
    TOKEN_SOURCE source=SetTokenSource(RewriteToken);
    REWRITE *previous=RewriteRewind(rewrite);
    if (CurToken==54) {
        LookPastName();
        if (LAToken==1) CurToken=54;
    }
    RewriteClose(previous);
    ResetTokenSource(source);
    RewriteFree(rewrite);
}

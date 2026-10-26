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


#include <io.h>
#include <string.h>
#include <errno.h>
#include "scanner.h"
#include "files.h"
#include "command.h"
#include "options.h"
#include "quads.h"
#include <recon.h>
int LastChar;
int GetCharCheck(int c)
{
    int nc,nnc,xc;
    if (NextChar==ReScanBuffer) {
        if (c=='\0') CompFlags.rescan_buffer_done=1;
        return c;
    }
    if (c!='?') {
        switch (c) {
        case '\0':
            CurrChar=0;
            if (ScanCharPtr==SrcFile->src_buf+SrcFile->src_cnt+1) {
                if (!ReadBuffer(SrcFile)) return GetNextChar();
            }
            return CurrChar;
        case '\n':
            ++SrcFile->src_line;
            SrcFileLineNum=SrcFile->src_line;
            break;
        case '\\':
            CurrChar=c;
            return tryBackSlashNewLine();
        }
        CurrChar=c;
        return c;
    }
    nc=getTestCharFromFile();
    if (nc!='?') {
        LastChar=nc;
        CurrChar=c;
        NextChar=getCharAfterOneQuestion;
        return c;
    }
    nnc=getTestCharFromFile();
    xc=translateTriGraph((unsigned char)nnc);
    if (nnc!=xc) {
        CurrChar=xc;
        if (xc=='\\') return tryBackSlashNewLine();
        return xc;
    }
    LastChar=nnc;
    CurrChar=c;
    if (nnc=='?') NextChar=tryTrigraphAgain;
    else NextChar=getCharAfterTwoQuestion;
    return c;
}

int GetNextChar(void)
{
    int c;
    c=(unsigned char)*ScanCharPtr++;
    if ((CharSet[c]&C_EX)==0) {
        CurrChar=c;
        return c;
    }
    return GetCharCheck(c);
}

/* The old reader has no later eof()/synthetic final-newline extension.
 * It reads directly into the App-owned scanner buffer via the descriptor. */
void CClose(FILE *stream)
{
    if (stream!=NULL && stream!=stdin) fclose(stream);
}

void CloseSrcFile(FCB *file)
{
    if (file->src_fp!=NULL) CClose(file->src_fp);
    FEfree(file->src_buf);
    --file->src_line;
    if (CompFlags.scanning_comment) CErr1(0x416);
    SrcFile=file->prev_file;
    CurrChar=file->prev_currchar;
    if (SrcFile==MainSrcFile && CompFlags.make_precompiled_header) {
        CompFlags.make_precompiled_header=0;
        if (ErrCount==0 && QuadIndex==0) BuildPreCompiledHeader(PCH_FileName);
    }
    if (SrcFile!=NULL) {
        ScanCharPtr=SrcFile->src_ptr;
        TokenFno=SrcFile->src_fno;
        ErrFName=SrcFile->src_name;
        IncLineCount+=file->src_line;
        if (CompFlags.cpp_output) EmitPoundLine(SrcFile->src_line+1,SrcFile->src_name);
    } else {
        CurrChar=0x100;
        SrcLineCount=file->src_line;
    }
    CMemFree(file);
}

int ReadBuffer(FCB *file)
{
    if (file->src_fp==NULL) {
        CloseSrcFile(file);
        return 0;
    }
    do {
        file->src_ptr=file->src_buf;
        file->src_cnt=read(fileno(file->src_fp),file->src_ptr,file->src_bufsize);
        if (file->src_cnt==0) {
            CloseSrcFile(file);
            return 1;
        } else if (file->src_cnt==-1) {
            CErr(0x487,file->src_name,strerror(errno));
            CloseSrcFile(file);
            return 1;
        }
    } while (file->src_cnt<0);
    file->src_ptr[file->src_cnt]='\0';
    ScanCharPtr=file->src_ptr;
    return 0;
}

int getTestCharFromFile(void)
{
    int c;
    for (;;) {
        c=(unsigned char)*ScanCharPtr++;
        if (c!='\0') break;
        if (ScanCharPtr!=SrcFile->src_buf+SrcFile->src_cnt+1) break;
        if (ReadBuffer(SrcFile)) return CurrChar;
    }
    return c;
}

int getCharAfterOneQuestion(void)
{
    int c;
    c=LastChar;
    NextChar=GetNextChar;
    if (c!='?') return GetCharCheck(c);
    CurrChar=c;
    return c;
}

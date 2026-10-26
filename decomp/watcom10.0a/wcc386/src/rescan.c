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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include "scanner.h"
#include <recon.h>
int CurToken,TokenLen;
char *ReScanPtr;
void ReScanInit(char *text)
{
    ReScanPtr=text;
}
char *ScanCharPtr;
int ReScanToken(void)
{
    int saved_currchar;
    char *saved_ptr;
    int (*saved_nextchar)();
    saved_currchar=CurrChar;
    saved_nextchar=NextChar;
    saved_ptr=ScanCharPtr;
    ScanCharPtr=ReScanPtr;
    CompFlags.rescan_buffer_done=0;
    NextChar=ReScanBuffer;
    CurrChar=ReScanBuffer();
    CompFlags.doing_macro_expansion=1;
    CurToken=ScanToken();
    CompFlags.doing_macro_expansion=0;
    if (CurToken==T_STRING && CompFlags.wide_char_string) CurToken=T_LSTRING;
    --ScanCharPtr;
    ReScanPtr=ScanCharPtr;
    ScanCharPtr=saved_ptr;
    CurrChar=saved_currchar;
    NextChar=saved_nextchar;
    return CompFlags.rescan_buffer_done==0;
}

int ReScanBuffer(void)
{
    int c;
    c=(unsigned char)*ScanCharPtr++;
    if (c==0) CompFlags.rescan_buffer_done=1;
    CurrChar=c;
    return c;
}

char *ReScanPos(void)
{
    return ReScanPtr;
}

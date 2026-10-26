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

/* The encountered1994 fixed token buffer predates the released linked
 * BUFFER_HDR representation. Its512-byte storage and cursor are separate
 * actual owners (originalbdc48..bde48 andbde48). */
#include <string.h>
#include "utility.h"
#include "macro.h"
#include "scan.h"
#include "tokenbuf.h"
#include "pending.h"
#include <recon.h>

static char tokenBuffer[512];
static char *tokenCursor;

void MacroTokenBufferInit(void)
{
    tokenCursor=tokenBuffer;
}

void MacroTokenBufferAddString(const char *name)
{
    tokenCursor=stpcpy(tokenCursor,name)+1;
}

unsigned MacroTokenBufferSize(void)
{
    return (unsigned)(tokenCursor-tokenBuffer);
}

char *MacroTokenBufferStart(void)
{
    return tokenBuffer;
}

void MacroTokenBufferAddChar(int token)
{
    *tokenCursor++=(char)token;
}
void MacroTokenBufferAddConstant(void)
{
    MacroTokenBufferAddChar(ConstType);
    if (ConstType<11 || ConstType>13) MacroTokenBufferAddValue(Constant);
    MacroTokenBufferAddString(Buffer);
}

void MacroTokenBufferAddValue(unsigned long value)
{
    memcpy(tokenCursor,&value,4);
    tokenCursor+=4;
}

void MacroTokenBufferGet(char *target)
{
    memcpy(target,tokenBuffer,MacroTokenBufferSize());
    MacroTokenBufferInit();
}
void MacroTokenBufferRestore(char *source, unsigned size)
{
    memcpy(tokenBuffer,source,size);
    tokenCursor=tokenBuffer+size;
}
void MacroTokenBufferRemoveWhiteSpace(void)
{
    if (tokenCursor>tokenBuffer) {
        do {
            --tokenCursor;
        } while (*tokenCursor==(char)0x8f && tokenCursor>tokenBuffer);
        ++tokenCursor;
    }
}

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

/* Adapted from bld/fe_misc/c/vbuf.c at initial commit
 *6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; actual body21B. */
#include <string.h>
#include "compiler.h"
#include "utility.h"
#include <recon.h>

void VbufInit(VBUF *buffer)
{
    buffer->buf = 0;
    buffer->len = 0;
    buffer->used = 0;
}

/* Same initial-source ownership/growth contract as fe_misc/c/vbuf.c.
 * The 1994 buffer has three 32-bit fields and grows in 32-byte units. */
void VbufReqd(VBUF *buffer,unsigned required)
{
    char *old;
    if (required>buffer->len) {
        required=(required+31)&~31U;
        old=buffer->buf;
        buffer->buf=(char *)CMemAlloc(required);
        buffer->len=required;
        if (old) {
            memcpy(buffer->buf,old,buffer->used);
            CMemFree(old);
        }
    }
}
void VStrNull(VBUF *buffer)
{
    VbufReqd(buffer,1);
    buffer->buf[0]='\0';
    buffer->used=1;
}
void VStrConcStr(VBUF *buffer,const char *text)
{
    unsigned count=strlen(text);
    unsigned required=buffer->used+count;
    VbufReqd(buffer,required);
    stvcpy(buffer->buf+buffer->used-1,text,count);
    buffer->used=required;
}

/* 1994 uses the original two-byte temporary, unlike the later fast path. */
void VStrConcChr(VBUF *buffer,char chr)
{
    char text[2];
    text[0]=chr;
    text[1]='\0';
    VStrConcStr(buffer,text);
}

void VbufFree(VBUF *buffer)
{
    CMemFree(buffer->buf);
    VbufInit(buffer);
}
void VStrPrepStr(VBUF *buffer,const char *text)
{
    unsigned count=strlen(text);
    unsigned required=count+buffer->used;
    VBUF temp;
    VbufInit(&temp);
    VbufReqd(&temp,required);
    stvcpy(temp.buf,text,count);
    stvcpy(temp.buf+count,buffer->buf,buffer->used-1);
    temp.used=required;
    VbufFree(buffer);
    *buffer=temp;
}
void VStrPrepChr(VBUF *buffer,char chr)
{
    char text[2];
    text[0]=chr;
    text[1]='\0';
    VStrPrepStr(buffer,text);
}

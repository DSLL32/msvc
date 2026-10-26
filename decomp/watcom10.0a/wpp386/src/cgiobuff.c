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

/* bld/plusplus/c/cgiobuff.c, initial commit6b9cb443. Actual1994
 * initialization registers only this cleanup callback; later directory
 * initialization and extra counters are absent. */
#include <string.h>
#include "compiler.h"
#include "cgio.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>

void CgioBuffInit(void)
{
    CMemRegisterCleanup(cgioBuffReleaseMemory);
}

/* Actual1994 writer uses512B class-partitioned rings, before the
 * later directory/doubly-linked-reuse architecture. */
static CGIOBUFF *allBufs,*reusableBufs;
unsigned IoSuppTempNextBlock(unsigned count);
int SymIsInline(SYMBOL sym);
int SymIsTemporary(SYMBOL sym);
void *RingPrune(void **head,void *entry);
void activateBuffer(CGIOBUFF *buffer)
{
    RingPrune((void **)&reusableBufs,buffer);
    RingAppend((void **)&allBufs,buffer);
}
void deactivateBuffer(CGIOBUFF *buffer)
{
    RingPrune((void **)&allBufs,buffer);
    RingAppend((void **)&reusableBufs,buffer);
}
unsigned short bufferClass(SYMBOL sym)
{
    if (sym==0) return 1;
    if (SymIsInline(sym)) return 4;
    if (SymIsTemporary(sym)) return 3;
    if (sym->id==7) return 2;
    return 0;
}
CGIOBUFF *allocateBuffer(unsigned short file_class)
{
    CGIOBUFF *buffer;
    buffer=CMemAlloc(sizeof(*buffer));
    buffer->next=0;
    buffer->disk_addr=0;
    buffer->free_offset=0;
    buffer->reuse_next=0;
    buffer->reading=0;
    buffer->file_class=file_class;
    buffer->flags&=~3U;
    RingAppend((void **)&allBufs,buffer);
    return buffer;
}
CGIOBUFF *findWrBuffer(unsigned short file_class,CGIOBUFF *head)
{
    CGIOBUFF *buffer;
    if (head!=0) {
        buffer=head;
        do {
            buffer=buffer->next;
            if (!(buffer->flags&2) && buffer->file_class==file_class
                && (unsigned)(buffer->free_offset+15)<=512) return buffer;
        } while (buffer!=head);
    }
    return 0;
}
CGIOBUFF *findClassWrBuffer(unsigned short file_class)
{
    CGIOBUFF *buffer;
    buffer=findWrBuffer(file_class,allBufs);
    if (buffer==0) {
        buffer=findWrBuffer(file_class,reusableBufs);
        if (buffer!=0) activateBuffer(buffer);
        else {
            buffer=allocateBuffer(file_class);
            buffer->disk_addr=IoSuppTempNextBlock(1);
        }
    }
    buffer->flags|=2;
    buffer->flags&=~1U;
    return buffer;
}

CGIOBUFF *CgioBuffWrOpen(SYMBOL sym)
{ return findClassWrBuffer(bufferClass(sym)); }
char *pointXfer(CGIOBUFF *buffer)
{ return buffer->data+buffer->free_offset; }
void finishWrBuffer(CGIOBUFF *buffer)
{
    buffer->flags&=~2U;
    deactivateBuffer(buffer);
}
static CGFILE_INS lastWrite;
CGFILE_INS CgioBuffLastWrite(void) { return lastWrite; }
#pragma pack(1)
struct buffer_transfer { unsigned block; unsigned short offset; };
#pragma pack()
CGIOBUFF *CgioBuffWriteIC(CGIOBUFF *buffer,const void *data,unsigned size)
{
    CGIOBUFF *next;
    char *p;
    if ((unsigned)(buffer->free_offset+size)>505) {
        next=findClassWrBuffer(buffer->file_class);
        p=pointXfer(buffer);
        *p++=1; /* IC_NEXT; packed old transfer record is7B. */
        ((struct buffer_transfer *)p)->block=next->disk_addr;
        ((struct buffer_transfer *)p)->offset=next->free_offset;
        buffer->free_offset+=7;
        finishWrBuffer(buffer);
        buffer=next;
    }
    memcpy(pointXfer(buffer),data,size);
    lastWrite.block=buffer->disk_addr;
    lastWrite.offset=buffer->free_offset;
    buffer->free_offset+=size;
    return buffer;
}

CGIOBUFF *findBuffer(CGIOBUFF *last,unsigned block)
{
    CGIOBUFF *buffer=last;
    if(buffer) {
        do {
            buffer=buffer->next;
            if(buffer->disk_addr==block) return buffer;
        } while(buffer!=last);
    }
    return NULL;
}
void IoSuppTempRead(unsigned block,unsigned length,void *dest);
CGIOBUFF *findRdBuffer(unsigned block,unsigned short file_class)
{
    CGIOBUFF *buffer=findBuffer(allBufs,block);
    if(!buffer) {
        buffer=findBuffer(reusableBufs,block);
        if(buffer) activateBuffer(buffer);
        else {
            buffer=allocateBuffer(file_class);
            IoSuppTempRead(block,512,buffer->data);
            buffer->free_offset=512;
            buffer->disk_addr=block;
            buffer->flags|=1;
        }
    }
    ++buffer->reading;
    return buffer;
}
void releaseRdBuffer(CGIOBUFF *buffer)
{
    if(--buffer->reading==0 && !(buffer->flags&2))
        deactivateBuffer(buffer);
}
void CgioBuffZap(CGFILE_INS location,const void *data,unsigned length)
{
    CGIOBUFF *buffer=findRdBuffer(location.block,5);
    memcpy(buffer->data+(short)location.offset,data,length);
    buffer->flags&=~1U;
    releaseRdBuffer(buffer);
}

void CgioBuffWrClose(CGIOBUFF *buffer)
{
    *pointXfer(buffer)=0; /* IC_EOF has no operand in the old stream. */
    ++buffer->free_offset;
    finishWrBuffer(buffer);
}

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
* Description:  Buffered object-file output.
*
****************************************************************************/


#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "codegen.h"
#include "memory.h"
#include "cgmemory.h"
#include "pragma.h"
#include "objemit.h"
#include <recon.h>

static char ObjName[128];
static int ObjFile;
struct obj_buffer {
    struct obj_buffer *nextbuf;
    char *bufptr,*buf;
    unsigned bytes_left,bytes_written;
};
static struct obj_buffer *BufList,*CurBuf;
int CreateStream(char *name);
struct obj_buffer *NewBuffer(void);

unsigned char CGOpenf(void)
{
    CopyStr(FEAuxInfo(NULL,4),ObjName);
    ObjFile=CreateStream(ObjName);
    /* The earlier object writer tests zero here, not the later -1. */
    if (ObjFile==0) return 0;
    BufList=NewBuffer();
    CurBuf=BufList;
    return 1;
}

void ObjError(int error);

int CreateStream(char *name)
{
    int handle;
    /* Actual old OSI caller passes permission0, unlike later PMODE. */
    handle=open(name,O_CREAT|O_TRUNC|O_RDWR|O_BINARY,0);
    if (handle==-1) ObjError(errno);
    return handle;
}

struct obj_buffer *NewBuffer(void)
{
    struct obj_buffer *buffer;
    buffer=CGAlloc(sizeof(*buffer));
    buffer->buf=CGAlloc(8192);
    buffer->bufptr=buffer->buf;
    buffer->bytes_left=8192;
    buffer->bytes_written=0;
    buffer->nextbuf=NULL;
    return buffer;
}

static unsigned long ObjOffset;
static unsigned char NeedSeek;

void OpenObj(void)
{
    ObjOffset=0;
    NeedSeek=0;
}

#pragma pack(1)
static struct {
    unsigned char class;
    unsigned short len;
    unsigned char data[1025];
} header;
#pragma pack()

void PutObjRec(unsigned char class,void *bytes,unsigned length)
{
    unsigned char checksum;
    if (NeedSeek) {
        SeekStream(ObjFile,ObjOffset);
        NeedSeek=0;
    }
    header.class=class;
    header.len=length+1;
    checksum=DoSum((unsigned char *)&header,3);
    checksum+=DoSum(bytes,length);
    checksum=-checksum;
    if (length<=1024) {
        memcpy(header.data,bytes,length);
        header.data[length]=checksum;
        PutStream(ObjFile,&header,length+4);
    } else {
        PutStream(ObjFile,&header,3);
        PutStream(ObjFile,bytes,length);
        PutStream(ObjFile,&checksum,1);
    }
    ObjOffset+=length+4;
}

unsigned char DoSum(unsigned char *bytes,int length)
{
    unsigned char sum;
    sum=0;
    while (--length>=0) sum+=*bytes++;
    return sum;
}

void cleanupLastBuffer(struct obj_buffer *buffer);

void PutStream(int handle,void *bytes,unsigned length)
{
    unsigned count;
    char *cursor;
    handle=handle;
    cursor=bytes;
    for (;;) {
        count=length;
        if (count>CurBuf->bytes_left) count=CurBuf->bytes_left;
        memcpy(CurBuf->bufptr,cursor,count);
        cursor+=count;
        CurBuf->bufptr+=count;
        CurBuf->bytes_left-=count;
        if (CurBuf->nextbuf==NULL) cleanupLastBuffer(CurBuf);
        length-=count;
        if (length==0) break;
        if (CurBuf->nextbuf==NULL) {
            CurBuf->nextbuf=NewBuffer();
            CurBuf=CurBuf->nextbuf;
        } else {
            CurBuf=CurBuf->nextbuf;
            CurBuf->bufptr=CurBuf->buf;
            CurBuf->bytes_left=8192;
        }
    }
}

void cleanupLastBuffer(struct obj_buffer *buffer)
{
    unsigned used;
    used=8192-buffer->bytes_left;
    if (buffer->bytes_written<used) buffer->bytes_written=used;
}

unsigned long AskObjHandle(void)
{
    /* The 1994 format compresses large positions into 64-byte blocks. */
    if (ObjOffset>0x7fff && (ObjOffset&0x3f)!=0) {
        if (NeedSeek) {
            SeekStream(ObjFile,ObjOffset);
            NeedSeek=0;
        }
        ObjOffset+=PadObj(64-(ObjOffset&0x3f));
    }
    return Offset(ObjOffset);
}

unsigned long Offset(unsigned long byte_offset)
{
    if (byte_offset>0x207fc0) {
        FatalError("Object file too large");
        return 0;
    }
    if (byte_offset>0x7fff) return ((byte_offset-0x8000)>>6)|0x8000;
    return byte_offset;
}

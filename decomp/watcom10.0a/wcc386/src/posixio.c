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
#include <io.h>
#include <errno.h>
#include "codegen.h"
#include "memory.h"
#include "cgmemory.h"
#include "pragma.h"
#include "objemit.h"
#include "stubs.h"
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

unsigned long Byte(unsigned long reference)
{
    /* The 1994 handle format packs offsets above32K into64-byte blocks;
     * later posixio.c uses uncompressed handles and an identity Byte. */
    if (reference&0x8000) return ((reference&0x7fff)<<6)+0x8000;
    return reference;
}
void GetStream(int handle,void *buffer,unsigned length)
{
    unsigned count;
    char *cursor;
    (void)handle;
    cursor=buffer;
    for (;;) {
        count=length;
        if (count>CurBuf->bytes_left) count=CurBuf->bytes_left;
        memcpy(cursor,CurBuf->bufptr,count);
        cursor+=count;
        CurBuf->bufptr+=count;
        CurBuf->bytes_left-=count;
        length-=count;
        if (length==0) break;
        CurBuf=CurBuf->nextbuf;
        if (CurBuf==NULL) Zoiks(6);
        CurBuf->bufptr=CurBuf->buf;
        CurBuf->bytes_left=8192;
    }
}

void SeekStream(int handle,unsigned long offset)
{
    struct obj_buffer *buffer;
    unsigned long position;
    (void)handle;
    buffer=BufList;
    position=0;
    for (;;) {
        position+=8192;
        if (position>offset) break;
        if (buffer->nextbuf==NULL) {
            buffer->bytes_written=8192;
            buffer->nextbuf=NewBuffer();
        }
        buffer=buffer->nextbuf;
    }
    position=offset-(position-8192);
    buffer->bufptr=buffer->buf+position;
    buffer->bytes_left=8192-position;
    if (buffer->nextbuf==NULL) cleanupLastBuffer(buffer);
    CurBuf=buffer;
}

void PatchObj(unsigned long reference,unsigned offset,unsigned char *buffer,int length)
{
    unsigned long record_offset;
    unsigned short record_length;
    unsigned char checksum,old_bytes[80];
    record_offset=Byte(reference);
    SeekStream(ObjFile,record_offset+1);
    GetStream(ObjFile,&record_length,2);
    SeekStream(ObjFile,record_offset+offset+3);
    GetStream(ObjFile,old_bytes,length);
    SeekStream(ObjFile,record_offset+offset+3);
    PutStream(ObjFile,buffer,length);
    SeekStream(ObjFile,record_offset+2+record_length);
    GetStream(ObjFile,&checksum,1);
    checksum+=DoSum(old_bytes,length);
    checksum-=DoSum(buffer,length);
    SeekStream(ObjFile,record_offset+2+record_length);
    PutStream(ObjFile,&checksum,1);
    NeedSeek=1;
}
void GetFromObj(unsigned long reference,unsigned offset,unsigned char *buffer,int length)
{
    SeekStream(ObjFile,Byte(reference)+offset+3);
    GetStream(ObjFile,buffer,length);
    NeedSeek=1;
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

void FlushBuffers(int handle)
{
    struct obj_buffer *buffer;
    int written;
    for (;;) {
        buffer=BufList;
        if (buffer==NULL) break;
        written=write(handle,buffer->buf,buffer->bytes_written);
        if ((unsigned short)written!=buffer->bytes_written) {
            /* The 1994 body reports errno before its disk-full fatal path. */
            if (written==-1) ObjError(errno);
            FatalError("Error writing object file - disk is full");
        }
        BufList=buffer->nextbuf;
        CGFreeSize(buffer->buf,8192);
        CGFreeSize(buffer,sizeof(*buffer));
    }
    CurBuf=NULL;
}
void CloseStream(int handle) { (void)handle;STUB_TRAP(); }
void CloseObj(void)
{
    /* The 1994 writer uses zero as its unopened sentinel; its separate
     * discard routine handles deletion rather than a later AbortObj flag. */
    if (ObjFile!=0) {
        FlushBuffers(ObjFile);
        /* This old caller directly calls close, unlike later CloseStream. */
        close(ObjFile);
        ObjFile=0;
    }
}

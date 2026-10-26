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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/womp/c/objio.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. ActualCF1a5d4..1a610. */
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <io.h>
#include "wasm.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

OBJ_WFILE *ObjWriteOpen(const char *filename)
{
    int fh;
    OBJ_WFILE *obj;
    fh = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
              S_IREAD | S_IWRITE);
    if (fh < 0) return NULL;
    obj = MemAlloc(sizeof(*obj) + 4096);
    obj->fh = fh;
    obj->in_buf = 0;
    obj->in_rec = 0;
    return obj;
}

/* CF1a968..1aa1e. Header length includes the final checksum byte. */
void ObjWriteRec(OBJ_WFILE *obj, unsigned char command, unsigned short length,
                 const char *contents)
{
    char buf[3];
    unsigned char checksum;
    if (obj == NULL || obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",384);
    checksum = buf[0] = command;
    checksum += buf[1] = (length + 1) & 0xff;
    checksum += buf[2] = (length + 1) >> 8;
    safeWrite(obj->fh,buf,3);
    checksum += checkSum((const unsigned char *)contents,length);
    safeWrite(obj->fh,contents,length);
    checksum = -checksum;
    safeWrite(obj->fh,(const char *)&checksum,1);
}

/* CF1a5c0..1a5d3.1994 uses the argumentless disk-error App helper,
 * rather than the later womp Fatal(MSG_DISK_ERROR,"write") call. */
void safeWrite(int handle, const char *data, size_t length)
{
    if (write(handle,data,length) != length) DiskError();
}

/* CF1a950..1a965; byte-sized modular OMF sum. */
unsigned char checkSum(const unsigned char *data, unsigned short length)
{
    unsigned char checksum = 0;
    while (length) {
        checksum += *data;
        ++data;
        --length;
    }
    return checksum;
}

/* CF1a648..1a6ae. Buffered record starts with a provisional zero length. */
void ObjWBegRec(OBJ_WFILE *obj, unsigned char command)
{
    char buf[3];
    if (obj == NULL || obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",244);
    buf[0] = command;
    buf[1] = buf[2] = 0;
    safeWrite(obj->fh,buf,3);
    obj->in_rec = 1;
    obj->checksum = command;
    obj->in_buf = 0;
    obj->length = 0;
}

/* CF1a7bc..1a7fb. */
void ObjWrite8(OBJ_WFILE *obj, unsigned char value)
{
    if (obj == NULL || !obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",300);
    if (obj->in_buf == 4096) objWFlushBuffer(obj);
    obj->buffer[obj->in_buf++] = value;
}

/* CF1a83c..1a879, target little-endian WriteU32 is an unalignedx86 store. */
void ObjWrite32(OBJ_WFILE *obj, unsigned long value)
{
    if (obj == NULL || !obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",321);
    if (obj->in_buf >= 4093) objWFlushBuffer(obj);
    *(unsigned long *)(obj->buffer + obj->in_buf) = value;
    obj->in_buf += 4;
}

/* CF1a87c..1a8ac, variable-length OMF index encoding. */
void ObjWriteIndex(OBJ_WFILE *obj, unsigned short value)
{
    if (value > 0x7f) ObjWrite8(obj,(unsigned char)(0x80 | (value >> 8)));
    ObjWrite8(obj,(unsigned char)value);
}

/* CF1a718..1a7bb, backpatch the record length after final checksum. */
void ObjWEndRec(OBJ_WFILE *obj)
{
    char buf[2];
    unsigned char checksum;
    if (obj == NULL || !obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",281);
    if (obj->in_buf > 0) objWFlushBuffer(obj);
    ++obj->length;
    *(unsigned short *)buf = obj->length;
    checksum = obj->checksum + buf[0] + buf[1];
    checksum = -checksum;
    safeWrite(obj->fh,(const char *)&checksum,1);
    safeSeek(obj->fh,-(long)obj->length - 2,SEEK_CUR);
    safeWrite(obj->fh,buf,2);
    safeSeek(obj->fh,0L,SEEK_END);
    obj->in_rec = 0;
}

/* CF1a6b0..1a718. Fold buffered bytes into length/checksum before write. */
void objWFlushBuffer(OBJ_WFILE *obj)
{
    size_t len_to_write;
    unsigned char checksum;
    char *ptr;
    if (obj == NULL) InternalError("g:\\bld\\womp\\c\\objio.c",262);
    len_to_write = obj->in_buf;
    if (len_to_write == 0) return;
    checksum = obj->checksum;
    for (ptr = obj->buffer; ptr < obj->buffer + len_to_write; ++ptr)
        checksum += *ptr;
    obj->checksum = checksum;
    obj->length += len_to_write;
    safeWrite(obj->fh,obj->buffer,len_to_write);
    obj->in_buf = 0;
}

/* CF1a5b0..1a5bf, error branch tails to the nonreturningDiskError App. */
void safeSeek(int handle, long offset, int mode)
{
    if (lseek(handle,offset,mode) == -1L) DiskError();
}

/* CF1a8ac..1a950, fill and flush the fixed4096-byte output buffer. */
void ObjWrite(OBJ_WFILE *obj, const char *buf, size_t length)
{
    const char *ptr;
    size_t amount;
    if (obj == NULL || buf == NULL)
        InternalError("g:\\bld\\womp\\c\\objio.c",343);
    ptr = buf;
    for (;;) {
        amount = 4096 - obj->in_buf;
        if (amount >= length) {
            memcpy(obj->buffer + obj->in_buf,ptr,length);
            obj->in_buf += length;
            break;
        } else if (amount > 0) {
            memcpy(obj->buffer + obj->in_buf,ptr,amount);
            obj->in_buf += amount;
            ptr += amount;
            length -= amount;
        }
        objWFlushBuffer(obj);
    }
}

/* CF1a7fc..1a83a, target little-endian WriteU16. */
void ObjWrite16(OBJ_WFILE *obj, unsigned short value)
{
    if (obj == NULL || !obj->in_rec)
        InternalError("g:\\bld\\womp\\c\\objio.c",310);
    if (obj->in_buf >= 4095) objWFlushBuffer(obj);
    *(unsigned short *)(obj->buffer + obj->in_buf) = value;
    obj->in_buf += 2;
}

/* CF1a610..1a645, close the actual output descriptor before freeing owner. */
void ObjWriteClose(OBJ_WFILE *obj)
{
    if (obj == NULL) InternalError("g:\\bld\\womp\\c\\objio.c",231);
    if (obj->in_rec) ObjWEndRec(obj);
    close(obj->fh);
    MemFree(obj);
}

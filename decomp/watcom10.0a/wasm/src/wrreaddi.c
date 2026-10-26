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

/* Adapted from bld/sdk/rc/wres/c/wrreaddi.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual1994 CF20bd4..20c41
 * has the older boolean resource-type test and no fileinfo argument. */
#include <string.h>
#include "wasm.h"
#include <recon.h>

int WResReadDir(int handle, WResDir dir, int *duplicate_discarded)
{
    int error;
    long seekpos;
    error = 0;
    if (duplicate_discarded != NULL) {
        *duplicate_discarded = 0;
    }
    if (dir->Head != NULL) {
        __FreeTypeList(dir);
    }
    seekpos = (*WResRtns.seek)(handle, 0, SEEK_SET);
    if (seekpos == -1L) {
        error = 1;
        WresRecordError(9); /* WRS_SEEK_FAILED */
    }
    if (!error) {
        if (WResIsWResFile(handle)) {
            error = readWResDir(handle, dir);
        } else {
            error = readMResDir(handle, dir, duplicate_discarded);
        }
    }
    return error;
}

/* Actual 1994 reader accepts resource versions through 2, without fileinfo. */
int readWResDir(int handle, WResDir dir)
{
    WResHeader head;
    WResExtHeader ext_head;
    int error;
    long seekpos;
    error = WResReadHeaderRecord(&head, handle);
    if (!error && (head.Magic[0] != 0xc3d4c1d7UL || head.Magic[1] != 0xc3d2cdcfUL)) {
        error = 1;
        WresRecordError(3);
    }
    if (!error && head.WResVer > 2) {
        error = 1;
        WresRecordError(4);
    }
    if (!error) {
        if (head.WResVer >= 1) {
            seekpos = (*WResRtns.seek)(handle, sizeof(head), SEEK_CUR);
            error = (seekpos == -1L);
            if (error) {
                WresRecordError(9);
            } else {
                error = WResReadExtHeader(&ext_head, handle);
            }
        } else {
            ext_head.TargetOS = 1;
        }
    }
    if (!error) {
        dir->NumResources = head.NumResources;
        dir->NumTypes = head.NumTypes;
        dir->TargetOS = ext_head.TargetOS;
        seekpos = (*WResRtns.seek)(handle, head.DirOffset, SEEK_SET);
        if (seekpos == -1L) {
            error = 1;
            WresRecordError(9);
        }
    }
    if (!error) {
        error = readTypeList(handle, dir, head.WResVer);
    }
    return error;
}

/* CF208b0..209a8 uses the original five-byte, byte-length ID format. */
int readTypeList(int handle, WResDir dir, unsigned short version)
{
    WResTypeNode *newnode;
    WResTypeInfo newtype;
    int error, typenum, extrabytes;
    for (error = 0, typenum = 0; typenum < dir->NumTypes && !error; ++typenum) {
        error = WResReadFixedTypeRecord(&newtype, handle);
        if (!error) {
            extrabytes = WResIDExtraBytes(&newtype.TypeName);
            newnode = (*WResRtns.alloc)(sizeof(*newnode) + extrabytes);
            if (newnode == NULL) {
                error = 1;
                WresRecordError(13);
            }
        }
        if (!error) {
            newnode->Head = NULL;
            newnode->Tail = NULL;
            newnode->Info = newtype;
            if (extrabytes > 0) {
                error = WResReadExtraWResID(&newnode->Info.TypeName, handle);
            }
        }
        if (!error) {
            ResAddLLItemAtEnd((void **)&dir->Head, (void **)&dir->Tail, newnode);
            error = readResList(handle, newnode, version);
        }
    }
    return error;
}

/* CF20708..208ad, before the later fileInfo field and v3 name conversion. */
int readResList(int handle, WResTypeNode *type, unsigned short version)
{
    WResResNode *newnode;
    WResResInfo newres;
    WResResInfo1 newres1;
    WResLangNode *langnode;
    WResID *resid;
    int error, resnum, extrabytes;
    for (resnum = 0, error = 0; resnum < type->Info.NumResources && !error; ++resnum) {
        if (version < 2) {
            error = WResReadFixedResRecord1(&newres1, handle);
            resid = &newres1.ResName;
        } else {
            error = WResReadFixedResRecord(&newres, handle);
            resid = &newres.ResName;
        }
        if (!error) {
            extrabytes = WResIDExtraBytes(resid);
            newnode = (*WResRtns.alloc)(sizeof(*newnode) + extrabytes);
            if (newnode == NULL) {
                error = 1;
                WresRecordError(13);
            }
        }
        if (!error) {
            newnode->Head = NULL;
            newnode->Tail = NULL;
            if (version < 2) {
                newnode->Info.NumResources = 1;
                newnode->Info.ResName = newres1.ResName;
            } else {
                newnode->Info = newres;
            }
            if (extrabytes > 0) {
                error = WResReadExtraWResID(&newnode->Info.ResName, handle);
            }
            if (version < 2) {
                langnode = (*WResRtns.alloc)(sizeof(*langnode));
                if (langnode == NULL) {
                    error = 1;
                    WresRecordError(13);
                }
                if (!error) {
                    langnode->data = NULL;
                    langnode->Info.MemoryFlags = newres1.MemoryFlags;
                    langnode->Info.Offset = newres1.Offset;
                    langnode->Info.Length = newres1.Length;
                    langnode->Info.lang.lang = 0;
                    langnode->Info.lang.sublang = 0;
                    ResAddLLItemAtEnd((void **)&newnode->Head, (void **)&newnode->Tail, langnode);
                }
            } else {
                error = readLangInfoList(handle, newnode);
            }
        }
        if (!error) {
            ResAddLLItemAtEnd((void **)&type->Head, (void **)&type->Tail, newnode);
        }
    }
    return error;
}

/* CF20650..20706, older wrreaddi.c has25B nodes and no fileinfo. */
int readLangInfoList(int handle, WResResNode *resource)
{
    WResLangNode *node;
    unsigned index;
    int error, count;
    error = 0;
    for (index = 0; index < resource->Info.NumResources; ++index) {
        node = (*WResRtns.alloc)(sizeof(*node));
        if (node == NULL) {
            error = 1;
            WresRecordError(13);
        }
        if (error) break;
        count = (*WResRtns.read)(handle, &node->Info, sizeof(node->Info));
        if (count != sizeof(node->Info)) {
            error = 1;
            WresRecordError(count == -1 ? 8 : 14);
            (*WResRtns.free)(node);
            break;
        }
        node->data = NULL;
        ResAddLLItemAtEnd((void **)&resource->Head, (void **)&resource->Tail, node);
    }
    return error;
}

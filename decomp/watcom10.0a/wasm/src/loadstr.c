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

/* Adapted from bld/sdk/rc/wres/c/loadstr.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF1e708..1e717. */
#include <string.h>
#include <malloc.h>
#include "wasm.h"
#include <recon.h>

#ifdef __WATCOMC__
/* Shipped body uses the compiler's inline CMPSB form. */
#pragma intrinsic(memcmp)
#endif

int OpenResFile(HANDLE_INFO *instance)
{
    return instance->handle = ResOpenFileRO(instance->filename);
}

#pragma pack(1)
typedef struct dbgheader {
    unsigned short signature;
    unsigned char exe_major_ver, exe_minor_ver;
    unsigned char obj_major_ver, obj_minor_ver;
    unsigned short lang_size, seg_size;
    unsigned long debug_size;
} dbgheader;
#pragma pack()

long FileShift;

/* CF1e758..1e84e, including the optional trailing patch-level marker. */
int FindResources(HANDLE_INFO *instance)
{
    long currpos, offset;
    dbgheader header;
    int notfound;
    char buffer[sizeof("WATCOM patch level .xxx")];

    notfound = 1;
    FileShift = 0;
    offset = sizeof(dbgheader);
    if ((*WResRtns.seek)(instance->handle, -(long)sizeof(buffer), SEEK_END) != -1L) {
        if ((*WResRtns.read)(instance->handle, buffer, sizeof(buffer)) == sizeof(buffer)) {
            if (memcmp(buffer, "WATCOM patch level .xxx", 19) == 0) {
                offset += sizeof(buffer);
            }
        }
    }
    currpos = (*WResRtns.seek)(instance->handle, -offset, SEEK_END);
    for (;;) {
        (*WResRtns.read)(instance->handle, &header, sizeof(header));
        if (header.signature == 0x8302) {
            notfound = 0;
            FileShift = currpos - header.debug_size + sizeof(header);
            break;
        } else if (header.signature == 0x8386 || header.signature == 0x8300 || header.signature == 0x8301) {
            currpos -= header.debug_size;
            (*WResRtns.seek)(instance->handle, currpos, SEEK_SET);
        } else {
            break;
        }
    }
    return notfound;
}

static WResDir MainDir;

/* CF1e718..1e740: allocation error precedes directory read. */
int InitResources(HANDLE_INFO *instance)
{
    MainDir = WResInitDir();
    if (MainDir == NULL) {
        return -1;
    }
    return WResReadDir(instance->handle, MainDir, NULL);
}

/* CF1e668..1e707. The shipped wrapper directly uses initialized MainDir;
 * no later WResLoadString2/lazy-init call exists in this version. */
int LoadString(HANDLE_INFO *instance, unsigned id, char __far *buffer, int capacity)
{
    WResID string_type, block_id;
    WResDirWindow window;
    WResLangType lang;
    WResLangInfo *res;
    lang.lang = 0;
    lang.sublang = 0;
    WResInitIDFromNum((id >> 4) + 1, &block_id);
    WResInitIDFromNum(6, &string_type);
    window = WResFindResource(&string_type, &block_id, MainDir, &lang);
    if (window.CurrType == NULL || window.CurrRes == NULL || window.CurrLang == NULL)
        return -1;
    res = WResGetLangInfo(window);
    return GetString(res, instance, id, buffer, capacity);
}

/* CF1e520..1e668. Initial-help owner: PrintfUsage char[128], MsgGet passes
 * capacity128 and DS as its far selector; LoadString forwards them unchanged.
 * Native __far erasure is limited to that proved flat-DS, owned-buffer path.
 * Original four-site GS preservation patch changes no resource bytes. */
int GetString(WResLangInfo *res, HANDLE_INFO *instance, unsigned id,
              char __far *buffer, int capacity)
{
    long prevpos;
    int length, stringbufflen, stringnum, stringlen, numread, ix1, ix2;
    char *stringbuff;
    prevpos = (*WResRtns.seek)(instance->handle, res->Offset, SEEK_SET);
    if (prevpos == -1L) return -1;
    length = res->Length;
    stringbufflen = length < 128 ? length : 128;
    stringbuff = alloca(stringbufflen);
    if (stringbuff == NULL) return -1;
    stringnum = id & 15;
    numread = stringlen = ix2 = ix1 = 0;
    do {
        if (numread == 0) {
            if (length > stringbufflen) length -= stringbufflen;
            else {
                stringbufflen = length;
                length = 0;
            }
            numread = (*WResRtns.read)(instance->handle, stringbuff, stringbufflen);
            if (numread == 0 || numread == -1) return -1;
            ix1 = 0;
        }
        if (ix1 < numread) {
            if (stringlen == 0) {
                stringlen = ((unsigned char *)stringbuff)[ix1++];
                --stringnum;
            }
            for (; stringlen > 0; --stringlen) {
                if (ix1 >= numread) break;
                if (stringnum < 0 && ix2 < capacity - 1)
                    buffer[ix2++] = stringbuff[ix1];
                ++ix1;
            }
        } else numread = 0;
    } while (!(stringlen == 0 && stringnum < 0));
    buffer[ix2] = 0;
    return 0;
}

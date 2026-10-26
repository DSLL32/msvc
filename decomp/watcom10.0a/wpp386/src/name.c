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

/* Name interning, corresponding to bld/plusplus/c/name.c at initial
 * commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual1994 uses
 *241 buckets and a6-byte header with byte length/hash, unlike later source. */
#include <string.h>
#include "compiler.h"
#include <recon.h>

#pragma pack(1)
struct name_record {
    NAME_RECORD *next;
    unsigned char length, hash;
    char name[1];
};
#pragma pack()
static NAME_RECORD *hashTable[241];

char *NameCreateNoLen(char *id)
{
    unsigned bucket, length;
    NAME_RECORD *name, **head, **prev;

    bucket = nameHash(id, &length);
    head = &hashTable[bucket];
    prev = head;
    for (name = *prev; name; name = *prev) {
        if (name->length == (unsigned char)length &&
            !memcmp(name->name, id, length + 1)) {
            *prev = name->next;
            name->next = *head;
            *head = name;
            return name->name;
        }
        prev = &name->next;
    }
    return nameAdd(head, bucket, id, length);
}

/* Original 61B hash/length routine predates the released word-at-a-time hash.
 * Preserve its three-times-first-byte seed, 12-bit folding and241 buckets. */
unsigned nameHash(char *id, unsigned *length)
{
    char *start;
    unsigned c, h, g;

    start = id;
    c = (unsigned char)*id;
    h = c * 3;
    do {
        h = (h << 4) + c;
        g = h & ~0xfffu;
        h ^= g;
        h ^= g >> 12;
        c = (unsigned char)*++id;
    } while (c);
    *length = (unsigned)(id - start);
    return h % 241;
}

static unsigned long nameCount;

char *nameAdd(NAME_RECORD **head, unsigned bucket, char *id, unsigned length)
{
    NAME_RECORD *name;

    name = (NAME_RECORD *)CPermAlloc(7 + length);
    memcpy(name->name, id, length + 1);
    name->length = (unsigned char)length;
    name->hash = (unsigned char)bucket;
    name->next = *head;
    *head = name;
    ++nameCount;
    return name->name;
}

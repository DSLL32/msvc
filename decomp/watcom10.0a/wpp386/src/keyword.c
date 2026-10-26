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


/* Actual1994 keyword.c:70 generated keyword slots. The released hash
 * inputs provide the spellings; actual1994 TokValue weights and token order
 * define this older hash. Every row's original hash index is verified in the
 * scratch derivation. Keywords are ordinary literal table initializers.
 * TokValue retains its original named character roles from ascii.c. */
#include <string.h>
#include "scan.h"
#include "macro.h"
#include "scanchar.h"
#include "pending.h"
#include <recon.h>



int KwLookup(unsigned length)
{
    unsigned first=(unsigned char)Buffer[1];
    unsigned second=length>=2 ? (unsigned char)Buffer[length-2] : 0;
    unsigned hash;
    /* Bytes beyond the actual168-cell role have no qualified table owner.
     * Do not reproduce the original out-of-bounds read into adjacent data. */
    if (first>=168 || second>=168) { STUB_TRAP(); return 0; }
    hash=length+TokValue[first];
    if (length>=2) hash+=TokValue[second];
    hash &= 127;
    if (hash>=70) hash-=70;
    if (memcmp(Tokens[hash+0x39],Buffer,length+1)) return 0x36;
    return hash+0x39;
}

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

/* Adapted from bld/womp/c/genmsomf.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual table1abe0..1ac70,
 * packed six-byte entries; all callback bodies remain named traps. */
#include "wasm.h"
#include <recon.h>

enum {
    CMD_THEADR = 0x80,
    CMD_COMENT = 0x88,
    CMD_MODEND = 0x8a,
    CMD_EXTDEF = 0x8c,
    CMD_LNAMES = 0x96,
    CMD_SEGDEF = 0x98,
    CMD_GRPDEF = 0x9a,
    CMD_LEDATA = 0xa0,
    CMD_FIXUP = 0x9c,
    CMD_PUBDEF = 0x90,
    CMD_TYPDEF = 0x8e,
    CMD_LINNUM = 0x94,
    CMD_LIDATA = 0xa2,
    CMD_COMDEF = 0xb0,
    CMD_CEXTDF = 0xbc,
    CMD_STATIC_EXTDEF = 0xb4,
    CMD_STATIC_PUBDEF = 0xb6,
    CMD_STATIC_COMDEF = 0xb8,
    CMD_BAKPAT = 0xb2,
    CMD_COMDAT = 0xc2,
    CMD_LINSYM = 0xc4,
    CMD_ALIAS = 0xc6,
    CMD_NBKPAT = 0xc8,
    CMD_LLNAMES = 0xca
};
#define POBJ_WRITE_PASS 2

/* External visibility lets the period MAP bound this code-resident table. */
const pobj_list myFuncs[] = {
    { CMD_THEADR, POBJ_WRITE_PASS, writeTheadr },
    { CMD_COMENT, POBJ_WRITE_PASS, writeComent },
    { CMD_MODEND, POBJ_WRITE_PASS, writeModend },
    { CMD_EXTDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LNAMES, POBJ_WRITE_PASS, writeMisc },
    { CMD_SEGDEF, POBJ_WRITE_PASS, writeSegdef },
    { CMD_GRPDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LEDATA, POBJ_WRITE_PASS, writeLedata },
    { CMD_FIXUP, POBJ_WRITE_PASS, writeFixup },
    { CMD_PUBDEF, POBJ_WRITE_PASS, writePubdef },
    { CMD_TYPDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LINNUM, POBJ_WRITE_PASS, writeLinnum },
    { CMD_LIDATA, POBJ_WRITE_PASS, writeLidata },
    { CMD_COMDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_CEXTDF, POBJ_WRITE_PASS, writeMisc },
    { CMD_STATIC_EXTDEF, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_STATIC_PUBDEF, POBJ_WRITE_PASS, writePubdef },
    { CMD_STATIC_COMDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_BAKPAT, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_COMDAT, POBJ_WRITE_PASS, writeComdat },
    { CMD_LINSYM, POBJ_WRITE_PASS, writeLinsym },
    { CMD_ALIAS, POBJ_WRITE_PASS, writeMisc },
    { CMD_NBKPAT, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_LLNAMES, POBJ_WRITE_PASS, writeMisc },
};

void GenMSOmfInit(void)
{
    PObjRegList(myFuncs, sizeof(myFuncs) / sizeof(myFuncs[0]));
}

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


/*
 *  OBJOMF:  object file processing routines specific to OMF
 *
*/

#ifndef WLINK_OMF_H
#define WLINK_OMF_H

#include <recon.h>

/* Object record kinds used by this target's pass-one dispatcher. */
enum {
    CMD_ALIAS = 0xc6,
    CMD_BAKP32 = 0xb3,
    CMD_BAKPAT = 0xb2,
    CMD_BLKD32 = 0x7b,
    CMD_BLKDEF = 0x7a,
    CMD_BLKE32 = 0x7d,
    CMD_BLKEND = 0x7c,
    CMD_CEXTDF = 0xbc,
    CMD_COMD32 = 0xc3,
    CMD_COMDAT = 0xc2,
    CMD_COMDEF = 0xb0,
    CMD_COMENT = 0x88,
    CMD_DEBSYM = 0x7e,
    CMD_ENDREC = 0x78,
    CMD_EXTDEF = 0x8c,
    CMD_FIXU32 = 0x9d,
    CMD_FIXUP = 0x9c,
    CMD_GRPDEF = 0x9a,
    CMD_LEDA32 = 0xa1,
    CMD_LEDATA = 0xa0,
    CMD_LHEADR = 0x82,
    CMD_LIBDIC = 0xaa,
    CMD_LIBHED = 0xa4,
    CMD_LIBLOC = 0xa8,
    CMD_LIBNAM = 0xa6,
    CMD_LIDA32 = 0xa3,
    CMD_LIDATA = 0xa2,
    CMD_LINN32 = 0x95,
    CMD_LINNUM = 0x94,
    CMD_LINS32 = 0xc5,
    CMD_LINSYM = 0xc4,
    CMD_LLNAME = 0xca,
    CMD_LNAMES = 0x96,
    CMD_LOCSYM = 0x92,
    CMD_MODE32 = 0x8b,
    CMD_MODEND = 0x8a,
    CMD_NBKP32 = 0xc9,
    CMD_NBKPAT = 0xc8,
    CMD_OVLDEF = 0x76,
    CMD_PEDATA = 0x84,
    CMD_PIDATA = 0x86,
    CMD_PUBD32 = 0x91,
    CMD_PUBDEF = 0x90,
    CMD_REDATA = 0x72,
    CMD_REGINT = 0x70,
    CMD_RHEADR = 0x6e,
    CMD_RIDATA = 0x74,
    CMD_SEGD32 = 0x99,
    CMD_SEGDEF = 0x98,
    CMD_STATIC_COMDEF = 0xb8,
    CMD_STATIC_EXTD32 = 0xb5,
    CMD_STATIC_EXTDEF = 0xb4,
    CMD_STATIC_PUBD32 = 0xb7,
    CMD_STATIC_PUBDEF = 0xb6,
    CMD_THEADR = 0x80,
    CMD_TYPDEF = 0x8e,
    CMD_VENDEXT = 0xce,
    CMD_VERNUM = 0xcc
};

#endif

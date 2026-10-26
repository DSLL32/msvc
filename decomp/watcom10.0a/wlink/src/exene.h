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


#ifndef WLINK_EXENE_H
#define WLINK_EXENE_H
#pragma pack(1)
typedef struct os2_exe_header {
    unsigned short         signature;      /* signature to mark valid EXE file */
    unsigned short         version;        /* version of linker                */
    unsigned short         entry_off;      /* offset from header of entry table*/
    unsigned short         entry_size;     /* size of entry table              */
    unsigned long         chk_sum;        /* checksum of entire file          */
    unsigned short         info;           /* various flags (see below)        */
    unsigned short         adsegnum;       /* automatic data segment number    */
    unsigned short         heap;           /* initial heap allocation - bytes  */
    unsigned short         stack;          /* initial stack allocation - bytes */
    unsigned short         IP;             /* value for IP                     */
    unsigned short         entrynum;       /* segment number of entry segment  */
    unsigned short         SP;             /* value for SP                     */
    unsigned short         stacknum;       /* segment number of stack segment  */
    unsigned short         segments;       /* total number of segments         */
    unsigned short         modrefs;        /* # entries in module ref table    */
    unsigned short         nonres_size;    /* size of nonresident name table   */
    unsigned short         segment_off;    /* header offset of   segment table */
    unsigned short         resource_off;   /* " " "             resource table */
    unsigned short         resident_off;   /* " " "        resident name table */
    unsigned short         module_off;     /* " " "     module reference table */
    unsigned short         import_off;     /* " " "       imported names table */
    unsigned long         nonres_off;     /* offset from start of file of
                                                    nonresident names table */
    unsigned short         movable;        /* number of movable entry points   */
    unsigned short         align;          /* segment alignment shift count    */
    unsigned short         resource;       /* # of entries in resource table   */
    unsigned char          target;         /* target operating system */
    unsigned char          otherflags;     /* more flags */
    unsigned short         gangstart;      /* these 4 added for windows */
    unsigned short         ganglength;
    unsigned short         swaparea;
    unsigned short         expver;
} os2_exe_header;
#pragma pack()
typedef char old_ne_header_64[(sizeof(os2_exe_header) == 64) ? 1 : -1];
#include <recon.h>
#endif

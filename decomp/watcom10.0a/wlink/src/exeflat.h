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


#ifndef WLINK_EXEFLAT_H
#define WLINK_EXEFLAT_H
#include <recon.h>
#pragma pack(1)
typedef struct os2_flat_header {
    unsigned short     signature;
    unsigned char      byte_order;     /* the byte ordering of the .exe */
    unsigned char      word_order;     /* the word ordering of the .exe */
    unsigned long     level;          /* the exe format level */
    unsigned short     cpu_type;       /* the cpu type */
    unsigned short     os_type;        /* the operating system type */
    unsigned long     version;        /* .exe version */
    unsigned long     flags;          /* .exe flags */
    unsigned long     num_pages;      /* # of pages in .exe */
    unsigned long     start_obj;      /* starting object number */
    unsigned long     eip;            /* starting value of eip */
    unsigned long     stack_obj;      /* object # for stack pointer */
    unsigned long     esp;            /* starting value of esp */
    unsigned long     page_size;      /* .exe page size */
    union {
        unsigned long     last_page;      /* size of last page - LE */
        unsigned long     page_shift;     /* left shift for page offsets - LX */
    }               l;
    unsigned long     fixup_size;     /* fixup section size */
    unsigned long     fixup_cksum;    /* fixup section checksum */
    unsigned long     loader_size;    /* loader section size */
    unsigned long     loader_cksum;   /* loader section checksum */
    unsigned long     objtab_off;     /* object table offset */
    unsigned long     num_objects;    /* number of objects in .exe */
    unsigned long     objmap_off;     /* object page map offset */
    unsigned long     idmap_off;      /* iterated data map offset */
    unsigned long     rsrc_off;       /* offset of resource table */
    unsigned long     num_rsrcs;      /* number of resource entries */
    unsigned long     resname_off;    /* offset of resident names table */
    unsigned long     entry_off;      /* offset of entry table */
    unsigned long     moddir_off;     /* offset of module directives table */
    unsigned long     num_moddirs;    /* number of module directives */
    unsigned long     fixpage_off;    /* offset of fixup page table */
    unsigned long     fixrec_off;     /* offset of fixup record table */
    unsigned long     impmod_off;     /* offset of import module name table */
    unsigned long     num_impmods;    /* # of entries in import mod name tbl */
    unsigned long     impproc_off;    /* offset of import procedure name table */
    unsigned long     cksum_off;      /* offset of per-page checksum table */
    unsigned long     page_off;       /* offset of enumerated data pages */
    unsigned long     num_preload;    /* number of preload pages */
    unsigned long     nonres_off;     /* offset of non-resident names table */
    unsigned long     nonres_size;    /* size of non-resident names table */
    unsigned long     nonres_cksum;   /* non-resident name table checksum */
    unsigned long     autodata_obj;   /* object # of autodata segment */
    unsigned long     debug_off;      /* offset of the debugging information */
    unsigned long     debug_len;      /* length of the debugging info */
    unsigned long     num_inst_preload;   /* # of instance pages in preload sect*/
    unsigned long     num_inst_demand;    /*# instance pages in demand load sect*/
    unsigned long     heapsize;       /* size of heap - for 16-bit apps */
    unsigned long     stacksize;      /* size of stack */
    unsigned char      reserved[ 20 ];   /* pad to 196 bytes. */
} os2_flat_header;
#pragma pack()
typedef char old_flat_header_is_196[(sizeof(os2_flat_header)==196)?1:-1];
#endif

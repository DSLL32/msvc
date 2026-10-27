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


#ifndef WPP_CGIO_H
#define WPP_CGIO_H
#include <stddef.h>
#include <recon.h>
#pragma pack(1)
typedef struct cginter { unsigned char opcode;
    union { unsigned uvalue; int ivalue; void *pvalue; } value; } CGINTER;
typedef struct cgio_ins { unsigned block; unsigned short offset; } CGFILE_INS;
#pragma pack()
typedef struct cgfile CGFILE;
typedef struct cgiobuff CGIOBUFF;
typedef struct cgfile_gen CGFILE_GEN;
struct symbol;
struct type;
struct src_file;
#pragma pack(1)
typedef struct cgio_location {
    struct src_file *src_file;
    unsigned line,column;
} CGIO_LOCATION;
/* Old buffer prefix, proved by initCGFILE. Remaining header ownership is
 * intentionally unresolved until the buffer implementation is encountered. */
struct cgiobuff {
    CGIOBUFF *next;
    unsigned disk_addr;
    short free_offset;
    CGIOBUFF *reuse_next;
    unsigned reading,flags;
    unsigned char file_class;
    char data[512];
};
struct cgfile_gen {
    unsigned counters[10],emitted_counters[10];
    unsigned cs_label,cs_allocated;
    unsigned deferred_range[2]; /* untouched by this initializer */
    void *switch_scope,*switch_label_scope;
    struct symbol *init_sym;
    struct type *curr_type,*emit_type;
    CGIO_LOCATION source_location,emit_line_no,switch_location;
    unsigned flags;
};
struct cgfile {
    CGFILE *next;
    unsigned first;
    short offset;
    CGIOBUFF *buffer;
    struct symbol *symbol;
    void *cursor;
    CGFILE_GEN *gen;
    CGIO_LOCATION defined;
    unsigned cond_flags,flags;
};
#pragma pack()
typedef char CgfileSize[(sizeof(CGFILE)==46)?1:-1];
typedef char CgfileGenSize[(sizeof(CGFILE_GEN)==156)?1:-1];
typedef char CgioDataOffset[(offsetof(CGIOBUFF,data)==23)?1:-1];
CGIOBUFF *CgioBuffWrOpen(struct symbol *sym);
CGFILE *CgioCreateFile(struct symbol *sym);
void CgioBuffInit(void);
void cgioBuffReleaseMemory(void);
#endif

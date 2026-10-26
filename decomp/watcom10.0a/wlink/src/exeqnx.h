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
* Description:  Packed record types used by the 1994 QNX load-file writer.
*
****************************************************************************/

#ifndef WLINK_EXEQNX_H
#define WLINK_EXEQNX_H
#include "link.h"
#pragma pack(1)
typedef struct lmf_record {
    unsigned char rec_type, reserved;
    unsigned short data_nbytes, spare;
} lmf_record;
typedef struct lmf_header {
    unsigned short version, cflags, cpu, fpu;
    unsigned short code_index, stack_index, heap_index, argv_index;
    unsigned short spare2[4];
    unsigned long code_offset, stack_nbytes, heap_nbytes, image_base;
    unsigned long spare3[2];
} lmf_header;
typedef struct lmf_rw_end { unsigned short verify; unsigned long signature; } lmf_rw_end;
typedef struct lmf_data { unsigned short segment; unsigned long offset; } lmf_data;
typedef struct lmf_resource { unsigned short res_type, spare[3]; } lmf_resource;
typedef struct lmf_eof { unsigned char spare[6]; } lmf_eof;
#pragma pack()
typedef char lmf_record_is_6[(sizeof(lmf_record) == 6) ? 1 : -1];
typedef char lmf_header_is_48[(sizeof(lmf_header) == 48) ? 1 : -1];
typedef char lmf_data_is_6[(sizeof(lmf_data) == 6) ? 1 : -1];
typedef char lmf_resource_is_8[(sizeof(lmf_resource) == 8) ? 1 : -1];
typedef char lmf_rw_end_is_6[(sizeof(lmf_rw_end) == 6) ? 1 : -1];
extern void WriteQNXLoad( unsigned long, lmf_data *, lmf_record * );
extern void WriteQNXResource( void );
extern void WriteQNXData( unsigned long * );
extern void WriteQNXRelocs( reloc_info *, unsigned, unsigned short );
extern reloc_info *FloatFixups;
#include <recon.h>
#endif

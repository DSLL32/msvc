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


#ifndef WCC_I86OPCODE_H
#define WCC_I86OPCODE_H
#include <stddef.h>
#include "generate-code.h"
#include <recon.h>
enum code_flags { BYTE_OPCODE=1,BYTE_WORD=2,SIGN_UNSIGN=4,NEED_WAIT=8 };
#pragma pack(4)
typedef struct pccode_def {
    unsigned char low_gen,width;
    unsigned short *opcode_table;
    unsigned char *opcode_list;
    unsigned char flags;
} pccode_def;
#pragma pack()
typedef char old_pccode_def_size[(sizeof(pccode_def)==16)?1:-1];
typedef char old_pccode_opcodes[(offsetof(pccode_def,opcode_table)==4)?1:-1];
typedef char old_pccode_index[(offsetof(pccode_def,opcode_list)==8)?1:-1];
typedef char old_pccode_flags[(offsetof(pccode_def,flags)==12)?1:-1];
extern pccode_def PCCodeTable[10];
#endif

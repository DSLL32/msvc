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


#ifndef WCC_SCORE_H
#define WCC_SCORE_H
#include "block.h"
#include <recon.h>
struct score_list;
typedef struct score_list *list_head;
#pragma pack(4)
typedef struct score_info {
    long offset;
    name *base;
    union { temp_name *t;var_name *v;void *p; } symbol;
    int index_reg;
    unsigned char class;
} score_info;
typedef char old_score_info_size[(sizeof(score_info)==20)?1:-1];
typedef struct score_list {
    struct score_list *next;
    score_info info;
} score_list;
typedef char old_score_list_size[(sizeof(score_list)==24)?1:-1];
typedef struct score_reg {
    register_name *reg_name;
    hw_reg_set reg;
    int low,high,high_of,low_of;
    long size;
    unsigned char class;
} score_reg;
typedef char old_score_reg_size[(sizeof(score_reg)==32)?1:-1];
#define NO_INDEX (-1)
typedef struct score {
    list_head *list;
    struct score *next_reg,*prev_reg;
    unsigned char index,generation;
    unsigned short __pad_to_16;
} score;
#pragma pack()
typedef char old_score_size[(sizeof(score)==16)?1:-1];
extern int ScoreCount;
#endif

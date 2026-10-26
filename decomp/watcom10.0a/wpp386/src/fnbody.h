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


#ifndef WPP_FNBODY_H
#define WPP_FNBODY_H
#include "compiler.h"
#include "symbol.h"
#include "carve.h"
#include "cgio.h"
#include <recon.h>
typedef struct label_memory { carve_t carve; void *blk_hdr; } LAB_MEM;
#pragma pack(1)
typedef struct function_data FUNCTION_DATA;
typedef struct control_stack CSTACK;
struct function_data {
    FUNCTION_DATA *next;
    SCOPE fn_scope;
    unsigned short depth,outer_depth,block_count;
    unsigned flag_count;
    CSTACK *control;
    void *labels;
    LAB_MEM label_mem;
    unsigned end_dtor;
    CGFILE_INS dtor_beg,init_state_tab;
    struct { SYMBOL var; unsigned mask; } init_var;
    void *access_errs;
    unsigned short is_ctor:1,is_dtor:1,is_assign:1,is_defarg:1;
    unsigned short dead_code:1,dead_diagnosed:1,ret_reqd:1,has_mem_init:1;
    unsigned short has_state_tab:1,reserved:7; /* actual old nine flags */
};
#pragma pack()
typedef char FunctionDataSize[(sizeof(FUNCTION_DATA)==64)?1:-1];
void LabelSwitchFunc(LAB_MEM *memory);
void FunctionBodyStartup(SYMBOL sym,FUNCTION_DATA *data,unsigned char flags);
void ModuleInitInit(void);
#endif

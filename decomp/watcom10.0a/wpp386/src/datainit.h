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


#ifndef WPP_DATAINIT_H
#define WPP_DATAINIT_H
#include "types.h"
#include "symbol.h"
#include <recon.h>
/* Encountered1994 initialization state, before simple_locn/padded_size. */
enum { DS_EXPRESSION,DS_OPEN_BRACE,DS_CLOSE_BRACE,DS_FINISH,DS_COMPLETED,
    DS_ERROR,DS_ABANDONED,DS_IGNORE };
enum { DL_IGNORE,DL_INTERNAL_AUTO,DL_INTERNAL_STATIC,DL_EXTERNAL_STATIC,DL_EXTERNAL_PUBLIC };
enum { DT_SCALAR,DT_ARRAY,DT_CLASS,DT_BITFIELD,DT_ERROR };
enum { DE_ROOT_TYPE,DE_BRACE,DE_TYPE };
typedef struct initialize_info INITIALIZE_INFO;
typedef struct initialize_queue INITIALIZE_QUEUE;
typedef struct data_init INITIALIZE_DATA;
#pragma pack(1)
struct initialize_info {
    INITIALIZE_INFO *stack,*previous;
    TYPE type;
    union { struct { SYMBOL stop,curr; } c; struct { unsigned index; } a;
        struct { TYPE type; unsigned mask; } b; struct { int bitf; } s; } u;
    unsigned char entry,target;
    unsigned base,offset,mem_size;
};
struct initialize_queue { INITIALIZE_QUEUE *ring; unsigned start,size; };
struct data_init {
    INITIALIZE_DATA *prev;
    DECL_INFO *dinfo;
    INITIALIZE_INFO *nest;
    INITIALIZE_QUEUE *queue;
    SYMBOL sym,auto_sym,ctor_sym;
    TYPE sym_type,base_type,auto_type;
    unsigned size_type;
    unsigned char state,location;
    unsigned once_only_label;
    unsigned char once_only:1,label_done:1,auto_static:1,need_storage:1,
        ctor_reqd:1,dtor_reqd:1,dtor_done:1,const_int:1;
    unsigned char emit_code:1,bracketed:1,all_zero:1,huge_sym:1,initted:1,
        no_size:1,unclassified_flags:2;
};
#pragma pack()
typedef char InitializeDataSize[(sizeof(INITIALIZE_DATA)==52)?1:-1];
typedef char InitializeInfoSize[(sizeof(INITIALIZE_INFO)==34)?1:-1];
#endif

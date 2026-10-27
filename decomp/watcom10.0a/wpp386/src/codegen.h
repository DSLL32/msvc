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


#ifndef WPP_CODEGEN_H
#define WPP_CODEGEN_H
#include <stddef.h>
#include <recon.h>
typedef unsigned short cg_type;
enum { U1,I1,U2,I2,U4,I4,CP,PT,FS,FD,XX,X8,ER };
/* The 1994 interface predates the 64-bit integer type codes. */
enum { T_UINT_1,T_INT_1,T_UINT_2,T_INT_2,T_UINT_4,T_INT_4,
       T_LONG_POINTER,T_HUGE_POINTER,T_NEAR_POINTER,T_LONG_CODE_PTR,T_NEAR_CODE_PTR,
       T_SINGLE,T_DOUBLE,T_UNKNOWN,T_DEFAULT,T_INTEGER,T_UNSIGNED,
       T_POINTER,T_CODE_PTR,T_BOOLEAN,T_PROC_PARM };
#define T_NEAR_INTEGER 0xfffaU
#define T_LONG_INTEGER 0xfffbU
#define T_HUGE_INTEGER 0xfffcU
typedef union cg_init_info {
    struct { unsigned revision:10,target:5,is_large:1; } version;
    int success;
} cg_init_info;
#pragma pack(4)
typedef struct type_def {
    int refno;
    long length;
    unsigned char attr;
} type_def;
#pragma pack()
typedef char old_type_def_size[(sizeof(type_def)==12)?1:-1];
extern type_def *TypeProcParm,*TypeInteger,*TypeHugeInteger,*TypeLongInteger;
extern type_def *TypeNearInteger,*TypeUnsigned,*TypeBoolean,*TypeNone,*TypePtr;
extern type_def *PTInteger,*PTUnsigned,*PTCodePointer,*PTPointer;
unsigned char TypeClass(type_def *type);
unsigned char ReturnClass(type_def *type,unsigned short attributes);
void InitFrl(void **list);
void InitTyping(void);
void TargTypeInit(void);
type_def *TypeAddress(cg_type type);
type_def *TypeAlias(cg_type alias,cg_type type);
cg_init_info BEInit(unsigned gen,unsigned target,unsigned size,unsigned short cpu);
extern unsigned Model,TargetModel;
extern unsigned char OptForSize;
extern unsigned short CGProcessorVersion;
#endif

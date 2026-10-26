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

/* Encountered1994 label ownership records: the released named fields retain
 * the actual28-byte definition and32-byte reference layouts. */
#ifndef WPP_LABEL_H
#define WPP_LABEL_H
#include "symbol.h"
#include "scope.h"
#include "cgio.h"
#include <recon.h>
typedef struct lab_def LAB_DEF;
typedef struct lab_ref LAB_REF;
#pragma pack(1)
typedef struct label_position { SCOPE scope; SYMBOL sym; unsigned var_no; } LAB_POSN;
struct lab_ref { LAB_REF *next; LAB_DEF *defn; LAB_POSN posn;
    CGFILE_INS ins_exit,ins_enter; };
struct lab_def { LAB_POSN posn; LAB_REF *forward; CGIO_LOCATION locn; };
#pragma pack()
typedef char LabDefSize[(sizeof(LAB_DEF)==28)?1:-1];
typedef char LabRefSize[(sizeof(LAB_REF)==32)?1:-1];
void LabelDefFree(LAB_DEF *def);
void LabelRefFree(LAB_REF *ref);
#endif

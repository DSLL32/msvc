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


#ifndef WPP_PTREE_H
#define WPP_PTREE_H
#include "types.h"
#include "srcfile.h"
#include "cgio.h"
#include <recon.h>
/* Actual old node layout, before decoration/id_cgop and64-bit constants. */
#pragma pack(1)
typedef struct parse_tree_node *PTREE;
struct parse_tree_node {
    unsigned char op,cgop;
    unsigned short flags;
    TYPE type;
    CGIO_LOCATION locn;
    union { long int_constant; unsigned uint_constant; double floating_constant;
        struct { unsigned low,high; } floating_bits;
        PTREE subtree[2];
        struct { struct symbol *symbol; struct search_result *result; } symcg;
        struct { char *name; unsigned char field_name,operator_code; } id; } u;
};
#pragma pack()
typedef char PtreeSize[(sizeof(struct parse_tree_node)==28)?1:-1];
/* Old identifier metadata has separate field-name and operator bytes. */
PTREE PTreeId(char *id);
PTREE ThrowsAnything(void);
PTREE PTreeType(TYPE type);
PTREE PTreeTListAppend(PTREE start,PTREE tree);
PTREE PTreeAlloc(void);
PTREE PTreeFree(PTREE tree);
void PTreeFreeSubtrees(PTREE tree);
PTREE PTreeTraversePostfix(PTREE tree,PTREE (*visit_rtn)(PTREE));
void ptreeSetConstantType(PTREE tree,type_id id);
PTREE PTreeIntConstant(long value,type_id id);
#endif

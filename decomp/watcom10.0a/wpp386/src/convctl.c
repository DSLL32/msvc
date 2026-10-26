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

/* PcPtrValidate/pcPtrScope from initial convctl.c, correlated to the
 * earlier binary. Its based-self/string/fetch/add case predates the later
 * identical-classification exception for implicit conversions. */
#include "compiler.h"
#include "types.h"
#include "scope.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
TYPE TypePointedAtModified(TYPE type);
SCOPE TypeScope(TYPE type);
unsigned char ScopeDerived(SCOPE derived,SCOPE possible_base);
void PTreeErrorExpr(PTREE expr,unsigned message);
void CFatal(const char *message);
enum {
    PC_REGULAR,PC_BASED_STRING,PC_BASED_SELF,PC_BASED_VOID,
    PC_BASED_FETCH,PC_BASED_ADD,PC_FAR16,PC_NOT
};
#define PC_BIT(cls) (0x80u>>(cls))
static unsigned char pcPtrChk[]={
    0,PC_BIT(PC_BASED_SELF),PC_BIT(PC_BASED_SELF),
    PC_BIT(PC_REGULAR)|PC_BIT(PC_BASED_SELF)|PC_BIT(PC_BASED_VOID)|PC_BIT(PC_FAR16),
    PC_BIT(PC_BASED_SELF),PC_BIT(PC_BASED_SELF),0,0
};
#undef PC_BIT
SCOPE pcPtrScope(TYPE type)
{
    return TypeScope(TypePointedAtModified(type));
}
unsigned PcPtrValidate(TYPE target,TYPE source,unsigned char target_class,
    unsigned char source_class,PTREE expr,unsigned conversion)
{
    SCOPE target_scope,source_scope;
    if((0x80u>>target_class)&pcPtrChk[source_class]) {
        switch(source_class) {
        case PC_BASED_VOID:
            switch(target_class) {
            case PC_REGULAR:
                if(conversion==8) return 0;
                /* Fall through: based(void) is not implicitly a segment. */
            case PC_FAR16:
                PTreeErrorExpr(expr,190);
                return 2;
            case PC_BASED_VOID:
                target_scope=pcPtrScope(target);
                if(!target_scope) return 0;
                source_scope=pcPtrScope(source);
                if(!source_scope) return 0;
                if(ScopeDerived(source_scope,target_scope)==2) {
                    PTreeErrorExpr(expr,506);
                    return 2;
                }
                return 0;
            }
            break;
        case PC_BASED_STRING: case PC_BASED_SELF:
        case PC_BASED_FETCH: case PC_BASED_ADD:
            break;
        default:
            CFatal("pcPtrValidate -- bad case");
            /* Its original fatal helper terminates this unsupported domain. */
            break;
        }
        return conversion==8 ? 0 : 4;
    }
    return 0;
}

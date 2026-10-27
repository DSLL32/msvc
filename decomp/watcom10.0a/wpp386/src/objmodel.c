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


/* Initial6b9cb443 objmodel.c; actual1994 return classifier uses
 * ClassCanPassByValue's older nontrivial-constructor/dtor/copy test,
 * before the later cached passed_ref class flag. */
#include "compiler.h"
#include "types.h"
#include "pending.h"
#include <recon.h>
TYPE StructType(TYPE type);
TYPE TypedefModifierRemoveOnly(TYPE type);
int ClassCanPassByValue(TYPE type);
unsigned char ObjModelFunctionReturn(TYPE type)
{
    TYPE result;
    type=FunctionDeclarationType(type);
    result=TypedefModifierRemoveOnly(type->of);
    switch (result->id) {
    case TYP_VOID: return 0;
    case TYP_CLASS:
        if ((type->flag&4) && !ClassCanPassByValue(StructType(type->of)))
            return 2;
        return 3;
    default: return 1;
    }
}

/* Actual1994 scalar=1, class-reference=2, class-value=3. */
unsigned char ObjModelArgument(TYPE type)
{
    type=StructType(type);
    if(!type) return 1;
    return ClassCanPassByValue(type) ? 3 : 2;
}

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

/* Initial Open Watcom arglist.c (6b9cb443). The 1994 entry combines the
 * later separate AllocArgListPerm/Temp bodies behind a permanence argument. */
#include <string.h>
#include "types.h"
#include "symbol.h"
#include <recon.h>

typedef char ArgListSize[(sizeof(arg_list)==16)?1:-1];

arg_list *AllocArgList(int count, int permanent)
{
    unsigned size;
    arg_list *args;

    size = sizeof(arg_list);
    if (count > 0) size += (count - 1) * sizeof(TYPE);
    if (permanent) args = (arg_list *)CPermAlloc(size);
    else args = (arg_list *)CMemAlloc(size);
    args = InitArgList(args);
    args->num_args = count;
    return args;
}

arg_list *InitArgList(arg_list *args)
{
    args->num_args = 0;
    args->except_spec = 0;
    args->qualifier = 0;
    args->type_list[0] = 0;
    return args;
}

TYPE TypeThisSymbol(SYMBOL sym,int reference);
arg_list *MakeMemberArgList(SYMBOL sym,unsigned count)
{
    arg_list *mock,*formals;
    mock=AllocArgList(count,0);
    formals=SymFuncArgList(sym);
    mock->except_spec=formals->except_spec;
    mock->type_list[0]=TypeThisSymbol(sym,1);
    if(count>1) memcpy(&mock->type_list[1],formals->type_list,
                        sizeof(TYPE)*(count-1));
    return mock;
}

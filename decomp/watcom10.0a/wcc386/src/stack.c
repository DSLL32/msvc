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
* Description:  SafeRecurse host-stack handling.
*
****************************************************************************/


#include "codegen.h"
#include "procdef.h"
#include "cgmemory.h"
#include <recon.h>

#if !defined(__linux__)
/* stack.c/stackok.h/cypfunc.h require these inline ABI operations because C
 * cannot read or relocate its active ESP/EBP frames. They introduce no
 * out-of-line assembly implementation or synthetic stack-low storage. */
extern char *bp(void);
#pragma aux bp = "mov eax,ebp" value [eax];
extern char *sp(void);
#pragma aux sp = value [esp];
extern void setsp(char *value);
#pragma aux setsp = "mov esp,eax" parm [eax] modify [esp];
extern void setbp(char *value);
#pragma aux setbp = "mov ebp,eax" parm [eax];
extern char *__STACKLOW;
#pragma aux __STACKLOW "*";
extern char *CypCopy(char *source,char *destination,unsigned size);
#pragma aux CypCopy = \
    "push es" "push ds" "pop es" "rep movsb" "pop es" \
    parm routine [esi] [edi] [ecx] value [edi];
#endif

void *SafeRecurse(recurse_callback callback,void *value)
{
#if defined(__linux__)
    /* The released NT host uses this real callback-only path. Linux owns a
     * different host stack; OSI low-stack relocation is not qualified here. */
    return callback(value);
#else
    char *savearea;
    void *result;
    unsigned char old_action;
    /* Keep the actual OSI period-build branch and named cold callees. The
     * current case qualifies only the normal callback, not stack exhaustion. */
    if ((unsigned)sp()-(unsigned)__STACKLOW<512U) {
        old_action=SetMemOut(MO_OK);
        savearea=CGAlloc(512);
        if (savearea==NULL) FatalError("No memory to save stack");
        SetMemOut(old_action);
        CypCopy(bp(),savearea,512);
        CypCopy(sp(),sp()+512,bp()-sp());
        setbp(bp()+512);
        setsp(sp()+512);
        result=callback(value);
        setsp(sp()-512);
        CypCopy(sp()+512,sp(),bp()-sp()-512);
        setbp(bp()-512);
        CypCopy(savearea,bp(),512);
        CGFreeSize(savearea,512);
        return result;
    }
    return callback(value);
#endif
}

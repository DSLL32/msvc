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


#include "compiler.h"
#include "stubs.h"
#include <recon.h>

/* The actual127a9 dispatcher selects five compiled-in pointer tables at
 * code thresholds2000/1000/300/200, then copies through the terminating NUL.
 * Later released cmsg.c factors table lookup into CGetMsgStr. Only the
 * encountered PHRASE_CODE_SIZE data has been reconstructed: original cell
 * 762e7 points to726d7, "Code size". Other message data is unqualified,
 * and must trap in BOTH builds rather than read placeholder table entries.
 * The old phrase number2004 differs from the later4000-based Misc group. */
void CGetMsg(char *text,int code)
{
    const char *message;
    if (code!=2004) {
        STUB_TRAP();
        return;
    }
    message="Code size";
    while ((*text++=*message++)!='\0') ;
}

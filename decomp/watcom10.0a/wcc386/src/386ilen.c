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
* Description:  Object instruction lengths for 16/32-bit x86 calls and branches.
*
****************************************************************************/


#include "codegen.h"
#include "options.h"
#include "stubs.h"
#include <recon.h>

/* SHORT, NEAR, CHEAP and FAR lengths, with 16/32-bit row pairs. */
static unsigned char InsSize[6][4]={
    {0,6,7,8},{0,5,6,7},
    {2,6,0,8},{2,5,0,7},
    {2,7,0,0},{2,6,0,0}
};
int OptInsSize(unsigned char kind,int destination)
{
    int i;
    switch (kind) {
    case 8: i=0; break; /* OC_CALL */
    case 12: i=2; break; /* OC_JMP */
    case 10: i=4; break; /* OC_JCOND */
    default: STUB_TRAP();return 0;
    }
    if ((unsigned)destination>3) STUB_TRAP();
    if (TargetModel&0x800) ++i;
    return InsSize[i][destination];
}

/* 386ilen.c's semantic NOP templates, from six bytes down to one.
 * Alternating EAX/ECX and EDX/EBX avoids repeated dependency chains. */
static unsigned char NopList1[]={
    6,
    0x8d,0x80,0,0,0,0,       /* lea eax,0[eax] */
    0x8d,0x40,0,0x8b,0xc9,   /* lea eax,0[eax]; mov ecx,ecx */
    0x8d,0x44,0x20,0,        /* lea eax,0[eax+no_index] */
    0x8d,0x40,0,             /* lea eax,0[eax] */
    0x8b,0xc0,               /* mov eax,eax */
    0x90                     /* nop */
};
static unsigned char NopList2[]={
    6,
    0x8d,0x92,0,0,0,0,
    0x8d,0x52,0,0x8b,0xdb,
    0x8d,0x54,0x22,0,
    0x8d,0x52,0,
    0x8b,0xd2,
    0x90
};
unsigned char *NopLists[]={NopList1,NopList2};

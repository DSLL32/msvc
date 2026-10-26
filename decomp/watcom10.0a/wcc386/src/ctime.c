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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <stdio.h>
#include <time.h>
#include "macros.h"
#include "files.h"
#include "pragma.h"
#include <recon.h>

char __Time[10],__Date[12];

void TimeInit(void)
{
    static const char *Months[]={
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    struct tm *tod;
    time_t time_of_day;

    time_of_day=time(&time_of_day);
    tod=localtime(&time_of_day);
    sprintf(__Time,"%.2d:%.2d:%.2d",tod->tm_hour,tod->tm_min,tod->tm_sec);
    sprintf(__Date,"%3s %.2d %d",Months[tod->tm_mon],tod->tm_mday,tod->tm_year+1900);
}

unsigned long *GetTimeStamp(struct fname_list *file)
{
    struct tm *local;
    unsigned short date,clock;
    static unsigned long dos_stamp;
    local=localtime((time_t *)&file->mtime);
    date=((unsigned)(local->tm_year-80)<<9) |
         ((unsigned)(local->tm_mon+1)<<5) | (unsigned)local->tm_mday;
    clock=((unsigned)local->tm_hour<<11) | ((unsigned)local->tm_min<<5) |
          (unsigned)(local->tm_sec/2);
    dos_stamp=(unsigned long)clock|((unsigned long)date<<16);
    return &dos_stamp;
}

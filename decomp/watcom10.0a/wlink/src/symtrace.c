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

#include <string.h>
#include "cmdline.h"
#include <recon.h>

#pragma pack(1)
typedef struct trace_info {
    struct trace_info *next;
    union { char *name; file_list *lib; } u;
    char *member;
    unsigned char found;
} trace_info;
#pragma pack()

trace_info *TraceList;          /* original441f84 */
static trace_info *CurrTrace;   /* original450218 */

void CheckTraces( void )
{
    trace_info *info, *next;
    trace_info **prev;
    file_list *lib;

    prev = &TraceList;
    for( info = TraceList; info != 0; info = next ) {
        next = info->next;
        if( info->member == 0 ) {
            CurrTrace = info;
            /* The1994 caller has no separate Root callback. */
            ProcAllOvl( CheckFileTrace );
            if( !info->found ) {
                LnkMsg( 0x3266, "s", info->u.name );
                _LnkFree( info->u.name );
            }
            _LnkFree( info );
        } else {
            for( lib = ObjLibFiles; lib != 0; lib = lib->next_file ) {
                if( stricmp( lib->file->name, info->u.name ) == 0 ) {
                    info->found = 1;
                    _LnkFree( info->u.name );
                    info->u.lib = lib;
                    break;
                }
            }
            *prev = info;
            prev = &info->next;
        }
    }
    *prev = 0;
}

void CheckLibTrace( file_list *list )
{
    trace_info *info;

    for( info = TraceList; info != 0; info = info->next ) {
        if( !info->found && stricmp( info->u.name, list->file->name ) == 0 ) {
            info->found = 1;
            _LnkFree( info->u.name );
            info->u.lib = list;
            return;
        }
    }
}

unsigned char FindLibTrace( mod_entry *mod )
{
    trace_info **prev;
    trace_info *info;

    prev = &TraceList;
    for( info = TraceList; info != 0; info = info->next ) {
        if( info->found && info->u.lib == mod->f.source ) {
            if( ModNameCompare( mod->name, info->member ) ) {
                *prev = info->next;
                _LnkFree( info->member );
                _LnkFree( info );
                return 1;
            }
        }
        prev = &info->next;
    }
    return 0;
}

void PrintBadTraces( void )
{
    trace_info *info, *next;

    for( info = TraceList; info != 0; info = next ) {
        next = info->next;
        if( info->found ) {
            LnkMsg( 0x3267, "12", info->u.lib->file->name, info->member );
        } else {
            LnkMsg( 0x3267, "12", info->u.name, info->member );
            _LnkFree( info->u.name );
        }
        _LnkFree( info->member );
        _LnkFree( info );
    }
    TraceList = 0;
}

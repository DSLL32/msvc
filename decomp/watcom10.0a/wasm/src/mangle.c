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


/* CF1a22c..1a260; symbol-specific mangler selection and conflict check. */
#include <string.h>
#include "direct.h"
#include "asmparse.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,strcat)
#endif

void SetMangler(asm_sym *sym, char *type)
{
    mangle_func mangle = GetMangler(type);
    if (mangle == NULL) {
        /* No explicit mangler. */
    } else if (sym->mangler == NULL) sym->mangler = mangle;
    else if (sym->mangler != mangle) AsmErr(1152, sym->name);
}

/* CF1a1a8..1a1f3; explicit C/native naming policies retain their function
 * pointer identities. */
mangle_func GetMangler(char *type)
{
    mangle_func mangle = NULL;
    if (type != NULL) {
        if (stricmp(type, "C") == 0) mangle = CMangler;
        else if (stricmp(type, "N") == 0) mangle = AsmMangler;
        else AsmErr(1151, type);
    }
    return mangle;
}

/* CF1a1f4..1a22c, cache selected mangler in the existing symbol owner. */
char *Mangle(asm_sym *sym, char *buffer)
{
    mangle_func mangle = sym->mangler;
    if (mangle == NULL) {
        mangle = GetMangler(Options.default_name_mangler);
        if (mangle == NULL) mangle = AsmMangler;
        sym->mangler = mangle;
    }
    return mangle(sym,buffer);
}

/* CF19fb0..1a000, preserve returned buffer identity. */
char *AsmMangler(asm_sym *sym, char *buffer)
{
    char *name;
    if (buffer == NULL) {
        name = AsmAlloc(strlen(sym->name) + 1);
        strcpy(name,sym->name);
        return name;
    }
    strcpy(buffer,sym->name);
    return buffer;
}

/* CF1a000..1a1a5; prepend/append/remove naming decorations. */
enum { NORMAL=0, USCORE_FRONT=1, USCORE_BACK=2, REM_USCORE_FRONT=4,
       REM_USCORE_BACK=8, UPPERCASE=16 };
char *CMangler( struct asm_sym *sym, char *buffer )
/*************************************************/
{
    char                *name;
    char                *ptr = sym->name;
    unsigned            changes = NORMAL;
    proc_info           *info;
    unsigned            additional_size = 1; // for null

    switch( Options.naming_convention ) {
    case DO_NOTHING:
        return( AsmMangler( sym, buffer ));
    case ADD_USCORES:
        if( sym->state == SYM_PROC ) {
            changes |= USCORE_BACK;
        } else {
            switch( sym->mem_type ) {
            case T_NEAR:
            case T_FAR:
            case EMPTY:
                changes |= USCORE_BACK;
                break;
            default:
                changes |= USCORE_FRONT;
            }
        }
        break;
    case REMOVE_USCORES:
        if( sym->state == SYM_PROC ) {
            changes |= REM_USCORE_BACK;
        } else {
            switch( sym->mem_type ) {
            case T_NEAR:
            case T_FAR:
            case EMPTY:
                changes |= REM_USCORE_BACK;
                break;
            default:
                changes |= REM_USCORE_FRONT;
            }
        }
    }
    if( sym->state == SYM_PROC ) {
        info = ((dir_node*)sym)->e.procinfo;
        if( info->langtype == 4 || info->langtype == 6 ) {
            changes |= USCORE_BACK;
        } else if( info->langtype >= 1 && info->langtype <= 3 ) {
            changes |= UPPERCASE;
        }
    }
    #define USCORE "_"

    if( changes & USCORE_FRONT ) {
        additional_size++;
    }
    if( changes & USCORE_BACK ) {
        additional_size++;
    }
    if( changes & REM_USCORE_FRONT ) {
        if( *ptr == '_' ) {
            ptr++;
        }
    }

    if( buffer == NULL ) {
        name = AsmAlloc( strlen( ptr ) + additional_size );
    } else {
        name = buffer;
    }

    if( changes & USCORE_FRONT ) {
        strcpy( name, USCORE );
    } else {
        strcpy( name, "" );
    }
    strcat( name, ptr );
    if( changes & UPPERCASE ) {
        strupr( name );
    }
    if( changes & USCORE_BACK ) {
        strcat( name, USCORE );
    } else if( changes & REM_USCORE_BACK ) {
        ptr = name + strlen( name ) - 1;
        if( *ptr == '_' ) {
            *ptr = '\0';
        }
    }
    return( name );
}

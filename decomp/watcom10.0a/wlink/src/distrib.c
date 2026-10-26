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

static unsigned short CurrModHandle;     /* original450066 */
static mod_entry **ModTable;             /* original441df8 */
static void *ArcBuffer;                  /* original441df4 */
/* SectOvlTab is shared with the overlay numbering owner. */

void FreeDistStuff( void )
{
    unsigned index;

    for( index = 1; index <= CurrModHandle; ++index ) {
        FreeAMod( ModTable[index] );
    }
    _LnkFree( ModTable );
    _LnkFree( ArcBuffer );
    _LnkFree( SectOvlTab );
    ReleasePass1();
}

/* The 1994 per-module segment list also holds its distribution reference. */
#pragma pack(1)
typedef struct module_segments {
    void *segments;
    unsigned short ovlref;
} module_segments;
#pragma pack()

/* Released SetSegments retains the phase role; this older build also
 * redistributes modules and replays saved segment definitions here. */
void SetSegments( void )
{
    unsigned bytes, index;
    unsigned short ovlref;
    mod_entry *mod, **owner;
    section *sect;

    if( !(LinkFlags & 0x10004UL) ) return;
    ObjFormat = (ObjFormat & 0x7f7f) | 0x80;
    if( LinkFlags & 4 ) {
        _LnkFree( ArcBuffer );
        ArcBuffer = 0;
    }
    sect = CurrSect;
    if( LinkFlags & 0x10000UL ) {
        bytes = (NextSegIndex + 7) / 8;
        SavedSegMask = PermAlloc( bytes );
        memset( SavedSegMask, 0xff, bytes );
        NextSegIndex = 0;
        SetSectSegments( Root );
        ProcAllOvl( SetSectSegments );
        CurrSect = Root;
        SetModSegments( LibModules );
        sect = CurrSect;
    }
    CurrSect = sect;
    if( LinkFlags & 4 ) {
        for( index = 1; index <= CurrModHandle; ++index ) {
            mod = ModTable[index];
            CurrMod = mod;
            ovlref = ((module_segments *)mod->unknown16)->ovlref;
            if( ovlref == 0xffff ) sect = Root;
            else sect = SectOvlTab[ovlref];
            CurrSect = sect;
            SetModSegments( mod );
            sect = CurrSect;
            mod->unknown16 = 0;
            owner = &sect->u.dist_mods;
            while( *owner != 0 ) owner = (mod_entry **)&(*owner)->unknown16;
            *owner = mod;
            mod->next_mod = (mod_entry *)sect;
        }
    }
    FixGroupProblems();
    FindRedefs();
    if( LinkFlags & 4 ) {
        _LnkFree( SectOvlTab );
        SectOvlTab = 0;
    }
    ReleasePass1();
}

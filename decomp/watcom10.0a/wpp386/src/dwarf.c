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


/* Adapted from initial6b9cb443 dwarf.c/browsio.c. The1994 browser
 * emitter calls virtTblTypes directly, before the later initDwarf factoring. */
#include "compiler.h"
#include "frontend.h"
#include "scope.h"
#include "pending.h"
#include <recon.h>
typedef struct dw_client *dw_client;
typedef struct dw_loc_id *dw_loc_id;
typedef struct dw_loc_handle *dw_loc_handle;
static unsigned char InDebug;
static dw_client Client;
static dw_loc_handle dummyLoc;
void virtTblTypes(void) { STUB_TRAP(); }
dw_client DwarfInit(void) { STUB_TRAP(); return NULL; }
dw_loc_id DWLocInit(dw_client client)
{ (void)client; STUB_TRAP(); return NULL; }
dw_loc_handle DWLocFini(dw_client client,dw_loc_id loc)
{ (void)client; (void)loc; STUB_TRAP(); return NULL; }
void dwarfEmitFundamentalType(void) { STUB_TRAP(); }
void dwarfEmitSymbolScope(SCOPE scope) { (void)scope; STUB_TRAP(); }
void DWLocTrash(dw_client client,dw_loc_handle loc)
{ (void)client; (void)loc; STUB_TRAP(); }
void DwarfFini(dw_client client) { (void)client; STUB_TRAP(); }
void DwarfBrowseEmit(void)
{
    if (!CompFlags.emit_browser_info) return;
    virtTblTypes();
    InDebug=0;
    Client=DwarfInit();
    dummyLoc=DWLocFini(Client,DWLocInit(Client));
    dwarfEmitFundamentalType();
    dwarfEmitSymbolScope(FileScope);
    DWLocTrash(Client,dummyLoc);
    DwarfFini(Client);
}

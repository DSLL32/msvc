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

/* bld/plusplus/c/cgio.c at initial commit6b9cb443. The1994 body
 * clears only the file ring; later thunk/removed rings are absent. */
#include "compiler.h"
#include "types.h"
#include "initstate.h"
#include "carve.h"
#include "cgio.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>

static void *cg_file_ring;
static carve_t carveCGFILE, carveCGFILE_GEN;

void cgioInit(INITFINI *entry)
{
    (void)entry;
    cg_file_ring = 0;
    CgioBuffInit();
    /* Actual packed1994 element sizes, before those fields are encountered. */
    carveCGFILE = CarveCreate(46, 64);
    carveCGFILE_GEN = CarveCreate(156, 32);
}

void *RingCarveAlloc(carve_t cv,void **head);
CGFILE *initCGFILE(CGFILE *file,struct symbol *sym)
{
    CGIOBUFF *buffer;
    CGFILE_GEN *gen;
    void clearCounters(unsigned *vector,unsigned count);
    file->symbol=sym;
    buffer=CgioBuffWrOpen(sym);
    file->buffer=buffer;
    file->offset=buffer->free_offset;
    file->cursor=buffer->data+buffer->free_offset;
    file->first=buffer->disk_addr;
    /* Old flags contain ten initialized bits, before later thunk states. */
    file->flags&=~0x3ffU;
    file->defined.src_file=0;
    file->defined.line=0;
    file->defined.column=0;
    file->cond_flags=0;
    gen=CarveAlloc(carveCGFILE_GEN);
    file->gen=gen;
    clearCounters(gen->emitted_counters,10);
    clearCounters(gen->counters,10);
    gen->cs_label=0;
    gen->cs_allocated=0;
    gen->source_location.src_file=0;
    gen->emit_line_no.src_file=0;
    gen->initialization_location.src_file=0;
    gen->curr_type=0;
    gen->emit_type=0;
    gen->pending_value=0;
    gen->emitted_value=0;
    gen->init_sym=0;
    gen->flags&=~0x1fU;
    return file;
}
CGFILE *CgioCreateFile(struct symbol *sym)
{
    CGFILE *file;
    file=RingCarveAlloc(carveCGFILE,&cg_file_ring);
    return initCGFILE(file,sym);
}

CGIOBUFF *CgioBuffWriteIC(CGIOBUFF *buffer,const void *data,unsigned size);
void CgioWriteIC(CGFILE *file,const void *data,unsigned size)
{ file->buffer=CgioBuffWriteIC(file->buffer,data,size); }

void CgioBuffWrClose(CGIOBUFF *buffer);
void freeGenData(CGFILE *file)
{
    CarveFree(carveCGFILE_GEN,file->gen);
    file->gen=NULL;
}
int saveGenData(CGFILE *file)
{
    return file->symbol==NULL || file->symbol->id==7;
}
void CgioCloseOutputFile(CGFILE *file)
{
    CgioBuffWrClose(file->buffer);
    file->buffer=NULL;
    file->flags|=0x40;
    if(!saveGenData(file)) freeGenData(file);
}

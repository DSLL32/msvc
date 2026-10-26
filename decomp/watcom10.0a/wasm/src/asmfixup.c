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


/* BackPatch CF90cc..9228, adapted from released bld/wasm/c/asmfixup.c.
 * The1994 write guard tests Parse_Pass alone; the later !PhaseError
 * restriction must not be imported from2002. */
#include "asmparse.h"
#include "direct.h"
#include <recon.h>

/* CF9228..92bd.1994 always adds the offset, including FIX_SEG;
 * the released later source added a FIX_SEG exception. */
void mark_fixupp(unsigned long determinant, int index)
{
    struct asmfixup *fixup = InsFixups[index];
    if (fixup != NULL) {
        fixup->fix_loc = GetCurrAddr();
        Code->data[index] += fixup->offset;
        switch (determinant) {
        case OP_I16:
        case OP_J32:
            switch (fixup->fix_type) {
            case FIX_OFF32: fixup->fix_type = FIX_OFF16; break;
            case FIX_PTR32: fixup->fix_type = FIX_PTR16; break;
            }
            break;
        case OP_I32:
        case OP_J48:
            switch (fixup->fix_type) {
            case FIX_OFF16: fixup->fix_type = FIX_OFF32; break;
            case FIX_PTR16: fixup->fix_type = FIX_PTR32; break;
            }
            break;
        }
    }
}

/* CF9630..967d, append the owned relocation in noninitial passes. */
int store_fixup(int index)
{
    struct fixup *fixnode;
    if (Parse_Pass == PASS_1) return NOT_ERROR;
    if (InsFixups[index] == NULL) return NOT_ERROR;
    fixnode = CreateFixupRec(index);
    if (fixnode == NULL) return ERROR;
    if (FixupListHead == NULL) {
        FixupListTail = FixupListHead = fixnode;
    } else {
        FixupListTail->next = fixnode;
        FixupListTail = fixnode;
    }
    return NOT_ERROR;
}

int BackPatch(asm_sym *sym)
{
    long disp, max_disp;
    unsigned patch_addr, size;
    struct asmfixup *patch, *next;
    dir_node *seg;

    patch = sym->fixup;
    sym->fixup = NULL;
    for (; patch != NULL; patch = next) {
        next = patch->next;
        seg = GetSeg(sym);
        if (seg == NULL || patch->def_seg != seg) {
            patch->next = sym->fixup;
            sym->fixup = patch;
            continue;
        }
        size = 0;
        switch (patch->fix_type) {
        case FIX_RELOFF8:
            size = 1;
        case FIX_RELOFF16:
            if (size == 0) size = 2;
        case FIX_RELOFF32:
            if (size == 0) size = 4;
            patch_addr = patch->fix_loc;
            disp = patch->offset + GetCurrAddr() - patch_addr - size;
            max_disp = (1UL << (size * 8 - 1)) - 1;
            if (disp > max_disp || disp < -max_disp - 1) {
                PhaseError = TRUE;
                switch (size) {
                case 1:
                    if (Code->use32) {
                        sym->offset += 3;
                        AsmByte(0);
                        AsmByte(0);
                    } else {
                        sym->offset += 1;
                        AsmByte(0);
                    }
                    break;
                case 2:
                    sym->offset += 2;
                    AsmByte(0);
                    AsmByte(0);
                case 4:
                    AsmWarn(4, 68);
                    return NOT_ERROR;
                }
            }
            if (Parse_Pass != PASS_1) {
                while (size > 0) {
                    CodeBuffer[patch_addr] = disp;
                    disp >>= 8;
                    patch_addr++;
                    size--;
                }
            }
            AsmFree(patch);
            break;
        default:
            patch->next = sym->fixup;
            sym->fixup = patch;
            break;
        }
    }
    return NOT_ERROR;
}

/* CF8fe4..90c9; allocation and fields match the original25B owner. */
struct asmfixup *AddFixup(asm_sym *sym, int type)
{
    struct asmfixup *fixup;
    switch (Code->info.token) {
    case T_CMPS: case T_LODS: case T_MOVS: case T_OUTS:
    case T_INS2: case T_SCAS: case T_STOS: case T_XLAT:
        return NULL;
    }
    fixup = AsmAlloc(sizeof(*fixup));
    if (fixup != NULL) {
        fixup->external = 0;
        fixup->name = sym->name;
        fixup->offset = sym->offset;
        if (fixup->offset == 0xa5a5a5a5UL)
            InternalError("..\\c\\asmfixup.c",112);
        fixup->def_seg = CurrSeg != NULL ? CurrSeg->seg : NULL;
        fixup->frame = Frame;
        fixup->frame_datum = Frame_Datum;
        fixup->next = sym->fixup;
        sym->fixup = fixup;
        fixup->fix_type = type;
        InsFixups[Opnd_Count] = fixup;
    }
    return fixup;
}

/* CF8fb0..8fe3, deferred frame update after the initial pass. */
void add_frame(void)
{
    struct asmfixup *fixup;
    if (Parse_Pass != PASS_1) {
        fixup = InsFixups[Opnd_Count];
        if (fixup == NULL) return;
        fixup->frame = Frame;
        fixup->frame_datum = Frame_Datum;
    }
}

/* CF92e0..962f, with the eight-entry table at92c0. Adapted from the
 * initial released source; actual1994 Address access calls GetCurrAddr. */
struct fixup *CreateFixupRec( int index )
/* Create a fixup record for WOMP;
   Note that if Modend is TRUE, it means the fixup is the starting address
   for the module.
*/
{
    struct asmfixup     *fixup;         // fixup structure from WASM
    struct fixup        *fixnode;       // fixup structure from WOMP
    struct asm_sym      *sym;

    fixup = InsFixups[index];

    if( fixup != NULL ) {

        fixnode = FixNew();
        fixnode->self_relative = FALSE;
        fixnode->lr.target_offset = 0;
        if( !Modend ) {
            fixnode->lr.is_secondary = TRUE;
        } else {
            fixnode->lr.is_secondary = FALSE;
            fixnode->lr.target_offset = fixup->offset;
        }

        if( !Modend ) {
            switch( fixup->fix_type ) {
                case FIX_RELOFF8:
                    fixnode->self_relative = TRUE;
                    fixnode->loc_method = FIX_LO_BYTE;
                    break;
                case FIX_RELOFF16:
                    fixnode->self_relative = TRUE;
                case FIX_OFF16:
                    fixnode->loc_method = FIX_OFFSET;
                    break;
                case FIX_RELOFF32:
                    fixnode->self_relative = TRUE;
                case FIX_OFF32:
                    fixnode->loc_method = FIX_OFFSET386;
                    break;
                case FIX_SEG:
                    fixnode->loc_method = FIX_BASE;
                    break;
                case FIX_PTR16:
                    fixnode->loc_method = FIX_POINTER;
                    break;
                case FIX_PTR32:
                    fixnode->loc_method = FIX_POINTER386;
                    break;
            }
        }

        sym = AsmLookup( fixup->name );
        if( sym == NULL ) return NULL;

        fixnode->loader_resolved = FALSE;

        fixnode->loc_offset = GetCurrAddr() - GetCurrSegStart();

        /*------------------------------------*/
        /* Determine the Target and the Frame */
        /*------------------------------------*/

        if( sym->state == SYM_UNDEFINED ) {
            AsmErr( 1148, sym->name );
            return( NULL );
        } else if( sym->state == SYM_GRP ) {

            if( Modend ) {
                AsmError( 1048 );
                return NULL;
            }

            fixnode->lr.frame = F_GRP;
            fixnode->lr.target = T_GRP;
            fixnode->lr.frame_datum =
            fixnode->lr.target_datum = GetDirIdx( fixup->name, TAB_GRP );

        } else if( sym->state == SYM_SEG ) {

            if( Modend ) {
                AsmError( 1048 );
                return NULL;
            }

            fixnode->lr.frame = F_SEG;
            fixnode->lr.target = T_SEG;
            fixnode->lr.frame_datum =
            fixnode->lr.target_datum = GetDirIdx( fixup->name, TAB_SEG );

        } else {

            /* symbol is a label */

            if( sym->state == SYM_EXTERNAL ) {
                if( Modend ) {
                    if( sym->mem_type == T_BYTE ||
                        sym->mem_type == T_SBYTE ||
                        sym->mem_type == T_WORD ||
                        sym->mem_type == T_SWORD ||
                        sym->mem_type == T_DWORD ||
                        sym->mem_type == T_SDWORD ||
                        sym->mem_type == T_FWORD ||
                        sym->mem_type == T_PWORD ||
                        sym->mem_type == T_QWORD ||
                        sym->mem_type == T_TBYTE ||
                        sym->mem_type == T_ABS2 ) {

                        AsmError( 1053 );
                        return NULL;
                    }
                    fixnode->lr.target = T_EXTWD;
                } else {
                    fixnode->lr.target = T_EXT;
                }
                fixnode->lr.target_datum = GetDirIdx( fixup->name, TAB_EXT );

                if( fixup->frame == F_GRP && fixup->frame_datum == 0 ) {
                    /* set the frame to the frame of the corresponding segment */
                    fixup->frame_datum = GetGrpIdx( sym );
                }
            } else {
                if( sym->segidx == 0 )
                    InternalError("..\\c\\asmfixup.c",431);
                if( Modend ) {
                    fixnode->lr.target = T_SEGWD;
                    fixup->frame = F_TARG;
                } else {
                    fixnode->lr.target = T_SEG;
                }
                fixnode->lr.target_datum = sym->segidx;
            }

            if( fixup->frame != EMPTY ) {
                fixnode->lr.frame = (unsigned char)fixup->frame;
            } else {
                fixnode->lr.frame = F_TARG;
            }
            fixnode->lr.frame_datum = fixup->frame_datum;


            if( Modend ) return fixnode;
        }

        /*--------------------*/
        /* Optimize the fixup */
        /*--------------------*/

        if( fixnode->lr.frame == ( fixnode->lr.target - 4 ) ) {
            fixnode->lr.frame = F_TARG;
        }

        fixnode->next = NULL;

        return fixnode;
    }
    return( NULL );
}

/* CF9680..96cb; preserve the ambient operand/frame around the extra
 * third instruction-fixup slot used by the x87 emulation patch. */
int MakeFpFixup(asm_sym *sym)
{
    int old_count;
    signed char old_frame;
    old_count = Opnd_Count;
    old_frame = Frame;
    Opnd_Count = 2;
    Frame = F_LOC;
    AddFixup(sym, FIX_OFF16);
    Frame = old_frame;
    Opnd_Count = old_count;
    if (store_fixup(2) == ERROR) return ERROR;
    return NOT_ERROR;
}

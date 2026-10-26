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


/* operator CFc708..c8b0, adapted to the1994 body without later MMX guards. */
#include <limits.h>
#include "asmparse.h"
#include "direct.h"
#include <recon.h>

int operator(int mem_type, unsigned char fix_mem_type)
{
    if (mem_type == T_PTR) {
        if (Code->use32 && MEM_TYPE(Code->mem_type,WORD)) {
            switch (Code->info.token) {
            case T_MOVSX: case T_MOVZX: case T_LEA: break;
            default: Code->opsiz = NOT_EMPTY;
            }
        } else if (!Code->use32 && MEM_TYPE(Code->mem_type,DWORD)) {
            if (!IS_BRANCH(Code->info.token)) {
                switch (Code->info.token) {
                case T_LDS: case T_LES: case T_LFS: case T_LGS: case T_LSS: break;
                default: Code->opsiz = NOT_EMPTY;
                }
            } else if (Code->distance == EMPTY) {
                mem_type = T_FAR;
            }
        } else if (!Code->use32 &&
                   (Code->mem_type == T_FWORD || Code->mem_type == T_PWORD)) {
            Code->opsiz = NOT_EMPTY;
        }
    } else if (mem_type == T_FAR || mem_type == T_NEAR || mem_type == T_SHORT) {
        Code->distance = mem_type;
    } else if (mem_type != 0 && !Code->mem_type_fixed && mem_type != T_STRUCT) {
        Code->mem_type = mem_type;
        if (fix_mem_type) Code->mem_type_fixed = TRUE;
    }
    if (mem_type == T_FAR || mem_type == T_FWORD || mem_type == T_PWORD) {
        Code->distance = T_FAR;
        if (Code->info.token == T_CALL || Code->info.token == T_JMP)
            ++Code->info.token;
    }
    return NOT_ERROR;
}

/* CFbd94..c707.1994 adjusts JCXZ only, and its extra opcode byte
 * test uses the adjusted displacement. The later loop-width handling
 * and unconditional range-validation block are absent. */
int jmp( int i )                // Bug: can't handle indirect jump
/*
  determine the displacement of jmp;
*/
{
    struct asm_sym      *sym;
    long              addr;
    long              temp;
    int      state;
    #if 1
        dir_node                *seg;
        unsigned char           old_use32;
    #endif

    sym = AsmLookup( AsmBuffer[i]->string_ptr );
    if( sym == NULL ) return( ERROR );

#if 1
    if( sym->mem_type == ERROR ) {
        AsmError( 130 );
        return ERROR;
    }
#endif
    if( Code->info.token == T_CALLF || Code->info.token == T_JMPF ) {
        if( Code->distance == EMPTY ) {
            Code->distance = T_FAR;
        }
    }
    state = sym->state;
    #if 1
        seg = GetSeg( sym );
        if( seg == NULL || CurrSeg == NULL || CurrSeg->seg != seg ) {
            /* jumps to another segment are just like to another file */
            state = SYM_EXTERNAL;
        }
    #endif

    switch( state ) {
    case SYM_INTERNAL:
    #if 1
        case SYM_PROC:
    #endif
        if(  ( Code->distance == EMPTY || Code->distance == T_SHORT
                || Code->distance == T_NEAR ) &&
            Code->mem_type == EMPTY &&
            sym->mem_type != T_WORD &&
            sym->mem_type != T_DWORD &&
            sym->mem_type != T_FWORD &&
            sym->mem_type != T_PWORD &&
            sym->mem_type != T_FAR ) {
            addr = sym->offset;
            addr -= GetCurrAddr() + 2;
            if (Code->info.token == T_JCXZ && Code->use32) --addr;
            if (Code->info.token == T_CALL && Code->distance == EMPTY)
                Code->distance = T_NEAR;
            if (Code->distance != T_NEAR && Code->info.token != T_CALL &&
                addr >= SCHAR_MIN && addr <= SCHAR_MAX) {
                Code->info.opnd_type[Opnd_Count] = OP_I8;
            } else {
                if (Code->use32) {
                    Code->info.opnd_type[Opnd_Count] = OP_I32;
                    addr -= 3;
                } else {
                    Code->info.opnd_type[Opnd_Count] = OP_I16;
                    --addr;
                }
                if (IS_JMP(Code->info.token)) {
                    switch (Code->info.token) {
                    case T_JMP: case T_JMPF: case T_JCXZ: case T_JECXZ: break;
                    default:
                        if (addr < SCHAR_MIN || addr > SCHAR_MAX) --addr;
                    }
                }
            }
            if ((Code->info.cpu & P_CPU_MASK) < P_386 && IS_JMP(Code->info.token)) {
                switch (Code->info.token) {
                case T_JMP: case T_JMPF: break;
                case T_JCXZ: case T_JECXZ:
                    if (addr < SCHAR_MIN || addr > SCHAR_MAX) {
                        AsmError(68);
                        return ERROR;
                    }
                    break;
                default:
                    if (addr < SCHAR_MIN || addr > SCHAR_MAX) {
                        if (Code->distance == EMPTY) {
                            jumpExtend();
                            return SCRAP_INSTRUCTION;
                        } else if (!PhaseError) {
                            AsmError(68);
                            return ERROR;
                        }
                    }
                    Code->info.opnd_type[Opnd_Count] = OP_I8;
                }
            }
            Code->data[Opnd_Count] = addr;
            break;
        }
        /* otherwise fall through & get handled like external symbols */
    case SYM_UNDEFINED:
    case SYM_EXTERNAL:

        /* forward ref, or external symbol */
        if( Code->mem_type == EMPTY && Code->distance == EMPTY &&
            sym->mem_type != 0 ) {
            switch( sym->mem_type ) {
            case T_FAR:
                if( Code->info.token == T_CALL || Code->info.token == T_JMP ) {
                    Code->info.token++;
                }
                // fall through
            case T_SHORT:
            case T_NEAR:
                Code->distance = sym->mem_type;
                break;
            case T_FWORD:
            case T_PWORD:
                if( operator( T_FWORD, TRUE ) == ERROR ) return( ERROR );
                break;
            default:
                Code->mem_type = sym->mem_type;
            }
        }
        if( operator( T_PTR, FALSE ) == ERROR ) return( ERROR ); // fixme

        if( !Code->use32 && MEM_TYPE( Code->mem_type, DWORD ) ) {
            Code->distance = T_FAR;
            if( Code->info.token == T_CALL  ||  Code->info.token == T_JMP ) {
                Code->info.token++;
            }
        }

        if( Code->info.token == T_CALL && Code->distance == EMPTY ) {
            Code->distance = T_NEAR;
        }

        switch( Code->info.token ) {
        case T_CALLF:
        case T_JMPF:

#if 1

            old_use32 = Code->use32;
            seg = GetSeg( sym );
            if( seg != NULL ) {
                Code->use32 = seg->e.seginfo->segrec->d.segdef.use_32;
            }
#endif

            switch( Code->distance ) {
            case T_SHORT:
            case T_NEAR:
                AsmError( 67 );
                return( ERROR );
            case EMPTY:
            case T_FAR:
                switch( Code->mem_type ) {
                case T_BYTE:
                case T_WORD:
#if 1
                case T_SBYTE:
                case T_SWORD:
#endif
                    AsmError( 148 );
                    return( ERROR );
                case T_DWORD:
                case T_FWORD:
                case T_PWORD:
#if 1
                case T_SDWORD:
#endif
                    return( INDIRECT_JUMP );
                case T_QWORD:
                case T_TBYTE:
                    AsmError( 148 );
                    return( ERROR );
                case EMPTY:
                    if( Code->use32 ) {
                        temp = FIX_PTR32;
                        Code->info.opnd_type[Opnd_Count] = OP_J48;
                        #if 1
                        if( !old_use32 ) {
                            Code->opsiz = NOT_EMPTY;
                        }
                        #endif
                    } else {
                        temp = FIX_PTR16;
                        Code->info.opnd_type[Opnd_Count] = OP_J32;
                        #if 1
                        if( old_use32 ) {
                            Code->opsiz = NOT_EMPTY;
                        }
                        #endif
                    }
                }
                break;
            }
            AddFixup( sym, temp );
            break;
        case T_CALL:
            if( Code->mem_type == T_SHORT ) {
                AsmError( 49 );
                return( ERROR );
            }
            /* fall through */
        case T_JMP:
            switch( Code->distance ) {
            case T_SHORT:
                temp = FIX_RELOFF8;
                Code->info.opnd_type[Opnd_Count] = OP_I8;
                break;
            case EMPTY:
                /* guess short, we will expand later if needed */
                if( Code->mem_type == EMPTY && Code->info.token == T_JMP && state != SYM_EXTERNAL ) {
                    temp = FIX_RELOFF8;
                    Code->info.opnd_type[Opnd_Count] = OP_I8;
                    break;
                }
            case T_NEAR:
                switch( Code->mem_type ) {
                case EMPTY:
                    /* near */
                    if( Code->use32 ) {
                        temp = FIX_RELOFF32;
                        Code->info.opnd_type[Opnd_Count] = OP_I32;
                    } else {
                        temp = FIX_RELOFF16;
                        Code->info.opnd_type[Opnd_Count] = OP_I16;
                    }
                    break;
                case T_DWORD:
                case T_WORD:
#if 1
                case T_SDWORD:
                case T_SWORD:
#endif
                    return( INDIRECT_JUMP );
#if 1
                case T_SBYTE:
#endif
                case T_BYTE:
                case T_FWORD:
                case T_PWORD:
                case T_QWORD:
                case T_TBYTE:
                    AsmError( 148 );
                    return( ERROR );
                default:
                    // something wierd?
                    if( Code->use32 ) {
                        temp = FIX_RELOFF32;
                        Code->info.opnd_type[Opnd_Count] = OP_I32;
                    } else {
                        temp = FIX_RELOFF16;
                        Code->info.opnd_type[Opnd_Count] = OP_I16;
                    }
                    break;
                }
                break;
            case T_FAR:
                AsmError( 51 );
#if 1
                InternalError("..\\c\\asmjump.c",429);
#endif
            }
            AddFixup( sym, temp );
            break;
        case T_JCXZ:
        case T_JECXZ:
            // JCXZ and JECXZ always require SHORT label
        case T_LOOP:
        case T_LOOPE:
        case T_LOOPNE:
        case T_LOOPNZ:
        case T_LOOPZ:










            if( Code->mem_type != EMPTY && Code->mem_type != T_SHORT ) {
                AsmError( 50 );
                return( ERROR );
            }
            Code->info.opnd_type[Opnd_Count] = OP_I8;
            AddFixup( sym, FIX_RELOFF8 );
            break;
        default:
            if( (Code->info.cpu&P_CPU_MASK) >= P_386 ) {
                switch( Code->distance ) {
                case EMPTY:
                case T_SHORT:
                    temp = FIX_RELOFF8;
                    Code->info.opnd_type[Opnd_Count] = OP_I8;
                    break;
                case T_NEAR:
                    temp = FIX_RELOFF32;
                    Code->info.opnd_type[Opnd_Count] = OP_I32;
                    break;
                default:
                    if( Code->use32 ) {
                        temp = FIX_RELOFF32;
                        Code->info.opnd_type[Opnd_Count] = OP_I32;
                    } else {
                        temp = FIX_RELOFF16;
                        Code->info.opnd_type[Opnd_Count] = OP_I16;
                    }
                }
            } else {
                // the only mode in 8086, 80186, 80286 is
                // Jxx SHORT
                temp = FIX_RELOFF8;
                Code->info.opnd_type[Opnd_Count] = OP_I8;
            }
            AddFixup( sym, temp );
        }
        break;
    default: /* SYM_STACK */
        AsmError( 74 );
        return( ERROR );
    }
    if( Code->mem_type != EMPTY ) {
        if( mem2code( SCALE_FACTOR_1, EMPTY, EMPTY ) == ERROR ) {
            return( ERROR );
        }
    }
    return( NOT_ERROR );
}

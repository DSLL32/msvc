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
* Description:  Track incoming floating-point parameters.
*
****************************************************************************/


#include "block.h"
#include "callnode.h"
#include "codegen.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

/* Eight parameter slots plus the original permanently NULL sentinel. */
name *Parm8087[9];
int FPRegNum(name *op);
void FPParms(void)
{
    instruction *ins,*next;
    int i;
    if (CurrProc->label==HeadBlock->label) {
        i=8;
        while (--i>=0) Parm8087[i]=NULL;
        ins=HeadBlock->ins.hd.next;
        i=0;
        while (ins->head.opcode!=0x4b) {
            if (ins->head.opcode==0x2c && FPRegNum(ins->result)==0) {
                next=ins->head.next;
                if (next->head.opcode==0x4b ||
                    (instruction *)ins->operands[2]!=next ||
                    (instruction *)next->operands[2]!=ins)
                    Parm8087[i]=(name *)CurrProc; /* Real ignored-parm marker. */
                else {
                    Parm8087[i]=next->result;
                    DoNothing(next);
                    ins=next;
                }
                ++i;
            }
            ins=ins->head.next;
        }
    }
}

/* i87opt.c has a private FSinCos peephole distinct from i87reg.c's pass. */
static opcode_entry FSINCOS={0x8000,0,98,0,9};
instruction *PrefFXCH(instruction *ins,int actual);
void FreeIns(instruction *ins);
#define OTHER_TRIG(op) ((op)==0x15 ? 0x14 : 0x15)
unsigned char OptFSinCos(instruction *ins)
{
    instruction *ins2,*ins3,*ins4;
    if (!(TargetModel&1)) return 0;
    ins2=ins->head.next;
    ins3=ins2->head.next;
    ins4=ins3->head.next;
    if (ins->u.gen_table->generate==44) {
        if (ins2->head.opcode!=0x15 && ins2->head.opcode!=0x14) return 0;
        if (ins3->u.gen_table->generate!=44) return 0;
        if (ins->operands[0]!=ins3->operands[0]) return 0;
        if (ins4->head.opcode!=OTHER_TRIG(ins2->head.opcode)) return 0;
    } else if (ins->u.gen_table->generate==45) {
        if (ins2->head.opcode!=0x15 && ins2->head.opcode!=0x14) return 0;
        if (ins3->u.gen_table->generate!=45) return 0;
        if (FPRegNum(ins->operands[0])!=FPRegNum(ins3->operands[0])-1) return 0;
        if (ins4->head.opcode!=OTHER_TRIG(ins2->head.opcode)) return 0;
    } else if (ins->head.opcode==0x15) {
        if (ins2->u.gen_table->generate!=45) return 0;
        if (FPRegNum(ins2->operands[0])!=1) return 0;
        if (ins3->head.opcode!=OTHER_TRIG(ins->head.opcode)) return 0;
        ins4=ins3;
        ins3=ins2;
        ins2=ins;
    } else return 0;
    ins2->u.gen_table=&FSINCOS;
    if (ins2->head.opcode!=0x15) PrefFXCH(ins3,1);
    FreeIns(ins3);
    FreeIns(ins4);
    return 1;
}
#undef OTHER_TRIG
/* Recovered i87opt.c peepholes, retaining1994 generator numbers. */
enum {
    G_RRFBIN=36,G_RNFBIN=37,G_RRFBINP=38,G_RNFBINP=39,
    G_MRFBIN=42,G_MNFBIN=43,G_MFLD=44,G_RFLD=45,
    G_MFST=47,G_RFST=48,G_MFSTNP=60,G_FXCH=100
};
#define GEN(x) ((x)->u.gen_table->generate)
unsigned char FPResultNotNeeded(instruction *ins);
name *ST(int reg);
instruction *Next87Ins(instruction *ins)
{
    instruction *next;
    next=ins->head.next;
    for (;;) {
        if (next->head.opcode==0x4b) return ins;
        if (DoesSomething(next)) break;
        next=next->head.next;
    }
    return next;
}
unsigned char RedundantStore(instruction *ins) { STUB_TRAP();return 0; }
void AdjustST(name **op,int adjust) { STUB_TRAP(); }
instruction *BackUpAndFree(instruction *ins,instruction *junk1,instruction *junk2) { STUB_TRAP();return NULL; }
instruction *To86Move(instruction *ins,instruction *next) { STUB_TRAP();return NULL; }
void NoPopRBin(instruction *ins) { STUB_TRAP(); }
void NoPopBin(instruction *ins) { STUB_TRAP(); }
void ToRFld(instruction *ins) { STUB_TRAP(); }
void NoMemBin(instruction *ins) { STUB_TRAP(); }
void ReverseFPGen(instruction *ins) { STUB_TRAP(); }
void NoPop(instruction *ins) { STUB_TRAP(); }
void ToPopBin(instruction *ins);

instruction *Opt87Sequence( instruction *ins, unsigned char *again ) {
/************************************************************************
    Look for silly 8087 sequences and fix them up.
*/

    instruction         *next;
    instruction         *third;
    instruction         *ret;
    unsigned char       class;
    int                 num_ops;

    next = Next87Ins( ins );
    ret = ins->head.next;
    if( next == ins ) return( ret );
    if( GEN( ins ) == G_RFLD && FPRegNum( ins->operands[ 0 ] ) == 0 ) {
        if( GEN( next ) == G_MFST ) {

            /* FLD ST, FSTP X ===> FST X */

            FreeIns( ins );
            if( FPResultNotNeeded( next ) ) {
                ret = BackUpAndFree( ins, next, NULL );
            } else {
                ret = next;
                NoPop( next );
            }
            *again = 1;
        } else if( GEN( next ) == G_RFST ) {

            /* FLD ST, FSTP ST(i) ===> FST ST(i-1) */

            if( FPRegNum( next->result ) == 0 ) {
                ret = BackUpAndFree( ins, ins, next );
            } else {
                AdjustST( &next->result, -1 );
                FreeIns( ins );
                ret = next;
                NoPop( next );
            }
            *again = 1;
        } else if( GEN( next ) == G_RNFBINP || GEN( next ) == G_RRFBINP ) {

            /* FLD ST, FopP ST(i),ST ==> Fop ST(i-1),ST */
            if( FPRegNum( next->operands[ 0 ] ) == 0 ) {
                ret = BackUpAndFree( ins, ins, next );
            } else {
                AdjustST( &next->operands[ 0 ], -1 );
                NoPopRBin( next );
                FreeIns( ins );
                ret = next;
            }
            *again = 1;
        } else if( GEN( next ) == G_FXCH && FPRegNum( next->result ) == 1 ) {
            FreeIns( next );
            *again = 1;
            ret = ins;
        }
    } else if( GEN( ins ) == G_RFLD || GEN( ins ) == G_MFLD ) {
        if( ( GEN( next ) == G_RRFBINP || GEN( next ) == G_RNFBINP ) ) {
            if( GEN( ins ) == G_MFLD ) {
                class = ins->operands[ 0 ]->n.name_class;
                if( !(class==FS || class==FD) ) return( ret ); /* need convert! */
            }
            if( ( FPRegNum( next->operands[ 0 ] ) == 1 ) ) {

                /* FLD X, FxxP ST(1) ==> Fxx X */

                next->result = ST( 0 );
                next->operands[ 0 ] = ins->operands[ 0 ];
                num_ops = NumOperands( ins );
                if( ins->num_operands > num_ops
                 && next->num_operands <= NumOperands( next ) ) {
                    next->operands[ next->num_operands ]
                                     = ins->operands[ num_ops ];
                    next->num_operands++;
                }
                FreeIns( ins );
                NoPopBin( next );
                *again = 1;
                ret = next;
            }
        } else if( GEN( next ) == G_RFLD || GEN( next ) == G_MFLD ) {
            if( GEN( ins ) == G_MFLD && GEN( next ) == G_MFLD &&

                /* FLD X, FLD X ==> FLD X, FLD ST(0) */

                ins->operands[0] == next->operands[0] ) {
                next->operands[0] = ST( 0 );
                ToRFld( next );
                *again = 1;
                ret = next;
            } else {

                /* FLD X, FLD Y, FXCH ST(1) ==> FLD Y, FLD X */
                third = Next87Ins( next );
                if( third == next ) return( ret );
                if( GEN( third ) == G_FXCH && FPRegNum( third->result ) == 1 ) {
                    FreeIns( third );
                    if( FPRegNum( next->operands[0] ) != 0 ) {
                        ins->head.next->head.prev = ins->head.prev;
                        ins->head.prev->head.next = ins->head.next;
                        SuffixIns( next, ins );
                        if( GEN( ins ) == G_RFLD ) {
                            AdjustST( &ins->operands[0], +1 );
                        }
                        if( GEN( next ) == G_RFLD ) {
                            AdjustST( &next->operands[0], -1 );
                        }
                        ret = next;
                    } else {
                        ret = ins;
                    }
                    *again = 1;
                }
            }
        } else if( GEN( ins ) == G_MFLD ) {
            if( GEN( next ) == G_MFST ) {
                if(ins->operands[0]->n.name_class==next->result->n.name_class) {
                    ret = To86Move( ins, next );
                }
            } else if( GEN( next ) == G_MRFBIN || GEN( next ) == G_MNFBIN ) {
                if( ins->operands[0] == next->operands[ 0 ] ) {

                    /* FLD X, FOP X ==> FLD X, FOP ST */

                    next->operands[ 0 ] = ST( 0 );
                    DelSeg( next );
                    NoMemBin( next );
                    *again = 1;
                    ret = ins;
                }
            }
        }
    } else if( GEN( ins ) == G_MFST || GEN( ins ) == G_RFST ) {
        if( GEN( next ) == G_MFLD
        && ( next->operands[ 0 ] == ins->result )
        && !IsVolatile( ins->result ) ) {

            /* FSTP X, FLD X ==> FST X */

            FreeIns( next );
            if( FPResultNotNeeded( ins ) ) {
                ret = BackUpAndFree( ins, ins, NULL );
            } else {
                ret = ins;
                NoPop( ins );
            }
            *again = 1;
        } else if( GEN( next ) == G_RFLD
             && FPRegNum( next->operands[0] ) + 1 == FPRegNum( ins->result ) ) {

            /* FSTP ST(i), FLD ST(i-1) ==> FST ST(i) */

            FreeIns( next );
            NoPop( ins );
            *again = 1;
            ret = ins;
        }
    } else if( GEN( ins ) == G_FXCH ) {
        if( GEN( next ) == G_FXCH ) {

            /* FXCH ST(i) FXCH ST(i) => nothing */

            if( FPRegNum( ins->result ) == FPRegNum( next->result ) ) {
                FreeIns( next );
                next = BackUpAndFree( ins, ins, NULL );
                *again = 1;
                ret = next;
            }
        } else {
            if( GEN( next ) == G_RRFBINP || GEN( next ) == G_RNFBINP ) {

                if( FPRegNum( ins->result ) == FPRegNum( next->operands[0] ) ) {

                    /* FXCH ST(i), FopP ST(i),ST -> FopRP ST(i),ST */
                    FreeIns( ins );
                    ReverseFPGen( next );
                    *again = 1;
                    ret = next;
                }
            }
        }
    } else if( GEN( ins ) == G_MFSTNP ) {
        if( GEN( next ) == G_MFLD && ins->result == next->operands[0] ) {

            /* FST X, FLD X => FST X, FLD ST */

            next->operands[0] = ST( 0 );
            ToRFld( next );
            *again = 1;
            ret = ins;
        } else if( RedundantStore( ins ) ) {
            ret = BackUpAndFree( ins, ins, NULL );
            *again = 1;
        }
    } else if( GEN( ins ) == G_RNFBIN || GEN( ins ) == G_RRFBIN ) {
        if( GEN( next ) == G_RFST && next->result == ins->operands[0] ) {
            ToPopBin( ins );
            FreeIns( next );
            *again = 1;
            ret = ins;
        }
    }
    return( ret );
}

#undef GEN
unsigned char FPResultNotNeeded(instruction *ins);
name *ST(int reg);
void ToRFstp(instruction *ins);
void Opt8087(void)
{
    block *blk;
    instruction *ins;
    int i;
    unsigned char again,generate;
    blk=HeadBlock;
    while (blk!=NULL) {
        i=0;
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) ins->sequence=++i;
        ins=blk->ins.hd.next;
        again=0;
        while (ins->head.opcode!=0x4b) {
            generate=ins->u.gen_table->generate;
            if ((generate>=36 && generate<=62) || (generate>=96 && generate<=100)) {
                if (!OptFSinCos(ins)) ins=Opt87Sequence(ins,&again);
                else ins=ins->head.next;
            } else {
                ins=ins->head.next;
            }
        }
        if (!again) {
            for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
                if (FPResultNotNeeded(ins)) {
                    ins->result=ST(0);
                    ToRFstp(ins);
                }
            }
            blk=blk->next_block;
        }
    }
}
void Wait8087(void);
void FPOptimize(void)
{
    if ((CGProcessorVersion&0x70U)>=0x10U) {
        if (!(Model&0x80000000U)) Opt8087();
        Wait8087();
    }
}

extern hw_reg_set FPRegs[8];
unsigned char CanPushImmed(struct parm_node *parm,int *count);
int FPPushImmed(struct parm_node *parm);
int FPPushDelay(struct parm_node *parm);
void FPPushParms(struct parm_node *parm,call_state *state)
{
    int count;
    /* The earlier body has no later ROUTINE_STACK_RESERVE condition. */
    if (((CGProcessorVersion&0xf0U)&0x7fU)>=0x10U) {
        count=0;
        if (CanPushImmed(parm,&count) && count<Max87Stk)
            count=FPPushImmed(parm);
        else if (count!=0) count=FPPushDelay(parm);
        state->parm.used&=~0x00ff0000U;
        while (--count>=0) state->parm.used|=FPRegs[count];
    }
}
unsigned char CanPushImmed(struct parm_node *parm,int *count)
{
    instruction *next;
    struct address_name *addr;
    while (parm!=NULL) {
        if (parm->ins==NULL) {
            addr=parm->name;
            if (addr->tipe->attr&1) {
                ++*count;
                next=addr->u.ins->head.next;
                while (next->head.opcode!=0x4b) {
                    if (next->head.opcode==0x36 || next->head.opcode==0x29)
                        return 0;
                    if (next->head.opcode>=0x0e && next->head.opcode<=0x20 &&
                        FPStkReq(next)!=0) return 0;
                    next=next->head.next;
                }
                if (((block_ins *)next)->blk!=CurrBlock) return 0;
            }
        }
        parm=parm->next;
    }
    return 1;
}
void Pushes(instruction *ins);
int FPPushImmed(struct parm_node *parm)
{
    struct parm_node *next;
    struct address_name *addr;
    int count;
    count=0;
    while (parm!=NULL) {
        if (parm->ins!=NULL) {
            parm=parm->next;
            continue;
        }
        next=parm->next;
        addr=parm->name;
        if ((addr->tipe->attr&1) && (parm->regs&0x00ff0000U)) {
            if (addr->format!=4) Zoiks(43);
            addr->u.ins->result=AllocRegName(0x00010000U); /* HW_ST0. */
            Pushes(addr->u.ins);
            parm->ins=addr->u.ins;
            addr->format=0;
            BGDone(addr);
            ++count;
        }
        parm=next;
    }
    return count;
}
void Pushes(instruction *ins) { STUB_TRAP(); }
int FPPushDelay(struct parm_node *parm) { STUB_TRAP();return 0; }

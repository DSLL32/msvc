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
#include <stdio.h>
#include "compiler.h"
#include "memory.h"
#include "macros.h"
#include "scanner.h"
#include "files.h"
#include "quads.h"
#include "expression.h"
#include "statement.h"
#include "declarations.h"
#include "options.h"
#include <recon.h>
extern int FuncCount;

/* Quad-stream predecessor of cstmt2.c Statement: return_info is one byte,
 * labels are leaf handles, and the final function quad is linked explicitly. */
int TryCount,TryScope,LoopDepth;
struct block_definition *LoopStack;
struct switch_definition *SwitchStack;

void Statement(void)
{
    unsigned end_of_func_label,func_result;
    char return_type,return_at_outer_level,skip_to_next_token;
    StmtInit();
    return_type=0;
    return_at_outer_level=0;
    TryCount=TryScope=-1;
    CompFlags.label_dropped=0;
    CompFlags.addr_of_auto_taken=0;
    ++FuncCount;
    end_of_func_label=0;
    func_result=0;
    DeadCode=0;
    LoopDepth=0;
    BlockStack=LoopStack=NULL;
    SwitchStack=NULL;
    for (;;) {
        CompFlags.pending_dead_code=0;
        GrabLabels();
        skip_to_next_token=0;
        switch (CurToken) {
        case 0x3b: /* if */
            startNewBlock();
            BlockStack->break_label=NextLabel();
            JumpFalse(BracketExpr(),BlockStack->break_label);
            if (CurToken==5 && SrcLineNum==TokenLine) {
                SetErrLoc(ErrFName,SrcLineNum);
                NextToken();
                if (CurToken!=0x44) ChkUseful();
                SetErrLoc(NULL,0);
                break;
            }
            continue;
        case 0x3e: /* while */
            NewLoop();
            BlockStack->break_label=NextLabel();
            if (!JumpFalse(BracketExpr(),BlockStack->break_label)) BlockStack->break_label=0;
            if (CurToken==5 && !CompFlags.useful_side_effect) CWarn1(0x0b,0x6f);
            continue;
        case 0x52: /* do */
            NewLoop();
            if (CurToken==0x0b) { CErr1(0x43b); break; }
            continue;
        case 0x5f: startNewBlock(); ForStmt(); continue;
        case 0x3a: startNewBlock(); SwitchStmt(); DeadCode=1; continue;
        case 0x57: DeadCode=0; CaseStmt(); continue;
        case 0x53: DefaultStmt(); continue;
        case 0x41: BreakStmt(); DeadCode=1; break;
        case 0x5b: ContinueStmt(); DeadCode=1; break;
        case 0x86: LeaveStmt(); DeadCode=1; break;
        case 0x3f: /* return */
            func_result=ReturnStmt(func_result,&return_type);
            if (BlockStack->prev_block==NULL) return_at_outer_level=1;
            MustRecog(5);
            if (SymLevel!=1 || CurToken!=0x0b || BlockStack->block_type!=0x0a) {
                if (!CompFlags.in_pcode_func) {
                    if (end_of_func_label==0) end_of_func_label=NextLabel();
                    Jump(end_of_func_label);
                }
                DeadCode=1;
            }
            break;
        case 0x40: GotoStmt(); break;
        case 5: NextToken(); break;
        case 0x0a: LeftBrace(); continue;
        case 0x0b:
            if (BlockStack->block_type!=0x0a) CErr1(0x3ec);
            if (BlockStack->prev_block==NULL) skip_to_next_token=1;
            else NextToken();
            goto end_statement;
        case 0x61: case 0x5e: case 0x5d: case 0x5c:
        case 0x56: case 0x50: case 0x4b: case 0x43: case 1:
            CErr1(0x435);
            goto end_statement;
        case 0x81: TryStmt(); continue;
        default: StmtExpr(); break;
        }
        if (BlockStack->block_type==0x0a) continue;
end_statement:
        EndOfStmt();
        if (BlockStack==NULL) break;
        if (skip_to_next_token) NextToken();
    }
    if (func_result==0) ChkRetValue();
    else if (!return_at_outer_level && !DeadCode) CWarn(7,0x6b,CurFunc->name);
    if (skip_to_next_token) NextToken();
    if (end_of_func_label!=0) DropLabel(end_of_func_label);
    DeadCode=0;
    if (CompFlags.in_pcode_func) func_result=0;
    GenQuadNoResult(0,0x68,func_result);
    LinkFunctionQuads(QuadIndex-1);
    SrcLineNum=TokenLine;
    SrcFno=TokenFno;
    FreeLabels();
    if (CompFlags.generate_prototypes) {
        if (DefFile==NULL) OpenDefFile();
        if (DefFile!=NULL && CurFunc->stg_class==SC_NULL) DumpFuncDefn();
    }
    if (CompFlags.addr_of_auto_taken) CurFunc->flags&=~0x200;
}

/* The original quad-based build has no tree-list initialization here. */
void StmtInit(void)
{
}

/* The earlier scanner has no saved-location pair; it retains the location
 * at the label-scan entry and recognizes the original single-underscore SEH
 * spellings before ordinary identifier look-ahead. No boolean result is used. */
void GrabLabels(void)
{
    struct label_entry *label;
    label=NULL;
    for (;;) {
        SrcLineNum=TokenLine;
        SrcFno=TokenFno;
        if (CurToken!=0x78) {
            if (CurToken!=0x33) break;
            if (Buffer[0]=='_') {
                if (strcmp(Buffer,"_try")==0) { CurToken=0x81; break; }
                if (strcmp(Buffer,"_leave")==0) { CurToken=0x86; break; }
            }
            LookAhead();
        }
        if (LAToken!=4) break;
        label=LkLabel(SavedId);
        if (label->defined) CErr2p(0x3f9,label->name);
        else {
            DropLabel(label->ref_list);
            label->defined=1;
        }
        CMemFree(SavedId);
        SavedId=NULL;
        NextToken();
    }
    if (label!=NULL && CurToken==0x0b) CErr1(0x43a);
}

void LeftBrace(void)
{
    unsigned first_quad;
    SYM_ENTRY sym;
    QUAD quad;
    startNewBlock();
    ++SymLevel;
    if (SymLevel<=1) DeclList(&CurFunc->u.func.locals);
    else {
        first_quad=QuadIndex;
        GenQuad(0,0x0a,0,0);
        DeclList(&CurFunc->u.func.locals);
        if (BlockStack->sym_head!=0) {
            QuadGet(&quad,first_quad);
            SymGet(&sym,BlockStack->sym_head);
            quad.operand2=MakeSymLeaf(&sym,BlockStack->sym_head);
            QuadReplace(&quad,first_quad);
            LinkFunctionQuads(first_quad);
        } else if (first_quad+1==QuadIndex) --QuadIndex;
    }
}

void startNewBlock(void)
{
    struct block_definition *block;
    block=CMemAlloc(sizeof(struct block_definition));
    block->continue_label=0;
    block->top_label=0;
    block->break_label=0;
    block->block_type=CurToken;
    block->prev_block=BlockStack;
    block->prev_loop=LoopStack;
    BlockStack=block;
    /* In the leaf/quad version this helper consumes the opening keyword. */
    NextToken();
}

unsigned BracketExpr(void)
{
    unsigned leaf;
    MustRecog(6);
    leaf=Expr();
    MustRecog(7);
    return leaf;
}

int JumpFalse(unsigned index,unsigned label)
{
    LEAF leaf;
    int generated;
    LeafGet(&leaf,index);
    generated=0;
    if (leaf.leaf_flags&0x40) {
        switch (leaf.leaf_class) {
        case 0x0a: case 0x0b: case 0x0c: case 0x0d:
            if (leaf.u.constant.value.integer==0) { Jump(label); generated=1; }
            break;
        case 0x0e: case 0x0f:
            /* The old helper tests only the high DWORD of the constant. */
            if (leaf.u.words[2]==0) { Jump(label); generated=1; }
            break;
        }
        FreeLeaf(index);
    } else {
        JumpCond(index,label,0x6b,0x6a);
        generated=1;
    }
    return generated;
}

void JumpCond(unsigned index,unsigned label,int op,int opposite)
{
    QUAD quad;
    index=BoolExpr(index);
    QuadGet(&quad,QuadIndex-1);
    if (quad.quad_class==0x10) {
        quad.quad_class=opposite;
        quad.operand1=quad.operand2;
        quad.operand2=label;
        quad.result=0;
        QuadReplace(&quad,QuadIndex-1);
    } else GenQuadNoResult(index,op,label);
    AddLabelRef(label,QuadIndex-1);
}

unsigned ReturnStmt(unsigned result,char *return_type)
{
    unsigned value;
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    struct block_definition *block;
    char kind;
    NextToken();
    if (CurToken!=5) {
        value=RValue(Expr());
        ChkRetType(value);
        if (CompFlags.in_pcode_func) {
            GenQuad(0,0x3f,value,0);
            result=1;
        } else {
            if (result==0) {
                handle=MakeNewSym(&sym,'R',CurFunc->sym_type->object,SC_AUTO);
                sym.flags|=0x400;
                result=MakeSymLeaf(&sym,handle);
                SymReplace(&sym,handle);
            }
            AsgnOp(result,0x77,value);
        }
        kind=2;
    } else {
        if (CompFlags.in_pcode_func) GenQuad(0,0x3f,0,0);
        kind=1;
    }
    if (*return_type==0) *return_type=kind;
    if (kind!=*return_type) CErr1(0x448);
    block=BlockStack;
    while (block!=NULL && block->block_type!=0x81) block=block->prev_block;
    if (block!=NULL) UnWindTry(-1);
    return result;
}

void Jump(unsigned label)
{
    if (!DeadCode) {
        GenQuadNoResult(0,0x69,label);
        AddLabelRef(label,QuadIndex-1);
        DeadCode=1;
    }
}

void EndOfStmt(void)
{
    do {
        switch (BlockStack->block_type) {
        case 0x0a: EndBlock(); break;
        case 0x3b:
            if (CurToken==0x44) { ElseStmt(); return; }
            /* fall through */
        case 0x44: DropBreakLabel(); break;
        case 0x3e:
            DropContinueLabel();
            Jump(BlockStack->top_label);
            --LoopDepth;
            DropBreakLabel();
            break;
        case 0x5f:
            EndForStmt();
            --LoopDepth;
            DropBreakLabel();
            break;
        case 0x52:
            DropContinueLabel();
            MustRecog(0x3e);
            SrcLineNum=TokenLine;
            SrcFno=TokenFno;
            JumpTrue(BracketExpr(),BlockStack->top_label);
            MustRecog(5);
            SrcLineNum=TokenLine;
            SrcFno=TokenFno;
            --LoopDepth;
            DropBreakLabel();
            break;
        case 0x3a: EndSwitch(); DropBreakLabel(); break;
        case 0x81: if (EndTry()) return; break;
        case 0x82:
            DropBreakLabel();
            TryScope=BlockStack->u.try_scope.parent_index;
            break;
        case 0x83:
            GenQuad(0,0x87,0,0);
            TryScope=BlockStack->u.try_scope.parent_index;
            break;
        }
        PopBlock();
        if (BlockStack==NULL) break;
    } while (BlockStack->block_type!=0x0a);
}

void DropLabel(unsigned label)
{
    DeadCode=0;
    CompFlags.label_dropped=1;
    GenQuadNoResult(label,0x70,0);
    AddLabelRef(label,QuadIndex-1);
    StmtInit();
}

void DropBreakLabel(void)
{
    if (BlockStack->break_label!=0) DropLabel(BlockStack->break_label);
}

void PopBlock(void)
{
    struct block_definition *block;
    SYM_ENTRY sym;
    unsigned leaf;
    if (BlockStack->sym_head!=0) {
        SymGet(&sym,BlockStack->sym_head);
        leaf=MakeSymLeaf(&sym,BlockStack->sym_head);
        GenQuad(0,0x0b,leaf,0);
        LinkFunctionQuads(QuadIndex-1);
    }
    block=BlockStack;
    LoopStack=block->prev_loop;
    BlockStack=block->prev_block;
    CMemFree(block);
}

void FreeLabels(void)
{
    struct label_entry *label;
    while ((label=LabelHead)!=NULL) {
        LabelHead=label->next_label;
        if (!label->defined) CErr2p(0x3fa,label->name);
        else if (!label->referenced) CWarn(0x1a,0x76,label->name);
        CMemFree(label);
    }
}

void LookAhead(void)
{
    SavedId=CStrSave(Buffer);
    SavedHash=HashValue;
    NextToken();
    LAToken=CurToken;
    CurToken=0x78;
}

void StmtExpr(void)
{
    ChkStmtExpr();
    switch (CurToken) {
    case 0x3b: case 0x3e: case 0x52: case 0x5f: case 0x3a:
    case 0x57: case 0x53: case 0x41: case 0x5b: case 0x3f:
    case 0x40: case 0x0a: case 0x0b:
        Expecting(Tokens[5]);
        break;
    default:
        Expecting(Tokens[5]);
        /* fall through */
    case 5:
        NextToken();
        break;
    }
}

/* The quad frontend finalizes only expressions which emitted a quad. */
void ChkStmtExpr(void)
{
    unsigned first_quad;
    first_quad=QuadIndex;
    Expr();
    if (CompFlags.meaningless_stmt) ChkUseful();
    if (first_quad!=QuadIndex) FinishStmtExpr(0);
    if (CompFlags.pending_dead_code) DeadCode=1;
}

void FinishStmtExpr(int useful)
{
    QUAD quad;
    if (SizeOfCount!=0) return;
    QuadGet(&quad,QuadIndex-1);
    if (quad.quad_class==0x67 || quad.quad_class==0x70) return;
    if (useful) {
        quad.quad_flags|=2;
    } else {
        if (quad.quad_class==0x0e) quad.quad_class=0x77;
        if (quad.quad_class==0x77) {
            FreeLeaf(quad.result);
            quad.result=0;
        }
        quad.quad_flags|=1;
    }
    QuadReplace(&quad,QuadIndex-1);
}

/* The older frontend retains quads for loop-induction analysis. */
void ForStmt(void)
{
    QUAD loop_quad;
    unsigned for_quad,condition_start;
    int saved_label_dropped;
    LoopStack=BlockStack;
    MustRecog(6);
    BlockStack->u.for_loop.init_quad=QuadIndex;
    if (CurToken!=5) ChkStmtExpr();
    MustRecog(5);
    StmtInit();
    ++LoopDepth;
    DeadCode=0;
    BlockStack->top_label=NextLabel();
    if (GenSwitches&0x80000000U) {
        LoopStack->continue_label=NextLabel();
    } else {
        for_quad=QuadIndex;
        GenQuad(0,T_FOR,0,0);
        QuadGet(&loop_quad,for_quad);
        BlockStack->u.for_loop.condition_end=QuadIndex;
    }
    loop_quad.operand1=0;
    if (CurToken!=5) {
        loop_quad.operand1=NextLabel();
        DropLabel(loop_quad.operand1);
        condition_start=QuadIndex;
        JumpTrue(Expr(),BlockStack->top_label);
        BlockStack->infinite_loop=DeadCode;
        if (GenSwitches&0x80000000U) {
            JumpBreak(BlockStack);
        } else {
            if (AnalyzeForInit(BlockStack->u.for_loop.init_quad,condition_start))
                loop_quad.operand1=0;
            else BlockStack->u.for_loop.induction_leaf=0;
            StmtInit();
        }
    } else {
        Jump(BlockStack->top_label);
        BlockStack->infinite_loop=1;
    }
    if (CurToken==7) Expecting(Tokens[5]);
    else MustRecog(5);
    BlockStack->u.for_loop.increment_start=QuadIndex;
    if (GenSwitches&0x80000000U) DropLabel(LoopStack->continue_label);
    if (CurToken!=7) {
        DeadCode=0;
        saved_label_dropped=CompFlags.label_dropped;
        CompFlags.label_dropped=1;
        ChkStmtExpr();
        AnalyzeForIncrement();
        CompFlags.label_dropped=saved_label_dropped;
    } else BlockStack->u.for_loop.induction_leaf=0;
    MustRecog(7);
    if (GenSwitches&0x80000000U) {
        if (loop_quad.operand1!=0) Jump(loop_quad.operand1);
        DropLabel(BlockStack->top_label);
    } else {
        loop_quad.operand2=(unsigned short)QuadIndex;
        BlockStack->u.for_loop.last_quad=QuadIndex;
        DropLabel(BlockStack->top_label);
        AddLabelRef(BlockStack->top_label,BlockStack->u.for_loop.increment_start-1);
        QuadReplace(&loop_quad,for_quad);
    }
}

void JumpTrue(unsigned index,unsigned label)
{
    LEAF leaf;
    LeafGet(&leaf,index);
    if (leaf.leaf_flags&0x40) {
        switch (leaf.leaf_class) {
        case 0x0a: case 0x0b: case 0x0c: case 0x0d:
            if (leaf.u.constant.value.integer!=0) Jump(label);
            break;
        case 0x0e: case 0x0f:
            /* As in JumpFalse, only the old high DWORD is tested. */
            if (leaf.u.words[2]!=0) Jump(label);
            break;
        }
        FreeLeaf(index);
    } else JumpCond(index,label,0x6a,0x6b);
}

/* Earlier induction recognizer; the later tree frontend has no quad analogue. */
int AnalyzeForInit(unsigned first,unsigned condition)
{
    QUAD quad;
    LEAF variable,initial,left,right;
    int start,limit;
    QuadGet(&quad,first);
    if (quad.quad_class!=0x77 || condition+2!=QuadIndex) return 0;
    LeafGet(&variable,quad.operand1);
    if (variable.leaf_class!=0 || variable.leaf_type->decl_type!=TYPE_INT) return 0;
    BlockStack->u.for_loop.induction_leaf=quad.operand1;
    LeafGet(&initial,quad.operand2);
    if (initial.leaf_class!=0x0a) return 0;
    QuadGet(&quad,condition);
    switch (quad.quad_class) {
    case 0x0f: case 0x11: case 0x1e: case 0x1f: case 0x1a: break;
    default: return 0;
    }
    LeafGet(&left,quad.operand1);
    LeafGet(&right,quad.operand2);
    if (!SameSymbolLeaf(&left,&variable) || right.leaf_class!=0x0a) return 0;
    start=initial.u.constant.value.integer;
    limit=right.u.constant.value.integer;
    BlockStack->u.for_loop.initial_value=start;
    switch (quad.quad_class) {
    case 0x0f:
        BlockStack->u.for_loop.limit_value=(int)((unsigned)limit+1);
        return start==limit;
    case 0x11:
        BlockStack->u.for_loop.limit_value=limit;
        return start!=limit;
    case 0x1e:
        BlockStack->u.for_loop.limit_value=limit;
        return start<limit;
    case 0x1f:
        BlockStack->u.for_loop.limit_value=(int)((unsigned)limit+1);
        return start<=limit;
    case 0x1a: return start>limit;
    case 0x1b: return start>=limit;
    }
    return 0;
}

void AnalyzeForIncrement(void)
{
    unsigned variable;
    QUAD quad;
    LEAF induction,operand;
    if (BlockStack->u.for_loop.induction_leaf==0) return;
    variable=BlockStack->u.for_loop.induction_leaf;
    BlockStack->u.for_loop.induction_leaf=0;
    if ((GenSwitches&0x80000000U) || (Toggles&4)) return;
    if (BlockStack->u.for_loop.increment_start+1!=QuadIndex) return;
    QuadGet(&quad,BlockStack->u.for_loop.increment_start);
    if (quad.quad_class!=0x23 && quad.quad_class!=0x24) return;
    LeafGet(&operand,quad.operand1);
    LeafGet(&induction,variable);
    if (!SameSymbolLeaf(&induction,&operand)) return;
    LeafGet(&operand,quad.operand2);
    if (operand.leaf_class==0x0a && operand.u.constant.value.integer==1)
        BlockStack->u.for_loop.induction_leaf=variable;
}

void NewLoop(void)
{
    startNewBlock();
    LoopStack=BlockStack;
    LoopStack->top_label=NextLabel();
    DropLabel(LoopStack->top_label);
    ++LoopDepth;
}

void DropContinueLabel(void)
{
    if (BlockStack->continue_label!=0) DropLabel(BlockStack->continue_label);
}

/* The old frontend recognizes small induction loops which fill a memory
 * range, replacing their quads with a real internal STOS call. */
void EndForStmt(void)
{
    unsigned index,count,stride,pattern,twice_count,callee,call_type;
    unsigned short induction_index;
    SYM_HANDLE fill_function;
    QUAD address_quad,quad;
    LEAF induction,operand;
    TYPEPTR typ;
    SYM_ENTRY sym;
    int optimized;
    union { float value; unsigned bits; } single;
    optimized=0;
    if (BlockStack->u.for_loop.induction_leaf==0) goto ordinary_loop;
    index=BlockStack->u.for_loop.last_quad+1;
    if (BlockStack->u.for_loop.last_quad+5<QuadIndex || index+3>QuadIndex ||
        BlockStack->u.for_loop.initial_value!=0) goto ordinary_loop;
    count=(unsigned)BlockStack->u.for_loop.limit_value-(unsigned)BlockStack->u.for_loop.initial_value;
    QuadGet(&address_quad,index);
    if (address_quad.quad_class!=0x71) goto ordinary_loop;
    LeafGet(&operand,address_quad.result);
    fill_function=(operand.leaf_flags&0x18) ? SymSTOD:SymSTOSB;
    LeafGet(&induction,BlockStack->u.for_loop.induction_leaf);
    QuadGet(&quad,++index);
    if (quad.quad_class==0x28) {
        if (quad.operand1==0) goto ordinary_loop;
        LeafGet(&operand,quad.operand1);
        if (!SameSymbolLeaf(&induction,&operand)) goto ordinary_loop;
        LeafGet(&operand,quad.operand2);
        if (operand.leaf_class!=0x0a && operand.leaf_class!=0x0b) goto ordinary_loop;
        stride=operand.u.constant.value.integer;
        induction_index=quad.operand1;
        QuadGet(&quad,++index);
        if (quad.quad_class!=0x22) goto ordinary_loop;
        QuadGet(&quad,++index);
        if (quad.quad_class!=0x77) goto ordinary_loop;
        LeafGet(&operand,quad.operand1);
        typ=operand.leaf_type;
        if (TypeSize(typ)!=stride) goto ordinary_loop;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type==TYPE_FIELD || typ->decl_type==TYPE_UFIELD) goto ordinary_loop;
        LeafGet(&operand,quad.operand2);
        optimized=1;
        twice_count=count+count;
        switch (operand.leaf_class) {
        case 0x0a: case 0x0b: case 0x0c: case 0x0d:
            pattern=operand.u.constant.value.integer;
            if (stride!=4) {
                pattern=(pattern&0xffff)|(pattern<<16);
                if (fill_function==SymSTOSB) count=twice_count;
                else fill_function=SymSTOW;
                if (stride!=2) optimized=0;
            }
            break;
        case 0x0e: case 0x0f:
            single.value=(float)operand.u.constant.value.floating;
            pattern=single.bits;
            if (stride==8) {
                if (operand.u.words[2]!=0) optimized=0;
                else count=twice_count;
            }
            break;
        default:
            optimized=0;
            break;
        }
        if ((stride==4 || operand.leaf_class==0x0e || operand.leaf_class==0x0f) &&
            fill_function==SymSTOSB) fill_function=SymSTOSD;
        if (!optimized) goto ordinary_loop;
        optimized=0;
    } else {
        if (quad.quad_class!=0x22) goto ordinary_loop;
        LeafGet(&operand,quad.operand2);
        if (!SameSymbolLeaf(&induction,&operand)) goto ordinary_loop;
        induction_index=quad.operand2;
        QuadGet(&quad,++index);
        if (quad.quad_class!=0x77) goto ordinary_loop;
        LeafGet(&operand,quad.operand2);
        if (operand.leaf_class!=0x0a && operand.leaf_class!=0x0b) goto ordinary_loop;
        pattern=operand.u.constant.value.integer&0xff;
        pattern+=pattern<<8;
        pattern|=pattern<<16;
        if (fill_function!=SymSTOSB) {
            if (!(count&3)) {
                count>>=2;
                fill_function=SymSTOD;
            } else {
                fill_function=(count&1) ? SymSTOWB:SymSTOW;
                count>>=1;
            }
        }
    }
    if (++index!=QuadIndex) goto ordinary_loop;
    QuadIndex=BlockStack->u.for_loop.init_quad;
    QuadGet(&quad,QuadIndex);
    address_quad.src_line=quad.src_line;
    QuadReplace(&address_quad,QuadIndex);
    ++QuadIndex;
    LeafGet(&operand,address_quad.result);
    SymGet(&sym,fill_function);
    callee=MakeSymLeaf(&sym,fill_function);
    call_type=CallResultLeaf(sym.sym_type);
    quad.quad_class=0x6c;
    quad.quad_flags=0;
    quad.operand1=callee;
    quad.operand2=call_type;
    quad.result=IntLeaf(count);
    QuadReplace(&quad,QuadIndex);
    ++QuadIndex;
    quad.quad_class=0x6d;
    quad.operand1=quad.operand2=0;
    quad.result=IntLeaf(pattern);
    QuadReplace(&quad,QuadIndex);
    ++QuadIndex;
    quad.result=address_quad.result;
    QuadReplace(&quad,QuadIndex);
    ++QuadIndex;
    quad.quad_class=T_CALL;
    quad.quad_flags=1;
    quad.operand1=0;
    quad.operand2=callee;
    quad.result=call_type;
    QuadReplace(&quad,QuadIndex);
    ++QuadIndex;
    quad.quad_class=0x77;
    quad.operand1=induction_index;
    quad.operand2=IntLeaf(BlockStack->u.for_loop.limit_value);
    quad.result=TypeLeaf(TypeDefault());
    QuadReplace(&quad,QuadIndex);
    ++QuadIndex;
    optimized=1;
ordinary_loop:
    if (!optimized) {
        if (GenSwitches&0x80000000U) Jump(BlockStack->continue_label);
        else {
            DropContinueLabel();
            DeadCode=0;
            GenQuad(BlockStack->u.for_loop.condition_end,T_END_FOR,
                    BlockStack->u.for_loop.increment_start,BlockStack->u.for_loop.last_quad);
        }
        DeadCode=BlockStack->infinite_loop;
    }
}

void SwitchStmt(void)
{
    struct switch_definition *sw;
    TYPEPTR typ;
    DATA_TYPE kind;
    unsigned index;
    sw=CMemAlloc(sizeof(*sw));
    sw->low_value=0xffffffffU;
    sw->high_value=0;
    sw->case_count=0;
    sw->case_format="%ld";
    sw->prev_switch=SwitchStack;
    SwitchStack=sw;
    kind=TYPE_INT;
    index=BracketExpr();
    typ=TypeOf(index);
    if (typ->decl_type==TYPE_ENUM) typ=typ->object;
    if (typ->decl_type==TYPE_UFIELD && typ->u.f.field_width==16) {
        kind=TYPE_UINT;
        sw->case_format="%lu";
    }
    switch (typ->decl_type) {
    case TYPE_USHORT: case TYPE_UINT:
        kind=TYPE_UINT;
        sw->case_format="%lu";
        break;
    case TYPE_ULONG:
        kind=TYPE_ULONG;
        sw->case_format="%lu";
        break;
    case TYPE_LONG:
        kind=TYPE_LONG;
        break;
    case TYPE_CHAR: case TYPE_UCHAR: case TYPE_SHORT: case TYPE_INT:
    case TYPE_FIELD: case TYPE_UFIELD:
        break;
    default:
        CErr1(0x436);
        break;
    }
    sw->quad_index=(unsigned short)QuadIndex;
    sw->case_type=GetType(kind);
    GenQuad(0,T_SWITCH,index,0);
}

void CaseStmt(void)
{
    NextToken();
    if (SwitchStack!=NULL) {
        AddCaseLabel(BoolConstExpr());
        MustRecog(4);
        if (CurToken==0x0b) CErr1(0x43c);
        StmtInit();
    } else {
        CErr1(0x3e9);
        BoolConstExpr();
        MustRecog(4);
    }
}

void AddCaseLabel(unsigned value)
{
    LEAF leaf;
    struct case_entry *entry,*previous,*new_entry;
    unsigned converted,old_value,quad;
    char buffer[12];
    leaf.u.constant.value.integer=value;
    leaf.leaf_type=SwitchStack->case_type;
    NormalizeConstant(&leaf,1);
    converted=leaf.u.constant.value.integer;
    previous=NULL;
    old_value=converted+1;
    for (entry=SwitchStack->case_list;entry!=NULL;entry=entry->next_case) {
        old_value=entry->value;
        if (old_value>=converted) break;
        previous=entry;
    }
    if (converted==old_value) {
        sprintf(buffer,SwitchStack->case_format,value);
        CErr2p(0x40f,buffer);
        return;
    }
    new_entry=CMemAlloc(sizeof(*new_entry));
    new_entry->value=converted;
    new_entry->label=NextCaseLabel(converted);
    if (previous==NULL) {
        new_entry->next_case=SwitchStack->case_list;
        SwitchStack->case_list=new_entry;
    } else {
        previous->next_case=new_entry;
        new_entry->next_case=entry;
    }
    ++SwitchStack->number_of_cases;
    if (converted<SwitchStack->low_value) SwitchStack->low_value=converted;
    if (converted>SwitchStack->high_value) SwitchStack->high_value=converted;
    quad=GenQuadNoResult(new_entry->label,T_CASE,SwitchStack->case_count);
    AddLabelRef(new_entry->label,quad);
    SwitchStack->case_count=(unsigned short)(QuadIndex-1);
}

void BreakStmt(void)
{
    struct block_definition *block;
    int try_scope;
    NextToken();
    try_scope=-2;
    block=BlockStack;
    if (block!=NULL) {
        while (block!=LoopStack) {
            if (block->block_type==T_SWITCH) break;
            if (block->block_type==T_TRY) try_scope=block->u.try_scope.parent_index;
            block=block->prev_block;
        }
    }
    if (block!=NULL) {
        if (try_scope!=-2) UnWindTry(try_scope);
        JumpBreak(block);
    } else CErr1(0x3e8);
    MustRecog(5);
}

void JumpBreak(struct block_definition *block)
{
    if (!DeadCode) {
        if (block->break_label==0) block->break_label=NextLabel();
        Jump(block->break_label);
    }
}

void DefaultStmt(void)
{
    NextToken();
    MustRecog(4);
    if (SwitchStack!=NULL) {
        if (SwitchStack->default_label==0) {
            SwitchStack->default_label=NextLabel();
            DropLabel(SwitchStack->default_label);
        } else CErr1(0x3f0);
        if (CurToken==0x0b) CErr1(0x43d);
    } else CErr1(0x3eb);
}

void EndSwitch(void)
{
    struct switch_definition *sw;
    struct case_entry *entry;
    QUAD quad;
    sw=SwitchStack;
    SwitchStack=sw->prev_switch;
    if (sw->default_label==0) {
        if (BlockStack->break_label==0) {
            sw->default_label=NextLabel();
            DropLabel(sw->default_label);
        } else sw->default_label=BlockStack->break_label;
    }
    QuadGet(&quad,sw->quad_index);
    quad.operand1=sw->case_count;
    quad.result=(unsigned short)sw->default_label;
    QuadReplace(&quad,sw->quad_index);
    while (sw->case_list!=NULL) {
        entry=sw->case_list;
        sw->case_list=entry->next_case;
        CMemFree(entry);
    }
    CMemFree(sw);
    StmtInit();
}

void ContinueStmt(void)
{
    struct block_definition *block;
    int try_scope;
    NextToken();
    if (LoopStack!=NULL) {
        if (!DeadCode) {
            try_scope=-2;
            block=BlockStack;
            while (block!=LoopStack) {
                if (block->block_type==T_TRY) try_scope=block->u.try_scope.parent_index;
                block=block->prev_block;
            }
            if (try_scope!=-2) UnWindTry(try_scope);
            if (LoopStack->continue_label==0) LoopStack->continue_label=NextLabel();
            Jump(LoopStack->continue_label);
        }
    } else CErr1(0x3ea);
    MustRecog(5);
}

void GotoStmt(void)
{
    struct label_entry *label;
    NextToken();
    if (CurToken!=T_ID) CErr1(0x40e);
    else {
        label=LkLabel(Buffer);
        label->referenced=1;
        Jump(label->ref_list);
    }
    NextToken();
    MustRecog(5);
}

struct label_entry *LkLabel(char *name)
{
    struct label_entry *label;
    for (label=LabelHead;label!=NULL;label=label->next_label)
        if (strcmp(name,label->name)==0) return label;
    label=CMemAlloc(strlen(name)+15);
    if (label!=NULL) {
        label->next_label=LabelHead;
        LabelHead=label;
        strcpy(label->name,name);
        label->defined=label->referenced=0;
        label->ref_list=NextLabel();
    }
    return label;
}

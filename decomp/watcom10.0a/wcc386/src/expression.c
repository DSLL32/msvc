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


#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "expression.h"
#include "options.h"
#include "quads.h"
#include "scanner.h"
#include "macros.h"
#include "preproc.h"
#include "stubs.h"
#include "declarations.h"
#include "statement.h"
#include "cstring.h"
#include "memory.h"
#include "pragma.h"
#include <recon.h>

int Level,DeadCode;
unsigned char Class[256],Token[256];
unsigned ValueStack[256];
struct nested_parm_lists *NestedParms;
void ExprInit(void)
{
    Level=0;
    NestedParms=NULL;
}

unsigned UMinus(unsigned tree)
{
    LEAF leaf;
    DATA_TYPE kind;
    unsigned type_leaf;
    /* Encountered diagonal of the older arithmetic result table. */
    static const unsigned char UnaryResult[10]={
        TYPE_INT,TYPE_INT,TYPE_INT,TYPE_INT,TYPE_INT,TYPE_UINT,
        TYPE_LONG,TYPE_ULONG,TYPE_FLOAT,TYPE_DOUBLE
    };

    tree=RValue(tree);
    LeafGet(&leaf,tree);
    switch (leaf.leaf_class) {
    case 0x0a: case 0x0b: case 0x0c: case 0x0d:
        leaf.u.constant.value.integer=(int)(0U-(unsigned)leaf.u.constant.value.integer);
        LeafReplace(&leaf,tree);
        break;
    case 0x0e: case 0x0f:
        leaf.u.constant.value.floating=-leaf.u.constant.value.floating;
        LeafReplace(&leaf,tree);
        break;
    case 0x13:
        break;
    default:
        kind=DataTypeOf((DATA_TYPE)TypeOf(tree)->decl_type);
        if (kind==TYPE_VOID) break;
        if (kind>=TYPE_POINTER) {
            CErr1(0x438);
            tree=DefaultLeaf;
        } else {
            type_leaf=TypeLeaf(GetType((DATA_TYPE)UnaryResult[kind]));
            tree=GenQuad(0,0x25,tree,type_leaf);
        }
    }
    return tree;
}

unsigned EnumLeaf(struct enum_info *info)
{
    unsigned char kind;
    kind=info->parent->sym_type->object->decl_type;
    if (kind==TYPE_USHORT || kind==TYPE_UINT) return UIntLeaf(info->value);
    return IntLeaf(info->value);
}

unsigned SymLeaf(void)
{
    SYM_HANDLE sym_handle,sym0_handle;
    SYM_ENTRY sym;
    struct enum_info ei;
    int hash,saved;
    char *name;
    unsigned leaf;

    saved=CurToken==0x78;
    if (saved) {
        CurToken=LAToken;
        hash=SavedHash;
        name=SavedId;
    } else {
        hash=HashValue;
        name=Buffer;
    }
    EnumLookup(hash,name,&ei);
    sym_handle=SymLook(hash,name);
    if (sym_handle==0) {
        if (ei.level>=0) {
            if (!saved) NextToken();
            return EnumLeaf(&ei);
        }
        SymCreate(&sym,name);
    } else {
        SymGet(&sym,sym_handle);
        if (ei.level>(int)sym.level) {
            if (!saved) NextToken();
            return EnumLeaf(&ei);
        }
        if (sym.stg_class==0x61 && sym.level>0) {
            sym0_handle=Sym0Look(hash,name);
            if (sym0_handle!=0) {
                sym_handle=sym0_handle;
                SymGet(&sym,sym_handle);
            }
        }
        if (sym.stg_class==0x58) {
            CErr2p(0x422,name);
            return IntLeaf(0);
        }
    }
    if (!saved) NextToken();
    sym.flags|=2;
    if (sym_handle==0) {
        if (CurToken==6) {
            sym.stg_class=0x73;
            sym_handle=SymAddL0(hash,&sym);
            sym.flags|=1;
            sym.sym_type=FuncNode(GetType(TYPE_INT),0,NULL);
        } else {
            sym.stg_class=0x61;
            CErr2p(0x3f3,sym.name);
            sym_handle=SymAdd(hash,&sym);
            sym.sym_type=GetType(TYPE_INT);
        }
    }
    IncSymWeight(&sym);
    leaf=MakeSymLeaf(&sym,sym_handle);
    if (sym.sym_leaf==0) sym.sym_leaf=leaf;
    SymReplace(&sym,sym_handle);
    return leaf;
}

unsigned ExprId(void)
{
    int value,count;
    if (CompFlags.pre_processing) {
        if (strcmp(Buffer,"defined")==0) {
            CompFlags.pre_processing=2;
            NextToken();
            if (CurToken==6) {
                NextToken();
                value=IsMacroDefined();
                MustRecog(7);
            } else value=IsMacroDefined();
        } else {
            if (SizeOfCount==0) CWarn(0x13,0xcb,Buffer);
            NextToken();
            if (CurToken==6) {
                count=0;
                for (;;) {
                    NextToken();
                    if (CurToken==0) break;
                    if (CurToken==6) ++count;
                    if (CurToken==7) {
                        if (count==0) {
                            NextToken();
                            break;
                        }
                        --count;
                    }
                }
            }
            value=0;
        }
        return IntLeaf(value);
    }
    return SymLeaf();
}

int BoolConstExpr(void)
{
    unsigned index;
    LEAF leaf;
    ++Level;
    index=SingleExpr();
    --Level;
    LeafGet(&leaf,index);
    FreeLeaf(index);
    if (leaf.leaf_flags&0x40) {
        if (leaf.leaf_class>=0x0e && leaf.leaf_class<=0x0f) {
            CErr1(0x437);
            return (int)leaf.u.constant.value.floating;
        }
        return leaf.u.constant.value.integer;
    }
    CErr1(0x41e);
    return 0;
}

unsigned SingleExpr(void)
{
    Class[Level]=TC_START2; /* no assignment or comma */
    return GetExpr();
}

/* Earlier leaf/quad form of cexpr2.c's shift/reduce GetExpr. */
unsigned GetExpr(void)
{
    unsigned tree,op1,type_leaf,first_quad;
    unsigned char curclass;
    TYPEPTR typ,*plist;
    CompFlags.useful_side_effect=0;
    first_quad=QuadIndex;
    tree=0;
    plist=NULL;
    for (;;) {
        if (tree==0) tree=ExprOpnd();
        if ((unsigned)CurToken>=101) STUB_TRAP();
        curclass=TokenClass[CurToken]&0x7f;
        while (curclass<=Class[Level]) {
            op1=ValueStack[Level];
            switch (Class[Level]) {
            case TC_START:
            case TC_START1:
            case TC_START2:
            case TC_START_UNARY:
                return tree;
            case TC_LEFT_PAREN:
                MustRecog(7);
                if ((unsigned)CurToken>=101) STUB_TRAP();
                curclass=TokenClass[CurToken]&0x7f;
                break;
            case TC_COMMA:
                op1=VoidRValue(op1);
                tree=VoidRValue(tree);
                typ=TypeOf(tree);
                type_leaf=TypeLeaf(typ);
                tree=GenQuad(op1,2,tree,type_leaf);
                break;
            case TC_ASSIGNMENT:
                ChkConst(op1);
                tree=AsgnOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=0;
                CompFlags.useful_side_effect=1;
                break;
            case TC_TERNARY:
                MustRecog(4);
                break;
            case TC_TERNARY_DONE:
                CompFlags.meaningless_stmt&=Token[Level];
                --Level;
                tree=TernOp(ValueStack[Level],op1,tree);
                CompFlags.pending_dead_code=0;
                break;
            case TC_OR_OR:
                if (Token[Level]==0) {
                    BoolExpr(tree);
                    --SizeOfCount;
                    tree=op1;
                } else {
                    tree=BoolExpr(tree);
                    tree=FlowOp(op1,0x14,tree);
                }
                CompFlags.meaningless_stmt=1;
                CompFlags.pending_dead_code=0;
                break;
            case TC_AND_AND:
                if (Token[Level]==0) {
                    BoolExpr(tree);
                    --SizeOfCount;
                    tree=op1;
                } else {
                    tree=BoolExpr(tree);
                    tree=FlowOp(op1,0x19,tree);
                }
                CompFlags.meaningless_stmt=1;
                CompFlags.pending_dead_code=0;
                break;
            case TC_OR:
            case TC_XOR:
            case TC_AND:
                tree=IntOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_EQ_NE:
            case TC_REL_OP:
                tree=RelOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_SHIFT_OP:
                tree=ShiftOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_ADD_OP:
                tree=AddOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_MUL_OP:
                tree=BinOp(op1,Token[Level],tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_PREINC:
                ChkConst(tree);
                op1=IntLeaf(1);
                tree=AddOp(tree,Token[Level],op1);
                CompFlags.meaningless_stmt=0;
                CompFlags.useful_side_effect=1;
                break;
            case TC_ADDR:
                tree=AddrOp(tree,1);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_EXCLAMATION:
                tree=RValue(tree);
                tree=NotOp(tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_PLUS:
                tree=UnaryPlus(tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_MINUS:
                tree=UMinus(tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_TILDE:
                tree=UComplement(tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_SIZEOF:
                typ=TypeOf(tree);
                tree=SizeofOp(typ);
                CompFlags.meaningless_stmt=1;
                --SizeOfCount;
                break;
            case TC_INDIRECTION:
                tree=RValue(tree);
                tree=PtrOp(tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_CAST:
                typ=TypeOf(op1);
                tree=CnvOp(tree,typ,1);
                FreeLeaf(op1);
                break;
            case TC_INDEX:
                tree=IndexOp(op1,tree);
                if ((unsigned)CurToken>=101) STUB_TRAP();
                curclass=TokenClass[CurToken]&0x7f;
                CompFlags.meaningless_stmt=1;
                break;
            case TC_SEG_OP:
                tree=SegOp(op1,tree);
                CompFlags.meaningless_stmt=1;
                break;
            case TC_PARM_LIST:
                tree=GenNextParm(tree,&plist);
                tree=GenFuncCall(tree);
                PopNestedParms(&plist);
                if ((unsigned)CurToken>=101) STUB_TRAP();
                curclass=TokenClass[CurToken]&0x7f;
                CompFlags.meaningless_stmt=0;
                CompFlags.useful_side_effect=1;
                break;
            }
            --Level;
        }
        switch (curclass) {
        case TC_RIGHT_PAREN:
            CErr1(0x48d);
            NextToken();
            continue;
        case TC_QUESTION:
            tree=StartTernary(tree);
            curclass=TC_TERNARY;
            break;
        case TC_COLON:
            tree=ColonOp(tree);
            CurToken=CompFlags.meaningless_stmt;
            curclass=TC_TERNARY_DONE;
            break;
        case TC_OR_OR:
            tree=BoolExpr(tree);
            tree=OrOr(tree);
            break;
        case TC_AND_AND:
            tree=BoolExpr(tree);
            tree=AndAnd(tree);
            break;
        case TC_ASSIGN_OP:
            curclass=TC_ASSIGNMENT;
            break;
        case TC_LEFT_BRACKET:
            curclass=TC_INDEX;
            break;
        case TC_COMMA:
            if (Class[Level]!=TC_PARM_LIST) {
                if (CompFlags.meaningless_stmt) {
                    if (CompFlags.useful_side_effect) CWarn1(0x16,0x12e);
                    else CWarn1(0x0b,0x6f);
                }
            } else {
                tree=GenNextParm(tree,&plist);
                curclass=TC_PARM_LIST;
            }
            break;
        case TC_FUNC_CALL:
            tree=StartFunc(tree,&plist,first_quad);
            continue;
        case TC_DOT:
            NextToken();
            tree=DotOp(tree);
            NextToken();
            CompFlags.meaningless_stmt=1;
            continue;
        case TC_ARROW:
            NextToken();
            tree=RValue(tree);
            tree=ArrowOp(tree);
            NextToken();
            CompFlags.meaningless_stmt=1;
            continue;
        case TC_POSTINC:
            ChkConst(tree);
            op1=IntLeaf(1);
            tree=AddOp(tree,CurToken,op1);
            NextToken();
            CompFlags.meaningless_stmt=0;
            CompFlags.useful_side_effect=1;
            continue;
        }
        if (Level>=256) CErr1(0x43e);
        if (tree!=0) {
            ++Level;
            ValueStack[Level]=tree;
            Class[Level]=curclass;
            Token[Level]=CurToken;
            tree=0;
        }
        NextToken();
    }
}

unsigned ExprOpnd(void)
{
    unsigned tree;
    TYPEPTR typ;
    CompFlags.meaningless_stmt=1;
    for (;;) {
        ++Level;
        switch (CurToken) {
        case 0x24: /* T_PLUS_PLUS */
        case 0x27: /* T_MINUS_MINUS */
            Token[Level]=CurToken-1;
            Class[Level]=TC_PREINC;
            NextToken();
            continue;
        case 0x17:
            Class[Level]=TC_ADDR;
            NextToken();
            continue;
        case 0x10:
            Class[Level]=TC_EXCLAMATION;
            NextToken();
            continue;
        case 0x22:
            Class[Level]=TC_PLUS;
            NextToken();
            continue;
        case 0x25:
            Class[Level]=TC_MINUS;
            NextToken();
            continue;
        case 0x0d:
            Class[Level]=TC_TILDE;
            NextToken();
            continue;
        case 0x59: /* T_SIZEOF */
            if (CompFlags.pre_processing) CErr1(0x451);
            Class[Level]=TC_SIZEOF;
            NextToken();
            Token[Level]=CurToken;
            if (CurToken==6) {
                NextToken();
                typ=TypeName();
                if (typ!=NULL) {
                    tree=SizeofOp(typ);
                    MustRecog(7);
                    break;
                }
                ++Level;
                Class[Level]=TC_LEFT_PAREN;
            }
            ++SizeOfCount;
            continue;
        case 0x28:
            Class[Level]=TC_INDIRECTION;
            NextToken();
            continue;
        case 6:
            NextToken();
            typ=TypeName();
            if (typ!=NULL) {
                if (CompFlags.pre_processing) CErr1(0x450);
                MustRecog(7);
                Class[Level]=TC_CAST;
                ValueStack[Level]=TypeLeaf(typ);
            } else Class[Level]=TC_LEFT_PAREN;
            continue;
        case T_ID:
            tree=ExprId();
            CompFlags.meaningless_stmt=0;
            break;
        case 0x78:
            tree=SymLeaf();
            CMemFree(SavedId);
            SavedId=NULL;
            CurToken=LAToken;
            CompFlags.meaningless_stmt=0;
            break;
        case T_CONSTANT:
            tree=ConstLeaf();
            NextToken();
            break;
        case T_STRING:
            tree=StringLeaf(CompFlags.strings_in_code_segment?1:0);
            break;
        case 0x7c:
            CErr1(BadTokenInfo);
            NextToken();
            tree=DefaultLeaf;
            break;
        default:
            if (CompFlags.pre_processing && CurToken>=54 && CurToken<0x65)
                tree=ExprId();
            else {
                CErr1(0x427);
                tree=DefaultLeaf;
            }
        }
        break;
    }
    --Level;
    return tree;
}

unsigned ConstLeaf(void)
{
    LEAF leaf;
    InitLeaf(&leaf);
    switch (ConstType) {
    case (DATA_TYPE)0x17: /* old TYPE_WCHAR scan mode */
        ConstType=TYPE_USHORT;
        leaf.leaf_class=0x0a;
        leaf.u.constant.value.integer=(int)Constant;
        break;
    case TYPE_CHAR:
        ConstType=TYPE_INT;
        /* fall through */
    case TYPE_INT:
        leaf.leaf_class=0x0a;
        leaf.u.constant.value.integer=(int)Constant;
        break;
    case TYPE_UINT:
        leaf.leaf_class=0x0b;
        leaf.u.constant.value.integer=(int)Constant;
        break;
    case TYPE_LONG:
        leaf.leaf_class=0x0c;
        leaf.u.constant.value.integer=(int)Constant;
        break;
    case TYPE_ULONG:
        leaf.leaf_class=0x0d;
        leaf.u.constant.value.integer=(int)Constant;
        break;
    case TYPE_FLOAT:
        leaf.leaf_class=0x0e;
        leaf.u.constant.value.floating=strtod(Buffer,NULL);
        break;
    case TYPE_DOUBLE:
        leaf.leaf_class=0x0f;
        leaf.u.constant.value.floating=strtod(Buffer,NULL);
        break;
    }
    leaf.leaf_flags=0x40;
    leaf.leaf_type=GetType(ConstType);
    return GenLeaf(&leaf);
}

unsigned BoolExpr(unsigned tree)
{
    QUAD quad;
    LEAF leaf;
    unsigned zero;
    if (QuadIndex==0) quad.quad_class=0;
    else QuadGet(&quad,QuadIndex-1);
    switch (quad.quad_class) {
    case 0x10:
        if (Toggles&4) quad.result=0;
        /* fall through */
    case 0x0f:
    case 0x11:
    case 0x14:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1e:
    case 0x1f:
        if (tree==quad.result) return tree;
    }
    if (quad.quad_class==0x0e) {
        LeafGet(&leaf,tree);
        if (leaf.leaf_class==4) {
            LeafGet(&leaf,quad.operand2);
            if (leaf.leaf_flags&0x40) CWarn1(5,0x69);
        }
    }
    tree=RValue(tree);
    if (DataTypeOf((DATA_TYPE)TypeOf(tree)->decl_type)>TYPE_POINTER) {
        CErr1(0x439);
    } else {
        zero=IntLeaf(0);
        tree=RelOp(tree,0x11,zero);
    }
    return tree;
}

unsigned RValue(unsigned leaf)
{
    return TakeRValue(leaf,0);
}

unsigned TakeRValue(unsigned tree,int void_ok)
{
    LEAF leaf;
    TYPEPTR typ;
    SYM_ENTRY sym;
    unsigned char sym_flags;
    LeafGet(&leaf,tree);
    if (leaf.leaf_class==0x13) return tree;
    if (leaf.leaf_class==0x12) {
        if (CompFlags.initializing_data) return tree;
        return StringAddress(&leaf,tree);
    }
    if (CompFlags.pre_processing &&
        (leaf.leaf_class==0x0e || leaf.leaf_class==0x0f)) CErr1(0x437);
    typ=leaf.leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_VOID || (leaf.leaf_flags&0x80)) {
        if (!void_ok) CErr1(0x41c);
    } else if (typ->decl_type==TYPE_ARRAY || typ->decl_type==TYPE_FUNCTION) {
        tree=AddrOp(tree,0);
        LeafGet(&leaf,tree);
        if (!CompFlags.initializing_data && leaf.leaf_class==0x12)
            return StringAddress(&leaf,tree);
    } else if (TypeSize(typ)==0) {
        CErr1(0x459);
    } else {
        if (SizeOfCount==0 && ((leaf.leaf_flags&2) || (typ->decl_flags&2)))
            CompFlags.useful_side_effect=1;
        if (leaf.leaf_class==0 || leaf.leaf_class==1) {
            SymGet(&sym,leaf.u.symbol.handle);
            sym_flags=(unsigned char)sym.flags;
            sym.flags|=SYM_REFERENCED;
            if (!CompFlags.label_dropped && SizeOfCount==0 &&
                sym.level!=0 && sym.stg_class!=SC_STATIC && sym.stg_class!=SC_EXTERN &&
                !(sym.flags&SYM_ASSIGNED)) {
                sym.flags|=SYM_ASSIGNED;
                CWarn(0x10,0xc8,SymName(&sym,leaf.u.symbol.handle));
            }
            if (sym_flags!=sym.flags) SymReplace(&sym,leaf.u.symbol.handle);
        }
    }
    return tree;
}

TYPEPTR TypeOf(unsigned index)
{
    LEAF leaf;
    LeafGet(&leaf,index);
    return LeafType(&leaf);
}

TYPEPTR LeafType(LEAFPTR leaf)
{
    TYPEPTR typ;
    unsigned flags;
    if (leaf->leaf_flags&0x80) return GetType(TYPE_VOID);
    typ=leaf->leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type!=TYPE_FUNCTION) {
        flags=leaf->leaf_flags&3;
        if (flags!=0) typ=DupType(typ,typ->decl_flags|flags,0);
    }
    return typ;
}

DATA_TYPE DataTypeOf(DATA_TYPE type)
{
    if (type==TYPE_ENUM || type==TYPE_FIELD) return TYPE_INT;
    if (type==TYPE_UFIELD) return TYPE_UINT;
    return type;
}

unsigned IntLeaf(int value)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=0x0a;
    leaf.leaf_flags=0x40;
    leaf.u.constant.value.integer=value;
    leaf.leaf_type=GetType(TYPE_INT);
    return GenLeaf(&leaf);
}

unsigned RelOp(unsigned op1,unsigned char opr,unsigned op2)
{
    LEAF leaf;
    TYPEPTR typ1,typ2;
    DATA_TYPE op1_type,op2_type;
    unsigned type_leaf;
    LeafGet(&leaf,op2);
    if (leaf.leaf_class==0x13) return op2;
    op1=RValue(op1);
    op2=RValue(op2);
    typ1=TypeOf(op1);
    typ2=TypeOf(op2);
    op1_type=DataTypeOf((DATA_TYPE)typ1->decl_type);
    op2_type=DataTypeOf((DATA_TYPE)typ2->decl_type);
    if (leaf.leaf_class==0x0a && leaf.u.constant.value.integer==0 &&
        (opr==0x1b || opr==0x1e)) {
        switch (op1_type) {
        case TYPE_UCHAR: case TYPE_USHORT: case TYPE_UINT:
        case TYPE_ULONG: case TYPE_POINTER: case TYPE_UFIELD:
            CWarn1(0x0b,0x6f);
        }
    }
    if (op1_type==TYPE_VOID || op2_type==TYPE_VOID) {
        /* The rvalue conversion has already diagnosed the void type. */
    } else if (op1_type==TYPE_POINTER && op2_type==TYPE_POINTER) {
        if (!CompatiblePointers(typ1,typ2)) {
            if (opr==0x0f || opr==0x11) {
                if (!IsVoidType(typ1) && !IsVoidType(typ2)) PointerTypeMismatch();
            } else PointerTypeMismatch();
        }
    } else if ((op1_type==TYPE_POINTER && (op2_type==TYPE_INT || op2_type==TYPE_LONG)) ||
               (op2_type==TYPE_POINTER && (op1_type==TYPE_INT || op1_type==TYPE_LONG))) {
        if (typ1->decl_type==TYPE_ENUM || typ2->decl_type==TYPE_ENUM) CWarn1(0x0d,0x71);
    } else if (op1_type==TYPE_STRUCT || op1_type==TYPE_UNION ||
               op2_type==TYPE_STRUCT || op2_type==TYPE_UNION) {
        CErr1(0x452);
    } else if (leaf.leaf_class!=0x0a &&
               (op1_type>TYPE_DOUBLE || op2_type>TYPE_DOUBLE)) {
        /* Every arithmetic pair in the old13x13 BinResult is valid;
         * its pointer/aggregate rows and columns are the ERR entries. */
        CErr1(0x3f2);
    }
    type_leaf=TypeLeaf(GetType(TYPE_INT));
    return GenQuad(op1,opr,op2,type_leaf);
}

unsigned TypeLeaf(TYPEPTR typ)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=4;
    leaf.leaf_flags=typ->decl_flags&0xff1f;
    leaf.leaf_type=typ;
    return GenLeaf(&leaf);
}

/* The 1994 compiler folds its stored leaves before emitting a quad.  The
 * later released compiler uses a different expression-tree representation. */
int FoldQuad(unsigned first,int op,unsigned second,unsigned type_leaf)
{
    LEAF left,right,result;
    int left_kind,right_kind,folded;
    if (first==0 || second==0 || !CanFoldQuad(op)) return 0;
    LeafGet(&left,first);
    LeafGet(&right,second);
    left_kind=ConstantKind(&left);
    right_kind=ConstantKind(&right);
    if (left_kind!=0 && right_kind!=0) {
        LeafGet(&result,type_leaf);
        if (left_kind==3 || right_kind==3) {
            ConstantToDouble(&left,left_kind);
            ConstantToDouble(&right,right_kind);
            folded=FoldDoubleQuad(&left,op,&right,&result);
        } else if (left_kind==2 || right_kind==2) {
            folded=FoldUnsignedQuad(&left,op,&right,&result);
        } else {
            folded=FoldSignedQuad(&left,op,&right,&result);
        }
        if (folded!=0) {
            if (!(left.leaf_flags&2)) FreeLeaf(first);
            if (!(right.leaf_flags&2)) FreeLeaf(second);
            NormalizeConstant(&result,folded);
            LeafReplace(&result,type_leaf);
        }
        return folded;
    }
    if (left_kind!=0) {
        if (right.leaf_type->decl_type>=8 && right.leaf_type->decl_type<=9 &&
            left.leaf_class!=0x0f) {
            ConstantToDouble(&left,left_kind);
            left.leaf_type=right.leaf_type;
            NormalizeConstant(&left,1);
            LeafReplace(&left,first);
        }
    } else if (right_kind!=0) {
        if (left.leaf_type->decl_type>=8 && left.leaf_type->decl_type<=9 &&
            right.leaf_class!=0x0f) {
            ConstantToDouble(&right,right_kind);
            right.leaf_type=left.leaf_type;
            NormalizeConstant(&right,1);
            LeafReplace(&right,second);
        }
    }
    return 0;
}

int CanFoldQuad(int op)
{
    switch (op) {
    case 0x0f: case 0x11: case 0x12:
    case 0x14: case 0x15: case 0x17:
    case 0x19: case 0x1a: case 0x1b: case 0x1c:
    case 0x1e: case 0x1f: case 0x20: case 0x22:
    case 0x25: case 0x28: case 0x2a: case 0x2c:
        return 1;
    }
    return 0;
}

int ConstantKind(LEAFPTR leaf)
{
    switch (leaf->leaf_class) {
    case 0x0a: case 0x0c:
        if (!(leaf->leaf_flags&0x100)) return 1;
        break;
    case 0x0b: case 0x0d:
        if (!(leaf->leaf_flags&0x100)) return 2;
        break;
    case 0x0e: case 0x0f:
        return 3;
    }
    return 0;
}

int FoldSignedQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result)
{
    int a,b;
    a=first->u.constant.value.integer;
    b=second->u.constant.value.integer;
    switch (op) {
    case 0x0f: result->u.constant.value.integer=(a==b); break;
    case 0x11: result->u.constant.value.integer=(a!=b); break;
    case 0x1a: result->u.constant.value.integer=(a>b); break;
    case 0x1b: result->u.constant.value.integer=(a>=b); break;
    case 0x1e: result->u.constant.value.integer=(a<b); break;
    case 0x1f: result->u.constant.value.integer=(a<=b); break;
    case 0x22: result->u.constant.value.integer=(unsigned)a+(unsigned)b; break;
    case 0x25: result->u.constant.value.integer=(unsigned)a-(unsigned)b; break;
    case 0x28:
        result->u.constant.value.integer=(unsigned)a*(unsigned)b;
        if ((double)result->u.constant.value.integer!=(double)a*(double)b) return 2;
        break;
    case 0x2a:
        if (b==0) { result->u.constant.value.integer=0; CErr1(0x48f); }
        else result->u.constant.value.integer=a/b;
        break;
    case 0x2c:
        if (b==0) { result->u.constant.value.integer=0; CErr1(0x48f); }
        else result->u.constant.value.integer=a%b;
        break;
    case 0x12: result->u.constant.value.integer=a|b; break;
    case 0x14: result->u.constant.value.integer=(a!=0 || b!=0); break;
    case 0x17: result->u.constant.value.integer=a&b; break;
    case 0x19: result->u.constant.value.integer=(a!=0 && b!=0); break;
    case 0x15: result->u.constant.value.integer=a^b; break;
    case 0x1c: result->u.constant.value.integer=a>>((unsigned)b&31); break;
    case 0x20: result->u.constant.value.integer=(unsigned)a<<((unsigned)b&31); break;
    default: return 0;
    }
    return 1;
}

void NormalizeConstant(LEAFPTR leaf,int kind)
{
    int type,value;
    type=leaf->leaf_type->decl_type;
    if (type==TYPE_POINTER && SizeOfArg(leaf->leaf_type)==4) type=TYPE_ULONG;
    switch (type) {
    case TYPE_CHAR:
        leaf->leaf_class=0x0a;
        leaf->leaf_flags=0x40;
        leaf->u.constant.value.integer=(signed char)leaf->u.constant.value.integer;
        break;
    case TYPE_UCHAR:
        leaf->leaf_class=0x0a;
        leaf->leaf_flags=0x40;
        leaf->u.constant.value.integer=(unsigned char)leaf->u.constant.value.integer;
        break;
    case TYPE_SHORT:
        leaf->leaf_class=0x0a;
        leaf->leaf_flags=0x40;
        leaf->u.constant.value.integer=(short)leaf->u.constant.value.integer;
        break;
    case TYPE_USHORT:
        leaf->leaf_class=0x0b;
        leaf->leaf_flags=0x40;
        leaf->u.constant.value.integer=(unsigned short)leaf->u.constant.value.integer;
        break;
    case TYPE_INT:
        value=leaf->u.constant.value.integer;
        /* The original passes the same retained value to both arguments. */
        if (kind!=2) CheckConstantTruncation(value,value);
        leaf->leaf_class=0x0a;
        leaf->leaf_flags=0x40;
        leaf->u.constant.value.integer=value;
        break;
    case TYPE_UINT: case TYPE_POINTER: leaf->leaf_class=0x0b; leaf->leaf_flags=0x40; break;
    case TYPE_LONG: leaf->leaf_class=0x0c; leaf->leaf_flags=0x40; break;
    case TYPE_ULONG: leaf->leaf_class=0x0d; leaf->leaf_flags=0x40; break;
    case TYPE_FLOAT: leaf->leaf_class=0x0e; leaf->leaf_flags=0x40; break;
    case TYPE_DOUBLE: leaf->leaf_class=0x0f; leaf->leaf_flags=0x40; break;
    }
    if (kind==2) CWarn1(6,0x6a);
}

void CheckConstantTruncation(unsigned first,unsigned second)
{
    if (first==second) return;
    first&=0xffff0000;
    if (first==0) first=0xffff0000;
    second&=0xffff0000;
    if (second==0) second=0xffff0000;
    if (first!=second) CWarn1(6,0x6a);
}

unsigned AndAnd(unsigned index)
{
    LEAF leaf;
    LeafGet(&leaf,index);
    if ((leaf.leaf_flags&0x40) && leaf.u.constant.value.integer==0) {
        CurToken=0;
        ++SizeOfCount;
    }
    return index;
}

unsigned FlowOp(unsigned first,unsigned char op,unsigned second)
{
    LEAF leaf;
    LeafGet(&leaf,first);
    if (leaf.leaf_class==0x13) return first;
    if (leaf.leaf_flags&0x40) {
        FreeLeaf(first);
        return second;
    }
    LeafGet(&leaf,second);
    if (leaf.leaf_class==0x13) return second;
    if (leaf.leaf_flags&0x40) {
        if ((op==0x14 && leaf.u.constant.value.integer==0) ||
            (op!=0x14 && leaf.u.constant.value.integer!=0)) {
            FreeLeaf(second);
            return first;
        }
        op=2;
    }
    return GenQuad(first,op,second,TypeLeaf(GetType(TYPE_INT)));
}

unsigned Expr(void)
{
    Class[Level]=TC_START;
    return GetExpr();
}

void IncSymWeight(SYMPTR sym)
{
    static int LoopWeights[]={1,0x10,0x100,0x1000};
    if (sym->level!=0 && !(sym->flags&SYM_FUNCTION)) {
        if (LoopDepth>3) sym->u.var.offset=0x7fffffff;
        else sym->u.var.offset+=LoopWeights[LoopDepth];
    }
}

unsigned ArrowOp(unsigned leaf)
{
    if (CurToken!=0x33) {
        CErr1(0x3f8);
        return DefaultLeaf;
    }
    return FieldOp(leaf,Buffer,0);
}

/* Shared old arrow-field operation. The optional initial offset is carried
 * through anonymous aggregate lookup; this is not the later tree DotOp. */
unsigned FieldOp(unsigned index,char *name,unsigned offset)
{
    LEAF leaf;
    TYPEPTR typ,class_type,field_type;
    FIELDPTR field;
    unsigned type_leaf;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return index;
    if (Ptr2Struct(LeafType(&leaf))==NULL) {
        CErr1(0x409);
        return DefaultLeaf;
    }
    if (leaf.leaf_class!=0x12) {
        index=LoadLeafValue(index);
        LeafGet(&leaf,index);
    }
    typ=leaf.leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    leaf.leaf_flags&=~1;
    if (typ->decl_type==TYPE_POINTER) {
        typ=typ->object;
        while (typ->decl_type==TYPE_TYPEDEF) {
            leaf.leaf_flags|=typ->decl_flags&3;
            typ=typ->object;
        }
    }
    class_type=typ;
    field=SearchFields(&class_type,&offset,name);
    if (field==NULL) {
        CErr(0x407,name,typ->u.tag->name);
        return DefaultLeaf;
    }
    field_type=field->field_type;
    while (field_type->decl_type==TYPE_TYPEDEF) field_type=field_type->object;
    if (leaf.leaf_flags&0x800) {
        type_leaf=TypeLeaf(field_type);
        index=GenQuad(0,0x74,index,type_leaf);
        LeafGet(&leaf,index);
        leaf.leaf_flags|=8;
    }
    leaf.leaf_flags|=typ->decl_flags&3;
    leaf.leaf_type=field_type;
    leaf.attributes=0;
    leaf.leaf_flags|=0x20|(field_type->decl_flags&3);
    if (leaf.leaf_class==0x0b || leaf.leaf_class==0x0d) {
        leaf.leaf_flags|=0x100;
        leaf.u.constant.value.integer+=offset;
    } else {
        leaf.u.symbol.offset=offset;
        switch (leaf.leaf_class) {
        case 0: case 1: leaf.leaf_class=3; break;
        case 4: case 5: case 8: leaf.leaf_class=7; break;
        case 0x12: leaf.leaf_class=1; break;
        }
    }
    LeafReplace(&leaf,index);
    return index;
}

TYPEPTR Ptr2Struct(TYPEPTR typ)
{
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type!=TYPE_POINTER) return NULL;
    typ=typ->object;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_STRUCT || typ->decl_type==TYPE_UNION) return typ;
    return NULL;
}

/* Earlier leaf load, distinct from cexpr2's general RValue conversion. */
unsigned LoadLeafValue(unsigned index)
{
    LEAF leaf;
    unsigned type_leaf;
    LeafGet(&leaf,index);
    switch (leaf.leaf_class) {
    case 1: case 2: case 3: case 5: case 6: case 7: case 8: case 9:
        type_leaf=TypeLeaf(leaf.leaf_type);
        index=GenQuad(0,0x28,index,type_leaf);
        break;
    case 0x12:
        index=StringAddress(&leaf,index);
        break;
    }
    return index;
}

FIELDPTR SearchFields(TYPEPTR *class_type,unsigned *offset,char *name)
{
    FIELDPTR field,subfield;
    TYPEPTR typ;
    field=(*class_type)->u.tag->u.field_list;
    while (field!=NULL) {
        typ=field->field_type;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (field->name[0]=='\0') {
            if (typ->decl_type==TYPE_STRUCT || typ->decl_type==TYPE_UNION) {
                subfield=SearchFields(&typ,offset,name);
                if (subfield!=NULL) {
                    *offset+=field->offset;
                    *class_type=typ;
                    return subfield;
                }
            }
        } else if (strcmp(name,field->name)==0) {
            if (typ->decl_type!=TYPE_FUNCTION) *offset+=field->offset;
            return field;
        }
        field=field->next_field;
    }
    return NULL;
}

unsigned DotOp(unsigned leaf)
{
    if (CurToken!=0x33) {
        CErr1(0x3f8);
        return DefaultLeaf;
    }
    return DotField(leaf,Buffer,0);
}

/* Direct aggregate member access in the old leaf representation. */
unsigned DotField(unsigned index,char *name,unsigned offset)
{
    LEAF leaf;
    QUAD quad;
    SYM_ENTRY sym;
    TYPEPTR typ,class_type,field_type;
    FIELDPTR field;
    SYM_HANDLE handle;
    unsigned left;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return index;
    typ=leaf.leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) {
        leaf.leaf_flags|=typ->decl_flags&3;
        typ=typ->object;
    }
    leaf.leaf_flags|=typ->decl_flags&3;
    if (typ->decl_type!=TYPE_STRUCT && typ->decl_type!=TYPE_UNION) {
        CErr1(0x408);
        return DefaultLeaf;
    }
    class_type=typ;
    field=SearchFields(&class_type,&offset,name);
    if (field==NULL) {
        CErr(0x407,name,typ->u.tag->name);
        return DefaultLeaf;
    }
    leaf.leaf_flags|=field->field_type->decl_flags&3;
    field_type=field->field_type;
    while (field_type->decl_type==TYPE_TYPEDEF) field_type=field_type->object;
    switch (leaf.leaf_class) {
    case 0x0b: case 0x0d:
        leaf.u.constant.value.integer+=offset;
        leaf.leaf_type=field_type;
        LeafReplace(&leaf,index);
        return index;
    case 0: leaf.leaf_class=1; break;
    case 6: case 9: leaf.leaf_class=7; break;
    case 2: leaf.leaf_class=3; break;
    case 1: case 3: case 5: case 7: break;
    default:
        InitLeaf(&leaf);
        leaf.leaf_class=5;
        leaf.leaf_flags=typ->decl_flags&0x1f;
        leaf.u.symbol.handle=index;
        leaf.u.symbol.offset=0;
        QuadGet(&quad,QuadIndex-1);
        if (quad.quad_class==2) {
            quad.quad_class=0x7d;
            QuadReplace(&quad,QuadIndex-1);
        } else if (quad.quad_class==0x0e) {
            handle=MakeNewSym(&sym,'S',typ,SC_AUTO);
            left=MakeSymLeaf(&sym,handle);
            AsgnOp(left,0x0e,index);
            FinishStmtExpr(1);
            MakeSymLeaf(&sym,handle);
            sym.flags|=0x40;
            SymReplace(&sym,handle);
            leaf.leaf_class=1;
            leaf.u.symbol.handle=handle;
        } else if (quad.quad_class==8) {
            leaf.leaf_flags|=0x20;
            leaf.attributes=0;
        }
        index=GenLeaf(&leaf);
    }
    leaf.u.symbol.offset+=offset;
    leaf.leaf_type=field_type;
    LeafReplace(&leaf,index);
    return index;
}

/* Older ccheck.c hook: compatibility conversion is done by AsgnOp later,
 * rather than an additional AsgnCheck vertex in this function. */
void ChkRetType(unsigned index)
{
    TYPEPTR ret_type,func_type;
    ret_type=TypeOf(index);
    func_type=CurFunc->sym_type->object;
    if (func_type->decl_type==TYPE_VOID) CErr2p(0x41b,CurFunc->name);
    else if (ret_type->decl_type==TYPE_POINTER && ret_type->u.p.segment==10) CWarn1(0x17,0x74);
}

unsigned AsgnOp(unsigned first,unsigned char op,unsigned second)
{
    LEAF left,right;
    SYM_ENTRY sym;
    TYPEPTR typ;
    unsigned type_leaf;
    if (!LValue(first)) return first;
    if (op!=0x0e && op!=0x77) {
        if (op==0x23 || op==0x26) return AddOp(first,op,second);
        return BinOp(first,op,second);
    }
    LeafGet(&left,first);
    LeafGet(&right,second);
    if (left.leaf_class==0 || left.leaf_class==1) {
        SymGet(&sym,left.u.symbol.handle);
        if (sym.level!=0 && !(sym.flags&0x40)) {
            sym.flags|=0x40;
            SymReplace(&sym,left.u.symbol.handle);
        }
    }
    if ((right.leaf_flags&0x40) && left.leaf_class==0 &&
        !(left.leaf_type->decl_flags&2) &&
        left.leaf_type->decl_type>=TYPE_INT && left.leaf_type->decl_type<=TYPE_ULONG) {
        right.leaf_flags|=2;
        LeafReplace(&right,second);
        /* The original prepares EAX=symbol,EDX=constant before the same
         * empty C3 hook used by Statement. Neither is consumed. The later
         * source dropped this instrumentation; no second identity or
         * parameter semantics are inferred from the dead register setup. */
        StmtInit();
    }
    typ=TypeOf(first);
    type_leaf=TypeLeaf(typ);
    second=RValue(second);
    AsgnCheck(typ,second);
    GenQuad(first,op,second,type_leaf);
    return type_leaf;
}

int LValue(unsigned index)
{
    LEAF leaf;
    TYPEPTR typ;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return 1;
    if (leaf.leaf_flags&0x20) {
        typ=TypeOf(index);
        if (typ->decl_type!=TYPE_ARRAY) {
            if (TypeSize(typ)==0) CErr1(0x459);
            if (leaf.attributes!=0) {
                leaf.attributes=0;
                LeafReplace(&leaf,index);
                CWarn1(0x1c,0x78);
            }
            return 1;
        }
    }
    CErr1(0x3f6);
    return 0;
}

/* Older ccheck.c: qualifier/pointer and constant-range diagnostics remain
 * inline here, rather than the later separate AssRangeChk helper. */
void AsgnCheck(TYPEPTR typ,unsigned index)
{
    LEAF leaf;
    int compatibility,value;
    unsigned uvalue;
    /* Historical FLT_MAX decimal conversion is one binary64 ULP below
     * today's conversion of 3.402823466E38. Keep that encountered threshold
     * exactly, independently of the host compiler's decimal conversion. */
    static union { unsigned words[2]; double value; } FloatUpperLimit=
        {{0xdff07035U,0x47efffffU}};
    LeafGet(&leaf,index);
    compatibility=CompatibleType(typ,leaf.leaf_type);
    switch (compatibility) {
    case 0: CErr1(0x3f2); break;
    case 1: CWarn1(4,0x68); break;
    case 2: case 5: CWarn1(0x0d,0x71); break;
    case 6: CWarn1(0x0c,0x70); break;
    case 3:
        if (leaf.leaf_flags&0x40) {
            if (leaf.u.constant.value.integer!=0) CWarn1(1,0x65);
        } else CWarn1(2,0x66);
        break;
    case 4: case 8:
        if (!(leaf.leaf_flags&0x40)) break;
        value=leaf.u.constant.value.integer;
        uvalue=value;
        switch (typ->decl_type) {
        case TYPE_CHAR:
            if (value>127 || value< -128) CWarn1(6,0x6a);
            break;
        case TYPE_UCHAR:
            if (uvalue>255) CWarn1(6,0x6a);
            break;
        case TYPE_SHORT:
            if (value>32767 || value< -32768) CWarn1(6,0x6a);
            break;
        case TYPE_USHORT:
            if (uvalue>65535 && (uvalue&0xffff0000U)!=0xffff0000U) CWarn1(6,0x6a);
            break;
        case TYPE_FLOAT:
            if (leaf.u.constant.value.floating>FloatUpperLimit.value) CWarn1(6,0x6a);
            break;
        }
        break;
    }
}

int CompatibleType(TYPEPTR first,TYPEPTR second)
{
    unsigned first_size,second_size;
    while (first->decl_type==TYPE_TYPEDEF) first=first->object;
    if (first->decl_type==TYPE_ENUM) first=first->object;
    while (second->decl_type==TYPE_TYPEDEF) second=second->object;
    if (second->decl_type==TYPE_ENUM) second=second->object;
    if (first==second) return 4;
    if (first->decl_type==TYPE_POINTER && second->decl_type==TYPE_POINTER) {
        first_size=TypeSize(first);
        second_size=TypeSize(second);
        if (first_size<second_size) return 6;
        if (first_size>second_size) return 7;
    }
    return DoCompatibleType(first,second,1);
}

/* cmath2.c arithmetic result rules, before long-long rows/columns existed.
 * Typed semantic tables preserve the 32-bit promotion rules. */
static const unsigned char AddResult[13][13]={
    /* CHR */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* UCH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* SHT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* USH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* INT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* UIN */ { TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* LNG */ { TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_ULONG, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* ULN */ { TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* FLT */ { TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* DBL */ { TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 255, 255, 255 },
    /* PTR */ { TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, 255, 255, 255, 255, 255 },
    /* ARR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* STC */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
static const unsigned char SubResult[13][13]={
    /* CHR */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* UCH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* SHT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* USH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* INT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* UIN */ { TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* LNG */ { TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_ULONG, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* ULN */ { TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER, 255, 255 },
    /* FLT */ { TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* DBL */ { TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 255, 255, 255 },
    /* PTR */ { TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, TYPE_POINTER, 255, 255, TYPE_INT, 255, 255 },
    /* ARR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* STC */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};

unsigned AddOp(unsigned first,unsigned char op,unsigned second)
{
    TYPEPTR first_type,second_type,result_type;
    DATA_TYPE first_kind,second_kind,result_kind;
    unsigned folded,size,type_leaf;
    switch (op) {
    case 0x23: case 0x24: case 0x26: case 0x27:
        LValue(first);
        break;
    case 0x22: case 0x25:
        folded=ArrayAddConst(first,op,second);
        if (folded!=0) return folded;
        /* fall through */
    default:
        first=RValue(first);
    }
    second=RValue(second);
    first_type=TypeOf(first);
    second_type=TypeOf(second);
    first_kind=DataTypeOf(first_type->decl_type);
    second_kind=DataTypeOf(second_type->decl_type);
    if (first_kind==TYPE_STRUCT || first_kind==TYPE_UNION ||
        second_kind==TYPE_STRUCT || second_kind==TYPE_UNION) result_kind=255;
    else if (first_kind==TYPE_VOID || second_kind==TYPE_VOID || first_kind==TYPE_ARRAY) result_kind=TYPE_INT;
    else {
        switch (op) {
        case 0x22:
            result_kind=AddResult[first_kind][second_kind];
            break;
        case 0x23: case 0x24: case 0x26: case 0x27:
            result_kind=first_kind;
            if (second_kind==TYPE_STRUCT || second_kind==TYPE_UNION) result_kind=255;
            else if (first_kind==TYPE_POINTER && second_kind>=TYPE_FLOAT) CErr1(0x437);
            break;
        case 0x25:
        default:
            result_kind=SubResult[first_kind][second_kind];
            if (first_kind==TYPE_POINTER && second_kind==TYPE_POINTER) {
                if (!CompatiblePointers(first_type,second_type)) CErr1(0x3f2);
                if ((first_type->decl_flags&0x10) || (second_type->decl_flags&0x10) ||
                    ((TargetSwitches&0x14)==4 && !(first_type->decl_flags&0x0c) &&
                     !(second_type->decl_flags&0x0c))) result_kind=TYPE_LONG;
            }
        }
    }
    if (result_kind==255) {
        result_kind=TYPE_INT;
        CErr1(0x439);
    }
    if (first_kind==TYPE_POINTER) {
        result_type=TypeOf(first);
        size=SizeOfArg(result_type->object);
        if (size==0) { size=1; CErr1(0x42a); }
        if (result_kind==TYPE_INT || result_kind==TYPE_LONG) result_type=GetType(result_kind);
        if (size!=1) {
            if (result_kind==TYPE_INT || result_kind==TYPE_LONG) {
                type_leaf=TypeLeaf(result_type);
                GenQuad(first,op,second,type_leaf);
                return PtrSubtract(type_leaf,size,result_kind);
            }
            if ((first_type->decl_flags&0x10) || (TargetSwitches&0x14)==4) {
                if (second_kind!=TYPE_LONG && second_kind!=TYPE_ULONG)
                    second=CnvOp(second,GetType(TYPE_LONG),1);
            }
            second=MulByConst(second,size);
        }
    } else if (second_kind==TYPE_POINTER) {
        if (op==0x23 || op==0x26 || op==0x25) CErr2p(0x40c,Tokens[op]);
        result_type=TypeOf(second);
        size=SizeOfArg(result_type->object);
        if (size==0) { size=1; CErr1(0x42a); }
        if (size!=1) {
            if ((second_type->decl_flags&0x10) || (TargetSwitches&0x14)==4) {
                /* Preserve the old code's conversion of operand2 here. */
                if (first_kind!=TYPE_LONG && first_kind!=TYPE_ULONG)
                    second=CnvOp(second,GetType(TYPE_LONG),1);
            }
            first=MulByConst(first,size);
        }
    } else result_type=GetType(result_kind);
    type_leaf=TypeLeaf(result_type);
    return GenQuad(first,op,second,type_leaf);
}

/* Combined +/- constant-address folder in the old leaf representation. */
unsigned ArrayAddConst(unsigned first,unsigned char op,unsigned second)
{
    LEAF left,right;
    unsigned result;
    LeafGet(&left,first);
    if (left.leaf_class==0x13) return first;
    LeafGet(&right,second);
    if (right.leaf_class==0x13) return second;
    if (left.u.array_address==1) {
        left.u.array_address=0;
        LeafReplace(&left,first);
        CWarn1(0x0f,0x73);
    }
    if (right.u.array_address==1) {
        right.u.array_address=0;
        LeafReplace(&right,second);
        CWarn1(0x0f,0x73);
    }
    result=FoldArrayOffset(&left,first,op,&right,second);
    if (result==0) result=FoldArrayOffset(&right,second,op,&left,first);
    return result;
}

unsigned FoldArrayOffset(LEAFPTR first,unsigned first_index,int op,LEAFPTR second,unsigned second_index)
{
    TYPEPTR typ;
    unsigned delta;
    if (!(second->leaf_flags&0x40)) return 0;
    typ=first->leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type!=TYPE_ARRAY && first->leaf_class!=0x12) return 0;
    switch (first->leaf_class) {
    case 0: first->leaf_class=1; break;
    case 1: case 0x12: break;
    default: return 0;
    }
    delta=SizeOfArg(typ->object)*(unsigned)second->u.constant.value.integer;
    if (op==0x22) first->u.symbol.offset=(unsigned)first->u.symbol.offset+delta;
    else first->u.symbol.offset=(unsigned)first->u.symbol.offset-delta;
    LeafReplace(first,first_index);
    FreeLeaf(second_index);
    return first_index;
}

/* The old leaf frontend carries constness on both the leaf and typedef chain. */
void ChkConst(unsigned operand)
{
    LEAF leaf;
    TYPEPTR typ;
    LeafGet(&leaf,operand);
    if (leaf.leaf_class==0x13) return;
    typ=leaf.leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF && !(typ->decl_flags&1))
        typ=typ->object;
    if ((typ->decl_flags&1) || (leaf.leaf_flags&1)) CErr1(0x3f5);
}

/* cmath2.c promotion matrices, without the later long-long rows/columns. */
static const unsigned char IntResult[13][13]={
    /* CHR */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* UCH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* SHT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* USH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* INT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* UIN */ { TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_ULONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* LNG */ { TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_ULONG, TYPE_LONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* ULN */ { TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, 255, 255, 255, 255, 255 },
    /* FLT */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* DBL */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* PTR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* ARR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* STC */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
static const unsigned char BinResult[13][13]={
    /* CHR */ { TYPE_CHAR, TYPE_UCHAR, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* UCH */ { TYPE_UCHAR, TYPE_UCHAR, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* SHT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* USH */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* INT */ { TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* UIN */ { TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* LNG */ { TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_LONG, TYPE_ULONG, TYPE_LONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* ULN */ { TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_ULONG, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* FLT */ { TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_DOUBLE, 255, 255, 255 },
    /* DBL */ { TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 255, 255, 255 },
    /* PTR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* ARR */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    /* STC */ { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};

unsigned BinOp(unsigned first,unsigned char op,unsigned second)
{
    TYPEPTR first_type;
    DATA_TYPE first_kind,second_kind,result_kind;
    LEAF leaf;
    unsigned maximum,type_leaf;
    first=RValue(first);
    second=RValue(second);
    first_type=TypeOf(first);
    first_kind=DataTypeOf(first_type->decl_type);
    second_kind=DataTypeOf(TypeOf(second)->decl_type);
    if (first_kind==TYPE_VOID || second_kind==TYPE_VOID) result_kind=TYPE_VOID;
    else if (first_kind==TYPE_STRUCT || first_kind==TYPE_UNION ||
             second_kind==TYPE_STRUCT || second_kind==TYPE_UNION) result_kind=255;
    else {
        result_kind=BinResult[first_kind][second_kind];
        if (op==0x2c && result_kind>=TYPE_FLOAT) CErr1(0x437);
        if (result_kind<TYPE_INT) result_kind=TYPE_INT;
    }
    if (result_kind==255) { result_kind=TYPE_INT; CErr1(0x438); }
    switch (op) {
    case 0x13: case 0x16:
        LeafGet(&leaf,second);
        if (leaf.leaf_flags&0x40) {
            switch (TypeSize(first_type)) {
            case 1: maximum=0xff; break;
            case 2: maximum=0xffff; break;
            default: maximum=0xffffffffU; break;
            }
            if ((unsigned)leaf.u.constant.value.integer>maximum) CWarn1(6,0x6a);
        }
        /* fall through */
    case 0x18: case 0x1d: case 0x21: case 0x2d:
        if (IntResult[first_kind][second_kind]==255) CErr1(0x437);
        /* fall through */
    case 0x29: case 0x2b:
        result_kind=first_kind;
        break;
    }
    type_leaf=TypeLeaf(GetType(result_kind));
    return GenQuad(first,op,second,type_leaf);
}

unsigned GetInitExpr(void)
{
    Class[Level]=TC_START1; /* assignment permitted, comma ends initializer */
    return GetExpr();
}

unsigned IndexOp(unsigned first,unsigned second)
{
    TYPEPTR typ;
    unsigned result;
    result=first;
    typ=TypeOf(first);
    if (typ->decl_type==TYPE_ARRAY) result=ArrayIndex(first,second,typ);
    else if (typ->decl_type==TYPE_POINTER) {
        if (Toggles&4) result=GenIndex(first,second,typ);
        else result=PtrOp(AddOp(first,0x22,second));
    } else {
        typ=TypeOf(second);
        if (typ->decl_type==TYPE_ARRAY) result=ArrayIndex(second,first,typ);
        else if (typ->decl_type==TYPE_POINTER) {
            if (Toggles&4) result=GenIndex(second,first,typ);
            else result=PtrOp(AddOp(second,0x22,first));
        } else CErr1(0x415);
    }
    MustRecog(9);
    return result;
}

unsigned MulByConst(unsigned index,unsigned amount)
{
    LEAF leaf;
    unsigned constant,type_leaf;
    DATA_TYPE kind;
    LeafGet(&leaf,index);
    kind=leaf.leaf_type->decl_type;
    if (kind==TYPE_LONG || kind==TYPE_ULONG) {
        type_leaf=TypeLeaf(GetType(TYPE_LONG));
        constant=LongLeaf((long)amount);
    } else {
        type_leaf=TypeLeaf(GetType(TYPE_INT));
        constant=IntLeaf((int)amount);
    }
    return GenQuad(index,0x28,constant,type_leaf);
}

unsigned PtrOp(unsigned index)
{
    LEAF leaf;
    TYPEPTR pointer_type,object_type;
    SYM_ENTRY sym;
    SYM_HANDLE handle;
    unsigned value;
    char saved_literal[sizeof(SYM_ENTRY)];
    STRING_LITERAL *literal;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return index;
    pointer_type=LeafType(&leaf);
    if (leaf.leaf_class==0x10) {
        literal=(STRING_LITERAL *)saved_literal;
        ReadLiteralChunk(saved_literal,leaf.u.string.handle);
        value=(unsigned char)literal->literal[0];
        if (pointer_type->object->decl_type==TYPE_USHORT)
            value|=(unsigned)(unsigned char)literal->literal[1]<<8;
        --leaf.u.string.references;
        LeafReplace(&leaf,index);
        return IntLeaf(value);
    }
    if (pointer_type->decl_type!=TYPE_POINTER) {
        CErr1(0x405);
        return DefaultLeaf;
    }
    if (leaf.leaf_class>=0x0a && leaf.leaf_class<=0x0d) {
        handle=MakeNewSym(&sym,'P',pointer_type,SC_AUTO);
        AsgnOp(MakeSymLeaf(&sym,handle),0x0e,index);
        FinishStmtExpr(1);
        index=MakeSymLeaf(&sym,handle);
        sym.flags|=SYM_ASSIGNED;
        SymReplace(&sym,handle);
    }
    index=LoadLeafValue(index);
    LeafGet(&leaf,index);
    if (leaf.leaf_class==4) {
        leaf.u.symbol.handle=0;
        if (pointer_type->decl_flags&0x800)
            leaf.u.symbol.handle=pointer_type->u.p.based_sym;
    }
    leaf.leaf_flags&=~1;
    object_type=pointer_type->object;
    while (object_type->decl_type==TYPE_TYPEDEF) object_type=object_type->object;
    leaf.leaf_flags|=0x20;
    leaf.attributes=0;
    leaf.leaf_type=object_type;
    switch (leaf.leaf_class) {
    case 4: case 5: case 8: leaf.leaf_class=6; break;
    case 0: case 1: leaf.leaf_class=2; break;
    }
    LeafReplace(&leaf,index);
    return index;
}

/* cexpr2.c's earlier quad call setup, including the original nested plist stack. */
unsigned StartFunc(unsigned index,TYPEPTR **plist,unsigned first_quad)
{
    LEAF leaf;
    TYPEPTR typ,original_type;
    SYM_ENTRY sym;
    SYM_HANDLE handle;
    struct nested_parm_lists *nested;
    int recursive;
    unsigned char op;
    char *name;
    recursive=0;
    if (*plist!=NULL) {
        nested=CMemAlloc(sizeof(*nested));
        nested->prev_list=NestedParms;
        nested->next_parm_type=*plist;
        NestedParms=nested;
        *plist=NULL;
    }
    LeafGet(&leaf,index);
    typ=original_type=leaf.leaf_type;
    if (typ->decl_type==TYPE_POINTER) {
        original_type=typ->object;
        typ=original_type;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type==TYPE_FUNCTION) {
            index=PtrOp(index);
            LeafGet(&leaf,index);
        }
    }
    if (leaf.leaf_class!=0) {
        if (CompFlags.initializing_data) CErr1(0x41e);
        else {
            handle=MakeNewSym(&sym,'F',original_type,SC_AUTO);
            sym.flags|=SYM_FUNCTION;
            sym.attrib=typ->decl_flags;
            SymReplace(&sym,handle);
            index=GenQuad(0,T_ID,index,MakeSymLeaf(&sym,handle));
        }
    } else {
        if (leaf.u.symbol.handle==CurFuncHandle) recursive=1;
        if (first_quad==QuadIndex) {
            leaf.leaf_flags|=0x8000;
            LeafReplace(&leaf,index);
        }
    }
    if (typ->decl_type!=TYPE_FUNCTION) {
        CErr1(0x3f4);
        typ=FuncNode(GetType(TYPE_INT),0,NULL);
        leaf.leaf_type=typ;
        LeafReplace(&leaf,index);
    } else {
        if ((typ->decl_flags&0x2000) && !DeadCode) CurFuncSym.flags|=0x800;
        *plist=typ->u.parms;
        do { typ=typ->object; } while (typ->decl_type==TYPE_TYPEDEF);
    }
    NextToken();
    if (CurToken!=7) {
        ++Level;
        Class[Level]=TC_PARM_LIST;
        Token[Level]=6;
        ValueStack[Level]=index;
        return 0;
    }
    op=T_CALL_NO_PARM;
    if (CompFlags.exception_filter_expr) {
        LeafGet(&leaf,index);
        if (leaf.leaf_class==0) {
            handle=leaf.u.symbol.handle;
            SymGet(&sym,handle);
            name=SymName(&sym,handle);
            if (strcmp(name,"_exception_code")==0) op=0x84;
            else if (strcmp(name,"_exception_info")==0) op=0x85;
        }
    }
    if (op!=T_CALL_NO_PARM) index=GenQuad(0,op,0,CallResultLeaf(typ));
    else {
        index=GenQuad(0,op,index,CallResultLeaf(typ));
        if (recursive) MarkRecursiveCall();
        LinkCallQuad();
    }
    CompFlags.meaningless_stmt=0;
    CompFlags.useful_side_effect=1;
    NextToken();
    PopNestedParms(plist);
    return index;
}

unsigned AddrOp(unsigned index,int address_array)
{
    LEAF leaf;
    TYPEPTR typ,sym_type;
    SYM_ENTRY sym;
    unsigned flags,type_leaf;
    SYM_HANDLE based_sym;
    int segment;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return index;
    typ=leaf.leaf_type;
    if (typ->decl_type==TYPE_FIELD || typ->decl_type==TYPE_UFIELD) {
        CErr1(0x41d);
        return DefaultLeaf;
    }
    if (typ->decl_type==TYPE_ARRAY) {
        if (!address_array) {
            do { typ=typ->object; } while (typ->decl_type==TYPE_TYPEDEF);
        } else if (leaf.leaf_class!=0x10) leaf.u.array_address=1;
    }
    flags=leaf.leaf_flags&0x1c;
    segment=4;
    switch (leaf.leaf_class) {
    case 0x10:
        if (!address_array) {
            typ=leaf.leaf_type;
            if (typ->object->decl_type==TYPE_USHORT)
                leaf.leaf_type=PtrNode(typ->object,0,4);
            else {
                StringArrayType=typ;
                leaf.leaf_type=StringType;
            }
            LeafReplace(&leaf,index);
            break;
        }
        /* fall through */
    case 0x0b: case 0x0d:
        if (leaf.leaf_flags&0x20) {
            leaf.leaf_flags&=0xfedf;
            leaf.leaf_type=TypeNode(TYPE_POINTER,typ);
            LeafReplace(&leaf,index);
            break;
        }
        CErr1(0x406);
        break;
    case 4: case 5:
        if (!(leaf.leaf_flags&0x20)) { CErr1(0x406); break; }
        /* fall through */
    case 3: case 7: case 9:
        type_leaf=TypeLeaf(PtrNode(typ,flags,segment));
        index=GenQuad(0,0x71,index,type_leaf);
        if (address_array && typ->decl_type==TYPE_ARRAY) {
            LeafGet(&leaf,index);
            leaf.u.array_address=1;
            LeafReplace(&leaf,index);
        }
        break;
    case 6:
        leaf.leaf_class=4;
        leaf.leaf_type=PtrNode(typ,flags,4);
        leaf.leaf_flags&=~0x20;
        leaf.attributes=0;
        LeafReplace(&leaf,index);
        break;
    case 2:
        if (typ->decl_type==TYPE_VOID) CErr1(0x406);
        SymGet(&sym,leaf.u.symbol.handle);
        leaf.leaf_class=0;
        if (sym.stg_class==SC_AUTO) segment=10;
        leaf.leaf_type=PtrNode(typ,flags,segment);
        LeafReplace(&leaf,index);
        break;
    case 0: case 1:
        SymGet(&sym,leaf.u.symbol.handle);
        if (sym.stg_class==0x62) CErr1(0x42b);
        if (sym.level!=0) sym.flags|=SYM_ASSIGNED;
        sym.flags|=0x82;
        SymReplace(&sym,leaf.u.symbol.handle);
        flags=sym.attrib&0x81c;
        based_sym=0;
        if (flags&0x800) {
            sym_type=sym.sym_type;
            while (sym_type->decl_type==TYPE_TYPEDEF) sym_type=sym_type->object;
            if (sym_type->decl_type==TYPE_POINTER) based_sym=sym_type->u.p.based_sym;
        } else if (sym.stg_class==SC_AUTO) {
            segment=10;
            if (TargetSwitches&0x200) flags=8;
        }
        leaf.leaf_class=0x12;
        leaf.leaf_type=BasedPtrNode(typ,flags,segment,based_sym);
        leaf.leaf_flags&=~0x20;
        leaf.attributes=0;
        LeafReplace(&leaf,index);
        break;
    default:
        CErr1(0x406);
        break;
    }
    if (segment==10) CompFlags.addr_of_auto_taken=1;
    return index;
}

/* The old argument converter precedes later ParmAsgnCheck/ParmAss helpers. */
unsigned GenNextParm(unsigned index,TYPEPTR **plistptr)
{
    TYPEPTR typ,parm_type,object;
    TYPEPTR *plist;
    LEAF leaf;
    unsigned type_leaf;
    index=RValue(index);
    typ=TypeOf(index);
    if (typ->decl_type==TYPE_FUNCTION) {
        typ=PtrNode(typ,typ->decl_flags,1);
        type_leaf=TypeLeaf(typ);
        index=GenQuad(0,0x71,index,type_leaf);
    }
    plist=*plistptr;
    if (plist!=NULL && (*plist==NULL || (*plist)->decl_type==TYPE_DOT_DOT_DOT))
        plist=NULL;
    if (plist!=NULL) {
        parm_type=*plist;
        if (parm_type!=NULL) {
            while (parm_type->decl_type==TYPE_TYPEDEF) parm_type=parm_type->object;
            if (parm_type!=typ) {
                if (parm_type->decl_type==TYPE_POINTER && typ->decl_type!=TYPE_POINTER) {
                    LeafGet(&leaf,index);
                    if ((leaf.leaf_flags&0x40) && leaf.u.constant.value.integer==0) {
                        typ=parm_type;
                        index=CnvOp(index,typ,0);
                    }
                } else if ((typ->decl_type!=TYPE_POINTER || parm_type->decl_type==TYPE_POINTER) &&
                           CompatiblePointers(typ,parm_type)) {
                    typ=parm_type;
                    if (typ->decl_type!=TYPE_STRUCT && typ->decl_type!=TYPE_UNION)
                        index=CnvOp(index,typ,0);
                }
            }
            ++*plistptr;
        }
    } else if (typ->decl_type==TYPE_FLOAT) {
        typ=GetType(TYPE_DOUBLE);
        index=CnvOp(index,typ,0);
    } else if (typ->decl_type==TYPE_POINTER) {
        object=typ->object;
        while (object->decl_type==TYPE_TYPEDEF) object=object->object;
        if (object->decl_type!=TYPE_FUNCTION && (typ->decl_flags&4) && DataPtrSize==6) {
            typ=PtrNode(object,0,4);
            index=CnvOp(index,typ,0);
        }
    }
    if (typ->decl_type==TYPE_POINTER) typ=typ->object;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_DOUBLE || typ->decl_type==TYPE_FLOAT) CompFlags.float_used=1;
    return index;
}

/* Array rvalue conversion shares the original historical helper identity. */
unsigned StringAddress(LEAFPTR leaf,unsigned index)
{
    TYPEPTR typ;
    SYM_ENTRY sym;
    unsigned type_leaf;
    typ=leaf->leaf_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    leaf->leaf_class=0;
    SymGet(&sym,leaf->u.symbol.handle);
    if (!(sym.flags&0x80)) {
        sym.flags|=0x80;
        SymReplace(&sym,leaf->u.symbol.handle);
    }
    if (leaf->u.symbol.offset!=0) leaf->leaf_class=1;
    leaf->leaf_type=typ->object;
    LeafReplace(leaf,index);
    type_leaf=TypeLeaf(typ);
    return GenQuad(0,0x71,index,type_leaf);
}

/* Named conversion actions from cmath2.c. The old 32-bit compiler shares
 * action10 for same-width short/pointer conversions; no long-long existed. */
enum old_conversion { NIL, C2S, C2I, C2L, C2U, C2F, C2D, S2C, S2I, S2L, S2U, S2F, S2D, I2C, I2S, I2U, I2L, I2F, I2D, L2C, L2S, L2I, L2U, L2F, L2D, U2C, U2L, U2F, U2D, F2C, F2S, F2I, F2L, F2D, D2C, D2S, D2I, D2L, D2F, P2P=10,A2P=10,P2A=10,CER=255 };
static const unsigned char CnvTable[14][14]={
    /* CHR */ { NIL, C2U, C2S, C2S, C2I, C2U, C2L, C2L, C2F, C2D, P2A, CER, CER, CER },
    /* UCH */ { U2C, NIL, C2S, C2S, C2I, C2U, C2L, C2L, C2F, C2D, P2A, CER, CER, CER },
    /* SHT */ { S2C, S2C, NIL, S2U, S2I, S2U, S2L, S2L, S2F, S2D, P2A, CER, CER, CER },
    /* USH */ { S2C, S2C, S2U, NIL, S2I, S2U, S2L, S2L, S2F, S2D, P2A, CER, CER, CER },
    /* INT */ { I2C, I2C, I2S, I2S, NIL, I2U, I2L, I2L, I2F, I2D, P2A, CER, CER, CER },
    /* UIN */ { I2C, I2C, I2S, I2S, I2U, NIL, U2L, U2L, U2F, U2D, P2A, CER, CER, CER },
    /* LNG */ { L2C, L2C, L2S, L2S, L2I, L2U, NIL, L2U, L2F, L2D, P2A, CER, CER, CER },
    /* ULN */ { L2C, L2C, L2S, L2S, L2I, L2U, L2U, NIL, L2F, L2D, P2A, CER, CER, CER },
    /* FLT */ { F2C, F2C, F2S, F2S, F2I, F2I, F2L, F2L, NIL, F2D, CER, CER, CER, CER },
    /* DBL */ { D2C, D2C, D2S, D2S, D2I, D2I, D2L, D2L, D2F, NIL, CER, CER, CER, CER },
    /* PTR */ { A2P, A2P, A2P, A2P, A2P, A2P, A2P, A2P, CER, CER, P2P, CER, CER, CER },
    /* ARR */ { CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER },
    /* STC */ { CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER },
    /* UNI */ { CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER, CER }
};

unsigned CnvOp(unsigned index,TYPEPTR new_type,int cast_op)
{
    LEAF leaf;
    TYPEPTR old_type;
    unsigned old_size,new_size;
    unsigned char conversion;
    unsigned first_type,second_type;
    while (new_type->decl_type==TYPE_TYPEDEF) new_type=new_type->object;
    LeafGet(&leaf,index);
    if (leaf.leaf_class==0x13) return index;
    if (leaf.leaf_class==0x12 && new_type->decl_type==TYPE_POINTER && CompFlags.initializing_data) {
        leaf.leaf_flags&=~0x20;
        leaf.leaf_type=new_type;
        leaf.leaf_flags|=new_type->decl_flags&0xff1f;
        LeafReplace(&leaf,index);
        return index;
    }
    if (new_type->decl_type==TYPE_VOID && TypeOf(index)->decl_type==TYPE_VOID) return index;
    if (!CompFlags.pre_processing) index=RValue(index);
    LeafGet(&leaf,index);
    old_type=TypeOf(index);
    if (new_type->decl_type>TYPE_POINTER) {
        if (new_type->decl_type==TYPE_VOID) {
            leaf.leaf_flags|=0x80;
            LeafReplace(&leaf,index);
            if (cast_op) CompFlags.meaningless_stmt=0;
            return index;
        } else if (new_type->decl_type==TYPE_ENUM) {
            if (old_type->decl_type==TYPE_POINTER) CWarn1(0x0d,0x71);
            new_type=new_type->object;
        } else {
            if (cast_op) CErr1(0x40d);
            else if (old_type!=new_type) CErr1(0x3f2);
            return index;
        }
    } else if (old_type->decl_type==TYPE_VOID) return index;
    conversion=CnvTable[DataTypeOf(old_type->decl_type)][DataTypeOf(new_type->decl_type)];
    if (conversion==CER) { CErr1(0x442); return index; }
    if (conversion!=NIL) {
        if (old_type==new_type) return index;
        if (conversion==P2P) {
            if (old_type->decl_type==TYPE_ENUM) CWarn1(0x0d,0x71);
            if ((old_type->decl_flags&0x201c)!=(new_type->decl_flags&0x201c)) {
                if (!cast_op) {
                    old_size=TypeSize(old_type);
                    new_size=TypeSize(new_type);
                    if (old_size>new_size &&
                        (old_type->u.p.segment!=10 || (TargetSwitches&0x200)))
                        CWarn1(0x0c,0x70);
                    cast_op=1;
                }
            } else if (IsFunctionPointer(old_type) || IsFunctionPointer(new_type) ||
                       TypeSize(old_type)!=TypeSize(new_type) ||
                       old_type->decl_type!=TYPE_POINTER || new_type->decl_type!=TYPE_POINTER) {
                cast_op=1;
            } else if (cast_op && CompFlags.extensions_enabled &&
                       (leaf.leaf_flags&0x20) && !(leaf.leaf_flags&0x2800)) {
                leaf.attributes=1;
                leaf.leaf_type=new_type;
                LeafReplace(&leaf,index);
                return index;
            }
        }
        if (!cast_op && conversion==P2P) return index;
        if (leaf.leaf_flags&0x40) { CastConstValue(&leaf,index,new_type); return index; }
        if (CompFlags.initializing_data) {
            leaf.u.array_address=0;
            leaf.leaf_type=new_type;
            LeafReplace(&leaf,index);
            return index;
        }
    } else if (old_type->decl_type!=TYPE_FIELD && old_type->decl_type!=TYPE_UFIELD &&
               old_type->decl_type!=TYPE_ENUM) {
        if (leaf.leaf_flags&0x40) { CastConstValue(&leaf,index,new_type); return index; }
        leaf.leaf_flags&=~0x20;
        leaf.leaf_type=new_type;
        leaf.leaf_flags|=new_type->decl_flags&0xff1f;
        LeafReplace(&leaf,index);
        return index;
    }
    first_type=TypeLeaf(new_type);
    second_type=TypeLeaf(new_type);
    return GenQuad(first_type,0x74,index,second_type);
}

/* cmathfun.h order predates the later third mathop byte: the old quad
 * stores this table index itself as the intrinsic selector. */
static struct mathfuncs { char *name; unsigned char parm_count; } MathFuncs[]={
    {"__LOG",1},{"__COS",1},{"__SIN",1},{"__TAN",1},
    {"__SQRT",1},{"__FABS",1},{"__POW",2},{"__ATAN2",2},
    {"__FMOD",2},{"__ACOS",1},{"__ASIN",1},{"__ATAN",1},
    {"__COSH",1},{"__EXP",1},{"__LOG10",1},{"__SINH",1},
    {"__TANH",1},{NULL,0}
};
unsigned GenFuncCall(unsigned last_parm)
{
    unsigned function,result,type_leaf,string_length,first_parm;
    LEAF function_leaf,last_leaf;
    TYPEPTR typ;
    SYM_ENTRY sym;
    char *name;
    unsigned char count,flags;
    int level,i,length,optimized,recursive;
    recursive=0;
    count=1;
    flags=0;
    level=Level;
    while (Token[level]!=6) { ++count; --level; }
    function=ValueStack[level];
    LeafGet(&function_leaf,function);
    typ=TypeOf(function);
    if (typ->decl_type==TYPE_FUNCTION) {
        flags=(unsigned char)typ->decl_flags;
        typ=typ->object;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type==TYPE_DOUBLE || typ->decl_type==TYPE_FLOAT) CompFlags.float_used=1;
        if ((FindInfo(&sym,function_leaf.u.symbol.handle)->flags&1) || (sym.attrib&0x2000)) {
            if (typ->decl_type==TYPE_INT) typ=GetType(TYPE_SHORT);
            else if (typ->decl_type==TYPE_UINT) typ=GetType(TYPE_USHORT);
        }
    } else typ=TypeDefault();
    type_leaf=CallResultLeaf(typ);
    if (ParmsToBeReversed(flags,NULL)) {
        i=level;
        first_parm=last_parm;
        if (i!=Level) first_parm=ValueStack[++i];
        GenQuad(function,0x6c,type_leaf,first_parm);
        while (i!=Level) GenQuad(0,0x6d,0,ValueStack[++i]);
        if (level!=i) GenQuad(0,0x6d,0,last_parm);
        Level=level;
    } else {
        optimized=0;
        LeafGet(&last_leaf,last_parm);
        if (last_leaf.leaf_class==0x10) {
            if (!(GenSwitches&0x80000000U) && !(Toggles&4)) {
                char saved_literal[sizeof(SYM_ENTRY)];
                ReadLiteralChunk(saved_literal,last_leaf.u.string.handle);
                string_length=((STRING_LITERAL *)saved_literal)->length;
                LeafGet(&last_leaf,function);
                if (last_leaf.leaf_class==0) {
                    SymGet(&sym,last_leaf.u.symbol.handle);
                    name=SymName(&sym,last_leaf.u.symbol.handle);
                    if (far_strcmp(name,"_inline_strcmp",15)==0) {
                        SymGet(&sym,SymMEMCMP);
                        function=MakeSymLeaf(&sym,SymMEMCMP);
                        optimized=1;
                    }
                }
            }
        } else {
            LeafGet(&last_leaf,function);
            if (last_leaf.leaf_class==0) {
                SymGet(&sym,last_leaf.u.symbol.handle);
                if (last_leaf.u.symbol.handle==CurFuncHandle) recursive=1;
                name=SymName(&sym,last_leaf.u.symbol.handle);
                length=far_strlen_plus1(name);
                for (i=0;MathFuncs[i].name!=NULL;++i) {
                    if (count==MathFuncs[i].parm_count && IntrinsicMathFunc(name,i,length,&sym)) {
                        if (!(GenSwitches&0x80000000U) && CompFlags.extensions_enabled && !(Toggles&4)) {
                            last_leaf.leaf_class=4;
                            last_leaf.leaf_flags=0;
                            last_leaf.leaf_type=GetType(TYPE_DOUBLE);
                            last_leaf.u.words[1]=i;
                            LeafReplace(&last_leaf,function);
                            first_parm=0;
                            if (count!=1) first_parm=ValueStack[Level--];
                            result=GenQuad(first_parm,0x7f,last_parm,function);
                            MustRecog(7);
                            return result;
                        }
                        function=MakeMathFunction(MathFuncs[i].name,sym.sym_type);
                    }
                }
            }
        }
        if (!optimized) GenQuad(function,0x6c,type_leaf,last_parm);
        else {
            GenQuad(function,0x6c,type_leaf,IntLeaf(string_length));
            GenQuad(0,0x6d,0,last_parm);
        }
        while (Token[Level]!=6) {
            GenQuad(0,0x6d,0,ValueStack[Level]);
            --Level;
        }
    }
    result=GenQuad(0,T_CALL,0,type_leaf);
    if (recursive) MarkRecursiveCall();
    LinkCallQuad();
    if (!DeadCode && function_leaf.leaf_class<=7)
        CompFlags.pending_dead_code|=FunctionAborts(&sym,function_leaf.u.symbol.handle);
    MustRecog(7);
    return result;
}

unsigned CallResultLeaf(TYPEPTR typ)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=8;
    leaf.u.symbol.offset=0;
    leaf.leaf_type=typ;
    if (typ->decl_type!=TYPE_VOID && TypeSize(typ)==0) CErr1(0x459);
    return GenLeaf(&leaf);
}

int IntrinsicMathFunc(char *name,int index,int length,SYMPTR sym)
{
    char function_name[8];
    if (far_strcmp(name,MathFuncs[index].name,length)==0) return 1;
    if (sym->flags&0x4000) {
        strcpy(function_name,MathFuncs[index].name+2);
        strlwr(function_name);
        if (far_strcmp(name,function_name,length)==0) return 1;
    }
    return 0;
}

void LinkCallQuad(void)
{
    QUAD quad;
    if (SizeOfCount!=0) return;
    if (LastCallQuad!=0) {
        QuadGet(&quad,LastCallQuad);
        quad.operand1=(unsigned short)(QuadIndex-1);
        QuadReplace(&quad,LastCallQuad);
    } else FirstCallQuad=QuadIndex-1;
    LastCallQuad=QuadIndex-1;
    StmtInit();
}

void PopNestedParms(TYPEPTR **plist)
{
    struct nested_parm_lists *nested;
    if (NestedParms!=NULL) {
        nested=NestedParms;
        *plist=nested->next_parm_type;
        NestedParms=nested->prev_list;
        CMemFree(nested);
    } else *plist=NULL;
}

unsigned ArrayIndex(unsigned first,unsigned second,TYPEPTR typ)
{
    if (!CheckIndex(first,second,typ)) {
        first=AddrOp(first,0);
        if (Toggles&4) first=GenIndex(first,second,typ);
        else first=PtrOp(AddOp(first,0x22,second));
    }
    return first;
}

/* Constant-index folding of the old symbol/constant leaf representation. */
int CheckIndex(unsigned first,unsigned second,TYPEPTR typ)
{
    LEAF base,index;
    unsigned offset;
    TYPEPTR element;
    LeafGet(&base,first);
    LeafGet(&index,second);
    if (base.leaf_class!=0 && base.leaf_class!=1 &&
        base.leaf_class!=0x0b && base.leaf_class!=0x0d) return 0;
    if (!(index.leaf_flags&0x40)) return 0;
    offset=SizeOfArg(typ->object)*(unsigned)index.u.constant.value.integer;
    if (base.leaf_class==0x0b || base.leaf_class==0x0d)
        base.u.constant.value.integer=(int)((unsigned)base.u.constant.value.integer+offset);
    else {
        offset+=(unsigned)base.u.symbol.offset;
        if (typ->decl_type==TYPE_POINTER) {
            if (base.leaf_class==1) return 0;
            base.leaf_class=3;
        } else base.leaf_class=1;
        base.u.symbol.offset=(int)offset;
    }
    element=typ->object;
    while (element->decl_type==TYPE_TYPEDEF) element=element->object;
    base.leaf_type=element;
    LeafReplace(&base,first);
    if (!(index.leaf_flags&2)) FreeLeaf(second);
    return 1;
}

unsigned IntOp(unsigned first,unsigned char op,unsigned second)
{
    DATA_TYPE first_kind,second_kind,result_kind;
    unsigned type_leaf;
    first=RValue(first);
    second=RValue(second);
    first_kind=DataTypeOf(TypeOf(first)->decl_type);
    second_kind=DataTypeOf(TypeOf(second)->decl_type);
    if (first_kind==TYPE_VOID || second_kind==TYPE_VOID) result_kind=TYPE_VOID;
    else if (first_kind==TYPE_UNION || second_kind==TYPE_UNION) result_kind=255;
    else result_kind=IntResult[first_kind][second_kind];
    if (result_kind==255) { CErr1(0x437); result_kind=TYPE_INT; }
    type_leaf=TypeLeaf(GetType(result_kind));
    return GenQuad(first,op,second,type_leaf);
}

/* cmath2.c shift promotions before the two long-long entries were added. */
static const unsigned char ShiftResult[13]={
    TYPE_INT,TYPE_INT,TYPE_INT,TYPE_INT,TYPE_INT,TYPE_UINT,TYPE_LONG,TYPE_ULONG,
    255,255,255,255,255
};
unsigned ShiftOp(unsigned first,unsigned char op,unsigned second)
{
    DATA_TYPE first_kind,second_kind,result_kind;
    unsigned type_leaf;
    first=RValue(first);
    second=RValue(second);
    first_kind=DataTypeOf(TypeOf(first)->decl_type);
    second_kind=DataTypeOf(TypeOf(second)->decl_type);
    if (first_kind==TYPE_VOID || second_kind==TYPE_VOID) result_kind=TYPE_VOID;
    else {
        result_kind=first_kind<=TYPE_STRUCT ? ShiftResult[first_kind]:255;
        if (result_kind==255 || second_kind>TYPE_ULONG) {
            CErr1(0x437);
            result_kind=TYPE_INT;
        }
    }
    type_leaf=TypeLeaf(GetType(result_kind));
    return GenQuad(first,op,second,type_leaf);
}

/* The quad-based frontend encodes constant true/false and surrounding dead
 * code in the token saved for the later colon/ternary reduction. */
unsigned StartTernary(unsigned index)
{
    LEAF leaf;
    int dead;
    index=BoolExpr(index);
    LeafGet(&leaf,index);
    dead=DeadCode!=0;
    CurToken=dead+4;
    if (leaf.leaf_flags&0x40) {
        CurToken=dead;
        if (leaf.u.constant.value.integer!=0) CurToken=dead+2;
        FreeLeaf(index);
        if (CurToken==dead) ++SizeOfCount;
    }
    return index;
}

unsigned ColonOp(unsigned index)
{
    TYPEPTR typ;
    unsigned before,type_leaf,result_type;
    if (Class[Level]!=TC_TERNARY) {
        CErr1(0x449);
        return 0;
    }
    if (Token[Level]<=1) --SizeOfCount;
    else if (Token[Level]<=3) ++SizeOfCount;
    else if (TypeOf(index)->decl_type!=TYPE_VOID) {
        index=RValue(index);
        before=QuadIndex;
        typ=TypeOf(index);
        result_type=TypeLeaf(typ);
        type_leaf=TypeLeaf(typ);
        GenQuad(type_leaf,0x74,index,result_type);
        if (SizeOfCount==0 && DeadCode==0) {
            Token[Level]=6;
            index=before;
        }
    }
    return index;
}

unsigned TernOp(unsigned condition,unsigned first,unsigned second)
{
    QUAD quad;
    TYPEPTR first_type,second_type,result_type;
    unsigned saved,index;
    saved=0;
    switch (Token[Level]) {
    case 0: DeadCode=0;
    case 1:
        if (CompFlags.pre_processing) { FreeLeaf(first); return second; }
        second=VoidRValue(second);
        ++SizeOfCount;
        first=VoidRValue(first);
        --SizeOfCount;
        break;
    case 2: DeadCode=0;
    case 3:
        --SizeOfCount;
        if (CompFlags.pre_processing) { FreeLeaf(second); return first; }
        first=VoidRValue(first);
        ++SizeOfCount;
        second=VoidRValue(second);
        --SizeOfCount;
        break;
    case 6:
        saved=first;
        QuadGet(&quad,first);
        first=quad.operand2;
    case 4:
        DeadCode=0;
    default:
        first=VoidRValue(first);
        second=VoidRValue(second);
        break;
    }
    first_type=TypeOf(first);
    second_type=TypeOf(second);
    if (first_type->decl_type==TYPE_VOID || second_type->decl_type==TYPE_VOID) {
        if (first_type->decl_type!=second_type->decl_type) CErr1(0x3f2);
        if (Token[Level]<=1) { FreeLeaf(first); return second; }
        if (Token[Level]<=3) { FreeLeaf(second); return first; }
        result_type=first_type;
    } else {
        result_type=TernType(first,second);
        if (Token[Level]<=1) {
            FreeLeaf(first);
            return CnvOp(second,result_type,0);
        }
        if (Token[Level]<=3) {
            FreeLeaf(second);
            return CnvOp(first,result_type,0);
        }
        if (saved!=0) {
            if (first_type!=result_type) {
                first=TypeLeaf(result_type);
                quad.result=(unsigned short)first;
            } else {
                quad.quad_class=0;
                quad.operand1=quad.operand2=quad.result=0;
            }
            QuadReplace(&quad,saved);
        }
        if (second_type!=result_type) second=CnvOp(second,result_type,0);
    }
    index=OptimizeTernary(condition,first,second,result_type);
    if (index==0) {
        GenQuad(condition,3,first,second);
        index=GenQuad(0,4,0,TypeLeaf(result_type));
    }
    return index;
}

unsigned VoidRValue(unsigned index)
{
    return TakeRValue(index,1);
}

TYPEPTR TernType(unsigned first,unsigned second)
{
    LEAF first_leaf,second_leaf;
    TYPEPTR first_type,second_type;
    DATA_TYPE first_kind,second_kind;
    LeafGet(&first_leaf,first);
    LeafGet(&second_leaf,second);
    first_type=first_leaf.leaf_type;
    while (first_type->decl_type==TYPE_TYPEDEF) first_type=first_type->object;
    second_type=second_leaf.leaf_type;
    while (second_type->decl_type==TYPE_TYPEDEF) second_type=second_type->object;
    if (first_type==second_type) return second_type;
    first_kind=DataTypeOf((DATA_TYPE)first_type->decl_type);
    second_kind=DataTypeOf((DATA_TYPE)second_type->decl_type);
    if (first_kind<=TYPE_DOUBLE && second_kind<=TYPE_DOUBLE)
        return GetType((DATA_TYPE)SubResult[first_kind][second_kind]);
    if (first_kind==TYPE_POINTER) {
        if (second_kind==TYPE_POINTER) {
            if (first_type->object->decl_type==TYPE_VOID)
                return MergedType(first_type,second_type);
            if (second_type->object->decl_type==TYPE_VOID)
                return MergedType(second_type,first_type);
            if (CompatibleTernaryPointers(first_type,second_type))
                return MergedType(first_type,second_type);
        } else if ((second_leaf.leaf_flags&0x40) && second_kind<TYPE_FLOAT) {
            if (second_leaf.u.constant.value.integer!=0) CWarn1(1,0x65);
            return first_type;
        }
    } else if (second_kind==TYPE_POINTER && (first_leaf.leaf_flags&0x40) &&
               first_kind<TYPE_FLOAT) {
        if (first_leaf.u.constant.value.integer!=0) CWarn1(1,0x65);
        return second_type;
    }
    CErr1(0x3f2);
    return first_type;
}

/* Recognize the older quad frontend's unsigned min/max selection pattern. */
unsigned OptimizeTernary(unsigned condition,unsigned first,unsigned second,TYPEPTR typ)
{
    QUAD quad;
    LEAF first_leaf,second_leaf,left,right;
    SYM_ENTRY sym;
    SYM_HANDLE handle;
    unsigned function,result,argument;
    int maximum;
    if (first==0 || second==0 || QuadIndex<=2 || (Toggles&4)) return 0;
    QuadGet(&quad,QuadIndex-2);
    if (quad.result!=condition) return 0;
    LeafGet(&first_leaf,first);
    LeafGet(&second_leaf,second);
    if (quad.quad_class==0x1a || quad.quad_class==0x1b) maximum=1;
    else if (quad.quad_class==0x1e || quad.quad_class==0x1f) maximum=0;
    else return 0;
    LeafGet(&left,quad.operand1);
    LeafGet(&right,quad.operand2);
    if (SameTernaryOperand(&first_leaf,&right) && SameTernaryOperand(&second_leaf,&left))
        maximum=!maximum;
    else if (!SameTernaryOperand(&first_leaf,&left) || !SameTernaryOperand(&second_leaf,&right))
        return 0;
    if (typ->decl_type!=TYPE_UCHAR && typ->decl_type!=TYPE_USHORT && typ->decl_type!=TYPE_UINT)
        return 0;
    handle=maximum ? SymMAX:SymMIN;
    SymGet(&sym,handle);
    function=MakeSymLeaf(&sym,handle);
    result=CallResultLeaf(sym.sym_type);
    argument=quad.operand1;
    quad.quad_class=0x6c;
    quad.quad_flags=0;
    quad.result=quad.operand2;
    quad.operand2=(unsigned short)result;
    quad.operand1=(unsigned short)function;
    QuadReplace(&quad,QuadIndex-2);
    quad.quad_class=0x6d;
    quad.operand1=quad.operand2=0;
    quad.result=(unsigned short)argument;
    QuadReplace(&quad,QuadIndex-1);
    return GenQuad(0,0x6e,function,result);
}

void CastConstValue(LEAFPTR leaf,unsigned index,TYPEPTR typ)
{
    int kind;
    kind=ConstantKind(leaf);
    switch (typ->decl_type) {
    case TYPE_CHAR: case TYPE_SHORT: case TYPE_INT: case TYPE_LONG:
        if (kind==3) leaf->u.constant.value.integer=(int)leaf->u.constant.value.floating;
        break;
    case TYPE_UCHAR: case TYPE_USHORT: case TYPE_UINT: case TYPE_ULONG:
        if (kind==3) leaf->u.constant.value.integer=(unsigned)leaf->u.constant.value.floating;
        break;
    case TYPE_FLOAT: case TYPE_DOUBLE:
        ConstantToDouble(leaf,kind);
        break;
    case TYPE_POINTER: break;
    default: return;
    }
    leaf->leaf_type=typ;
    NormalizeConstant(leaf,1);
    leaf->u.words[0]=0;
    LeafReplace(leaf,index);
}

unsigned SizeofOp(TYPEPTR typ)
{
    unsigned size;
    if (typ->decl_type==TYPE_FIELD || typ->decl_type==TYPE_UFIELD) CErr1(0x455);
    size=SizeOfArg(typ);
    if (size==0) CErr1(0x459);
    return UIntLeaf(size);
}

unsigned UIntLeaf(unsigned value)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=0x0b;
    leaf.leaf_flags=0x40;
    leaf.u.constant.value.integer=(int)value;
    leaf.leaf_type=GetType(TYPE_UINT);
    return GenLeaf(&leaf);
}

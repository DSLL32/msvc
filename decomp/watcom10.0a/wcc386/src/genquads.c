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
* Description:  Emit the earlier serialized quad representation through the CG interface.
*
****************************************************************************/


#include "compiler.h"
#include "genquads.h"
#include "scanner.h"
#include "options.h"
#include "files.h"
#include <recon.h>

unsigned GenQuadIndex,InlineQuadIndex,InlinedQuadCount;
SYM_HANDLE InlineFuncHandle,InlineReturnLeaf;
cg_label ReturnLabel,SavedReturnLabel;

/* The later cgen2.c uses tree visitors. This target still walks saved quads.
 * Only operators selected by the dispatch below index this token table;
 * other slots have no semantic use and need no copied token-number bytes. */
static unsigned char CGOperator[]={
/* EOL EOF ,       ? : ; ( ) [ ] { } . ~ = == */
 O_NOP,O_NOP,O_COMMA,0,0,0,0,0,0,0,0,0,0,0,0,O_EQ,
/* ! !=    |    |=   ||        ^     ^=    &     &=    && */
 0,O_NE,O_OR,O_OR,O_FLOW_OR,O_XOR,O_XOR,O_AND,O_AND,O_FLOW_AND,
/* >    >=   >>       >>=      <    <=   <<       <<= */
 O_GT,O_GE,O_RSHIFT,O_RSHIFT,O_LT,O_LE,O_LSHIFT,O_LSHIFT,
/* +      +=     ++     -       -=      -- */
 O_PLUS,O_PLUS,O_PLUS,O_MINUS,O_MINUS,O_MINUS,
/* *       *=      /     /=    %     %= */
 O_TIMES,O_TIMES,O_DIV,O_DIV,O_MOD,O_MOD
};
/* The encountered intrinsic IDs retain their named operation meanings. */
static unsigned char MathOp[]={
 O_LOG,O_COS,O_SIN,O_TAN,O_SQRT,O_FABS,O_POW,O_ATAN2,O_FMOD,
 O_ACOS,O_ASIN,O_ATAN,O_COSH,O_EXP,O_LOG10,O_SINH,O_TANH
};


struct local_vars *GenerateQuads(unsigned source_line,unsigned first,unsigned last,
                               struct local_vars *locals,int inline_call)
{
    QUAD quad;
    LEAF left_leaf,right_leaf,result;
    unsigned index,immediate;
    cg_type type;
    cg_name left,right,third,value;

    index=first;
    while (index<last) {
        GenQuadIndex=index;
        QuadGet(&quad,index);
        if (quad.quad_flags&0x40) { ++index;continue; }
        if (quad.quad_class==T_FOR) {
            if (quad.operand1!=0) {
                LeafGet(&left_leaf,quad.operand1);
                GenJump(&left_leaf);
            }
            index=quad.operand2;
            continue;
        }
        if (quad.quad_class==T_END_FOR) {
            locals=GenerateQuads(quad.src_line,quad.operand2,quad.result,locals,1);
            locals=GenerateQuads(quad.src_line,quad.operand1,quad.operand2,locals,1);
            ++index;
            continue;
        }
        SrcFno=quad.src_file;
        switch (quad.quad_class) {
        case T_UNWIND:
            quad.operand2=0;
            /* fall through */
        case T_FUNC_DEFN:case T_FUNC_END:case T_CALL:case T_CALL_NO_PARM:
        case T_LABEL_DEFN:case T_UNROLL:case T_TRY:
        case 0x0a:case 0x0b:case T_SWITCH:
            immediate=quad.operand1;
            quad.operand1=0;
            break;
        case T_EXCEPT:case T_FINALLY:case T_CASE:
            quad.operand2=0;
            break;
        case 1:
            SrcFno=quad.operand1;
            quad.operand1=0;
            break;
        }
        if (quad.operand1!=0) LeafGetString(&left_leaf,quad.operand1);
        if (quad.operand2!=0) LeafGetString(&right_leaf,quad.operand2);
        if (quad.result!=0) LeafGetString(&result,quad.result);
        SrcLineNum=source_line!=0 ? source_line:quad.src_line;
        /* This old compiler reuses its final line counter while emitting. */
        if (SrcLineNum!=SrcLineCount && InlineFuncHandle==0)
            DBSrcCue(SrcFno,SrcLineNum,1);
        SrcLineCount=SrcLineNum;
        /* The target's dead-state comparisons preceding dispatch have no
         * branch or other effect; control is determined by the actual quads. */
        switch (quad.quad_class) {
        case 0x0a: /* lexical block */
            if (quad.operand2!=0) {
                DBBegBlock();
                EmitLocals(right_leaf.u.symbol.handle);
            }
            break;
        case 0x0b:
            DBEndBlock();
            locals=ReleaseVars(right_leaf.u.symbol.handle,locals);
            break;
        case 3: /* conditional expression, followed by its result quad */
            type=(cg_type)LeafCGType(&result);
            third=LeafValue(&result);
            right=LeafValue(&right_leaf);
            left=LeafValue(&left_leaf);
            value=CGChoose(left,right,third,type);
            if (quad.operand1!=0) ReleaseString(&left_leaf,quad.operand1);
            if (quad.operand2!=0) ReleaseString(&right_leaf,quad.operand2);
            if (quad.result!=0) ReleaseString(&result,quad.result);
            QuadGet(&quad,++index);
            LeafGet(&result,quad.result);
            result.u.cg_name=value;
            break;
        case T_FUNC_DEFN:
            DoFuncDefn(&right_leaf);
            DefineLabels(index+1);
            CGLastParm();
            ReturnLabel=BENewLabel();
            locals=NULL;
            CGControl(O_LABEL,NULL,ReturnLabel);
            break;
        case T_ID:case 0x71:
            result.u.cg_name=LeafAddress(&right_leaf);
            break;
        case 0x6c:
            if (quad.quad_flags&0x10) {
                index=EmitForLoop(&quad,index,NULL,NULL);
                quad.result=0;
                CGControl(O_GOTO,NULL,ReturnLabel);
            } else {
                type=(cg_type)CGenType(right_leaf.leaf_type);
                index=GenCall(&left_leaf,type,index,&quad,&result,inline_call);
            }
            break;
        case T_CALL_NO_PARM:
            if (quad.quad_flags&0x10) {
                if (!(quad.quad_flags&1)) ++index;
                quad.quad_flags=0;
                quad.result=0;
                CGControl(O_GOTO,NULL,ReturnLabel);
            } else {
                type=(cg_type)CGenType(result.leaf_type);
                index=GenCall(&right_leaf,type,index,&quad,&result,inline_call);
            }
            break;
        case 0x7f: /* arithmetic intrinsic selected by a constant leaf */
            type=(cg_type)LeafResultType(&result);
            if (quad.operand1==0) {
                right=LeafValue(&right_leaf);
                result.u.cg_name=CGUnary(MathOp[result.u.constant.value.integer],right,type);
            } else {
                right=LeafValue(&right_leaf);
                left=LeafValue(&left_leaf);
                result.u.cg_name=CGBinary(MathOp[result.u.constant.value.integer],left,right,type);
            }
            break;
        case 0x74:
            type=(cg_type)LeafResultType(&result);
            result.u.cg_name=GenCast(&right_leaf,&result,type);
            break;
        case 0x77:
            quad.quad_flags|=1;
            /* fall through */
        case 0x0e:
            GenAssignment(&left_leaf,&right_leaf,&result);
            break;
        case 0x0d:
            type=(cg_type)LeafResultType(&result);
            right=LeafValue(&right_leaf);
            result.u.cg_name=CGUnary(O_COMPLEMENT,right,type);
            break;
        case 0x10:
            type=(cg_type)CGenType(right_leaf.leaf_type);
            left=CGInteger(0,T_INTEGER);
            right=LeafValue(&right_leaf);
            result.u.cg_name=CGCompare(O_EQ,right,left,type);
            break;
        case 0x25:
            if (quad.operand1!=0) {
                type=(cg_type)LeafResultType(&result);
                right=LeafValue(&right_leaf);
                left=LeafValue(&left_leaf);
                result.u.cg_name=CGBinary(O_MINUS,left,right,type);
                break;
            }
            /* fall through */
        case 0x28:
            if (quad.operand1==0) {
                if (quad.quad_class==0x28) {
                    result.u.cg_name=LeafAddressEx(&right_leaf,&result,0);
                } else {
                    type=(cg_type)LeafResultType(&result);
                    right=LeafValue(&right_leaf);
                    result.u.cg_name=CGUnary(O_UMINUS,right,type);
                }
                break;
            }
            /* fall through */
        case 2:case 0x12:case 0x15:case 0x17:case 0x1c:case 0x20:
        case 0x22:case 0x2a:case 0x2c:
            type=(cg_type)LeafResultType(&result);
            if (result.leaf_flags&0x800) type=T_LONG_POINTER;
            right=LeafValueEx(&right_leaf,&result,0);
            left=LeafValueEx(&left_leaf,&result,0);
            result.u.cg_name=CGBinary(CGOperator[quad.quad_class],left,right,type);
            break;
        case 0x7d:
            type=(cg_type)LeafResultType(&result);
            right=LeafAddress(&right_leaf);
            left=LeafValue(&left_leaf);
            result.u.cg_name=CGBinary(O_COMMA,left,right,type);
            break;
        case 0x13:case 0x16:case 0x18:case 0x1d:case 0x21:
        case 0x23:case 0x26:case 0x29:case 0x2b:case 0x2d:
            type=(cg_type)LeafCGType(&left_leaf);
            right=LeafValue(&right_leaf);
            left=LeafAddress(&left_leaf);
            result.u.cg_name=CGPreGets(CGOperator[quad.quad_class],left,right,type);
            break;
        case 0x24:case 0x27:
            type=(cg_type)LeafCGType(&left_leaf);
            right=LeafValue(&right_leaf);
            left=LeafAddress(&left_leaf);
            result.u.cg_name=CGPostGets(CGOperator[quad.quad_class],left,right,type);
            break;
        case 0x32:
            right=LeafValue(&right_leaf);
            left=LeafValue(&left_leaf);
            result.u.cg_name=GenSegmentOp(left,right);
            break;
        case 0x0f:case 0x11:case 0x1a:case 0x1b:case 0x1e:case 0x1f:
            type=ComparisonType(&left_leaf,&right_leaf);
            right=LeafValue(&right_leaf);
            left=LeafValue(&left_leaf);
            result.u.cg_name=CGCompare(CGOperator[quad.quad_class],left,right,type);
            break;
        case 0x14:case 0x19:
            right=LeafValue(&right_leaf);
            left=LeafValue(&left_leaf);
            result.u.cg_name=CGFlow(CGOperator[quad.quad_class],left,right);
            break;
        case T_SWITCH:
            GenSwitch(immediate,&right_leaf,&result);
            break;
        case T_TRY:
            StartTry(immediate);
            break;
        case T_EXCEPT:case T_FINALLY:
            CGBigLabel(FEBack(left_leaf.u.symbol.handle));
            break;
        case 0x87:
            EndFinally();
            break;
        case T_UNWIND:
            EmitUnwind(immediate);
            break;
        case 0x84:
            result.u.cg_name=CGUnary(O_POINTS,ExceptCode(),T_INTEGER);
            break;
        case 0x85:
            result.u.cg_name=ExceptCode();
            break;
        case T_LABEL_DEFN:
            CGControl(O_LABEL,NULL,left_leaf.u.label.handle);
            if (index==left_leaf.u.label.index) BEFiniLabel(left_leaf.u.label.handle);
            break;
        case T_CASE:
            if (index==left_leaf.u.label.index) {
                CGControl(O_LABEL,NULL,left_leaf.u.label.handle);
                BEFiniLabel(left_leaf.u.label.handle);
            }
            break;
        case 0x69:
            GenJump(&right_leaf);
            if (index==right_leaf.u.label.index) BEFiniLabel(right_leaf.u.label.handle);
            break;
        case 0x6a:case 0x6b:
            left=LeafValue(&left_leaf);
            CGControl(quad.quad_class==0x6a ? O_IF_TRUE:O_IF_FALSE,left,right_leaf.u.label.handle);
            if (index==right_leaf.u.label.index) BEFiniLabel(right_leaf.u.label.handle);
            break;
        case T_UNROLL:
            BEUnrollCount(immediate);
            break;
        case T_FUNC_END:
            BEFiniLabel(ReturnLabel);
            if (CurFunc->flags&0x8000) {
                CallTryFini();
                FreeTrySymBackInfo();
            }
            if (InlineFuncHandle!=0) {
                quad.result=InlineReturnLeaf;
                if (quad.result!=0) {
                    LeafGetString(&result,quad.result);
                    result.leaf_class=0x14;
                    if (quad.operand2!=0) result.u.cg_name=LeafValue(&right_leaf);
                    else result.u.cg_name=CGInteger(0,T_INTEGER);
                }
            } else {
                if (quad.operand2==0) {
                    CGReturn(NULL,T_DEFAULT);
                } else {
                    type=(cg_type)CGenType(right_leaf.leaf_type);
                    if (CompFlags.returns_promoted) type=FEParmType(NULL,NULL,type);
                    value=LeafValue(&right_leaf);
                    CGReturn(value,type);
                }
            }
            if (TargetSwitches&0x200000) {
                BEFiniBack(FunctionProfileBlock);
                BEFreeBack(FunctionProfileBlock);
            }
            FreeLocalVars(CurFunc->u.func.parms);
            FreeLocalVars(CurFunc->u.func.locals);
            if (InlineFuncHandle==0) {
                RelLocalVars(locals);
                FreeCS_Strings();
                locals=NULL;
            } else {
                CurFuncHandle=0;
                InlinedQuadCount+=CurFunc->u.func.number_of_quads;
                index=InlineQuadIndex;
                SymGet(CurFunc,InlineFuncHandle);
                CurFuncHandle=InlineFuncHandle;
                InlineFuncHandle=0;
                ReturnLabel=SavedReturnLabel;
            }
            break;
        }
        if (quad.operand1!=0) ReleaseString(&left_leaf,quad.operand1);
        if (quad.operand2!=0) ReleaseString(&right_leaf,quad.operand2);
        if (quad.result!=0) ReleaseString(&result,quad.result);
        if (quad.quad_flags&2) {
            CGTrash(result.u.cg_name);
        } else if (quad.quad_flags&1) {
            CGDone(result.u.cg_name);
        } else if (quad.result!=0 && quad.result<0xfffb) {
            LeafReplace(&result,quad.result);
        }
        ++index;
    }
    return locals;
}

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


#include <string.h>
#include "compiler.h"
#include "genquads.h"
#include "options.h"
#include "pragma.h"
#include "expression.h"
#include "scanner.h"
#include "memory.h"
#include "cstring.h"
#include <recon.h>

void *SSVar;

/* The earlier serialized-leaf entry corresponds to later DoFuncDefn. */
int DoFuncDefn(LEAFPTR function)
{
    SYM_ENTRY sym;
    SYM_HANDLE handle;
    int reversed;
    cg_type type;
    char *name;
    unsigned length;
    unsigned short old;

    FreeCS_Strings();
    SSVar=NULL;
    CurFunc=&CurFuncSym;
    SymGet(CurFunc,function->u.symbol.handle);
    CurFuncHandle=function->u.symbol.handle;
    if (!CompFlags.zu_switch_used) {
        if ((CurFunc->attrib&0x0c)==0x0c) TargetSwitches|=0x200;
        else TargetSwitches&=~0x200;
    }
    type=(cg_type)CGenType(CurFunc->sym_type->object);
    if (CompFlags.returns_promoted) type=FEParmType(NULL,NULL,type);
    CGProcDecl((void *)(unsigned)function->u.symbol.handle,type);
    if (TargetSwitches&0x200000) {
        name=FEName((void *)(unsigned)function->u.symbol.handle);
        length=strlen(name);
        old=BESetSeg((unsigned short)FunctionProfileSegment);
        FunctionProfileBlock=BENewBack(NULL);
        DGLabel(FunctionProfileBlock);
        DGInteger(0,T_INTEGER);
        DGInteger(0xffffffffU,T_INTEGER);
        DGInteger(0,T_INTEGER);
        DGInteger(0,T_INTEGER);
        DGBytes(length+1,(unsigned char *)name);
        BESetSeg(old);
    }
    if (GenSwitches&0x02000000) DBModSym((void *)(unsigned)CurFuncHandle,T_DEFAULT);
    reversed=0;
    handle=CurFunc->u.func.parms;
    if (handle!=0) {
        GetCallClass(CurFuncHandle);
        if (CallClass&1) {
            ParmReverse(handle);
            reversed=1;
        } else {
            while (handle!=0) {
                SymGet(&sym,handle);
                if (sym.sym_type->decl_type==TYPE_DOT_DOT_DOT) break;
                type=(cg_type)CGenType(sym.sym_type);
                CGParmDecl((void *)(unsigned)handle,type);
                if (((GenSwitches&0x80000000U) ||
                     (!CompFlags.register_conventions && CompFlags.debug_info_some)) &&
                    (GenSwitches&0x02000000))
                    DBLocalSym((void *)(unsigned)handle,T_DEFAULT);
                handle=sym.handle;
            }
        }
    }
    EmitLocals(CurFunc->u.func.locals);
    if (CurFunc->flags&0x8000) {
        CGAutoDecl((void *)(unsigned)TrySymHandle,(cg_type)TryRefno);
        CallTryInit();
    }
    return reversed;
}

/* The old serialized driver has a separate one-symbol helper and reverses
 * local declarations for debug ordering. Later DoAutoDecl folds the loop. */
void EmitLocals(SYM_HANDLE first)
{
    SYMPTR sym;
    if (GenSwitches&0x02000000) { /* DBG_LOCALS */
        if (first!=0) EmitLocalsReverse(first);
    } else {
        while (first!=0) {
            sym=SymGetPtr(first);
            EmitLocal(first);
            first=sym->handle;
        }
    }
}

/* One-symbol predecessor of later cgen2.c DoAutoDecl. The old build excludes
 * externs here but does not yet have the later SC_TYPEDEF exclusion. */
void EmitLocal(SYM_HANDLE handle)
{
    SYM_ENTRY sym;
    TYPEPTR type;
    cg_type dtype;
    char emit_debug_info,emit_extra_info;
    SymGet(&sym,handle);
    emit_debug_info=(GenSwitches&0x80000000U)!=0;
    emit_extra_info=0;
    if (sym.stg_class==SC_STATIC) {
        emit_debug_info=0;
        if (!(sym.flags&4) && sym.sym_type->decl_type!=TYPE_VOID) {
            EmitSym(&sym,handle);
            emit_debug_info=1;
            SymGet(&sym,handle);
            sym.flags|=4; /* SYM_EMITTED */
            SymReplace(&sym,handle);
        }
    } else if (sym.stg_class!=SC_EXTERN) {
        if (sym.flags&0x80) emit_extra_info=1; /* SYM_ADDR_TAKEN */
        type=sym.sym_type;
        while (type->decl_type==TYPE_TYPEDEF) type=type->object;
        if (type->decl_type==TYPE_ARRAY || type->decl_type==TYPE_STRUCT ||
            type->decl_type==TYPE_UNION) emit_extra_info=1;
        dtype=(cg_type)CGenType(type);
        if (sym.flags&0x400) { /* SYM_FUNC_RETURN_VAR */
            sym.info.return_var=CGTemp(dtype);
            SymReplace(&sym,handle);
        } else CGAutoDecl((void *)(unsigned)handle,dtype);
    }
    if (!CompFlags.debug_info_some) emit_extra_info=0;
    /* This earlier driver's InlineFuncHandle becomes Saved_CurFunc later. */
    if ((emit_debug_info || emit_extra_info) && InlineFuncHandle==0 &&
        (GenSwitches&0x02000000) && !(sym.flags&0x20))
        DBLocalSym((void *)(unsigned)handle,T_DEFAULT);
}

/* The serialized predecessor combines label creation with GenerateTryBlock.
 * Class/Token/ValueStack are the parser's actual scratch arrays, reused here. */
void DefineLabels(unsigned first)
{
    QUAD quad;
    LEAF label,other;
    unsigned index,reverse,try_index;
    int max_try_index;
    unsigned short old_segment;
    void *except_table,*except_label;
    struct try_table_back_handle *back;
    max_try_index=-1;
    try_index=0;
    for (index=first;index<QuadIndex;++index) {
        QuadGet(&quad,index);
        if (quad.quad_class==T_FUNC_END) break;
        switch (quad.quad_class) {
        case T_TRY:
            try_index=quad.operand2;
            if ((int)try_index>max_try_index) max_try_index=(int)try_index;
            break;
        case T_EXCEPT:case T_FINALLY:
            ValueStack[try_index]=quad.operand1;
            Class[try_index]=(unsigned char)quad.operand2;
            Token[try_index]=quad.quad_class;
            break;
        case T_CASE:case T_LABEL_DEFN:
            LeafGet(&label,quad.operand1);
            label.u.label.handle=BENewLabel();
            LeafReplace(&label,quad.operand1);
            if (quad.quad_class!=T_CASE) break;
            do { QuadGet(&quad,++index); } while (quad.quad_class==T_CASE);
            --index;
            reverse=index;
            for (;;) {
                QuadGet(&quad,reverse);
                if (quad.quad_class!=T_CASE) break;
                LeafGet(&other,quad.operand1);
                --reverse;
                other.u.label.handle=label.u.label.handle;
                other.u.label.index=index;
                LeafReplace(&other,quad.operand1);
            }
            break;
        }
    }
    if (max_try_index!=-1) {
        old_segment=BESetSeg(4); /* SEG_DATA */
        except_table=BENewBack(NULL);
        back=CMemAlloc(sizeof(*back));
        back->try_table_back_handle=except_table;
        back->next=TryTableBackHandles;
        TryTableBackHandles=back;
        DGLabel(except_table);
        for (try_index=0;(int)try_index<=max_try_index;++try_index) {
            DGInteger(Class[try_index],T_UINT_1);
            DGInteger(Token[try_index]==T_EXCEPT ? 0:1,T_UINT_1);
            LeafGet(&label,ValueStack[try_index]);
            except_label=FEBack(label.u.symbol.handle);
            DGBackPtr(except_label,(unsigned short)FESegID((void *)(unsigned)CurFuncHandle),0,T_CODE_PTR);
        }
        BESetSeg(old_segment);
        SetTryTable(except_table);
        BEFiniBack(except_table);
    }
}

/* Earlier serialized-leaf comparison typing. Unsigned int has unsigned-long
 * rank on this 32-bit target; null constants defer to the other operand. */
cg_type ComparisonType(LEAFPTR left,LEAFPTR right)
{
    TYPEPTR left_type,right_type,type;
    int left_kind,right_kind;
    left_type=left->leaf_type;
    while (left_type->decl_type==TYPE_TYPEDEF) left_type=left_type->object;
    right_type=right->leaf_type;
    while (right_type->decl_type==TYPE_TYPEDEF) right_type=right_type->object;
    if (left_type->decl_type==right_type->decl_type) return (cg_type)CGenType(left_type);
    if ((right->leaf_flags&0x40) && right->u.constant.value.integer==0)
        return (cg_type)CGenType(left_type);
    if ((left->leaf_flags&0x40) && left->u.constant.value.integer==0)
        return (cg_type)CGenType(right_type);
    left_kind=left_type->decl_type;
    right_kind=right_type->decl_type;
    if (left_kind==TYPE_UINT) left_kind=TYPE_ULONG;
    if (right_kind==TYPE_UINT) right_kind=TYPE_ULONG;
    type=right_kind>left_kind ? right_type:left_type;
    if (type->decl_type==TYPE_UCHAR || type->decl_type==TYPE_USHORT)
        return T_INTEGER;
    return (cg_type)CGenType(type);
}

cg_name LeafValue(LEAFPTR leaf)
{
    return LeafValueEx(leaf,NULL,0);
}

/* Serialized-leaf value construction before the later tree representation.
 * Flags select lvalue suppression, address context and assignment context;
 * keep pointer-domain conversions as actual separate CG vertices. */
cg_name LeafValueEx(LEAFPTR leaf,LEAFPTR result,unsigned flags)
{
    TYPEPTR type;
    cg_name value;
    char text[32];
    if (!(leaf->leaf_flags&0x2800) && !(flags&6)) result=NULL;
    type=leaf->leaf_type;
    while (type->decl_type==TYPE_TYPEDEF) type=type->object;
    switch (leaf->leaf_class) {
    case 0x0a: /* integer constant */
        value=CGInteger(leaf->u.constant.value.integer,
            type->decl_type==TYPE_INT ? T_INTEGER:T_UNSIGNED);
        goto integer;
    case 0x0b:
        value=CGInteger(leaf->u.constant.value.integer,T_UNSIGNED);
        goto integer;
    case 0x0c:
        value=CGInteger(leaf->u.constant.value.integer,
            type->decl_type==TYPE_LONG ? T_INT_4:T_UINT_4);
        goto integer;
    case 0x0d:
        value=CGInteger(leaf->u.constant.value.integer,T_UINT_4);
integer:
        if (type->decl_type==TYPE_POINTER)
            value=CGUnary(36,value,(cg_type)CGenType(type)); /* O_CONVERT */
        if ((leaf->leaf_flags&0x100) && !(flags&8))
            value=CGUnary(O_POINTS,value,(cg_type)CGenType(type));
        break;
    case 0x0e:case 0x0f:
        ftoa(leaf->u.constant.value.floating,text);
        value=CGFloat(text,(cg_type)CGenType(type));
        break;
    case 0x10: /* materialized string */
        value=CGBackName((void *)leaf->u.string.back_handle,T_UINT_1);
        break;
    case 4:case 0x14:
        value=leaf->u.cg_name;
        if ((flags&4) && result!=NULL) {
            if (type->u.p.based_sym!=0) {
                if (TypeSize(result->leaf_type)==6)
                    value=GenBasedPointer(type->u.p.based_sym,value);
            } else if (result->leaf_flags&0x2000)
                value=GenPointerConversion(value,34); /* O_PTR_TO_FOREIGN */
        }
        break;
    case 8: /* previously constructed address */
        value=leaf->u.cg_name;
        if (type->decl_type!=TYPE_VOID)
            value=CGUnary(O_POINTS,value,(cg_type)CGenType(type));
        if (leaf->leaf_flags&0x2000)
            value=GenPointerConversion(value,33); /* O_PTR_TO_NATIVE */
        break;
    default:
        value=LeafAddressEx(leaf,result,flags|1);
        if ((type->decl_flags&2) || (leaf->leaf_flags&2)) value=CGVolatile(value);
        break;
    }
    return value;
}

void *CGInitCall(void *function,cg_type type,void *symbol);
void CGAddParm(void *call,void *value,cg_type type);
void *CGCall(void *call);
void *GenCallArgument(LEAFPTR value,cg_type *type);
unsigned GenInlineStrcpy(QUAD *quad,LEAFPTR result,void *literal,unsigned index,unsigned length);
extern unsigned Inline_Threshold;
/* Serialized predecessor of cgen2.c's call builder. The old driver consumes
 * argument quads itself, including its single-level inline-function path. */
unsigned GenQuadCall(LEAFPTR function,cg_type type,unsigned index,QUAD *quad,
                     LEAFPTR result,int inline_call)
{
    SYM_ENTRY symbol;
    struct aux_info *info;
    QUAD next_quad;
    LEAF parameter;
    union { SYM_ENTRY symbol;STRING_LITERAL literal; } chunk;
    TYPEPTR pointer_type;
    cg_name address,call,value;
    cg_type parameter_type;
    unsigned start;

    info=FindInfo(&symbol,function->u.symbol.handle);
    if ((info->flags&1) || (symbol.attrib&0x2000)) {
        if (symbol.flags&0x20) address=function->u.cg_name;
        else address=LeafAddress(function);
        if (type==T_INTEGER) {
            type=T_INT_2;
            result->leaf_type=GetType(TYPE_SHORT);
        }
        if (type==T_UNSIGNED) {
            type=T_UINT_2;
            result->leaf_type=GetType(TYPE_USHORT);
        }
        call=CGInitCall(address,type,(void *)(unsigned)function->u.symbol.handle);
        for (;;) {
            QuadGet(quad,index);
            LeafGetString(result,quad->result);
            if (quad->quad_class==T_CALL || quad->quad_class==T_CALL_NO_PARM) break;
            value=GenCallArgument(result,&parameter_type);
            CGAddParm(call,value,parameter_type);
            ReleaseString(result,quad->result);
            ++index;
        }
        result->u.cg_name=CGCall(call);
        if (LeafCGType(result)==T_POINTER) result->leaf_flags|=0x2000;
        return index;
    }

    SymGet(&symbol,function->u.symbol.handle);
    if (symbol.flags&0x20) address=function->u.cg_name;
    else {
        if ((symbol.flags&8) && (symbol.attrib&0x0c)!=0x0c &&
            (symbol.attrib&0x8000) && !VarParm(&symbol) &&
            (function->leaf_flags&0x8000) && InlineFuncHandle==0 &&
            !inline_call && function->u.symbol.handle!=CurFuncHandle &&
            symbol.u.func.number_of_quads<Inline_Threshold) {
            Inline_Threshold-=symbol.u.func.number_of_quads;
            InlineFuncHandle=CurFuncHandle;
            CurFuncHandle=0;
            SymGet(CurFunc,function->u.symbol.handle);
            CurFuncHandle=function->u.symbol.handle;
            /* Actual older inline mode supplies the flag value1 through this
             * shared serialized-parameter entry's fourth transport slot. */
            InlineQuadIndex=EmitForLoop(quad,index,NULL,(void *)1);
            QuadGet(quad,index);
            InlineReturnLeaf=quad->result;
            if (quad->quad_flags&1) InlineReturnLeaf=0;
            EmitLocals(CurFunc->u.func.locals);
            start=CurFunc->u.func.start_of_func;
            DefineLabels(start+1);
            SavedReturnLabel=ReturnLabel;
            ReturnLabel=BENewLabel();
            CGControl(O_LABEL,NULL,ReturnLabel);
            quad->operand2=0;
            quad->result=0;
            quad->quad_flags=0;
            return start;
        }
        if (result->leaf_class==0x10 &&
            (strcmp(symbol.name,"strcpy")==0 ||
             strcmp(symbol.name,"_inline_strcpy")==0)) {
            QuadGet(&next_quad,index+1);
            LeafGet(&parameter,next_quad.result);
            pointer_type=parameter.leaf_type;
            while (pointer_type->decl_type==TYPE_TYPEDEF)
                pointer_type=pointer_type->object;
            if (pointer_type->decl_type==TYPE_POINTER) {
                ReadLiteralChunk(&chunk,result->u.string.handle);
                ReleaseString(result,quad->result);
                value=CGBackName((void *)result->u.string.back_handle,T_UINT_1);
                return GenInlineStrcpy(quad,result,value,index,chunk.literal.length);
            }
        }
        address=LeafAddress(function);
    }
    call=CGInitCall(address,type,(void *)(unsigned)function->u.symbol.handle);
    while (quad->quad_class!=T_CALL && quad->quad_class!=T_CALL_NO_PARM) {
        result->leaf_flags&=~0x2000;
        parameter_type=(cg_type)LeafCGType(result);
        value=LeafValue(result);
        CGAddParm(call,value,parameter_type);
        ReleaseString(result,quad->result);
        QuadGet(quad,++index);
        LeafGetString(result,quad->result);
    }
    result->u.cg_name=CGCall(call);
    result->leaf_class=8;
    return index;
}

struct local_vars *ReleaseVars(SYM_HANDLE first,struct local_vars *previous)
{
    struct local_vars *entry;
    entry=CMemAlloc(sizeof(*entry));
    entry->next=previous;
    entry->sym_list=first;
    return entry;
}

void *CGSelInit(void);
void CGSelCase(void *table,void *label,long value);
void CGSelOther(void *table,void *label);
void CGSelect(void *table,cg_name value);
/* Earlier serialized switch cases use quad.operand2 as the next index;
 * their leaf retains the already-created label and DWORD case value. */
void GenSwitch(unsigned case_index,LEAFPTR value,LEAFPTR default_label)
{
    QUAD quad;
    LEAF entry;
    void *table;
    table=CGSelInit();
    while (case_index!=0) {
        QuadGet(&quad,case_index);
        LeafGet(&entry,quad.operand1);
        CGSelCase(table,entry.u.label.handle,(long)entry.u.words[3]);
        case_index=quad.operand2;
    }
    CGSelOther(table,default_label->u.label.handle);
    CGSelect(table,LeafValue(value));
}

/* Serialized predecessor of the later cgen2.c pointer-conversion cases.
 * Preserve the old based-pointer and segment-value paths as separate calls. */
cg_name GenCast(LEAFPTR source,LEAFPTR result,cg_type cgtype)
{
    TYPEPTR type;
    unsigned flags;
    cg_name value;
    if (result->leaf_class==6 && result->u.symbol.handle!=0)
        return GenBasedPointer(result->u.symbol.handle,LeafValue(source));
    if (result->leaf_class==7 && (source->leaf_flags&0x800))
        return GenBasedPointer(source->leaf_type->u.p.based_sym,LeafValue(source));
    flags=4;
    type=result->leaf_type;
    while (type->decl_type==TYPE_TYPEDEF) type=type->object;
    switch (type->decl_type) {
    case TYPE_LONG:case TYPE_ULONG:
        break;
    case TYPE_POINTER:
        if (source->leaf_flags&0x800) {
            if (TypeSize(type)!=6) result=NULL;
        } else if (type->decl_flags&0x800) {
            return GenBasedPointer(type->u.p.based_sym,LeafValue(source));
        } else if (source->leaf_flags&0x2000) {
            source->leaf_flags&=~0x2000;
            value=LeafValueEx(source,result,flags);
            return GenPointerConversion(value,33); /* O_PTR_TO_NATIVE. */
        }
        break;
    case TYPE_USHORT:
        if (type->decl_flags&0x1000) {
            flags|=2;
            return LeafValueEx(source,result,flags);
        }
        break;
    default:
        if (source->leaf_flags&0x800) result=NULL;
        break;
    }
    return CGUnary(0x24,LeafValueEx(source,result,flags),cgtype);
}

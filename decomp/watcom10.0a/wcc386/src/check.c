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


#include "compiler.h"
#include "expression.h"
#include "declarations.h"
#include "pragma.h"
#include "files.h"
#include <recon.h>

/* ccheck.c compatibility results before distinct signed-pointer diagnostics. */
enum { CMP_NO,CMP_POINTER_LEVEL,CMP_POINTER_MISMATCH,CMP_ZERO_POINTER,
       CMP_OK,CMP_QUALIFIED,CMP_TRUNCATED,CMP_EXPANDED,CMP_ASSIGNMENT };
/* Public data symbol also bounds the CODE-resident table in period MAPs. */
const unsigned char CompTable[20][20]={
    /* CHAR */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* UCHAR */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* SHORT */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* USHORT */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* INT */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* UINT */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* LONG */ { CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* ULONG */ { CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_OK, CMP_NO, CMP_OK },
    /* FLOAT */ { CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_OK, CMP_ASSIGNMENT, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_ASSIGNMENT, CMP_NO, CMP_ASSIGNMENT, CMP_NO, CMP_ASSIGNMENT },
    /* DOUBLE */ { CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_ASSIGNMENT, CMP_NO, CMP_ASSIGNMENT, CMP_NO, CMP_ASSIGNMENT },
    /* POINTER */ { CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_ZERO_POINTER, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* ARRAY */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* STRUCT */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* UNION */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* FUNCTION */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* FIELD */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_OK },
    /* VOID */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO },
    /* ENUM */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO },
    /* TYPEDEF */ { CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO },
    /* UFIELD */ { CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_OK, CMP_ASSIGNMENT, CMP_ASSIGNMENT, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_NO, CMP_OK, CMP_NO, CMP_NO, CMP_NO, CMP_OK }
};

int DoCompatibleType(TYPEPTR first,TYPEPTR second,int assignment)
{
    unsigned char first_flags,second_flags;
    int first_level,result;
    first_level=1;
    for (;;) {
        if (first==NULL || second==NULL) return first==second ? CMP_OK:CMP_NO;
        first_flags=second_flags=0;
        while (first->decl_type==TYPE_TYPEDEF) {
            first_flags|=(unsigned char)first->decl_flags;
            first=first->object;
        }
        if (first->decl_type==TYPE_ENUM) first=first->object;
        first_flags|=(unsigned char)first->decl_flags;
        while (second->decl_type==TYPE_TYPEDEF) {
            second_flags|=(unsigned char)second->decl_flags;
            second=second->object;
        }
        if (second->decl_type==TYPE_ENUM) second=second->object;
        second_flags|=(unsigned char)second->decl_flags;
        if (first==second) return CMP_OK;
        if (first->decl_type==TYPE_POINTER && second->decl_type==TYPE_POINTER) {
            for (;;) {
                if (first->decl_type!=second->decl_type) break;
                if (!first_level && (second_flags&3) &&
                    (first_flags&3)!=(second_flags&3) && assignment)
                    return CMP_QUALIFIED;
                if (first==second) return CMP_OK;
                if (first->decl_type==TYPE_STRUCT || first->decl_type==TYPE_UNION) {
                    result=CompatibleStructs(first->u.tag,second->u.tag);
                    if (result==CMP_OK) return result;
                    return CompFlags.extensions_enabled ? CMP_QUALIFIED:CMP_NO;
                }
                if (first->decl_type==TYPE_FUNCTION) {
                    if ((first_flags&0xe0)!=(second_flags&0xe0) ||
                        CompatibleFunction(first,second)!=0) return CMP_NO;
                    if (first->object==NULL || second->object==NULL) return CMP_OK;
                }
                first=first->object;
                second=second->object;
                if (first==NULL || second==NULL) break;
                first_flags=second_flags=0;
                while (first->decl_type==TYPE_TYPEDEF) {
                    first_flags|=(unsigned char)first->decl_flags;
                    first=first->object;
                }
                if (first->decl_type==TYPE_ENUM) first=first->object;
                first_flags|=(unsigned char)first->decl_flags;
                while (second->decl_type==TYPE_TYPEDEF) {
                    second_flags|=(unsigned char)second->decl_flags;
                    second=second->object;
                }
                if (second->decl_type==TYPE_ENUM) second=second->object;
                second_flags|=(unsigned char)second->decl_flags;
            }
            if (first==second) {
                if ((second_flags&3&~first_flags) && assignment) return CMP_QUALIFIED;
                return CMP_OK;
            }
            if (first==NULL || second==NULL) return CMP_NO;
            if (first->decl_type==TYPE_VOID || second->decl_type==TYPE_VOID) return CMP_OK;
            if (first->decl_type==TYPE_ARRAY) {
                if (IdenticalType(first->object,second)) return CMP_OK;
            } else if (second->decl_type==TYPE_ARRAY) {
                if (IdenticalType(first,second->object)) return CMP_OK;
            }
            if (first->decl_type==TYPE_POINTER || second->decl_type==TYPE_POINTER)
                return CMP_POINTER_LEVEL;
            if (first->decl_type==TYPE_UNION) return InUnion(first,second,0);
            if (second->decl_type==TYPE_UNION) return InUnion(second,first,1);
            if (first->decl_type==TYPE_FUNCTION || second->decl_type==TYPE_FUNCTION)
                return CMP_NO;
            if (TypeSize(first)==1 && TypeSize(second)==1) return CMP_OK;
            return CMP_POINTER_MISMATCH;
        }
        if (first->decl_type==TYPE_DOT_DOT_DOT && second->decl_type==TYPE_DOT_DOT_DOT)
            return CMP_OK;
        if (first->decl_type>=TYPE_DOT_DOT_DOT || second->decl_type>=TYPE_DOT_DOT_DOT)
            return CMP_NO;
        result=CompTable[first->decl_type][second->decl_type];
        if (result!=CMP_OK) return result;
        if (first->decl_type==TYPE_STRUCT || first->decl_type==TYPE_UNION)
            return CompatibleStructs(first->u.tag,second->u.tag);
        first_level=0;
        first=first->object;
        second=second->object;
    }
}

int CompatiblePointers(TYPEPTR first,TYPEPTR second)
{
    int result;
    if (IsFunctionPointer(first) && IsFunctionPointer(second) &&
        TypeSize(first)!=TypeSize(second)) return 0;
    result=DoCompatibleType(first,second,0);
    return result==CMP_ZERO_POINTER || result==CMP_OK || result==CMP_ASSIGNMENT;
}

int IsFunctionPointer(TYPEPTR typ)
{
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type!=TYPE_POINTER) return 0;
    do { typ=typ->object; } while (typ->decl_type==TYPE_TYPEDEF);
    return typ->decl_type==TYPE_FUNCTION;
}

int CompatibleStructs(struct tag_entry *first,struct tag_entry *second)
{
    FIELDPTR first_field,second_field;
    TYPEPTR first_type,second_type;
    if (first==second) return CMP_OK;
    if (first->size!=second->size) return CMP_NO;
    first_field=first->u.field_list;
    second_field=second->u.field_list;
    while (first_field!=NULL && second_field!=NULL) {
        first_type=first_field->field_type;
        while (first_type->decl_type==TYPE_TYPEDEF) first_type=first_type->object;
        second_type=second_field->field_type;
        while (second_type->decl_type==TYPE_TYPEDEF) second_type=second_type->object;
        if (!IdenticalType(first_type,second_type)) {
            if (!((first_type->decl_type==TYPE_STRUCT && second_type->decl_type==TYPE_STRUCT) ||
                  (first_type->decl_type==TYPE_UNION && second_type->decl_type==TYPE_UNION)))
                return CMP_NO;
            if (CompatibleStructs(first_type->u.tag,second_type->u.tag)!=CMP_OK) return CMP_NO;
        }
        first_field=first_field->next_field;
        second_field=second_field->next_field;
    }
    return first_field==NULL && second_field==NULL ? CMP_OK:CMP_NO;
}

/* The earlier frontend retains a linked list in call-quad operand1 and walks
 * its parameter quads backwards after all declarations have been parsed. */
void ChkCallParms(void)
{
    unsigned next,callsite,index,flags,i,j,temporary;
    QUAD quad;
    LEAF leaf;
    SYM_ENTRY sym;
    TYPEPTR typ,empty,*master;
    if (ErrCount!=0) return;
    empty=NULL;
    next=FirstCallQuad;
    while (next!=0) {
        index=callsite=next;
        QuadGet(&quad,next);
        if (!CallInTailPosition(&quad,next)) {
            quad.quad_flags&=~0x10;
            QuadReplace(&quad,next);
        }
        flags=quad.quad_flags;
        next=quad.operand1;
        Level=0;
        if (quad.quad_class==0x6f) LeafGet(&leaf,quad.operand2);
        else {
            do {
                QuadGet(&quad,--index);
                ValueStack[Level++]=quad.result;
            } while (quad.quad_class!=0x6c);
            callsite=index;
            if (flags&0x10) {
                quad.quad_flags|=0x10;
                QuadReplace(&quad,index);
            }
            LeafGet(&leaf,quad.operand1);
            if (ParmsToBeReversed(leaf.leaf_type->decl_flags,NULL)) {
                i=0;j=Level-1;
                while (j>i) {
                    temporary=ValueStack[i];ValueStack[i]=ValueStack[j];ValueStack[j]=temporary;
                    ++i;--j;
                }
            }
        }
        master=NULL;
        if (leaf.leaf_class==0) {
            SymGet(&sym,leaf.u.symbol.handle);
            if ((flags&0x10) && !(sym.flags&0x200)) {
                quad.quad_flags&=~0x10;
                QuadReplace(&quad,callsite);
            }
            if (sym.flags&8) master=&empty;
            typ=sym.sym_type;
        } else typ=leaf.leaf_type;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->u.parms!=NULL) master=typ->u.parms;
        ValueStack[Level]=0;
        CompareParms(master,ValueStack,quad.src_line);
    }
    Level=0;
}

/* Recognize an unused call or assignment into the function return symbol,
 * followed only by block/label markers and jumps to the function end. */
int CallInTailPosition(QUAD *call,unsigned index)
{
    QUAD quad;
    LEAF leaf;
    SYM_ENTRY sym;
    if (!(call->quad_flags&1)) {
        QuadGet(&quad,++index);
        if (quad.quad_class!=0x77) return 0;
        LeafGet(&leaf,quad.operand1);
        if (leaf.leaf_class!=0) return 0;
        SymGet(&sym,leaf.u.symbol.handle);
        if (!(sym.flags&0x400)) return 0;
    }
    for (;;) {
        QuadGet(&quad,++index);
        switch (quad.quad_class) {
        case 0x68: return 1;
        case 0x0b: case 0x70: break;
        case 0x69:
            LeafGet(&leaf,quad.operand2);
            index=leaf.u.words[1];
            QuadGet(&quad,index);
            if (quad.quad_class!=0x70) return 0;
            break;
        default: return 0;
        }
    }
}

void CompareParms(TYPEPTR *master,unsigned *actual,unsigned line)
{
    TYPEPTR typ,expected,provided;
    LEAF leaf;
    unsigned parameter;
    int result;
    if (master==NULL) return;
    typ=*master;
    if (typ!=NULL && typ->decl_type==TYPE_VOID) typ=NULL;
    parameter=1;
    ErrLine=line;
    while (typ!=NULL && *actual!=0) {
        SymLoc=ErrFName;
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        expected=AdjParmType(typ,0);
        LeafGet(&leaf,*actual);
        provided=leaf.leaf_type;
        result=CompatibleType(expected,provided);
        switch (result) {
        case CMP_POINTER_LEVEL:
            CWarn2(0,0x64,parameter);
            break;
        case CMP_POINTER_MISMATCH: case CMP_QUALIFIED:
            CWarn1(13,0x71);
            break;
        case CMP_ZERO_POINTER:
            if (leaf.leaf_flags&0x40) {
                if (leaf.u.constant.value.integer!=0) CWarn1(1,0x65);
            } else if (TypeSize(expected->object)==TypeSize(provided->object))
                CWarn1(13,0x71);
            else {
                /* This older two-argument diagnostic entry carries either a
                 * string pointer or one DWORD integer, according to code. */
                CErr2p(0x42f,(char *)parameter);
            }
            break;
        case CMP_OK: break;
        default: CErr2p(0x42f,(char *)parameter); break;
        }
        typ=*++master;
        ++actual;
        if (typ==NULL) break;
        if (typ->decl_type==TYPE_DOT_DOT_DOT) return;
        ++parameter;
    }
    if (typ!=NULL || *actual!=0) {
        SymLoc=ErrFName;
        if (!CompFlags.register_conventions) CWarn1(3,0x67);
        else CErr1(0x47f);
    }
    SymLoc=NULL;
}

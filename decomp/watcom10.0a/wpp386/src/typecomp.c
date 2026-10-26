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

/* The original 1994 type comparator uses a variable stack of14-byte
 * comparison records; the later source instead creates a TC_DATA carve. */
#include <stddef.h>
#include "compiler.h"
#include "vstk.h"
#include "types.h"
#include "pending.h"
#include <recon.h>
static VSTK_CTL compareStack;

void typeCompareInit(INITFINI *defn)
{
    (void)defn;
    VstkOpen(&compareStack, 14, 32);
}

#pragma pack(1)
typedef struct compare_record {
    TYPE type1,type2;
    unsigned char mask;
    unsigned arg_index;
    unsigned char flavour;
} TC_DATA;
#pragma pack()
typedef char CompareRecordSize[(sizeof(TC_DATA)==14)?1:-1];
void *VstkTop(VSTK_CTL *stack);
void *VstkPop(VSTK_CTL *stack);
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **base,unsigned char mask);
TYPE TypeModFlagsEC(TYPE type,unsigned short *flags);
int TypeBasesEqual(unsigned short flags,void *base1,void *base2)
{ (void)flags; (void)base1; (void)base2; STUB_TRAP(); return 0; }
void *VstkPush(VSTK_CTL *stack);
int TemplateUnboundSame(TYPE type1,TYPE type2)
{ (void)type1; (void)type2; STUB_TRAP(); return 0; }
int PragmasTypeEquivalent(void *pragma1,void *pragma2)
{ (void)pragma1; (void)pragma2; STUB_TRAP(); return 0; }
int typeCompareCurrent(TYPE type1,TYPE type2,unsigned char mask)
{
    unsigned short flag1,flag2;
    arg_list *args1,*args2;
    TC_DATA *top;
    if(!type1 || !type2 || type1->id!=type2->id) return 0;
    flag1=type1->flag;
    flag2=type2->flag;
    switch(type1->id) {
    case TYP_SINT: case TYP_VOID: break;
    case TYP_ERROR: case TYP_CHAR: case TYP_SCHAR: case TYP_UCHAR:
    case TYP_WCHAR: case TYP_SSHORT: case TYP_USHORT: case TYP_UINT:
    case TYP_SLONG: case TYP_ULONG: case TYP_FLOAT: case TYP_DOUBLE:
    case TYP_LONG_DOUBLE: case TYP_POINTER: case TYP_DOT_DOT_DOT:
        if(flag1!=flag2) return 0;
        break;
    case TYP_ENUM: return 0;
    case TYP_CLASS:
        if(flag1!=flag2 || !(flag2&4)) return 0;
        return TemplateUnboundSame(type1,type2);
    case TYP_TYPEDEF:
        if(flag1!=flag2 || type1->u.t.sym!=type2->u.t.sym ||
           type1->u.t.scope!=type2->u.t.scope) return 0;
        break;
    case TYP_BITFIELD:
        if(flag1!=flag2 || type1->u.b.field_start!=type2->u.b.field_start ||
           type1->u.b.field_width!=type2->u.b.field_width) return 0;
        break;
    case TYP_FUNCTION:
        if(mask&4) { flag1&=0xfa02; flag2&=0xfa02; }
        else { flag1&=0xfa06; flag2&=0xfa06; }
        if(flag1!=flag2) return 0;
        if(type1->u.f.pragma!=type2->u.f.pragma &&
           !PragmasTypeEquivalent(type1->u.f.pragma,type2->u.f.pragma)) return 0;
        args1=type1->u.f.args;
        args2=type2->u.f.args;
        if(args1!=args2) {
            if(!args1 || !args2 || args1->num_args!=args2->num_args ||
               args1->qualifier!=args2->qualifier) return 0;
            if(args1->num_args>0) {
                top=(TC_DATA *)VstkPush(&compareStack);
                top->type1=type1;
                top->type2=type2;
                top->arg_index=args1->num_args;
                top->flavour=0;
                top->mask=mask|2;
            }
        }
        break;
    case TYP_ARRAY:
        if(flag1!=flag2 || type1->u.a.array_size!=type2->u.a.array_size) return 0;
        break;
    case TYP_MODIFIER:
        if(flag1!=flag2 || type1->u.m.base!=type2->u.m.base ||
           type1->u.m.pragma!=type2->u.m.pragma) return 0;
        break;
    case TYP_MEMBER_POINTER:
        if(flag1!=flag2) return 0;
        top=(TC_DATA *)VstkPush(&compareStack);
        top->type1=type1;
        top->type2=type2;
        top->arg_index=0;
        top->flavour=1;
        top->mask=mask|2;
        break;
    case TYP_GENERIC:
        if(flag1!=flag2 || type1->u.g.index!=type2->u.g.index) return 0;
        break;
    default: return 0;
    }
    return 1;
}
int TypeCompareExclude(TYPE type1,TYPE type2,unsigned char mask)
{
    unsigned state=0;
    unsigned short flag1,flag2,extra;
    void *base1,*base2;
    TC_DATA *top;
    unsigned arg_index;
    if(type1==type2) return 1;
    for(;;) {
        if(!type1 || !type2) break;
        type1=TypeModExtract(type1,&flag1,&base1,mask|8);
        type2=TypeModExtract(type2,&flag2,&base2,mask|8);
        if(mask&2) {
            if(type1->id==TYP_POINTER && type2->id==TYP_FUNCTION) {
                type2=TypeModFlagsEC(MakePointerTo(type2),&extra);
                flag2|=extra;
            } else if(type2->id==TYP_POINTER && type1->id==TYP_FUNCTION) {
                type1=TypeModFlagsEC(MakePointerTo(type1),&extra);
                flag1|=extra;
            }
        }
        flag1&=~0x1800;
        flag2&=~0x1800;
        if(flag1!=flag2) break;
        if((flag1&0x700) && !TypeBasesEqual(flag1,base1,base2)) break;
        if(type1==type2) state=1;
        else if(!typeCompareCurrent(type1,type2,mask)) break;
        for(;;) {
            if(state==0) {
                type1=type1->of;
                type2=type2->of;
            } else if(state==1) {
                top=(TC_DATA *)VstkTop(&compareStack);
                if(!top) return 1;
                state=0;
                switch(top->flavour) {
                case 0:
                    arg_index=--top->arg_index;
                    type1=top->type1->u.f.args->type_list[arg_index];
                    type2=top->type2->u.f.args->type_list[arg_index];
                    mask=top->mask;
                    if(arg_index==0) VstkPop(&compareStack);
                    break;
                case 1:
                    type1=top->type1->u.mp.host;
                    type2=top->type2->u.mp.host;
                    mask=top->mask;
                    VstkPop(&compareStack);
                    break;
                }
            }
            if(type1==type2) { state=1; continue; }
            break;
        }
    }
    while(VstkPop(&compareStack)) ;
    return 0;
}
int TypesIdentical(TYPE first,TYPE second)
{
    return TypeCompareExclude(first,second,3);
}

int TypesSameExclude(TYPE source,TYPE target,unsigned char mask)
{ return TypeCompareExclude(source,target,mask|2); }

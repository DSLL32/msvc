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
/* Adapted initial6b9cb443 cgtype.c against actual1994 cgSize297c1..298fe.
 * The old target has eight-byte long double and no BOOL/64-bit/zero-array
 * extensions. Every pointer-size helper remains its genuine named boundary. */
#include "types.h"
#include "cmdline.h"
#include "pending.h"
#include <recon.h>
TYPE TypeModFlags(TYPE type,unsigned short *flags);
int TypeDefined(TYPE type);
/* The encountered1994 PTR_NR/PTR_FR/PTR_HG/PTR_FAR16 table. */
static unsigned ptr_size[4]={4,6,6,4};
unsigned char ptr_type(unsigned short flags,unsigned char classification)
{
    if(flags&0x714) classification=0;
    else if(flags&8) classification=1;
    else if(flags&0x20) classification=2;
    else if(TargetSwitches&0x20) classification=0;
    return classification;
}
unsigned char data_ptr_type(unsigned short flags)
{
    unsigned char classification;
    if(IsHugeData()) classification=2;
    else if(IsBigData()) classification=1;
    else classification=0;
    return ptr_type(flags,classification);
}
unsigned char code_ptr_type(unsigned short flags)
{ return ptr_type(flags,IsBigCode()!=0); }
unsigned data_ptr_size(unsigned short flags)
{ return ptr_size[data_ptr_type(flags)]; }
unsigned code_ptr_size(unsigned short flags)
{ return ptr_size[code_ptr_type(flags)]; }
unsigned CgCodePtrSize(void) { return code_ptr_size(0); }
unsigned cgSize(TYPE type,unsigned char ref_as_ptr)
{
    unsigned short modifiers;
    unsigned size;
    type=TypeModFlags(type,&modifiers);
    switch (type->id) {
    case TYP_ERROR: case TYP_UCHAR: case TYP_SCHAR: return 1;
    case TYP_WCHAR: case TYP_USHORT: case TYP_SSHORT: return 2;
    case TYP_SINT: case TYP_UINT: case TYP_ULONG: case TYP_SLONG:
    case TYP_FLOAT: return 4;
    case TYP_DOUBLE: case TYP_LONG_DOUBLE: return 8;
    case TYP_POINTER:
        if (!ref_as_ptr && (type->flag&1)) return cgSize(type->of,0);
        type=TypeModFlags(type->of,&modifiers);
        if (type->id==TYP_FUNCTION) return code_ptr_size(modifiers);
        return data_ptr_size(modifiers);
    case TYP_BITFIELD: return cgSize(type->of,1);
    case TYP_ARRAY: return cgSize(type->of,0)*type->u.a.array_size;
    case TYP_CLASS:
        return TypeDefined(type) ? type->u.c.info->size : 0;
    case TYP_FUNCTION: return code_ptr_size(modifiers);
    case TYP_MEMBER_POINTER:
        size=2*cgSize(GetBasicType(TYP_UINT),0);
        return CgCodePtrSize()+size;
    default: return 0;
    }
}
unsigned CgMemorySize(TYPE type) { return cgSize(type,1); }

TYPE TypePointedAt(TYPE type,unsigned short *flags);
TYPE TypedefModifierRemove(TYPE type);
TYPE TypeConvertFromPcPtr(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
unsigned CgTypeSize(TYPE type) { return cgSize(type,0); }
unsigned cgPtrSize(TYPE type)
{
    unsigned short flags;
    type=TypeModFlags(type,&flags);
    switch(type->id) {
    case TYP_ARRAY: case TYP_POINTER:
        TypeModFlags(type->of,&flags);
        return data_ptr_size(flags);
    case TYP_FUNCTION:
        return code_ptr_size(flags);
    default:
        return 0;
    }
}
unsigned cgTypeTruncSize(TYPE type)
{
    TYPE base=TypedefModifierRemove(type);
    unsigned short flags;
    switch(base->id) {
    case TYP_ARRAY: case TYP_FUNCTION: case TYP_POINTER:
        TypePointedAt(type,&flags);
        if(flags&0x710) type=TypeConvertFromPcPtr(type);
        return cgPtrSize(type);
    default:
        return CgTypeSize(type);
    }
}
unsigned CgTypeTruncation(TYPE target,TYPE source)
{
    TYPE pointed;
    unsigned short flags;
    unsigned target_size,source_size;
    pointed=TypePointedAt(target,&flags);
    if(pointed && (flags&0x710)) return 0;
    pointed=TypePointedAt(source,&flags);
    if(pointed && (flags&0x710)) source=TypeConvertFromPcPtr(source);
    target_size=cgTypeTruncSize(target);
    source_size=cgTypeTruncSize(source);
    return target_size<source_size ? 1 : 0;
}

/* Original four pointer-class entries address the initialized compilation
 * type roles, rather than encoding a host pointer-size assumption. */
static TYPE *ptr_diff_type[]={
    &CompInfoTypes.ptr_diff_near,&CompInfoTypes.ptr_diff_far,
    &CompInfoTypes.ptr_diff_huge,&CompInfoTypes.ptr_diff_far16
};
TYPE TypePointerDiff(TYPE type)
{
    unsigned short flags;
    type=TypeModFlags(type,&flags);
    switch(type->id) {
    case TYP_POINTER: case TYP_ARRAY:
        TypeModFlags(type->of,&flags);
        return *ptr_diff_type[data_ptr_type(flags)];
    case TYP_FUNCTION:
        return *ptr_diff_type[code_ptr_type(flags)];
    default:
        return 0;
    }
}

TYPE CgStripType(TYPE type)
{ return TypedefModifierRemove(type)->of; }

unsigned CgDataPtrSize(void) { return data_ptr_size(0); }

TYPE TypeNeedsCtor(TYPE type);
int TypeIsCharString(TYPE type);
int IsCgTypeAggregate(TYPE type,int string)
{
    struct class_info *info;
    type=TypedefModifierRemove(type);
    switch(type->id) {
    case TYP_ARRAY: return !string || !TypeIsCharString(type);
    case TYP_BITFIELD: return 1;
    case TYP_CLASS:
        info=type->u.c.info;
        return !info->corrupted && info->bases==0 && info->size==info->vsize &&
            info->last_vfn==0 && info->last_vbase==0 && info->has_data && !TypeNeedsCtor(type);
    }
    return 0;
}

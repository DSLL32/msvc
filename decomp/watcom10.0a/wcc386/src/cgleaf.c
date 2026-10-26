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
#include "options.h"
#include "stubs.h"
#include <recon.h>

/* The 1994 leaf representation predates the released expression-tree driver.
 * Symbol, indirect-symbol and computed-address variants retain their own
 * qualifier, based-pointer and foreign-pointer conversion order. */
cg_name LeafAddressEx(LEAFPTR leaf,LEAFPTR result,unsigned flags)
{
    TYPEPTR type,result_type;
    SYM_ENTRY sym;
    LEAF parent;
    SYM_HANDLE handle;
    cg_name value;
    cg_type address_type;
    type=leaf->leaf_type;
    result_type=result!=NULL ? result->leaf_type:NULL;
    while (result_type!=NULL && result_type->decl_type==TYPE_TYPEDEF)
        result_type=result_type->object;
    switch (leaf->leaf_class) {
    case 0:case 1: /* symbol, optionally with a member offset */
        handle=leaf->u.symbol.handle;
        SymGet(&sym,handle);
        address_type=(cg_type)CGenType(sym.sym_type);
        if (sym.flags&0x400) value=CGTempName(sym.info.return_var,address_type);
        else value=CGFEName((void *)(unsigned)handle,address_type);
        address_type=(cg_type)SymbolPointerType(leaf,&sym);
        if (leaf->u.symbol.offset!=0)
            value=CGBinary(O_PLUS,value,CGInteger(leaf->u.symbol.offset,T_UNSIGNED),address_type);
        if ((type->decl_flags&2) || (sym.attrib&2) ||
            (sym.sym_type->decl_flags&2) || (sym.flags&0x2000))
            value=CGVolatile(value);
        else if (!(sym.flags&0x400)) value=ForceVolatileFloat(value,type);
        if (type->decl_type!=TYPE_POINTER) break;
        if (result_type!=NULL && ((type->decl_flags&0x800) ||
            (result_type->decl_flags&0x800) || (flags&2))) {
            if (sym.sym_type->decl_flags&0x800) {
                value=CGUnary(O_POINTS,value,(cg_type)CGenType(sym.sym_type));
                value=GenBasedPointer(sym.sym_type->u.p.based_sym,value);
                if (flags&2)
                    value=CGUnary(O_POINTS,FarPointerSegmentAddress(value),T_UINT_2);
            } else if (flags&2) {
                value=CGUnary(O_POINTS,value,(cg_type)CGenType(sym.sym_type));
                value=ConvertToBased(value,result,flags);
            } else if (flags&4) {
                if (type->decl_flags&0x800)
                    value=ConvertFromBased(value,(cg_type)CGenType(type));
                else {
                    value=GenBasedPointer(result_type->u.p.based_sym,value);
                    value=CGUnary(O_POINTS,value,(cg_type)CGenType(result_type));
                }
            } else value=CGUnary(O_POINTS,value,(cg_type)CGenType(sym.sym_type));
            if ((flags&4) || result->leaf_class==7 || result->leaf_class==6) flags&=~1U;
            result=NULL;
        } else if ((leaf->leaf_flags&0x2000) && (flags&1)) {
            value=GenPointerConversion(CGUnary(O_POINTS,value,address_type),33);
            flags&=~1U;
            result=NULL;
        } else if (result!=NULL && (result->leaf_flags&0x2000) && (flags&1)) {
            value=GenPointerConversion(CGUnary(O_POINTS,value,address_type),34);
            flags&=~1U;
            result=NULL;
        }
        break;
    case 2:case 3: /* indirection through a symbol */
        handle=leaf->u.symbol.handle;
        SymGet(&sym,handle);
        address_type=(cg_type)CGenType(sym.sym_type);
        value=CGFEName((void *)(unsigned)handle,address_type);
        if ((sym.attrib&2) || (sym.flags&0x2000)) value=CGVolatile(value);
        value=CGUnary(O_POINTS,value,address_type);
        if (sym.sym_type->decl_flags&0x800) {
            value=GenBasedPointer(sym.sym_type->u.p.based_sym,value);
            address_type=T_LONG_POINTER;
        }
        if ((sym.sym_type->decl_flags&0x2000) && type->decl_type!=TYPE_FUNCTION)
            value=GenPointerConversion(value,33);
        if (leaf->u.symbol.offset!=0)
            value=CGBinary(O_PLUS,value,CGInteger(leaf->u.symbol.offset,T_UNSIGNED),address_type);
        if ((type->decl_flags&2) || (leaf->leaf_flags&2)) value=CGVolatile(value);
        else value=ForceVolatileFloat(value,type);
        if (flags&4) {
            if (type->decl_type==TYPE_POINTER && result!=NULL && (type->decl_flags&0x800)) {
                flags&=~1U;
                value=ConvertFromBased(value,(cg_type)CGenType(type));
            }
            result=NULL;
        }
        if (type->decl_type==TYPE_POINTER && (flags&1) &&
            ((leaf->leaf_flags&0x2000) || (type->decl_flags&0x2000))) {
            value=GenPointerConversion(CGUnary(O_POINTS,value,address_type),33);
            flags&=~1U;
            result=NULL;
        }
        break;
    case 6: /* computed address with a based symbol */
        value=leaf->u.cg_name;
        if (leaf->u.symbol.handle!=0) value=GenBasedPointer(leaf->u.symbol.handle,value);
        if (flags&4) {
            if (type->decl_type==TYPE_POINTER && result!=NULL && (type->decl_flags&0x800)) {
                flags&=~1U;
                value=ConvertFromBased(value,(cg_type)CGenType(type));
            }
            result=NULL;
        }
        break;
    case 7: /* computed member address */
        value=leaf->u.cg_name;
        if (leaf->u.symbol.offset!=0) {
            address_type=(cg_type)PtrType(type,leaf->leaf_flags);
            if (leaf->leaf_flags&0x800) address_type=T_LONG_POINTER;
            if ((TargetSwitches&0x14)==4 && address_type==T_POINTER &&
                TypeSize(type)+(unsigned)leaf->u.symbol.offset<0xfff0U)
                address_type=T_LONG_POINTER;
            value=CGBinary(O_PLUS,value,CGInteger(leaf->u.symbol.offset,T_UNSIGNED),address_type);
        }
        if ((type->decl_flags&2) || (leaf->leaf_flags&2)) value=CGVolatile(value);
        if (flags&4) {
            if (type->decl_type==TYPE_POINTER && result!=NULL && (type->decl_flags&0x800)) {
                flags&=~1U;
                value=ConvertFromBased(value,(cg_type)CGenType(type));
            }
            result=NULL;
        }
        break;
    case 5: /* member of an intermediate leaf */
        LeafGet(&parent,leaf->u.symbol.handle);
        address_type=(cg_type)PtrType(type,leaf->leaf_flags);
        value=CGBinary(O_PLUS,parent.u.cg_name,
            CGInteger(leaf->u.symbol.offset,T_UNSIGNED),address_type);
        break;
    case 0x14:
        result=NULL;
        /* fall through */
    case 4:case 8:case 9:
        value=leaf->u.cg_name;
        break;
    case 0x0a:case 0x0b:case 0x0c:case 0x0d:case 0x0e:case 0x0f:case 0x10:
        value=LeafValueEx(leaf,NULL,8);
        break;
    default:
        /* Original has no initialized value for an invalid address class. */
        STUB_TRAP();
        return NULL;
    }
    if (type->decl_type==TYPE_FIELD || type->decl_type==TYPE_UFIELD)
        value=CGBitMask(value,type->u.f.field_start,type->u.f.field_width,(cg_type)CGenType(type));
    if (result!=NULL) {
        if (type->decl_type==TYPE_POINTER && ((result->leaf_flags&0x800) || (flags&2)))
            return ConvertToBased(value,result,flags);
        if (leaf!=result && !(flags&4))
            address_type=(leaf->leaf_flags&0x100) ? T_LONG_POINTER:(cg_type)LeafResultType(result);
        else address_type=(cg_type)CGenType(type);
        value=CGUnary(O_POINTS,value,address_type);
        value=ForceVolatileFloat(value,type);
    } else if (flags&1) {
        address_type=(flags&2) ? T_UINT_2:(cg_type)CGenType(type);
        value=CGUnary(O_POINTS,value,address_type);
        value=ForceVolatileFloat(value,type);
    }
    return value;
}

/* cgen2.c's optional forced spills for source floating-point precision. */
cg_name ForceVolatileFloat(cg_name value,TYPEPTR type)
{
    if (CompFlags.op_switch_used &&
        (type->decl_type==TYPE_FLOAT || type->decl_type==TYPE_DOUBLE))
        value=CGVolatile(value);
    return value;
}

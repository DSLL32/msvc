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


/* Initial rtfun.c/rtfunnam.h names, adapted to the55 old1994 codes.
 * Later releases reorder/add codes and use a wider RTS_TYPE. */
#include "compiler.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
/* These are the encountered runtime-name table, not a data-section image. */
#define RUNTIME_NAMES(F,D,C) \
 F(MOD_INIT_OPEN,mod_init_open) F(BLOCK_POP,block_pop) \
 F(BLOCK_OPEN,block_open) F(BLOCK_CLOSE,block_close) \
 F(BLOCK_PART,block_part) F(BLOCK_PART_CURR,block_part_curr) \
 F(COPY_ARR,copy_array) F(COPY_VARR,copy_varray) \
 F(CTOR_ARR,ctor_array) F(CTOR_VARR,ctor_varray) \
 F(NEW_ARR,new_array) F(NEW_VARR,new_varray) F(NEW_DARR,new_darray) \
 F(DTOR_ARR,dtor_array) F(DTOR_DEL_ARR,dtor_del_array) \
 F(PURE_VIRT,pure_error) D(MOD_DTOR,module_dtor_ref) \
 F(TRY_INIT,try_init) F(TRY_DONE,try_done) C(SETJMP,_setjmp) \
 F(PROLOGUE_EXCEPTION,prologue_exception) F(BLOCK_IGNORE,block_ignore) \
 F(THROW,throw) F(RETHROW,rethrow) F(CATCH_END,catch_done) \
 F(THROW_ZERO,throw_zero) F(CTOR_OBJ,ctor_obj) F(CTOR_OBJ_VIRT,ctor_obj_virt) \
 F(DTOR_OBJ,dtor_obj) F(ASSIGN_ARR,assign_array) F(UNDEF_CDTOR,undefed_cdtor) \
 F(DTOR_DEL_ARR_X,dtor_del_array_x) F(UNDEF_VFUN,undef_vfun) \
 F(STATIC_INIT,static_init) F(LOCAL_INIT,local_init) \
 F(DTOR_AFTER,destruct_after) F(DTOR,destruct) F(DTOR_ALL,destruct_all) \
 F(REG_FUN,fun_register) F(REG_MOD,mod_register) F(REG_OBJ,obj_register) \
 F(REG_CTOR,ctor_register) F(REG_DTOR,dtor_register) \
 F(DEREGISTER,base_deregister) F(DTOR_TABULAR,dtor_tabular) \
 F(REG_LCL,lcl_register) F(UNDEF_MEMBER,undefined_member_function) \
 F(DTOR_AFTER_ALL,destruct_after_all) F(DTOR_AR_STORE,dtor_array_store) \
 F(CTAS_G,ctor_array_storage_g) F(CTAS_1,ctor_array_storage_1) \
 F(CTAS_2,ctor_array_storage_2) D(INLINE_FREG,stab_init_fun) \
 D(LONGJMP_REF,init_longjmp) D(UNDEF_DATA,undefined_member_data)
#define RTF_ENUM(code,name) RTF_##code,
typedef enum { RUNTIME_NAMES(RTF_ENUM,RTF_ENUM,RTF_ENUM) RTF_COUNT } RTF;
#undef RTF_ENUM
#define RTF_FUN(code,name) "__wcpp_2_" #name "_",
#define RTF_DATA(code,name) "___wcpp_2_data_" #name "_",
#define RTF_C(code,name) #name,
static char *runTimeCodeString[]={ RUNTIME_NAMES(RTF_FUN,RTF_DATA,RTF_C) };
#undef RTF_FUN
#undef RTF_DATA
#undef RTF_C
char *RunTimeCodeString(unsigned code)
{
    if(code>=RTF_COUNT) { STUB_TRAP(); return NULL; }
    return runTimeCodeString[code];
}
char *NameCreateNoLen(char *name);
TYPE TypeVoidFunOfVoid(void);
TYPE TypePtrVoidFunOfVoid(void);
TYPE TypePtrToVoid(void);
SYMBOL rtSymbolLookup(char *name)
{
    SEARCH_RESULT *result=ScopeFindNaked(FileScope,name);
    SYMBOL sym=NULL;
    if(result) {
        sym=result->sym_name->name_syms;
        ScopeFreeResult(result);
    }
    return sym;
}
SYMBOL rtSymbolCreate(unsigned char flags,char *name)
{
    TYPE type;
    SYMBOL sym;
    if(flags&0x40) {
        if(flags&0x20) type=TypePtrVoidFunOfVoid();
        else type=TypeVoidFunOfVoid();
    } else if(flags&1) {
        if(flags&0x20) type=TypePtrToVoid();
        else type=GetBasicType(TYP_VOID);
    } else type=GetBasicType(TYP_SINT);
    sym=SymCreateFileScope(type,1,1,name);
    LinkageSet(sym,"C");
    if(flags&0x80) sym->flag|=0x10000000;
    return sym;
}
SYMBOL RunTimeCallSymbol(unsigned code)
{
    char *name=NameCreateNoLen(RunTimeCodeString(code));
    SYMBOL sym=rtSymbolLookup(name);
    unsigned char flags;
    if(!sym) {
        switch(code) {
        case RTF_COPY_ARR: case RTF_COPY_VARR: case RTF_CTOR_ARR:
        case RTF_CTOR_VARR: case RTF_DTOR_ARR: case RTF_ASSIGN_ARR:
        case RTF_DEREGISTER: case RTF_DTOR_AR_STORE: case RTF_CTAS_G:
        case RTF_CTAS_1: case RTF_CTAS_2: flags=0x61; break;
        case RTF_STATIC_INIT: flags=0x42; break;
        case RTF_SETJMP: flags=0x44; break;
        case RTF_MOD_DTOR: case RTF_INLINE_FREG: case RTF_LONGJMP_REF:
        case RTF_UNDEF_DATA: flags=2; break;
        case RTF_THROW: case RTF_RETHROW: case RTF_THROW_ZERO: flags=0xc1; break;
        default: flags=0x41; break;
        }
        sym=rtSymbolCreate(flags,name);
    }
    return sym;
}
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE args);
PTREE RunTimeCall(PTREE args,TYPE type,unsigned code)
{ return NodeMakeCall(RunTimeCallSymbol(code),type,args); }

SCOPE SymScope(SYMBOL sym);
int RunTimeIsThrow(SYMBOL sym)
{ return SymScope(sym)==FileScope && (sym->flag&0x10000000)!=0; }

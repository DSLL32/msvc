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


/* Recovered actual1994 type-signature pool initialization5a6b7..5a6d8.
 * initial6b9cb443 typesig.h supplies the semantic names; its typesig.c
 * no longer contains the implementation. The old28-byte TYPE_SIG lacks
 * the later base link. PCH relocation visits next/type/four symbol slots,
 * and the actual free flag is bit2 at24. typeSigInit is a descriptive
 * recovered name, not a symbol imported from the stripped binary. */
#include "compiler.h"
#include "carve.h"
#include "types.h"
#include "symbol.h"
#include "cgio.h"
#include "pending.h"
#include "typesig.h"
#include <recon.h>
static TYPE_SIG *type_sigs;
static carve_t carveTYPE_SIG;
void typeSigInit(INITFINI *defn)
{
    (void)defn;
    type_sigs = 0;
    carveTYPE_SIG = CarveCreate(sizeof(TYPE_SIG), 16);
}

TYPE TypeCanonicalThr(TYPE type);
TYPE ReferenceType(TYPE type);
unsigned char ThrowCategory(TYPE type);
void *RingLookup(void *head,int (*compare)(void *,const void *),const void *type);
void *RingCarveAlloc(carve_t cv,void **head);
int TypesSameExclude(TYPE first,TYPE second,unsigned char mask);
char *CppNameTypeSig(TYPE type);
SYMBOL ScopeAlreadyExists(SCOPE scope,char *name);
TYPE MakeUCharArray(unsigned count);
TYPE MakeCompilerConstCommonData(TYPE type);
unsigned short CgSegId(SYMBOL sym);
void SegmentMarkUsed(unsigned short id);
unsigned CgCodePtrSize(void);
unsigned CgDataPtrSize(void);
typedef struct tsig_access_control {
    TYPE type;
    CGIO_LOCATION *locn;
    int *error;
    SCOPE saved_scope,class_scope;
    unsigned char access;
} TSIG_ACCESS_CONTROL;
SCOPE TypeScope(TYPE type);
SYMBOL DtorFindLocn(TYPE type,CGIO_LOCATION *locn);
SYMBOL CopyCtorFind(TYPE type,CGIO_LOCATION *locn);
SYMBOL ClassFunMakeAddressable(SYMBOL sym);
SYMBOL SymMarkRefed(SYMBOL sym);
void CDtorScheduleArgRemap(SYMBOL sym);
unsigned ClassDefaultCtorFind(TYPE type,SYMBOL *ctor,CGIO_LOCATION *locn);
void SetErrLoc(CGIO_LOCATION *locn);
SYMBOL tsigDefaultCtor(TYPE type,CGIO_LOCATION *locn,int indirect)
{
    SYMBOL ctor;
    unsigned result=ClassDefaultCtorFind(type,&ctor,locn);
    if(result==2) ctor=NULL;
    else if(result==4 && !indirect) {
        SetErrLoc(locn);
        CErr2p(390,(char *)type);
        ctor=NULL;
    }
    return ctor;
}
void tsigAccessMember(unsigned char mask,SYMBOL *slot,TSIG_ACCESS_CONTROL *ctl)
{
    SYMBOL sym=*slot;
    int indirect;
    if(ctl->access&mask) {
        indirect=(ctl->access&2)!=0;
        CurrScope=indirect?ctl->class_scope:ctl->saved_scope;
    } else {
        indirect=1;
        CurrScope=ctl->class_scope;
    }
    if(CurrScope==ctl->class_scope && sym) return;
    switch(mask) {
    case 16: sym=DtorFindLocn(ctl->type,ctl->locn); break;
    case 8: sym=tsigDefaultCtor(ctl->type,ctl->locn,indirect); break;
    case 4: sym=CopyCtorFind(ctl->type,ctl->locn); break;
    }
    if(!sym) {
        if(!indirect) *ctl->error=1;
        return;
    }
    sym=ClassFunMakeAddressable(sym);
    sym->flag|=8;
    if(!(ctl->access&0x20) && (ctl->access&mask)) sym=SymMarkRefed(sym);
    *slot=sym;
    CDtorScheduleArgRemap(sym);
}
void tsigAccess(unsigned char access,TYPE_SIG *sig,CGIO_LOCATION *locn,int *error)
{
    TSIG_ACCESS_CONTROL ctl;
    if(access&1) return;
    ctl.type=StructType(sig->type);
    if(!ctl.type) return;
    ctl.access=access;
    ctl.locn=locn;
    ctl.error=error;
    ctl.saved_scope=CurrScope;
    ctl.class_scope=TypeScope(ctl.type);
    tsigAccessMember(16,&sig->dtor,&ctl);
    tsigAccessMember(8,&sig->default_ctor,&ctl);
    tsigAccessMember(4,&sig->copy_ctor,&ctl);
    CurrScope=ctl.saved_scope;
}
int tsigMatchesType(void *entry,const void *type)
{ return TypesSameExclude(((TYPE_SIG *)entry)->type,(TYPE)type,1); }
TYPE_SIG *TypeSigFind(unsigned access,TYPE type,CGIO_LOCATION *locn,int *error)
{
    TYPE_SIG *sig;
    SYMBOL sym;
    char *name;
    TYPE storage;
    unsigned size;
    int nested_error=0;
    access=(unsigned char)access;
    type=TypeCanonicalThr(type);
    sig=RingLookup(type_sigs,tsigMatchesType,type);
    if(!sig) {
        sig=RingCarveAlloc(carveTYPE_SIG,(void **)&type_sigs);
        sig->sym=sig->dtor=sig->default_ctor=sig->copy_ctor=NULL;
        sig->type=type;
        sig->cgref=sig->cggen=sig->free=0;
        switch(ThrowCategory(type)) {
        case 0: case 4: size=CgCodePtrSize()+2*CgDataPtrSize()+1; break;
        case 1: case 3: case 6: case 7: size=2; break;
        case 2:
            TypeSigFind(access,ReferenceType(type),locn,&nested_error);
            size=CgDataPtrSize()+1;
            break;
        default: size=0; break;
        }
        sym=NULL;
        if(size) {
            name=CppNameTypeSig(type);
            sym=ScopeAlreadyExists(FileScope,name);
            if(!sym) {
                storage=MakeCompilerConstCommonData(MakeUCharArray(size));
                sym=SymCreateFileScope(storage,10,9,name);
                LinkageSet(sym,"C++");
                CgSegId(sym);
            }
        }
        sig->sym=sym;
    }
    if(nested_error) *error=1;
    else {
        if(sig->sym) SegmentMarkUsed(sig->sym->segid);
        *error=0;
        tsigAccess((unsigned char)access,sig,locn,error);
    }
    return sig;
}

void CgFrontCodePtr(int opcode,void *value);
/* This old code queues the TYPE, not a generated signature SYMBOL. */
void TypeSigReferenced(TYPE_SIG *sig)
{ CgFrontCodePtr(0x24,sig->type); }

/* Older backend getter asks for a signature with no access diagnostics,
 * then marks the real descriptor for backend generation. */
TYPE_SIG *BeTypeSignature(TYPE type)
{
    int error;
    TYPE_SIG *sig=TypeSigFind(1,type,NULL,&error);
    sig->cgref=1;
    return sig;
}

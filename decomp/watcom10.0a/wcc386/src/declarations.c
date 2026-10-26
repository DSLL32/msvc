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
#include "compiler.h"
#include "scanner.h"
#include "files.h"
#include "macros.h"
#include "options.h"
#include "memory.h"
#include "expression.h"
#include "declarations.h"
#include "pragma.h"
#include "stubs.h"
#include <recon.h>

/* The leaf/quad predecessor of cdecl1.c ParsePgm. It emits the function
 * quad and links the intermediate stream directly before parameter parsing. */
void ParsePgm(void)
{
    int n;
    SYM_HANDLE dummy;
    n=0;
    dummy=0;
    GlobalSym=0;
    CompFlags.external_defn_found=0;
    CompFlags.initializing_data=0;
    CompFlags.pcode_was_generated=0;
    do {
        if (DeclList(&dummy)) {
            FuncDefn(CurFunc);
            if (n==0) SymDump();
            SrcLineNum=CurFunc->d.defn_line;
            SrcFno=CurFunc->defn_file_index;
            GenQuad(0,T_FUNC_DEFN,MakeSymLeaf(CurFunc,CurFuncHandle),0);
            ++n;
            LinkFunctionQuads(QuadIndex-1);
            SymLevel=1;
            CompFlags.in_pcode_func=0;
            ParmDeclList();
            SymLevel=0;
            if (CurToken==0x0a) {
                BeginFunc();
                Statement();
                CMemFree(CurFunc->name);
                CurFunc->name=NULL;
                SymReplace(CurFunc,CurFuncHandle);
                CurFunc=NULL;
                CurFuncHandle=0;
            } else MustRecog(0x0a);
        }
    } while (CurToken!=1);
    if (!CompFlags.external_defn_found && !CompFlags.extensions_enabled) CErr1(0x463);
}

/* Older cdecl1.c variant: all definitions receive the default language
 * attribute. The later main/wmain strcmp special case is absent. */
void FuncDefn(SYMPTR sym)
{
    char *sym_name;
    int sym_len;
    TYPEPTR typ;
    sym_name=SymName(sym,CurFuncHandle);
    sym_len=far_strlen_plus1(sym_name);
    sym->name=CMemAlloc(sym_len);
    far_memcpy(sym->name,sym_name,sym_len);
    if (sym->flags&8) CErr2p(0x40a,sym->name);
    typ=sym->sym_type->object;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type!=TYPE_VOID && TypeSize(typ)==0) CErr(0x414,sym_name);
    sym->flags|=8; /* SYM_DEFINED */
    if (!(GenSwitches&0x80000000U)) sym->flags|=0x200; /* SYM_OK_TO_RECURSE */
    if (sym->stg_class==SC_EXTERN || sym->stg_class==0x73) sym->stg_class=SC_NULL;
    CompFlags.external_defn_found=1;
    if (Toggles&1) sym->flags|=0x40; /* SYM_CHECK_STACK */
    if (!CompFlags.zu_switch_used) {
        if ((sym->attrib&0x0c)==0x0c) TargetSwitches|=0x200;
        else TargetSwitches&=~0x200U;
    }
    sym->attrib|=0x8000;
    SymReplace(sym,CurFuncHandle);
}

int VoidType(TYPEPTR typ)
{
    if (typ!=NULL) {
        while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        if (typ->decl_type==TYPE_VOID) return 1;
    }
    return 0;
}

unsigned DefDataSegment;

SYM_HANDLE VarDecl(SYMPTR sym,int stg_class)
{
    TYPEPTR typ;
    SYM_HANDLE sym_handle,old_sym_handle,temp_handle;
    SYM_ENTRY old_sym,sym2;
    unsigned value,left;

    if (SymLevel==0) {
        if (stg_class==0x39 || stg_class==0x62) {
            stg_class=0x5d;
            CErr1(0x423);
        } else if (stg_class==0) CompFlags.external_defn_found=1;
    } else if (stg_class==0) stg_class=0x39;
    if (!(Toggles&2)) sym->flags|=0x100;
    old_sym_handle=SymLook(sym->level,sym->name);
    if (old_sym_handle!=0) {
        SymGet(&old_sym,old_sym_handle);
        if (old_sym.level==SymLevel && old_sym.stg_class==0x61 && stg_class==0x61 &&
            !IdenticalType(old_sym.sym_type,sym->sym_type)) CErr2p(0x469,sym->name);
    }
    if (stg_class==0x61) old_sym_handle=Sym0Look(sym->level,sym->name);
    if (old_sym_handle!=0) {
        SymGet(&old_sym,old_sym_handle);
        if ((old_sym.level==SymLevel || stg_class==0x61) &&
            (sym->attrib&0x79ff)!=(old_sym.attrib&0x79ff)) CErr2p(0x421,sym->name);
    }
    if (old_sym_handle!=0 && (stg_class==0 || stg_class==0x61 ||
        (stg_class==0x5d && SymLevel==0))) {
        if (VerifyType(sym->sym_type,old_sym.sym_type,sym)) {
            old_sym.sym_type=sym->sym_type;
            SymReplace(&old_sym,old_sym_handle);
        }
        if (stg_class==0x61 && SymLevel!=0) goto new_var;
        CMemFree(sym->name);
        memcpy(sym,&old_sym,sizeof(SYM_ENTRY));
        sym_handle=old_sym_handle;
        if (stg_class==0 || stg_class==0x5d) {
            if (sym->stg_class==0x61) sym->stg_class=stg_class;
            else if (sym->stg_class==0x5d && stg_class==0) CErrSymName(0x430,sym,sym_handle);
        }
    } else {
        if (stg_class!=0x61 || SymLevel==0) VfyNewSym(sym->level,sym->name);
new_var:
        old_sym_handle=0;
        sym->flags|=8;
        typ=sym->sym_type;
        if (typ->decl_type==TYPE_TYPEDEF) {
            SymGet(&sym2,typ->u.typedefn);
            if (sym->u.var.segment==0 && sym2.u.var.segment!=0)
                sym->u.var.segment=sym2.u.var.segment;
            while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
        }
        if (typ->decl_type==TYPE_VOID) {
            CErr2p(0x404,sym->name);
            sym->sym_type=TypeDefault();
        }
        sym->stg_class=stg_class;
        sym_handle=SymAdd(sym->level,sym);
    }
    if (sym->u.var.segment==0 && (stg_class==0x5d || stg_class==0 || stg_class==0x61) &&
        DefDataSegment!=0) {
        sym->u.var.segment=DefDataSegment;
        SymReplace(sym,sym_handle);
    }
    if (CurToken==0x0e) {
        sym->flags|=0x40;
        if (stg_class==0x61) {
            stg_class=0x5d;
            if (SymLevel==0) {
                CompFlags.external_defn_found=1;
                stg_class=0;
            } else CErr1(0x456);
        }
        sym->stg_class=stg_class;
        NextToken();
        if (SymLevel==0 || stg_class==0x5d) {
            if (sym->flags&0x10) CErrSymName(0x42c,sym,sym_handle);
            sym->flags|=0x10;
            CompFlags.initializing_data=1;
            StaticInit(sym,sym_handle);
            CompFlags.initializing_data=0;
        } else {
            SymReplace(sym,sym_handle);
            SrcLineNum=TokenLine;
            SrcFno=TokenFno;
            typ=sym->sym_type;
            while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
            if (CurToken!=0x0a && typ->decl_type!=TYPE_ARRAY) {
                value=GetInitExpr();
                left=MakeSymLeaf(sym,sym_handle);
                AsgnOp(left,0x77,value);
            } else if (typ->decl_type==TYPE_ARRAY || typ->decl_type==TYPE_STRUCT ||
                typ->decl_type==TYPE_UNION) {
                temp_handle=MakeNewSym(&sym2,'X',sym->sym_type,0x5d);
                sym2.flags|=0x10;
                CompFlags.initializing_data=1;
                StaticInit(&sym2,temp_handle);
                sym->sym_type=sym2.sym_type;
                CompFlags.initializing_data=0;
                value=MakeSymLeaf(&sym2,temp_handle);
                left=MakeSymLeaf(sym,sym_handle);
                GenQuad(left,0x77,value,0);
                SymReplace(&sym2,temp_handle);
            } else {
                NextToken();
                value=GetInitExpr();
                left=MakeSymLeaf(sym,sym_handle);
                AsgnOp(left,0x77,value);
                MustRecog(0x0b);
            }
        }
    }
    SymReplace(sym,sym_handle);
    if (old_sym_handle!=0) sym_handle=0;
    return sym_handle;
}

TYPEPTR PrevProtoType;

SYM_HANDLE FuncDecl(SYMPTR sym,int stg_class)
{
    SYM_HANDLE sym_handle,old_sym_handle;
    SYM_ENTRY old_sym,sym_typedef;
    struct enum_info ei;
    TYPEPTR old_typ;
    char *name,*sym_name;
    int sym_len;

    PrevProtoType=NULL;
    if (stg_class==0x62 || stg_class==0x39 || stg_class==0x58) {
        stg_class=0;
        CErr1(0x403);
    }
    old_sym_handle=SymLook(sym->level,sym->name);
    if (old_sym_handle!=0) SymGet(&old_sym,old_sym_handle);
    if (old_sym_handle==0) {
        EnumLookup(sym->level,sym->name,&ei);
        if (ei.level>=0) CErr2p(0x40a,sym->name);
        sym_handle=SymAddL0(sym->level,sym);
    } else if (!(old_sym.flags&1)) {
        CErr2p(0x3f7,sym->name);
        sym_handle=SymAddL0(sym->level,sym);
    } else {
        CmpFuncDecls(sym,&old_sym);
        PrevProtoType=old_sym.sym_type;
        if (!(old_sym.flags&8)) {
            if (sym->sym_type->u.parms!=NULL || (CurToken!=2 && CurToken!=5)) {
                old_typ=old_sym.sym_type;
                if (old_typ->decl_type==TYPE_TYPEDEF && old_typ->object->decl_type==TYPE_FUNCTION) {
                    SymGet(&sym_typedef,old_typ->u.typedefn);
                    sym_name=SymName(&sym_typedef,old_typ->u.typedefn);
                    sym_len=far_strlen_plus1(sym_name);
                    name=CMemAlloc(sym_len);
                    far_memcpy(name,sym_name,sym_len);
                    XferPragInfo(name,sym->name);
                    CMemFree(name);
                }
                old_sym.sym_type=sym->sym_type;
                old_sym.d.defn_line=sym->d.defn_line;
                old_sym.defn_file_index=sym->defn_file_index;
            }
        }
        CMemFree(sym->name);
        if (stg_class==0) stg_class=old_sym.stg_class;
        memcpy(sym,&old_sym,sizeof(SYM_ENTRY));
        sym_handle=old_sym_handle;
    }
    sym->flags|=1;
    if (!(sym->flags&8)) {
        if (stg_class==0) stg_class=0x61;
        sym->stg_class=stg_class;
    }
    return sym_handle;
}

void FreeParmList(void)
{
    struct parm_entry *parm;
    while ((parm=ParmList)!=NULL) {
        ParmList=parm->next_parm;
        CMemFree(parm->sym.name);
        CMemFree(parm);
    }
}

int FuncHeadIndex(TYPEPTR *parm_types)
{
    int index;
    index=0;
    if (parm_types!=NULL) {
        while (*parm_types!=NULL) {
            ++parm_types;
            ++index;
            if (index==5) break;
        }
    }
    return index;
}

TYPEPTR FuncNode(TYPEPTR return_typ,unsigned flags,TYPEPTR *parm_types)
{
    TYPEPTR typ;
    int index;

    index=FuncHeadIndex(parm_types);
    for (typ=FuncTypeHead[index];typ;typ=typ->next_type) {
        if (typ->decl_flags==flags && typ->object==return_typ && typ->u.parms==parm_types)
            return typ;
    }
    typ=TypeNode(TYPE_FUNCTION,return_typ);
    typ->decl_flags=flags;
    typ->u.parms=parm_types;
    TypeHead=typ->next_type;
    typ->next_type=FuncTypeHead[index];
    FuncTypeHead[index]=typ;
    return typ;
}

TYPEPTR *MakeParmList(struct parm_list *parm,int parm_count,int reversed)
{
    TYPEPTR *type_list,typ;
    struct parm_list *next_parm,*prev_parm;
    int index;

    type_list=NULL;
    if (parm!=NULL) {
        if (!reversed) {
            prev_parm=NULL;
            for (;;) {
                next_parm=parm->next_parm;
                parm->next_parm=prev_parm;
                if (next_parm==NULL) break;
                prev_parm=parm;
                parm=next_parm;
            }
        }
        parm_count=0;
        for (next_parm=parm;next_parm;next_parm=next_parm->next_parm) ++parm_count;
        index=parm_count;
        if (index>5) index=5;
        for (typ=FuncTypeHead[index];typ;typ=typ->next_type) {
            type_list=typ->u.parms;
            next_parm=parm;
            for (;;) {
                if (next_parm==NULL) {
                    if (*type_list!=NULL) break;
                    while (parm!=NULL) {
                        next_parm=parm->next_parm;
                        CMemFree(parm);
                        parm=next_parm;
                    }
                    return typ->u.parms;
                }
                if (next_parm->parm_type!=*type_list) break;
                next_parm=next_parm->next_parm;
                ++type_list;
            }
        }
        type_list=(TYPEPTR *)CPermAlloc((parm_count+1)*sizeof(TYPEPTR));
        if (type_list!=NULL) {
            type_list[parm_count]=NULL;
            parm_count=0;
            while (parm!=NULL) {
                type_list[parm_count]=parm->parm_type;
                next_parm=parm->next_parm;
                CMemFree(parm);
                parm=next_parm;
                ++parm_count;
            }
        }
    }
    return type_list;
}

void CheckUniqueName(char *name)
{
    struct parm_entry *parm;
    if (name==NULL || *name==0) return;
    for (parm=ParmList;parm;parm=parm->next_parm) {
        if (parm->sym.name!=NULL && strcmp(parm->sym.name,name)==0) {
            CErr2p(0x40a,name);
            parm->sym.flags|=2;
        }
    }
}

struct parm_list *NewParm(TYPEPTR typ,struct parm_list *previous)
{
    struct parm_list *parm;
    parm=(struct parm_list *)CMemAlloc(sizeof(struct parm_list));
    parm->parm_type=typ;
    parm->next_parm=previous;
    return parm;
}

TYPEPTR AdjParmType(TYPEPTR typ,unsigned flags)
{
    TYPEPTR original;
    original=typ;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_FUNCTION) return PtrNode(original,typ->decl_flags,1);
    if (typ->decl_type==TYPE_ARRAY)
        return PtrNode(typ->object,(flags&0x1c)|typ->decl_flags|0x8000,4);
    return original;
}

TYPEPTR *GetProtoType(TYPEPTR typ,int stg_class)
{
    struct parm_entry *parm,*prev_parm;
    struct parm_list *parmlist;
    TYPEPTR explicit_type;
    int parm_count;

    parm_count=0;
    parmlist=NULL;
    for (;;) {
        if (stg_class!=0 && stg_class!=0x62) CErr1(0x457);
        explicit_type=typ;
        if (typ==NULL) {
            CErr1(0x467);
            typ=TypeDefault();
        }
        parm=(struct parm_entry *)CMemAlloc(sizeof(struct parm_entry));
        parm->next_parm=NULL;
        Declarator(&parm->sym,typ,explicit_type);
        if (stg_class==0) {
            stg_class=GetStgClass();
            if (stg_class==0) stg_class=0x39;
        }
        parm->sym.stg_class=stg_class;
        parm->sym.sym_type=AdjParmType(parm->sym.sym_type,parm->sym.attrib);
        parmlist=NewParm(parm->sym.sym_type,parmlist);
        if (parm_count==0) ParmList=parm;
        else {
            CheckUniqueName(parm->sym.name);
            prev_parm->next_parm=parm;
        }
        if (!(Toggles&2)) parm->sym.flags|=2;
        ++parm_count;
        if (CurToken==7 || CurToken==1 || CurToken==0x0a) break;
        MustRecog(2);
        if (CurToken==0x2f) {
            typ=GetType(TYPE_DOT_DOT_DOT);
            parmlist=NewParm(typ,parmlist);
            NextToken();
            break;
        }
        stg_class=GetStgClass();
        prev_parm=parm;
        typ=TypeSpecifier();
    }
    return MakeParmList(parmlist,parm_count,0);
}

TYPEPTR *FuncProtoType(void)
{
    TYPEPTR *type_list,typ;
    struct tag_entry *old_taghead;
    int stg_class;

    if (!CompFlags.extensions_enabled) ++SymLevel;
    old_taghead=TagHead;
    stg_class=GetStgClass();
    typ=TypeSpecifier();
    if (stg_class==0 && typ==NULL) {
        GetFuncParmList();
        if (CurToken==7) NextToken();
        else Expecting(")");
        type_list=NULL;
    } else {
        if (VoidType(typ) && stg_class==0 && CurToken==7) type_list=VoidParmList;
        else type_list=GetProtoType(typ,stg_class);
        if (CurToken==7) NextToken();
        else Expecting(")");
        if (CurToken!=0x0a && (SymLevel>1 || !CompFlags.extensions_enabled)) {
            TagHead=old_taghead;
            FreeEnums();
        }
    }
    if (!CompFlags.extensions_enabled) --SymLevel;
    return type_list;
}

TYPEPTR DeclPart3(TYPEPTR typ,unsigned modifier)
{
    struct parm_entry *parm_list;
    TYPEPTR *parms,typ2;

    NextToken();
    parms=NULL;
    if (CurToken!=7) {
        parm_list=ParmList;
        ParmList=NULL;
        parms=FuncProtoType();
        if (parms==NULL && ParmList!=NULL && (CurToken==5 || CurToken==2))
            CErr1(0x45c);
        if (parm_list!=NULL) {
            FreeParmList();
            ParmList=parm_list;
        }
    } else NextToken();
    if (typ!=NULL) {
        typ2=typ;
        while (typ2->decl_type==TYPE_TYPEDEF) typ2=typ2->object;
        if (typ2->decl_type==TYPE_ARRAY) CErr1(0x445);
        else if (typ2->decl_type==TYPE_FUNCTION) CErr1(0x446);
    }
    return FuncNode(typ,modifier,parms);
}

void ParseDeclPart2(TYPEPTR *typep,TYPEPTR typ,unsigned modifier)
{
    TYPEPTR decl1,decl2;

    decl1=*typep;
    if (decl1!=NULL) {
        while (decl1->object!=NULL) decl1=decl1->object;
        if (decl1->decl_type==TYPE_POINTER) {
            modifier|=decl1->decl_flags&0x20e0;
            if ((decl1->decl_flags&0x0c)==0x0c) modifier|=0x0c;
        }
    }
    decl2=DeclPart2(typ,modifier);
    if (decl1==NULL) *typep=decl2;
    else decl1->object=decl2;
}

TYPEPTR TypeName(void)
{
    TYPEPTR typ;
    typ=TypeSpecifier();
    if (typ!=NULL) typ=AbsDecl(typ);
    return typ;
}

struct parm_entry *ParmList;
struct block_definition *BlockStack;

int DeclList(SYM_HANDLE *sym_head)
{
    SYM_ENTRY sym;
    SYM_HANDLE handle;
    TYPEPTR typ,explicit_type;
    int stg_class;
    ParmList=NULL;
    for (;;) {
        while (CurToken==5) NextToken();
        if (CurToken==1) return 0;
        stg_class=GetStgClass();
        typ=TypeSpecifier();
        if (stg_class==0 && typ==NULL && SymLevel!=0) return 0;
        explicit_type=typ;
        if (typ==NULL) typ=TypeDefault();
        else if (stg_class==0) stg_class=GetStgClass();
        if (stg_class==0 && explicit_type==NULL) {
            switch (CurToken) {
            case 0x33: case 6: case 0x46: case 0x48: case 0x49: case 0x4a:
            case 0x4e: case 0x4f: case 0x42: case 0x36: case 0x37:
            case 0x38: case 0x60:
                break;
            case 0x3a: case 0x3b: case 0x3e: case 0x3f: case 0x40: case 0x41:
            case 0x44: case 0x52: case 0x53: case 0x57: case 0x5b: case 0x5f:
                FlushBadCode();
                continue;
            default:
                CErr2p(0x425,Tokens[CurToken]);
                NextToken();
            }
        }
        if (CurToken!=5) {
            for (;;) {
                handle=InitDeclarator(&sym,stg_class,typ,explicit_type);
                if (handle!=0) {
                    if (sym.flags&SYM_FUNCTION) {
                        if (explicit_type!=NULL) sym.flags|=4;
                    } else if (SymLevel>1) {
                        sym.handle=BlockStack->sym_head;
                        BlockStack->sym_head=handle;
                    } else {
                        sym.handle=*sym_head;
                        *sym_head=handle;
                    }
                    SymReplace(&sym,handle);
                }
                if (CurToken==0x28) continue;
                if (CurToken!=2) break;
                NextToken();
            }
            if (SymLevel==0 && CurToken!=5 && handle!=0 &&
                sym.sym_type->decl_type==TYPE_FUNCTION && sym.sym_type!=typ) {
                CurFuncHandle=handle;
                CurFunc=&CurFuncSym;
                memcpy(CurFunc,&sym,sizeof(SYM_ENTRY));
                return 1;
            }
            MustRecog(5);
        } else {
            Chk_Struct_Union_Enum(typ);
            NextToken();
        }
    }
}

int GetStgClass(void)
{
    int stg_class;
    stg_class=0;
    if ((unsigned)CurToken>=101) STUB_TRAP();
    if (TokenClass[CurToken]==TC_STG_CLASS) {
        stg_class=CurToken;
        NextToken();
    }
    return stg_class;
}

TYPEPTR TypeSpecifier(void)
{
    char plain_int;
    return GetFieldTypeSpecifier(&plain_int);
}

int SymTypedef;

TYPEPTR GetFieldTypeSpecifier(char *plain_int)
{
    unsigned flags,more,qualifiers;
    int type;
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    TYPEPTR typ,underlying;
    *plain_int=0;
    flags=TypeQualifier();
    switch (CurToken) {
    case 0x4c: return StructDecl(TYPE_STRUCT,flags);
    case 0x63: return StructDecl(TYPE_UNION,flags);
    case 0x3c: return EnumDecl(flags);
    case 0x43:
        NextToken();
        flags|=TypeQualifier();
        type=TYPE_VOID;
        break;
    case 0x47:
        NextToken();
        type=TYPE_USHORT;
        flags|=0x1000;
        break;
    case T_ID:
        handle=SymLookTypedef(HashValue,Buffer,&sym);
        if (handle!=0 && sym.stg_class==0x58) {
            ++SymTypedef;
            NextToken();
            typ=sym.sym_type;
            more=TypeQualifier();
            if (flags&more) CErr1(0x424);
            flags|=more;
            if (flags!=0) {
                qualifiers=0;
                underlying=typ;
                for (;;) {
                    qualifiers|=underlying->decl_flags&3;
                    if (underlying->decl_type!=TYPE_TYPEDEF) break;
                    underlying=underlying->object;
                }
                if (flags&qualifiers) CErr1(0x424);
                if (qualifiers!=flags)
                    typ=DupType(typ,(underlying->decl_flags-qualifiers)|flags,0);
            }
            return typ;
        }
        if (flags==0) return NULL;
        type=TYPE_INT;
        break;
    default:
        type=GetScalarType(&flags);
        if (type==TYPE_UFIELD) {
            type=TYPE_INT;
            *plain_int=1;
        }
        if (type==-1) {
            if (flags==0) return NULL;
            type=TYPE_INT;
            *plain_int=1;
        }
    }
    if (type==TYPE_PLAIN_CHAR && flags!=0) {
        if (flags==1) return ConstCharType;
        typ=DupType(BaseTypes[TYPE_PLAIN_CHAR],flags,0);
        typ->object=DupType(typ->object,flags,0);
        return typ;
    }
    typ=GetType(type);
    if (typ->decl_flags!=flags) typ=DupType(typ,flags,0);
    return typ;
}

unsigned TypeQualifier(void)
{
    unsigned flags,bit;
    flags=bit=0;
    for (;;) {
        if (flags&bit) CErr1(0x424);
        flags|=bit;
        if (CurToken==0x5a) {
            NextToken();
            bit=1;
        } else if (CurToken==0x3d) {
            NextToken();
            bit=2;
        } else if (CurToken==T_ID && strcmp(Buffer,"_Packed")==0) {
            NextToken();
            bit=0x4000;
        } else return flags;
    }
}

enum scalar_mask {
    M_CHAR=1,M_INT=2,M_SHORT=4,M_LONG=8,M_SIGNED=0x10,
    M_UNSIGNED=0x20,M_FLOAT=0x40,M_DOUBLE=0x80
};
/* This older Valid_Types is a list of valid mask/type pairs.  It predates
 * the released dense table and long-long entries. */
static struct { unsigned char mask,type; } Valid_Types[]={
    {M_CHAR,TYPE_PLAIN_CHAR},{M_INT,TYPE_UFIELD},
    {M_DOUBLE,TYPE_DOUBLE},{M_FLOAT,TYPE_FLOAT},
    {M_LONG,TYPE_LONG},{M_SHORT,TYPE_SHORT},
    {M_LONG|M_DOUBLE,TYPE_DOUBLE},{M_SHORT|M_INT,TYPE_SHORT},
    {M_LONG|M_INT,TYPE_LONG},{M_SIGNED,TYPE_INT},{M_UNSIGNED,TYPE_UINT},
    {M_SIGNED|M_CHAR,TYPE_CHAR},{M_UNSIGNED|M_CHAR,TYPE_UCHAR},
    {M_SIGNED|M_INT,TYPE_INT},{M_UNSIGNED|M_INT,TYPE_UINT},
    {M_UNSIGNED|M_SHORT,TYPE_USHORT},{M_SIGNED|M_SHORT,TYPE_SHORT},
    {M_SIGNED|M_LONG,TYPE_LONG},{M_UNSIGNED|M_LONG,TYPE_ULONG},
    {M_SIGNED|M_SHORT|M_INT,TYPE_SHORT},
    {M_UNSIGNED|M_SHORT|M_INT,TYPE_USHORT},
    {M_SIGNED|M_LONG|M_INT,TYPE_LONG},
    {M_UNSIGNED|M_LONG|M_INT,TYPE_ULONG},{0,0}
};

int GetScalarType(unsigned *flags)
{
    unsigned mask,bit,qualifier;
    int i;
    mask=0;
    for (;;) {
        qualifier=0;
        switch (CurToken) {
        case 0x64: bit=M_CHAR; break;
        case 0x5e: bit=M_INT; break;
        case 0x4b: bit=M_SHORT; break;
        case 0x5c: bit=M_LONG; break;
        case 0x54: bit=M_SIGNED; break;
        case 0x55: bit=M_UNSIGNED; break;
        case 0x56: bit=M_FLOAT; break;
        case 0x50: bit=M_DOUBLE; break;
        case 0x5a: bit=0x100; qualifier=1; break;
        case 0x3d: bit=0x200; qualifier=2; break;
        default: bit=0;
        }
        if (bit==0) break;
        if (qualifier!=0) {
            if (*flags&qualifier) CErr1(0x424);
            *flags|=qualifier;
        } else if (mask&bit) CErr1(0x424);
        else mask|=bit;
        NextToken();
    }
    if (mask!=0) {
        for (i=0;Valid_Types[i].mask!=0;++i)
            if (Valid_Types[i].mask==mask) return Valid_Types[i].type;
        CErr1(0x424);
    }
    return -1;
}

SYM_HANDLE InitDeclarator(SYMPTR sym,int stg_class,TYPEPTR type,TYPEPTR explicit_type)
{
    SYM_HANDLE old_handle;
    SYM_ENTRY old_sym;
    TYPEPTR old_type,typ;
    if (ParmList!=NULL) FreeParmList();
    memset(sym,0,sizeof(SYM_ENTRY));
    sym->name="";
    Declarator(sym,type,explicit_type);
    if (sym->name[0]==0) {
        InvDecl();
        return 0;
    }
    if (stg_class==0x58) {
        /* Until SymAdd assigns the actual scope, the old level byte carries
         * the identifier's hash bucket. It fits the241-entry hash table. */
        if (CompFlags.extensions_enabled) {
            old_handle=SymLook(sym->level,sym->name);
            if (old_handle!=0) {
                SymGet(&old_sym,old_handle);
                old_type=old_sym.sym_type;
                while (old_type->decl_type==TYPE_TYPEDEF) old_type=old_type->object;
                if (old_sym.stg_class==0x58 && old_sym.level==SymLevel &&
                    old_type==sym->sym_type) {
                    CWarn1(8,0x6c);
                    return 0;
                }
            }
        }
        VfyNewSym(sym->level,sym->name);
        sym->stg_class=stg_class;
        return SymAdd(sym->level,sym);
    }
    typ=sym->sym_type;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_FUNCTION) return FuncDecl(sym,stg_class);
    return VarDecl(sym,stg_class);
}

void Declarator(SYMPTR sym,TYPEPTR typ,TYPEPTR explicit_type)
{
    TYPEPTR parm_type,*type_list;
    struct mod_info info;
    typ=Pointer(typ,&info);
    if (CurToken==6) {
        NextToken();
        parm_type=TypeName();
        if (parm_type!=NULL || CurToken==7) {
            type_list=NULL;
            if (parm_type!=NULL) type_list=MakeParmList(NewParm(parm_type,NULL),1,0);
            typ=FuncNode(typ,info.modifier,type_list);
            typ=PtrNode(typ,0,4);
        } else {
            Declarator(sym,NULL,NULL);
            info.modifier|=sym->attrib;
        }
        MustRecog(7);
        ParseDeclPart2(&sym->sym_type,typ,info.modifier);
        typ=sym->sym_type;
    } else {
        if (CurToken==T_ID || CurToken==0x78) {
            for (;;) {
                if (CurToken==T_ID) {
                    SymCreate(sym,Buffer);
                    sym->level=HashValue;
                    NextToken();
                } else {
                    SymCreate(sym,SavedId);
                    sym->level=SavedHash;
                    CurToken=LAToken;
                }
                if (CurToken!=T_ID && CurToken!=0x28) break;
                if (explicit_type==NULL) {
                    CErr2p(0x3fe,sym->name);
                    if (CurToken==0x28) typ=Pointer(typ,&info);
                } else {
                    if (CurToken==0x28) Expecting(",");
                    else Expecting(",' or ';");
                }
                if (CurToken!=T_ID) break;
                CMemFree(sym->name);
            }
        } else SymCreate(sym,"");
        sym->attrib=info.modifier;
        if (typ!=NULL && typ->decl_type==TYPE_TYPEDEF) sym->attrib|=typ->decl_flags;
        sym->u.var.segment=info.segment;
        typ=DeclPart2(typ,info.modifier);
        sym->sym_type=typ;
    }
    if (typ!=NULL && typ->decl_type==TYPE_FUNCTION && info.segment!=0)
        SetFuncSegment(sym,info.segment);
}

TYPEPTR Pointer(TYPEPTR typ,struct mod_info *info)
{
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    TYPEPTR underlying;
    unsigned flags;
    info->segment=0;
    info->modifier=0;
    handle=0;
    if (typ!=NULL && typ->decl_type==TYPE_TYPEDEF) {
        SymGet(&sym,typ->u.typedefn);
        info->segment=sym.u.var.segment;
        info->modifier=sym.attrib&0x80c;
        if (info->modifier&0x800) handle=SegSymHandle(info->segment);
    }
    for (;;) {
        info->modifier|=GetModifiers();
        if (CurToken==0x4d) {
            NextToken();
            MustRecog(6);
            switch (CurToken) {
            case T_ID:
                handle=SymLook(HashValue,Buffer);
                if (handle==0) {
                    SymCreate(&sym,Buffer);
                    sym.stg_class=SC_EXTERN;
                    CErr2p(0x3f3,Buffer);
                    sym.sym_type=GetType(TYPE_INT);
                    handle=SymAdd(HashValue,&sym);
                } else {
                    SymGet(&sym,handle);
                    underlying=sym.sym_type;
                    while (underlying->decl_type==TYPE_TYPEDEF) underlying=underlying->object;
                    if (!(underlying->decl_flags&0x1000)) CErr1(0x482);
                }
                if (!(sym.flags&SYM_REFERENCED)) {
                    sym.flags|=SYM_REFERENCED;
                    SymReplace(&sym,handle);
                }
                NextToken();
                break;
            case 0x43: case 0x51: /* void / __self */
                NextToken();
                break;
            case 0x45: /* __segname */
                NextToken();
                MustRecog(6);
                if (CurToken==T_STRING) {
                    info->segment=AddSegName(Buffer,"",3);
                    handle=SegSymHandle(info->segment);
                    NextToken();
                } else CErr1(0x480);
                MustRecog(7);
                break;
            case 6:
                NextToken();
                MustRecog(0x47);
                MustRecog(7);
                MustRecog(0x51);
                break;
            default:
                CErr1(0x481);
            }
            MustRecog(7);
            /* Unlike the later OR, the1994 assignment discards earlier flags. */
            info->modifier=0x804;
        }
        if (CurToken!=0x28) return typ;
        NextToken();
        if (CurToken==T_ID && strcmp(Buffer,"_Seg16")==0) {
            info->modifier|=0x2000;
            NextToken();
        }
        flags=info->modifier;
        flags&=~0x400;
        flags|=TypeQualifier();
        typ=BasedPtrNode(typ,flags,4,handle);
        info->segment=0;
        info->modifier=0;
        handle=0;
    }
}

static unsigned ModifierFlags[]={
    0,4,8,0x10,0x0c,0x20,0x40,0x60,0x80,0x400,0x200,0x100
};
unsigned GetModifiers(void)
{
    unsigned flags;
    int cls;
    flags=0;
    for (;;) {
        if (CurToken==T_ID) {
            if (strcmp(Buffer,"__far16")==0) {
                NextToken();
                flags|=0x2000;
                continue;
            }
            if (strcmp(Buffer,"__stdcall")==0) {
                NextToken();
                flags|=0xa0;
                continue;
            }
        }
        if ((unsigned)CurToken>=101) STUB_TRAP();
        cls=TokenClass[CurToken];
        if (cls>=TC_KEYWORD || CurToken==0x4d) return flags;
        /* No independent modifier rows have been reconstructed beyond
         * these language/memory classes; trap in both builds if reached. */
        if ((unsigned)cls>=sizeof(ModifierFlags)/sizeof(ModifierFlags[0])) STUB_TRAP();
        flags|=ModifierFlags[cls];
        if (cls==TC_PASCAL) CompFlags.pragma_state|=4;
        else if (cls==TC_FORTRAN) CompFlags.pragma_state|=0x10;
        else if (cls==TC_CDECL) CompFlags.pragma_state|=1;
        NextToken();
    }
}

TYPEPTR DeclPart2(TYPEPTR typ,unsigned modifier)
{
    if (CurToken==6) typ=DeclPart3(typ,modifier);
    for (;;) {
        if (CurToken==8) typ=ArrayDecl(typ);
        if (CurToken!=6) return typ;
        typ=DeclPart3(typ,modifier);
    }
}

void VfyNewSym(int hash,char *name)
{
    int enum_var;
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    struct enum_info ei;
    enum_var=EnumLookup(hash,name,&ei);
    if (enum_var!=0 && ei.level!=SymLevel) enum_var=0;
    handle=SymLook(hash,name);
    if (handle!=0) {
        SymGet(&sym,handle);
        if (sym.level!=SymLevel) handle=0;
    }
    if (handle!=0 || enum_var!=0) CErr2p(0x40a,name);
}

void MustRecog(unsigned char token)
{
    if (token==CurToken) {
        NextToken();
        return;
    }
    if (token>=101) STUB_TRAP();
    Expecting(Tokens[token]);
    if (CurToken!=1) NextToken();
}

struct tag_entry *TagLookup(void)
{
    struct tag_entry *tag;
    for (tag=TagHead;tag;tag=tag->next_tag)
        if (strcmp(Buffer,tag->name)==0) return tag;
    return NewTag(Buffer);
}

struct tag_entry *NewTag(char *name)
{
    struct tag_entry *tag;
    tag=CPermAlloc(sizeof(struct tag_entry)+strlen(name));
    tag->level=SymLevel;
    tag->next_tag=TagHead;
    TagHead=tag;
    strcpy(tag->name,name);
    ++TagCount;
    return tag;
}

struct tag_entry *VfyNewTag(struct tag_entry *tag,DATA_TYPE kind)
{
    if (tag->sym_type!=NULL) {
        if (tag->level!=SymLevel) return NewTag(tag->name);
        if (tag->size!=0 || tag->sym_type->decl_type!=kind) CErr2p(0x3fb,tag->name);
    }
    return tag;
}

void Chk_Struct_Union_Enum(TYPEPTR typ)
{
    switch (typ->decl_type) {
    case TYPE_STRUCT: case TYPE_UNION:
        if (typ->u.tag->name[0]==0) InvDecl();
        break;
    case TYPE_ENUM:
        break;
    default:
        InvDecl();
    }
}

TYPEPTR StructDecl(DATA_TYPE kind,unsigned flags)
{
    TYPEPTR typ;
    struct tag_entry *tag;
    unsigned saved_packamount;
    saved_packamount=PackAmount;
    if (flags&0x4000) PackAmount=1;
    NextToken();
    if (CurToken==0x0a) {
        tag=NullTag();
        NextToken();
    } else {
        while (CurToken!=T_ID && CurToken!=0x0a && CurToken!=1) {
            ExpectMessage(0x7d9);
            NextToken();
        }
        if (CurToken==T_ID) {
            tag=TagLookup();
            NextToken();
        } else tag=NullTag();
        if (CurToken!=0x0a) {
            flags|=TypeQualifier();
            if (CurToken==5 && tag->level!=SymLevel) tag=NewTag(tag->name);
            typ=tag->sym_type;
            if (typ==NULL) {
                typ=TypeNode(kind,NULL);
                typ->decl_flags=flags;
            } else {
                if (typ->decl_type!=kind) CErr2p(0x3fb,tag->name);
                if (typ->decl_flags!=flags) typ=DupType(typ,flags,0);
            }
            tag->sym_type=typ;
            PackAmount=saved_packamount;
            typ->u.tag=tag;
            return typ;
        }
        NextToken();
        tag=VfyNewTag(tag,kind);
    }
    typ=tag->sym_type;
    if (typ==NULL) {
        typ=TypeNode(kind,NULL);
        typ->decl_flags=flags;
        tag->sym_type=typ;
    }
    typ->u.tag=tag;
    tag->u.field_list=NULL;
    tag->size=GetFields(typ);
    PackAmount=saved_packamount;
    return typ;
}

unsigned long GetFields(TYPEPTR decl)
{
    TYPEPTR typ,explicit_type;
    FIELDPTR field;
    unsigned scalar_size,bits_available,bits_total,worst_alignment;
    unsigned long struct_size,next_offset;
    int width;
    unsigned char unqualified_type,previous_type;
    char plain_int;
    previous_type=TYPE_VOID;
    worst_alignment=1;
    bits_available=1;
    bits_total=0;
    struct_size=next_offset=0;
    for (;;) {
        typ=GetFieldTypeSpecifier(&plain_int);
        explicit_type=typ;
        if (typ==NULL) {
            CErr2p(0x3fe,Buffer);
            typ=TypeDefault();
            if (CurToken==T_ID) NextToken();
        }
        unqualified_type=UnQualifiedType(typ);
        if (bits_available==bits_total || unqualified_type!=previous_type) {
            if (bits_available!=bits_total) next_offset+=bits_total>>3;
            bits_available=TypeSize(typ)*8;
            bits_total=bits_available;
        }
        previous_type=unqualified_type;
        for (;;) {
            field=NULL;
            if (CurToken!=4) {
                field=NewField(FieldDecl(typ,explicit_type),decl);
                next_offset=FieldAlign(next_offset,field,&worst_alignment);
            }
            if (CurToken==4) {
                CheckBitfieldType(typ);
                NextToken();
                width=BoolConstExpr();
                if (width==0 && field!=NULL) CErr1(0x411);
                if (width>32) { width=32; CErr1(0x410); }
                else if (width<0) { width=0; CErr1(0x412); }
                if ((unsigned)width>bits_available || width==0) {
                    scalar_size=TypeSize(typ);
                    if (bits_available!=bits_total) next_offset+=bits_total>>3;
                    else if (width==0) {
                        next_offset+=scalar_size-1;
                        next_offset&=~(scalar_size-1);
                    }
                    bits_available=scalar_size*8;
                    bits_total=bits_available;
                }
                if (field!=NULL) {
                    field->offset=next_offset;
                    field->field_type=EnumFieldType(typ,plain_int,bits_total-bits_available,width);
                }
                bits_available-=width;
            } else {
                if (bits_available!=bits_total) {
                    next_offset+=bits_total>>3;
                    field->offset=next_offset;
                    bits_available=TypeSize(typ)*8;
                    bits_total=bits_available;
                }
                next_offset+=SizeOfArg(field->field_type);
            }
            if (next_offset>struct_size) struct_size=next_offset;
            if (decl->decl_type==TYPE_UNION) next_offset=0;
            if (CurToken!=2) break;
            NextToken();
        }
        if (CurToken==0x0b) CWarn1(0x0e,0x72);
        else MustRecog(5);
        if (CurToken==0x0b) break;
    }
    if (bits_available!=bits_total) {
        next_offset+=bits_total>>3;
        if (next_offset>struct_size) struct_size=next_offset;
    }
    decl->u.tag->alignment=worst_alignment;
    if (worst_alignment>PackAmount) worst_alignment=PackAmount;
    struct_size+=worst_alignment-1;
    struct_size&=-(long)worst_alignment;
    NextToken();
    return struct_size;
}

unsigned char UnQualifiedType(TYPEPTR typ)
{
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_ENUM) typ=typ->object;
    switch (typ->decl_type) {
    case TYPE_SHORT: case TYPE_USHORT: return TYPE_SHORT;
    case TYPE_INT: case TYPE_UINT: return TYPE_INT;
    case TYPE_LONG: case TYPE_ULONG: return TYPE_LONG;
    }
    return 0;
}

FIELDPTR FieldDecl(TYPEPTR typ,TYPEPTR explicit_type)
{
    FIELDPTR field;
    struct mod_info info;
    typ=Pointer(typ,&info);
    if (CurToken==6) {
        NextToken();
        field=FieldDecl(NULL,NULL);
        MustRecog(7);
        ParseDeclPart2(&field->field_type,typ,info.modifier);
    } else {
        if (CurToken==T_ID) {
            for (;;) {
                field=FieldCreate(Buffer);
                NextToken();
                if (CurToken!=T_ID && CurToken!=0x28) break;
                if (explicit_type==NULL) {
                    CErr2p(0x3fe,field->name);
                    if (CurToken==0x28) typ=Pointer(typ,&info);
                } else if (CurToken==0x28) Expecting(",");
                else Expecting(",' or ';");
                if (CurToken!=T_ID) break;
                /* The old recovery loop leaves the previous permanent field. */
            }
        } else field=FieldCreate("");
        field->field_type=DeclPart2(typ,info.modifier);
    }
    return field;
}

FIELDPTR FieldCreate(char *name)
{
    FIELDPTR field;
    field=CPermAlloc(sizeof(FIELD_ENTRY)+strlen(name));
    strcpy(field->name,name);
    return field;
}

FIELDPTR NewField(FIELDPTR new_field,TYPEPTR decl)
{
    FIELDPTR field,prev_field;
    TYPEPTR typ;
    struct tag_entry *tag;
    ++FieldCount;
    typ=new_field->field_type;
    if (typ!=NULL) while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (new_field->name[0]==0 &&
        ((typ->decl_type!=TYPE_STRUCT && typ->decl_type!=TYPE_UNION) ||
         !CompFlags.extensions_enabled)) CErr1(0x402);
    if (typ==decl) CErr1(0x45a);
    else if (SizeOfArg(typ)==0) {
        if ((typ->decl_type==TYPE_ARRAY &&
             (SizeOfArg(typ->object)==0 || !CompFlags.extensions_enabled)) ||
             typ->decl_type!=TYPE_ARRAY) CErr(0x414,new_field->name);
    }
    tag=decl->u.tag;
    for (field=tag->u.field_list;field;field=field->next_field) {
        if (new_field->name[0]!=0 && strcmp(field->name,new_field->name)==0)
            CErr2p(0x46a,field->name);
        prev_field=field;
    }
    if (tag->u.field_list==NULL) tag->u.field_list=new_field;
    else {
        prev_field->next_field=new_field;
        if (SizeOfArg(prev_field->field_type)==0) CErr(0x414,prev_field->name);
    }
    return new_field;
}

unsigned long FieldAlign(unsigned long offset,FIELDPTR field,unsigned *worst)
{
    unsigned align,pack_adjustment;
    pack_adjustment=PackAmount;
    align=GetTypeAlignment(field->field_type);
    if (align>*worst) *worst=align;
    switch (align) {
    case 1: pack_adjustment=1; break;
    case 2: if (pack_adjustment>2) pack_adjustment=2; break;
    case 4: if (pack_adjustment>4) pack_adjustment=4; break;
    case 8: if (pack_adjustment>8) pack_adjustment=8; break;
    }
    if (PackAmount!=1) {
        offset+=pack_adjustment-1;
        offset&=-(long)pack_adjustment;
    }
    field->offset=offset;
    return offset;
}

unsigned GetTypeAlignment(TYPEPTR typ)
{
    while (typ->decl_type==TYPE_TYPEDEF || typ->decl_type==TYPE_ARRAY) typ=typ->object;
    if (typ->decl_type==TYPE_STRUCT || typ->decl_type==TYPE_UNION)
        return typ->u.tag->alignment;
    return TypeSize(typ);
}

void CheckBitfieldType(TYPEPTR typ)
{
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (CompFlags.extensions_enabled && typ->decl_type==TYPE_ENUM) typ=typ->object;
    switch (typ->decl_type) {
    case TYPE_INT: case TYPE_UINT:
        return;
    case TYPE_CHAR: case TYPE_UCHAR: case TYPE_SHORT: case TYPE_USHORT:
    case TYPE_LONG: case TYPE_ULONG:
        if (CompFlags.extensions_enabled) return;
    }
    CErr1(0x413);
}

TYPEPTR EnumFieldType(TYPEPTR field_type,unsigned char plain_int,unsigned start,unsigned width)
{
    TYPEPTR typ;
    unsigned char data_type;
    typ=TypeNode(TYPE_FIELD,NULL);
    typ->u.f.field_start=start;
    typ->u.f.field_width=width;
    if (plain_int) data_type=TYPE_INT;
    else {
        while (field_type->decl_type==TYPE_TYPEDEF) field_type=field_type->object;
        if (field_type->decl_type==TYPE_ENUM) field_type=field_type->object;
        data_type=field_type->decl_type;
    }
    typ->u.f.field_type=data_type;
    switch (data_type) {
    case TYPE_UCHAR: case TYPE_USHORT: case TYPE_UINT: case TYPE_ULONG:
        typ->decl_type=TYPE_UFIELD;
    }
    return typ;
}

struct tag_entry *NullTag(void)
{
    return NewTag("");
}

TYPEPTR ArrayDecl(TYPEPTR typ)
{
    int dimension;
    unsigned flags;
    TYPEPTR first_node,next_node,prev_node;
    flags=0;
    if (typ!=NULL) {
        if (typ->decl_type==TYPE_FUNCTION) CErr1(0x444);
        flags=typ->decl_flags&3;
    }
    first_node=prev_node=NULL;
    while (CurToken==8) {
        NextToken();
        if (CurToken!=9) {
            dimension=BoolConstExpr();
            if (dimension<=0) { dimension=1; CErr1(0x3fc); }
        } else if (first_node==NULL) dimension=0;
        else { dimension=1; CErr1(0x3fd); }
        MustRecog(9);
        next_node=ArrayNode(typ);
        next_node->u.array->dimension=dimension;
        next_node->decl_flags=flags;
        if (first_node==NULL) first_node=next_node;
        else prev_node->object=next_node;
        prev_node=next_node;
    }
    return first_node;
}

/* Old-style declarations followed by the prototype/parameter-list handoff. */
void ParmDeclList(void)
{
    TYPEPTR typ,explicit_type;
    struct parm_entry *parm;
    SYM_ENTRY sym;
    int stg_class;
    while (CurToken!=0x0a) {
        stg_class=GetStgClass();
        typ=TypeSpecifier();
        if (stg_class==SC_NULL && typ==NULL && CurToken==0x33) CErr2p(0x3fe,Buffer);
        if (stg_class!=SC_NULL && stg_class!=0x62) {
            CErr1(0x3ff);
            stg_class=SC_NULL;
        }
        explicit_type=typ;
        if (typ==NULL) typ=TypeDefault();
        if (stg_class==SC_NULL) stg_class=SC_AUTO;
        for (;;) {
            if (CurToken==5) Chk_Struct_Union_Enum(typ);
            else {
                sym.name=NULL;
                Declarator(&sym,typ,explicit_type);
                if (sym.name==NULL || sym.name[0]=='\0') InvDecl();
                else {
                    for (parm=ParmList;parm!=NULL;parm=parm->next_parm) {
                        if (parm->sym.name!=NULL && strcmp(parm->sym.name,sym.name)==0) break;
                    }
                    if (parm==NULL) CErr2p(0x400,sym.name);
                    else if (parm->sym.sym_type!=NULL) CErr2p(0x401,sym.name);
                    else {
                        parm->sym.sym_type=ArgPromotion(sym.sym_type,sym.attrib);
                        parm->sym.stg_class=stg_class;
                    }
                }
                CMemFree(sym.name);
            }
            if (CurToken==5) { NextToken(); break; }
            if (CurToken==0x0a) { CErr1(0x434); break; }
            if (CurToken==1) return;
            MustRecog(2);
        }
    }
    ReverseParms();
    if (CurFunc->sym_type->u.parms==NULL) {
        CurFunc->flags|=0x10;
        AddParms();
    } else ChkParms();
    ParmList=NULL;
    if (VarParm(CurFunc)) CurFunc->flags&=~0x200;
}

void ReverseParms(void)
{
    struct parm_entry *parm,*previous,*next;
    if (ParmsToBeReversed(CurFunc->attrib,NULL)) {
        previous=NULL;
        parm=ParmList;
        while (parm!=NULL) {
            next=parm->next_parm;
            parm->next_parm=previous;
            previous=parm;
            parm=next;
        }
        ParmList=previous;
    }
}

void ChkParms(void)
{
    struct parm_entry *parm,*previous;
    SYM_HANDLE handle,previous_handle;
    TYPEPTR typ;
    CurFunc->u.func.locals=0;
    CurFunc->u.func.parms=0;
    typ=*CurFunc->sym_type->u.parms;
    /* The 1994 body tests this type directly, before the later typedef walk. */
    if (typ->decl_type==TYPE_VOID) return;
    previous=NULL;
    parm=ParmList;
    while (parm!=NULL) {
        if (parm->sym.name==NULL) {
            parm->sym.name=".J";
            parm->sym.flags|=2;
        }
        if (parm->sym.name[0]=='\0') InvDecl();
        if (parm->sym.sym_type==NULL) parm->sym.sym_type=TypeDefault();
        /* Until insertion, this old temporary level byte carries the hash. */
        handle=SymAdd(parm->sym.level,&parm->sym);
        if (previous==NULL) CurFunc->u.func.parms=handle;
        else {
            previous->sym.handle=handle;
            SymReplace(&previous->sym,previous_handle);
            CMemFree(previous);
        }
        previous=parm;
        previous_handle=handle;
        parm->sym.flags|=0x48;
        parm=parm->next_parm;
    }
    if (previous!=NULL) {
        SymReplace(&previous->sym,previous_handle);
        CMemFree(previous);
    }
}

struct textsegment *DefCodeSegment;
void BeginFunc(void)
{
    char *name;
    if (CurFunc->seginfo==NULL) {
        CurFunc->seginfo=DefCodeSegment;
        if (CurFunc->seginfo==NULL && CompFlags.zm_switch_used) {
            name="";
            if (TargetSwitches&8) name=CurFunc->name;
            CurFunc->seginfo=NewTextSeg(name,"_TEXT","");
        }
    }
    if (strcmp(CurFunc->name,"main")==0) {
        CompFlags.main_has_parms=CurFunc->u.func.parms!=0;
        CompFlags.has_main=1;
        MainSymHandle=CurFuncHandle;
    } else if (TargSys==TS_WINDOWS || TargSys==TS_CHEAP_WINDOWS || TargSys==TS_NT) {
        if (stricmp(CurFunc->name,"WinMain")==0) {
            CompFlags.has_winmain=1;
            MainSymHandle=CurFuncHandle;
        } else if (stricmp(CurFunc->name,"LibMain")==0) CompFlags.has_libmain=1;
    }
}

void GetFuncParmList(void)
{
    struct parm_entry *parm,*new_parm;
    /* The old entry has no initialized tail if a preexisting list is passed.
     * Normal FuncProtoType resets ParmList before this path. */
    if (ParmList!=NULL) STUB_TRAP();
    while (CurToken==0x33) {
        if (ParmList==NULL) {
            parm=CMemAlloc(sizeof(struct parm_entry));
            SymCreate(&parm->sym,Buffer);
            ParmList=parm;
        } else {
            new_parm=CMemAlloc(sizeof(struct parm_entry));
            SymCreate(&new_parm->sym,Buffer);
            CheckUniqueName(Buffer);
            parm->next_parm=new_parm;
            parm=new_parm;
        }
        parm->sym.level=HashValue;
        if (!(Toggles&2)) parm->sym.flags|=2;
        NextToken();
        if (CurToken==7 || CurToken==1) break;
        if (CurToken!=2) {
            MustRecog(2);
            while (CurToken!=7 && CurToken!=1) NextToken();
            break;
        }
        MustRecog(2);
        if (CurToken==0x2f) {
            parm->next_parm=CMemAlloc(sizeof(struct parm_entry));
            parm=parm->next_parm;
            SymCreate(&parm->sym,"");
            parm->sym.flags|=0x62;
            NextToken();
            break;
        }
    }
}

TYPEPTR ArgPromotion(TYPEPTR original,unsigned flags)
{
    TYPEPTR typ;
    original=AdjParmType(original,flags);
    typ=original;
    while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
    if (typ->decl_type==TYPE_ENUM) original=GetType(TYPE_INT);
    return original;
}

void AddParms(void)
{
    struct parm_entry *parm,*previous;
    struct parm_list *parmlist;
    SYM_HANDLE handle,previous_handle,new_handle;
    SYM_ENTRY new_sym;
    TYPEPTR typ;
    int count,hash;
    CurFunc->u.func.locals=0;
    CurFunc->u.func.parms=0;
    parmlist=NULL;
    count=0;
    previous=NULL;
    parm=ParmList;
    while (parm!=NULL) {
        parm->sym.flags|=0x48;
        new_handle=0;
        hash=parm->sym.level;
        if (parm->sym.name[0]=='\0') {
            parm->sym.sym_type=GetType(TYPE_DOT_DOT_DOT);
            parm->sym.stg_class=SC_AUTO;
        } else if (parm->sym.sym_type==NULL) {
            parm->sym.sym_type=TypeDefault();
            parm->sym.stg_class=SC_AUTO;
        } else {
            typ=parm->sym.sym_type;
            while (typ->decl_type==TYPE_TYPEDEF) typ=typ->object;
            if (typ->decl_type==TYPE_FLOAT) {
                memcpy(&new_sym,&parm->sym,sizeof(SYM_ENTRY));
                new_sym.handle=CurFunc->u.func.locals;
                new_handle=SymAdd(hash,&new_sym);
                CurFunc->u.func.locals=new_handle;
                SymReplace(&new_sym,new_handle);
                parm->sym.name=".P";
                parm->sym.flags|=2;
                parm->sym.sym_type=GetType(TYPE_DOUBLE);
            }
        }
        handle=SymAdd(hash,&parm->sym);
        if (new_handle!=0) {
            unsigned left,right;
            right=MakeSymLeaf(&parm->sym,handle);
            left=MakeSymLeaf(&new_sym,new_handle);
            GenQuad(left,0x77,right,0);
        }
        if (previous==NULL) CurFunc->u.func.parms=handle;
        else {
            previous->sym.handle=handle;
            SymReplace(&previous->sym,previous_handle);
            CMemFree(previous);
        }
        previous=parm;
        previous_handle=handle;
        ++count;
        parmlist=NewParm(parm->sym.sym_type,parmlist);
        parm=parm->next_parm;
    }
    if (previous!=NULL) {
        previous->sym.handle=0;
        SymReplace(&previous->sym,previous_handle);
        CMemFree(previous);
    }
    typ=CurFunc->sym_type;
    CurFunc->sym_type=FuncNode(typ->object,typ->decl_flags,
        MakeParmList(parmlist,count,ParmsToBeReversed(CurFunc->attrib,NULL)));
    if (PrevProtoType!=NULL) ChkProtoType();
}

TYPEPTR AbsDecl(TYPEPTR typ)
{
    struct mod_info info;
    TYPEPTR result;
    typ=Pointer(typ,&info);
    if (CurToken==6) {
        NextToken();
        if (CurToken==7) return FuncNode(typ,info.modifier,NULL);
        result=AbsDecl(NULL);
        MustRecog(7);
        ParseDeclPart2(&result,typ,info.modifier);
        return result;
    }
    return DeclPart2(typ,info.modifier);
}

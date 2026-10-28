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


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include "compiler.h"
#include "codegen.h"
#include "quads.h"
#include "genquads.h"
#include "scanner.h"
#include "options.h"
#include "declarations.h"
#include "cstring.h"
#include "files.h"
#include "dbinfo.h"
#include "data.h"
#include <recon.h>

struct segment_list;
struct segment_list *SegListHead;
int SegImport;
int Refno;

void EmitInit(void)
{
    SegListHead=NULL;
    SegImport=-1;
    Refno=21; /* This version's T_FIRST_FREE. */
}

void DoCompile(void)
{
    if (RunCodeGenerator()!=0) ++ErrCount;
}

/* The old host releases preprocessing storage before entering the code
 * generator and reports normal return as successful. */
int RunCodeGenerator(void)
{
    FreeMacroSegments();
    GenerateCode();
    return 0;
}

unsigned TryRefno;
struct try_table_back_handle *TryTableBackHandles;
int FunctionProfileSegment;

/* cgen2's integrated 1994 driver, before the later DLL-loading split. */
void GenerateCode(void)
{
    void *old_environment;
    jmp_buf env;
    cg_init_info info;
    old_environment=Environment;
    if (!setjmp(env)) {
        CStringList=0;
        Environment=&env;
        if (!CompFlags.zu_switch_used) TargetSwitches&=~0x200;
        info=BEInit(GenSwitches,TargetSwitches,OptSize,(unsigned short)ProcRevision);
        if (info.success) {
            if (info.version.revision!=9) WrongCodeGen();
            if (info.version.target!=1) WrongCodeGen();
            if (TargetSwitches&0x200000) FunctionProfileSegment=AddSegName("TI","DATA",4);
            SetSegs();
            BEStart();
            TryRefno=NewRefno();
            BEDefType((cg_type)TryRefno,1,28);
            TryTableBackHandles=NULL;
            EmitSegLabels();
            if (GenSwitches&0x01000000) EmitDBType();
            EmitSyms();
            EmitCS_Strings();
            SrcLineCount=0;
            FListSrcQue();
            EmitDataQuads();
            FreeDataQuads();
            GenerateQuads(0,0,QuadIndex,0,0);
            FreeCS_Strings();
            FiniSegLabels();
            BEStop();
            FreeGblVars(GlobalSym);
            FreeGblVars(SpecialSyms);
            FreeExtVars();
            FreeTryTableBackHandles();
            BEFini();
        }
    }
    Environment=old_environment;
}

unsigned NewRefno(void)
{
    return Refno++;
}

void FreeTryTableBackHandles(void)
{
    struct try_table_back_handle *entry;
    while ((entry=TryTableBackHandles)!=NULL) {
        TryTableBackHandles=entry->next;
        BEFreeBack(entry->try_table_back_handle);
        CMemFree(entry);
    }
}

/* cgen2.c releases each recorded back handle without clearing the symbol
 * cell. Save the next16-bit handle before the backend free callback. */
void FreeGblVars(SYM_HANDLE first)
{
    SYMPTR sym;
    while (first!=0) {
        sym=SymGetPtr(first);
        first=sym->handle;
        if (sym->info.backinfo!=NULL) BEFreeBack(sym->info.backinfo);
    }
}

/* Earlier RelExtVars lacks the later SC_STATIC/SYM_FUNC_RETURN_VAR
 * conditions: actuald465 accepts SC_EXTERN, or TYPE_VOID, and nothing else. */
void RelExtVars(SYM_HANDLE first)
{
    SYMPTR sym;
    while (first!=0) {
        sym=SymGetPtr(first);
        first=sym->handle;
        if (sym->stg_class==SC_EXTERN || sym->sym_type->decl_type==TYPE_VOID) {
            if (sym->info.backinfo!=NULL) BEFreeBack(sym->info.backinfo);
        }
    }
}
/* This1994 body walks the one-based serialized quad chain, before later
 * cgen2.c used SymListHeads. Function and lexical-block leaves retain the
 * list of symbol handles whose external back handles must be released. */
void FreeExtVars(void)
{
    unsigned index;
    QUAD quad;
    LEAF leaf;
    SYMPTR sym;
    index=FirstQuad;
    while (index!=0) {
        QuadGet(&quad,index-1);
        index=quad.operand1;
        if (quad.quad_class==T_FUNC_DEFN) {
            LeafGet(&leaf,quad.operand2);
            sym=SymGetPtr(leaf.u.symbol.handle);
            RelExtVars(sym->u.func.locals);
        } else if (quad.quad_class==0x0a && quad.operand2!=0) {
            LeafGet(&leaf,quad.operand2);
            RelExtVars(leaf.u.symbol.handle);
        }
    }
}

void EmitSyms(void)
{
    SYM_HANDLE handle;
    SYM_ENTRY sym;
    for (handle=GlobalSym;handle!=0;handle=sym.handle) {
        SymGet(&sym,handle);
        EmitSym(&sym,handle);
        if ((GenSwitches&0x2000000) && sym.sym_type->decl_type!=TYPE_FUNCTION &&
            !(sym.flags&0x20) && sym.stg_class!=0x58) DBModSym(handle,T_DEFAULT);
    }
}

void EmitSym(SYMPTR sym,SYM_HANDLE handle)
{
    TYPEPTR type;
    int segment;
    unsigned long size;
    type=sym->sym_type;
    if ((GenSwitches&0x1000000) && sym->stg_class==0x58 && type->decl_type!=TYPE_TYPEDEF)
        DBEndName(DBBegName(sym->name,0),DBType(type));
    while (type->decl_type==TYPE_TYPEDEF) type=type->object;
    CGenType(type);
    if (sym->stg_class!=SC_EXTERN && sym->stg_class!=0x58 && !(sym->flags&SYM_FUNCTION)) {
        segment=sym->u.var.segment;
        if (!(sym->flags&0x10) || segment==9) {
            BESetSeg(segment);
            AlignIt(type);
            DGLabel(FEBack(handle));
            size=SizeOfArg(type);
            if (segment==9) DGUBytes(size);
            else if (size!=0) EmitZeros(size);
        }
    }
}

/* Old cgen2.c mapping predates long-long and wide-character entries. */
static unsigned char CGDataType[]={
    T_INT_1,T_UINT_1,T_INT_2,T_UINT_2,T_INTEGER,T_UNSIGNED,T_INT_4,T_UINT_4,
    T_SINGLE,T_DOUBLE,T_POINTER,T_POINTER,T_POINTER,T_POINTER,T_DEFAULT,
    T_DEFAULT,T_INTEGER,T_INTEGER,T_INTEGER,T_INTEGER,T_INTEGER,T_INTEGER
};

int CGenType(TYPEPTR type)
{
    int result;
    unsigned short flags;
    while (type->decl_type==TYPE_TYPEDEF) type=type->object;
    switch (type->decl_type) {
    case TYPE_STRUCT:case TYPE_UNION:
        if (type->object!=NULL) {
            result=NewRefno();
            BEDefType(result,1,TypeSize(type));
        } else if (type->u.tag->refno==0) {
            result=NewRefno();
            BEDefType(result,1,TypeSize(type));
            type->u.tag->refno=result;
        } else result=type->u.tag->refno;
        break;
    case TYPE_ARRAY:
        if (type->u.array->refno==0) {
            result=NewRefno();
            BEDefType(result,1,SizeOfArg(type));
            type->u.array->refno=result;
        }
        result=type->u.array->refno;
        break;
    case TYPE_FIELD:case TYPE_UFIELD:result=CGDataType[type->u.f.field_type];break;
    case TYPE_FUNCTION:result=CodePtrType(type->decl_flags);break;
    case TYPE_ENUM:type=type->object;
    default:
        result=CGDataType[type->decl_type];
        if (result==T_POINTER) {
            flags=type->decl_flags;
            if (type->u.p.segment==10 && (TargetSwitches&0x200)) flags|=8;
            result=PtrType(type->object,flags);
        }
        break;
    }
    return result;
}

void AlignIt(TYPEPTR type)
{
    if (OptSize==0) DGAlign(GetTypeAlignment(type));
}

int PtrType(TYPEPTR type,unsigned short flags)
{
    while (type->decl_type==TYPE_TYPEDEF) type=type->object;
    if (type->decl_type==TYPE_FUNCTION) return CodePtrType(flags);
    if (flags&8) return T_LONG_POINTER;
    if (flags&0x10) return T_HUGE_POINTER;
    if (flags&4) return T_NEAR_POINTER;
    return T_POINTER;
}

int CodePtrType(unsigned short flags)
{
    if (flags&8) return T_LONG_CODE_PTR;
    if (flags&4) return T_NEAR_CODE_PTR;
    return T_CODE_PTR;
}

void EmitCS_Strings(void)
{
    int i;
    if (CompFlags.strings_in_code_segment) {
        DumpCS_Strings();
        for (i=29;i>=0;--i) {
            CS_StringList=StringHash[i];
            DumpCS_Strings();
        }
    }
}

/* cgen2.c's genuine empty hook. Its sole original C3 is also the final
 * instruction of EmitCS_Strings. Current case explicitly excludes this
 * static entry from order; no observed EmitAbort frontier is claimed. */
void EmitAbort(void)
{
}

int FListSrcQue(void)
{
    struct fname_list *file;
    char fullpath[260];
    file=FNames;
    /* This build skips the already-recorded primary filename. */
    if (file!=NULL) file=file->next;
    while (file!=NULL) {
        _fullpath(fullpath,file->name,sizeof(fullpath));
        DBSrcFile(fullpath);
        file=file->next;
    }
    return 0;
}


/* Old string handles name serialized leaves, rather than literal pointers. */
void EmitStrPtr(SYM_HANDLE handle,cg_type pointer_type)
{
    LEAF leaf;
    LeafGet(&leaf,handle);
    Emit1String(&leaf,handle);
    DGBackPtr((struct bck_info *)leaf.u.string.back_handle,StringSegment(&leaf),0,pointer_type);
    ReleaseString(&leaf,handle);
}

void Emit1String(LEAFPTR leaf,unsigned index)
{
    if (leaf->u.string.back_handle==0) {
        leaf->u.string.back_handle=(unsigned)BENewBack(NULL);
        LeafReplace(leaf,index);
        if (!(leaf->leaf_flags&1)) EmitLiteral(leaf);
    }
}

void EmitLiteral(LEAFPTR leaf)
{
    unsigned short previous;
    previous=BESetSeg(StringSegment(leaf));
    if (OptSize==0) DGAlign(4);
    DGLabel((void *)leaf->u.string.back_handle);
    EmitBytes(leaf->u.string.handle);
    BESetSeg(previous);
}

int StringSegment(LEAFPTR leaf)
{
    if (leaf->leaf_flags&8) return SegmentNum-1;
    if (leaf->leaf_flags&1) return 1;
    return 2;
}

unsigned EmitBytes(SYM_HANDLE handle)
{
    union { SYM_ENTRY symbol;STRING_LITERAL literal; } chunk;
    unsigned remaining,length;
    ReadLiteralChunk(&chunk,handle);
    length=remaining=chunk.literal.length;
    DGBytes(remaining>29 ? 29:remaining,(unsigned char *)chunk.literal.literal);
    if (remaining>29) {
        remaining-=29;
        for (;;) {
            ReadLiteralChunk(&chunk,++handle);
            DGBytes(remaining>sizeof(SYM_ENTRY) ? sizeof(SYM_ENTRY):remaining,(unsigned char *)&chunk);
            if (remaining<=sizeof(SYM_ENTRY)) break;
            remaining-=sizeof(SYM_ENTRY);
        }
    }
    return length;
}

void ReleaseString(LEAFPTR leaf,unsigned index)
{
    union { SYM_ENTRY symbol;STRING_LITERAL literal; } chunk;
    if (leaf->leaf_class==0x10) {
        if (--leaf->u.string.references==0) {
            ReadLiteralChunk(&chunk,leaf->u.string.handle);
            chunk.literal.next_string=CStringList;
            CStringList=leaf->u.string.handle;
            WriteLiteralChunk(&chunk,CStringList);
        }
        LeafReplace(leaf,index);
    }
}

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
* Description:  x86 symbol fixup and relocation records.
*
****************************************************************************/


#include "block.h"
#include "address.h"
#include "objemit.h"
#include "encode.h"
#include "stubs.h"
#include <recon.h>

void EmitByte(unsigned char value);
void EmitOffset(long value);
void DoRelocRef(void *symbol,unsigned char memory_type,unsigned short segment,
    long value,unsigned char kind) { STUB_TRAP(); }
void DoFESymRef(void *symbol,unsigned char memory_type,long value,int fix_class)
{
    unsigned attr;
    unsigned char kind;
    int segment;
    kind=0;
    switch (fix_class) {
    case 0: value=0;kind|=0x20;break; /* FE_FIX_BASE / BASE */
    case 2: kind|=0x80;break; /* FE_FIX_SELF / SELF */
    }
    if (value!=0) kind|=0x10; /* OFST */
    if (memory_type==CG_FE) {
        attr=FEAttr(symbol);
        if (!(kind&0x20) && (TargetModel&0x2000U) &&
            (attr&1) && IsFarFunc(symbol)) kind|=0x40; /* LDOF */
        if (UseImportForm((unsigned short)attr)) {
            EmitByte(0x6b);
            EmitByte(1|kind); /* IMP */
            /* The1994 image folds source EmitPtr and EmitOffset into one
             * DWORD append. Preserve target-i386 pointer bits, no dereference. */
            EmitOffset((long)(unsigned long)symbol);
            if (kind&0x10) EmitOffset(value);
        } else {
            segment=AskSegID(symbol,CG_FE);
            if (attr&4) DoRelocRef(symbol,CG_FE,segment,value,kind);
            else DoRelocRef(AskForSymLabel(symbol,CG_FE),CG_LBL,segment,value,kind);
        }
    } else {
        segment=AskSegID(symbol,memory_type);
        DoRelocRef(symbol,memory_type,segment,value,kind);
    }
}
void DoSymRef(name *operand,long value,unsigned char base)
{
    DoFESymRef(operand->v.symbol,operand->m.memory_type,value,base ? 0 : 1);
}

void DoSegRef(unsigned short segment) { STUB_TRAP(); }

static unsigned char SaveDbgOc=0x90;
static void *SaveDbgPtr;
static unsigned long LastUnique=0xffffffffUL;
unsigned long AskLocation(void);
void DbgRtnBeg(void *handle,unsigned long location);
void DbgBlkBeg(void *handle,unsigned long location);
void DbgEpiBeg(void *handle,unsigned long location);
void DumpSavedDebug(void)
{
    switch (SaveDbgOc) {
    case 0x30: DbgRtnBeg(SaveDbgPtr,AskLocation());break;
    case 0x40: DbgBlkBeg(SaveDbgPtr,AskLocation());break;
    case 0x60: DbgEpiBeg(SaveDbgPtr,AskLocation());break;
    }
    SaveDbgOc=0x90;
}
struct code_lbl *ExpandObj(unsigned char *data,int length);
extern unsigned char *NopLists[];
unsigned SavePendingLine(unsigned value);
void SendBytes(unsigned char *data,unsigned length);
void DoAlignment(int length)
{
    unsigned char *data;
    int nop,i;
    unsigned saved_line;
    saved_line=SavePendingLine(0);
    i=0;
    for (nop=NopLists[i][0];length>nop;length-=nop) {
        SendBytes(&NopLists[i][1],nop);
        i=!i;
    }
    for (data=&NopLists[i][1];nop!=length;--nop) data+=nop;
    SendBytes(data,length);
    SavePendingLine(saved_line);
}
void OutDataByte(unsigned char value);
/* Older implementation emits individually rather than later OutDBytes. */
void SendBytes(unsigned char *data,unsigned length)
{
    while (length!=0) {
        OutDataByte(*data++);
        --length;
    }
}
void ExpandCJ(any_oc *record);
unsigned char AskIfUniqueLabel(struct code_lbl *label);
unsigned char AskIfRTLabel(struct code_lbl *label);
unsigned char AskIfCommonLabel(struct code_lbl *label);
void DbgSetBase(void);
void DbgBlkEnd(void *handle,unsigned long location);
void DbgProEnd(void *handle,unsigned long location);
void DbgRtnEnd(void *handle,unsigned long location);
void OutRTImport(int symbol,unsigned char fix_class);
void OutSpecialCommon(int symbol,unsigned char fix_class,unsigned char relative);
void OutImport(void *symbol,unsigned char fix_class,unsigned char relative);
unsigned long AskLocation(void);
void OutDataByte(unsigned char value);
void OutDataInt(int value);
void TellScrapLabel(struct code_lbl *label);
void OutputOC(any_oc *record,any_oc *next_label)
{
    struct code_lbl *label;
    void *symbol;
    unsigned char kind;
    int length;
    unsigned long location;
    unsigned char *data;
    kind=record->op.kind&15;
    if (kind!=6) DumpSavedDebug();
    SetUpObj(0);
    switch (kind) {
    case 2:case 3:
        ExpandObj((unsigned char *)record+3,record->op.reclen-3);
        break;
    case 4:
        if (next_label!=NULL) {
            length=-(AskLocation()+record->op.objlen);
            length&=next_label->op.objlen;
            DoAlignment(length);
        }
        OutSelect(1);
        SendBytes((unsigned char *)record+3,record->op.objlen);
        OutSelect(0);
        break;
    case 5:
        SendBytes((unsigned char *)record+3,record->op.objlen);
        break;
    case 6:
        location=AskLocation();
        length=-location&record->op.objlen;
        if (AskIfUniqueLabel(record->handle.handle)) {
            if (location==LastUnique && length==0) length=record->op.objlen+1;
            LastUnique=location+length;
        }
        DoAlignment(length);
        OutLabel(record->handle.handle);
        DbgSetBase();
        DumpSavedDebug();
        break;
    case 7:
        label=record->handle.handle;
        symbol=AskForLblSym(label);
        if (AskIfRTLabel(label)) {
            OutRTImport((int)symbol,1);
            location=0;
        } else if (AskIfCommonLabel(label)) {
            OutSpecialCommon((int)symbol,1,0);
            location=0;
        } else if (symbol!=NULL && UseImportForm(FEAttr((SYM_HANDLE)(unsigned)symbol))) {
            OutImport(symbol,1,0);
            location=0;
        } else {
            OutReloc(AskOP(),1,0);
            location=AskAddress(label);
            if (location==0xffffffffUL) {
                OutPatch(label,5);
                location=0;
            }
        }
        OutDataLong(location);
        break;
    case 9:case 11:case 13:
        data=(unsigned char *)record+3;
        length=0;
        while (data[length]!=0xff) ++length;
        ++length;
        if (!(TargetModel&0x800)) OutDataByte(0x66);
        ExpandObj(data,length);
        data+=length;
        *data|=kind==9?0x10:0x20;
        OutDataByte(*data++);
        label=ExpandObj(data,record->op.reclen-4-length);
        if (label!=NULL && kind==13) {
            TellKeepLabel(label);
            GenKillLabel(label);
        }
        break;
    case 8:case 10:case 12:
        ExpandCJ(record);
        break;
    case 14:
        if (!(TargetModel&0x800)) OutDataByte(0x66);
        length=0xc2;
        if (record->op.kind&0x10) length|=8;
        if (record->op.kind&0x20) length|=12;
        if (record->op.kind&0x40) {
            OutDataByte((unsigned char)length);
            OutDataInt(record->ret.pops);
        } else OutDataByte((unsigned char)(length|1));
        break;
    case 1:
        kind=record->op.kind&0xf0;
        switch (kind) {
        case 0: OutLineNum(record->linenum.line,record->linenum.label_line);break;
        case 0x10: TellScrapLabel(record->handle.handle);break;
        case 0x20: Zoiks(36);break;
        case 0x30:case 0x40:case 0x60:
            SaveDbgOc=kind;
            SaveDbgPtr=record->debug.ptr;
            break;
        case 0x70: DbgBlkEnd(record->debug.ptr,AskLocation());break;
        case 0x50: DbgProEnd(record->debug.ptr,AskLocation());break;
        case 0x80:
            DbgRtnEnd(record->debug.ptr,AskLocation());
            OutLineNum(0,0);
            break;
        case 0x90: OutSelect(record->select.starts);break;
        }
        break;
    default: Zoiks(36);break;
    }
}

unsigned char CodeHasAbsPatch(oc_header *code)
{
    unsigned char *data,*end;
    data=(unsigned char *)code+3;
    end=(unsigned char *)code+code->reclen;
    while (data<end) {
        if (*data++==0x6b && *data++==4) return 1;
    }
    return 0;
}

void OutAbsPatch(void *patch,unsigned char size);
void OutFPPatch(unsigned char kind);
/* Decode the compiler's own escape-record stream. This old implementation
 * emits literal bytes individually and has no later LBL runtime-import arm. */
struct code_lbl *ExpandObj(unsigned char *data,int length)
{
    unsigned char *end;
    unsigned char key,fix_class,relative;
    struct code_lbl *label;
    void *symbol;
    unsigned long value;
    unsigned short segment;
    label=NULL;
    end=data+length;
    while (data<end) {
        if (*data++!=0x6b) {
            OutDataByte(data[-1]);
            continue;
        }
        key=*data++;
        if (key==0x6b) {
            OutDataByte(0x6b);
            continue;
        }
        if (key&0x20) fix_class=2;
        else if (key&0x40) fix_class=5;
        else fix_class=1;
        relative=(key&0x80)!=0;
        switch (key&15) {
        case 2: /* REL: encoded WORD segment. */
            segment=*(unsigned short *)data;
            data+=2;
            OutReloc(segment,fix_class,relative);
            value=0;
            break;
        case 0: /* SYM: encoded target-i386 symbol handle. */
            symbol=*(void **)data;
            data+=4;
            label=AskForSymLabel(symbol,CG_FE);
            value=AskAddress(label);
            if (value==0xffffffffUL) {
                OutPatch(label,(fix_class==1 || fix_class==5)?5:3);
                value=0;
            }
            OutReloc((unsigned short)AskSegID(symbol,CG_FE),fix_class,relative);
            break;
        case 3: /* LBL: segment then pointer-sized label handle. */
            segment=*(unsigned short *)data;
            data+=2;
            label=*(struct code_lbl **)data;
            data+=4;
            if (AskIfCommonLabel(label))
                OutSpecialCommon((int)AskForLblSym(label),fix_class,relative);
            else OutReloc(segment,fix_class,relative);
            value=AskAddress(label);
            if (value==0xffffffffUL) {
                OutPatch(label,(fix_class==1 || fix_class==5)?5:3);
                value=0;
            }
            break;
        case 1: /* IMP */
            OutImport(*(void **)data,fix_class,relative);
            data+=4;
            value=0;
            break;
        case 4: /* ABS */
            value=*data++;
            OutAbsPatch(*(void **)data,(unsigned char)value);
            data+=4;
            continue;
        case 5: /* FUN */
            OutFPPatch(*data++);
            continue;
        default:
            Zoiks(38);
            STUB_TRAP(); /* Original error path has no established value. */
            return NULL;
        }
        if (key&0x10) {
            value+=*(unsigned long *)data;
            data+=4;
        }
        if (fix_class==2) OutDataInt(value);
        else if (fix_class==1 || fix_class==5) OutDataLong(value);
        else if (fix_class==3) {
            OutDataLong(value);
            OutDataInt(0);
        }
    }
    return label;
}

int OptInsSize(unsigned char kind,int destination);
unsigned char ReverseCondition(unsigned char condition);
void OutShortDisp(struct code_lbl *label)
{
    unsigned long address;
    address=AskAddress(label);
    if (address==0xffffffffUL) {
        OutPatch(label,1);
        address=0;
    }
    OutDataByte((unsigned char)(address-AskLocation()-1));
}
void OutCodeDisp(struct code_lbl *label,unsigned char fix_class,
                 unsigned char relative,unsigned char kind)
{
    unsigned long address;
    void *symbol;
    symbol=AskForLblSym(label);
    if (AskIfRTLabel(label)) {
        OutRTImport((int)symbol,fix_class);
        goto out_zero;
    }
    if (AskIfCommonLabel(label)) {
        OutSpecialCommon((int)symbol,fix_class,relative);
        address=0;
        goto out_value;
    }
    if (symbol!=NULL && UseImportForm((unsigned short)FEAttr(symbol))) {
        OutImport(symbol,fix_class,relative);
        goto out_zero;
    }
    address=AskAddress(label);
    if (!(kind&0x10)) {
        if (address==0xffffffffUL) {
            OutPatch(label,5);            /* ADD_PATCH | LONG_PATCH */
            address=0;
        }
        address=address-AskLocation()-4;
        goto out_value;
    }
    if (address==0xffffffffUL) OutPatch(label,4); /* LONG_PATCH */
    OutDataLong((long)address);
    OutReloc((unsigned short)AskSegID(symbol,CG_FE),2,0);
    OutDataInt(0);
    return;
out_zero:
    /* jumps.h's80386 _OutFarD emits the segment before the offset. */
    if (kind&0x10) OutDataInt(0);
    address=0;
out_value:
    OutDataLong((long)address);
}
void ExpandCJ(any_oc *record)
{
    struct code_lbl *label;
    unsigned char kind,fix_class,relative;
    label=record->handle.handle;
    kind=record->op.kind;
    if ((kind&15)==10) {
        if (record->op.objlen==OptInsSize(10,1)) {
            if ((CGProcessorVersion&15)>=3) {
                if (!(TargetModel&0x800)) OutDataByte(0x66);
                OutDataByte(0x0f);
                OutDataByte(0x80+record->jcond.cond);
            } else {
                OutDataByte(0x70+ReverseCondition(record->jcond.cond));
                OutDataByte((unsigned char)OptInsSize(12,1));
                if (!(TargetModel&0x800)) OutDataByte(0x66);
                OutDataByte(0xe9);
            }
            OutCodeDisp(label,1,1,kind);
        } else {
            OutDataByte(0x70+record->jcond.cond);
            OutShortDisp(label);
        }
    } else if ((kind&15)==12 && record->op.objlen==OptInsSize(12,0)) {
        OutDataByte(0xeb);
        OutShortDisp(label);
    } else {
        if (kind&0x10) {
            fix_class=3;
            relative=0;
            if ((kind&15)==8) {
                if (record->op.objlen==OptInsSize(8,2)) {
                    fix_class=1;
                    relative=1;
                    kind&=~0x10;
                    OutDataByte(0x0e);
                    if (!(TargetModel&0x800)) OutDataByte(0x66);
                    OutDataByte(0xe8);
                } else {
                    if (!(TargetModel&0x800)) OutDataByte(0x66);
                    OutDataByte(0x9a);
                }
            } else {
                if (!(TargetModel&0x800)) OutDataByte(0x66);
                OutDataByte(0xea);
            }
        } else {
            fix_class=1;
            relative=1;
            if (!(TargetModel&0x800)) OutDataByte(0x66);
            if ((kind&15)==8) OutDataByte(0xe8);
            else OutDataByte(0xe9);
        }
        OutCodeDisp(label,fix_class,relative,kind);
    }
}

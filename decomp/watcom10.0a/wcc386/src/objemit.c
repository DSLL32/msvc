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
#include <string.h>
#include "codegen.h"
#include "cgmemory.h"
#include "memory.h"
#include "objemit.h"
#include "pragma.h"
#include "dbinfo.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>
#pragma pack(1)
struct segdef {
    struct segdef *next;
    char *str;
    unsigned short id;
    unsigned char align,attr;
};
struct unsafe_fixup_comment {
    unsigned short type;
    unsigned char directive;
};
#pragma pack()
typedef char old_segdef_size[(sizeof(struct segdef)==12)?1:-1];
typedef char old_unsafe_fixup_comment_size[(sizeof(struct unsafe_fixup_comment)==3)?1:-1];
#pragma pack(4)
struct lname_cache {
    struct lname_cache *next;
    unsigned idx;
    unsigned char name[1];
};
#pragma pack()
typedef char old_lname_cache_size[(sizeof(struct lname_cache)==12)?1:-1];
static struct segdef *SegDefs;
static struct lname_cache *NameCache,*NameCacheDumped;
static unsigned NameIndex,GroupIndex,DGroupIndex,FlatGIndex,SegsDefd,BackSegIdx;
static unsigned short CodeSeg;
static unsigned short BackSeg;

/* i86obj.c before separate thread-local-storage groups were added. */
void InitSegDefs(void)
{
    SegDefs=NULL;
    NameCache=NULL;
    NameCacheDumped=NULL;
    NameIndex=0;
    GroupIndex=0;
    DGroupIndex=0;
    FlatGIndex=0;
    SegsDefd=0;
    CodeSeg=0x7fff;
    BackSegIdx=0x7fff;
}

void DefSegment(int id,unsigned char attr,char *name,unsigned alignment,unsigned char use16)
{
    struct segdef *segment,**owner;
    unsigned short first_code;
    segment=CGAlloc(sizeof(*segment));
    segment->id=id;
    segment->attr=attr;
    segment->align=alignment;
    segment->str=CGAlloc(Length(name)+1);
    CopyStr(name,segment->str);
    owner=&SegDefs;
    while (*owner!=NULL) owner=&(*owner)->next;
    first_code=0x7fff;
    *owner=segment;
    segment->next=NULL;
    if (attr&1) {
        if (CodeSeg==0x7fff) { CodeSeg=id;first_code=id; }
        if (OptForSize==0 && segment->align<16) segment->align=16;
    }
    if (attr&0x10) BackSeg=id;
    if (NameIndex!=0) {
        DoSegment(segment,NULL,use16);
        SegDefs=NULL;
    }
    if (first_code!=0x7fff && (Model&0x10000000)) DFBegCCU(first_code);
}

static unsigned long CodeSize,DataSize;
static unsigned char CurrFNo,GenStaticImports;
static unsigned FlatNIndex,ImportHdl;
static array_control *Imports;
struct abspatch;
static struct abspatch *AbsPatches;
static struct index_rec *CurrSeg;

/* The earlier OMF32 writer uses one DGROUP array; the later code adds a
 * separate thread-local group. Record fields are little-endian on x86. */
extern unsigned char GeneratingProfileCode;
void ObjInit(void)
{
    array_control *names,*group;
    void *depend;
    InitFPPatches();
    GeneratingProfileCode=0;
    CurrFNo=0;
    CodeSize=0;
    DataSize=0;
    OpenObj();
    names=InitArray(1,50,50);
    OutName(FEAuxInfo(NULL,10),names);
    PutObjRec(0x80,names->array,names->used);
    if (TargetModel&(2|0x20)) {
        names->used=0;
        NeedMore(names,2);
        *(unsigned short *)((char *)names->array+names->used)=0xaa80;
        names->used+=2;
        OutString((TargetModel&2) ? "80386":"OS220",names);
        PutObjRec(0x88,names->array,names->used);
    }
    names->used=0;
    NeedMore(names,2);
    *(unsigned short *)((char *)names->array+names->used)=0x9b80;
    names->used+=2;
    OutModel(names);
    PutObjRec(0x88,names->array,names->used);
    if ((TargetModel&0x20) && (Model&0x10000000)) {
        names->used=0;
        NeedMore(names,2);
        *(unsigned short *)((char *)names->array+names->used)=0xfe80;
        names->used+=2;
        NeedMore(names,1);
        ((char *)names->array)[names->used++]='F';
        PutObjRec(0x88,names->array,names->used);
    }
    if (!(Model&0x18000000)) {
        names->used=0;
        NeedMore(names,2);
        *(unsigned short *)((char *)names->array+names->used)=0xfe80;
        names->used+=2;
        NeedMore(names,1);
        ((char *)names->array)[names->used++]='D';
        NeedMore(names,1);
        ((char *)names->array)[names->used++]=1;
        NeedMore(names,1);
        ((char *)names->array)[names->used++]=(Model&0x03000000) ? 3:0;
        OutString(FEAuxInfo(NULL,16),names);
        PutObjRec(0x88,names->array,names->used);
    }
    names->used=0;
    depend=NULL;
    for (;;) {
        depend=FEAuxInfo(depend,13);
        if (depend==NULL) break;
        NeedMore(names,2);
        *(unsigned short *)((char *)names->array+names->used)=0xe980;
        names->used+=2;
        NeedMore(names,4);
        *(unsigned long *)((char *)names->array+names->used)=*(unsigned long *)FEAuxInfo(depend,14);
        names->used+=4;
        OutName(FEAuxInfo(depend,15),names);
        PutObjRec(0x88,names->array,names->used);
        names->used=0;
    }
    NeedMore(names,2);
    *(unsigned short *)((char *)names->array+names->used)=0xe980;
    names->used+=2;
    PutObjRec(0x88,names->array,names->used);
    names->used=0;
    group=InitArray(1,5,5);
    DoSegGrpNames(group);
    if (group->used!=1) {
        FlushNames();
        PutObjRec(0x9a,group->array,group->used);
    }
    group->used=0;
    if ((TargetModel&0x20) && !(TargetModel&2)) {
        FlatGIndex=++GroupIndex;
        OutIdx(FlatNIndex,group);
        FlushNames();
        PutObjRec(0x9a,group->array,group->used);
        group->used=0;
    }
    KillArray(group);
    CurrSeg=AskSegIndex(CodeSeg);
    KillArray(names);
    ImportHdl=1;
    Imports=NULL;
    GenStaticImports=0;
    AbsPatches=NULL;
    if (Model&0x10000000) DFObjInitInfo();
    else if (Model&0x08000000) CVObjInitInfo();
    else WVObjInitInfo();
}

/* This target has nine8087 patch-import roles. */
static unsigned FPPatchImp[9];

void InitFPPatches(void)
{
    int i;
    i=9;
    while (--i>=0) FPPatchImp[i]=0;
}

array_control *InitArray(int entry,int initial,int increment)
{
    array_control *array;
    array=CGAlloc(sizeof(*array));
    FillArray(array,entry,initial,increment);
    return array;
}

void FillArray(array_control *array,int size,int initial,int increment)
{
    array->array=CGAlloc((unsigned)initial*(unsigned)size);
    array->alloc=initial;
    array->used=0;
    array->entry=size;
    array->inc=increment;
}

void OutName(char *name,void *context)
{
    array_control *dest;
    unsigned length;
    dest=context;
    length=Length(name);
    NeedMore(dest,length+1);
    ((unsigned char *)dest->array)[dest->used++]=length;
    Copy(name,(char *)dest->array+dest->used,length);
    dest->used+=length;
}

static array_control *Out;
static unsigned char *OutBuff;

void NeedMore(array_control *array,int more)
{
    char *buffer;
    unsigned need,allocated;
    need=(unsigned)array->used+(unsigned)more;
    if (need>(unsigned)array->alloc) {
        allocated=array->alloc;
        do { allocated+=array->inc; } while (allocated<need);
        buffer=CGAlloc((unsigned)array->entry*allocated);
        Copy(array->array,buffer,(unsigned)array->entry*(unsigned)array->used);
        if (array==Out) OutBuff=(unsigned char *)buffer;
        CGFreeSize(array->array,(unsigned)array->entry*(unsigned)array->alloc);
        array->array=buffer;
        array->alloc=allocated;
    }
}

void OutString(char *name,array_control *dest)
{
    unsigned length;
    length=Length(name);
    NeedMore(dest,length);
    Copy(name,(char *)dest->array+dest->used,length);
    dest->used+=length;
}

void OutModel(array_control *dest)
{
    char model[5];
    unsigned cpu,fpu;
    cpu=CGProcessorVersion&0x0f;
    model[0]=cpu>=3 ? '3':cpu>=2 ? '2':'0';
    model[1]=GetMemModel();
    model[2]='O';
    fpu=(CGProcessorVersion&0xf0)&0x7f;
    if (fpu>=0x10) model[3]=(CGProcessorVersion&0x80) ? 'e':'p';
    else model[3]='c';
    /* The old comment is four characters; no later PIC marker byte. */
    model[4]='\0';
    OutString(model,dest);
}

unsigned char GetMemModel(void)
{
    if (TargetModel&8) {
        if (TargetModel&4) return (TargetModel&0x10) ? 'l':'h';
        return 'm';
    }
    if (TargetModel&4) return 'c';
    if (TargetModel&0x20) return 'f';
    return 's';
}

static unsigned SegmentIndex,PrivateIndexRO,PrivateIndexRW;
static char CodeGroup[80],DataGroup[80];
static unsigned CodeGroupNIdx,CodeGroupGIdx;
static unsigned char NoDGroup;
static array_control *SegInfo;
static long DbgTypeSize;
struct dbg_seg_info { short *id;char *seg_name,*class_name; };
static struct dbg_seg_info DbgSegs[]={
    {&DbgLocals,"$$SYMBOLS","DEBSYM"},
    {&DbgTypes,"$$TYPES","DEBTYP"}
};

void DoSegGrpNames(array_control *group)
{
    struct segdef *segment,*next;
    char *name;
    unsigned index;
    GetNameIdx("","",1);
    GetNameIdx("CODE","",1);
    GetNameIdx("DATA","",1);
    GetNameIdx("BSS","",1);
    if ((TargetModel&0x20) && !(TargetModel&2)) FlatNIndex=GetNameIdx("FLAT","",1);
    SegmentIndex=0;
    PrivateIndexRO=0;
    PrivateIndexRW=0;
    CopyStr(FEAuxInfo(NULL,30),CodeGroup);
    if (CodeGroup[0]=='\0') CodeGroupGIdx=0;
    else {
        CodeGroupNIdx=GetNameIdx(CodeGroup,"",1);
        CodeGroupGIdx=++GroupIndex;
    }
    name=FEAuxInfo(NULL,32);
    if (name==NULL) NoDGroup=1;
    else CopyStr(name,DataGroup);
    if (DataGroup[0]!='\0') {
        TargetModel|=0x200;
        index=GetNameIdx(DataGroup,"_GROUP",1);
    } else index=GetNameIdx("DGROUP","",1);
    OutIdx(index,group);
    segment=SegDefs;
    /* Recovered old index_rec contribution is64B. Fields are encountered
     * by DoSegment later; no opaque copied record is initialized here. */
    SegInfo=InitArray(sizeof(struct index_rec),5,5);
    while (segment!=NULL) {
        next=segment->next;
        DoSegment(segment,group,0);
        segment=next;
    }
    SegDefs=NULL;
    if (Model&0x10000000) DFDefSegs();
    else if (Model&0x08000000) CVDefSegs();
    else {
        DbgTypeSize=0;
        if (Model&0x02000000) DbgLocals=DbgSegDef(DbgSegs[0].seg_name,DbgSegs[0].class_name,0);
        if (Model&0x01000000) DbgTypes=DbgSegDef(DbgSegs[1].seg_name,DbgSegs[1].class_name,0);
    }
}

unsigned GetNameIdx(char *name,char *suffix,unsigned char allocate)
{
    struct lname_cache **owner,*current;
    unsigned length,suffix_length;
    length=Length(name);
    suffix_length=Length(suffix);
    owner=&NameCache;
    while ((current=*owner)!=NULL) {
        if (length+suffix_length==current->name[0] &&
            memcmp(name,&current->name[1],length)==0 &&
            memcmp(suffix,&current->name[length+1],suffix_length)==0)
            return current->idx;
        owner=&current->next;
    }
    if (!allocate) return 0;
    current=CGAlloc(sizeof(*current)+length+suffix_length);
    *owner=current;
    current->next=NULL;
    current->idx=++NameIndex;
    current->name[0]=length+suffix_length;
    memcpy(&current->name[1],name,length);
    memcpy(&current->name[length+1],suffix,suffix_length);
    return NameIndex;
}

void OutIdx(int value,array_control *dest)
{
    NeedMore(dest,2);
    if (value>=128) {
        ((unsigned char *)dest->array)[dest->used++]=(value>>8)|0x80;
    }
    ((unsigned char *)dest->array)[dest->used++]=value;
}

void DoSegment(struct segdef *segment,void *group,unsigned char use16)
{
    struct index_rec *record;
    unsigned short old;
    record=AskSegIndex(segment->id);
    if (record==NULL) {
        if (CurrSeg!=NULL) old=CurrSeg->seg;
        NeedMore(SegInfo,1);
        record=(struct index_rec *)SegInfo->array+SegInfo->used++;
        if (CurrSeg!=NULL) CurrSeg=AskSegIndex(old);
    }
    record->private=(segment->attr&0x40)!=0;
    record->location=0;
    record->max_written=0;
    record->max_size=0;
    record->comdat_label=NULL;
    record->comdat_symbol=NULL;
    record->total_comdat_size=0;
    record->comdat_size=0;
    record->virt_func_refs=NULL;
    record->big=0;
    record->rom=0;
    record->need_base_set=1;
    record->data_in_code=0;
    record->start_data_in_code=0;
    record->data_ptr_in_code=0;
    record->prefix_comdat_state=0;
    record->sidx=++SegmentIndex;
    record->seg=segment->id;
    record->attr=SegmentAttr(segment->align,segment->attr,use16);
    if (segment->attr&1) {
        record->exec=1;
        record->rom=1;
        record->cidx=2;
        if (segment->attr&0x80) record->nidx=GetNameIdx(segment->str,"",1);
        else if (CodeGroupGIdx!=0) record->nidx=GetNameIdx(CodeGroup,segment->str,1);
        else if (TargetModel&8) record->nidx=GetNameIdx(FEModuleName(),segment->str,1);
        else record->nidx=GetNameIdx(segment->str,"",1);
        SegmentClass(record);
        if (CodeGroupGIdx!=0) {
            record->base=CodeGroupGIdx;
            record->btype=1;
            DoASegDef(record,use16);
            OutCGroup(record->sidx);
        } else {
            record->base=SegmentIndex;
            record->btype=0;
            DoASegDef(record,use16);
        }
    } else {
        record->exec=0;
        if (segment->attr&8) record->rom=1;
        if (segment->attr&0x40) {
            record->nidx=GetNameIdx(segment->str,"",1);
            if (segment->attr&8) {
                if (PrivateIndexRO==0) PrivateIndexRO=GetNameIdx("FAR_CONST","",1);
                record->cidx=PrivateIndexRO;
            } else {
                if (PrivateIndexRW==0) PrivateIndexRW=GetNameIdx("FAR_DATA","",1);
                record->cidx=PrivateIndexRW;
            }
            SegmentClass(record);
            record->base=SegmentIndex;
            record->btype=0;
            DoASegDef(record,use16);
        } else {
            if (NoDGroup) {
                record->btype=0;
                record->base=SegmentIndex;
            } else {
                OutGroup(record->sidx,group);
                record->base=DGroupIndex;
                record->btype=1;
            }
            if ((segment->attr&0x20) || (segment->attr&0x14)) record->cidx=3;
            else record->cidx=4;
            record->nidx=GetNameIdx(DataGroup,segment->str,1);
            SegmentClass(record);
            DoASegDef(record,use16);
        }
    }
    CGFreeSize(segment->str,Length(segment->str)+1);
    CGFreeSize(segment,sizeof(*segment));
}

struct index_rec *AskSegIndex(unsigned short segment)
{
    struct index_rec *record;
    int i;
    i=0;
    record=SegInfo->array;
    for (;;) {
        if (++i>SegInfo->used) return NULL;
        if (record->seg==segment) return record;
        ++record;
    }
}

unsigned char SegmentAttr(unsigned char alignment,unsigned char flags,unsigned char use16)
{
    unsigned char attr;
    if (alignment<=1) attr=0x20;
    else if (alignment<=2) attr=0x40;
    else if (alignment<=4) attr=0xa0;
    else if (alignment<=16) attr=0x60;
    else {
        attr=0x80;
        /* The old byte-sized alignment makes the 4K test unreachable. */
        if ((TargetModel&2) && (unsigned)alignment>256) attr=0xc0;
    }
    if (flags&0x20) attr|=0x18;
    else if (!(flags&0x40) || (flags&2)) attr|=8;
    if (!(TargetModel&2) && (TargetModel&0x800) && !use16) attr|=1;
    return attr;
}

void SegmentClass(struct index_rec *record)
{
    char *name;
    name=FEAuxInfo((void *)(unsigned)record->seg,29);
    if (name!=NULL) record->cidx=GetNameIdx(name,"",1);
}

void DoASegDef(struct index_rec *record,unsigned char use16)
{
    struct object *object;
    object=CGAlloc(sizeof(*object));
    record->obj=object;
    object->index=record->sidx;
    object->patches=NULL;
    object->gen_static_exports=0;
    object->pending_line_number=0;
    Out=&object->data;
    FillArray(&object->data,1,256,256);
    FillArray(&object->fixes,1,20,50);
    object->exports=record->exec ? InitArray(1,256,50):NULL;
    object->lines=NULL;
    object->line_info=0;
    Out=&object->data;
    OutBuff=Out->array;
    OutByte(record->attr);
    OutOffset(0);
    OutIdx(record->nidx,Out);
    OutIdx(record->cidx,Out);
    OutIdx(1,Out);
    /* Earlier OSI build writes this index through OutIdx, not OutByte. */
    if ((!(TargetModel&0x800) && (TargetModel&2)) || use16) OutIdx(2,Out);
    FlushNames();
    object->segfix=AskObjHandle();
    if (++SegsDefd>32000) FEMessage(4,"too many segments");
    PutObjRec(PickOMF(0x98),Out->array,Out->used);
    if (record->exec) {
        Out->used=0;
        OutInt(0xfe80);
        OutByte(0x4f);
        OutIdx(record->sidx,Out);
        PutObjRec(0x88,Out->array,Out->used);
    }
    Out->used=0;
    record->location=0;
}

void OutByte(unsigned char value)
{
    NeedMore(Out,1);
    OutBuff[Out->used++]=value;
}

void OutOffset(unsigned long value)
{
    NeedMore(Out,sizeof(value));
    *(unsigned long *)(OutBuff+Out->used)=value;
    Out->used+=sizeof(value);
}

void FlushNames(void)
{
    unsigned char buffer[512];
    unsigned used;
    struct lname_cache *name;
    used=0;
    name=NameCacheDumped!=NULL ? NameCacheDumped->next:NameCache;
    for (;name!=NULL;name=name->next) {
        if (used+name->name[0]>sizeof(buffer)-1) {
            PutObjRec(0x96,buffer,used);
            used=0;
        }
        buffer[used++]=name->name[0];
        Copy((char *)name->name+1,(char *)buffer+used,name->name[0]);
        used+=name->name[0];
        NameCacheDumped=name;
    }
    if (used>0) PutObjRec(0x96,buffer,used);
}

unsigned char PickOMF(unsigned char command)
{
    if (!(TargetModel&2)) ++command;
    return command;
}

void OutInt(unsigned value)
{
    NeedMore(Out,sizeof(unsigned short));
    *(unsigned short *)(OutBuff+Out->used)=value;
    Out->used+=sizeof(unsigned short);
}

void OutGroup(unsigned segment,array_control *group)
{
    if (DGroupIndex==0) DGroupIndex=++GroupIndex;
    NeedMore(group,1);
    ((unsigned char *)group->array)[group->used++]=0xff;
    OutIdx(segment,group);
}

void KillArray(array_control *array)
{
    KillStatic(array);
    CGFreeSize(array,sizeof(*array));
}

void KillStatic(array_control *array)
{
    CGFreeSize(array->array,(unsigned)array->entry*(unsigned)array->alloc);
}

unsigned short SetOP(unsigned short segment)
{
    unsigned short old;
    old=CurrSeg==NULL ? (unsigned short)-1:CurrSeg->seg;
    CurrSeg=segment==(unsigned short)-1 ? NULL:AskSegIndex(segment);
    return old;
}

void DataLabel(struct code_lbl *label)
{
    TellObjNewLabel(AskForLblSym(label));
    TellOptimizerByPassed();
    SetUpObj(1);
    OutLabel(label);
    TellByPassOver();
}

void TellObjNewLabel(void *symbol)
{
    if (symbol==NULL || CurrSeg==NULL || CodeSeg!=CurrSeg->seg) return;
    if (FEAttr(symbol)&0x200) {
        DoEmptyQueue();
        if ((Model&0x10000000) && CurrSeg->comdat_symbol!=NULL)
            DFSymRange(CurrSeg->comdat_symbol,CurrSeg->comdat_size);
    } else if (CurrSeg->comdat_symbol!=NULL) {
        DoEmptyQueue();
        SetUpObj(0);
        if ((Model&0x10000000) && CurrSeg->comdat_symbol!=NULL)
            DFSymRange(CurrSeg->comdat_symbol,CurrSeg->comdat_size);
    }
}

void SetUpObj(unsigned char is_data)
{
    struct object *object;
    unsigned char old_data;
    for (;;) {
        object=CurrSeg->obj;
        if (object==NULL) return;
        Out=&object->data;
        OutBuff=Out->array;
        if (object->fixes.used>=192 || (Imports!=NULL && Imports->used>=192)) {
            EjectLEData();
            return;
        }
        CheckLEDataSize(16,0);
        if (CurrSeg->exec) {
            old_data=CurrSeg->data_in_code;
            CurrSeg->data_in_code=is_data;
            if (is_data!=old_data) {
                if (!is_data) {
                    OutSelect(0);
                    continue;
                }
                CurrSeg->start_data_in_code=1;
            }
        }
        return;
    }
}

void CheckLEDataSize(unsigned maximum,unsigned char need_init)
{
    unsigned long start,end_valid;
    unsigned used;
    struct object *object;
    object=CurrSeg->obj;
    start=object->start;
    used=object->data.used;
    if (CurrSeg->location<start || CurrSeg->location-start+maximum>
            192-CurrSeg->data_prefix_size) EjectLEData();
    else {
        end_valid=start+used-CurrSeg->data_prefix_size;
        if (CurrSeg->max_written>end_valid && CurrSeg->location>end_valid) EjectLEData();
    }
    if (need_init) OutLEDataStart(0);
}

void EjectImports(void)
{
    unsigned char command;
    if (Imports!=NULL && Imports->used!=0) {
        command=GenStaticImports ? 0xb4:0x8c;
        PutObjRec(command,Imports->array,Imports->used);
        Imports->used=0;
    }
}
void SetPatches(void)
{
    struct temp_patch *current,*old;
    array_control *control;
    struct patch *patch;
    current=CurrSeg->obj->patches;
    while (current!=NULL) {
        control=AskLblPatch(current->label);
        NeedMore(control,1);
        patch=(struct patch *)control->array+control->used++;
        patch->ref=AskObjHandle();
        patch->where=current->patch.where;
        patch->attr=current->patch.attr;
        old=current;
        current=current->link;
        CGFreeSize(old,sizeof(*old));
    }
}
void SetAbsPatches(void)
{
    struct object *object;
    struct abspatch *patch;
    object=CurrSeg->obj;
    for (patch=AbsPatches;patch!=NULL;patch=patch->link) {
        if (patch->patch.ref==0 && patch->obj==object) {
            patch->patch.ref=AskObjHandle();
            patch->flags|=2;               /* AP_HAVE_OFFSET */
        }
    }
}

void DoPatch(struct patch *patch,unsigned long value)
{
    unsigned long lword;
    unsigned short word;
    unsigned char bite,*bytes;
    int length;
    if (patch->attr&4) {                  /* LONG_PATCH */
        if (patch->attr&1) {
            GetFromObj(patch->ref,patch->where,(unsigned char *)&lword,4);
            lword+=value;
        } else lword=value;
        bytes=(unsigned char *)&lword;
        length=4;
    } else if (patch->attr&2) {           /* WORD_PATCH */
        if (patch->attr&1) {
            GetFromObj(patch->ref,patch->where,(unsigned char *)&word,2);
            word+=value;
        } else word=value;
        bytes=(unsigned char *)&word;
        length=2;
    } else {
        if (patch->attr&1) {
            GetFromObj(patch->ref,patch->where,&bite,1);
            bite+=value;
        } else bite=value;
        bytes=&bite;
        length=1;
    }
    PatchObj(patch->ref,patch->where,bytes,length);
}

void EjectLEData(void)
{
    struct object *object;
    struct unsafe_fixup_comment comment;
    unsigned char command;
    EjectImports();
    object=CurrSeg->obj;
    if (object->data.used>CurrSeg->data_prefix_size) {
        SetPatches();
        SetAbsPatches();
        command=CurrSeg->comdat_label!=NULL ? 0xc2:0xa0;
        PutObjRec(PickOMF(command),object->data.array,object->data.used);
        if (object->fixes.used!=0) {
            if (CurrSeg->data_ptr_in_code) {
                comment.type=0xfe80;       /* LINKER_COMMENT */
                comment.directive=0x55;    /* LDIR_OPT_UNSAFE */
                PutObjRec(0x88,&comment,sizeof(comment));
            }
            PutObjRec(PickOMF(0x9c),object->fixes.array,object->fixes.used);
            object->fixes.used=0;
        }
        CurrSeg->data_ptr_in_code=0;
        object->data.used=0;
        object->patches=NULL;
    }
}

void OutLabel(struct code_lbl *label)
{
    struct temp_patch **owner,*current;
    struct virt_func_ref_list *reference,*next;
    array_control *patches;
    struct patch *patch;
    struct object *object;
    unsigned char *where;
    void *symbol,*cookie;
    unsigned short attributes;
    unsigned long location;
    int i;
    symbol=AskForLblSym(label);
    if (symbol!=NULL) {
        attributes=FEAttr(symbol);
        /* This version predates the explicit OutDLLExport call here. */
        if (attributes&0x200) ComdatData(label);
        else {
            if (CurrSeg->comdat_label!=NULL) NormalData();
            if (UseImportForm(attributes)) OutExport(symbol);
        }
        for (reference=CurrSeg->virt_func_refs;reference!=NULL;reference=next) {
            cookie=reference->cookie;
            while (cookie!=NULL) {
                OutVirtFuncRef(FEAuxInfo(cookie,48));
                cookie=FEAuxInfo(cookie,49);
            }
            next=reference->next;
            CGFreeSize(reference,sizeof(*reference));
        }
        CurrSeg->virt_func_refs=NULL;
    } else if (CurrSeg->prefix_comdat_state!=0) {
        if (CurrSeg->prefix_comdat_state==1) {
            ComdatData(label);
            CurrSeg->prefix_comdat_state=2;
        }
        TellCommonLabel(label,CurrSeg->comdat_prefix_import);
    }
    i=SegInfo->used;
    location=CurrSeg->location;
    TellAddress(label,location);
    while (--i>=0) {
        object=((struct index_rec *)SegInfo->array)[i].obj;
        if (object!=NULL) {
            owner=&object->patches;
            for (;;) {
                current=*owner;
                if (current==NULL) break;
                if (current->label==label) {
                    where=(unsigned char *)object->data.array+current->patch.where;
                    if (current->patch.attr&1) {
                        if (current->patch.attr&4) *(unsigned long *)where+=location;
                        else if (current->patch.attr&2) *(unsigned short *)where+=location;
                        else *where+=location;
                    } else {
                        if (current->patch.attr&4) *(unsigned long *)where=location;
                        else if (current->patch.attr&2) *(unsigned short *)where=location;
                        else *where=location;
                    }
                    *owner=current->link;
                    CGFreeSize(current,sizeof(*current));
                } else owner=&current->link;
            }
        }
    }
    patches=AskLblPatch(label);
    i=patches->used;
    patch=patches->array;
    while (i>0) {
        DoPatch(patch,location);
        ++patch;
        --i;
    }
    KillArray(patches);
    TellDonePatch(label);
}

unsigned char UseImportForm(unsigned short attributes)
{
    if (attributes&0x0c) return 1;
    if (attributes&0x1000) return 0;
    return 1;
}

void OutExport(void *symbol)
{
    array_control *exports;
    struct object *object;
    unsigned short attributes;
    object=CurrSeg->obj;
    exports=object->exports;
    if (exports==NULL) {
        exports=InitArray(1,256,50);
        object->exports=exports;
    }
    if (object->exports->used>=192) EjectExports();
    attributes=FEAttr(symbol);
    if (object->gen_static_exports) {
        if (attributes&4) {
            EjectExports();
            object->gen_static_exports=0;
        }
    } else if (!(attributes&4)) {
        EjectExports();
        object->gen_static_exports=1;
    }
    if (object->exports->used==0) {
        OutIdx(CurrSeg->btype==1 ? CurrSeg->base:FlatGIndex,exports);
        OutIdx(object->index,exports);
    }
    OutObjectName(symbol,exports);
    NeedMore(exports,sizeof(unsigned long));
    *(unsigned long *)((char *)exports->array+exports->used)=CurrSeg->location;
    exports->used+=sizeof(unsigned long);
    OutIdx(0,exports);
}

void EjectExports(void)
{
    struct object *object;
    unsigned char command;
    object=CurrSeg->obj;
    if (object->exports!=NULL && object->exports->used!=0) {
        command=object->gen_static_exports ? 0xb6:0x90;
        PutObjRec(PickOMF(command),object->exports->array,object->exports->used);
        object->exports->used=0;
    }
}

void OutObjectName(void *symbol,array_control *array)
{
    DoOutObjectName(symbol,OutName,array,0);
}

/* objname.c before fixed output buffers and FEExtName replaced this
 * dynamically expanded AUX_LOOKUP/OBJECT_NAME pattern. */
void DoOutObjectName(void *symbol,void (*output)(char *,void *),
                     void *context,unsigned char type)
{
    void *aux;
    char *pattern,*source,*buffer,*dest,*end;
    unsigned name_length,required;
    aux=FEAuxInfo(symbol,0);
    pattern=CGAlloc(Length(FEAuxInfo(aux,39))+1);
    CopyStr(FEAuxInfo(aux,39),pattern);
    name_length=Length(FEName(symbol));
    required=1;
    for (source=pattern;*source!='\0';++source) {
        switch (*source) {
        case '*':case '^':case '!':required+=name_length;break;
        case '\\':++source;
        default:++required;break;
        }
    }
    if (type!=0) required+=13;
    buffer=CGAlloc(required);
    dest=buffer;
    if (type!=0) dest=CopyStr(".PREFIX_DATA.",dest);
    for (source=pattern;*source!='\0';++source) {
        switch (*source) {
        case '\\':*dest++=*++source;break;
        case '*':dest=CopyStr(FEName(symbol),dest);break;
        case '^':
            end=CopyStr(FEName(symbol),dest);
            while (dest!=end) {
                if (*dest>='a' && *dest<='z') *dest-='a'-'A';
                ++dest;
            }
            break;
        case '!':
            end=CopyStr(FEName(symbol),dest);
            while (dest!=end) {
                if (*dest>='A' && *dest<='Z') *dest+='a'-'A';
                ++dest;
            }
            break;
        default:*dest++=*source;break;
        }
    }
    *dest='\0';
    output(buffer,context);
    CGFreeSize(pattern,Length(pattern)+1);
    CGFreeSize(buffer,required);
}

array_control *InitPatch(void)
{
    return InitArray(sizeof(struct patch),10,10);
}

unsigned short AskOP(void)
{
    return CurrSeg->seg;
}

void IncLocation(unsigned long size)
{
    unsigned long sum;
    CurrSeg->obj->pending_line_number=0;
    sum=CurrSeg->location+size;
    if (sum<CurrSeg->location) {
        /* Earlier build has neither USE16 truncation nor the later
         * repeated-overflow test; retain its actual location=1 update. */
        if (sum!=0) FatalError("segment too large");
        else if (CurrSeg->comdat_label==NULL) {
            CurrSeg->max_size=(unsigned long)-1;
            CurrSeg->location=1;
            CurrSeg->big=1;
        }
    } else SetLocation(sum);
}

void SetLocation(unsigned long location)
{
    CurrSeg->location=location;
    if (CurrSeg->comdat_label!=NULL) {
        if (location>CurrSeg->comdat_size) CurrSeg->comdat_size=location;
    } else if (location>CurrSeg->max_size) CurrSeg->max_size=location;
}

void OutDataByte(unsigned char value)
{
    int index;
    SetPendingLine();
    CheckLEDataSize(1,1);
    index=CurrSeg->location-CurrSeg->obj->start+CurrSeg->data_prefix_size;
    IncLocation(1);
    if (index>=Out->used) {
        NeedMore(Out,index-Out->used+1);
        Out->used=index+1;
    }
    SetMaxWritten();
    OutBuff[index]=value;
}

#pragma pack(1)
struct line_num_entry { unsigned short line;unsigned long offset; };
#pragma pack()
void SetPendingLine(void)
{
    struct object *obj=CurrSeg->obj;
    struct line_num_entry *previous;
    unsigned line=obj->pending_line_number;
    if (line==0) return;
    obj->pending_line_number=0;
    obj->pending_label_line=0;
    if (obj->lines==NULL || obj->lines->used==0) {
        InitLineInfo();
        AddLineInfo((unsigned short)line,obj,CurrSeg->location);
        return;
    }
    previous=(struct line_num_entry *)((unsigned char *)obj->lines->array+obj->lines->used)-1;
    if (line==previous->line) return;
    if (CurrSeg->location>previous->offset) {
        AddLineInfo((unsigned short)line,obj,CurrSeg->location);
    } else {
        previous->line=(unsigned short)line;
    }
}

void OutLEDataStart(unsigned char iterated)
{
    unsigned char flags;
    struct index_rec *rec=CurrSeg;
    if (Out->used==0) {
        if (rec->comdat_label!=NULL) {
            flags=0;
            if (rec->location!=0) flags|=1;
            if (iterated) flags|=2;
            if (!(FEAttr(rec->comdat_symbol)&4)) flags|=4;
            OutByte(flags);
            OutByte(0x10);
            OutByte(0);
            OutOffset(rec->location);
            OutIdx(0,Out);
            OutIdx(rec->btype==1 ? CurrSeg->base:0,Out);
            OutIdx(rec->sidx,Out);
            OutIdx(NeedComdatNidx(0),Out);
        } else {
            OutIdx(rec->sidx,Out);
            OutOffset(rec->location);
        }
        rec->obj->start=rec->location;
        rec->data_prefix_size=(unsigned char)Out->used;
    }
    if (rec->start_data_in_code) OutSelect(1);
}

void SetMaxWritten(void)
{
    if (CurrSeg->location>CurrSeg->max_written) CurrSeg->max_written=CurrSeg->location;
}

unsigned short AskBackSeg(void)
{
    return BackSeg;
}

void OutReloc(unsigned short segment,unsigned char fix_class,unsigned char relative)
{
    struct index_rec *rec;
    rec=AskSegIndex(segment);
    CheckLEDataSize(12,1);
    DoFix(rec->base,relative,rec->btype,fix_class,rec->sidx);
}

#pragma pack(1)
struct fixup { unsigned char locatof,fset,fixdat; };
#pragma pack()
void DoFix(unsigned index,unsigned char relative,unsigned char base,unsigned char fix_class,unsigned segment_index)
{
    struct fixup *cursor;
    struct object *obj;
    struct index_rec *rec;
    int where;
    unsigned char locat=relative ? 0x80:0xc0;
    if (fix_class==3 && CurrSeg->data_in_code) CurrSeg->data_ptr_in_code=1;
    if (fix_class==0xff) {
        fix_class=3;
    } else if (TargetModel&2) {
        switch (fix_class) {
        case 1:case 5:fix_class=5;break;
        case 3:fix_class=6;break;
        }
    } else {
        switch (fix_class) {
        case 1:fix_class=9;break;
        case 5:fix_class=13;break;
        case 3:fix_class=11;break;
        }
    }
    obj=CurrSeg->obj;
    NeedMore(&obj->fixes,sizeof(*cursor));
    cursor=(struct fixup *)((unsigned char *)obj->fixes.array+obj->fixes.used);
    where=CurrSeg->location-obj->start;
    cursor->locatof=locat+(fix_class<<2)+(where>>8);
    cursor->fset=(unsigned char)where;
    obj->fixes.used+=sizeof(*cursor);
    if (base!=2) {
        rec=AskIndexRec((unsigned short)segment_index);
        if (rec->exec && rec->comdat_symbol!=NULL) {
            index=GenImport(rec->comdat_symbol,0);
            base=2;
        }
    }
    if ((TargetModel&0x20) && !(TargetModel&2) && fix_class!=3) {
        cursor->fixdat=0x14+(base==1 ? 0:base);
        OutIdx(FlatGIndex,&obj->fixes);
        OutIdx(base==1 ? segment_index:index,&obj->fixes);
    } else if (base==1) {
        cursor->fixdat=0x14;
        OutIdx(index,&obj->fixes);
        OutIdx(segment_index,&obj->fixes);
    } else {
        cursor->fixdat=0x54+base;
        OutIdx(index,&obj->fixes);
    }
}

struct index_rec *AskIndexRec(unsigned short segment_index)
{
    struct index_rec *rec=SegInfo->array;
    int i=0;
    while (++i<=SegInfo->used) {
        if (rec->sidx==segment_index) break;
        ++rec;
    }
    return rec;
}

void OutDataLong(long value)
{
    OutDataInt(value);
    OutDataInt(value>>16);
}

void OutDataInt(int value)
{
    int index;
    SetPendingLine();
    CheckLEDataSize(2,1);
    index=CurrSeg->location-CurrSeg->obj->start+CurrSeg->data_prefix_size;
    IncLocation(2);
    if ((unsigned)(index+2)>(unsigned)Out->used) {
        NeedMore(Out,index-Out->used+2);
        Out->used=index+2;
    }
    SetMaxWritten();
    *(unsigned short *)(OutBuff+index)=(unsigned short)value;
}

unsigned short AskCodeSeg(void)
{
    return CodeSeg;
}

/* Earlier profile-data markers control immediate OMF source-line emission. */
void OutSourceLine(unsigned short line)
{
    if (GeneratingProfileCode) OutLineNum(line,0);
}

/* i86obj.c's procedure transition before the later pre-switch debug range. */
void TellObjNewProc(void *symbol)
{
    unsigned short old;
    int proc_segment;
    old=SetOP(CodeSeg);
    proc_segment=FESegID(symbol);
    if (CodeSeg!=proc_segment) {
        if (CurrSeg->obj!=NULL) {
            DoEmptyQueue();
            FlushObject();
        }
        CodeSeg=(unsigned short)proc_segment;
        SetOP(CodeSeg);
        CurrSeg->need_base_set=1;
        if (!CurrSeg->exec) Zoiks(88);
    }
    if (FEAttr(symbol)&0x200) {
        DoEmptyQueue();
        if ((Model&0x10000000) && CurrSeg->comdat_symbol!=NULL)
            DFSymRange(CurrSeg->comdat_symbol,CurrSeg->comdat_size);
        CurrSeg->comdat_symbol=symbol;
        CurrSeg->prefix_comdat_state=1;
    } else if (CurrSeg->comdat_symbol!=NULL) {
        DoEmptyQueue();
        SetUpObj(0);
        if ((Model&0x10000000) && CurrSeg->comdat_symbol!=NULL)
            DFSymRange(CurrSeg->comdat_symbol,CurrSeg->comdat_size);
        NormalData();
    }
    SetOP(old);
}

/* The signed segment_id test precedes the WORD lookup in the OSI body. */
unsigned char AskSegNear(int id)
{
    struct index_rec *rec;
    if (id<0) return 0;
    rec=AskSegIndex((unsigned short)id);
    if (rec->btype!=1) return 0;
    return rec->base>CodeGroupGIdx;
}

unsigned char AskNameCode(void *symbol,unsigned char kind)
{
    switch (kind) {
    case CG_FE: return (FEAttr(symbol)&1)!=0;
    case CG_TBL:case CG_VTB:case CG_CLB: return 1;
    }
    return 0;
}

/* The old AskNameROM inlines AskSegROM's sign check, then shares the
 * segment-ROM query body at365f0. Preserve its actual two callees. */
unsigned char AskNameROM(void *symbol,unsigned char kind)
{
    int id;
    id=AskSegID(symbol,kind);
    if (id<0) return 0;
    return AskSegIndex((unsigned short)id)->rom;
}

unsigned char InlineFunction(void *symbol);
int AskSegID(void *symbol,unsigned char kind)
{
    switch (kind) {
    case CG_FE:
        if (InlineFunction(symbol)) return AskCodeSeg();
        return FESegID(symbol);
    case CG_BACK: return ((struct bck_info *)symbol)->segment;
    case CG_TBL: case CG_VTB: case CG_CLB:
        /* The 1994 CLB branch shares AskCodeSeg; the later source has
         * a separate AskAltCodeSeg there. */
        return AskCodeSeg();
    default: return AskBackSeg();
    }
}
unsigned char InlineFunction(void *symbol)
{
    void *aux;
    if (!(FEAttr(symbol)&1)) return 0;
    aux=FEAuxInfo(symbol,0); /* AUX_LOOKUP */
    if (FEAuxInfo(aux,1)!=NULL) return 1; /* CALL_BYTES */
    return *(unsigned *)FEAuxInfo(aux,2)&8; /* CALL_CLASS / MAKE_CALL_INLINE */
}

unsigned char AskSegPrivate(int segment)
{
    struct index_rec *record;
    if (segment<0) return 1;
    record=AskSegIndex((unsigned short)segment);
    return record->private || record->exec;
}

/* The old debug segment visitor returns a continuation flag; the later
 * released ObjFini inlines this scan with a void DoSegARange callback. */
int DoSegARange(void *context,struct index_rec *record)
{
    (void)context;(void)record;STUB_TRAP();return 0;
}
int EnumObjSegments(int (*visit)(void *,struct index_rec *),void *context)
{
    (void)visit;(void)context;STUB_TRAP();return 0;
}
void FiniWVTypes(void) { STUB_TRAP(); }
void FiniTarg(void)
{
    struct index_rec *record;
    struct object *object;
    unsigned long size;
    unsigned char attr;
    FlushObject();
    object=CurrSeg->obj;
    if (object->exports!=NULL) {
        KillArray(object->exports);
        object->exports=NULL;
    }
    record=AskIndexRec(object->index);
    size=record->max_size;
    if (record->exec) CodeSize+=record->max_size+record->total_comdat_size;
    else if (record->cidx==3) DataSize+=record->max_size+record->total_comdat_size;
    if (record->big) {
        attr=record->attr|2;               /* SEG_BIG */
        PatchObj(object->segfix,0,&attr,1); /* SEGDEF_ATTR */
    } else {
        PatchObj(object->segfix,1,(unsigned char *)&size,4); /* SEGDEF_SIZE */
    }
    if (object->exports!=NULL) KillArray(object->exports);
    KillStatic(&object->data);
    KillStatic(&object->fixes);
    CGFreeSize(object,sizeof(*object));
}
void FiniAbsPatches(void)
{
    struct abspatch *patch,*old;
    patch=AbsPatches;
    while (patch!=NULL) {
        DoPatch(&patch->patch,patch->value);
        old=patch;
        patch=patch->link;
        CGFreeSize(old,sizeof(*old));
    }
}
void EndModule(void)
{
    unsigned char flags;
    flags=0;                             /* Non-main; no start address. */
    /* Use16-bit MODEND even for386 records: the old writer avoids the
     * documented LINK386 MODEND386 incompatibility. */
    PutObjRec(0x8a,&flags,1);
}
void GenComdef(void)
{
    array_control *comdef;
    unsigned count;
    unsigned char type,indicator,command;
    unsigned long size;
    void *symbol;
    if (CurrSeg->comdat_label==NULL || CurrSeg->max_written>=CurrSeg->comdat_size)
        return;
    if (CurrSeg->max_written!=0) Zoiks(80);
    EjectImports();
    size=CurrSeg->comdat_size-CurrSeg->max_written;
    comdef=InitArray(1,256,50);
    symbol=CurrSeg->comdat_symbol;
    OutObjectName(symbol,comdef);
    NeedMore(comdef,4);
    ((unsigned char *)comdef->array)[comdef->used++]=0; /* Type index */
    type=CurrSeg->btype==1 && CurrSeg->base==DGroupIndex ? 0x62:0x61;
    ((unsigned char *)comdef->array)[comdef->used++]=type;
    if (type==0x61) ((unsigned char *)comdef->array)[comdef->used++]=1;
    /* Keep128 out of the single-byte encoding, as in the original writer. */
    if (size<0x80) { count=1;indicator=0; }
    else if (size<0x1000) { count=2;indicator=0x81; }
    else if (size<0x100000) { count=3;indicator=0x84; }
    else { count=4;indicator=0x88; }
    if (indicator!=0) ((unsigned char *)comdef->array)[comdef->used++]=indicator;
    NeedMore(comdef,count);
    do {
        ((unsigned char *)comdef->array)[comdef->used++]=(unsigned char)size;
        size>>=8;
    } while (--count!=0);
    command=(FEAttr(symbol)&4) ? 0xb0:0xb8;
    PutObjRec(command,comdef->array,comdef->used);
    KillArray(comdef);
    TellImportHandle(symbol,ImportHdl++);
}
void FlushLineNum(void) { STUB_TRAP(); }
unsigned char FreeObjCache(void)
{
    struct lname_cache *next;
    if (NameCache==NULL) return 0;
    FlushNames();
    while (NameCache!=NULL) {
        next=NameCache->next;
        CGFreeSize(NameCache,sizeof(*NameCache)+NameCache->name[0]);
        NameCache=next;
    }
    NameCacheDumped=NULL;
    return 1;
}

void FlushObject(void)
{
    struct object *object;
    SetUpObj(0);
    GenComdef();
    CurrSeg->total_comdat_size+=CurrSeg->comdat_size;
    EjectLEData();
    EjectExports();
    object=CurrSeg->obj;
    if (object->lines!=NULL) {
        FlushLineNum();
        KillArray(object->lines);
        object->lines=NULL;
    }
    FreeObjCache();
}

void ObjFini(void)
{
    struct index_rec *record;
    unsigned long code_size;
    void *auto_import;
    char *library;
    int i;
    if (Model&0x10000000) {
        code_size=0;
        EnumObjSegments(DoSegARange,&code_size);
        DFObjFiniDbgInfo(code_size);
    } else if (Model&0x08000000) CVObjFiniDbgInfo();
    else if (Model&0x01000000) FiniWVTypes();
    i=0;
    record=SegInfo->array;
    while (++i<=SegInfo->used) {
        if (record->obj!=NULL) {
            CurrSeg=record;
            FiniTarg();
        }
        ++record;
    }
    auto_import=NULL;
    if (Imports==NULL) Imports=InitArray(1,20,20);
    if (Used87) FEAuxInfo(NULL,38);         /* USED_8087 */
    for (;;) {
        auto_import=FEAuxInfo(auto_import,35); /* NEXT_IMPORT */
        if (auto_import==NULL) break;
        OutName(FEAuxInfo(auto_import,33),Imports); /* IMPORT_NAME */
        OutIdx(0,Imports);
        if (Imports->used>=192) {
            PutObjRec(0x8c,Imports->array,Imports->used);
            Imports->used=0;
        }
    }
    if (Imports->used!=0) {
        PutObjRec(0x8c,Imports->array,Imports->used);
        Imports->used=0;
    }
    library=NULL;
    for (;;) {
        library=FEAuxInfo(library,36);      /* NEXT_LIBRARY */
        if (library==NULL) break;
        NeedMore(Imports,2);
        *(unsigned short *)((unsigned char *)Imports->array+Imports->used)=0x9f80;
        Imports->used+=2;
        OutString((char *)FEAuxInfo(library,34)+1,Imports); /* LIBRARY_NAME */
        PutObjRec(0x88,Imports->array,Imports->used);
        Imports->used=0;
    }
    KillArray(Imports);
    Imports=NULL;
    KillArray(SegInfo);
    FiniAbsPatches();
    EndModule();
    CloseObj();
    FEMessage(1,(void *)CodeSize);
    FEMessage(2,(void *)DataSize);
}

void OutRTImport(int symbol,unsigned char fix_class) { STUB_TRAP(); }

void OutSpecialCommon(int symbol,unsigned char fix_class,unsigned char relative)
{
    CheckLEDataSize(12,1);
    DoFix((unsigned)symbol,relative,2,fix_class,0); /* BASE_IMP */
}

void CheckImportSwitch(unsigned char next_static)
{
    if ((GenStaticImports!=0)!=(next_static!=0)) EjectImports();
    GenStaticImports=next_static;
}
void GetSymLName(char *name,void *context)
{
    (void)name;(void)context;STUB_TRAP();
}
void DumpImportResolve(void *symbol,unsigned handle)
{
    void *default_symbol,*condition;
    unsigned default_handle,name_index;
    array_control *comment;
    int type;
    default_symbol=FEAuxInfo(symbol,17);    /* DEFAULT_IMPORT_RESOLVE */
    if (default_symbol==NULL || default_symbol==symbol) return;
    default_handle=GenImport(default_symbol,0);
    EjectImports();
    comment=InitArray(1,50,50);
    type=(int)FEAuxInfo(symbol,43);         /* IMPORT_TYPE */
    switch (type) {
    case 1:                              /* IMPORT_IS_LAZY */
        NeedMore(comment,2);
        *(unsigned short *)((unsigned char *)comment->array+comment->used)=0xa980;
        comment->used+=2;
        OutIdx(handle,comment);
        OutIdx(default_handle,comment);
        break;
    case 0:                              /* IMPORT_IS_WEAK */
        NeedMore(comment,2);
        *(unsigned short *)((unsigned char *)comment->array+comment->used)=0xa880;
        comment->used+=2;
        OutIdx(handle,comment);
        OutIdx(default_handle,comment);
        break;
    case 3:                              /* IMPORT_IS_CONDITIONAL_PURE */
        NeedMore(comment,2);
        *(unsigned short *)((unsigned char *)comment->array+comment->used)=0xa880;
        comment->used+=2;
        OutIdx(handle,comment);
        OutIdx(default_handle,comment);
        PutObjRec(0x88,comment->array,comment->used);
        comment->used=0;
        /* fall through */
    case 2:                              /* IMPORT_IS_CONDITIONAL */
        NeedMore(comment,2);
        *(unsigned short *)((unsigned char *)comment->array+comment->used)=0xfe80;
        comment->used+=2;
        NeedMore(comment,1);
        ((unsigned char *)comment->array)[comment->used++]=type==2 ? 0x56:0x50;
        OutIdx(handle,comment);
        OutIdx(default_handle,comment);
        condition=FEAuxInfo(symbol,44);    /* CONDITIONAL_IMPORT */
        while (condition!=NULL) {
            symbol=FEAuxInfo(condition,46); /* CONDITIONAL_SYMBOL */
            DoOutObjectName(symbol,GetSymLName,&name_index,0);
            OutIdx(name_index,comment);
            condition=FEAuxInfo(condition,45); /* NEXT_CONDITIONAL */
        }
        FlushNames();
        break;
    }
    PutObjRec(0x88,comment->array,comment->used);
    KillArray(comment);
}
unsigned GenImport(void *symbol,unsigned char type)
{
    unsigned handle;
    handle=AskImportHandle(symbol);
    if (handle==0 || type!=0) {
        handle=ImportHdl++;
        if (Imports==NULL) Imports=InitArray(1,256,50);
        CheckImportSwitch(!(FEAttr(symbol)&4)); /* !FE_GLOBAL */
        if (type==0) TellImportHandle(symbol,handle);
        /* The older NORMAL/SPECIAL path has no later DLLIMPORT/TLS rewrite. */
        DoOutObjectName(symbol,OutName,Imports,type);
        OutIdx(0,Imports);
        if (type==0) DumpImportResolve(symbol,handle);
    }
    return handle;
}

void OutImport(void *symbol,unsigned char fix_class,unsigned char relative)
{
    void *aux;
    unsigned *call_class;
    /* The 1994 body queries attributes only for an absolute offset and
     * predates the later FE_THREAD_DATA fixup adjustment. Its FAR16 class
     * is the actual byte sentinel255, rather than the later OR mask. */
    if (!relative && fix_class==1 && (FEAttr(symbol)&1)) {
        aux=FEAuxInfo(symbol,0);           /* AUX_LOOKUP */
        call_class=FEAuxInfo(aux,2);       /* CALL_CLASS */
        if (*call_class&0x40000000UL) fix_class=255;
    }
    OutSpecialCommon((int)GenImport(symbol,0),fix_class,relative);
}

unsigned long AskLocation(void)
{
    return CurrSeg->location;
}

unsigned SavePendingLine(unsigned value)
{
    unsigned old;
    old=CurrSeg->obj->pending_line_number;
    CurrSeg->obj->pending_line_number=value;
    return old;
}

unsigned char NeedBaseSet(void) { STUB_TRAP();return 0; }

void OutAbsPatch(void *patch,unsigned char size) { STUB_TRAP(); }

void OutFPPatch(unsigned char kind) { STUB_TRAP(); }

void OutPatch(struct code_lbl *label,unsigned char attributes)
{
    struct temp_patch *patch;
    struct object *object;
    CheckLEDataSize(12,1);
    patch=CGAlloc(sizeof(*patch));
    object=CurrSeg->obj;
    patch->link=object->patches;
    patch->label=label;
    patch->patch.ref=0;
    patch->patch.where=CurrSeg->location-object->start+CurrSeg->data_prefix_size;
    patch->patch.attr=attributes;
    object->patches=patch;
}

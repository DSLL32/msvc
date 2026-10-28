#ifndef WCC_OBJEMIT_H
#define WCC_OBJEMIT_H
#include <stddef.h>
void DoFix(unsigned index,unsigned char relative,unsigned char base,unsigned char fix_class,unsigned segment_index);
void SetPendingLine(void);
void OutSourceLine(unsigned short line);
void OutLineNum(unsigned short line,unsigned char label_line);
void SetMaxWritten(void);
#include <recon.h>
typedef struct array_control {
    void *array;
    int alloc,used,entry,inc;
} array_control;
array_control *InitArray(int entry,int initial,int increment);
void FillArray(array_control *array,int size,int initial,int increment);
void NeedMore(array_control *array,int count);
void KillArray(array_control *array);
void KillStatic(array_control *array);
void OutName(char *name,void *context);
void OutString(char *name,array_control *array);
void OutModel(array_control *array);
unsigned char GetMemModel(void);
void OutIdx(int index,array_control *array);
void PutObjRec(unsigned char command,void *bytes,unsigned length);
void PatchObj(unsigned long reference,unsigned offset,unsigned char *buffer,int length);
void GetFromObj(unsigned long reference,unsigned offset,unsigned char *buffer,int length);
unsigned char DoSum(unsigned char *bytes,int length);
void SeekStream(int handle,unsigned long offset);
void PutStream(int handle,void *bytes,unsigned length);
void OpenObj(void);
void CloseObj(void);
void InitFPPatches(void);
void DoSegGrpNames(array_control *group);
unsigned GetNameIdx(char *name,char *suffix,unsigned char required);
void DFDefSegs(void);
void CVDefSegs(void);
short DbgSegDef(char *name,char *class_name,unsigned char modifier);
void FlushNames(void);
void EjectLEData(void);
void CheckLEDataSize(unsigned maximum,unsigned char need_init);
void OutSelect(unsigned char first);
void OutLEDataStart(unsigned char iterated);
void FlushObject(void);
unsigned NeedComdatNidx(unsigned char type);
void IncLocation(unsigned long size);
void SetLocation(unsigned long location);
struct object;
void InitLineInfo(void);
void AddLineInfo(unsigned short line,struct object *obj,unsigned long location);
struct temp_patch;
struct code_lbl;
#pragma pack(4)
struct index_rec {
    struct object *obj;
    unsigned long location,max_written,max_size;
    unsigned short base,sidx,nidx,cidx;
    unsigned big:1,need_base_set:1,rom:1,data_in_code:1,data_ptr_in_code:1;
    unsigned private:1,exec:1,start_data_in_code:1,prefix_comdat_state:2;
    unsigned :22; /* Watcom reserves the complete DWORD allocation unit. */
    unsigned short seg;
    unsigned char btype,attr,data_prefix_size;
    unsigned long comdat_size,total_comdat_size;
    void *comdat_symbol;
    struct code_lbl *comdat_label;
    unsigned short comdat_nidx;
    void *virt_func_refs;
    unsigned comdat_prefix_import;
};
#pragma pack()
typedef char old_index_size[(sizeof(struct index_rec)==64)?1:-1];
typedef char old_index_flags[(offsetof(struct index_rec,seg)==28)?1:-1];
typedef char old_index_attr[(offsetof(struct index_rec,attr)==31)?1:-1];
typedef char old_index_prefix[(offsetof(struct index_rec,data_prefix_size)==32)?1:-1];
typedef char old_index_label[(offsetof(struct index_rec,comdat_label)==48)?1:-1];
#pragma pack(4)
struct object {
    array_control data,fixes;
    struct temp_patch *patches;
    array_control *exports,*lines;
    unsigned long start,segfix;
    unsigned short index,sofar;
    unsigned gen_static_exports:1,lines_generated:1,line_info:1,pending_label_line:1;
    unsigned :28;
    unsigned pending_line_number;
};
#pragma pack()
typedef char old_object_size[(sizeof(struct object)==72)?1:-1];
typedef char old_object_index[(offsetof(struct object,index)==60)?1:-1];
#pragma pack(4)
struct patch { unsigned long where,ref;unsigned char attr; };
struct temp_patch { struct patch patch;struct temp_patch *link;struct code_lbl *label; };
struct abspatch {
    struct patch patch;
    struct object *obj;
    struct abspatch *link;
    unsigned long value;
    unsigned char flags;
};
struct virt_func_ref_list { struct virt_func_ref_list *next;void *cookie; };
#pragma pack()
typedef char old_patch_size[(sizeof(struct patch)==12)?1:-1];
typedef char old_temp_patch_size[(sizeof(struct temp_patch)==20)?1:-1];
typedef char old_abspatch_size[(sizeof(struct abspatch)==28)?1:-1];
typedef char old_abspatch_flags[(offsetof(struct abspatch,flags)==24)?1:-1];
void ComdatData(struct code_lbl *label);
void NormalData(void);
unsigned char UseImportForm(unsigned short attributes);
void OutExport(void *symbol);
void EjectExports(void);
void OutObjectName(void *symbol,array_control *array);
void DoOutObjectName(void *symbol,void (*output)(char *,void *),void *context,unsigned char type);
void OutVirtFuncRef(void *symbol);
void TellCommonLabel(struct code_lbl *label,unsigned import);
void TellAddress(struct code_lbl *label,unsigned long address);
array_control *AskLblPatch(struct code_lbl *label);
array_control *InitPatch(void);
void DoPatch(struct patch *patch,unsigned long value);
void TellDonePatch(struct code_lbl *label);
void OutByte(unsigned char value);
void OutOffset(unsigned long value);
void OutInt(unsigned value);
unsigned char PickOMF(unsigned char command);
unsigned long AskObjHandle(void);
unsigned PadObj(unsigned length);
unsigned long Offset(unsigned long byte_offset);
void FEMessage(unsigned message,void *data);
struct index_rec *AskSegIndex(unsigned short segment);
struct index_rec *AskIndexRec(unsigned short segment_index);
unsigned GenImport(void *symbol,unsigned char type);
unsigned AskImportHandle(void *symbol);
void TellImportHandle(void *symbol,unsigned handle);
void SegmentClass(struct index_rec *record);
unsigned char SegmentAttr(unsigned char alignment,unsigned char flags,unsigned char use16);
void DoASegDef(struct index_rec *record,unsigned char use16);
void OutCGroup(unsigned segment);
void OutGroup(unsigned segment,array_control *group);
char *FEModuleName(void);
void DFObjInitInfo(void);
void CVObjInitInfo(void);
void WVObjInitInfo(void);
#endif

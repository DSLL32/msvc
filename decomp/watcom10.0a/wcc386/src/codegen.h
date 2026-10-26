#ifndef WCC_CODEGEN_H
#define WCC_CODEGEN_H
#include <stddef.h>
#include "symbols.h"
#include <recon.h>
typedef unsigned short cg_type;
enum { CG_FE,CG_LBL,CG_BACK,CG_TBL,CG_VTB,CG_CLB };
typedef struct tbl_control { struct code_lbl *label,*value_label;unsigned size;struct code_lbl *cases[1]; } tbl_control;
enum { U1,I1,U2,I2,U4,I4,CP,PT,FS,FD,XX,X8,ER };
/* The 1994 interface predates the 64-bit integer type codes. */
enum { T_UINT_1,T_INT_1,T_UINT_2,T_INT_2,T_UINT_4,T_INT_4,
       T_LONG_POINTER,T_HUGE_POINTER,T_NEAR_POINTER,T_LONG_CODE_PTR,T_NEAR_CODE_PTR,
       T_SINGLE,T_DOUBLE,T_UNKNOWN,T_DEFAULT,T_INTEGER,T_UNSIGNED,
       T_POINTER,T_CODE_PTR,T_BOOLEAN,T_PROC_PARM };
#define T_NEAR_INTEGER 0xfffaU
#define T_LONG_INTEGER 0xfffbU
#define T_HUGE_INTEGER 0xfffcU
typedef union cg_init_info {
    struct { unsigned revision:10,target:5,is_large:1; } version;
    int success;
} cg_init_info;
cg_init_info BEInit(unsigned gen_switches,unsigned target_switches,unsigned opt_size,unsigned short revision);
void BEMemInit(void);
void InitWeights(unsigned optsize);
void CGMemInit(void);
void InitSegDefs(void);
#pragma pack(4)
typedef struct type_def {
    int refno;
    long length;
    unsigned char attr;
} type_def;
#pragma pack()
typedef char old_type_def_size[(sizeof(type_def)==12)?1:-1];
extern type_def *TypeProcParm,*TypeInteger,*TypeHugeInteger,*TypeLongInteger;
extern type_def *TypeNearInteger,*TypeUnsigned,*TypeBoolean,*TypeNone,*TypePtr;
void InitTyping(void);
void TargTypeInit(void);
type_def *TypeAddress(cg_type type);
type_def *TypeAlias(cg_type alias,cg_type type);
extern type_def *PTInteger,*PTUnsigned,*PTCodePointer,*PTPointer;
void TypeInit(void);
void InitDbgInfo(void);
void TInit(void);
unsigned char TypeClass(type_def *type);
unsigned char ReturnClass(type_def *type,unsigned short attributes);
unsigned char MapIntReturn(cg_type type);
unsigned char MapPointer(cg_type type);
void Zoiks(int message);
unsigned char MapFloat(cg_type type,unsigned short attributes);
unsigned char MapStruct(unsigned long size,unsigned short attributes);
void InitFrl(void **list);
unsigned char CGOpenf(void);
int CreateStream(char *name);
void ObjError(int error);
struct obj_buffer;
struct obj_buffer *NewBuffer(void);
void cleanupLastBuffer(struct obj_buffer *buffer);
extern unsigned Model,TargetModel;
extern unsigned char OptForSize;
extern unsigned short CGProcessorVersion;
void BEStart(void);
unsigned char BGInInline(void);
void BGProcInline(void *symbol,type_def *type);
void TellObjNewProc(void *symbol);
void BGProcDecl(void *symbol,type_def *type);
void InitCG(void);
void InitFP(void);
void InitFPStkReq(void);
extern int Max87Stk;
extern unsigned char Used87;
void InitRegTbl(void);
void ScoreInit(void);
void InitQueue(void);
void InitMakeAddr(void);
void RegTreeInit(void);
void InitIns(void);
void InitConflict(void);
void InitRT(void);
void InitNames(void);
void ReInitNames(void);
type_def *ClassType(unsigned char class);
extern long TypeClassSize[XX+1];
void CFInit(void);
void ObjInit(void);

void InitSegment(void);
void BEStop(void);
void BEFini(void);
void BEDefType(cg_type type,unsigned alignment,unsigned long size);
type_def *TypeDefn(cg_type type,unsigned long size);
void BEDefSeg(unsigned short segment,unsigned char flags,char *name,unsigned alignment);
void DefSegment(int segment,unsigned char flags,char *name,unsigned alignment,unsigned char use16);
struct segdef;
void DoSegment(struct segdef *segment,void *group,unsigned char use16);
void DFBegCCU(unsigned short segment);
unsigned long BETypeLength(cg_type type);
void WrongCodeGen(void);
void SetSegs(void);
unsigned NewRefno(void);
void EmitSegLabels(void);
void *BENewBack(void *symbol);
#pragma pack(4)
struct label_def {
    struct code_lbl *link;
    void *patch,*symbol;
    unsigned long address;
    unsigned short status;
};
struct code_lbl {
    struct label_def label;
    struct code_lbl *alias;
    void *ins,*refs;
    struct code_lbl *redirect;
};
#pragma pack()
typedef char old_label_size[(sizeof(struct code_lbl)==36)?1:-1];
typedef char old_label_status[(offsetof(struct label_def,status)==16)?1:-1];
struct code_lbl *AskForLabel(void *symbol);
struct code_lbl *DoAskForLabel(void *symbol);
extern struct code_lbl *Handles;
unsigned FEAttr(void *symbol);
char *FEName(void *symbol);
unsigned short AskBackSeg(void);
unsigned short AskCodeSeg(void);
int FESegID(void *symbol);
unsigned short BESetSeg(unsigned short segment);
unsigned short SetOP(unsigned short segment);
void DGLabel(void *back);
void DGBlip(void);
void DataLabel(struct code_lbl *label);
void *AskForLblSym(struct code_lbl *label);
unsigned char ValidLbl(struct code_lbl *label);
void TellObjNewLabel(void *symbol);
void DoEmptyQueue(void);
void DFSymRange(void *symbol,unsigned long size);
void TellOptimizerByPassed(void);
void SetUpObj(unsigned char need_init);
void OutLabel(struct code_lbl *label);
void TellByPassOver(void);
unsigned short AskOP(void);
void EmitDBType(void);
void EmitSyms(void);
void EmitSym(SYMPTR sym,SYM_HANDLE handle);
unsigned long DBType(TYPEPTR type);
struct name_entry;
struct name_entry *DBBegName(char *name,unsigned long scope);
unsigned long DBEndName(struct name_entry *entry,unsigned long type);
int CGenType(TYPEPTR type);
int PtrType(TYPEPTR type,unsigned short flags);
int CodePtrType(unsigned short flags);
void AlignIt(TYPEPTR type);
void DGAlign(unsigned alignment);
void *FEBack(SYM_HANDLE handle);
void DGUBytes(unsigned long size);
void EmitZeros(unsigned long size);
unsigned long TypeLength(cg_type type);
void DGBytes(unsigned long length,unsigned char *data);
unsigned char Equal(unsigned char *a,unsigned char *b,int length);
void EmptyQueue(void);
void OutDataByte(unsigned char value);
void DataBytes(unsigned long length,unsigned char *data);
void DGInteger(unsigned long value,cg_type type);
void DGFloat(char *text,cg_type type);
struct cfloat;
union cg_float { float single;double real; };
struct cfloat *CFCnvLongToF(long value,unsigned char is_signed);
unsigned char CFSignedSize(struct cfloat *value,unsigned size);
unsigned char CFUnSignedSize(struct cfloat *value,unsigned size);
unsigned char CFIs32(struct cfloat *value);
long CFConvertByType(struct cfloat *value,type_def *type);
struct cfloat *CFAdd(struct cfloat *left,struct cfloat *right);
unsigned char CFIsType(struct cfloat *value,struct cfloat *limit);
unsigned char CFIsI16(struct cfloat *value);
unsigned char CFIsU16(struct cfloat *value);
short CFCnvF16(struct cfloat *value);
unsigned char CFIsU32(struct cfloat *value);
unsigned char CFIsI32(struct cfloat *value);
long CFGetDec32(char *text);
long CFCnvF32(struct cfloat *value);
void CFNegate(struct cfloat *value);
int CFTest(struct cfloat *value);
int CFAccess(struct cfloat *value,int index);
int CFOrder(struct cfloat *left,struct cfloat *right);
int CFCompare(struct cfloat *left,struct cfloat *right);
struct cfloat *IntToCF(long value,type_def *type);
long CFGetIntegerValue(struct cfloat *value);
struct cfloat *CnvCFToType(struct cfloat *value,type_def *type);
struct cfloat *CFTrunc(struct cfloat *value);
struct cfloat *CFRound(struct cfloat *value);
struct cfloat *CFCopy(struct cfloat *value);
unsigned long U32ModDiv(unsigned long *value,unsigned long divisor);
struct cfloat *CFAlloc(int size);
void *AllocFrl(void **head,int size);
void FrlFreeSize(void **head,void *block,int size);
void DoConvert(struct cfloat *number,char *text);
short CFGetDec16(char *text);
void CFClean(struct cfloat *number);
struct cfloat *CFCnvSF(char *begin,char *end);
void CFCnvTarget(struct cfloat *value,union cg_float *buffer,unsigned char type_class);
void CFFree(struct cfloat *value);
double CFToF(struct cfloat *value);
void __ZBuf2F(char *digits,double *result);
double _Scale10V(double value,int exponent);
#ifdef __WATCOMC__
/* Compiler runtime public has no C trailing underscore; retain its real ABI. */
#pragma aux __ZBuf2F "__ZBuf2F" parm [eax] [edx];
void BGParmDecl(void *symbol,type_def *type);
void BGParmInline(void *symbol,type_def *type);
#endif
void ftoa(double value,char *text);
struct leaf;
struct bck_info {
    unsigned imp;
    struct code_lbl *label;
    int segment;
};
void TellKeepLabel(struct code_lbl *label);
void TellNoSymbol(struct code_lbl *label);
void GenKillLabel(struct code_lbl *label);
void TryScrapLabel(struct code_lbl *label);
void CodeHandle(unsigned char kind,unsigned char length,struct code_lbl *label);
void OutReloc(unsigned short segment,unsigned char fix_class,unsigned char relative);
unsigned long AskAddress(struct code_lbl *label);
void OutPatch(struct code_lbl *label,unsigned char attributes);
void OutDataLong(long value);
void OutDataInt(int value);
void OutLblPatch(struct code_lbl *label,unsigned char fix_class,long offset);
void DoLblPtr(struct code_lbl *label,unsigned short segment,unsigned char fix_class,long offset);
void EmitLiteral(struct leaf *leaf);
void Emit1String(struct leaf *leaf,unsigned index);
int StringSegment(struct leaf *leaf);
void ReleaseString(struct leaf *leaf,unsigned index);
void DGBackPtr(struct bck_info *back,unsigned short segment,long offset,cg_type type);
void BackPtr(struct bck_info *back,unsigned short segment,long offset,type_def *type);
void EmitStrPtr(SYM_HANDLE string,cg_type type);
void DGFEPtr(void *symbol,cg_type type,long offset);
unsigned EmitBytes(SYM_HANDLE string);
void DBModSym(SYM_HANDLE handle,cg_type type);
void EmitCS_Strings(void);
int FListSrcQue(void);
void EmitDataQuads(void);
void FreeDataQuads(void);
struct local_vars;
struct local_vars *GenerateQuads(unsigned source_line,unsigned first,unsigned last,struct local_vars *locals,int inline_call);
void FreeCS_Strings(void);
void FiniSegLabels(void);
void FreeGblVars(SYM_HANDLE first);
void FreeExtVars(void);
void FreeTryTableBackHandles(void);
extern unsigned TryRefno;
struct try_table_back_handle;
extern struct try_table_back_handle *TryTableBackHandles;
extern int FunctionProfileSegment;
extern void *FunctionProfileBlock;
void BGParmDecl(void *symbol,type_def *type);
void BGParmInline(void *symbol,type_def *type);
#endif

#ifndef WCC_GENQUADS_H
#define WCC_GENQUADS_H
#include "codegen.h"
#include "quads.h"
#include <recon.h>
typedef void *cg_name;
typedef void *cg_label;
typedef unsigned char cg_op;
/* Values passed by the earlier front end's CG interface. */
enum {
 O_NOP=0,O_PLUS=1,O_MINUS=3,O_TIMES=5,O_DIV=7,O_MOD=8,
 O_AND=9,O_OR=10,O_XOR=11,O_RSHIFT=12,O_LSHIFT=13,
 O_POW=14,O_ATAN2=15,O_FMOD=16,O_UMINUS=17,O_COMPLEMENT=18,
 O_LOG=19,O_COS=20,O_SIN=21,O_TAN=22,O_SQRT=23,O_FABS=24,
 O_ACOS=25,O_ASIN=26,O_ATAN=27,O_COSH=28,O_EXP=29,O_LOG10=30,
 O_SINH=31,O_TANH=32,
 O_EQ=48,O_NE=49,O_GT=50,O_LE=51,O_LT=52,O_GE=53,
 O_FLOW_AND=76,O_FLOW_OR=77,O_POINTS=80,O_GOTO=81,
 O_IF_TRUE=83,O_IF_FALSE=84,O_LABEL=86,O_COMMA=92
};
#pragma pack(1)
struct local_vars { struct local_vars *next;SYM_HANDLE sym_list; };
#pragma pack()
typedef char old_local_vars_size[(sizeof(struct local_vars)==6)?1:-1];
extern unsigned GenQuadIndex,InlineQuadIndex,InlinedQuadCount;
extern SYM_HANDLE InlineFuncHandle,InlineReturnLeaf;
extern cg_label ReturnLabel,SavedReturnLabel;
unsigned EmitForLoop(QUAD *quad,unsigned index,void *top,void *exit);
cg_name GenSegmentOp(cg_name left,cg_name right);
cg_name ExceptCode(void);
void StartTry(unsigned scope);
void EndFinally(void);
void CallTryFini(void);
void EmitUnwind(unsigned scope);
void DefineLabels(unsigned first);
void GenAssignment(LEAFPTR left,LEAFPTR right,LEAFPTR result);
cg_name GenCast(LEAFPTR source,LEAFPTR result,cg_type type);
cg_name LeafAddress(LEAFPTR leaf);
cg_name LeafAddressEx(LEAFPTR leaf,LEAFPTR result,unsigned flags);
cg_name LeafValue(LEAFPTR leaf);
cg_name LeafValueEx(LEAFPTR leaf,LEAFPTR result,unsigned flags);
unsigned GenCall(LEAFPTR function,cg_type type,unsigned index,QUAD *quad,LEAFPTR result,int inline_call);
void GenJump(LEAFPTR label);
cg_type ComparisonType(LEAFPTR left,LEAFPTR right);
int DoFuncDefn(LEAFPTR function);
void EmitLocals(SYM_HANDLE first);
void EmitLocal(SYM_HANDLE handle);
void EmitLocalsReverse(SYM_HANDLE first);
void ParmReverse(SYM_HANDLE first);
void CGProcDecl(void *symbol,cg_type type);
void CGParmDecl(void *symbol,cg_type type);
void CGAutoDecl(void *symbol,cg_type type);
void DBLocalSym(void *symbol,cg_type type);
void CallTryInit(void);
struct local_vars * ReleaseVars(SYM_HANDLE first,struct local_vars *previous);
void RelLocalVars(struct local_vars *list);
void FreeTrySymBackInfo(void);
void FreeLocalVars(SYM_HANDLE first);
void GenSwitch(unsigned switch_leaf,LEAFPTR value,LEAFPTR result);
int LeafCGType(LEAFPTR leaf);
int LeafResultType(LEAFPTR leaf);
void LeafGetString(LEAFPTR leaf,unsigned index);
int FEParmType(void *function,void *parameter,int type);
void DBSrcCue(unsigned file,unsigned line,unsigned column);
void DBBegBlock(void);
void DBEndBlock(void);
cg_label BENewLabel(void);
void BEFiniLabel(cg_label label);
unsigned long BEUnrollCount(unsigned long count);
void BEFiniBack(void *back);
void BEFreeBack(void *back);
cg_label CGLastParm(void);
void CGReturn(cg_name value,cg_type type);
cg_name CGInteger(long value,cg_type type);
cg_name CGPostGets(cg_op op,cg_name left,cg_name right,cg_type type);
cg_name CGPreGets(cg_op op,cg_name left,cg_name right,cg_type type);
cg_name CGBinary(cg_op op,cg_name left,cg_name right,cg_type type);
cg_name CGUnary(cg_op op,cg_name value,cg_type type);
cg_name CGCompare(cg_op op,cg_name left,cg_name right,cg_type type);
cg_name CGFlow(cg_op op,cg_name left,cg_name right);
cg_name CGChoose(cg_name condition,cg_name first,cg_name second,cg_type type);
void CGControl(cg_op op,cg_name condition,cg_label label);
void CGBigLabel(void *back);
void CGTrash(cg_name value);
void CGDone(cg_name value);
void *CGTemp(cg_type type);
struct try_table_back_handle {
    struct try_table_back_handle *next;
    void *try_table_back_handle;
};
void SetTryTable(void *table);
cg_name CGFloat(char *value,cg_type type);
cg_name CGBackName(void *back,cg_type type);
cg_name CGVolatile(cg_name value);
cg_name GenPointerConversion(cg_name value,cg_op op);
cg_name GenBasedPointer(SYM_HANDLE symbol,cg_name value);
cg_name CGFEName(void *symbol,cg_type type);
cg_name CGTempName(void *temp,cg_type type);
cg_name CGBitMask(cg_name value,unsigned char start,unsigned char width,cg_type type);
int SymbolPointerType(LEAFPTR leaf,SYMPTR symbol);
cg_name ForceVolatileFloat(cg_name value,TYPEPTR type);
cg_name ConvertFromBased(cg_name value,cg_type type);
cg_name ConvertToBased(cg_name value,LEAFPTR result,unsigned flags);
cg_name FarPointerSegmentAddress(cg_name value);
#endif

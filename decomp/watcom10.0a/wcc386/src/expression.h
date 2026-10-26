#ifndef WCC_EXPRESSION_H
#define WCC_EXPRESSION_H
#include "types.h"
#include "quads.h"
#include <recon.h>
/* Actual old precedence classes: ternary-done precedes TC_START1. */
enum {
 TC_NEAR=1,TC_FAR=2,TC_HUGE=3,TC_INTERRUPT=4,TC_CDECL=5,
 TC_PASCAL=6,TC_FORTRAN=7,TC_SYSCALL=8,TC_EXPORT=9,TC_LOADDS=10,
 TC_SAVEREGS=11,TC_BASED=12,TC_SEGMENT=13,TC_SEGNAME=14,TC_SELF=15,
 TC_KEYWORD=16,TC_QUALIFIER=17,TC_STG_CLASS=18,
 TC_XX=20,TC_RIGHT_BRACKET=21,TC_INDEX=22,TC_RIGHT_PAREN=23,
 TC_PARM_LIST=24,TC_LEFT_PAREN=25,TC_START=26,TC_TERNARY=27,
 TC_COLON=28,TC_COMMA=29,TC_TERNARY_DONE=30,TC_START1=31,
 TC_ASSIGNMENT=32,TC_ASSIGN_OP=33,TC_START2=34,TC_QUESTION=35,
 TC_OR_OR=36,TC_AND_AND=37,TC_OR=38,TC_XOR=39,TC_AND=40,
 TC_EQ_NE=41,TC_REL_OP=42,TC_SHIFT_OP=43,TC_ADD_OP=44,TC_MUL_OP=45,
 TC_START_UNARY=47,TC_PREINC=48,TC_ADDR=49,TC_EXCLAMATION=50,
 TC_PLUS=51,TC_MINUS=52,TC_TILDE=53,TC_SIZEOF=54,TC_INDIRECTION=55,
 TC_CAST=56,TC_POSTINC=58,TC_LEFT_BRACKET=59,TC_SEG_OP=60,
 TC_FUNC_CALL=61,TC_DOT=62,TC_ARROW=63
};
extern int Level,DeadCode;
extern unsigned char Class[256],Token[256],TokenClass[101];
extern unsigned ValueStack[256];
unsigned OrOr(unsigned leaf);
unsigned AndAnd(unsigned leaf);
unsigned Expr(void);
unsigned SingleExpr(void);
unsigned GetExpr(void);
int BoolConstExpr(void);
TYPEPTR TypeName(void);
unsigned ExprId(void);
int IsMacroDefined(void);
unsigned SymLeaf(void);
struct enum_info;
unsigned EnumLeaf(struct enum_info *info);
void IncSymWeight(SYMPTR sym);
unsigned ConstLeaf(void);
unsigned StringLeaf(unsigned flags);
unsigned ExprOpnd(void);
unsigned VoidRValue(unsigned leaf);
TYPEPTR LeafType(LEAFPTR leaf);
TYPEPTR TypeOf(unsigned leaf);
unsigned TypeLeaf(TYPEPTR type);
/* Descriptive names for the earlier quad folder, absent from the later tree IR. */
int CanFoldQuad(int op);
int ConstantKind(LEAFPTR leaf);
void ConstantToDouble(LEAFPTR leaf,int kind);
void CheckConstantTruncation(unsigned first,unsigned second);
void NormalizeConstant(LEAFPTR leaf,int kind);
int FoldDoubleQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result);
int FoldUnsignedQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result);
int FoldSignedQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result);
int FoldQuad(unsigned first,int op,unsigned second,unsigned type_leaf);
unsigned GetQuadOperand(unsigned leaf);
unsigned GenQuad(unsigned first,int op,unsigned second,unsigned type_leaf);
void ChkConst(unsigned leaf);
unsigned AsgnOp(unsigned first,unsigned char op,unsigned second);
int LValue(unsigned leaf);
int CompatibleType(TYPEPTR first,TYPEPTR second);
int DoCompatibleType(TYPEPTR first,TYPEPTR second,int top_level);
int InUnion(TYPEPTR first,TYPEPTR second,int reversed);
int CompatibleStructs(struct tag_entry *first,struct tag_entry *second);
int CompatibleFunction(TYPEPTR first,TYPEPTR second);
int CallInTailPosition(QUAD *quad,unsigned index);
void CompareParms(TYPEPTR *master,unsigned *actual,unsigned line);
void AsgnCheck(TYPEPTR type,unsigned leaf);
unsigned TernOp(unsigned condition,unsigned first,unsigned second);
TYPEPTR TernType(unsigned first,unsigned second);
TYPEPTR MergedType(TYPEPTR first,TYPEPTR second);
int CompatibleTernaryPointers(TYPEPTR first,TYPEPTR second);
unsigned OptimizeTernary(unsigned condition,unsigned first,unsigned second,TYPEPTR typ);
int SameTernaryOperand(LEAFPTR first,LEAFPTR second);
unsigned BoolExpr(unsigned leaf);
DATA_TYPE DataTypeOf(DATA_TYPE type);
unsigned FlowOp(unsigned first,unsigned char op,unsigned second);
unsigned IntOp(unsigned first,unsigned char op,unsigned second);
int CompatiblePointers(TYPEPTR first,TYPEPTR second);
int IsFunctionPointer(TYPEPTR typ);
int IsVoidType(TYPEPTR type);
void PointerTypeMismatch(void);
unsigned RelOp(unsigned first,unsigned char op,unsigned second);
unsigned ShiftOp(unsigned first,unsigned char op,unsigned second);
unsigned AddOp(unsigned first,unsigned char op,unsigned second);
unsigned ArrayAddConst(unsigned first,unsigned char op,unsigned second);
unsigned FoldArrayOffset(LEAFPTR first,unsigned first_index,int op,LEAFPTR second,unsigned second_index);
unsigned MulByConst(unsigned leaf,unsigned size);
unsigned PtrSubtract(unsigned leaf,unsigned size,DATA_TYPE type);
unsigned BinOp(unsigned first,unsigned char op,unsigned second);
unsigned IntLeaf(int value);
unsigned LongLeaf(long value);
unsigned ArrayIndex(unsigned first,unsigned second,TYPEPTR typ);
int CheckIndex(unsigned first,unsigned second,TYPEPTR typ);
unsigned GenIndex(unsigned first,unsigned second,TYPEPTR typ);
unsigned UIntLeaf(unsigned value);
unsigned AddrOp(unsigned leaf,int mode);
unsigned StringAddress(LEAFPTR leaf,unsigned index);
unsigned TakeRValue(unsigned leaf,int void_ok);
unsigned RValue(unsigned leaf);
unsigned NotOp(unsigned leaf);
unsigned UnaryPlus(unsigned leaf);
unsigned UMinus(unsigned leaf);
unsigned UComplement(unsigned leaf);
unsigned SizeofOp(TYPEPTR type);
unsigned PtrOp(unsigned leaf);
unsigned CnvOp(unsigned leaf,TYPEPTR type,int explicit_cast);
void CastConstValue(LEAFPTR leaf,unsigned index,TYPEPTR typ);
unsigned IndexOp(unsigned first,unsigned second);
unsigned SegOp(unsigned first,unsigned second);
unsigned GenNextParm(unsigned leaf,TYPEPTR ** plist);
unsigned GenFuncCall(unsigned leaf);
int IntrinsicMathFunc(char *name,int index,int length,SYMPTR sym);
unsigned MakeMathFunction(char *name,TYPEPTR typ);
void PopNestedParms(TYPEPTR ** plist);
unsigned StartTernary(unsigned leaf);
unsigned ColonOp(unsigned leaf);
struct nested_parm_lists {
    struct nested_parm_lists *prev_list;
    TYPEPTR *next_parm_type;
};
extern struct nested_parm_lists *NestedParms;
unsigned StartFunc(unsigned leaf,TYPEPTR ** plist,unsigned first_quad);
unsigned CallResultLeaf(TYPEPTR typ);
void MarkRecursiveCall(void);
void LinkCallQuad(void);
unsigned DotField(unsigned leaf,char *name,unsigned offset);
unsigned DotOp(unsigned leaf);
TYPEPTR Ptr2Struct(TYPEPTR type);
FIELDPTR SearchFields(TYPEPTR *type,unsigned *offset,char *name);
unsigned LoadLeafValue(unsigned leaf);
unsigned FieldOp(unsigned leaf,char *name,unsigned offset);
unsigned ArrowOp(unsigned leaf);
#endif

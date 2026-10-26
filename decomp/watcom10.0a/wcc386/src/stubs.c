#include "memory.h"
#include "compiler.h"
#include "expression.h"
#include "cmemory.h"
#include "types.h"
#include "macros.h"
#include "symbols.h"
#include "quads.h"
#include "command.h"
#include "stubs.h"
#include "options.h"
#include "scanner.h"
#include "files.h"
#include "preproc.h"
#include "declarations.h"
#include "data.h"
#include "cstring.h"
#include "statement.h"
#include "codegen.h"
#include "objemit.h"
#include "cgmemory.h"
#include "regsave.h"
#include "cgmemory.h"
#include "dbinfo.h"
#include "genquads.h"
#include "encode.h"
#include "procdef.h"
#include "block.h"
#include "address.h"
#include "genopt.h"
#include <recon.h>
int myatoi(char *text) { (void)text; STUB_TRAP(); return 0; }
void myitoa(int value,char *text) { (void)value; (void)text; STUB_TRAP(); }


void PurgeMemory(void) { STUB_TRAP(); }


void PragmaFini(void) { STUB_TRAP(); }
void FreeFNames(void) { STUB_TRAP(); }
void CErr1(int code) { (void)code; STUB_TRAP(); }
void AddIncList(char *text,unsigned length) { (void)text; (void)length; STUB_TRAP(); }
void PrtChar(int character) { (void)character; STUB_TRAP(); }

void PageOutQuads(void) { STUB_TRAP(); }
void PageOutLeafs(void) { STUB_TRAP(); }
void PageOutSyms(void) { STUB_TRAP(); }
void CSuicide(void) { STUB_TRAP(); }




int MacroCompare(MEPTR first,MEPTR second) { (void)first; (void)second; STUB_TRAP(); return 0; }
void CErr2p(int code,char *text) { (void)code; (void)text; STUB_TRAP(); }




void OpenPageFile(void) { STUB_TRAP(); }


void PageRead(char *buffer,unsigned count) { (void)buffer; (void)count; STUB_TRAP(); }
void PageWrite(char *buffer,unsigned count) { (void)buffer; (void)count; STUB_TRAP(); }
void PageSeek(unsigned long offset) { (void)offset; STUB_TRAP(); }




void MakeTmpName(char *name) { (void)name; STUB_TRAP(); }

void SetStackConventions(void) { STUB_TRAP(); }
char *DefineCmdLineMacro(char *text) { (void)text; STUB_TRAP(); return 0; }
char *ProcessFPUStack(char *text) { (void)text; STUB_TRAP(); return 0; }
char *ProcessMemoryModel(char *text) { (void)text; STUB_TRAP(); return 0; }
char *ProcessSegmentName(char *text) { (void)text; STUB_TRAP(); return 0; }
char *AddIncludePath(char *text) { (void)text; STUB_TRAP(); return 0; }
char *AddUndefName(char *text) { (void)text; STUB_TRAP(); return 0; }
char *SetDataThreshold(char *text) { (void)text; STUB_TRAP(); return 0; }
char *LoadUnicodeTable(char *text) { (void)text; STUB_TRAP(); return 0; }
char *BadCmdLine(int code,char *text) { (void)code; (void)text; STUB_TRAP(); return 0; }
int OptionDelimiter(int character) { (void)character; STUB_TRAP(); return 0; }
void SetCppWidth(int width) { (void)width; STUB_TRAP(); }
void SetSignedChar(void) { STUB_TRAP(); }
void SetDBChar(int kind) { (void)kind; STUB_TRAP(); }
int fndlen(char *text) { (void)text; STUB_TRAP(); return 0; }
int CharConst(DATA_TYPE type) { (void)type; STUB_TRAP(); return 0; }

void CppComment(int character) { (void)character; STUB_TRAP(); }
int ScanStringContinue(void) { STUB_TRAP(); return 0; }
int ScanCharConst(void) { STUB_TRAP(); return 0; }
int ScanInvalid(void) { STUB_TRAP(); return 0; }
int ScanMacroToken(void) { STUB_TRAP(); return 0; }


int SpecialMacro(MEPTR entry) { (void)entry; STUB_TRAP(); return 0; }
int BuildString(int length,char *tokens)
{ (void)length; (void)tokens; STUB_TRAP(); return 0; }
struct macro_token *Glue2Tokens(struct macro_token *first,struct macro_token *second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }
int MacroBeingExpanded(MEPTR mentry) { (void)mentry; STUB_TRAP(); return 0; }
int Expandable(struct macro_token *mtok,int macro_parm)
{ (void)mtok; (void)macro_parm; STUB_TRAP(); return 0; }

int tryBackSlashNewLine(void) { STUB_TRAP(); return 0; }

unsigned char translateTriGraph(unsigned char c) { (void)c; STUB_TRAP(); return 0; }

int getCharAfterTwoQuestion(void) { STUB_TRAP(); return 0; }
int tryTrigraphAgain(void) { STUB_TRAP(); return 0; }
void CWarn1(int level,int code) { (void)level; (void)code; STUB_TRAP(); }
int UsePreCompiledHeader(char *name) { (void)name; STUB_TRAP(); return 0; }
void CantOpenFile(char *name) { (void)name; STUB_TRAP(); }

void OpenCppFile(void) { STUB_TRAP(); }
void EmitPoundLine(int line,char *name) { (void)line; (void)name; STUB_TRAP(); }


void AsmError(int code) { (void)code; STUB_TRAP(); }

struct AsmCodeName;

void CPP_Parse(void) { STUB_TRAP(); }

void CWarn2(int level,int code,unsigned parameter)
{ (void)level;(void)code;(void)parameter;STUB_TRAP(); }

void DwarfBrowseEmit(void) { STUB_TRAP(); }




void ObjError(int error) { (void)error;STUB_TRAP(); }














void *SegPeggedReg(unsigned segment) { (void)segment;STUB_TRAP();return NULL; }

unsigned char FlushSomeOpt(unsigned size) { (void)size;STUB_TRAP();return 0; }
unsigned MemInUse(void) { STUB_TRAP();return 0; }
unsigned char MemCheck(int size) { (void)size;STUB_TRAP();return 0; }
void FatalError(char *message) { (void)message;STUB_TRAP(); }
void Suicide(void) { STUB_TRAP(); }
void AddCueBlk(cue_ctl *ctl) { (void)ctl;STUB_TRAP(); }
void DFInitDbgInfo(void) { STUB_TRAP(); }
void CVInitDbgInfo(void) { STUB_TRAP(); }




void FreeGblVars(SYM_HANDLE first) { (void)first;STUB_TRAP(); }

















void SeekStream(int handle,unsigned long offset) { (void)handle;(void)offset;STUB_TRAP(); }




void DFDefSegs(void) { STUB_TRAP(); }
void CVDefSegs(void) { STUB_TRAP(); }
short DbgSegDef(char *name,char *class_name,unsigned char modifier) { (void)name;(void)class_name;(void)modifier;STUB_TRAP();return 0; }


void DFObjInitInfo(void) { STUB_TRAP(); }
void CVObjInitInfo(void) { STUB_TRAP(); }

void BEStop(void) { STUB_TRAP(); }
void BEFini(void) { STUB_TRAP(); }
void WrongCodeGen(void) { STUB_TRAP(); }
void OutCGroup(unsigned segment) { (void)segment;STUB_TRAP(); }
char *FEModuleName(void) { STUB_TRAP();return NULL; }
void DFBegCCU(unsigned short segment) { (void)segment;STUB_TRAP(); }
unsigned long BETypeLength(cg_type type) { (void)type;STUB_TRAP();return 0; }
void EmitDBType(void) { STUB_TRAP(); }

void FiniSegLabels(void) { STUB_TRAP(); }
void FreeExtVars(void) { STUB_TRAP(); }
void FreeTryTableBackHandles(void) { STUB_TRAP(); }

void SymFini(void) { STUB_TRAP(); }




void BuildPreCompiledHeader(char *name) { (void)name; STUB_TRAP(); }
void __cdecl CErr(int code,...) { (void)code; STUB_TRAP(); }

void ScanComment(void) { STUB_TRAP(); }
void UnGetChar(int c) { (void)c; STUB_TRAP(); }

void CIfDef(void) { STUB_TRAP(); }
void ExpectIdentifier(void) { STUB_TRAP(); }
void ExpectMessage(int message) { (void)message; STUB_TRAP(); }
void CElif(void) { STUB_TRAP(); }
void CUndef(void) { STUB_TRAP(); }
void CLine(void) { STUB_TRAP(); }
void CError(void) { STUB_TRAP(); }
void CIdent(void) { STUB_TRAP(); }
void CUnknown(void) { STUB_TRAP(); }

int getCharAfterBackSlash(void) { STUB_TRAP(); return 0; }

int ESCChar(int c,int rtn) { (void)c; (void)rtn; STUB_TRAP(); return 0; }

void __cdecl CppPrtf(char *format,...) { (void)format; STUB_TRAP(); }

int IsMacroDefined(void) { STUB_TRAP(); return 0; }

unsigned JIS2Unicode(unsigned character) { (void)character; STUB_TRAP(); return 0; }





void ConstantToDouble(LEAFPTR leaf,int kind) { (void)leaf; (void)kind; STUB_TRAP(); }
int FoldDoubleQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result)
{ (void)first; (void)op; (void)second; (void)result; STUB_TRAP(); return 0; }
int FoldUnsignedQuad(LEAFPTR first,int op,LEAFPTR second,LEAFPTR result)
{ (void)first; (void)op; (void)second; (void)result; STUB_TRAP(); return 0; }




TYPEPTR MergedType(TYPEPTR first,TYPEPTR second) { (void)first;(void)second;STUB_TRAP();return NULL; }
int CompatibleTernaryPointers(TYPEPTR first,TYPEPTR second) { (void)first;(void)second;STUB_TRAP();return 0; }
int SameTernaryOperand(LEAFPTR first,LEAFPTR second)
{ (void)first;(void)second;STUB_TRAP();return 0; }




int IsVoidType(TYPEPTR type) { (void)type; STUB_TRAP(); return 0; }
void PointerTypeMismatch(void) { STUB_TRAP(); }






void __cdecl CWarn(int level,int code,...)
{ (void)level; (void)code; STUB_TRAP(); }

unsigned NotOp(unsigned leaf) { (void)leaf; STUB_TRAP(); return 0; }

unsigned UnaryPlus(unsigned leaf) { (void)leaf; STUB_TRAP(); return 0; }


unsigned UComplement(unsigned leaf) { (void)leaf; STUB_TRAP(); return 0; }






unsigned SegOp(unsigned first,unsigned second) { (void)first; (void)second; STUB_TRAP(); return 0; }











unsigned OrOr(unsigned leaf) { (void)leaf; STUB_TRAP(); return 0; }




void FlushBadCode(void) { STUB_TRAP(); }



void InvDecl(void) { STUB_TRAP(); }
void CmpFuncDecls(SYMPTR new_sym,SYMPTR old_sym) { (void)new_sym; (void)old_sym; STUB_TRAP(); }
void XferPragInfo(char *old_name,char *new_name) { (void)old_name; (void)new_name; STUB_TRAP(); }
int IdenticalType(TYPEPTR first,TYPEPTR second) { (void)first; (void)second; STUB_TRAP(); return 0; }
int VerifyType(TYPEPTR first,TYPEPTR second,SYMPTR sym) { (void)first; (void)second; (void)sym; STUB_TRAP(); return 0; }
SYM_HANDLE Sym0Look(int hash,char *name) { (void)hash; (void)name; STUB_TRAP(); return 0; }
void CErrSymName(int code,SYMPTR sym,SYM_HANDLE handle) { (void)code; (void)sym; (void)handle; STUB_TRAP(); }
void InitCharArray(TYPEPTR type) { (void)type; STUB_TRAP(); }
void InitWCharArray(TYPEPTR type) { (void)type; STUB_TRAP(); }
void ZeroBytes(unsigned long count) { (void)count; STUB_TRAP(); }

void SetFuncSegment(SYMPTR sym,int segment) { (void)sym; (void)segment; STUB_TRAP(); }

void Expecting(char *text) { (void)text; STUB_TRAP(); }

int AddSegName(char *name,char *class_name,int kind)
{ (void)name; (void)class_name; (void)kind; STUB_TRAP(); return 0; }
SYM_HANDLE SegSymHandle(int segment) { (void)segment; STUB_TRAP(); return 0; }







int CppPrinting(void) { STUB_TRAP(); return 0; }
int GetNextToken(void) { STUB_TRAP(); return 0; }
void PrtToken(void) { STUB_TRAP(); }
void PragFlag(int value) { (void)value; STUB_TRAP(); }
void PragAux(void) { STUB_TRAP(); }
void PragLibs(void) { STUB_TRAP(); }
void PragAllocText(void) { STUB_TRAP(); }
void PragCodeSeg(void) { STUB_TRAP(); }
void PragDataSeg(void) { STUB_TRAP(); }
void PragEnableDisableMessage(int enable) { (void)enable; STUB_TRAP(); }
void PragIntrinsic(int enable) { (void)enable; STUB_TRAP(); }
void PragUnroll(void) { STUB_TRAP(); }



struct textsegment *NewTextSeg(char *name,char *class_name,char *group)
{ (void)name; (void)class_name; (void)group; STUB_TRAP(); return 0; }






void SetErrLoc(char *file,unsigned line) { (void)file; (void)line; STUB_TRAP(); }

void ChkUseful(void) {  STUB_TRAP(); }








void LeaveStmt(void) {  STUB_TRAP(); }






void TryStmt(void) {  STUB_TRAP(); }



void ChkRetValue(void) {  STUB_TRAP(); }




void OpenDefFile(void) {  STUB_TRAP(); }

void DumpFuncDefn(void) {  STUB_TRAP(); }












void UnWindTry(int scope) { (void)scope; STUB_TRAP(); }






void ElseStmt(void) {  STUB_TRAP(); }






int EndTry(void) {  STUB_TRAP(); return 0; }













void SetSymLoc(SYMPTR sym) { (void)sym; STUB_TRAP(); }

void ChkProtoType(void) { STUB_TRAP(); }

unsigned PtrSubtract(unsigned leaf,unsigned size,DATA_TYPE type)
{ (void)leaf; (void)size; (void)type; STUB_TRAP(); return 0; }




int InUnion(TYPEPTR first,TYPEPTR second,int reversed)
{ (void)first; (void)second; (void)reversed; STUB_TRAP(); return 0; }
int CompatibleFunction(TYPEPTR first,TYPEPTR second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }

int SameSymbolLeaf(struct leaf *first,struct leaf *second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }

unsigned GenIndex(unsigned first,unsigned second,TYPEPTR typ)
{ (void)first; (void)second; (void)typ; STUB_TRAP(); return 0; }
unsigned LongLeaf(long value) { (void)value; STUB_TRAP(); return 0; }

void MarkRecursiveCall(void) { STUB_TRAP(); }



unsigned MakeMathFunction(char *name,TYPEPTR typ)
{ (void)name; (void)typ; STUB_TRAP(); return 0; }

struct inline_funcs *IF_Lookup(char *name) { (void)name;STUB_TRAP();return NULL; }







void FEMessage(unsigned message,void *data) { (void)message;(void)data;STUB_TRAP(); }
unsigned PadObj(unsigned length) { (void)length;STUB_TRAP();return 0; }

void BuffOffset(unsigned long value) { (void)value;STUB_TRAP(); }
void Zoiks(int message) { (void)message;STUB_TRAP(); }
void BuffEnd(unsigned short segment) { (void)segment;STUB_TRAP(); }
void ChkDbgSegSize(unsigned size,unsigned char need) { (void)size;(void)need;STUB_TRAP(); }


unsigned char MapStruct(unsigned long size,unsigned short attr) { (void)size;(void)attr;STUB_TRAP();return 0; }
void DBModSym(SYM_HANDLE handle,cg_type type) { (void)handle;(void)type;STUB_TRAP(); }

unsigned long DBType(TYPEPTR type) { (void)type;STUB_TRAP();return 0; }
struct name_entry *DBBegName(char *name,unsigned long scope) { (void)name;(void)scope;STUB_TRAP();return NULL; }
unsigned long DBEndName(struct name_entry *entry,unsigned long type) { (void)entry;(void)type;STUB_TRAP();return 0; }
void EmitZeros(unsigned long size) { (void)size;STUB_TRAP(); }
void DGAlign(unsigned alignment) { (void)alignment;STUB_TRAP(); }

void DoEmptyQueue(void) { STUB_TRAP(); }
void DFSymRange(void *symbol,unsigned long size) { (void)symbol;(void)size;STUB_TRAP(); }
void EjectLEData(void) { STUB_TRAP(); }
void OutSelect(unsigned char first) { (void)first;STUB_TRAP(); }

void ComdatData(struct code_lbl *label) { (void)label;STUB_TRAP(); }
void NormalData(void) { STUB_TRAP(); }
void OutVirtFuncRef(void *symbol) { (void)symbol;STUB_TRAP(); }
void TellCommonLabel(struct code_lbl *label,unsigned import) { (void)label;(void)import;STUB_TRAP(); }
void DoPatch(struct patch *patch,unsigned long value) { (void)patch;(void)value;STUB_TRAP(); }
void DumpCS_Strings(void) { STUB_TRAP(); }

void DGFEPtr(void *symbol,cg_type type,long offset) { (void)symbol;(void)type;(void)offset;STUB_TRAP(); }


unsigned char Equal(unsigned char *a,unsigned char *b,int length) { (void)a;(void)b;(void)length;STUB_TRAP();return 0; }
void EmptyQueue(void) { STUB_TRAP(); }


void InitLineInfo(void) { STUB_TRAP(); }
void AddLineInfo(unsigned short line,struct object *obj,unsigned long location) { (void)line;(void)obj;(void)location;STUB_TRAP(); }

unsigned NeedComdatNidx(unsigned char type) { (void)type;STUB_TRAP();return 0; }







unsigned GenImport(void *symbol,unsigned char type) { (void)symbol;(void)type;STUB_TRAP();return 0; }

void OutPatch(struct code_lbl *label,unsigned char attributes) { (void)label;(void)attributes;STUB_TRAP(); }









unsigned EmitForLoop(QUAD *quad,unsigned index,void *top,void *exit) { (void)quad;(void)index;(void)top;(void)exit;STUB_TRAP();return 0; }

cg_name GenSegmentOp(cg_name left,cg_name right) { (void)left;(void)right;STUB_TRAP();return NULL; }

cg_name ExceptCode(void) { STUB_TRAP();return NULL; }

void StartTry(unsigned scope) { (void)scope;STUB_TRAP(); }

void EndFinally(void) { STUB_TRAP(); }

void CallTryFini(void) { STUB_TRAP(); }

void EmitUnwind(unsigned scope) { (void)scope;STUB_TRAP(); }

void SetTryTable(void *table) { (void)table;STUB_TRAP(); }

void GenAssignment(LEAFPTR left,LEAFPTR right,LEAFPTR result) { (void)left;(void)right;(void)result;STUB_TRAP(); }

cg_name GenCast(LEAFPTR source,LEAFPTR result,cg_type type) { (void)source;(void)result;(void)type;STUB_TRAP();return NULL; }

cg_name LeafAddress(LEAFPTR leaf) { (void)leaf;STUB_TRAP();return NULL; }



cg_name CGTempName(void *temp,cg_type type) { (void)temp;(void)type;STUB_TRAP();return 0; }
cg_name CGBitMask(cg_name value,unsigned char start,unsigned char width,cg_type type) { (void)value;(void)start;(void)width;(void)type;STUB_TRAP();return 0; }
int SymbolPointerType(LEAFPTR leaf,SYMPTR symbol) { (void)leaf;(void)symbol;STUB_TRAP();return 0; }
cg_name ConvertFromBased(cg_name value,cg_type type) { (void)value;(void)type;STUB_TRAP();return 0; }
cg_name ConvertToBased(cg_name value,LEAFPTR result,unsigned flags) { (void)value;(void)result;(void)flags;STUB_TRAP();return 0; }
cg_name FarPointerSegmentAddress(cg_name value) { (void)value;STUB_TRAP();return 0; }



cg_name CGFloat(char *value,cg_type type) { (void)value;(void)type;STUB_TRAP();return 0; }
cg_name CGBackName(void *back,cg_type type) { (void)back;(void)type;STUB_TRAP();return 0; }
cg_name CGVolatile(cg_name value) { (void)value;STUB_TRAP();return 0; }
cg_name GenPointerConversion(cg_name value,cg_op op) { (void)value;(void)op;STUB_TRAP();return 0; }
cg_name GenBasedPointer(SYM_HANDLE symbol,cg_name value) { (void)symbol;(void)value;STUB_TRAP();return 0; }

unsigned GenCall(LEAFPTR function,cg_type type,unsigned index,QUAD *quad,LEAFPTR result,int inline_call) { (void)function;(void)type;(void)index;(void)quad;(void)result;(void)inline_call;STUB_TRAP();return 0; }

void GenJump(LEAFPTR label) { (void)label;STUB_TRAP(); }






void EmitLocalsReverse(SYM_HANDLE first) { (void)first;STUB_TRAP(); }

struct local_vars * ReleaseVars(SYM_HANDLE first,struct local_vars *previous) { (void)first;(void)previous;STUB_TRAP();return NULL; }

void RelLocalVars(struct local_vars *list) { (void)list;STUB_TRAP(); }

void FreeTrySymBackInfo(void) { STUB_TRAP(); }

void FreeLocalVars(SYM_HANDLE first) { (void)first;STUB_TRAP(); }

void GenSwitch(unsigned switch_leaf,LEAFPTR value,LEAFPTR result) { (void)switch_leaf;(void)value;(void)result;STUB_TRAP(); }

int LeafCGType(LEAFPTR leaf) { (void)leaf;STUB_TRAP();return 0; }

int LeafResultType(LEAFPTR leaf) { (void)leaf;STUB_TRAP();return 0; }






void OutLineNum(unsigned short line,unsigned char label_line) { (void)line;(void)label_line;STUB_TRAP(); }

void DBBegBlock(void) { STUB_TRAP(); }

void DBEndBlock(void) { STUB_TRAP(); }



void BEFiniLabel(cg_label label) { (void)label;STUB_TRAP(); }

unsigned long BEUnrollCount(unsigned long count) { (void)count;STUB_TRAP();return 0; }





void InsToAddr(address_name *addr) { (void)addr;STUB_TRAP(); }
name *GenIns(address_name *addr) { (void)addr;STUB_TRAP();return 0; }
void CopyAddr(address_name *source,address_name *dest) { (void)source;(void)dest;STUB_TRAP(); }


void BoolFree(bool_node *node) { (void)node;STUB_TRAP(); }

void CGReturn(cg_name value,cg_type type) { (void)value;(void)type;STUB_TRAP(); }


cg_name CGPostGets(cg_op op,cg_name left,cg_name right,cg_type type) { (void)op;(void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

cg_name CGPreGets(cg_op op,cg_name left,cg_name right,cg_type type) { (void)op;(void)left;(void)right;(void)type;STUB_TRAP();return NULL; }


struct tree_node *FoldUMinus(struct tree_node *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return 0; }
struct tree_node *Fold1sComp(struct tree_node *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return 0; }
struct tree_node *FoldSqrt(struct tree_node *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return 0; }
struct tree_node *FoldLog(unsigned char op,struct tree_node *value,type_def *type) { (void)op;(void)value;(void)type;STUB_TRAP();return 0; }


cg_name CGFlow(cg_op op,cg_name left,cg_name right) { (void)op;(void)left;(void)right;STUB_TRAP();return NULL; }

cg_name CGChoose(cg_name condition,cg_name first,cg_name second,cg_type type) { (void)condition;(void)first;(void)second;(void)type;STUB_TRAP();return NULL; }

unsigned char FoldIfTrue(struct tree_node *node,struct code_lbl *label) { (void)node;(void)label;STUB_TRAP();return 0; }

void CGBigLabel(void *back) { (void)back;STUB_TRAP(); }

void CGTrash(cg_name value) { (void)value;STUB_TRAP(); }

void CGDone(cg_name value) { (void)value;STUB_TRAP(); }



void ParmReverse(SYM_HANDLE first) { (void)first;STUB_TRAP(); }





void BGParmInline(void *symbol,type_def *type) { (void)symbol;(void)type;STUB_TRAP(); }

void CGAutoDecl(void *symbol,cg_type type) { (void)symbol;(void)type;STUB_TRAP(); }

void DBLocalSym(void *symbol,cg_type type) { (void)symbol;(void)type;STUB_TRAP(); }

void CallTryInit(void) { STUB_TRAP(); }

void TryScrapLabel(struct code_lbl *label) { (void)label;STUB_TRAP(); }

unsigned char PrevClass(ins_entry *entry) { (void)entry;STUB_TRAP();return 0; }

void PullQueue(void) { STUB_TRAP(); }









void AddLblDef(ins_entry *entry) { (void)entry;STUB_TRAP(); }

void AddLblRef(ins_entry *entry) { (void)entry;STUB_TRAP(); }

void UnLinkInstr(ins_entry *entry) { (void)entry;STUB_TRAP(); }

ins_entry * Untangle(ins_entry *entry) { (void)entry;STUB_TRAP();return NULL; }

void TraceCommon(ins_entry *entry) { (void)entry;STUB_TRAP(); }

void CallRet(ins_entry *entry) { (void)entry;STUB_TRAP(); }

ins_entry * PrevIns(ins_entry *entry) { (void)entry;STUB_TRAP();return NULL; }

unsigned char ComCode(ins_entry *entry) { (void)entry;STUB_TRAP();return 0; }

void JmpRet(ins_entry *entry) { (void)entry;STUB_TRAP(); }

unsigned char RetAftrLbl(ins_entry *entry) { (void)entry;STUB_TRAP();return 0; }

void RetAftrCall(ins_entry *entry) { (void)entry;STUB_TRAP(); }

unsigned char ComTail(ins_entry *list,ins_entry *entry) { (void)list;(void)entry;STUB_TRAP();return 0; }

void FreePendingDeletes(void) { STUB_TRAP(); }



void BGProcInline(void *symbol,type_def *type) { (void)symbol;(void)type;STUB_TRAP(); }

void FlushObject(void) { STUB_TRAP(); }








void ZapXX(name *entry,unsigned char type_class,long size) { (void)entry;(void)type_class;(void)size;STUB_TRAP(); }


name *STempOffset(name *temp,long offset,unsigned char type_class,long size) { (void)temp;(void)offset;(void)type_class;(void)size;STUB_TRAP();return 0; }

name *Display(void *symbol,int level) { (void)symbol;(void)level;STUB_TRAP();return 0; }
unsigned char AskNameCode(void *symbol,unsigned char kind) { (void)symbol;(void)kind;STUB_TRAP();return 0; }
unsigned char IsFarFunc(void *symbol) { (void)symbol;STUB_TRAP();return 0; }
unsigned short AskSegID(void *symbol,unsigned char kind) { (void)symbol;(void)kind;STUB_TRAP();return 0; }
unsigned char AskSegPrivate(unsigned short segment) { (void)segment;STUB_TRAP();return 0; }
void FlowOff(struct bool_node *node) { (void)node;STUB_TRAP(); }
void FreeIns(instruction *entry) { (void)entry;STUB_TRAP(); }

long ParmMem(type_def *type,call_state *state) { (void)type;(void)state;STUB_TRAP();return 0; }

void DbgParmLoc(name *value,void *symbol) { (void)value;(void)symbol;STUB_TRAP(); }







struct instruction * MakeMove(name *source,name *destination,unsigned char type_class) { (void)source;(void)destination;(void)type_class;STUB_TRAP();return NULL; }

















void SaveToTargProc(void) { STUB_TRAP(); }









hw_reg_set StructReg(void) { STUB_TRAP();return 0; }



unsigned char IsRegClass(hw_reg_set reg,unsigned char type_class) { (void)reg;(void)type_class;STUB_TRAP();return 0; }

void InitStackMap(void) { STUB_TRAP(); }

void FlushBlocks(unsigned char routine_done) { (void)routine_done;STUB_TRAP(); }

void FreeExtraSyms(name *last) { (void)last;STUB_TRAP(); }

void BlowAwayFreeLists(void) { STUB_TRAP(); }

void GenPartialRoutine(unsigned char routine_done) { (void)routine_done;STUB_TRAP(); }



unsigned char FixReturns(void) { STUB_TRAP();return 0; }

void FixEdges(void) { STUB_TRAP(); }

unsigned Renumber(void) { STUB_TRAP();return 0; }

void BlockTrim(void) { STUB_TRAP(); }

void FindReferences(void) { STUB_TRAP(); }

unsigned char TailRecursion(void) { STUB_TRAP();return 0; }

void NullConflicts(unsigned short usage) { (void)usage;STUB_TRAP(); }

unsigned char InsDead(void) { STUB_TRAP();return 0; }

void FixMemRefs(void) { STUB_TRAP(); }

void PreOptimize(void) { STUB_TRAP(); }

void FixMemBases(void) { STUB_TRAP(); }

unsigned char MemCritical(void) { STUB_TRAP();return 0; }

void Panic(unsigned char routine_done) { (void)routine_done;STUB_TRAP(); }

void MakeConflicts(void) { STUB_TRAP(); }

void SplitVars(void) { STUB_TRAP(); }

void AddCacheRegs(void) { STUB_TRAP(); }

void MakeLiveInfo(void) { STUB_TRAP(); }

void AxeDeadCode(void) { STUB_TRAP(); }

void FixIndex(void) { STUB_TRAP(); }

void FixSegments(void) { STUB_TRAP(); }

void FPRegAlloc(void) { STUB_TRAP(); }

unsigned char RegAlloc(unsigned char part) { (void)part;STUB_TRAP();return 0; }

void FPParms(void) { STUB_TRAP(); }

void PostOptimize(void) { STUB_TRAP(); }

void AssignTemps(void) { STUB_TRAP(); }

void FiniStackMap(void) { STUB_TRAP(); }

void FreeConflicts(void) { STUB_TRAP(); }

void SortBlocks(void) { STUB_TRAP(); }

void GenProlog(void) { STUB_TRAP(); }

void UnFixEdges(void) { STUB_TRAP(); }

void OptSegs(void) { STUB_TRAP(); }

void GenObject(void) { STUB_TRAP(); }

void GenEpilog(void) { STUB_TRAP(); }

void FreeProc(void) { STUB_TRAP(); }

void PropNullInfo(void) { STUB_TRAP(); }







unsigned char CFIsI32(struct cfloat *value) { (void)value;STUB_TRAP();return 0; }





unsigned char CFIsU16(struct cfloat *value) { (void)value;STUB_TRAP();return 0; }

struct tree_node *TGTrash(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }


unsigned char RHSLongPointer(struct tree_node *value) { (void)value;STUB_TRAP();return 0; }
unsigned char NeedPtrConvert(struct address_name *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return 0; }

unsigned char CFSignedSize(struct cfloat *value,unsigned size) { (void)value;(void)size;STUB_TRAP();return 0; }
unsigned char CFUnSignedSize(struct cfloat *value,unsigned size) { (void)value;(void)size;STUB_TRAP();return 0; }


struct cfloat *CFRound(struct cfloat *value) { (void)value;STUB_TRAP();return NULL; }
void TGDemote(struct tree_node *value,type_def *type) { (void)value;(void)type;STUB_TRAP(); }


void CFNegate(struct cfloat *value) { (void)value;STUB_TRAP(); }


struct tree_node *FoldMinus(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldTimes(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldDiv(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldMod(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldLShift(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldRShift(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldAnd(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldOr(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldXor(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *FoldPow(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

unsigned char CFIs32(struct cfloat *value) { (void)value;STUB_TRAP();return 0; }
long CFConvertByType(struct cfloat *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return 0; }
struct tree_node *IntToType(long value,type_def *type) { (void)value;(void)type;STUB_TRAP();return NULL; }
struct tree_node *CFToType(struct cfloat *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return NULL; }
struct cfloat *CFAdd(struct cfloat *left,struct cfloat *right) { (void)left;(void)right;STUB_TRAP();return NULL; }
type_def *FixAddType(struct tree_node *left,struct tree_node *right,type_def *type) { (void)left;(void)right;(void)type;STUB_TRAP();return NULL; }

struct tree_node *TGVolatile(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }




int GetLog2(unsigned long value) { (void)value;STUB_TRAP();return 0; }
name *AllocIntConst(int value) { (void)value;STUB_TRAP();return NULL; }

unsigned long Mask(struct bit_tree_node *value) { (void)value;STUB_TRAP();return 0; }

unsigned char SimpleLeaf(struct tree_node *value) { (void)value;STUB_TRAP();return 0; }



unsigned char SetMemOut(unsigned char value) { (void)value;STUB_TRAP();return 0; }

struct address_name *TNBinary(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }



struct address_name *TNAssign(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *TNPreGets(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *TNPostGets(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *NotAddrGen(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *TNCall(struct tree_node *value,unsigned char use_result) { (void)value;(void)use_result;STUB_TRAP();return NULL; }

struct address_name *Arithmetic(struct address_name *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return NULL; }

struct address_name *TNQuestion(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *AddrGen(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *BGUnary(unsigned char op,struct address_name *value,type_def *type) { (void)op;(void)value;(void)type;STUB_TRAP();return NULL; }

struct address_name *TNBitShift(struct address_name *value,struct bit_tree_node *node,unsigned char masked) { (void)value;(void)node;(void)masked;STUB_TRAP();return NULL; }

struct address_name *TNWarp(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *TNCallback(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct address_name *TNPatch(struct tree_node *value) { (void)value;STUB_TRAP();return NULL; }

struct bool_node *BGFlow(unsigned char op,struct bool_node *left,struct bool_node *right) { (void)op;(void)left;(void)right;STUB_TRAP();return NULL; }

struct bool_node *BGCompare(unsigned char op,struct address_name *left,struct address_name *right,struct code_lbl *entry,type_def *type) { (void)op;(void)left;(void)right;(void)entry;(void)type;STUB_TRAP();return NULL; }
struct bool_node *Boolean(struct address_name *value,struct code_lbl *entry) { (void)value;(void)entry;STUB_TRAP();return NULL; }

struct address_name *Flip(struct address_name *value,unsigned char when_false,unsigned char when_true) { (void)value;(void)when_false;(void)when_true;STUB_TRAP();return NULL; }

struct address_name *BGConvert(struct address_name *value,type_def *type) { (void)value;(void)type;STUB_TRAP();return NULL; }
void AddrDemote(struct address_name *value) { (void)value;STUB_TRAP(); }

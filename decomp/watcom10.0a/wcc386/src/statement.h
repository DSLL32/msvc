#ifndef WCC_STATEMENT_H
#define WCC_STATEMENT_H
#include "declarations.h"
#include <recon.h>
extern int TryCount,TryScope,LoopDepth;
extern struct block_definition *LoopStack;
struct case_entry {
    struct case_entry *next_case;
    unsigned value,label;
};
#pragma pack(1)
struct switch_definition {
    struct switch_definition *prev_switch;
    unsigned default_label,number_of_cases;
    struct case_entry *case_list;
    unsigned low_value,high_value;
    unsigned short quad_index,case_count;
    char *case_format;
    TYPEPTR case_type;
};
#pragma pack()
extern struct switch_definition *SwitchStack;
void StmtInit(void);
void startNewBlock(void);
unsigned NextLabel(void);
unsigned BracketExpr(void);
int JumpFalse(unsigned leaf,unsigned label);
void JumpCond(unsigned leaf,unsigned label,int op,int opposite);
void SetErrLoc(char *file,unsigned line);
void ChkUseful(void);
void NewLoop(void);
void ForStmt(void);
void JumpBreak(struct block_definition *block);
/* Descriptive names for the earlier quad-based induction analyzers. */
int AnalyzeForInit(unsigned first,unsigned last);
void AnalyzeForIncrement(void);
struct leaf;
int SameSymbolLeaf(struct leaf *first,struct leaf *second);
void SwitchStmt(void);
void CaseStmt(void);
void AddCaseLabel(unsigned value);
unsigned NextCaseLabel(unsigned value);
void DefaultStmt(void);
void BreakStmt(void);
void ContinueStmt(void);
void LeaveStmt(void);
unsigned ReturnStmt(unsigned result,char *return_type);
void ChkRetType(unsigned leaf);
void UnWindTry(int scope);
void Jump(unsigned label);
void GotoStmt(void);
void LeftBrace(void);
void TryStmt(void);
void StmtExpr(void);
void ChkStmtExpr(void);
void FinishStmtExpr(int useful);
void EndOfStmt(void);
void EndBlock(void);
void ElseStmt(void);
void DropBreakLabel(void);
void DropContinueLabel(void);
void EndForStmt(void);
void JumpTrue(unsigned leaf,unsigned label);
void EndSwitch(void);
int EndTry(void);
void PopBlock(void);
void ChkRetValue(void);
void DropLabel(unsigned label);
void AddLabelRef(unsigned label,unsigned quad);
unsigned GenQuadNoResult(unsigned first,int op,unsigned second);
void FreeLabels(void);
void OpenDefFile(void);
void DumpFuncDefn(void);
void GrabLabels(void);
void LookAhead(void);
#pragma pack(1)
struct label_entry {
    SYMPTR thread;
    struct label_entry *next_label;
    unsigned ref_list;
    unsigned char defined,referenced;
    char name[1];
};
#pragma pack()
struct label_entry *LkLabel(char *name);
void Statement(void);
#endif

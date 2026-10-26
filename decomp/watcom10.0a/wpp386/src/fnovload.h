#ifndef WPP_FNOVLOAD_H
#define WPP_FNOVLOAD_H
#include "symbol.h"
#include <recon.h>
/* Actual old rank stride10, coarse rank+6 and control+7. The old
 * scalar conversion counts/flags occupy three bytes per conversion. */
#pragma pack(1)
typedef struct fnov_scalar {
    unsigned char standard,udcnv;
    unsigned char trivial:1,promotion:1,not_exact:1;
} FNOV_SCALAR;
typedef struct fnov_rank {
    union { FNOV_SCALAR no_ud; struct { FNOV_SCALAR in,out; } ud; } u;
    unsigned char rank;
    unsigned short control;
    unsigned char userdef:1;
} FNOV_RANK;
typedef struct func_list FNOV_LIST;
struct func_list {
    FNOV_LIST *next;
    SYMBOL sym;
    FNOV_RANK *rankvector;
    arg_list *alist;
    unsigned num_args;
    unsigned short flags;
    FNOV_RANK thisrank;
    unsigned char free_args:1,member:1,stdops:1;
};
#pragma pack()
typedef char FnovRankSize[(sizeof(FNOV_RANK)==10)?1:-1];
typedef char FnovListSize[(sizeof(FNOV_LIST)==33)?1:-1];
void FnovListFree(FNOV_LIST **list);
void FnovFreeDiag(void);
void deleteRank(FNOV_RANK *rank,unsigned count);
int buildOverloadListFromSym(unsigned short control,FNOV_LIST **candidates,arg_list *args,SYMBOL old);
void fnovNumCandidatesSet(FNOV_LIST *candidates);
int processSym(unsigned short control,FNOV_LIST **candidates,arg_list *args,SYMBOL sym);
int isSimpleCandidate(TYPE type,unsigned count);
int isMemberCandidate(TYPE type,unsigned count);
int isEllipsisCandidate(TYPE type,unsigned count);
void addListEntry(unsigned short control,FNOV_LIST **candidates,SYMBOL sym,arg_list *args,int free_args);
arg_list *MakeMemberArgList(SYMBOL sym,unsigned count);
arg_list *MakeMockArgList(TYPE type,unsigned count);
int SymIsFunctionTemplateModel(SYMBOL sym);
unsigned char resolveOverload(unsigned short control,arg_list *args,void *plist,FNOV_LIST **candidates,FNOV_LIST **match);
int isRank(FNOV_LIST *entry,unsigned char rank);
int isReturnIdentical(TYPE left,TYPE right);
void setFnovDiagnosticRejectList(FNOV_LIST **list);
void setFnovDiagnosticAmbigList(FNOV_LIST **list);
unsigned char doFunctionDistinctCheck(unsigned short control,SYMBOL *previous,SYMBOL current,char *name);
#endif

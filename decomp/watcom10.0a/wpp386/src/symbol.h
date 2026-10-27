#ifndef WPP_SYMBOL_H
#define WPP_SYMBOL_H
#include "types.h"
#include "scope.h"
#include <recon.h>
typedef struct symbol *SYMBOL;
#pragma pack(1)
struct symbol {
    SYMBOL next, thread;
    TYPE sym_type;
    SYMBOL_NAME name;
    void *locn;
    union { unsigned uval; int sval; void *tinfo; SYMBOL sym,thunk_calls,alias; TYPE type; } u;
    unsigned flag;
    unsigned char flag2,id;
    unsigned short segid;
};
#pragma pack()
typedef char SymbolSize[(sizeof(struct symbol)==32)?1:-1];
SYMBOL AllocSymbol(void);
SYMBOL SymDefaultBase(SYMBOL sym);
arg_list *SymFuncArgList(SYMBOL sym);
SYMBOL symAllocate(TYPE type,unsigned char id,unsigned flags);
SYMBOL ScopeInsert(SCOPE scope,SYMBOL sym,char *name);
SYMBOL SymCreate(TYPE type,unsigned char id,unsigned flags,char *name,SCOPE scope);
SYMBOL SymCreateFileScope(TYPE type,unsigned char id,unsigned flags,char *name);
void LinkageSet(SYMBOL sym,char *language);
unsigned ScopeCarefulInsert(SCOPE scope,SYMBOL *psym,char *name);
SYMBOL_NAME scopeInsertName(SCOPE scope,SYMBOL sym,char *name);
SYMBOL DeclCheck(SYMBOL_NAME name,SYMBOL sym,unsigned *status);
void FreeSymbol(SYMBOL sym);
void addOrdered(SCOPE scope,SYMBOL sym);
void ClassMember(SCOPE scope,SYMBOL sym);
void DeclDefaultStorageClass(SCOPE scope,SYMBOL sym);
#endif

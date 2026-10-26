#ifndef WPP_SCOPE_H
#define WPP_SCOPE_H
#include "compiler.h"
#include "carve.h"
#include "cgio.h"
#include <recon.h>
typedef struct scope *SCOPE;
typedef struct symbol_name *SYMBOL_NAME;
typedef struct hash_tab *HASHTAB;
#pragma pack(1)
struct hash_tab {
    union { SYMBOL_NAME list; SYMBOL_NAME *table; } u;
    unsigned *bits;
    unsigned names;
    unsigned char table_class;
};
#pragma pack()
typedef char HashTabSize[(sizeof(struct hash_tab)==13)?1:-1];
#pragma pack(1)
struct scope {
    SCOPE enclosing;
    HASHTAB names;
    void *ordered;
    void *owner;
    unsigned char id;
    unsigned char keep:1, dtor_reqd:1, dtor_naked:1, try_catch:1, arg_check:1;
};
struct symbol_name {
    SYMBOL_NAME next;
    struct symbol *name_type, *name_syms;
    char *name;
    SCOPE containing;
};
#pragma pack()
typedef char ScopeSize[(sizeof(struct scope)==18)?1:-1];
typedef char SymbolNameSize[(sizeof(struct symbol_name)==20)?1:-1];
/* Old60-byte lookup result: no later overload region or diagnostic-info fields. */
typedef struct search_result SEARCH_RESULT;
#pragma pack(1)
struct search_result {
    SCOPE scope;
    SYMBOL_NAME sym_name;
    struct symbol *sym;
    SCOPE start,access_decl;
    long vb_offset,vb_index,delta,exact_delta,offset,vf_offset;
    CGIO_LOCATION errlocn;
    unsigned short perm;
    unsigned char flags,flags2;
};
#pragma pack()
typedef char SearchResultSize[(sizeof(SEARCH_RESULT)==60)?1:-1];
SEARCH_RESULT *ScopeFindLexicalClassType(SCOPE scope,char *name);
SEARCH_RESULT *ScopeFindNaked(SCOPE scope,char *name);
void ScopeFreeResult(SEARCH_RESULT *result);
int ScopeCheckSymbol(SEARCH_RESULT *result,struct symbol *sym);
extern SCOPE CurrScope, FileScope;
SYMBOL_NAME ScopeYYLexical(SCOPE scope,char *name);
SYMBOL_NAME ScopeYYMember(SCOPE scope,char *name);
SYMBOL_NAME AllocSymbolName(char *name, SCOPE scope);
SCOPE ScopeSetContaining(SYMBOL_NAME name, SCOPE scope);
SCOPE ScopeBegin(unsigned char id);
SCOPE ScopeEnd(unsigned char id);
SCOPE ScopeCreate(unsigned char id);
void ScopeOpen(SCOPE scope);
SCOPE ScopeEstablishEnclosing(SCOPE scope,SCOPE enclosing);
void ScopeSetClassOwner(SCOPE scope,struct type *type);
HASHTAB HashCreate(void);
int HashEmpty(HASHTAB hash);
void HashPostInit(SCOPE scope);
void HashMakeMax(HASHTAB hash);
SYMBOL_NAME HashLookup(HASHTAB hash,char *name);
void HashInsert(HASHTAB hash,SYMBOL_NAME sym,char *name);
void ScopeKeep(SCOPE scope);
int ScopeType(SCOPE scope,unsigned char id);
#endif

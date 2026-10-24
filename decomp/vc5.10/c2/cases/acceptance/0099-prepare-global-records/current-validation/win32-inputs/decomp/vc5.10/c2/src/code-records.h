#ifndef C2_CODE_RECORDS_H
#define C2_CODE_RECORDS_H
#include "functions.h"
#include <recon.h>

typedef struct C2Expression C2Expression;
/* Typed references are prefixes, with unvisited fields deliberately unnamed. */
typedef struct C2SymbolReference { C2Symbol *symbol; } C2SymbolReference;
typedef struct C2ExpressionReference {
    unsigned char pending00[8];
    C2SymbolReference *reference;
} C2ExpressionReference;
struct C2Expression {
    C2Expression *next;
    unsigned opcode;
    unsigned char kind, pending09[11];
    union { C2Symbol *symbol; C2ExpressionReference *reference; } value;
    unsigned pending18;
    C2ExpressionReference *indirect;
};
struct C2Record {
    C2Record *next;
    unsigned opcode;
    unsigned char kind, pending09[7];
    unsigned short line, pending12;
    C2SymbolReference *reference;
    union { C2Expression *arguments; C2InputSymbol *declaration; } value;
    C2Expression *results;
};
typedef char C2ExpressionKindCheck[offsetof(C2Expression, kind) == 8 ? 1 : -1];
typedef char C2ExpressionValueCheck[offsetof(C2Expression, value) == 20 ? 1 : -1];
typedef char C2ExpressionIndirectCheck[offsetof(C2Expression, indirect) == 28 ? 1 : -1];
typedef char C2RecordKindCheck[offsetof(C2Record, kind) == 8 ? 1 : -1];
typedef char C2RecordLineCheck[offsetof(C2Record, line) == 16 ? 1 : -1];
typedef char C2RecordReferenceCheck[offsetof(C2Record, reference) == 20 ? 1 : -1];
typedef char C2RecordValueCheck[offsetof(C2Record, value) == 24 ? 1 : -1];
typedef char C2RecordResultsCheck[offsetof(C2Record, results) == 28 ? 1 : -1];
extern C2InitRecord *global_initializers;
extern int global_records_processing;
C2Symbol *__fastcall resolve_alias_symbol(C2Symbol *symbol);
void __fastcall record_symbol_references(C2Symbol *symbol);
int __fastcall symbol_requires_references(C2Symbol *symbol);
int __fastcall initializer_is_static(C2InitRecord *record);
C2DebugLink *new_symbol_link(void);
extern unsigned record_source_line;
extern unsigned short record_line_delta;
#endif

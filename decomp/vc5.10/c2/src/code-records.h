#ifndef C2_CODE_RECORDS_H
#define C2_CODE_RECORDS_H
#include "functions.h"
#include <recon.h>

/* Typed references are prefixes, with unvisited fields deliberately unnamed. */
typedef struct C2SymbolReference { C2Symbol *symbol; } C2SymbolReference;
typedef struct C2ExpressionReference {
    unsigned char pending00[8];
    C2SymbolReference *reference;
} C2ExpressionReference;
/* Kind29 context links carry a defining record. The real allocator clears
 * all16 bytes; only next/kind/record have encountered semantic consumers. */
typedef struct C2StatementLink {
    struct C2StatementLink *next;
    unsigned pending04;
    unsigned char kind, pending09[3];
    C2Expression *record;
} C2StatementLink;
typedef char C2StatementLinkSize[sizeof(C2StatementLink) == 16 ? 1 : -1];
typedef char C2StatementLinkRecordOffset[offsetof(C2StatementLink, record) == 12 ? 1 : -1];
/* Integer payloads are two DWORD-aligned words at record+24. */
#pragma pack(push, 4)
typedef union C2IntegerWords {
    struct { unsigned low, high; };
    unsigned __int64 value;
} C2IntegerWords;
#pragma pack(pop)
typedef struct C2SymbolInputUse {
    struct C2SymbolInputUse *next;
    unsigned pending04;
    unsigned char kind, pending09[3];
    C2Expression *expression;
    unsigned pending10;
} C2SymbolInputUse;
typedef char C2SymbolInputUseSizeCheck[sizeof(C2SymbolInputUse) == 20 ? 1 : -1];
typedef char C2SymbolInputUseValueCheck[offsetof(C2SymbolInputUse, expression) == 12 ? 1 : -1];
struct C2Expression {
    C2Expression *next;
    unsigned opcode;
    unsigned char kind, flags;
    union { unsigned short type; struct { union { unsigned char convention, condition_code; }; unsigned char pending0b; }; };
    /* The same DWORD is a list link or an alias/order key in distinct passes. */
    union { C2Expression *previous, *global_operands, *cached_next; unsigned optimizer_alias_number, order_key, allocation_class_number; long stack_adjustment; };
    union {
        unsigned short line, attributes;
        struct { unsigned char properties;
            union { unsigned char comparison, use_flags; }; } expression;
    } location;
    union {
        struct {
            unsigned char control_flags;
            union { unsigned char pending13, precision_flags; };
        };
        unsigned short peephole_number;
    };
    union {
        C2Symbol *symbol;
        C2ExpressionReference *reference;
        C2FunctionRegister *reg;
        C2RegisterSet *registers;
        C2Real *real;
        C2Function *function;
        C2InputUse *use;
        C2Expression *branch_record;
        unsigned control;
        void *pointer;
        unsigned optimizer_index;
    } value;
    union {
        struct {
            union { C2Expression *left; C2InputSymbol *declaration; C2FunctionRegister *original_register; struct C2MachineRegister *fixed_register; unsigned immediate; };
            union { C2Expression *right; C2ExpressionReference *indirect; C2SymbolInputUse *symbol_uses; C2FunctionRegister *aggregate_register; } tail;
        };
        C2IntegerWords integer;
    };
    union { void *data; C2StatementLink *statement_links; C2Expression *assembly; C2InputUse *use; struct C2InputClause *clauses; C2BranchTarget *branch; unsigned short intrinsic; unsigned char condition; long aggregate_displacement; } detail;
    union { C2Expression *aggregate_value;
        struct { unsigned char call_convention, pending_call25[3]; }; };
    union { C2Expression *aggregate_second; unsigned branch_flags; };
};
struct C2Record {
    C2Record *next;
    unsigned opcode;
    unsigned char kind, pending09[3];
    C2Record *previous;
    unsigned short line, pending12;
    union { C2SymbolReference *reference; C2BasicBlock *block; C2RegisterSet *registers; } owner;
    union { C2Expression *arguments; C2InputSymbol *declaration; } value;
    C2Expression *results;
};
typedef char C2ExpressionIntegerCheck[offsetof(C2Expression, integer) == 24 ? 1 : -1];
typedef char C2ExpressionTypeCheck[offsetof(C2Expression, type) == 10 ? 1 : -1];
typedef char C2ExpressionLocationCheck[offsetof(C2Expression, location) == 16 ? 1 : -1];
typedef char C2ExpressionPeepholeNumberCheck[offsetof(C2Expression, peephole_number) == 18 ? 1 : -1];
typedef char C2ExpressionControlCheck[offsetof(C2Expression, control_flags) == 18 ? 1 : -1];
typedef char C2ExpressionDetailCheck[offsetof(C2Expression, detail) == 32 ? 1 : -1];
typedef char C2ExpressionCallConventionCheck[offsetof(C2Expression, call_convention) == 36 ? 1 : -1];
typedef char C2ExpressionAggregateCheck[offsetof(C2Expression, aggregate_value) == 36 ? 1 : -1];
typedef char C2ExpressionKindCheck[offsetof(C2Expression, kind) == 8 ? 1 : -1];
typedef char C2ExpressionValueCheck[offsetof(C2Expression, value) == 20 ? 1 : -1];
typedef char C2ExpressionIndirectCheck[offsetof(C2Expression, tail) == 28 ? 1 : -1];
typedef char C2RecordKindCheck[offsetof(C2Record, kind) == 8 ? 1 : -1];
typedef char C2RecordPreviousCheck[offsetof(C2Record, previous) == 12 ? 1 : -1];
typedef char C2RecordLineCheck[offsetof(C2Record, line) == 16 ? 1 : -1];
typedef char C2RecordReferenceCheck[offsetof(C2Record, owner) == 20 ? 1 : -1];
typedef char C2RecordValueCheck[offsetof(C2Record, value) == 24 ? 1 : -1];
typedef char C2RecordResultsCheck[offsetof(C2Record, results) == 28 ? 1 : -1];
extern C2DebugLink *pending_debug_symbols;
void __fastcall align_pending_debug_symbols(C2DebugLink *symbols);
void write_pending_debug_symbols(void);
void write_assembly_symbols(void);
void __fastcall write_record_initializers(C2DebugLink *records);
void __fastcall write_assembly_records(C2DebugLink *records);
C2InitRecord *__fastcall lookup_initializer(unsigned serial);
void __fastcall write_assembly_initializer(C2InitRecord *record);
extern C2InitRecord *global_initializers;
extern int global_records_processing;
C2Symbol *__fastcall resolve_alias_symbol(C2Symbol *symbol);
void __fastcall record_symbol_references(C2Symbol *symbol);
int __fastcall symbol_requires_references(C2Symbol *symbol);
int __fastcall initializer_is_static(C2InitRecord *record);
C2DebugLink *new_symbol_link(void);
extern unsigned record_source_line;
extern unsigned short record_line_delta;
void __fastcall link_code_nodes(C2Record *first, C2Record *last);
void __fastcall clear_function_symbol_references(C2Function *function, C2InputSymbol *symbol);
void __fastcall remove_pending_symbol_references(C2InputSymbol *symbol);
C2Expression *__fastcall retain_input_result(C2Expression *expression);
#endif

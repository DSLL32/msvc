#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "backend.h"
#include "symbols.h"
#include "warnings.h"
#include <recon.h>
typedef struct FunctionSymbol {
    SymbolHeader symbol;
    unsigned field20, field24, field28, field2c, field30, parameter_bytes;
    unsigned code_bytes;
    unsigned short prologue_bytes, epilogue_offset;
    unsigned field40, field44, field48;
    struct FunctionSymbol *pending_next;
    unsigned short field50;
    unsigned char flags52, field53;
} FunctionSymbol;
typedef char FunctionSymbolSizeCheck[sizeof(FunctionSymbol) == 84 ? 1 : -1];
typedef struct WorkItem {
    struct WorkItem *next;
    struct WorkItem *previous;
    union { struct Expression *expressions; struct MachineBlockHeader *block; } body;
    struct FlowLink *successors, *predecessors;
    union { unsigned traversal_mark; struct BitSet *dominators; } analysis;
    struct FlowRegion *region;
    union {
        struct { struct BitSet *uses, *definitions; } selection;
        struct { union WorkReferenceTable *references; unsigned successor_count; } optimization;
    } pass;
    struct BitSet *flow_sets[6];
    short number;
    unsigned short region_depth;
    short exception_scope;
    unsigned short flags;
} WorkItem;
typedef char WorkItemSizeCheck[sizeof(WorkItem) == 68 ? 1 : -1];
typedef struct Expression {
    struct Expression *next;
    Node *tree;
    union { unsigned field08; struct BitSet *clobbered_definitions; };
    unsigned source_location;
} Expression;
typedef char ExpressionSizeCheck[sizeof(Expression) == 16 ? 1 : -1];
typedef struct Function {
    Node *root;
    SymbolHeader *symbol;
    SymbolHeader *return_temp;
    WorkItem *work;
    Expression *expressions;
    ScopeHash *scope;
    struct ExceptionScope *exception_scopes;
    unsigned options;
    const SavedWarning *warning_state;
    struct DebugScope *debug_scope;
    unsigned field28;
} Function;
extern FunctionSymbol *pending_functions, *current_symbol;
extern WorkItem *current_work;
extern Function *current_function;
extern int backend_stage;
extern WorkItem *global_exit_work;
extern Node *(__fastcall *tree_transform)(Node *node);
void __fastcall read_global_records(Function *function);
void emit_backend_work(void);
Function *__fastcall read_function(FunctionSymbol *symbol);
Function *__fastcall read_function_body(FunctionSymbol *symbol);
char *__fastcall format_function_name(SymbolHeader *symbol);
void __cdecl ide_message(int code, ...);
void __fastcall activate_scope(ScopeHash *scope);
void __fastcall apply_function_options(unsigned options);
void __fastcall prepare_inline_function(Function *function);
void __fastcall canonicalize_function(Function *function);
Node *__fastcall canonicalize_tree(Function *function, Node *node);
void __fastcall rewrite_expression(Expression *expression);
void __fastcall finish_inline_function(Function *function);
void __fastcall expand_inline_calls(Function *function);
Node *__fastcall optimize_tree_global(Node *node);
Node *__fastcall optimize_tree_local(Node *node);
void __fastcall transform_function_trees(Function *function);
void __fastcall cleanup_expressions(Expression **expressions);
void __fastcall optimize_function_flow(Function *function);
WorkItem *__fastcall build_global_work(Expression *expressions);
WorkItem *__fastcall build_local_work(Expression *expressions);
WorkItem *new_work_item(void);
Expression *__fastcall insert_tree_expression(Expression *expressions, Node *before, Node *tree);
void __fastcall optimize_work(WorkItem *work);
void __fastcall prepare_work(WorkItem *work);
void finish_global_work(void);
void __fastcall finalize_work_trees(WorkItem *work);
void __fastcall generate_work(WorkItem *work);
void __fastcall free_work(WorkItem *work);
void reset_work(void);
void release_function_symbols(void);
#endif

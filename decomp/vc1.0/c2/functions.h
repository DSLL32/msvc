#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "backend.h"
#include "symbols.h"
typedef struct FunctionSymbol {
    SymbolHeader symbol;
    unsigned field20, field24, field28, field2c, field30, field34;
    unsigned field38, field3c, field40, field44, field48;
    struct FunctionSymbol *pending_next;
    unsigned short field50;
    unsigned char flags52, field53;
} FunctionSymbol;
typedef struct WorkItem { struct WorkItem *next; } WorkItem;
typedef struct Function {
    Node *root;
    SymbolHeader *symbol;
    SymbolHeader *return_temp;
    WorkItem *work;
    void *expressions;
    ScopeHash *scope;
    unsigned field18, options, state;
    void *debug_scope;
    unsigned field28;
} Function;
extern FunctionSymbol *pending_functions, *current_symbol;
extern WorkItem *current_work;
extern Function *current_function;
extern int backend_stage, global_work_enabled;
extern Node *(__fastcall *tree_transform)(Node *node);
void __fastcall read_global_records(Function *function);
void initialize_work_state(void);
Function *__fastcall read_function(FunctionSymbol *symbol);
char *__fastcall format_function_name(FunctionSymbol *symbol);
void __cdecl ide_message(int code, ...);
void __fastcall activate_scope(ScopeHash *scope);
void __fastcall apply_function_options(unsigned options);
void __fastcall initialize_function_state(unsigned state);
void __fastcall prepare_inline_function(Function *function);
void __fastcall canonicalize_function(Function *function);
void __fastcall finish_inline_function(Function *function);
void __fastcall prepare_function_codegen(Function *function);
Node *__fastcall optimize_tree_global(Node *node);
Node *__fastcall optimize_tree_local(Node *node);
void __fastcall transform_function_trees(Function *function);
void __fastcall optimize_function_flow(Function *function);
WorkItem *__fastcall build_global_work(void *expressions);
WorkItem *__fastcall build_local_work(void *expressions);
void optimize_work(void);
void prepare_work(void);
void finish_global_work(void);
void __fastcall finalize_work_trees(WorkItem *work);
void generate_work(void);
void __fastcall free_work(WorkItem *work);
void reset_work(void);
void release_function_symbols(void);
#endif

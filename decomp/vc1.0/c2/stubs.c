#include "options.h"
#include "input.h"
#include "backend.h"
#include "extended.h"
#include "segments.h"
#include "initializers.h"
#include "functions.h"
#include "records.h"
BOOL WINAPI compiler_control(DWORD event) { __asm int 3 }
void compiler_cleanup(void) { __asm int 3 }
void initialize_debugging(void) { __asm int 3 }
void finish_output(void) { __asm int 3 }
void finish_debugging(void) { __asm int 3 }
void finish_globals(void) { __asm int 3 }
void __fastcall compiler_internal_error(char *source, unsigned code) { __asm int 3 }
void __cdecl compiler_report(unsigned code, ...) { __asm int 3 }
__declspec(naked) void report_context_error(void) { __asm int 3 }
int __fastcall suffix_matches(char *pattern, char *text, unsigned action) { __asm int 3 }
Initializer *__fastcall new_initializer(int kind) { __asm int 3 }
void __fastcall read_initializer_body(Initializer *initializer) { __asm int 3 }
void __fastcall insert_initializer(Initializer *initializer) { __asm int 3 }
void __fastcall adjust_initializer_debug(Initializer *initializer) { __asm int 3 }
void __fastcall release_initializer(Initializer *initializer) { __asm int 3 }
void initialize_work_state(void) { __asm int 3 }
Function *__fastcall read_function(FunctionSymbol *symbol) { __asm int 3 }
char *__fastcall format_function_name(FunctionSymbol *symbol) { __asm int 3 }
void __cdecl ide_message(int code, ...) { __asm int 3 }
void __fastcall activate_scope(ScopeHash *scope) { __asm int 3 }
void __fastcall apply_function_options(unsigned options) { __asm int 3 }
void __fastcall initialize_function_state(unsigned state) { __asm int 3 }
void __fastcall prepare_inline_function(Function *function) { __asm int 3 }
void __fastcall canonicalize_function(Function *function) { __asm int 3 }
void __fastcall finish_inline_function(Function *function) { __asm int 3 }
void __fastcall prepare_function_codegen(Function *function) { __asm int 3 }
Node *__fastcall optimize_tree_global(Node *node) { __asm int 3 }
Node *__fastcall optimize_tree_local(Node *node) { __asm int 3 }
void __fastcall transform_function_trees(Function *function) { __asm int 3 }
void __fastcall optimize_function_flow(Function *function) { __asm int 3 }
WorkItem *__fastcall build_global_work(void *expressions) { __asm int 3 }
WorkItem *__fastcall build_local_work(void *expressions) { __asm int 3 }
void optimize_work(void) { __asm int 3 }
void prepare_work(void) { __asm int 3 }
void finish_global_work(void) { __asm int 3 }
void __fastcall finalize_work_trees(WorkItem *work) { __asm int 3 }
void generate_work(void) { __asm int 3 }
void __fastcall free_work(WorkItem *work) { __asm int 3 }
void reset_work(void) { __asm int 3 }
void release_function_symbols(void) { __asm int 3 }
SymbolHeader *__fastcall type_of_symbol(SymbolHeader *symbol, int unwrap) { __asm int 3 }
SymbolHeader *__fastcall lookup_symbol(ScopeHash *scope, unsigned identifier) { __asm int 3 }
void __fastcall queue_symbol_debug(SymbolHeader *symbol) { __asm int 3 }
void __fastcall reserve_symbol_storage(SymbolHeader *symbol) { __asm int 3 }
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena) { __asm int 3 }
SymbolHeader *__fastcall lookup_global_symbol(unsigned identifier) { __asm int 3 }
void __fastcall clone_alias_symbol(SymbolHeader *symbol) { __asm int 3 }
void __fastcall apply_il_option(int option) { __asm int 3 }
void __cdecl compiler_warning(int level, int code, ...) { __asm int 3 }
void __fastcall uppercase_name(char *name) { __asm int 3 }
int __fastcall is_constant_symbol(SymbolHeader *symbol) { __asm int 3 }
unsigned next_symbol_identifier(void) { __asm int 3 }

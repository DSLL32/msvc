#ifndef C2_OPTIMIZER_H
#define C2_OPTIMIZER_H
#include "flow.h"
#include "register-sets.h"
#include "code-input.h"
#include <recon.h>

typedef struct C2OptimizerContext C2OptimizerContext;
typedef struct C2OptimizerNode C2OptimizerNode;
/* Prefix views for the scope information this pass actually reads. Their
 * remaining allocation/defaults are not inferred from these consumers. */
typedef struct C2OptimizerScope {
    unsigned pending00, kind;
    unsigned char pending08[20];
    C2FunctionSymbol *symbol;
    unsigned char pending20[8];
    C2FunctionSymbol *alternate;
} C2OptimizerScope;
typedef struct C2OptimizerCapture {
    unsigned char pending00[16];
    C2FunctionSymbol *symbol;
} C2OptimizerCapture;
unsigned __fastcall count_optimizer_records(C2Record *first);
unsigned __fastcall assign_optimizer_registers(int aliases, int mode);
void __fastcall prepare_alias_input(C2Function *function);
C2Expression * __fastcall build_optimizer_input(C2Record *first, unsigned alias_count);
C2OptimizerNode * __fastcall collect_optimizer_blocks(C2BasicBlock *block, unsigned alias_count, C2Expression *records);
C2OptimizerNode * __fastcall translate_optimizer_input(C2BasicBlock *block, unsigned count, int aliases, int no_global);
C2Expression * __fastcall fold_optimizer_records(C2Expression *records);
void __fastcall prepare_optimizer_values(C2Expression *records);
void __fastcall analyze_optimizer_values(unsigned alias_count, C2Expression *records, int aliases);
void __fastcall rewrite_optimizer_records(C2Expression *records);
void initialize_optimizer_registers(void);
void number_optimizer_registers(void);
void __fastcall build_optimizer_register_sets(C2OptimizerNode *root, unsigned count, unsigned alias_count, unsigned register_count, int aliases);
C2Expression * __fastcall finish_optimizer_records(C2Record *first, unsigned limit, unsigned original_limit, int aliases);
void __fastcall emit_optimizer_records(C2Expression *records, void *register_map, unsigned limit);
unsigned __fastcall collect_optimizer_symbols(C2RegisterSet *used, C2RegisterSet *defined);
C2RegisterSet * __fastcall copy_optimizer_symbol_set(C2RegisterSet *source);
void __fastcall merge_optimizer_symbol_sets(C2RegisterSet *destination, C2RegisterSet *source);
C2OptimizerNode * __fastcall map_optimizer_records(C2BasicBlock *block, unsigned count, C2RegisterSet *symbols, C2OptimizerNode **table, C2RegisterSet *copy);
void __fastcall set_optimizer_record_mode(C2BasicBlock *block, int mode);
void __fastcall reset_block_analysis(C2BasicBlock *block);
C2OptimizerContext * __fastcall build_optimizer_symbol_map(C2OptimizerNode *root, C2RegisterSet *symbols, unsigned count, C2OptimizerNode **table);
C2OptimizerNode * __fastcall apply_optimizer_symbol_map(C2BasicBlock *block, C2OptimizerContext *context, unsigned count, C2RegisterSet *used, C2RegisterSet *defined, C2OptimizerNode **table, C2RegisterSet *copy);
void __fastcall release_optimizer_symbol_set(C2RegisterSet *set);
void __fastcall finish_optimizer_symbol_map(C2OptimizerContext *context, C2OptimizerNode *root, unsigned register_count);
C2Expression *__fastcall get_optimizer_handler_body(C2Expression *record);
void __fastcall mark_optimizer_register_range(C2Expression *first,
    C2Expression *last, int mark, int nested);
void __fastcall scan_optimizer_clauses(C2InputClause *clauses);
struct C2OptimizerRegisterInfo {
    C2RegisterSet *aliases;
    unsigned number;
    unsigned char pending08[8];
    C2OptimizerRegisterInfo *secondary;
};
typedef char C2OptimizerInfoSizeCheck[sizeof(C2OptimizerRegisterInfo) == 20 ? 1 : -1];
C2OptimizerUse *__fastcall add_optimizer_use(C2OptimizerUse *head,
    C2FunctionRegister *reg, int unique);
#endif

#ifndef C2_OPTIMIZER_H
#define C2_OPTIMIZER_H
#include "flow.h"
#include "register-sets.h"
#include <recon.h>

typedef struct C2OptimizerContext C2OptimizerContext;
typedef struct C2OptimizerNode C2OptimizerNode;
unsigned __fastcall count_optimizer_records(C2Record *first);
C2OptimizerContext * __fastcall new_optimizer_context(int aliases, int mode);
void __fastcall prepare_alias_input(C2Function *function);
C2Expression * __fastcall build_optimizer_input(C2Record *first, C2OptimizerContext *context);
C2OptimizerNode * __fastcall collect_optimizer_blocks(C2BasicBlock *block, C2OptimizerContext *context, C2Expression *records);
C2OptimizerNode * __fastcall translate_optimizer_input(C2BasicBlock *block, unsigned count, int aliases, int no_global);
C2Expression * __fastcall fold_optimizer_records(C2Expression *records);
void __fastcall prepare_optimizer_values(C2Expression *records);
void __fastcall analyze_optimizer_values(C2OptimizerContext *context, C2Expression *records, int aliases);
void __fastcall rewrite_optimizer_records(C2Expression *records);
void initialize_optimizer_registers(void);
void number_optimizer_registers(void);
void __fastcall build_optimizer_register_sets(C2OptimizerNode *root, unsigned count, C2OptimizerContext *context, void *map, int aliases);
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
void __fastcall finish_optimizer_symbol_map(C2OptimizerContext *context, C2OptimizerNode *root, void *map);
#endif

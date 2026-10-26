#ifndef C2_OPTIMIZER_H
#define C2_OPTIMIZER_H
#include "flow.h"
#include "register-sets.h"
#include "code-input.h"
#include <recon.h>

typedef struct C2OptimizerContext C2OptimizerContext;
typedef struct C2OptimizerNode C2OptimizerNode;
struct C2OptimizerUse {
    C2OptimizerUse *next;
    C2FunctionRegister *reg;
};
typedef struct C2OptimizerAlias {
    C2FunctionRegister *reg;
    union {
        unsigned char pending04[8];
        struct { C2OptimizerUse *uses; C2RegisterSet *effects; };
    };
    C2Expression *record;
    union { unsigned pending10, address_exposed; };
} C2OptimizerAlias;
typedef char C2OptimizerAliasSizeCheck[sizeof(C2OptimizerAlias) == 20 ? 1 : -1];
/* Prefix views for the scope information this pass actually reads. Their
 * remaining allocation/defaults are not inferred from these consumers. */
typedef struct C2OptimizerScope {
    unsigned pending00, kind;
    union {
        unsigned char pending08[20];
        struct {
            struct C2OptimizerScope *parent;
            unsigned limit;
            unsigned char category, pending11[3];
            unsigned serial, pending18;
        };
    };
    C2FunctionSymbol *symbol;
    unsigned char pending20[8];
    C2FunctionSymbol *alternate;
    union {
        unsigned char pending2c[8];
        struct { unsigned pending2c_word; C2Expression *end; };
    };
    C2Function *handler_function;
    union { struct C2OptimizerRecordLink *targets; unsigned flags; };
    C2OptimizerUse *uses;
    unsigned optimizer_index;
} C2OptimizerScope;
typedef struct C2OptimizerCapture {
    unsigned char pending00[16];
    C2FunctionSymbol *symbol;
} C2OptimizerCapture;
unsigned __fastcall count_optimizer_records(C2Record *first);
unsigned __fastcall assign_optimizer_registers(int aliases, int mode);
void __fastcall prepare_alias_input(C2Function *function);
unsigned __fastcall number_optimizer_operands(C2Record *first, unsigned alias_count);
C2OptimizerAlias * __fastcall build_optimizer_aliases(C2BasicBlock *block, unsigned alias_count, unsigned operand_limit);
unsigned __fastcall translate_optimizer_input(C2BasicBlock *block, unsigned count, int aliases, int no_global);
unsigned __fastcall fold_optimizer_records(unsigned operand_limit);
void __fastcall prepare_optimizer_values(unsigned operand_limit);
void __fastcall analyze_optimizer_values(unsigned alias_count, unsigned operand_limit, int aliases);
void __fastcall rewrite_optimizer_records(unsigned operand_limit);
void initialize_optimizer_registers(void);
void number_optimizer_registers(void);
void __fastcall build_optimizer_register_sets(unsigned expression_limit, unsigned count, unsigned alias_count, unsigned register_count, int aliases);
C2RegisterSet * __fastcall finish_optimizer_records(C2Record *first, unsigned limit, unsigned original_limit, int aliases);
void __fastcall build_optimizer_conflicts(C2RegisterSet *records, unsigned final_index_limit, unsigned limit);
unsigned __fastcall collect_optimizer_symbols(C2RegisterSet *used, C2RegisterSet *defined);
C2RegisterSet * __fastcall copy_optimizer_symbol_set(C2RegisterSet *source);
void __fastcall subtract_optimizer_symbol_sets(C2RegisterSet *destination, C2RegisterSet *source);
C2OptimizerNode * __fastcall map_optimizer_records(C2BasicBlock *block, unsigned count, C2RegisterSet *symbols, C2OptimizerNode **table, C2RegisterSet *copy);
void __fastcall solve_optimizer_alias_sets(C2BasicBlock *block, int mode);
void __fastcall solve_allocation_liveness(C2BasicBlock *block);
C2OptimizerContext * __fastcall build_optimizer_symbol_map(C2OptimizerNode *root, C2RegisterSet *symbols, unsigned count, C2OptimizerNode **table);
C2OptimizerNode * __fastcall apply_optimizer_symbol_map(C2BasicBlock *block, C2OptimizerContext *context, unsigned count, C2RegisterSet *used, C2RegisterSet *defined, C2OptimizerNode **table, C2RegisterSet *copy);
void __fastcall release_optimizer_symbol_set(C2RegisterSet *set);
void __fastcall release_optimizer_set_header(C2RegisterSet *set);
void __fastcall finish_optimizer_symbol_map(C2OptimizerContext *context, C2OptimizerNode *root, unsigned register_count);
C2Expression *__fastcall get_optimizer_handler_body(C2Expression *record);
void __fastcall mark_optimizer_register_range(C2Expression *first,
    C2Expression *last, int mark, int nested);
void __fastcall scan_optimizer_clauses(C2InputClause *clauses);
struct C2OptimizerRegisterInfo {
    C2RegisterSet *aliases;
    unsigned number;
    union {
        unsigned char pending08[8];
        struct { struct C2OptimizerValue *values; C2FunctionRegister *owner; };
    };
    C2OptimizerRegisterInfo *secondary;
};
typedef char C2OptimizerInfoSizeCheck[sizeof(C2OptimizerRegisterInfo) == 20 ? 1 : -1];
C2OptimizerUse *__fastcall add_optimizer_use(C2OptimizerUse *head,
    C2FunctionRegister *reg, int pool);
void __fastcall merge_optimizer_symbol_sets(C2RegisterSet *destination,
    C2RegisterSet *source);
void __fastcall remove_register_set_node(C2RegisterSet *set,
    C2RegisterSetNode *node);
void __fastcall intersect_optimizer_symbol_sets(C2RegisterSet *destination,
    C2RegisterSet *source);
void __fastcall copy_optimizer_symbol_set_into(C2RegisterSet *destination,
    C2RegisterSet *source);
void __fastcall update_optimizer_alias_set(C2RegisterSet *destination,
    C2RegisterSet *input, C2RegisterSet *kills, C2RegisterSet *uses);
int __fastcall optimizer_symbol_sets_equal(C2RegisterSet *first,
    C2RegisterSet *second);
C2RegisterSetNode *__fastcall insert_register_set_node(C2RegisterSet *set,
    C2RegisterSetNode *after);
void __fastcall copy_optimizer_symbol_difference(C2RegisterSet *destination,
    C2RegisterSet *input, C2RegisterSet *kills);
unsigned __fastcall next_optimizer_symbol(C2RegisterSet *set);
int __fastcall compare_input_operands(C2Expression *left, C2Expression *right);
int __fastcall operation_is_associative(unsigned opcode);
#endif

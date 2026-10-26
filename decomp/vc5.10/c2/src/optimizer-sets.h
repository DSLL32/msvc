#ifndef C2_OPTIMIZER_SETS_H
#define C2_OPTIMIZER_SETS_H
#include "optimizer-translate.h"
#include <recon.h>

/* Consumer prefix for a register's deferred optimizer values. Allocation
 * ownership and any further fields are left to its actual producer. */
typedef struct C2OptimizerValue {
    struct C2OptimizerValue *next;
    unsigned number;
    C2RegisterSet *effects;
    union { C2FunctionRegister *reg; C2BasicBlock *block; };
    unsigned value_bytes;
} C2OptimizerValue;
typedef char C2OptimizerValueSize[sizeof(C2OptimizerValue) == 20 ? 1 : -1];
typedef char C2OptimizerValueBlockOffset[offsetof(C2OptimizerValue, block) == 12 ? 1 : -1];

extern C2OptimizerUse **optimizer_index_uses;
extern C2RegisterSet **optimizer_index_sets;
extern C2RegisterSet *optimizer_alias_registers;
C2RegisterSet *__fastcall new_optimizer_dense_set(unsigned pool, unsigned count);
C2RegisterSet *__fastcall initialize_optimizer_dense_set(C2RegisterSet *set,
    unsigned pool, unsigned count);
void __fastcall add_optimizer_dense_member(C2RegisterSet *set, unsigned number);
void __fastcall merge_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source);
void __fastcall subtract_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source);
void __fastcall build_optimizer_register_state(C2FunctionRegister *reg,
    unsigned alias_count, unsigned register_count, int aliases);
void __fastcall prepare_optimizer_register_aliases(C2FunctionRegister *reg,
    unsigned register_count);
void __fastcall fill_optimizer_dense_uses(C2OptimizerUse *uses, C2RegisterSet *set,
    unsigned register_count);
C2FunctionRegister *__fastcall find_optimizer_register_variant(C2FunctionRegister *reg,
    long displacement);
void __fastcall add_optimizer_register_range(C2FunctionRegister *reg, C2RegisterSet *set,
    long displacement, unsigned bytes);
int __fastcall get_optimizer_operand_range(C2Expression *operand, long *displacement, unsigned *bytes);
int __fastcall optimizer_dense_sets_equal(C2RegisterSet *first, C2RegisterSet *second);
int __fastcall optimizer_indices_overlap(unsigned first, unsigned second);
void __fastcall fill_optimizer_dense_set(C2RegisterSet *set, unsigned count);
void __fastcall intersect_optimizer_dense_sets(C2RegisterSet *destination, C2RegisterSet *source);
void __fastcall copy_optimizer_dense_set(C2RegisterSet *destination, C2RegisterSet *source);
int __fastcall optimizer_dense_contains(C2RegisterSet *set, unsigned number);

#endif

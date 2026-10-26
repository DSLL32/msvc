#ifndef C2_GLOBAL_EXPRESSIONS_H
#define C2_GLOBAL_EXPRESSIONS_H
#include "global-optimizer.h"
#include <recon.h>

/* The hash chain's encountered fields. The actual factory allocates 24 bytes;
 * the final four and the type-adjacent two bytes have no semantic role here. */
struct C2GlobalExpression {
    C2GlobalExpression *next;
    unsigned opcode;
    unsigned short type;
    unsigned char pending0a[2];
    C2Expression *operands;
    C2FunctionRegister *reg;
    unsigned char pending14[4];
};
typedef char C2GlobalExpressionRegisterOffset[
    offsetof(C2GlobalExpression, reg) == 16 ? 1 : -1];
extern C2GlobalExpression *global_expression_buckets[101];
extern unsigned global_expression_count, global_expression_cursor;
extern C2FunctionRegister *global_cached_expression_registers;
C2FunctionRegister *__fastcall new_global_result_register(unsigned short type);
C2Expression *clone_global_expression_operands(void);
struct C2GlobalDependency {
    struct C2GlobalDependency *next_dependent, *next_input;
    C2FunctionRegister *dependent, *input;
};
typedef struct C2GlobalDependency C2GlobalDependency;
struct C2GlobalRegisterLink {
    struct C2GlobalRegisterLink *next;
    C2FunctionRegister *reg;
};
typedef struct C2GlobalRegisterLink C2GlobalRegisterLink;
typedef char C2GlobalDependencySize[sizeof(C2GlobalDependency) == 16 ? 1 : -1];
typedef char C2GlobalRegisterLinkSize[sizeof(C2GlobalRegisterLink) == 8 ? 1 : -1];
typedef char C2GlobalDependentsOffset[offsetof(C2FunctionRegister, global_dependents) == 52 ? 1 : -1];
typedef char C2GlobalDependenciesOffset[offsetof(C2FunctionRegister, global_dependencies) == 56 ? 1 : -1];
typedef char C2GlobalAssignmentsOffset[offsetof(C2FunctionRegister, global_assignments) == 60 ? 1 : -1];
typedef char C2GlobalDependencySetOffset[offsetof(C2FunctionRegister, global_dependency_set) == 64 ? 1 : -1];
extern C2RegisterSet *global_pending_expressions;
void __fastcall propagate_global_dependencies(C2FunctionRegister *reg);
extern C2RegisterSet **global_index_registers, **global_symbol_registers;
unsigned __fastcall merge_global_dependency_sets(C2RegisterSet *destination, C2RegisterSet *source);
void __fastcall link_global_expression_dependencies(C2FunctionRegister *reg);
#endif

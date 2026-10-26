#ifndef C2_OPTIMIZER_TRANSLATE_H
#define C2_OPTIMIZER_TRANSLATE_H
#include "optimizer.h"
#include <recon.h>

typedef struct C2OptimizerConflict {
    struct C2OptimizerConflict *next;
    unsigned number;
} C2OptimizerConflict;
extern C2OptimizerConflict **optimizer_conflicts;
extern C2RegisterSet *optimizer_live_registers;

typedef struct C2OptimizerIndex {
    unsigned number;
    C2FunctionRegister *parameter;
} C2OptimizerIndex;
typedef struct C2OptimizerIndexMap {
    struct C2OptimizerIndexMap *next;
    unsigned original, replacement;
} C2OptimizerIndexMap;
typedef struct C2OptimizerRecordLink {
    struct C2OptimizerRecordLink *next;
    C2Expression *record;
} C2OptimizerRecordLink;
typedef struct C2OptimizerCallState {
    struct C2OptimizerCallState *next;
    C2RegisterSet *registers;
    C2FunctionRegister *parameter;
    C2OptimizerUse *additional_uses;
    unsigned number;
} C2OptimizerCallState;
typedef struct C2OptimizerScopeState {
    struct C2OptimizerScopeState *next;
    C2OptimizerUse *inputs, *outputs;
    unsigned input_index, output_index;
    C2FunctionRegister *parameter;
} C2OptimizerScopeState;
/* Consumer prefixes for exception records. These views do not claim an
 * allocation extent or defaults beyond the fields accessed by this pass. */
typedef struct C2OptimizerCaptureRecord {
    struct C2OptimizerCaptureRecord *next;
    unsigned char pending04[12];
    C2InputSymbol *declaration;
    C2Symbol *symbol;
} C2OptimizerCaptureRecord;
typedef struct C2OptimizerExceptionContext {
    unsigned char pending00[32];
    C2OptimizerCaptureRecord *captures;
    unsigned pending24;
    C2Expression *first;
} C2OptimizerExceptionContext;
typedef struct C2OptimizerHandler {
    union { C2Expression *body; C2FunctionRegister *reg; };
    C2OptimizerScope *scope;
    unsigned kind;
    C2OptimizerExceptionContext *context;
} C2OptimizerHandler;
typedef struct C2OptimizerCaptureState {
    unsigned char pending00[8];
    C2FunctionRegister *reg;
} C2OptimizerCaptureState;

typedef char C2OptimizerUseSize[sizeof(C2OptimizerUse) == 8 ? 1 : -1];
typedef char C2OptimizerIndexSize[sizeof(C2OptimizerIndex) == 8 ? 1 : -1];
typedef char C2OptimizerScopeStateSize[sizeof(C2OptimizerScopeState) == 24 ? 1 : -1];
typedef char C2OptimizerCallStateSize[sizeof(C2OptimizerCallState) == 20 ? 1 : -1];
typedef char C2OptimizerScopeHandlerOffset[offsetof(C2OptimizerScope, handler_function) == 52 ? 1 : -1];
extern C2OptimizerCallState *optimizer_call_states;

C2Expression *__fastcall new_optimizer_index_operand(unsigned number);
void __fastcall attach_optimizer_output(C2Expression *record, C2Expression *value);
void __fastcall collect_optimizer_result_uses(C2Expression *record, int aliases,
    C2FunctionRegister *parameter);
void __fastcall mark_optimizer_operand(C2Expression *operand, C2RegisterSet *active);
C2Expression *__fastcall translate_optimizer_operand(C2Expression *operand,
    C2RegisterSet *active, unsigned number, C2BasicBlock *block);
unsigned __fastcall resolve_optimizer_memory(C2FunctionRegister *reg,
    C2RegisterSet *active, unsigned number, C2BasicBlock *block);
unsigned __fastcall resolve_optimizer_alias(C2Expression *operand,
    C2RegisterSet *active, unsigned number);
int __fastcall compare_optimizer_operand(C2Expression *operand,
    C2RegisterSet *active, C2OptimizerAlias *alias);
int __fastcall compare_optimizer_alias(C2FunctionRegister *reg,
    C2Expression *operand, C2RegisterSet *active, void *context);
void __fastcall collect_optimizer_call_uses(C2Expression *operand,
    C2OptimizerCallState *state, C2RegisterSet *active);
void __fastcall finish_optimizer_record(C2Expression *record, C2RegisterSet *active);
void __fastcall collect_optimizer_scope_uses(C2Expression *first,
    C2Expression *last, int inputs, int outputs, C2OptimizerScopeState *state);
C2OptimizerUse *__fastcall merge_optimizer_uses(C2OptimizerUse *destination,
    C2OptimizerUse *source, int pool);
C2OptimizerCaptureState *current_optimizer_capture(void);
void __fastcall add_optimizer_exception_indices(C2Expression *record,
    C2OptimizerScope *scope, unsigned number, C2FunctionRegister *parameter);
C2OptimizerScope *__fastcall lookup_optimizer_scope(C2OptimizerScope *index,
    unsigned serial);
void __fastcall remove_optimizer_symbol(C2RegisterSet *set, unsigned number);
unsigned __fastcall count_optimizer_symbols(C2RegisterSet *set);
C2Expression *__fastcall follow_optimizer_result(C2Expression *record,
    C2Expression **operand);
C2RegisterSet *__fastcall collect_optimizer_register_aliases(C2FunctionRegister *reg,
    C2RegisterSet *active);
C2Expression *__fastcall find_register_use(C2Expression *record,
    C2FunctionRegister *reg, C2Expression **operand);
C2Expression *__fastcall find_register_input(C2Expression *record,
    C2FunctionRegister *reg);
#endif

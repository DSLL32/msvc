#ifndef LOOP_OUTPUT_H
#define LOOP_OUTPUT_H
#include "parser.h"
#include "work-list.h"
#include <recon.h>

/* Switch-case dispatch lists share the label-stack payload slot. */
typedef struct SwitchCaseEntry {
    struct SwitchCaseEntry *next;
    unsigned value, label_index;
} SwitchCaseEntry;
typedef char SwitchCaseEntrySizeMustBe12[(sizeof(SwitchCaseEntry) == 12) ? 1 : -1];
typedef char SwitchCaseValueOffsetMustBe4[(offsetof(SwitchCaseEntry, value) == 4) ? 1 : -1];
typedef char SwitchCaseLabelOffsetMustBe8[(offsetof(SwitchCaseEntry, label_index) == 8) ? 1 : -1];
/* Compact qc operands preserve either an integer/symbol word or a pair
   of indexed operands. Original allocations establish8B and36B records. */
typedef struct QcOperand {
    unsigned char operation;
    union { unsigned integer; CppSymbol *symbol; struct QcOperand *pair; } value;
} QcOperand;
typedef struct QcLoopState {
    unsigned char condition, increment;
    QcOperand left, right, target, step;
} QcLoopState;
typedef char QcOperandSizeMustBe8[(sizeof(QcOperand) == 8) ? 1 : -1];
typedef char QcOperandValueOffsetMustBe4[(offsetof(QcOperand, value) == 4) ? 1 : -1];
typedef char QcLoopSizeMustBe36[(sizeof(QcLoopState) == 36) ? 1 : -1];
int __stdcall describe_qc_operand(IrToken *expression, QcOperand *operand);
extern LabelStack *qc_loop_markers;
extern WorkList *qc_loop_analyses;
void __stdcall push_loop_label(LabelStack **stack, unsigned attributes);
void finish_qc_loop(void);
int __stdcall emit_qc_loop_control(IrToken *condition, IrToken *increment,
    WorkList *analysis);
#endif

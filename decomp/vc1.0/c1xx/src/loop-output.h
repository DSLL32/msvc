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
extern LabelStack *qc_loop_markers;
extern WorkList *qc_loop_analyses;
void __stdcall push_loop_label(LabelStack **stack, unsigned attributes);
void finish_qc_loop(void);
int __stdcall emit_qc_loop_control(IrToken *condition, IrToken *increment,
    WorkList *analysis);
#endif

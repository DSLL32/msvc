#ifndef LOOP_OUTPUT_H
#define LOOP_OUTPUT_H
#include "parser.h"
#include "work-list.h"
#include <recon.h>

extern LabelStack *qc_loop_markers;
extern WorkList *qc_loop_analyses;
void __stdcall push_loop_label(LabelStack **stack, unsigned attributes);
void finish_qc_loop(void);
int __stdcall emit_qc_loop_control(IrToken *condition, IrToken *increment,
    WorkList *analysis);
#endif

/* Structured-exception statements lowered into frontend IR. */
#include "parser.h"
#include "ir.h"
#include "loop-output.h"
#include "function-definitions.h"
#include "operators.h"
#include "diagnostics.h"
#include <recon.h>

void adjust_seh_label_depths(void);

LabelStack *seh_exit_labels; /* original0x91574, independent protected exit head */

/* Original0x50d20..0x50d6f, cdecl. Each protected body owns a leave label;
   function attributes request backend exception-state support. */
void begin_seh_try(void)
{
    if (function_symbol->storage[2] & 1)
        report_syntax_error(490,token_spellings[178]);
    write_ir_record(0x8f,0);
    function_symbol->attributes |= 0x1100;
    ++seh_try_depth;
    push_loop_label(&seh_exit_labels,26);
}

/* Original0x50c80..0x50c9a and0x50ca0..0x50cae, cdecl. __leave
   targets the innermost protected statement's common completion label. */
void emit_seh_leave(void)
{
    if (seh_exit_labels)
        write_ir_record(0x3a,seh_exit_labels->label);
    else
        report_syntax_error(703);
}

/* Original0x50d70..0x50da5, cdecl. Move label references outward before
   dropping protected depth and defining the common normal/__leave exit. */
void end_seh_try(void)
{
    adjust_seh_label_depths();
    --seh_try_depth;
    emit_label(seh_exit_labels->label);
    seh_exit_labels = seh_exit_labels->next;
    if (!--label_stack_count) release_arena(3);
}

/* Original0x3b680..0x3b6cf, cdecl. Defined labels gain an entry barrier;
   unresolved references move outward by one protected nesting level. */
void adjust_seh_label_depths(void)
{
    CppSymbol *label;
    int depth;
    for (label = label_symbols; label; label = label->next) {
        depth = seh_try_depth + seh_except_depth + seh_finally_depth;
        if (depth <= label->auxiliary.reference.seh_depth) {
            if (label->flags & 4) label->attributes |= 0x200;
            else --label->auxiliary.reference.seh_depth;
        }
    }
}

/* Original0x50df0..0x50dfa, cdecl. The lexer exposes exception intrinsics
   only while parsing an __except filter; this is the existing flag object's
   high-word0x0400 bit, not a separate scanner state. */
void begin_seh_filter(void)
{
    cpp_parser_flags |= 0x04000000UL;
}

/* Original0x50cb0..0x50cfa, stdcall4. A filter's expression precedes
   its handler marker; an illegal finally nesting still enters except depth. */
void __stdcall emit_seh_filter(IrToken *expression)
{
    if (seh_finally_depth > 0) report_syntax_error(702);
    else {
        emit_expression_tree(expression);
        write_ir_record(0x8d,expression);
    }
    ++seh_except_depth;
}

/* Original0x50e00..0x50e0a, cdecl. Restore ordinary lexical context after
   the exception filter has been parsed and serialized. */
void end_seh_filter(void)
{
    cpp_parser_flags &= ~0x04000000UL;
}

/* Original0x50db0..0x50dcd, cdecl. Handler completion moves references
   outward before closing its depth and writing the common handler-end tag. */
void end_seh_except(void)
{
    adjust_seh_label_depths();
    --seh_except_depth;
    write_ir_record(0x90,0);
}

/* Original0x50d00..0x50d18, cdecl. Finally handlers have their own
   nesting depth for the filter-nesting diagnostics and label barriers. */
void begin_seh_finally(void)
{
    write_ir_record(0x8e,0);
    ++seh_finally_depth;
}

/* Original0x50dd0..0x50ded, cdecl. Finally completion shares label
   adjustment and the handler-end tag while dropping its separate depth. */
void end_seh_finally(void)
{
    adjust_seh_label_depths();
    --seh_finally_depth;
    write_ir_record(0x90,0);
}

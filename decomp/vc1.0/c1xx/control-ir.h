#ifndef CONTROL_IR_H
#define CONTROL_IR_H
#include "source-input.h"
/* Logical fields of source/option control records. Ignored fields belong to
   the shared native descriptor vocabulary but have no action in this writer. */
enum ControlField {
    CONTROL_END = 0, CONTROL_LINE = 1, CONTROL_SOURCE_FILE = 3,
    CONTROL_OPTIMIZATION = 7, CONTROL_CONTEXT = 8,
    CONTROL_PLMN = 9, CONTROL_PLMF = 10, CONTROL_IGNORED_11 = 11,
    CONTROL_COUNTED_DATA = 12, CONTROL_FUNCTION_FLAGS = 13,
    CONTROL_WORD = 14, CONTROL_IGNORED_18 = 18, CONTROL_IGNORED_19 = 19,
    CONTROL_OPTION_BITS = 20, CONTROL_ALIGNMENT = 21, CONTROL_WARNINGS = 23,
    CONTROL_IGNORED_98 = 98, CONTROL_IGNORED_113 = 113
};
typedef char PragmaValueSizeMustBe12[(sizeof(PragmaValue) == 12) ? 1 : -1];
extern unsigned char *control_record_formats[36];
extern CppSymbol *record_context_symbol;
extern unsigned pending_pragma_kind;
extern unsigned char *pending_pragma_text;
extern unsigned char function_control_flags;
extern unsigned short control_word, control_alignment, control_alignment_mode;
/* Only the signed count prefix is established by the encountered consumer;
   its still-unreconstructed producer will establish the remaining capacity. */
extern char control_counted_data[1];
void write_counted_ir(short length, char *text);
void __stdcall emit_warning_controls(unsigned tag);
#endif

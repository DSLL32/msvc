#ifndef CONTROL_IR_H
#define CONTROL_IR_H
#include "source-input.h"
#include <recon.h>
/* Control-record schemas retain their logical fields, not the original
   combined table's alternate output descriptors. */
enum ControlField {
    CONTROL_END, CONTROL_LINE, CONTROL_IGNORED_2, CONTROL_SOURCE_FILE,
    CONTROL_IGNORED_4, CONTROL_IGNORED_5, CONTROL_IGNORED_6,
    CONTROL_OPTIMIZATION, CONTROL_CONTEXT, CONTROL_PLMN, CONTROL_PLMF,
    CONTROL_IGNORED_11, CONTROL_COUNTED_NAME, CONTROL_FUNCTION_FLAGS,
    CONTROL_WORD, CONTROL_IGNORED_15, CONTROL_IGNORED_16,
    CONTROL_IGNORED_17, CONTROL_IGNORED_18, CONTROL_IGNORED_19,
    CONTROL_OPTION_BITS, CONTROL_ALIGNMENT, CONTROL_IGNORED_22,
    CONTROL_WARNINGS
};
extern unsigned char *control_record_formats[36];
extern FrontendSymbol *record_context_symbol;
extern unsigned pending_pragma_kind;
extern unsigned char *pending_pragma_text;
extern unsigned char function_control_flags;
extern unsigned short control_word;
extern unsigned control_alignment, control_alignment_mode;
extern char control_counted_name[1];
void write_counted_ir(short length, char *text);
void __stdcall emit_warning_controls(unsigned tag);
#endif

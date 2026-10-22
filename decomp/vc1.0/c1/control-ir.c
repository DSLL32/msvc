/* Expression-stream source and option control records. */
#include "control-ir.h"
#include "parser.h"

FrontendSymbol *record_context_symbol; /* original 0x6b6c4 */
unsigned pending_pragma_kind; /* original 0x6b144 */
unsigned char *pending_pragma_text; /* original 0x6b148 */
unsigned char function_control_flags; /* original 0x60058 */
unsigned short control_word; /* original 0x5b020 */
unsigned control_alignment = 8, control_alignment_mode; /* 0x6bfa0 / 0x6bfa4 */
char control_counted_name[152]; /* original 0x5b0a4..0x5b13c */

#define CONTROL_PUT_WORD(file, value) do { \
    FILE *s_ = (file); short w_ = (value); \
    if ((s_->_cnt -= 2) >= 0) { *(short *)s_->_ptr = w_; s_->_ptr += 2; } \
    else { s_->_cnt += 2; fwrite(&w_, 2, 1, s_); } \
} while (0)
#define CONTROL_PUT_LONG(file, value) do { \
    FILE *s_ = (file); unsigned v_ = (value); \
    if ((s_->_cnt -= 4) >= 0) { *(unsigned *)s_->_ptr = v_; s_->_ptr += 4; } \
    else { s_->_cnt += 4; fwrite(&v_, 4, 1, s_); } \
} while (0)

/* Original 0x3a690..0x3ad2c, dispatch table 0x3ad2c..0x3ad88.
   Tag 11 defers a pragma; other tags serialize the associated control state. */
void __stdcall record_line_information(unsigned tag)
{
    unsigned char *field;
    PragmaValue *pragma;
    FILE *stream;
    short word;
    unsigned value;
    if (options.prototypes_only) return;
    if (tag == 1) {
        if (parser_flags & 0x200) return;
        if ((int)source_line_number > 65535) {
            compiler_warning(1, 49);
            return;
        }
    } else if (tag == 11) {
        pragma = allocate_permanent(sizeof(PragmaValue));
        pragma->next = pragma_values;
        pragma->kind = pending_pragma_kind;
        pending_pragma_kind = 7;
        pragma->value = copy_arena_bytes(pending_pragma_text, 0, pending_pragma_text[0] + 1);
        pragma_values = pragma;
        return;
    }
    if (tag != 0x23) {
        putc(0x4f, expression_file);
        putc(tag, expression_file);
    }
    field = control_record_formats[tag];
    if (!field) return;
    while (*field) {
        switch (*field++) {
        case CONTROL_LINE:
            word = (short)source_line_number;
            if (word >= -127 && word <= 127) putc((signed char)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_PUT_WORD(expression_file, word); }
            break;
        case CONTROL_SOURCE_FILE:
            if (*source_file_index == 65535) write_global_record(18, 0);
            auxiliary_ir_stream = expression_file;
            write_ir_index(source_file_index);
            break;
        case CONTROL_OPTIMIZATION:
            putc(optimization_enabled, expression_file);
            break;
        case CONTROL_CONTEXT:
            value = record_context_symbol ? record_context_symbol->index : 0;
            putc((signed char)value, expression_file);
            putc((signed char)(value >> 8), expression_file);
            break;
        case CONTROL_PLMN:
            putc(normalized_PLMN, expression_file);
            break;
        case CONTROL_PLMF:
            putc(normalized_PLMF, expression_file);
            break;
        case CONTROL_COUNTED_NAME:
            auxiliary_ir_stream = expression_file;
            write_counted_ir((signed char)control_counted_name[0], control_counted_name);
            break;
        case CONTROL_FUNCTION_FLAGS:
            putc(function_control_flags, expression_file);
            break;
        case CONTROL_WORD:
            word = (short)control_word;
            if (word >= -127 && word <= 127) putc((signed char)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_PUT_WORD(expression_file, word); }
            break;
        case CONTROL_OPTION_BITS:
            normalized_Oa |= normalized_Ow;
            value = pack_option_flags();
            stream = expression_file;
            if ((long)value >= -127 && (long)value <= 127) putc((signed char)value, stream);
            else { putc(0x80, stream); CONTROL_PUT_LONG(stream, value); }
            break;
        case CONTROL_ALIGNMENT:
            word = (short)((unsigned char)control_alignment | ((unsigned char)control_alignment_mode << 8));
            if (word >= -127 && word <= 127) putc((signed char)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_PUT_WORD(expression_file, word); }
            break;
        case CONTROL_WARNINGS:
            emit_warning_controls(tag);
            break;
        }
    }
}

/* Original 0x3ad90..0x3af86. The warning range contains only command-line
   overrides; a single record header precedes its compact list and sentinel. */
void __stdcall emit_warning_controls(unsigned tag)
{
    unsigned warning;
    int flags, emitted = 0;
    short code;
    for (warning = lowest_warning_700; warning <= highest_warning_700; ++warning) {
        flags = options.option_E ? 13 : (signed char)warning_levels[warning];
        if (!flags) continue;
        if (!emitted) {
            putc(0x4f, expression_file);
            putc(tag, expression_file);
            emitted = 1;
        }
        code = (short)(warning - 700);
        if (code >= -127 && code <= 127) putc((signed char)code, expression_file);
        else { putc(0x80, expression_file); CONTROL_PUT_WORD(expression_file, code); }
        putc((signed char)flags, expression_file);
    }
    if (emitted) putc(-1, expression_file);
}

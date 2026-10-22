/* Source, pragma and option records in the expression intermediate stream. */
#include "control-ir.h"
#include "parser.h"

CppSymbol *record_context_symbol; /* original 0x9f25c */
unsigned pending_pragma_kind; /* original 0x9e54c */
unsigned char *pending_pragma_text; /* original 0x9e550 */
unsigned char function_control_flags; /* original 0x91378 */
unsigned short control_word; /* original 0x8c020 */
unsigned short control_alignment = 8; /* original 0x9fbf4 */
unsigned short control_alignment_mode; /* original 0x9fbf8 */
char control_counted_data[1]; /* original count byte at 0x8c0a4 */

#define CONTROL_RAW(stream_, type_, value_) do { \
    FILE *out_ = (stream_); type_ raw_ = (value_); \
    out_->_cnt -= sizeof(type_); \
    if (out_->_cnt >= 0) { *(type_ *)out_->_ptr = raw_; out_->_ptr += sizeof(type_); } \
    else { out_->_cnt += sizeof(type_); fwrite(&raw_, sizeof(type_), 1, out_); } \
} while (0)

/* Original code 0x5b0f0..0x5b7db, switch data 0x5b7dc..0x5b837. */
void __stdcall record_line_information(unsigned tag)
{
    unsigned char *field;
    PragmaValue *pragma;
    FILE *stream;
    short word;
    unsigned value;
    if (options.prototypes_only) return;
    if (tag == 1) {
        if (cpp_parser_flags & 0x200) return;
        if ((long)source_line_number > 65535) {
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
            word = (long)source_line_number <= 0 ? 0 : (short)source_line_number;
            if (word >= -127 && word <= 127) putc((int)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_RAW(expression_file, short, word); }
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
        case CONTROL_COUNTED_DATA:
            auxiliary_ir_stream = expression_file;
            write_counted_ir((signed char)control_counted_data[0], control_counted_data);
            break;
        case CONTROL_FUNCTION_FLAGS:
            putc(function_control_flags, expression_file);
            break;
        case CONTROL_WORD:
            word = (short)control_word;
            if (word >= -127 && word <= 127) putc((int)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_RAW(expression_file, short, word); }
            break;
        case CONTROL_OPTION_BITS:
            normalized_Oa |= normalized_Ow;
            value = pack_option_flags();
            stream = expression_file;
            if ((long)value >= -127 && (long)value <= 127) putc((signed char)value, stream);
            else { putc(0x80, stream); CONTROL_RAW(stream, unsigned, value); }
            break;
        case CONTROL_ALIGNMENT:
            word = (short)((unsigned char)control_alignment | ((unsigned char)control_alignment_mode << 8));
            if (word >= -127 && word <= 127) putc((int)word, expression_file);
            else { putc(0x80, expression_file); CONTROL_RAW(expression_file, short, word); }
            break;
        case CONTROL_WARNINGS:
            emit_warning_controls(tag);
            break;
        }
    }
}

/* Original 0x5b840..0x5ba35. Emit one header for the command-line
   warning override range and terminate a nonempty list with -1. */
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
        if (code >= -127 && code <= 127) putc((int)code, expression_file);
        else { putc(0x80, expression_file); CONTROL_RAW(expression_file, short, code); }
        putc(flags, expression_file);
    }
    if (emitted) putc(-1, expression_file);
}

#include <io.h>
#include <string.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#include "pragma.h"
#pragma intrinsic(strlen)
#include <recon.h>

/* The encountered control-record format column at RVA653c8. Each nonNULL
 * format has exactly one operand tag; numbered unused tags remain explicit. */
enum ControlOperand {
    CONTROL_NONE = 0, CONTROL_CONTEXT = 1, CONTROL_LINE_DELTA = 2,
    CONTROL_SOURCE = 3, CONTROL_TEXT = 12, CONTROL_BYTE = 13,
    CONTROL_SHORT = 14, CONTROL_OPTIONS = 20, CONTROL_STATE = 21,
    CONTROL_SUBCODE = 23
};
static const BYTE control_operands[37] = {
    18, CONTROL_CONTEXT, CONTROL_SOURCE, CONTROL_SOURCE, CONTROL_NONE,
    19, 19, CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 7, 11, 19, 19, 19,
    CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 8, 9, 10,
    CONTROL_TEXT, CONTROL_BYTE, CONTROL_SHORT, CONTROL_NONE, CONTROL_NONE,
    CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 98, CONTROL_OPTIONS,
    CONTROL_STATE, 113, CONTROL_NONE, CONTROL_SUBCODE, CONTROL_LINE_DELTA
};

/* Source-only contextual inputs: native guards precede their reads. Their
 * initial values, producers and wider storage are not modeled here. */
unsigned control_option_kind; /* RVA5e22c */
const BYTE *control_option_value; /* RVA5e230 */
/* RVA54fd8 is the shared counted assembler record in c1_bss. */
static BYTE control_byte; /* RVA550f0 */
static short control_short; /* RVA54b34 */
WORD control_state_first, control_state_second; /* RVA5f364/5f368 */
extern "C" {
void __cdecl write_expression_marker(unsigned);
int __cdecl has_control_location(void);
void *__cdecl copy_control_option(const void *, void *, unsigned);
void __cdecl write_global_record(unsigned, CompilerIrSymbol *);
void __cdecl initialize_semantic_context(void);
unsigned __cdecl pack_option_flags(void);
void __cdecl write_warning_controls(unsigned);
}

#define CONTROL_RESERVE(n) \
    (buffer->good &= buffer->Ensure(n))
#define CONTROL_ENCODED_SHORT(v, n) do { \
    unsigned encoded_value = (unsigned)(v); \
    if (CONTROL_RESERVE(n)) buffer->WriteShort((short)encoded_value); \
} while (0)

extern "C" void __cdecl emit_control_record(int code)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    CompilerSourceFile *source;
    unsigned value, operand;
    if (c1_data.debug_module_kind == 4) return;
    if (!c1_data.c9_il && code != 11 && !(c1_data.function_record_flags & 0x1000)) return;
    if (c1_bss.function_output_mode) return;
#ifdef RECON_NATIVE_TEST
    if (!c1_data.c9_il || code < 0 || code > 36 || code == 11)
        recon_unsupported("non-IL or option-list control record is not yet modeled");
#endif
    if (code == 1 || code == 36) {
        if (code == 36) {
            if (c1_data.parser_flags & 0x80) return;
            if (c1_data.c9_il) code = 1;
            else {
                if (c1_data.function_label_state_first == -1) {
                    if (c1_bss.current_scope_symbol && (c1_bss.current_scope_symbol->flags & 0x40)) return;
                    c1_data.function_label_state_first = c1_bss.nonil_type_index;
                    c1_bss.pending_line_delta = c1_bss.current_context.value - c1_bss.nonil_type_index;
                    goto write_record;
                }
                c1_bss.pending_line_delta = c1_bss.current_context.value - c1_data.function_label_state_first;
                if (c1_bss.pending_line_delta <= 0xffff) return;
                code = 1;
            }
        }
        if (!has_control_location()) return;
    } else if (code == 2) {
        if (c1_data.parser_flags & 0x80) return;
        if (!c1_data.c9_il) {
            source = (CompilerSourceFile *)c1_bss.current_context.object;
            if ((source->reference_flags >> 3) == (unsigned)c1_data.function_label_state_second) return;
            if (c1_bss.declaration_scope_active >= 2) _unlink(c1_data.options.object_file);
        }
    } else if (code == 11) {
        CompilerArena *arena = &c1_bss.arenas[0];
        CompilerGlobalOption *item = (CompilerGlobalOption *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        item->next = c1_data.global_options;
        item->kind = control_option_kind;
        unsigned count = control_option_kind == 7 ? control_option_value[0] + 1 :
            strlen((const char *)control_option_value) + 1;
        item->value = (const BYTE *)copy_control_option(control_option_value, NULL, count);
        c1_data.global_options = item;
        return;
    }
write_record:
    if (code != 35) {
        write_expression_marker(79);
        if (CONTROL_RESERVE(2)) *buffer->cursor++ = (BYTE)code;
    }
    operand = control_operands[code];
    if (operand == CONTROL_CONTEXT) {
        value = c1_bss.current_context.value;
        c1_data.function_label_state_first = value;
        if (c1_data.c9_il) CONTROL_ENCODED_SHORT(value, 4);
        else {
            if (CONTROL_RESERVE(6) && !buffer->WriteSmall(value)) {
                *buffer->cursor++ = 0x80;
                *(unsigned *)buffer->cursor = value;
                buffer->cursor += 4;
            }
            c1_data.last_line_delta = 0;
        }
    } else if (operand == CONTROL_LINE_DELTA) {
        value = c1_bss.pending_line_delta;
        c1_data.last_line_delta = value;
        CONTROL_ENCODED_SHORT(value, 4);
    } else if (operand == CONTROL_SOURCE) {
        source = (CompilerSourceFile *)c1_bss.current_context.object;
        if (c1_data.c9_il) {
            if (!(source->record_id & 0xffffff)) write_global_record(18, NULL);
            value = source->record_id & 0xffffff;
            buffer->WriteIndex(value);
        } else {
            initialize_semantic_context();
            value = source->reference_flags >> 3;
            c1_data.function_label_state_second = value;
            if (CONTROL_RESERVE(4)) {
                if (value >= 0x3fff80) *buffer->cursor++ = (BYTE)(value + 0x80);
                else if (value <= 0x3fff) {
                    *buffer->cursor++ = (BYTE)((value & 0x3f) | 0x80);
                    *buffer->cursor++ = (BYTE)(value >> 6);
                } else {
                    *buffer->cursor++ = (BYTE)(value | 0xc0);
                    *buffer->cursor++ = (BYTE)(value >> 6);
                    *buffer->cursor++ = (BYTE)(value >> 14);
                }
            }
        }
    } else if (operand == CONTROL_TEXT) {
        const BYTE *control_text = (const BYTE *)&c1_bss.assembly_record;
        unsigned count = c1_bss.assembly_record.length;
        if (count < 3 || count > sizeof(c1_bss.assembly_record)) {
#ifdef __linux__
            recon_unsupported("control-record counted text exceeds known assembler prefix");
#else
            __asm { int 3 }
#endif
        }
        if (CONTROL_RESERVE(count + 4)) {
            if (!buffer->WriteSmall((unsigned)(int)(short)count)) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)count;
                buffer->cursor += 2;
            }
            buffer->WriteBytes(control_text, count);
        }
    } else if (operand == CONTROL_BYTE) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("control-record byte input is not yet modeled");
#endif
        value = control_byte;
        if (CONTROL_RESERVE(2)) *buffer->cursor++ = (BYTE)value;
    } else if (operand == CONTROL_SHORT) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("control-record short input is not yet modeled");
#endif
        short short_value = control_short;
        if (CONTROL_RESERVE(4) && !buffer->WriteSmall((unsigned)(int)short_value)) {
            *buffer->cursor++ = 0x80;
            *(WORD *)buffer->cursor = (WORD)short_value;
            buffer->cursor += 2;
        }
    } else if (operand == CONTROL_OPTIONS) {
        c1_data.options.assume_no_alias |= c1_data.options.assume_no_global_alias;
        value = pack_option_flags();
        if (CONTROL_RESERVE(6) && !buffer->WriteSmall(value)) {
            *buffer->cursor++ = 0x80;
            *(unsigned *)buffer->cursor = value;
            buffer->cursor += 4;
        }
    } else if (operand == CONTROL_STATE) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("control-record state bytes are not yet modeled");
#endif
        value = control_state_first | ((unsigned)control_state_second << 8);
        CONTROL_ENCODED_SHORT(value, 4);
    } else if (operand == CONTROL_SUBCODE) write_warning_controls(code);
}

extern "C" void __cdecl emit_pending_line_record(void);
extern "C" void __cdecl write_expression_marker(unsigned code)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    emit_pending_line_record();
    int ready = 1;
    if (buffer->cursor + 2 > buffer->data + buffer->capacity)
        ready = buffer->Grow(2);
    buffer->good &= ready;
    if (buffer->good) *buffer->cursor++ = (BYTE)code;
}
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" int __cdecl has_control_location(void)
{
    if (c1_bss.current_scope_symbol && (c1_bss.current_scope_symbol->flags & 0x40)) return 0;
    if (c1_data.parser_flags & 0x80) return 0;
    if (!c1_data.c9_il) return 1;
    if ((int)c1_bss.current_context.value > 0xffff) {
        if (c1_data.line_range_warning_enabled) {
            report_language_diagnostic(1, 49);
            c1_data.line_range_warning_enabled = 0;
        }
        return 0;
    }
    c1_data.line_range_warning_enabled = 1;
    return 1;
}
extern "C" void *__cdecl copy_control_option(const void *, void *, unsigned)
{ __asm { int 3 } return NULL; }
/* The warning table and its unsigned scan bounds are existing option
 * owners. Offsets in the stream are signed WORDs relative to warning700. */
#define WARNING_SPACE(n) \
    (ready = (buffer->cursor + (n) > buffer->data + buffer->capacity ? buffer->Grow(n) : 1), \
     buffer->good &= ready)
extern "C" void __cdecl write_warning_controls(unsigned code)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    unsigned i;
    int level, ready, written = 0;
#ifdef RECON_NATIVE_TEST
    if (c1_data.options.lowest_warning <= c1_data.options.highest_warning &&
        c1_data.options.highest_warning >= 1000)
        recon_unsupported("warning-control scan exceeds the modeled option table");
#endif
    for (i = c1_data.options.lowest_warning; i <= c1_data.options.highest_warning; ++i) {
        level = (signed char)c1_data.options.warning_levels[i];
        if (!level) continue;
        if (!written) {
            write_expression_marker(79);
            if (WARNING_SPACE(2)) *buffer->cursor++ = (BYTE)code;
            written = 1;
        }
        short delta = (short)(i - 700);
        if (WARNING_SPACE(4)) {
            if ((unsigned)(int)delta & ~0x7fU) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)delta;
                buffer->cursor += 2;
            } else *buffer->cursor++ = (BYTE)delta;
        }
        if (WARNING_SPACE(2)) *buffer->cursor++ = (BYTE)level;
    }
    if (written && WARNING_SPACE(4)) {
        *buffer->cursor++ = 0x80;
        *(WORD *)buffer->cursor = 0xffff;
        buffer->cursor += 2;
    }
}
#undef WARNING_SPACE
void RECON_THISCALL CompilerOutputBuffer::WriteShort(short value)
{
    if ((unsigned)(int)value & ~0x7fU) {
        *cursor++ = 0x80;
        *(WORD *)cursor = (WORD)value;
        cursor += 2;
    } else *cursor++ = (BYTE)value;
}
#undef CONTROL_ENCODED_SHORT
#undef CONTROL_RESERVE

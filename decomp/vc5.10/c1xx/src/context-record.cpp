#include <io.h>
#include <string.h>
#include "frontend-init.h"
#include "global-output.h"
#pragma intrinsic(strlen)
#include <recon.h>

/* The first format column of the encountered CF158 records. Every known
 * nonNULL format has one operand. The other column is not copied or read;
 * numbered tags reaching the original default remain uninterpreted. This
 * is the supported opcode domain, not a claim about the table's full extent. */
enum ControlOperand {
    CONTROL_NONE, CONTROL_POSITION, CONTROL_LINE_DELTA, CONTROL_SOURCE,
    CONTROL_TEXT = 12, CONTROL_BYTE, CONTROL_SHORT,
    CONTROL_OPTIONS = 20, CONTROL_INLINE_STATE, CONTROL_SUBCODE = 23
};
static const BYTE control_operand_kind[37] = {
    18, CONTROL_POSITION, CONTROL_SOURCE, CONTROL_SOURCE, CONTROL_NONE,
    19, 19, CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 7, 11, 19, 19, 19,
    CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 8, 9, 10,
    CONTROL_TEXT, CONTROL_BYTE, CONTROL_SHORT, CONTROL_NONE, CONTROL_NONE,
    CONTROL_NONE, CONTROL_NONE, CONTROL_NONE, 98, CONTROL_OPTIONS,
    CONTROL_INLINE_STATE, 113, CONTROL_NONE, CONTROL_SUBCODE, CONTROL_LINE_DELTA
};
extern "C" {
void __cdecl write_expression_marker(unsigned); /*6dbb*/
int __cdecl has_control_location(void); /*6e1f*/
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
void *__cdecl copy_control_option(const void *, void *, unsigned); /*46670*/
void __cdecl write_warning_controls(unsigned); /*4547*/
void __cdecl initialize_semantic_context(void);
void __cdecl write_global_ir_record(unsigned, CompilerSymbol *);
unsigned __cdecl pack_option_flags(void);
}

#define CONTROL_SPACE(n) (buffer->ok &= buffer->Ensure(n))
#define CONTROL_COMPACT_DWORD(v,n) do { unsigned encoded = (unsigned)(v); \
    if (CONTROL_SPACE(n) && !buffer->WriteCompact(encoded)) { \
        *buffer->cursor++ = 0x80; *(DWORD *)buffer->cursor = encoded; buffer->cursor += 4; \
    } } while (0)

extern "C" void __cdecl emit_context_transition(unsigned code)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3]; /* C0360 */
    CompilerSourceFile *source;
    CompilerGlobalOption *item;
    CompilerArena *arena;
    unsigned value, count;
    if (frontend_data.debug_module_kind == 4) return;
    if (!frontend_data.option_c9il && code != 11 && !(frontend_data.source_mode_flags & 0x1000)) return;
    if (frontend_bss.suppress_parser_codegen) return;
    if (code > 36) { __asm { int 3 } }
    switch (code) {
    case 36:
        if (frontend_data.compiler_mode_flags & 0x80) return;
        if (frontend_data.option_c9il) code = 1;
        else {
            if (frontend_data.function_location_cache[0] == 0xffffffff) {
                if (frontend_bss.current_function && (frontend_bss.current_function->scope_flags & 1)) return;
                value = frontend_bss.nonil_type_index;
                frontend_data.function_location_cache[0] = value;
                frontend_bss.ir_line_delta = frontend_bss.saved_context_stack.position - value;
                break;
            }
            value = frontend_bss.saved_context_stack.position - frontend_data.function_location_cache[0];
            frontend_bss.ir_line_delta = value;
            if ((int)value <= 0xffff) return;
            code = 1;
        }
    case 1:
        if (!has_control_location()) return;
        break;
    case 2:
        if (frontend_data.compiler_mode_flags & 0x80) return;
        if (!frontend_data.option_c9il) {
            source = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
            if ((source->reference_flags >> 3) == frontend_data.function_location_cache[1]) return;
            if (frontend_bss.global_output_active >= 2) _unlink(frontend_data.options.object_file);
        }
        break;
    case 11:
        arena = &frontend_bss.arenas[0];
        item = (CompilerGlobalOption *)arena->cursor;
        arena->cursor += 12;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        item->next = frontend_data.global_options;
        item->kind = frontend_data.control_option_kind;
        count = item->kind == 7 ? (BYTE)frontend_data.control_option_text[0] + 1 :
            strlen(frontend_data.control_option_text) + 1;
        item->value = (const BYTE *)copy_control_option(frontend_data.control_option_text, NULL, count);
        frontend_data.global_options = item;
        return;
    }
    if (code != 35) {
        write_expression_marker(0x4f);
        if (CONTROL_SPACE(2)) *buffer->cursor++ = (BYTE)code;
    }
    switch (control_operand_kind[code]) {
    case CONTROL_POSITION:
        value = frontend_bss.saved_context_stack.position;
        frontend_data.function_location_cache[0] = value;
        if (frontend_data.option_c9il) {
            if (CONTROL_SPACE(4)) buffer->WriteEncodedShort(value);
        } else {
            CONTROL_COMPACT_DWORD(value, 6);
            frontend_data.last_ir_line_delta = 0;
        }
        break;
    case CONTROL_LINE_DELTA:
        value = frontend_bss.ir_line_delta;
        frontend_data.last_ir_line_delta = value;
        if (CONTROL_SPACE(4)) buffer->WriteEncodedShort(value);
        break;
    case CONTROL_SOURCE:
        source = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
        if (frontend_data.option_c9il) {
            if (!(source->record_index & 0xffffff)) write_global_ir_record(18, NULL);
            source = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
            value = source->record_index & 0xffffff;
        } else {
            initialize_semantic_context();
            source = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
            value = source->reference_flags >> 3;
            frontend_data.function_location_cache[1] = value;
        }
        if (frontend_data.option_c9il) buffer->WriteIndex(value);
        else if (CONTROL_SPACE(4)) {
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
        break;
    case CONTROL_TEXT:
        count = frontend_bss.control_text_count;
        /* Only this actual one-byte BSS owner is known. Nonzero payloads
         * stop in both builds before any address/span beyond it is used. */
        if (count) { __asm { int 3 } }
        if (CONTROL_SPACE(count + 4)) {
            if (!buffer->WriteCompact((unsigned)(int)(short)count)) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)count;
                buffer->cursor += 2;
            }
            buffer->WriteBytes(&frontend_bss.control_text_count, count);
        }
        break;
    case CONTROL_BYTE:
        value = frontend_bss.control_byte;
        if (CONTROL_SPACE(2)) *buffer->cursor++ = (BYTE)value;
        break;
    case CONTROL_SHORT:
        value = (unsigned)(int)frontend_bss.control_short;
        if (CONTROL_SPACE(4) && !buffer->WriteCompact(value)) {
            *buffer->cursor++ = 0x80;
            *(WORD *)buffer->cursor = (WORD)value;
            buffer->cursor += 2;
        }
        break;
    case CONTROL_OPTIONS:
        frontend_data.options.assume_no_alias |= frontend_data.options.assume_no_global_alias;
        value = pack_option_flags();
        CONTROL_COMPACT_DWORD(value, 6);
        break;
    case CONTROL_INLINE_STATE:
        value = (BYTE)frontend_data.inline_depth | ((unsigned)(BYTE)frontend_data.inline_recursion << 8);
        if (CONTROL_SPACE(4)) buffer->WriteEncodedShort(value);
        break;
    case CONTROL_SUBCODE:
        write_warning_controls(code);
        break;
    }
}

extern "C" void __cdecl write_current_source_line(void);
extern "C" void __cdecl write_expression_marker(unsigned code)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    int good;
    write_current_source_line();
    good = buffer->cursor + 2 <= buffer->data + buffer->capacity || buffer->Grow(2) != 0;
    buffer->ok &= good;
    if (buffer->ok) *buffer->cursor++ = (BYTE)code;
}
extern "C" int __cdecl has_control_location(void)
{
    if (frontend_bss.current_function && (frontend_bss.current_function->scope_flags & 1)) return 0;
    if (frontend_data.compiler_mode_flags & 0x80) return 0;
    if (!frontend_data.option_c9il) return 1;
    if ((int)frontend_bss.saved_context_stack.position > 0xffff) {
        if (frontend_data.line_range_warning_enabled) {
            report_language_diagnostic(1, 0x31);
            frontend_data.line_range_warning_enabled = 0;
        }
        return 0;
    }
    frontend_data.line_range_warning_enabled = 1;
    return 1;
}
extern "C" void *__cdecl copy_control_option(const void *, void *, unsigned) { __asm { int 3 } return NULL; }
#define WARNING_SPACE(n) (ready = (buffer->cursor + (n) <= buffer->data + buffer->capacity || \
    buffer->Grow(n) != 0), buffer->ok &= ready)
extern "C" void __cdecl write_warning_controls(unsigned code)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    unsigned warning;
    int level, ready, written = 0;
    short delta;
    if (frontend_data.options.lowest_warning <= frontend_data.options.highest_warning &&
        frontend_data.options.highest_warning >= 1000) { __asm { int 3 } }
    for (warning = frontend_data.options.lowest_warning;
         warning <= frontend_data.options.highest_warning; ++warning) {
        level = (signed char)frontend_data.options.warning_levels[warning];
        if (!level) continue;
        if (!written) {
            write_expression_marker(0x4f);
            if (WARNING_SPACE(2)) *buffer->cursor++ = (BYTE)code;
            written = 1;
        }
        delta = (short)(warning - 700);
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
void RECON_THISCALL OutputBuffer::WriteEncodedShort(unsigned value)
{
    short word = (short)value;
    if ((unsigned)(int)word & ~0x7fU) {
        *cursor++ = 0x80;
        *(WORD *)cursor = (WORD)word;
        cursor += 2;
    } else *cursor++ = (BYTE)word;
}

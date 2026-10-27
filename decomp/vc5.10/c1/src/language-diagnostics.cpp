#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "frontend.h"
#include "compiler-message-ids.h"
#include <recon.h>

extern "C" {
const char *__stdcall get_diagnostic_message(unsigned);
void __cdecl emit_language_diagnostic(unsigned, unsigned, const char *, va_list);
void __cdecl compiler_warning(int, ...);
void __cdecl set_warning_state(unsigned, unsigned);
}

extern "C" void __cdecl report_language_diagnostic(unsigned level, unsigned number, ...)
{
    int category;
    const char *message;
    unsigned threshold = level;
    va_list arguments;
    if (c1_data.parser_flags & 0x20000) return;
    if (number >= 1000) {
#ifdef __linux__
        recon_unsupported("language diagnostic index exceeds the recovered controls");
#else
        __asm { int 3 }
#endif
    }
    category = (signed char)c1_data.options.warning_levels[number];
    if ((unsigned)category > 14) return;
    switch (category) {
    case 1: case 2: case 3: case 4: case 5:
        threshold = category;
        /* fall through */
    case 0: case 6:
        if (c1_data.options.warning_level < (int)threshold) return;
        break;
    case 14:
        break;
    default:
        return;
    }
    message = get_diagnostic_message(number + 4000);
    /* The original LEA after the message query points past the two named
     * arguments. Ordinary va_start preserves that genuine argument role. */
    va_start(arguments, number);
    if (category == 14) {
        emit_language_diagnostic(2000, number + 2000, message, arguments);
        if (c1_bss.debug_module && !c1_bss.debug_module->Slot44())
            compiler_error(73, ".\\error.c", 626);
        if (!c1_bss.parser_early_result) c1_bss.parser_early_result = 1;
    } else {
        emit_language_diagnostic(4000, number, message, arguments);
        if (c1_bss.debug_module && !c1_bss.debug_module->Slot40())
            compiler_error(73, ".\\error.c", 583);
        if (c1_bss.options.warnings_are_errors && !c1_bss.parser_early_result && c1_bss.options.warnings_are_errors == 1) {
            c1_bss.options.warnings_are_errors = 2;
            compiler_warning(220);
            --c1_bss.parser_early_result;
        }
        if (category == 5) set_warning_state(number, 13);
    }
    va_end(arguments);
}
extern "C" const char *__stdcall get_diagnostic_message(unsigned number)
{
    if (number != C1_TEXT_FASTCALL_STACK_CHECK && number != C1_TEXT_ILLEGAL_INITIALIZATION &&
        number != C1_TEXT_SYNTAX_ERROR_TOKEN && number != C1_TEXT_MEMBER_NOT_FOUND &&
        number != C1_TEXT_TOO_FEW_ARGUMENTS && number != C1_TEXT_INCOMPATIBLE_TYPES) {
#ifdef __linux__
        recon_unsupported("language diagnostic resource is not reconstructed");
#else
        __asm { int 3 }
#endif
    }
    LoadStringA(c1_dll_instance, number, c1_bss.diagnostic_message,
        sizeof(c1_bss.diagnostic_message));
    return c1_bss.diagnostic_message;
}
extern "C" {
int __cdecl format_buffer(char *, const char *, unsigned, ...);
int __cdecl format_buffer_v(char *, const char *, unsigned, va_list);
const char *__cdecl get_compiler_message(unsigned);
void __cdecl write_language_diagnostic(const char *);
}
#pragma intrinsic(strlen)
extern "C" void __cdecl emit_language_diagnostic(unsigned category, unsigned number,
    const char *message, va_list arguments)
{
    char buffer[2048], *cursor = buffer;
    const char *label;
    if ((int)c1_bss.current_context.value > 0 && c1_bss.current_context.object) {
        format_buffer(cursor, "%s(%d) : ", 2048,
            c1_bss.current_context.FileName(), c1_bss.current_context.value);
        cursor += strlen(cursor);
    }
    if (category) {
        if (category == 1000) label = get_compiler_message(315);
        else if (category == 2000) label = get_compiler_message(314);
        else if (category == 4000) label = get_compiler_message(313);
        else if (category == 6000) label = get_compiler_message(312);
        else {
            /* The original default consumes an unwritten private stack
             * word as its label. No source-level value is inferred for it. */
#ifdef __linux__
            recon_unsupported("diagnostic category label is unrecovered");
#else
            __asm { int 3 }
#endif
        }
        cursor += format_buffer(cursor, "%s %c%d: ", (unsigned)(buffer + 2048 - cursor),
            label, 'C', category + number);
    }
    c1_data.diagnostic_line = c1_bss.current_context.value;
    c1_data.diagnostic_source = c1_bss.current_context.object;
    c1_data.diagnostic_prefix_characters = _mbstrlen(buffer);
    format_buffer_v(cursor, message, (unsigned)(buffer + 2048 - cursor), arguments);
    write_language_diagnostic(buffer);
}
extern "C" void __cdecl write_language_diagnostic(const char *text)
{
    fwrite(text, strlen(text), 1, stderr);
    fwrite("\n", 1, 1, stderr);
    fflush(stderr);
}

extern "C" void __cdecl initialize_compiler_messages(void);
extern "C" const char *__cdecl lookup_compiler_message(unsigned);
extern "C" const char *__cdecl get_compiler_message(unsigned number)
{
    if (!c1_data.compiler_messages_initialized) {
        initialize_compiler_messages();
        c1_data.compiler_messages_initialized = 1;
    }
    return lookup_compiler_message(number);
}
extern "C" const char *__cdecl lookup_compiler_message(unsigned number)
{
    unsigned index;
    for (index = 0; ; ++index) {
        if (index >= C1_COMPILER_MESSAGE_COUNT) {
#ifdef __linux__
            recon_unsupported("compiler text catalog has lost its encountered sentinel");
#else
            __asm { int 3 }
#endif
        }
        const CompilerMessageText *entry = &c1_data.compiler_messages[index];
        if (!entry->id || entry->id == number) return entry->text;
    }
}
extern "C" void __cdecl initialize_compiler_messages(void)
{
    char buffer[256];
    unsigned index;
    for (index = 0; ; ++index) {
        if (index >= C1_COMPILER_MESSAGE_COUNT) {
#ifdef __linux__
            recon_unsupported("compiler text catalog has lost its encountered sentinel");
#else
            __asm { int 3 }
#endif
        }
        CompilerMessageText *entry = &c1_data.compiler_messages[index];
        if (!entry->id) break;
        LoadStringA(c1_dll_instance, entry->id, buffer, sizeof(buffer));
        entry->text = _strdup(buffer);
    }
}

/* Separate warning entry used when a caller already owns the argument list. */
extern "C" void __cdecl compiler_warning_v(unsigned number, va_list arguments)
{
    const char *message = get_diagnostic_message(number + 2000);
    emit_language_diagnostic(2000, number, message, arguments);
    if (++c1_bss.parser_early_result > 100) compiler_error(3, 100);
    if (c1_bss.debug_module && !c1_bss.debug_module->Slot44())
        compiler_error(73, ".\\error.c", 198);
}

/* Source-only DWORD at5f34c. Suppression-mode count ownership/default has not
 * been qualified; stop before its read/write in either build. */
static int unmodeled_suppressed_diagnostic_count;
extern "C" void __cdecl compiler_warning(int code, ...)
{
    va_list arguments;
    if (c1_data.parser_flags & 0x20000) {
#ifdef __linux__
        recon_unsupported("suppressed diagnostic counter is not yet modeled");
#else
        __asm { int 3 }
#endif
        ++unmodeled_suppressed_diagnostic_count;
        return;
    }
    va_start(arguments, code);
    compiler_warning_v(code, arguments);
    va_end(arguments);
}

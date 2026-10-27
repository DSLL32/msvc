#include <string.h>
#include "frontend.h"
#include <recon.h>

extern "C" {
const char *__cdecl get_diagnostic_message(unsigned);
void __cdecl emit_language_diagnostic(unsigned, unsigned, const char *, const unsigned *);
void __cdecl compiler_warning(int, ...);
void __cdecl set_warning_state(unsigned, unsigned);
}

extern "C" void __cdecl report_language_diagnostic(unsigned level, unsigned number, ...)
{
    int category;
    const char *message;
    unsigned threshold = level;
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
    /* VC5 passes the address of the named number stack slot to the variadic
     * formatter. Native parameter spills are not that contiguous original
     * argument representation; keep output held until its real caller needs it. */
#ifdef __linux__
    recon_unsupported("language diagnostic native argument-list layout is not recovered");
#endif
    /* The original formatter receives the contiguous list beginning at the
     * named number slot, before this routine's optional arguments. Native
     * output is held above rather than assuming this stack layout. */
    message = get_diagnostic_message(number + 4000);
    if (category == 14) {
        emit_language_diagnostic(2000, level + 2000, message, &number);
        if (c1_bss.debug_module && !c1_bss.debug_module->Slot44())
            compiler_error(73, ".\\error.c", 626);
        if (!c1_bss.parser_early_result) c1_bss.parser_early_result = 1;
    } else {
        emit_language_diagnostic(4000, level, message, &number);
        if (c1_bss.debug_module && !c1_bss.debug_module->Slot40())
            compiler_error(73, ".\\error.c", 583);
        if (c1_bss.options.warnings_are_errors && !c1_bss.parser_early_result && c1_bss.options.warnings_are_errors == 1) {
            c1_bss.options.warnings_are_errors = 2;
            compiler_warning(220);
            --c1_bss.parser_early_result;
        }
        if (category == 5) set_warning_state(number, 13);
    }
}
extern "C" const char *__cdecl get_diagnostic_message(unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl emit_language_diagnostic(unsigned, unsigned, const char *, const unsigned *) { __asm { int 3 } }

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

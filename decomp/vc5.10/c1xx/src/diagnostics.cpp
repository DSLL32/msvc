/* Compiler-specific diagnostic formatting, not the public CRT formatter.
 * Native currently reaches its true diagnostic-session child only; wider
 * object formats and fault recovery are source/graph coverage. */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <mbctype.h>
#include "diagnostics.h"
#include "frontend-init.h"
#include "pointer-array.h"
#include <recon.h>
#pragma intrinsic(strlen, strcpy, memset)

#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
/* The original imports the data array directly, without a CRT accessor. */
#undef _mbctype
extern "C" __declspec(dllimport) unsigned char _mbctype[];
#endif

/* Borrowed source-only views. No size, allocation or unknown-byte value is
 * inferred for either record; their current format branches are guarded. */
struct DiagnosticClassView {
    BYTE kind;
    BYTE unmodeled_01[19];
    short type_index; /* original+14 */
};
struct DiagnosticNamespaceView : CompilerNamedSymbol {
    void **encoded_name; /* original+10, then reads its first pointer */
};
extern "C" {
const char *__cdecl format_type_code(WORD);
void __cdecl append_diagnostic_class(CompilerDiagnosticContext, CompilerSymbol *);
CompilerDiagnosticContext __cdecl describe_diagnostic_declaration(CompilerSymbol *, CompilerType *, unsigned);
CompilerDiagnosticContext __cdecl describe_diagnostic_identifier(unsigned, CompilerNameEntry *);
const char *__cdecl format_encoded_symbol_name(void *, int);
const char *__cdecl format_diagnostic_symbol(CompilerSymbol *);
const char *__cdecl get_compiler_message(unsigned);
char *__cdecl format_integer64(const __int64 *, char *, int, int);
}

extern "C" int __cdecl format_source_text(char *output, const char *format,
    int capacity, va_list arguments)
{
    char *start = output;
    int remaining = capacity;
    char number[64];
    const char *text;
    int length, radix, code;
    __int64 wide;
    CompilerDiagnosticContext context;
    CompilerSymbol *symbol;
    CompilerNameEntry *identifier;
    void **encoded;
    if (!format) return 0;
    if (remaining == -1) remaining = 0x7fffffff;
    begin_declaration_diagnostic();
#ifdef RECON_NATIVE_TEST
    if (capacity != 1024 || format[0] != '%' || format[1] != 's' ||
        format[2] != '\n' || format[3])
        recon_unsupported("format_source_text: only builtin-source string format supported");
#endif
    while (*format && remaining > 0) {
        if (*format == '%') {
            __try {
                text = NULL;
                switch (format[1]) {
                case 'D': case 'U': case 'X':
                    wide = va_arg(arguments, __int64);
                    text = format_integer64(&wide, number,
                        format[1] == 'X' ? 16 : 10, format[1] == 'U');
                    format += 2;
                    break;
                case 'd': case 'u': case 'x':
                    radix = format[1] == 'x' ? 16 : 10;
                    if (format[1] == 'u') text = _ultoa(va_arg(arguments, unsigned long), number, radix);
                    else text = _ltoa(va_arg(arguments, long), number, radix);
                    format += 2;
                    break;
                case 'c':
                    *output++ = (char)va_arg(arguments, int);
                    --remaining;
                    format += 2;
                    break;
                case 's':
                    text = va_arg(arguments, const char *);
                    format += 2;
                    break;
                case '$':
                    format += 2;
                    code = (signed char)*format++;
                extended_format:
                    switch (code) {
                    case '*':
                        code = (signed char)va_arg(arguments, int);
                        goto extended_format;
                    case 'B':
                        text = format_type_code(*va_arg(arguments, WORD *));
                        break;
                    case 'C':
                        symbol = va_arg(arguments, CompilerSymbol *);
                        context = allocate_diagnostic_context();
                        if (symbol->kind == 15)
                            symbol = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[
                                ((DiagnosticClassView *)symbol)->type_index];
                        append_diagnostic_class(context, symbol);
                        text = format_declaration_context(context);
                        break;
                    case 'D':
                        text = format_declaration_context(describe_diagnostic_declaration(
                            va_arg(arguments, CompilerSymbol *), NULL, 0));
                        break;
                    case 'e':
                        text = strerror(va_arg(arguments, int));
                        break;
                    case 'E':
                        FormatMessageA(0x1100, NULL, va_arg(arguments, unsigned), 0x400,
                            (LPSTR)&text, 0, NULL);
                        break;
                    case 'I':
                        identifier = va_arg(arguments, CompilerNameEntry *);
                        text = identifier->name;
                        if (text[0] == '?') {
                            if (text[1] == '%') text = "`anonymous-namespace'";
                            else text = format_declaration_context(
                                describe_diagnostic_identifier(0, identifier));
                        }
                        break;
                    case 'L':
                        text = compiler_token_spellings[(BYTE)va_arg(arguments, int)];
                        break;
                    case 'M':
                        code = va_arg(arguments, unsigned);
                        if (code) text = get_compiler_message(code);
                        break;
                    case 'S':
                        symbol = va_arg(arguments, CompilerSymbol *);
                        if (symbol->kind == 18) {
                            encoded = ((DiagnosticNamespaceView *)symbol)->encoded_name;
                            text = format_encoded_symbol_name(*encoded, 0);
                        } else text = format_diagnostic_symbol(symbol);
                        break;
                    case 'T':
                        {
                            CompilerType *type = va_arg(arguments, CompilerType *);
                            if (type) text = format_declaration_context(
                                describe_diagnostic_declaration(NULL, type, 0));
                            else text = get_compiler_message(0x159);
                        }
                        break;
                    case 's':
                        text = va_arg(arguments, const char *);
                        break;
                    default:
                        text = get_compiler_message(0x134);
                        break;
                    }
                    break;
                default:
                    *output++ = *format++;
                    --remaining;
                    break;
                }
                if (text) {
                    length = strlen(text);
                    if (length > remaining) length = remaining;
                    strncpy(output, text, length);
                    output += length;
                    remaining -= length;
                }
            } __except (compiler_exception_filter(GetExceptionInformation())) {
                text = get_compiler_message(0x134);
                length = strlen(text);
                if (length > remaining) length = remaining;
                strncpy(output, text, length);
                output += length;
                remaining -= length;
            }
        } else {
            BYTE character = (BYTE)*format++;
            *output++ = (char)character;
            --remaining;
            if ((_mbctype[character + 1] & 4) && remaining > 0) {
                *output++ = *format++;
                --remaining;
            }
        }
    }
    end_declaration_diagnostic();
    if (remaining <= 0 && capacity > 0) strcpy(output - 4, "...");
    else *output = 0;
    return output - start;
}

/* The original six-byte entry falls directly into the separately callable
 * allocator. Its ignored allocator result is not a begin-session return. */
extern "C" void __cdecl begin_declaration_diagnostic(void)
{
    ++frontend_data.diagnostic_depth;
    allocate_diagnostic_context();
}

extern "C" const char *__cdecl format_type_code(WORD) { __asm { int 3 } return NULL; }
extern "C" void __cdecl append_diagnostic_class(CompilerDiagnosticContext, CompilerSymbol *) { __asm { int 3 } }
extern "C" void *__cdecl checked_reallocate(void *, unsigned);
extern "C" CompilerDiagnosticContext __cdecl allocate_diagnostic_context(void)
{
    int i, selected = -1;
    CompilerDiagnosticRecord *records = frontend_data.diagnostic_records;
    CompilerDiagnosticRecord *record;
    for (i = 0; i < frontend_data.diagnostic_record_count; ++i) {
        if (!records[i].id) { selected = i; break; }
    }
    if (selected == -1) {
        if (records) records = (CompilerDiagnosticRecord *)checked_reallocate(records,
            (frontend_data.diagnostic_record_count + 64) * 20);
        else records = (CompilerDiagnosticRecord *)checked_malloc(0x500);
        frontend_data.diagnostic_records = records;
        if (!records) {
            report_compiler_error(60);
            records = frontend_data.diagnostic_records;
        }
        memset(records + frontend_data.diagnostic_record_count, 0, 0x500);
        selected = frontend_data.diagnostic_record_count;
        frontend_data.diagnostic_record_count += 64;
    }
    record = records + selected;
    if (!record->text) {
        record->capacity = 0x80;
        record->text = (char *)checked_malloc(0x80);
    }
    record->id = frontend_data.next_diagnostic_context++;
    record->prefix_cursor = 0x40;
    record->suffix_cursor = 0x41;
    memset(record->text, 0, record->capacity);
    return record->id;
}
extern "C" void *__cdecl checked_reallocate(void *, unsigned) { __asm { int 3 } return NULL; }
extern "C" CompilerDiagnosticContext __cdecl describe_diagnostic_declaration(CompilerSymbol *, CompilerType *, unsigned) { __asm { int 3 } return 0; }
extern "C" CompilerDiagnosticContext __cdecl describe_diagnostic_identifier(unsigned, CompilerNameEntry *) { __asm { int 3 } return 0; }
extern "C" const char *__cdecl format_encoded_symbol_name(void *, int) { __asm { int 3 } return NULL; }
extern "C" const char *__cdecl format_diagnostic_symbol(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" const char *__cdecl get_compiler_message(unsigned) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl format_integer64(const __int64 *, char *, int, int) { __asm { int 3 } return NULL; }

extern "C" void __cdecl end_declaration_diagnostic(void)
{
    int i;
    if (!--frontend_data.diagnostic_depth) {
        for (i = 0; i < frontend_data.diagnostic_record_count; ++i)
            frontend_data.diagnostic_records[i].id = 0;
    }
}

extern "C" const char *__stdcall load_diagnostic_message(unsigned) { __asm { int 3 } return NULL; }
extern "C" void __cdecl write_compiler_diagnostic(unsigned, unsigned, const char *, va_list) { __asm { int 3 } }
extern "C" void __cdecl compiler_warning(int, ...);
extern "C" void __cdecl set_warning_state(unsigned, unsigned);
extern "C" void __cdecl report_language_diagnostic(unsigned level, unsigned number, ...)
{
    const char *message;
    int state, severity;
    va_list arguments;
    if (frontend_data.compiler_mode_flags & 0x20000) return;
    if (number >= 1000) { __asm { int 3 } }
    state = (signed char)frontend_data.options.warning_levels[number];
    switch (state) {
    case 0: case 5:
        severity = (int)level;
        break;
    case 1: case 2: case 3: case 4:
        severity = state;
        break;
    case 14:
        break;
    default:
        return;
    }
    if (state != 14 && frontend_data.options.warning_level < severity) return;
    va_start(arguments, number);
    message = load_diagnostic_message(number + 4000);
    if (state == 14) {
        write_compiler_diagnostic(2000, number + 2000, message, arguments);
        if (frontend_bss.debug_module && !frontend_bss.debug_module->Slot44())
            report_compiler_diagnostic(0x49, ".\\error.c", 0x272);
        if (!frontend_bss.error_count) frontend_bss.error_count = 1;
    } else {
        write_compiler_diagnostic(4000, number, message, arguments);
        if (frontend_bss.debug_module && !frontend_bss.debug_module->Slot40())
            report_compiler_diagnostic(0x49, ".\\error.c", 0x247);
        if (frontend_bss.options.warnings_are_errors == 1 && !frontend_bss.error_count) {
            frontend_bss.options.warnings_are_errors = 2;
            compiler_warning(0xdc);
            --frontend_bss.error_count;
        }
        if (state == 5) set_warning_state(number, 13);
    }
    va_end(arguments);
}

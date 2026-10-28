#include <new.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <direct.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#include "browser.h"
#include <recon.h>
#pragma intrinsic(strlen, memcpy, memset)

extern "C" void __cdecl report_response_error(int, ...);
extern "C" void __cdecl advance_browser_generation(void) { __asm { int 3 } }
/* Encountered window of DWORD column62990: kind3/member, kind4/tag and
 * kind5/enumerator cells6299c..629a8 are0. This is not the full column capacity. */
static const unsigned browser_declaration_suppressed[3] = { 0, 0, 0 };
/* Separate encountered cell629ac (kind7/function); intervening kind6 is
 * still unread and unmodeled, with no implied value or column capacity. */
static const unsigned browser_function_suppressed = 0;
static const unsigned browser_value_suppressed[3] = { 0, 0, 1 }; /* kind10/static,11/parameter,12/local, cells629b8..629c0. */
enum {
    BROWSER_MEMBER_CATEGORY = 17, /* BYTE cell62973, kind3. */
    BROWSER_ENUMERATOR_CATEGORY = 10, /* BYTE cell62975, kind5. */
    BROWSER_FUNCTION_CATEGORY = 1, /* BYTE cell62977, kind7. */
    BROWSER_STATIC_CATEGORY = 4, /* BYTE cell6297a, kind10. */
    BROWSER_PARAMETER_CATEGORY = 3 /* BYTE cell6297b, kind11. */
};
extern "C" void __cdecl fill_browser_symbol_name(CompilerBrowserRecord *record, CompilerIrSymbol *symbol)
{
    record->symbol_id = symbol->record_id;
    if (!record->name) {
        record->name = symbol->binding.name->name;
        record->identifier = symbol->binding.name;
    }
}
extern "C" void __cdecl prepare_browser_symbol(CompilerBrowserRecord *record, CompilerIrSymbol *symbol)
{
    unsigned result;
    /* The original uses its dead incoming symbol-pointer slot for this result;
     * the live symbol receiver has already been retained separately. */
    symbol->record_id = *c1_data.backend->SetSymbolId(&result, symbol->record_id);
    fill_browser_symbol_name(record, symbol);
}
extern "C" void __cdecl fill_browser_symbol(CompilerBrowserRecord *record, void *value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)value;
    unsigned storage, member;
    if ((symbol->kind < 3 || symbol->kind > 5) && symbol->kind != 7 && symbol->kind != 10 && symbol->kind != 11 && symbol->kind != 12) {
#ifdef __linux__
        recon_unsupported("browser symbol suppression column kind is not recovered");
#else
        __asm { int 3 }
#endif
    }
    if ((symbol->kind == 7 ? browser_function_suppressed :
         symbol->kind >= 10 ? browser_value_suppressed[symbol->kind - 10] :
         browser_declaration_suppressed[symbol->kind - 3]) || (symbol->flags & 0x40)) {
        record->kind = 20;
        return;
    }
    prepare_browser_symbol(record, symbol);
    storage = symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5
        ? symbol->storage_class & 7 : 0;
    if (storage == 7) {
        record->attributes = 0;
        record->category = 7;
    } else {
        member = symbol->kind == 2 || symbol->kind == 3;
        if (storage >= 1 && storage <= 3) {
            record->attributes = storage == 3 ? 2 : 0x40;
            if (symbol->kind == 7 || symbol->kind == 8) {
                if (!(symbol->flags & 1)) record->attributes |= 0x20;
            } else if (storage == 1 && !(symbol->declaration_flags & 4)) {
                record->attributes |= 0x20;
            }
        } else if (c1_bss.parameter_nesting >= 2) {
            record->attributes |= 1;
        }
        if (member) record->attributes |= 0x40;
        if (symbol->kind == 4) {
            unsigned flags = symbol->value_type->flags;
            if ((flags & 0x408) == 0x408) record->category = 15;
            else if ((flags & 0x402) == 0x402) record->category = 8;
            else if ((flags & 0x404) == 0x404) record->category = 11;
            else if (flags & 0x200) record->category = 9;
        } else if (symbol->kind == 3) {
            record->category = BROWSER_MEMBER_CATEGORY;
        } else if (symbol->kind == 5) {
            record->category = BROWSER_ENUMERATOR_CATEGORY;
        } else if (symbol->kind == 7) {
            record->category = BROWSER_FUNCTION_CATEGORY;
        } else if (symbol->kind == 10) {
            record->category = BROWSER_STATIC_CATEGORY;
        } else if (symbol->kind == 11) {
            record->category = BROWSER_PARAMETER_CATEGORY;
        } else {
            /* Other BYTE category cells remain unrecovered. */
#ifdef __linux__
            recon_unsupported("browser symbol category column kind is not recovered");
#else
            __asm { int 3 }
#endif
        }
    }
    if (record->kind == 25) {
        record->kind = 3;
        record->attributes |= 0x20;
    }
}
extern "C" int __cdecl check_browser_symbol_generation(CompilerBrowserRecord *record, CompilerIrSymbol *symbol)
{
    if ((BYTE)(symbol->record_id >> 24) == c1_data.browser_generation) return 0;
    record->kind = 25;
    fill_browser_symbol(record, symbol);
    return 1;
}
extern "C" void __cdecl fill_browser_reference(CompilerBrowserRecord *record, void *value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)value;
    if ((symbol->kind < 3 || symbol->kind > 5) && symbol->kind != 7 && symbol->kind != 10 && symbol->kind != 11 && symbol->kind != 12) {
#ifdef __linux__
        recon_unsupported("browser reference suppression column kind is not recovered");
#else
        __asm { int 3 }
#endif
    }
    record->kind = 20;
    if ((symbol->kind == 7 ? browser_function_suppressed :
         symbol->kind >= 10 ? browser_value_suppressed[symbol->kind - 10] :
         browser_declaration_suppressed[symbol->kind - 3]) ||
        symbol == c1_bss.browser_last_symbol || (symbol->flags & 0x40))
        return;
    if (!check_browser_symbol_generation(record, symbol)) {
        fill_browser_symbol_name(record, symbol);
        record->kind = 4;
    }
}
extern "C" void __cdecl fill_browser_owner(CompilerBrowserRecord *record, void *value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)value;
    if ((symbol->kind < 3 || symbol->kind > 5) && symbol->kind != 7 && symbol->kind != 10 && symbol->kind != 11 && symbol->kind != 12) {
#ifdef __linux__
        recon_unsupported("browser owner suppression column kind is not recovered");
#else
        __asm { int 3 }
#endif
    }
    if ((symbol->kind == 7 ? browser_function_suppressed :
         symbol->kind >= 10 ? browser_value_suppressed[symbol->kind - 10] :
         browser_declaration_suppressed[symbol->kind - 3]) || symbol == c1_bss.browser_last_symbol ||
        (symbol->flags & 0x40)) {
        record->kind = 20;
        return;
    }
    c1_data.backend->SymbolId(symbol);
    fill_browser_symbol_name(record, symbol);
}
extern "C" void __cdecl fill_browser_token(CompilerBrowserRecord *) { __asm { int 3 } }
extern "C" char *__cdecl duplicate_browser_name(const char *name)
{
    unsigned length = strlen(name) + 1;
    char *copy = (char *)allocate_compiler_memory(length);
    length = strlen(name) + 1;
    memcpy(copy, name, length);
    return copy;
}
extern "C" void __cdecl fill_browser_filename(CompilerBrowserRecord *record)
{
    record->name = duplicate_browser_name(c1_bss.current_context.FileName());
    record->identifier = NULL;
}
extern "C" void __cdecl fill_browser_source(CompilerBrowserRecord *record, void *value)
{
    CompilerMacro *macro = (CompilerMacro *)value;
    unsigned symbol;
    record->attributes = 0;
    record->name = macro->identifier->name;
    record->identifier = macro->identifier;
    record->kind = 3;
    record->category = macro->body->kind != 0 ? 6 : 5;
    macro->body->browser_symbol_id =
        *c1_data.backend->SetSymbolId(&symbol, macro->body->browser_symbol_id);
    record->symbol_id = macro->body->browser_symbol_id;
}
extern "C" void __cdecl fill_browser_generation(CompilerBrowserRecord *record, void *value)
{
    CompilerMacro *macro = (CompilerMacro *)value;
    if ((BYTE)(macro->body->browser_symbol_id >> 24) == c1_data.browser_generation) {
        record->kind = 4;
        record->symbol_id = macro->body->browser_symbol_id;
    } else {
        fill_browser_source(record, macro);
        record->attributes |= 0x20;
    }
}
extern "C" void __cdecl fill_browser_record(CompilerBrowserRecord *record, void *value)
{
    unsigned kind;
    record->line = c1_bss.current_context.value;
    if (!record->line) {
        kind = record->kind & ~0x40U;
        switch (kind) {
        case 0: case 1: case 10: case 14: case 17: case 22:
            break;
        case 3: case 25:
            ((CompilerIrSymbol *)value)->binding.name->flags |= 0x200000;
            record->kind = 20;
            return;
        case 19:
            ((CompilerSourceFile *)value)->name->flags |= 0x200000;
            /* fall through */
        default:
            record->kind = 20;
            return;
        }
    }
    kind = record->kind & ~0x40U;
    switch (kind) {
    case 1: fill_browser_filename(record); break;
    case 3: case 25: fill_browser_symbol(record, value); break;
    case 4: fill_browser_reference(record, value); break;
    case 11: fill_browser_owner(record, value); break;
    case 17: fill_browser_token(record); break;
    case 18: fill_browser_generation(record, value); break;
    case 19: fill_browser_source(record, value); break;
    }
}
extern "C" const char *__cdecl browser_working_directory(void)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    char *directory = (char *)arena->cursor;
    arena->cursor += 256;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (c1_data.options.reproducible) {
        directory[0] = '.';
        directory[1] = 0;
        return directory;
    }
    return _getcwd(directory, 256);
}
extern "C" void __cdecl write_browser_string(const char *text, FILE *stream)
{
    do {
        if (--stream->_cnt >= 0)
            *stream->_ptr++ = *text;
        else
            _flsbuf((signed char)*text, stream);
    } while (*text++);
}
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
/* The target uses the SDK's unlocked putc expansion, including the signed
 * byte passed to _flsbuf. Its FILE representation is supplied by the CRT. */
#define BROWSER_PUT_BYTE(value) \
    (--c1_data.browser_output_stream->_cnt >= 0 \
     ? (void)(*c1_data.browser_output_stream->_ptr++ = (char)(value)) \
     : (void)_flsbuf((signed char)(value), c1_data.browser_output_stream))
#define BROWSER_PUT_WORD(value) do { \
    c1_data.browser_output_stream->_cnt -= 2; \
    if (c1_data.browser_output_stream->_cnt >= 0) { \
        *(WORD *)c1_data.browser_output_stream->_ptr = (WORD)(value); \
        c1_data.browser_output_stream->_ptr += 2; \
    } else { \
        WORD word = (WORD)(value); \
        c1_data.browser_output_stream->_cnt += 2; \
        fwrite(&word, 2, 1, c1_data.browser_output_stream); \
    } \
} while (0)
extern "C" void __cdecl emit_browser_record(CompilerBrowserRecord *record)
{
    unsigned kind, symbol;
    char name[256];
    if (((record->symbol_id & 0xffffff) + 0x8000U) >= 0x10000U)
        record->kind |= 0x40;
    BROWSER_PUT_BYTE(record->kind);
    kind = record->kind & ~0x40U;
    if (kind >= 32 && kind <= 47) return;
    switch (kind) {
    case 0: case 22:
        BROWSER_PUT_BYTE(2);
        BROWSER_PUT_BYTE(0);
        BROWSER_PUT_BYTE(2);
        BROWSER_PUT_BYTE(0);
        write_browser_string(browser_working_directory(),
                             c1_data.browser_output_stream);
        return;
    case 1: case 15:
        write_browser_string(record->name, c1_data.browser_output_stream);
        return;
    case 2: case 21:
        BROWSER_PUT_WORD(record->line);
        return;
    case 4: case 5: case 11:
        symbol = record->symbol_id & 0xffffff;
        BROWSER_PUT_BYTE(symbol);
        BROWSER_PUT_BYTE(symbol >> 8);
        if (record->kind & 0x40) BROWSER_PUT_BYTE(symbol >> 16);
        return;
    case 3:
        BROWSER_PUT_BYTE(record->category);
        BROWSER_PUT_WORD(record->attributes);
        symbol = record->symbol_id & 0xffffff;
        BROWSER_PUT_BYTE(symbol);
        BROWSER_PUT_BYTE(symbol >> 8);
        if (record->kind & 0x40) BROWSER_PUT_BYTE(symbol >> 16);
        if (!record->name) {
            write_browser_string("", c1_data.browser_output_stream);
        } else if (strlen(record->name) < 256) {
            write_browser_string(record->name, c1_data.browser_output_stream);
        } else {
            report_language_diagnostic(3, 786, record->name, 255, 357);
            memcpy(name, record->name, 255);
            name[255] = 0;
            write_browser_string(name, c1_data.browser_output_stream);
        }
        return;
    }
}
#undef BROWSER_PUT_WORD
#undef BROWSER_PUT_BYTE
extern "C" void __cdecl queue_browser_record(CompilerBrowserRecord *record)
{
    unsigned kind;
    if (!record || c1_data.browser_suppressed) return;
    kind = record->kind & ~0x40U;
    if (kind == 18 || kind == 19 || kind == 20) return;
    switch (kind) {
    case 1:
        c1_data.browser_last_line = 0;
        c1_data.browser_absolute_line = 0;
        break;
    case 10:
        c1_data.browser_absolute_line = 1;
        break;
    case 21:
        if (record->line <= 16) record->kind = (BYTE)(record->line + 31);
        break;
    case 0: case 2: case 8: case 9: case 14: case 15: case 16: case 22:
        break;
    default:
        if (c1_data.browser_last_line != record->line) {
            CompilerBrowserRecord location = { 0 };
            c1_bss.browser_last_symbol = NULL;
            if (c1_data.options.function_records && !c1_data.browser_absolute_line) {
                location.kind = 21;
                location.line = record->line - c1_data.browser_last_line;
            } else {
                location.kind = 2;
                location.line = record->line;
                c1_data.browser_absolute_line = 0;
            }
            c1_data.browser_last_line = record->line;
            queue_browser_record(&location);
        }
        break;
    }
    c1_data.browser_output_stream = c1_bss.browser_stream;
    emit_browser_record(record);
}
extern "C" void __cdecl write_browser_record(unsigned kind, void *value)
{
    CompilerBrowserRecord record;
    if (c1_data.browser_suppressed) return;
    record.kind = (BYTE)kind;
    record.category = 0;
    record.line = c1_bss.current_context.value;
    record.symbol_id = 0;
    record.name = NULL;
    record.identifier = NULL;
    record.attributes = 0;
    fill_browser_record(&record, value);
    queue_browser_record(&record);
}

/* The first declared override has its own target body and supplies the ELF
 * key function; the inherited base declaration order owns the vtable slots. */
struct CompilerNullBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned value);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(void *value);
    void RECON_THISCALL Slot20(void *value);
    void RECON_THISCALL Slot24(void *value);
    void RECON_THISCALL Slot28(void *value);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(void *value);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(void *value);
    void RECON_THISCALL Slot40(void);
    int RECON_THISCALL Disable(void);
    void RECON_THISCALL CheckLocation(void);
    unsigned RECON_THISCALL SymbolId(const void *value);
    unsigned *RECON_THISCALL SetSymbolId(unsigned *result, unsigned value);
    void RECON_THISCALL Slot58(void);
    void RECON_THISCALL Slot5c(void);
    RECON_FORCEINLINE CompilerNullBrowser() {}
};

struct CompilerNamedBrowser : CompilerBackend {
    void RECON_THISCALL ResetMode(void);
    void RECON_THISCALL Finish(void);
    void RECON_THISCALL Slot04(void);
    void RECON_THISCALL Slot08(void);
    void RECON_THISCALL Slot0c(void);
    void RECON_THISCALL Slot10(void);
    void RECON_THISCALL Slot14(unsigned value);
    void RECON_THISCALL Slot18(void);
    void RECON_THISCALL Slot1c(void *value);
    void RECON_THISCALL Slot20(void *value);
    void RECON_THISCALL Slot24(void *value);
    void RECON_THISCALL Slot28(void *value);
    void RECON_THISCALL Slot2c(void);
    void RECON_THISCALL Slot30(void *value);
    void RECON_THISCALL Slot34(void);
    void RECON_THISCALL Slot38(void);
    void RECON_THISCALL Slot3c(void *value);
    void RECON_THISCALL Slot40(void);
    int RECON_THISCALL Disable(void);
    void RECON_THISCALL CheckLocation(void);
    unsigned RECON_THISCALL SymbolId(const void *value);
    unsigned *RECON_THISCALL SetSymbolId(unsigned *result, unsigned value);
    void RECON_THISCALL Slot58(void);
    void RECON_THISCALL Slot5c(void);
    void RECON_THISCALL MergeRecords(void);
    char *filename;
    long saved_file_position; /* Original+8; constructor leaves this unwritten. */
    RECON_FORCEINLINE CompilerNamedBrowser(const char *name) : filename(_strdup(name)) {}
    RECON_FORCEINLINE ~CompilerNamedBrowser() { free(filename); }
};

typedef char NullBrowser_size[sizeof(CompilerNullBrowser) == 4 ? 1 : -1];
typedef char NamedBrowser_size[sizeof(CompilerNamedBrowser) == 12 ? 1 : -1];
typedef char NamedBrowser_position[offsetof(CompilerNamedBrowser, saved_file_position) == 8 ? 1 : -1];

extern "C" void __cdecl begin_browser_output(const char *filename)
{
    void *memory;
    if (filename) {
        c1_data.browser_mode = 3;
        memory = ::operator new(12);
        c1_data.named_browser = memory ? new(memory) CompilerNamedBrowser(filename) : NULL;
    } else {
        memory = ::operator new(4);
        c1_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
}

extern "C" CompilerBackend *__cdecl acquire_initial_backend(void)
{
    if (!c1_data.initial_backend) {
        void *memory = ::operator new(4);
        c1_data.initial_backend = memory ? new(memory) CompilerNullBrowser : NULL;
    }
    return c1_data.initial_backend;
}

void RECON_THISCALL CompilerNullBrowser::Slot20(void *value)
{
    if (c1_data.browser_mode == 3)
        ((CompilerIrSymbol *)value)->binding.name->flags |= 0x200000;
}
void RECON_THISCALL CompilerNullBrowser::Slot28(void *value)
{
    if (c1_data.browser_mode == 3)
        ((CompilerMacro *)value)->identifier->flags |= 0x200000;
}
void RECON_THISCALL CompilerNullBrowser::ResetMode(void) { __asm { int 3 }  }
int RECON_THISCALL CompilerNullBrowser::Disable(void)
{
    c1_data.browser_mode = 0;
    return 1;
}
void RECON_THISCALL CompilerNullBrowser::CheckLocation(void) { __asm { int 3 }  }
unsigned RECON_THISCALL CompilerNullBrowser::SymbolId(const void *value) { __asm { int 3 } return 0; }
unsigned *RECON_THISCALL CompilerNullBrowser::SetSymbolId(unsigned *result, unsigned value) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerNullBrowser::Slot5c(void) {}
extern "C" void __cdecl close_browser_stream(FILE *stream)
{
    char error_padding[512];
    fflush(stream);
    if (ferror(stream)) {
        /* Original defect: the write-error path writes512 uninitialized stack
         * bytes. Their values are undefined; emit zeros if this path is used. */
        memset(error_padding, 0, sizeof(error_padding));
        _write(_fileno(stream), error_padding, sizeof(error_padding));
        report_response_error(85, 339, NULL);
    }
}
void RECON_THISCALL CompilerNamedBrowser::MergeRecords(void) { __asm { int 3 } }
void RECON_THISCALL CompilerNamedBrowser::Finish(void)
{
    if (c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1))
        c1_data.backend->Slot38();
    c1_data.backend->Slot04();
    if (c1_data.browser_mode == 2) {
        MergeRecords();
        close_browser_stream(c1_bss.browser_stream);
    } else {
        c1_bss.debug_module->Slotd8(ftell(c1_bss.browser_stream));
        close_browser_stream(c1_bss.browser_stream);
    }
}
void RECON_THISCALL CompilerNamedBrowser::Slot04(void) {}
void RECON_THISCALL CompilerNamedBrowser::Slot08(void)
{
    if (c1_data.browser_scope_depth <= 0) {
        c1_data.browser_scope_depth = 0;
        return;
    }
    --c1_data.browser_scope_depth;
    write_browser_record(9, NULL);
}
void RECON_THISCALL CompilerNamedBrowser::Slot0c(void)
{
    c1_data.browser_scope_depth = (int)((unsigned)c1_data.browser_scope_depth + 1);
    write_browser_record(8, NULL);
}
void RECON_THISCALL CompilerNamedBrowser::Slot10(void)
{
    write_browser_record(10, NULL);
}
void RECON_THISCALL CompilerNamedBrowser::Slot14(unsigned value)
{
    if (c1_data.browser_mode != 2 || value) Slot10();
}
void RECON_THISCALL CompilerNamedBrowser::Slot18(void)
{
    write_browser_record(1, NULL);
}
void RECON_THISCALL CompilerNamedBrowser::Slot1c(void *value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)value;
    if (!(c1_data.function_record_flags & 0x8000) &&
        !(symbol->binding.name->flags & 0x200000))
        write_browser_record(4, symbol);
}
void RECON_THISCALL CompilerNamedBrowser::Slot20(void *value)
{
    write_browser_record(3, value);
}
void RECON_THISCALL CompilerNamedBrowser::Slot24(void *value)
{
    CompilerMacro *macro = (CompilerMacro *)value;
    if (!(c1_data.function_record_flags & 0x8000) &&
        !(macro->identifier->flags & 0x200000))
        write_browser_record(18, macro);
}
void RECON_THISCALL CompilerNamedBrowser::Slot28(void *value)
{
    CompilerMacro *macro = (CompilerMacro *)value;
    if ((BYTE)(macro->body->browser_symbol_id >> 24) != c1_data.browser_generation)
        write_browser_record(19, value);
}
void RECON_THISCALL CompilerNamedBrowser::Slot2c(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot30(void *value)
{
    write_browser_record(11, value);
}
void RECON_THISCALL CompilerNamedBrowser::Slot34(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot38(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot3c(void *value) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot40(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::ResetMode(void)
{
    c1_bss.browser_stream = fopen(filename, "w+b");
    if (!c1_bss.browser_stream) report_response_error(83, 340, filename);
    setvbuf(c1_bss.browser_stream, NULL, _IOFBF, 4096);
    c1_bss.debug_module->Slotd4();
    c1_bss.debug_module->Slotd0();
    write_browser_record(0, NULL);
    Slot18();
    if (c1_data.options.pch_auto || c1_data.options.pch_no_preprocess) {
        Slot04();
        c1_data.browser_header_position = ftell(c1_bss.browser_stream);
    }
    c1_data.browser_generation = 1;
    c1_data.browser_mode = 1;
}
int RECON_THISCALL CompilerNamedBrowser::Disable(void)
{
    struct _stat status;
    long position;
    unsigned signature;
    if (!c1_data.options.function_records) return 0;
    if (_stat(filename, &status) == -1) {
        if (errno == ENOENT) return 0;
        report_response_error(83, 340, filename);
        return 0;
    }
    if (!status.st_size) {
        c1_bss.browser_stream = fopen(filename, "w+b");
        if (!c1_bss.browser_stream) report_response_error(83, 340, filename);
        setvbuf(c1_bss.browser_stream, NULL, _IOFBF, 4096);
        if (!c1_bss.debug_module->Slotbc()) compiler_error(73, ".\\nbrowser.c", 1771);
    } else {
        if (status.st_size > (long)((unsigned)c1_bss.debug_module->Slotdc() << 1)) return 0;
        c1_bss.browser_stream = fopen(filename, "r+b");
        if (!c1_bss.browser_stream) report_response_error(83, 340, filename);
        setvbuf(c1_bss.browser_stream, NULL, _IOFBF, 4096);
        if (!fseek(c1_bss.browser_stream, -8, SEEK_END) &&
            fread(&position, 4, 1, c1_bss.browser_stream) == 1 &&
            fread(&signature, 4, 1, c1_bss.browser_stream) == 1 &&
            signature == 0x4a525301) {
            if (fseek(c1_bss.browser_stream, position, SEEK_SET))
                report_response_error(83, 340, filename);
        } else if (fseek(c1_bss.browser_stream, 0, SEEK_END))
            report_response_error(83, 340, filename);
    }
    c1_data.browser_mode = 2;
    return 1;
}
void RECON_THISCALL CompilerNamedBrowser::CheckLocation(void) { __asm { int 3 }  }
unsigned RECON_THISCALL CompilerNamedBrowser::SymbolId(const void *value)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)value;
    if ((BYTE)(symbol->record_id >> 24) != c1_data.browser_generation)
        write_browser_record(25, symbol);
    return symbol->record_id & 0xffffff;
}
unsigned *RECON_THISCALL CompilerNamedBrowser::SetSymbolId(unsigned *result, unsigned value)
{
    if ((BYTE)(value >> 24) != c1_data.browser_generation) {
        if (!(value & 0xffffff)) {
            if (c1_data.next_symbol_record_id >= 0x7fffffffU) compiler_fatal(55);
            value = (value & 0xff000000U) | (c1_data.next_symbol_record_id++ & 0xffffff);
        }
        value = (value & 0xffffff) | ((unsigned)c1_data.browser_generation << 24);
    }
    *result = value;
    return result;
}
void RECON_THISCALL CompilerNamedBrowser::Slot58(void) { __asm { int 3 }  }
void RECON_THISCALL CompilerNamedBrowser::Slot5c(void)
{
    if (c1_data.browser_mode == 2) {
        Slot04();
        saved_file_position = ftell(c1_bss.browser_stream);
        write_browser_record(22, NULL);
        advance_browser_generation();
        c1_data.browser_last_line = c1_bss.browser_context_value;
    } else {
        saved_file_position = -1;
    }
}

/* The original destroys the named backend inline, then releases the separate
 * four-byte null backend. Neither operation calls a virtual destructor. */
extern "C" void __cdecl finish_symbols(void)
{
    if (c1_data.named_browser) {
        CompilerNamedBrowser *browser = (CompilerNamedBrowser *)c1_data.named_browser;
        browser->~CompilerNamedBrowser();
        ::operator delete(browser);
        c1_data.named_browser = NULL;
    }
    if (c1_data.initial_backend) {
        ::operator delete(c1_data.initial_backend);
        c1_data.initial_backend = NULL;
    }
}

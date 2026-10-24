/* Original output callback constructor [00068dc0,00069100). */
#include "output.h"
#include "options.h"
extern int dos_object, coff_object, codeview_debug, line_debug;
extern int default_library, ansi_mode, floating_frontend;
OutputCallbacks output_callbacks;
char *target_segment_group;
/* Original [00068d30,00068d40): disabled module-completion callback. */
void output_noop(void)
{
}
/* Original [0004e500,0004e510): this build has no per-module symbol hook. */
void finish_output(void)
{
}
/* Original [000692b0,000692c0): debug completion precedes format closure. */
void finish_debugging(void)
{
    output_callbacks.finish_debug();
    output_callbacks.finish_module();
}
/* Original [00068d40,00068d50): disabled filename-output callback. */
void __fastcall output_noop_pair(char *filename)
{
}
/* Original [00068d90,00068da0): disabled line/offset callback. */
void __fastcall output_noop_debug(unsigned line, unsigned offset)
{
}
/* Original [00068d60,00068d70): disabled single-symbol callback. */
void __fastcall output_noop_symbol(void *value)
{
}
void initialize_output_callbacks(void)
{
    char *library;
    target_segment_group = dos_object ? "FLAT" : "DGROUP";
    if (object_filename && codeview_debug) {
    output_callbacks.end_function = end_debug_function;
    output_callbacks.debug_function = (OutputRoutine)begin_debug_function;
    output_callbacks.finish_debug = (OutputRoutine)finish_debug_module;
    } else {
    output_callbacks.end_function = (void (__fastcall *)(FunctionSymbol *))output_noop_symbol;
    output_callbacks.debug_function = (OutputRoutine)output_noop_symbol;
    output_callbacks.finish_debug = (OutputRoutine)output_noop;
    }
    if (!object_filename) {
    output_callbacks.symbol = (void (__fastcall *)(FunctionSymbol *))output_noop_symbol;
    output_callbacks.function = (void (__fastcall *)(FunctionSymbol *))output_noop_symbol;
    output_callbacks.begin_function = (void (__fastcall *)(FunctionSymbol *, char *, int, unsigned))output_noop_function;
    output_callbacks.data = (void (__fastcall *)(void *, unsigned))output_noop_data;
    output_callbacks.select_segment = (void (__fastcall *)(Segment *))output_noop_symbol;
    output_callbacks.segment_reference = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.absolute_reference = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.address = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.relative_reference = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.size = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.debug_pair = (void (__fastcall *)(char *))output_noop_pair;
    output_callbacks.debug_record = (void (__fastcall *)(unsigned, unsigned))output_noop_debug;
    output_callbacks.finish_module = (OutputRoutine)output_noop;
    output_callbacks.relocation = output_noop_relocation;
    output_callbacks.record = (void (__fastcall *)(ModuleDirective *))output_noop_record;
    output_callbacks.mark_symbol = (void (__fastcall *)(SymbolHeader *))mark_unwritten_symbol;
        return;
    }
    if (coff_object) {
    output_callbacks.symbol = (void (__fastcall *)(FunctionSymbol *))coff_output_symbol;
    output_callbacks.function = (void (__fastcall *)(FunctionSymbol *))coff_output_function;
    output_callbacks.begin_function = coff_begin_function;
    output_callbacks.data = (void (__fastcall *)(void *, unsigned))coff_write_data;
    output_callbacks.select_segment = (void (__fastcall *)(Segment *))coff_select_segment;
    output_callbacks.segment_reference = (void (__fastcall *)(unsigned, SymbolHeader *))coff_output_value;
    output_callbacks.absolute_reference = coff_output_bytes;
    output_callbacks.address = coff_output_address;
    output_callbacks.relative_reference = coff_output_offset;
    output_callbacks.size = coff_output_size;
    output_callbacks.finish_module = (OutputRoutine)coff_finish_module;
    output_callbacks.relocation = coff_output_relocation;
    output_callbacks.record = (void (__fastcall *)(ModuleDirective *))coff_output_record;
    output_callbacks.mark_symbol = coff_mark_symbol;
        if (codeview_debug || line_debug) {
    output_callbacks.debug_pair = coff_output_debug_pair;
    output_callbacks.debug_record = coff_output_debug_record;
        } else {
    output_callbacks.debug_pair = (void (__fastcall *)(char *))output_noop_pair;
    output_callbacks.debug_record = (void (__fastcall *)(unsigned, unsigned))output_noop_debug;
        }
    } else {
    output_callbacks.symbol = (void (__fastcall *)(FunctionSymbol *))omf_output_symbol;
    output_callbacks.function = (void (__fastcall *)(FunctionSymbol *))output_noop_symbol;
    output_callbacks.begin_function = (void (__fastcall *)(FunctionSymbol *, char *, int, unsigned))output_noop_function;
    output_callbacks.select_segment = omf_select_segment;
    output_callbacks.data = (void (__fastcall *)(void *, unsigned))omf_output_data;
    output_callbacks.segment_reference = (void (__fastcall *)(unsigned, SymbolHeader *))omf_output_value;
    output_callbacks.absolute_reference = (void (__fastcall *)(unsigned, SymbolHeader *))omf_output_bytes;
    output_callbacks.address = (void (__fastcall *)(unsigned, SymbolHeader *))output_noop_value;
    output_callbacks.relative_reference = (void (__fastcall *)(unsigned, SymbolHeader *))omf_output_offset;
    output_callbacks.size = (void (__fastcall *)(unsigned, SymbolHeader *))omf_output_size;
    output_callbacks.finish_module = (OutputRoutine)omf_finish_module;
    output_callbacks.relocation = omf_output_relocation;
    output_callbacks.record = omf_output_record;
    output_callbacks.mark_symbol = (void (__fastcall *)(SymbolHeader *))omf_mark_symbol;
        if (codeview_debug || line_debug) {
    output_callbacks.debug_pair = (void (__fastcall *)(char *))omf_output_debug_pair;
    output_callbacks.debug_record = (void (__fastcall *)(unsigned, unsigned))omf_output_debug_record;
        } else {
    output_callbacks.debug_pair = (void (__fastcall *)(char *))output_noop_pair;
    output_callbacks.debug_record = (void (__fastcall *)(unsigned, unsigned))output_noop_debug;
        }
    }
    if (default_library) {
        library = default_runtime_library();
        if (library) add_module_directive(4, library);
        if (!ansi_mode && !floating_frontend) add_module_directive(4, "OLDNAMES");
    }
    if (coff_object) coff_open_module();
    else omf_open_module();
    if (codeview_debug) initialize_codeview_output();
}

/* Original [00027bc0,00027c00). */
extern int runtime_single, runtime_dll, runtime_multi;
char *default_runtime_library(void)
{
    if (runtime_dll) return "MSVCRT";
    if (runtime_multi) return "LIBCMT";
    if (runtime_single) return "LIBC";
    return 0;
}

#include "backend.h"
#include <recon.h>
ModuleDirective *module_directives; /* 00084fdc */
/* Original [00069120,00069160). */
void __fastcall add_module_directive(unsigned kind, char *text)
{
    ModuleDirective **tail = &module_directives;
    while (*tail) tail = &(*tail)->next;
    *tail = arena_alloc(1, sizeof(ModuleDirective));
    (*tail)->kind = kind;
    (*tail)->text = text;
}

/* Original [00069420,00069440). */
void write_module_directives(void)
{
    ModuleDirective *directive = module_directives;
    while (directive) {
        output_callbacks.record(directive);
        directive = directive->next;
    }
}

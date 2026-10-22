/* Original output callback constructor [00068dc0,00069100). */
#include "output.h"
#include "options.h"
extern int dos_object, coff_object, codeview_debug, line_debug;
extern int default_library, ansi_mode, floating_frontend;
OutputCallbacks output_callbacks;
char *target_segment_group;
void initialize_output_callbacks(void)
{
    char *library;
    target_segment_group = dos_object ? "FLAT" : "DGROUP";
    if (object_filename && codeview_debug) {
    output_callbacks.end_function = (void (__fastcall *)(FunctionSymbol *))end_debug_function;
    output_callbacks.debug_function = (OutputRoutine)begin_debug_function;
    output_callbacks.finish_debug = (OutputRoutine)finish_debug_module;
    } else {
    output_callbacks.end_function = (void (__fastcall *)(FunctionSymbol *))output_noop_symbol;
    output_callbacks.debug_function = (OutputRoutine)output_noop_symbol;
    output_callbacks.finish_debug = (OutputRoutine)output_noop;
    }
    if (!object_filename) {
    output_callbacks.symbol = (OutputRoutine)output_noop_symbol;
    output_callbacks.function = (OutputRoutine)output_noop_symbol;
    output_callbacks.begin_function = (void (__fastcall *)(FunctionSymbol *, char *, int, unsigned))output_noop_function;
    output_callbacks.data = (OutputRoutine)output_noop_data;
    output_callbacks.select_segment = (OutputRoutine)output_noop_symbol;
    output_callbacks.value = (OutputRoutine)output_noop_value;
    output_callbacks.bytes = (OutputRoutine)output_noop_value;
    output_callbacks.address = (OutputRoutine)output_noop_value;
    output_callbacks.offset = (OutputRoutine)output_noop_value;
    output_callbacks.size = (OutputRoutine)output_noop_value;
    output_callbacks.debug_pair = (OutputRoutine)output_noop_pair;
    output_callbacks.debug_record = (OutputRoutine)output_noop_debug;
    output_callbacks.finish_module = (OutputRoutine)output_noop;
    output_callbacks.relocation = (OutputRoutine)output_noop_relocation;
    output_callbacks.record = (void (__fastcall *)(ModuleDirective *))output_noop_record;
    output_callbacks.mark_symbol = (void (__fastcall *)(SymbolHeader *))mark_unwritten_symbol;
        return;
    }
    if (coff_object) {
    output_callbacks.symbol = (OutputRoutine)coff_output_symbol;
    output_callbacks.function = (OutputRoutine)coff_output_function;
    output_callbacks.begin_function = (void (__fastcall *)(FunctionSymbol *, char *, int, unsigned))coff_begin_function;
    output_callbacks.data = (OutputRoutine)coff_write_data;
    output_callbacks.select_segment = (OutputRoutine)coff_select_segment;
    output_callbacks.value = (OutputRoutine)coff_output_value;
    output_callbacks.bytes = (OutputRoutine)coff_output_bytes;
    output_callbacks.address = (OutputRoutine)coff_output_address;
    output_callbacks.offset = (OutputRoutine)coff_output_offset;
    output_callbacks.size = (OutputRoutine)coff_output_size;
    output_callbacks.finish_module = (OutputRoutine)coff_finish_module;
    output_callbacks.relocation = (OutputRoutine)coff_output_relocation;
    output_callbacks.record = (void (__fastcall *)(ModuleDirective *))coff_output_record;
    output_callbacks.mark_symbol = (void (__fastcall *)(SymbolHeader *))coff_mark_symbol;
        if (codeview_debug || line_debug) {
    output_callbacks.debug_pair = (OutputRoutine)coff_output_debug_pair;
    output_callbacks.debug_record = (OutputRoutine)coff_output_debug_record;
        } else {
    output_callbacks.debug_pair = (OutputRoutine)output_noop_pair;
    output_callbacks.debug_record = (OutputRoutine)output_noop_debug;
        }
    } else {
    output_callbacks.symbol = (OutputRoutine)omf_output_symbol;
    output_callbacks.function = (OutputRoutine)output_noop_symbol;
    output_callbacks.begin_function = (void (__fastcall *)(FunctionSymbol *, char *, int, unsigned))output_noop_function;
    output_callbacks.select_segment = (OutputRoutine)omf_select_segment;
    output_callbacks.data = (OutputRoutine)omf_output_data;
    output_callbacks.value = (OutputRoutine)omf_output_value;
    output_callbacks.bytes = (OutputRoutine)omf_output_bytes;
    output_callbacks.address = (OutputRoutine)output_noop_value;
    output_callbacks.offset = (OutputRoutine)omf_output_offset;
    output_callbacks.size = (OutputRoutine)omf_output_size;
    output_callbacks.finish_module = (OutputRoutine)omf_finish_module;
    output_callbacks.relocation = (OutputRoutine)omf_output_relocation;
    output_callbacks.record = (void (__fastcall *)(ModuleDirective *))omf_output_record;
    output_callbacks.mark_symbol = (void (__fastcall *)(SymbolHeader *))omf_mark_symbol;
        if (codeview_debug || line_debug) {
    output_callbacks.debug_pair = (OutputRoutine)omf_output_debug_pair;
    output_callbacks.debug_record = (OutputRoutine)omf_output_debug_record;
        } else {
    output_callbacks.debug_pair = (OutputRoutine)output_noop_pair;
    output_callbacks.debug_record = (OutputRoutine)output_noop_debug;
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

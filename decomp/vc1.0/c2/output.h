#ifndef OUTPUT_H
#define OUTPUT_H
#include "backend-flow.h"
#include "segments.h"
/* Callback slots 0007b8f0..0007b938. The register-only slot signatures
   are refined with their callers; no opaque image data is retained. */
typedef struct ModuleDirective {
    struct ModuleDirective *next;
    unsigned kind;
    char *text;
} ModuleDirective;
typedef void (__fastcall *OutputRoutine)(void);
typedef struct OutputCallbacks {
    OutputRoutine debug_function;
    void (__fastcall *end_function)(FunctionSymbol *);
    OutputRoutine debug_record;
    OutputRoutine debug_pair;
    OutputRoutine size;
    OutputRoutine offset;
    OutputRoutine address;
    OutputRoutine bytes;
    OutputRoutine value;
    OutputRoutine data;
    OutputRoutine select_segment;
    void (__fastcall *begin_function)(FunctionSymbol *, char *, int, unsigned);
    OutputRoutine function;
    OutputRoutine symbol;
    OutputRoutine finish_debug;
    OutputRoutine finish_module;
    void (__fastcall *record)(ModuleDirective *);
    OutputRoutine relocation;
    void (__fastcall *mark_symbol)(SymbolHeader *);
} OutputCallbacks;
extern OutputCallbacks output_callbacks;
extern char *target_segment_group;
extern ModuleDirective *module_directives;
char *default_runtime_library(void);
void __fastcall add_module_directive(unsigned kind, char *text);
void coff_open_module(void);
void omf_open_module(void);
void initialize_codeview_output(void);
void output_noop(void);
void output_noop_pair(void);
void output_noop_data(void);
void output_noop_symbol(void);
void output_noop_function(void);
void output_noop_value(void);
void output_noop_debug(void);
void output_noop_relocation(void);
void output_noop_record(void);
void mark_unwritten_symbol(void);
void end_debug_function(void);
void begin_debug_function(void);
void finish_debug_module(void);
void coff_output_symbol(void);
void coff_output_function(void);
void coff_begin_function(void);
void __fastcall coff_write_data(void *data, unsigned bytes);
void __fastcall coff_select_segment(Segment *segment);
void coff_output_debug_pair(void);
void coff_output_debug_record(void);
void coff_output_value(void);
void coff_output_bytes(void);
void coff_output_address(void);
void coff_output_offset(void);
void coff_output_size(void);
void coff_finish_module(void);
void coff_output_relocation(void);
void __fastcall coff_output_record(ModuleDirective *directive);
void coff_mark_symbol(void);
void omf_output_symbol(void);
void omf_select_segment(void);
void omf_output_data(void);
void omf_output_debug_pair(void);
void omf_output_debug_record(void);
void omf_output_value(void);
void omf_output_bytes(void);
void omf_output_offset(void);
void omf_output_size(void);
void omf_finish_module(void);
void omf_output_relocation(void);
void omf_output_record(void);
void omf_mark_symbol(void);
#endif

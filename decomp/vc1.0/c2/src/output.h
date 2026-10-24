#ifndef OUTPUT_H
#define OUTPUT_H
#include "backend-flow.h"
#include "segments.h"
#include "initializers.h"
#include <recon.h>
/* Callback slots 0007b8f0..0007b938. Signatures are refined with their
   callers; no opaque image data is retained. */
typedef struct ModuleDirective {
    struct ModuleDirective *next;
    unsigned kind;
    char *text;
} ModuleDirective;
typedef void (__fastcall *OutputRoutine)(void);
typedef struct OutputCallbacks {
    OutputRoutine debug_function;
    void (__fastcall *end_function)(FunctionSymbol *);
    void (__fastcall *debug_record)(unsigned, unsigned);
    void (__fastcall *debug_pair)(char *);
    void (__fastcall *size)(unsigned, SymbolHeader *);
    void (__fastcall *relative_reference)(unsigned, SymbolHeader *);
    void (__fastcall *address)(unsigned, SymbolHeader *);
    void (__fastcall *absolute_reference)(unsigned, SymbolHeader *);
    void (__fastcall *segment_reference)(unsigned, SymbolHeader *);
    void (__fastcall *data)(void *, unsigned);
    void (__fastcall *select_segment)(Segment *);
    void (__fastcall *begin_function)(FunctionSymbol *, char *, int, unsigned);
    void (__fastcall *function)(FunctionSymbol *);
    void (__fastcall *symbol)(FunctionSymbol *);
    OutputRoutine finish_debug;
    OutputRoutine finish_module;
    void (__fastcall *record)(ModuleDirective *);
    unsigned (__fastcall *relocation)(Initializer *, SymbolHeader *, Segment *);
    void (__fastcall *mark_symbol)(SymbolHeader *);
} OutputCallbacks;
extern OutputCallbacks output_callbacks;
extern Segment *initializer_reference_segment; /* original79150; reset per initializer */
extern char *target_segment_group;
extern ModuleDirective *module_directives;
char *default_runtime_library(void);
void __fastcall add_module_directive(unsigned kind, char *text);
void coff_open_module(void);
void omf_open_module(void);
void initialize_codeview_output(void);
void output_noop(void);
void __fastcall output_noop_pair(char *filename);
void output_noop_data(void);
void __fastcall output_noop_symbol(void *value);
void output_noop_function(void);
void output_noop_value(void);
void __fastcall output_noop_debug(unsigned line, unsigned offset);
unsigned __fastcall output_noop_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment);
void output_noop_record(void);
void mark_unwritten_symbol(void);
void __fastcall end_debug_function(FunctionSymbol *symbol);
void begin_debug_function(void);
void finish_debug_module(void);
void __fastcall coff_output_symbol(FunctionSymbol *symbol);
void __fastcall coff_output_function(FunctionSymbol *symbol);
void __fastcall coff_begin_function(FunctionSymbol *symbol, char *filename, int frame_metadata, unsigned line);
void __fastcall coff_write_data(void *data, unsigned bytes);
void __fastcall coff_select_segment(Segment *segment);
void __fastcall coff_output_debug_pair(char *filename);
void __fastcall coff_output_debug_record(unsigned line, unsigned offset);
void coff_output_value(void);
void __fastcall coff_output_bytes(unsigned offset, SymbolHeader *symbol);
void __fastcall coff_output_address(unsigned offset, SymbolHeader *symbol);
void __fastcall coff_output_offset(unsigned offset, SymbolHeader *symbol);
void __fastcall coff_output_size(unsigned displacement, SymbolHeader *symbol);
void coff_finish_module(void);
unsigned __fastcall coff_output_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment);
void __fastcall coff_output_record(ModuleDirective *directive);
void __fastcall coff_mark_symbol(SymbolHeader *symbol);
void omf_output_symbol(void);
void __fastcall omf_select_segment(Segment *segment);
void __fastcall omf_output_data(void *data, unsigned bytes);
void omf_output_debug_pair(void);
void omf_output_debug_record(void);
void omf_output_value(void);
void omf_output_bytes(void);
void omf_output_offset(void);
void omf_output_size(void);
void omf_finish_module(void);
unsigned __fastcall omf_output_relocation(Initializer *initializer, SymbolHeader *symbol, Segment *segment);
void __fastcall omf_output_record(ModuleDirective *directive);
void omf_mark_symbol(void);
unsigned count_function_saved_registers(void);
void __fastcall write_function_frame_record(FunctionSymbol *symbol, unsigned offset, unsigned bytes, unsigned locals, unsigned short parameters, unsigned char prologue, unsigned char saved, unsigned char frame, unsigned char base_pointer);
#endif

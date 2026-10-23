#ifndef INITIALIZATION_H
#define INITIALIZATION_H
#include "frontend.h"
#include <recon.h>

typedef struct WorkingStack {
    unsigned count;
    unsigned char *next, *read, *storage;
} WorkingStack;
typedef CType TypeDescription;

extern WorkingStack *compiler_stack;
extern int pch_mode;
extern int pch_separate_debug, pch_read_enabled, pch_write_enabled;
extern unsigned pch_expression_bytes, pch_symbol_bytes;
extern unsigned char pch_include_state;
extern int data_pointer_bytes, code_pointer_bytes;
extern int data_pointer_alignment, code_pointer_alignment;
extern int data_pointer_kind, code_pointer_kind;
extern int data_pointer_model, code_pointer_model;
extern int normalized_Op, normalized_Oy, normalized_Oa, normalized_Ow, normalized_Oz;
extern int normalized_Gs, normalized_PLMN, normalized_PLMF, normalized_Zr;
extern int normalized_Oq, normalized_Gn, normalized_Ol;
extern int target_cpu, target_cpu_mask;
extern char token_buffer[7936];
extern char *token_buffer_end;
extern char *source_filename, *source_buffer_cursor;
extern int browser_full, browser_enabled;
extern char *browser_filename;
extern FILE *diagnostic_trace_file, *listing_file;
/* The numeric text limit admits byte128, which aliases the output stream
   used for preprocessing and /Zg declaration printing. Preserve that field. */
typedef union LexerTextStorage {
    char bytes[132];
    struct { char text[128]; FILE *output; } fields;
} LexerTextStorage;
extern LexerTextStorage lexer_text_storage;
#define token_text lexer_text_storage.bytes
#define preprocessor_stream lexer_text_storage.fields.output
extern CType *type_0000, *type_4041, *type_0064;
extern CType *type_4064_first, *type_4064_second;
extern CType *type_0042, *type_0044, *type_4044;
extern CType *type_0108, *type_0802, *type_0080;
extern CType *type_0041_q1000, *type_4041_q1000, *type_0064_q1000;
extern CType *type_implicit_int_function, *type_tagged_0080;
extern CType *type_indirect_0080_2, *type_indirect_0080_1, *type_4041_q1;

void __stdcall initialize_work_stack(WorkingStack **stack);
void __stdcall reset_work_stack(WorkingStack *stack);
void initialize_preprocessor(void);
void initialize_keyword_table(void);
void initialize_floating_point(void);
PackingEntry *top_packing(PackingStack *stack);
void __stdcall disable_keyword(char *name);
void begin_browser_output(void);
FILE *open_intermediate(char *prefix, char *extension, char *mode);
void open_expression_intermediate(void);
void __stdcall prepare_pch_output(char *filename);
extern int pch_expression_output, intermediate_status;
extern char *pch_diagnostic_name;
void initialize_inline_queries(void);
void cleanup_failed_compilation(void);
void report_fatal(unsigned code, char *kind, char *filename);
void initialize_type_table(void);
void initialize_global_scope(void);
CType *__stdcall intern_type(TypeDescription *description);
CType *__stdcall make_function_type(unsigned short attributes, CType *base);
CType *__stdcall make_tagged_type(unsigned short flags);
unsigned short __stdcall normalize_type_attributes(unsigned short kind, unsigned short attributes);
CType *__stdcall make_qualified_type(CType *base, unsigned short kind, unsigned short attributes, void *parameter);
void initialize_intermediate_output(void);
void initialize_translation_state(void);
void __stdcall define_command_line_macros(Vector *definitions);
#endif

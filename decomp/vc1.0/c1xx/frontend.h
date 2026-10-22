#ifndef FRONTEND_H
#define FRONTEND_H
#include <stdio.h>
#include "options.h"

typedef struct CType CType;
typedef struct IrToken {
    unsigned char kind, flags;
    unsigned short field02;
    CType *type;
    unsigned field08;
    union { char *text; void *pointer; unsigned integer; } value;
    unsigned short length, auxiliary_flags;
} IrToken;
typedef char IrTokenSizeMustBe20[(sizeof(IrToken) == 20) ? 1 : -1];

typedef struct CppSymbol CppSymbol;
extern unsigned initial_option_flags, token_allocation_flags;
extern unsigned cpp_parser_flags;
extern CompilerConfiguration saved_configuration;
extern int global_expression_scope_active, error_count;
extern int pch_mode;
extern char version_string[256];
extern Vector *forced_source_inputs;
extern short forced_source_index;
extern char *current_forced_source;
extern FILE *expression_file, *symbol_file, *line_file, *global_file;
extern FILE *debug_file, *preprocessor_file;

void initialize_frontend(void);
unsigned pack_option_flags(void);
void preprocess_translation_unit(void);
void __stdcall record_string_literal(IrToken *token);
void __stdcall emit_version_token(IrToken *token);
int compare_filenames(char *first, char *second);
void begin_automatic_pch(void);
void begin_debug_info(void);
void load_precompiled_header(void);
int __stdcall open_forced_include(char *filename, int flags);
int parse_translation_unit(void);
void __stdcall release_arena(int pool);
void finish_declarations(void);
void finish_types(void);
void finish_inline_functions(void);
void finish_expression_output(void);
void __stdcall write_ir_record(unsigned tag, void *value);
IrToken *__stdcall make_integer_constant(unsigned value);
IrToken *__stdcall new_integer_constant(unsigned value);
void finish_debug_info(void);
void write_global_header(void);
void generate_inline_queries(void);
void close_intermediate(FILE *file);
void finish_browser_output(void);
void report_error(unsigned code, ...);
void cleanup_precompiled_files(void);
#endif

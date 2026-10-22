#ifndef FRONTEND_H
#define FRONTEND_H
#include <stdio.h>
#include "options.h"
#include <recon.h>

typedef struct TypeShape TypeShape;
typedef struct CType {
    unsigned short flags, attributes;
    TypeShape *related;
    unsigned field08, field0c;
} CType;
typedef struct IrToken {
    unsigned char kind, flags;
    unsigned short field02;
    CType *type;
    unsigned field08;
    union { char *text; void *pointer; unsigned integer; } value;
    unsigned short length, auxiliary_flags;
} IrToken;

/* Expression nodes use the same twenty-byte header/payload as IrToken.
   The eight-byte payload is a string value, two children, or symbol pointer. */
typedef struct ExpressionNode {
    unsigned char kind, flags;
    unsigned short attributes;
    CType *type;
    unsigned auxiliary;
    union {
        struct { unsigned low, high; } words;
        struct { IrToken *left, *right; } children;
        struct FrontendSymbol *symbol;
    } value;
} ExpressionNode;
typedef char ExpressionNode_must_be_twenty_bytes[(sizeof(ExpressionNode) == 20) ? 1 : -1];

/* Argument nodes share the IR header, with a circular forward link and
   the parser's reverse argument chain occupying their two link words. */
typedef struct ArgumentNode {
    unsigned char kind, flags;
    unsigned short attributes;
    CType *type;
    struct ArgumentNode *next;
    IrToken *expression;
    struct ArgumentNode *previous;
} ArgumentNode;
typedef char ArgumentNode_must_be_twenty_bytes[sizeof(ArgumentNode) == 20 ? 1 : -1];

extern unsigned initial_option_flags, precompiled_option_flags;
extern int optimization_enabled;
extern int *packed_option_values[21];
extern int global_expression_scope_active, error_count;
extern char version_string[256];
extern Vector *forced_source_inputs;
extern short forced_source_index;
extern char *current_forced_source;
extern FILE *expression_file, *symbol_file, *line_file, *global_file;
extern FILE *debug_file, *preprocessor_file;

void initialize_frontend(void);
unsigned pack_option_flags(void);
void preprocess_translation_unit(void);
void __stdcall assign_string_type(IrToken *token);
IrToken *__stdcall materialize_string_literal(IrToken *token);
int compare_filenames(char *first, char *second);
void begin_automatic_pch(void);
void begin_debug_info(void);
void load_precompiled_header(void);
int __stdcall open_include_file(char *filename, int flags);
unsigned char scan_token(void);
void __stdcall unget_token(unsigned char token);
int parse_translation_unit(void);
void finish_declarations(void);
void __stdcall write_ir_record(unsigned tag, void *value);
IrToken *__stdcall make_integer_constant(unsigned value);
IrToken *__stdcall new_integer_constant(unsigned value);
void finish_debug_info(void);
void write_global_header(void);
void generate_inline_queries(void);
void close_intermediate(FILE *file);
void finish_browser_output(void);
void report_error(unsigned code, ...);
void report_syntax_error(unsigned code, ...);
void cleanup_precompiled_files(void);
#endif

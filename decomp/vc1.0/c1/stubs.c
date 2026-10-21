/* Unreconstructed function entries. Each body deliberately traps. */
#include "keywords.h"
#include "ir.h"
#include "types.h"
#include "builtins.h"
#include "macros.h"
#include "global-ir.h"
#include "source-input.h"
#include "lexer.h"

/* Original 0x43ea0. */
void compiler_signal(int signal_number) { __asm { int 3 } }

void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }

void clear_packing(PackingEntry *entry) { __asm { int 3 } }

void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }

void compiler_warning(unsigned category, unsigned code, ...) { __asm { int 3 } }

/* Original 0x4bee0. */

/* Original 0x43ec0. */
void preprocess_translation_unit(void) { __asm { int 3 } }

/* Original 0x276c0. */
void __stdcall record_version_token(IrToken *token) { __asm { int 3 } }

/* Original 0x3c080. */
void __stdcall emit_version_token(IrToken *token) { __asm { int 3 } }

/* Original 0x213f0. */
int compare_filenames(char *first, char *second) { __asm { int 3 } return 0; }

/* Original 0x45880. */
void begin_automatic_pch(void) { __asm { int 3 } }

/* Original 0x47450. */
void begin_debug_info(void) { __asm { int 3 } }

/* Original 0x1fda0. */
void load_precompiled_header(void) { __asm { int 3 } }

/* Original 0x20490. */
int __stdcall open_forced_include(char *filename, int flags) { __asm { int 3 } return 0; }

/* Original 0x199a0. */

/* Original 0x18bd0. */

/* Original 0x23192. */
void parse_translation_unit(void) { __asm { int 3 } }

/* Original 0x362c0. */
void finish_declarations(void) { __asm { int 3 } }

/* Original 0x474b0. */
void finish_debug_info(void) { __asm { int 3 } }

/* Original 0x36b30. */

/* Original 0x2e414. */
void generate_inline_queries(void) { __asm { int 3 } }

/* Original 0x4bb60. */
void close_intermediate(FILE *file) { __asm { int 3 } }

/* Original 0x27210. */
void finish_browser_output(void) { __asm { int 3 } }

/* Original 0x4aa40. */
void report_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x4a9d0. */
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }

/* Original 0x46e30. */
void cleanup_precompiled_files(void) { __asm { int 3 } }

/* Original 0x271a0. */
void begin_browser_output(void) { __asm { int 3 } }

/* Original 0x2e370. */
void initialize_inline_queries(void) { __asm { int 3 } }

/* Original 0x4aa90. */
void report_fatal(unsigned code, ...) { __asm { int 3 } }

/* Original 0x1bf70. */

void __stdcall prepare_pch_output(char *filename) { __asm { int 3 } }

void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode) { __asm { int 3 } }
void __stdcall reference_ir_symbol(IrSymbol *symbol) { __asm { int 3 } }
unsigned get_ir_symbol_index(IrSymbol *symbol) { __asm { int 3 } return 0; }

long ftell(FILE *stream) { __asm { int 3 } return -1; }

ParameterInfo *void_parameter_info(void) { __asm { int 3 } return 0; }
unsigned __stdcall hash_parameters(ParameterInfo *parameters) { __asm { int 3 } return 0; }

void browser_index_overflow(void) { __asm { int 3 } }
void __stdcall browser_builtin(FrontendSymbol *symbol) { __asm { int 3 } }

void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol) { __asm { int 3 } }



void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }

void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall browser_macro(MacroDefinition *definition) { __asm { int 3 } }


CType *__stdcall remove_outer_type(CType *type) { __asm { int 3 } return 0; }
int __stdcall type_size(CType *type) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int __stdcall parameter_stack_bytes(FrontendSymbol *symbol) { __asm { int 3 } return 0; }
unsigned short __stdcall type_alignment(CType *type) { __asm { int 3 } return 0; }
void __stdcall emit_symbol_declaration(FrontendSymbol *symbol) { __asm { int 3 } }
FrontendSymbol *__stdcall symbol_alias(FrontendSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall write_decorated_name(FILE *stream, FrontendSymbol *symbol) { __asm { int 3 } }
void write_ir_filename(char *text) { __asm { int 3 } }
void write_ir_string(char *text) { __asm { int 3 } }
void write_ir_index(unsigned *index) { __asm { int 3 } }

long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin) { __asm { int 3 } return -1; }
void emit_line_directive(void) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
void __stdcall record_line_information(unsigned flags) { __asm { int 3 } }
void browser_include_begin(void) { __asm { int 3 } }
void browser_open_file(void) { __asm { int 3 } }

void release_input_handle(void) { __asm { int 3 } }

char *__stdcall copy_string(char *text, int arena) { __asm { int 3 } return 0; }

int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes) { __asm { int 3 } return 0; }


int skip_source_comment(void) { __asm { int 3 } return 0; }

unsigned recover_special_character(void) { __asm { int 3 } return 0; }

unsigned recover_trigraph(void) { __asm { int 3 } return 0; }

int consume_line_escape(void) { __asm { int 3 } return 0; }

unsigned char next_source_character(void) { __asm { int 3 } return 0; }


void skip_source_line(void) { __asm { int 3 } }

void __stdcall copy_quoted_literal(int delimiter) { __asm { int 3 } }

unsigned char __stdcall scan_character_literal(int wide) { __asm { int 3 } return 0; }

void __stdcall scan_string_literal(int wide, int delimiter) { __asm { int 3 } }


int scan_macro_identifier(void) { __asm { int 3 } return 0; }

int __stdcall expand_macro(MacroDefinition *definition) { __asm { int 3 } return 0; }

unsigned char close_source_file(void) { __asm { int 3 } return 0; }

void finish_pch_output(void) { __asm { int 3 } }

int __stdcall match_source_character(int character) { __asm { int 3 } return 0; }

unsigned __stdcall read_number_text(char *destination) { __asm { int 3 } return 0; }

unsigned char __stdcall scan_number(int first_character) { __asm { int 3 } return 0; }

unsigned char __stdcall scan_float_tail(char *destination) { __asm { int 3 } return 0; }

unsigned char __stdcall scan_assembler_number(int first_character) { __asm { int 3 } return 0; }

void report_ide_line(void) { __asm { int 3 } }
void finish_pch_capture(void) { __asm { int 3 } }
void preprocess_directive(void) { __asm { int 3 } }

void scan_continued_identifier(char *start, unsigned hash) { __asm { int 3 } }

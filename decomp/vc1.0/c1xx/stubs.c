/* Named, unaccepted reconstruction frontiers. */
#include "initialization.h"
#include "inline-vector.h"
#include "ir.h"
#include "hash-table.h"
#include "types.h"
#include "parameters.h"
#include "macros.h"
#include "runtime.h"
#include "global-ir.h"
void interrupt_compile(int signal_number) { __asm { int 3 } }
void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }
void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }
void compiler_warning(int level, unsigned code, ...) { __asm { int 3 } }
unsigned pack_option_flags(void) { __asm { int 3 } return 0; }
void preprocess_translation_unit(void) { __asm { int 3 } }
void __stdcall record_version_token(IrToken *token) { __asm { int 3 } }
void __stdcall emit_version_token(IrToken *token) { __asm { int 3 } }
int compare_filenames(char *first, char *second) { __asm { int 3 } return 0; }
void begin_automatic_pch(void) { __asm { int 3 } }
void begin_debug_info(void) { __asm { int 3 } }
void load_precompiled_header(void) { __asm { int 3 } }
int __stdcall open_forced_include(char *filename, int flags) { __asm { int 3 } return 0; }
void parse_translation_unit(void) { __asm { int 3 } }
void __stdcall release_arena(int pool) { __asm { int 3 } }
void finish_declarations(void) { __asm { int 3 } }
void finish_types(void) { __asm { int 3 } }
void finish_inline_functions(void) { __asm { int 3 } }
void finish_expression_output(void) { __asm { int 3 } }
void finish_debug_info(void) { __asm { int 3 } }
void generate_inline_queries(void) { __asm { int 3 } }
void close_intermediate(FILE *file) { __asm { int 3 } }
void finish_browser_output(void) { __asm { int 3 } }
void report_error(unsigned code, ...) { __asm { int 3 } }
void cleanup_precompiled_files(void) { __asm { int 3 } }
void begin_browser_output(void) { __asm { int 3 } }
void initialize_inline_queries(void) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
void initialize_operators(void) { __asm { int 3 } }
void declare_runtime_helpers(void) { __asm { int 3 } }
void initialize_purecall(void) { __asm { int 3 } }
InlineVector *grow_inline_vector(InlineVector *vector) { __asm { int 3 } return 0; }
void __stdcall prepare_pch_output(char *filename) { __asm { int 3 } }
CType *builtin_integer_type(void) { __asm { int 3 } return 0; }
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode) { __asm { int 3 } }
void __stdcall reference_ir_symbol(CppSymbol *symbol) { __asm { int 3 } }
unsigned get_ir_symbol_index(CppSymbol *symbol) { __asm { int 3 } return 0; }
long ftell(FILE *file) { __asm { int 3 } return 0; }
DeclarationParameterInfo *intern_declaration_parameters(int count, CppSymbol *head) { __asm { int 3 } return 0; }
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }
int __stdcall open_source_file(char *filename, int mode) { __asm { int 3 } return 0; }
void *__stdcall retry_malloc(unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall report_macro_error(char *directive) { __asm { int 3 } }
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count) { __asm { int 3 } return 0; }
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall browser_macro(MacroDefinition *definition) { __asm { int 3 } }
CType *__stdcall drop_first_type_shape(CType *type) { __asm { int 3 } return 0; }
CType *__stdcall make_class_pointer_type(CppSymbol *class_symbol, unsigned short attributes) { __asm { int 3 } return 0; }
unsigned char __stdcall member_pointer_is_aggregate(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall method_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value) { __asm { int 3 } return 0; }
int __stdcall type_size(CType *type) { __asm { int 3 } return 0; }
void __stdcall require_complete_type(CType *type) { __asm { int 3 } }
int __stdcall parameter_stack_bytes(CppSymbol *symbol) { __asm { int 3 } return 0; }
unsigned short __stdcall type_alignment(CType *type) { __asm { int 3 } return 0; }
void __stdcall emit_symbol_declaration(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall write_decorated_name(FILE *stream, CppSymbol *symbol, void *context, int flags) { __asm { int 3 } return 0; }
void write_ir_filename(char *text) { __asm { int 3 } }
void write_ir_string(char *text) { __asm { int 3 } }
void write_ir_index(unsigned *index) { __asm { int 3 } }

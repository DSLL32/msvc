/* Named, unaccepted reconstruction frontiers. */
#include "initialization.h"
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
void __stdcall write_ir_record(unsigned tag, void *value) { __asm { int 3 } }
IrToken *__stdcall make_integer_constant(unsigned value) { __asm { int 3 } return 0; }
void finish_debug_info(void) { __asm { int 3 } }
void write_global_header(void) { __asm { int 3 } }
void generate_inline_queries(void) { __asm { int 3 } }
void close_intermediate(FILE *file) { __asm { int 3 } }
void finish_browser_output(void) { __asm { int 3 } }
void report_error(unsigned code, ...) { __asm { int 3 } }
void cleanup_precompiled_files(void) { __asm { int 3 } }
void initialize_preprocessor(void) { __asm { int 3 } }
void initialize_keyword_table(void) { __asm { int 3 } }
void initialize_floating_point(void) { __asm { int 3 } }
PackingEntry *top_packing(PackingStack *stack) { __asm { int 3 } return 0; }
void __stdcall disable_keyword(char *name) { __asm { int 3 } }
void initialize_cpp_context_stack(void) { __asm { int 3 } }
void begin_browser_output(void) { __asm { int 3 } }
FILE *open_intermediate(char *prefix, char *extension, char *mode) { __asm { int 3 } return 0; }
void open_expression_intermediate(void) { __asm { int 3 } }
void initialize_inline_queries(void) { __asm { int 3 } }
void report_fatal(unsigned code, ...) { __asm { int 3 } }
void initialize_type_table(void) { __asm { int 3 } }
void initialize_global_scope(void) { __asm { int 3 } }
void initialize_declaration_table(void) { __asm { int 3 } }
CType *__stdcall intern_type(TypeDescription *description) { __asm { int 3 } return 0; }
CType *__stdcall make_function_type(unsigned short attributes, CType *base) { __asm { int 3 } return 0; }
CType *__stdcall make_tagged_type(unsigned short flags) { __asm { int 3 } return 0; }
unsigned short __stdcall normalize_type_attributes(unsigned short kind, unsigned short attributes) { __asm { int 3 } return 0; }
CType *__stdcall make_qualified_type(CType *base, unsigned short kind, unsigned short attributes, void *parameter) { __asm { int 3 } return 0; }
void initialize_intermediate_output(void) { __asm { int 3 } }
void initialize_translation_state(void) { __asm { int 3 } }
void __stdcall define_command_line_macros(Vector *definitions) { __asm { int 3 } }
void initialize_operators(void) { __asm { int 3 } }
void declare_runtime_helpers(void) { __asm { int 3 } }
void initialize_purecall(void) { __asm { int 3 } }

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
#include "source-input.h"
#include "operators.h"
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
void declare_runtime_helpers(void) { __asm { int 3 } }
void initialize_purecall(void) { __asm { int 3 } }
InlineVector *grow_inline_vector(InlineVector *vector) { __asm { int 3 } return 0; }
void __stdcall prepare_pch_output(char *filename) { __asm { int 3 } }
CType *builtin_integer_type(void) { __asm { int 3 } return 0; }
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode) { __asm { int 3 } }
void __stdcall reference_ir_symbol(CppSymbol *symbol) { __asm { int 3 } }
unsigned get_ir_symbol_index(CppSymbol *symbol) { __asm { int 3 } return 0; }
long ftell(FILE *file) { __asm { int 3 } return 0; }
void report_syntax_error(unsigned code, ...) { __asm { int 3 } }
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
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin) { __asm { int 3 } return 0; }
void emit_line_directive(void) { __asm { int 3 } }
void report_ide_source(void) { __asm { int 3 } }
void __stdcall record_line_information(unsigned flags) { __asm { int 3 } }
void browser_include_begin(void) { __asm { int 3 } }
void browser_open_file(void) { __asm { int 3 } }
int release_input_handle(void) { __asm { int 3 } return 0; }
char *__stdcall copy_string(char *text, int arena) { __asm { int 3 } return 0; }
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes) { __asm { int 3 } return 0; }
void __stdcall declare_operator_delete(unsigned short attributes) { __asm { int 3 } }
void __stdcall restore_declaration_context(unsigned char mode, SymbolScope *scope) { __asm { int 3 } }
void browser_symbol_rollover(void) { __asm { int 3 } }
void __stdcall prepend_declarator(CppSymbol *list, CppSymbol *declaration) { __asm { int 3 } }
void __stdcall resolve_deferred_declaration(void *declaration) { __asm { int 3 } }
int __stdcall is_current_class(CppSymbol *symbol) { __asm { int 3 } return 0; }
char *__stdcall format_symbol_name(CppSymbol *symbol) { __asm { int 3 } return 0; }
void report_declaration_diagnostic(unsigned warning_code, unsigned error_code, ...) { __asm { int 3 } }
unsigned short __stdcall type_qualifiers(CType *type) { __asm { int 3 } return 0; }
CType *__stdcall strip_based_shape(CType *type) { __asm { int 3 } return 0; }
unsigned short __stdcall classify_incomplete_type(CType *type) { __asm { int 3 } return 0; }
void __stdcall warn_ignored_distance(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *get_current_class_symbol(void) { __asm { int 3 } return 0; }
CType *__stdcall replace_first_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter) { __asm { int 3 } return 0; }
DeclarationType *__stdcall global_declaration_type(DeclarationType *context) { __asm { int 3 } return 0; }
int __stdcall is_unqualified_class_type(CType *type) { __asm { int 3 } return 0; }
void retire_scope_directory_entry(void) { __asm { int 3 } }
void pop_symbol_scope(void) { __asm { int 3 } }
int __stdcall is_static_qualified_symbol(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall unwind_qualified_scopes(int stop_at_class) { __asm { int 3 } }
unsigned short begin_type_diagnostic(void) { __asm { int 3 } return 0; }
void end_type_diagnostic(void) { __asm { int 3 } }
short __stdcall format_declarator(CppSymbol *symbol, CType *type) { __asm { int 3 } return 0; }
char *__stdcall type_diagnostic_text(short text) { __asm { int 3 } return 0; }
void __stdcall browser_declaration(CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall declare_qualified_member(CppSymbol *symbol, QualifiedDeclarator *qualifier, CppSymbol *member) { __asm { int 3 } return 0; }
char *__stdcall format_qualified_name(QualifierBinding **binding, int verbose) { __asm { int 3 } return 0; }
CppSymbol *__stdcall symbol_construction_class(CppSymbol *symbol) { __asm { int 3 } return 0; }
int __stdcall symbol_has_array_shape(CppSymbol *symbol) { __asm { int 3 } return 0; }
void __stdcall initialize_zero_object(CppSymbol *symbol) { __asm { int 3 } }
void __stdcall finalize_object_storage(CppSymbol *symbol) { __asm { int 3 } }
void __stdcall validate_special_function(CppSymbol *symbol, int definition) { __asm { int 3 } }
short __stdcall format_storage_spec(short text, StorageSpec storage) { __asm { int 3 } return 0; }
int __stdcall merge_storage_spec(StorageSpec *destination, StorageSpec source) { __asm { int 3 } return 0; }
CType *__stdcall function_return_type(CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall report_symbol_declaration_error(unsigned code, CppSymbol *symbol) { __asm { int 3 } }
CppSymbol *__stdcall resolve_declaration_alias(CppSymbol *symbol, CppSymbol *declaration) { __asm { int 3 } return 0; }
int __stdcall merge_declared_symbol(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall static_member_alias(CppSymbol *symbol) { __asm { int 3 } return 0; }
int __stdcall is_class_name_type(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall lookup_outer_declaration(CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall find_outer_name(char *name) { __asm { int 3 } return 0; }
void __stdcall record_local_declaration(CppSymbol *declaration) { __asm { int 3 } }
void __stdcall check_declarator_prefix(CppSymbol *symbol, TypeShape *prefix) { __asm { int 3 } }
void __stdcall finish_array_declarator(CppSymbol *symbol, TypeShape **result, ShapeCursor current, int is_typedef) { __asm { int 3 } }
void __stdcall check_pointer_attributes(CppSymbol *symbol, ShapeCursor *current, unsigned diagnostic) { __asm { int 3 } }
int __stdcall merge_member_pointer_shape(CppSymbol *symbol, ShapeCursor inner, ShapeCursor *current, ShapeCursor outer) { __asm { int 3 } return 0; }
unsigned char __stdcall member_pointer_class_kind(CppSymbol *class_symbol) { __asm { int 3 } return 0; }
TypeShape *__stdcall merge_type_shape_prefix(TypeShape *head, TypeShape *tail) { __asm { int 3 } return 0; }
void __stdcall copy_shape_prefix(CppSymbol *symbol, TypeShape *shape, TypeShape **prefix, int is_typedef) { __asm { int 3 } }
int __stdcall has_class_initialization(CType *type) { __asm { int 3 } return 0; }
CppSymbol *__stdcall bind_function_overload(CppSymbol *overloads, CppSymbol *declaration) { __asm { int 3 } return 0; }
CppSymbol *__stdcall reconcile_typedef(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } return 0; }
void __stdcall merge_declaration_storage(CppSymbol *existing, CppSymbol *declaration) { __asm { int 3 } }
void __stdcall merge_function_defaults(FunctionParameters *current, FunctionParameters *previous, CppSymbol *declaration, int inherit) { __asm { int 3 } }
void __stdcall initialize_scope_buckets(SymbolScope *scope, CppSymbol *symbol) { __asm { int 3 } }
void __stdcall insert_class_member(CppSymbol *symbol) { __asm { int 3 } }
int __stdcall find_intrinsic_index(char *name) { __asm { int 3 } return 0; }
int __stdcall intrinsic_enabled(int index) { __asm { int 3 } return 0; }
void __stdcall apply_intrinsic(CppSymbol *symbol, int index) { __asm { int 3 } }
void __stdcall report_parameter_error(unsigned code, CppSymbol *symbol, unsigned parameter) { __asm { int 3 } }

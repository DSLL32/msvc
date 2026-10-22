#ifndef LEXER_H
#define LEXER_H
#include "parser.h"
#include "operators.h"
#include "macros.h"

/* A captured token retains both its grammar class and its lexical code. */
typedef struct DeferredToken {
    unsigned char parser_token, lexical_token;
    unsigned short reserved;
    ParserValue value;
    CppSymbol *type_specifier;
} DeferredToken;
typedef struct TokenReplayStorage {
    DeferredToken deferred[200], pushed[3];
} TokenReplayStorage;
extern TokenReplayStorage token_replay;
#define deferred_tokens (token_replay.deferred)
#define pushed_tokens (token_replay.pushed)
extern DeferredToken *deferred_token_end, *deferred_token_cursor;
extern DeferredToken *pushed_token_top, *pushed_token_begin;
extern int pushed_token_count;
/* Deferred-argument text scanning temporarily uses the same /E flag. */
#define argument_text_mode (options.option_E)
extern unsigned char current_token_code;
extern CppSymbol *type_specifier_symbol;

void __stdcall capture_declaration_tokens(int initial_parentheses);
int deferred_tokens_may_be_declaration(void);
unsigned char read_deferred_argument_text(void);
void __stdcall restore_qualified_lookup(QualifierBinding *binding);
unsigned char __stdcall resolve_parenthesis_candidate(int allow_expression);
unsigned char __stdcall resolve_typename_token(CppSymbol *symbol);
unsigned char read_operator_function_id(void);
unsigned char __stdcall resolve_identifier_token(char *name);
unsigned char read_lexical_token(void);
void __stdcall add_storage_specifier(StorageSpec *storage, unsigned char kind,
                                    unsigned char modifier, unsigned unused);
CType *__stdcall merge_type_specifier(CType *type, unsigned short *flags,
                                      unsigned short added_flags);
unsigned char __stdcall read_tag_specifier(unsigned short kind, unsigned short *attributes);
extern int suppress_vtordisp;
CppSymbol *__stdcall reference_tag_symbol(char *name, unsigned short kind,
                                         void (__stdcall *insert)(CppSymbol *));
CppSymbol *__stdcall match_tag_symbol(CppSymbol *symbol, unsigned short kind);
CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol);
CppSymbol *__stdcall define_tag_symbol(char *name, unsigned short kind, unsigned short attributes);
char *__stdcall type_kind_name(unsigned short kind);
extern CppSymbol *current_enum_symbol;
void __stdcall flush_current_function(int retain);
CppSymbol *__stdcall find_tag_in_chain(char *name, CppSymbol *first);
CppSymbol *__stdcall allocate_tag_symbol(char *name, unsigned short kind,
                                        void (__stdcall *insert)(CppSymbol *), int definition);
void __stdcall defer_nested_tag(CppSymbol *owner, CppSymbol *tag);
void browser_begin_tag(void);
int __stdcall browser_name_reference(int reference, char *name);
void __stdcall browser_enter_tag(CppSymbol *tag);
SymbolScope *__stdcall enter_class_scope(CppSymbol *tag, signed char mode);
int __stdcall symbol_has_tag_scope(CppSymbol *symbol);
extern int tag_declaration_seen;
TagSymbol *__stdcall new_tag_record(int arena);
void __stdcall register_tag_context(CppSymbol *tag);
int __stdcall is_current_class_name(char *name);
void __stdcall apply_inheritance_model(CppSymbol *tag);
void __stdcall prepare_base_layout(CppSymbol *tag);
unsigned __stdcall place_base_subobject(CppSymbol *base, CppSymbol **previous_base, CppSymbol *derived);
void __stdcall inherit_class_pointer_attributes(CppSymbol *tag);
void __stdcall apply_default_vtordisp(CppSymbol *unused_tag);
void __stdcall check_inheritance_model(CppSymbol *tag);
int __stdcall query_inheritance_model(int *model);
int __stdcall infer_inheritance_model(CppSymbol *tag);
CType *__stdcall apply_type_symbol(CppSymbol *symbol, unsigned short *flags,
                                   unsigned short *attributes);
void read_declspec_attributes(StorageSpec *storage);
unsigned char resolve_type_parenthesis(void);
extern unsigned char token_categories[213];
extern unsigned char pch_output_written;
int skip_source_comment(void);
int consume_line_escape(void);
unsigned char next_source_character(void);
unsigned recover_trigraph(void);
unsigned recover_special_character(void);
unsigned char close_source_file(void);
void finish_pch_output(void);
void finish_translation_symbols(void);
int emit_pending_runtime_functions(void);
void begin_source_line(void);
void report_ide_line(void);
void preprocess_directive(void);
void finish_pch_capture(void);
void skip_source_line(void);
int __stdcall write_work_character(int character, WorkingStack *stack);
void __stdcall write_work_bytes(const void *source, unsigned bytes, unsigned count, WorkingStack *stack);
unsigned char __stdcall scan_character_literal(int wide);
void __stdcall scan_string_literal(int wide, unsigned char delimiter);
void __stdcall copy_quoted_literal(unsigned char delimiter);
unsigned __stdcall read_number_text(char *destination);
unsigned char __stdcall scan_number(unsigned char first_character);
unsigned char __stdcall scan_float_tail(char *destination);
unsigned char __stdcall scan_assembler_number(unsigned char first_character);
void scan_source_identifier(void);
void scan_identifier_continuation(char *begin, unsigned hash);
int scan_macro_identifier(void);
int __stdcall expand_macro(MacroDefinition *macro);
int __stdcall match_source_character(unsigned char character);
typedef char DeferredTokenSizeMustBe16[(sizeof(DeferredToken) == 16) ? 1 : -1];
typedef char TokenReplayStorageSizeMustBe3248[(sizeof(TokenReplayStorage) == 3248) ? 1 : -1];
#endif

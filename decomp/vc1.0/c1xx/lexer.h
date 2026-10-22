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
extern unsigned char token_categories[213];
extern unsigned char pch_output_written;
extern int source_text_pending;
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

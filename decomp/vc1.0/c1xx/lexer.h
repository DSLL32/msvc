#ifndef LEXER_H
#define LEXER_H
#include "parser.h"
#include "operators.h"

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
typedef char DeferredTokenSizeMustBe16[(sizeof(DeferredToken) == 16) ? 1 : -1];
typedef char TokenReplayStorageSizeMustBe3248[(sizeof(TokenReplayStorage) == 3248) ? 1 : -1];
#endif

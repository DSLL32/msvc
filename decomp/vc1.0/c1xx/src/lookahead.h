#ifndef LOOKAHEAD_H
#define LOOKAHEAD_H
#include "lexer.h"
#include <recon.h>
extern int parenthesis_lookahead_depth;
/* The first value is the original yypv=-1 cell. The last value overlaps the
   first state cells; yys=-1 is the final halfword of the last allocated value.
   The final state halfword is the boundary admitted by the native > test. */
typedef struct LookaheadStateStorage {
    ParserValue earlier_values[150];
    unsigned short last_value_prefix[3];
    short cells[152];
} LookaheadStateStorage;
typedef union LookaheadParserStacks {
    ParserValue value_cells[153];
    LookaheadStateStorage state;
} LookaheadParserStacks;
extern LookaheadParserStacks lookahead_stacks;
#define lookahead_state_stack (lookahead_stacks.state.cells + 1)
#define lookahead_value_stack (lookahead_stacks.value_cells + 1)
extern int lookahead_token, lookahead_error_count;
extern short lookahead_recovery_count, lookahead_state;
extern short *lookahead_state_top;
extern ParserValue *lookahead_value_top;
typedef char LookaheadStatePrefixOffsetMustBe1206[
    (offsetof(LookaheadStateStorage, cells) == 1206) ? 1 : -1];
typedef char LookaheadParserStacksSizeMustBe1512[
    (sizeof(LookaheadParserStacks) == 1512) ? 1 : -1];
void __stdcall set_deferred_type_name(DeferredToken *token);
void __stdcall classify_deferred_type(DeferredToken *token);
int __stdcall deferred_scope_allows_expression(int depth);
void __stdcall resolve_nested_deferred_types(unsigned char unused_mode);
void initialize_lookahead_parser(void);
void __stdcall report_lookahead_error(char *unused_text);
int lookahead_parse(void);
int recursive_lookahead_parse(void);
unsigned char *__stdcall copy_lookahead_spelling(unsigned char *destination, char *text);
#endif

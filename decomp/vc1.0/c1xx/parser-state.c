/* Shared yacc state and its observed macro-workspace boundary aliases. */
#include "parser-state.h"

ParserStackStorage parser_stacks; /* original 0x94e44..0x956e4 */
ParserValue parser_value; /* original 0x8c1f0..0x8c1f8 */
ParserValue current_token_value; /* original 0x8c150..0x8c158 */
int parser_lookahead = -1; /* original 0x9eb9c */
int parser_error_count; /* original 0x9eba0 */
short parser_recovery_count; /* original 0x9eba4 */

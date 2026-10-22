#ifndef LOOKAHEAD_GRAMMAR_H
#define LOOKAHEAD_GRAMMAR_H
#include "parser-grammar.h"
extern ParserState lookahead_states[279];
extern ParserTransition lookahead_shifts[], lookahead_exceptions[];
extern ParserTransition lookahead_goto_transitions[];
extern ParserGoto lookahead_gotos[52];
extern ParserProduction lookahead_productions[170];
#endif

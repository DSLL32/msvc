#ifndef C1XX_DECLARATION_GRAMMAR_H
#define C1XX_DECLARATION_GRAMMAR_H
#include "parser-grammar.h"
#include <recon.h>
/* Separate logical declaration-classifier grammar; lexical classes are shared. */
#define DECLARATION_STATE_COUNT 301
#define DECLARATION_PRODUCTION_COUNT 182
#define DECLARATION_NONTERMINAL_COUNT 62
extern const ParserTransition declaration_shifts[];
extern const ParserTransition declaration_exceptions[];
extern const ParserState declaration_states[DECLARATION_STATE_COUNT];
extern const ParserGoto declaration_gotos[DECLARATION_NONTERMINAL_COUNT];
extern const ParserTransition declaration_goto_transitions[];
extern const ParserProduction declaration_productions[DECLARATION_PRODUCTION_COUNT + 1];
#endif

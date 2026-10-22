#ifndef PARSER_GRAMMAR_H
#define PARSER_GRAMMAR_H
#include <recon.h>

/* Logical yacc tables, generated from grammar.bnf rather than copied from
   the packed tables of the executable. State identities remain observable. */
typedef struct ParserTransition {
    short symbol, destination;
} ParserTransition;
typedef struct ParserState {
    unsigned short first_shift, shift_count, first_exception;
    unsigned char exception_count, needs_token;
    short default_rule;
} ParserState;
typedef struct ParserGoto {
    unsigned short first_transition, transition_count;
    short default_state;
} ParserGoto;
typedef struct ParserProduction {
    unsigned char nonterminal, length;
} ParserProduction;
typedef struct ParserRecovery {
    short state, token, next_state;
} ParserRecovery;

extern ParserState parser_states[575];
extern ParserTransition parser_shifts[], parser_exceptions[];
extern ParserTransition parser_goto_transitions[];
extern ParserGoto parser_gotos[145];
extern ParserProduction parser_productions[357];
extern ParserRecovery parser_recoveries[61];
#define PARSER_ERROR_TOKEN 256
#endif

#ifndef PARSER_STATE_H
#define PARSER_STATE_H
#include "declarations.h"

/* yacc copies exactly two words for each token/reduction value. Its typed
   views overlap; declaration storage uses the same three bytes as symbols. */
typedef union ParserValue {
    struct { unsigned low, high; } words;
    DeclarationType declaration;
    struct { CppSymbol *symbol; CType *type; } type_name;
    unsigned char bytes[8];
    unsigned short modifier;
    char *name;
    CppSymbol *symbol;
    IrToken *expression;
    TypeShape *shape;
    void *context;
} ParserValue;

/* The driver admits index 220 although the original arrays have 220 cells.
   Its last state aliases the first semantic halfword. Its last semantic cell
   aliases the macro workspace pointer and the following macro control word.
   These are shared original objects, not independent parser allocations. */
typedef union ParserSemanticStack {
    ParserValue values[221];
    struct {
        ParserValue allocated_values[220];
        char *macro_midpoint;
        unsigned macro_control;
    } control;
} ParserSemanticStack;
typedef union ParserStackStorage {
    short states_with_boundary[221];
    struct {
        short allocated_states[220];
        ParserSemanticStack semantic;
    } data;
} ParserStackStorage;
extern ParserStackStorage parser_stacks;
extern ParserValue parser_value, current_token_value;
extern int parser_lookahead, parser_error_count;
extern short parser_recovery_count;
#define parser_state_stack parser_stacks.states_with_boundary
#define parser_value_stack parser_stacks.data.semantic.values
#define macro_work_midpoint parser_stacks.data.semantic.control.macro_midpoint

typedef char ParserValueSizeMustBe8[(sizeof(ParserValue) == 8) ? 1 : -1];
typedef char ParserStackStorageSizeMustBe2208[(sizeof(ParserStackStorage) == 2208) ? 1 : -1];
typedef char ParserValueStackOffsetMustBe440[
    (sizeof(ParserStackStorage) - sizeof(ParserSemanticStack) == 440) ? 1 : -1];
typedef char ParserMacroPointerOffsetMustBe2200[
    (sizeof(ParserStackStorage) - sizeof(char *) - sizeof(unsigned) == 2200) ? 1 : -1];
#endif

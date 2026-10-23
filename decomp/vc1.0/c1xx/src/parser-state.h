#ifndef PARSER_STATE_H
#define PARSER_STATE_H
#include "declarations.h"
#include <recon.h>

/* yacc copies exactly two words for each token/reduction value. Its typed
   views overlap; declaration storage uses the same three bytes as symbols. */
typedef struct StringTokenValue {
    char *text;
    unsigned short length, flags;
} StringTokenValue;
typedef char StringTokenValueSizeMustBe8[(sizeof(StringTokenValue) == 8) ? 1 : -1];
typedef char StringTokenLengthOffsetMustBe4[(offsetof(StringTokenValue, length) == 4) ? 1 : -1];
typedef char StringTokenFlagsOffsetMustBe6[(offsetof(StringTokenValue, flags) == 6) ? 1 : -1];
typedef union ParserValue {
    struct { unsigned low, high; } words;
    StringTokenValue string;
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
   aliases the macro workspace pointer and the following signed rescan depth.
   These are shared original objects, not independent parser allocations. */
typedef union ParserSemanticStack {
    ParserValue values[221];
    struct {
        ParserValue allocated_values[220];
        char *macro_midpoint;
        int rescan_depth;
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
extern unsigned syntax_diagnostic_count;
extern short parser_recovery_count;
#define parser_state_stack parser_stacks.states_with_boundary
#define parser_value_stack parser_stacks.data.semantic.values
#define macro_work_midpoint parser_stacks.data.semantic.control.macro_midpoint
#define macro_rescan_depth parser_stacks.data.semantic.control.rescan_depth

typedef char ParserValueSizeMustBe8[(sizeof(ParserValue) == 8) ? 1 : -1];
typedef char ParserStackStorageSizeMustBe2208[(sizeof(ParserStackStorage) == 2208) ? 1 : -1];
typedef char ParserValueStackOffsetMustBe440[
    (sizeof(ParserStackStorage) - sizeof(ParserSemanticStack) == 440) ? 1 : -1];
typedef char ParserMacroPointerOffsetMustBe2200[
    (sizeof(ParserStackStorage) - sizeof(char *) - sizeof(int) == 2200) ? 1 : -1];
/* Original 0x94e44 + 2204 = 0x956e0. The 221st semantic value's
   high word is the same object read by the general character reader. */
typedef char ParserMacroRescanDepthOffsetMustBe2204[
    (sizeof(ParserStackStorage) - sizeof(int) == 2204) ? 1 : -1];
#endif

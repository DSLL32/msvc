#ifndef FUNCTION_INPUT_H
#define FUNCTION_INPUT_H
#include "functions.h"
#include <recon.h>
typedef struct TemporarySymbol {
    SymbolHeader symbol;
    unsigned field20, attributes;
    unsigned short field28;
    unsigned char anonymous, field2b;
} TemporarySymbol;
extern unsigned pending_source_location, function_source_location, current_source_location;
extern int lexical_depth, procedure_depth, end_function_records;
ScopeHash *new_scope(void);
Node *__fastcall read_expression(Function *function);
int __fastcall is_register_return_size(unsigned bytes);
int __fastcall needs_return_temp(unsigned convention, unsigned short kind, unsigned bytes);
TemporarySymbol *__fastcall create_temporary(ScopeHash *scope, unsigned type, long size,
    short storage, unsigned arena, signed char subclass, unsigned short attributes);
void __fastcall release_node(Node *node);
void __fastcall decode_inline_assembly(Node *node, ScopeHash *scope);
Expression *new_expression(void);
#endif

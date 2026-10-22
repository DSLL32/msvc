/* Declaration-level object initialization and its frontend records. */
#include "lexer.h"
#include "parser-state.h"
#include "object-initializers.h"

int initializer_runtime_override; /* original 0x9157c */
CppSymbol *initializer_storage_symbol; /* original 0x91590 */
unsigned initializer_offset; /* original 0x91598 */
int initializer_value_error_reported; /* original 0xa0a84 */
unsigned initializer_constructor_offset; /* original 0xa0a88 */
unsigned initializer_constructor_count; /* original 0xa0a8c */

/* Original 0x46160..0x461d9. The native EAX remains the incoming symbol
   through the final RET 4; its caller consumes that return value. */
CppSymbol *__stdcall prepare_initializer_symbol(CppSymbol *symbol)
{
    unsigned storage;
    initializer_runtime_override = 0;
    token_allocation_flags &= ~0x00100000UL;
    if (declaration_mode) {
        storage = symbol->storage[0] & 7;
        if (storage >= 4 && storage <= 6) token_allocation_flags |= 0x00200000UL;
        else token_allocation_flags &= ~0x00200000UL;
    } else token_allocation_flags &= ~0x00200000UL;
    initializer_storage_symbol = symbol;
    initializer_offset = 0;
    initializer_value_error_reported = 0;
    initializer_constructor_offset = 0;
    initializer_constructor_count = 0;
    return symbol;
}

/* Original 0x266a0..0x2670c. Recovery preserves a type/brace lookahead
   in token replay before inserting a semicolon. */
int __stdcall reject_function_initializer(CppSymbol *declaration)
{
    if (parser_lookahead == -1) parser_lookahead = read_parser_token();
    switch (parser_lookahead) {
    case 0x28:
    case 0x7f:
        unget_token((unsigned char)parser_lookahead);
        parser_lookahead = 0x8a;
        cpp_parser_flags |= 0x100;
        break;
    case 0x3f:
        break;
    default:
        return 0;
    }
    report_syntax_error(448, declaration->name);
    return 1;
}

/* Original 0x461e0..0x4628b. An initialized extern becomes a definition;
   prototype-only mode skips storage setup after the declaration checks. */
CppSymbol *__stdcall begin_object_initializer(CppSymbol *declaration)
{
    unsigned storage;
    if (!declaration) return 0;
    storage = declaration->storage[0] & 7;
    if (storage == 7) {
        report_syntax_error(71, declaration->name);
        return 0;
    }
    if (storage == 1) {
        if (declaration_mode) report_syntax_error(205, declaration->name);
        declaration->storage[0] = (declaration->storage[0] & 0xfa) | 2;
    }
    if (options.prototypes_only) return declaration;
    if ((declaration->storage[2] & 4) && !(declaration->storage[2] & 8))
        cpp_parser_flags |= 0x20000000UL;
    return prepare_initializer_symbol(declaration);
}

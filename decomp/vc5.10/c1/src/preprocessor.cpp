#include "compiler-state.h"
#include <recon.h>

/* One boolean flag selects which encountered token IDs receive a shared
 * prototype. Dynamic lexer records have their own later constructors.
 * Original table RVA63d70; only the used ID0..244 domain is modeled here. */
static const BYTE token_has_prototype[245] = {
    0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0,
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1,
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 1,
};

extern "C" void __cdecl initialize_predefined_tokens(void)
{
    unsigned kind;
    CompilerPool *pool = &c1_bss.pools[6];
    CompilerTokenPrototype *token;
    for (kind = 1; kind < 245; ++kind) {
        if (token_has_prototype[kind]) {
            if (pool->remaining < 32)
                pool->Refill(32);
            token = (CompilerTokenPrototype *)pool->cursor;
            pool->cursor += 32;
            pool->remaining -= 32;
            if (token) {
                token->kind = (BYTE)kind;
                token->tag = (BYTE)kind;
            }
            c1_data.predefined_tokens[kind] = token;
        } else {
            c1_data.predefined_tokens[kind] = NULL;
        }
    }
}

/* Native-only layout/alias check; no target virtual method is invoked. */
#include "input.h"
#include <recon.h>

extern "C" int create_input(int, CompilerInput **, CompilerTextInput **);
extern "C" void canonical_reset(void) __asm__("_ZN13CompilerLexer5ResetEv");

extern "C" int c1xx_test_input_alias(void)
{
    CompilerInput *input;
    CompilerTextInput *text;
    void **table;
    if (create_input(0, &input, &text) != 1 || !input || !text)
        return 0;
    if ((BYTE *)text != (BYTE *)input + 4)
        return 0;
    table = *(void ***)text;
    return table[2] == (void *)canonical_reset;
}

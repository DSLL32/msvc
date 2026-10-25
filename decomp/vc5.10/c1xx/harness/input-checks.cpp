/* Native-only layout/alias check; no target virtual method is invoked. */
#include "frontend-init.h"
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

extern "C" void begin_browser_output(const char *);
extern "C" void canonical_trace(unsigned) __asm__("_ZN13CompilerLexer5TraceEj");
extern "C" int c1xx_test_browser_aliases(void)
{
    static const unsigned reset_slots[] = {0,1,2,3,4,6,11,13,14,16,18};
    static const unsigned trace_slots[] = {5,7,9,12,15,17,19,20};
    void **table;
    unsigned i;
    begin_browser_output(NULL);
    if (!frontend_data.initial_backend || sizeof(CompilerBackend) != 4)
        return 0;
    table = *(void ***)frontend_data.initial_backend;
    for (i = 0; i < sizeof(reset_slots)/sizeof(reset_slots[0]); ++i)
        if (table[reset_slots[i]] != (void *)canonical_reset) return 0;
    for (i = 0; i < sizeof(trace_slots)/sizeof(trace_slots[0]); ++i)
        if (table[trace_slots[i]] != (void *)canonical_trace) return 0;
    return 1;
}

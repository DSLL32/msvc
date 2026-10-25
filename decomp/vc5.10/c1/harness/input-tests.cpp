#include "input.h"
#include <recon.h>

extern "C" int __cdecl create_input(int, CompilerInput **, CompilerTextInput **);

/* Independent factory contract: two interface subobjects, and the recorded
 * stream's initially empty token buffer. No pending virtual method is called. */
extern "C" int c1_test_input_factory(void)
{
    CompilerInput *input = NULL;
    CompilerTextInput *text = NULL;
    int ok;
    if (create_input(0, &input, &text) != 1 || !input || !text)
        return 0;
    ok = (BYTE *)text == (BYTE *)input + 4 &&
        *(void **)input != NULL && *(void **)text != NULL &&
        *(void **)input != *(void **)text;
    recon_cpp_delete(input);
    if (!ok || create_input(1, &input, &text) != 1 || !input || !text)
        return 0;
    CompilerRecordedInput *recorded = (CompilerRecordedInput *)input;
    ok = (BYTE *)text == (BYTE *)input + 4 &&
        *(void **)input != NULL && *(void **)text != NULL &&
        *(void **)input != *(void **)text &&
        recorded->token_bytes == (BYTE *)-1 && recorded->token_length == 0;
    /* token_start is deliberately neither read nor assigned here. */
    recon_cpp_delete(input);
    return ok;
}

extern "C" void __cdecl begin_browser_output(const char *);
extern "C" int c1_test_browser_factory(void)
{
    static const unsigned empty_slots[] = { 1, 2, 3, 4, 6, 11, 13, 14, 16, 22 };
    static const unsigned argument_slots[] = { 7, 9, 12, 15 };
    const char filename[] = "owned-browser.sbr";
    unsigned owned_marker = 0, i;
    CompilerBackend *default_object;
    void **table;
    char *copy;
    c1_data.browser_mode = 17;
    c1_data.named_browser = (CompilerBackend *)&owned_marker;
    begin_browser_output(NULL);
    default_object = c1_data.initial_backend;
    if (!default_object || c1_data.browser_mode != 17 ||
        c1_data.named_browser != (CompilerBackend *)&owned_marker) return 0;
    table = *(void ***)default_object;
    if (!table || !table[0] || !table[5] || table[0] == table[5]) return 0;
    for (i = 0; i < sizeof(empty_slots) / sizeof(empty_slots[0]); ++i)
        if (table[empty_slots[i]] != table[0]) return 0;
    for (i = 0; i < sizeof(argument_slots) / sizeof(argument_slots[0]); ++i)
        if (table[argument_slots[i]] != table[5]) return 0;
    begin_browser_output(filename);
    if (!c1_data.named_browser || c1_data.browser_mode != 3 ||
        c1_data.initial_backend != default_object) return 0;
    /* The only recovered named-browser data is its owned filename at +4.
     * Its final four allocation bytes are deliberately never inspected. */
    copy = *(char **)((BYTE *)c1_data.named_browser + 4);
    if (!copy || copy == filename) return 0;
    for (i = 0; i < sizeof(filename); ++i)
        if (copy[i] != filename[i]) return 0;
    recon_free(copy); /* _strdup owns a CRT allocation, not an operator-new block. */
    recon_cpp_delete(c1_data.named_browser);
    recon_cpp_delete(default_object);
    return 1;
}

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

/* Check the actual C++ table without invoking pending virtual methods. */
extern "C" void scope_has_buckets(void) __asm__("_ZN13CompilerScope10HasBucketsEv");
extern "C" void scope_has_secondary(void) __asm__("_ZN13CompilerScope12HasSecondaryEv");
extern "C" void scope_insert(void) __asm__("_ZN13CompilerScope6InsertEP14CompilerSymbol");
extern "C" void scope_find(void) __asm__("_ZN13CompilerScope4FindEP17CompilerNameEntryj");
extern "C" void scope_find_ordinary(void) __asm__("_ZN13CompilerScope12FindOrdinaryEP17CompilerNameEntry");
extern "C" void scope_lookup(void) __asm__("_ZN13CompilerScope6LookupEP17CompilerNameEntryjPj");
extern "C" void scope_find_nested(void) __asm__("_ZN13CompilerScope10FindNestedEP17CompilerNameEntryPP14CompilerSymbol");
extern "C" void scope_find_link(void) __asm__("_ZN13CompilerScope8FindLinkEP18CompilerSymbolLink");
extern "C" void scope_serialize(void) __asm__("_ZN13CompilerScope9SerializeEv");
extern "C" void scope_check(void) __asm__("_ZN13CompilerScope5CheckEv");
extern "C" void scope_link(void) __asm__("_ZN13CompilerScope4LinkEP14CompilerSymbol");
extern "C" int c1xx_test_scope_table(void)
{
    CompilerScope scope;
    void **table = *(void ***)&scope;
    static void (*const expected[])(void) = {scope_has_buckets, scope_has_secondary, scope_insert, scope_find, scope_find_ordinary, scope_lookup, scope_find_nested, scope_find_link, scope_serialize, scope_check, scope_link};
    unsigned i;
    for (i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i)
        if (table[i] != (void *)expected[i]) return 0;
    return sizeof(scope) == 32 && scope.next == NULL && scope.level == 0 &&
        scope.outer == NULL && scope.context == NULL && scope.bucket_mask == 0x3ff &&
        scope.secondary == NULL && sizeof(CompilerNamespaceSymbol) <= 20;
}

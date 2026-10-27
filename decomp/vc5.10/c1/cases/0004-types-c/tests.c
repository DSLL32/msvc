#include <string.h>
#include "compiler-state.h"
#include "case.h"
#include <recon.h>
extern void __cdecl merge_tag_reference_attributes(CompilerIrSymbol *, CompilerDeclarationAttributes *);
static int tag_attribute_write_extent(void)
{
    DWORD storage[15];
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)(storage + 1);
    CompilerDeclarationAttributes attributes, saved;
    unsigned pass, i;
    static const WORD choices[] = { 0, 4, 8, 12 };
    for (pass = 0; pass < 4; ++pass) {
        BYTE expected = choices[pass] ? (BYTE)((0xa5 & ~12) | choices[pass]) : 0xa5;
        memset(storage, 0xa5, sizeof(storage));
        memset(&attributes, 0x5a, sizeof(attributes));
        attributes.extended_flags = choices[pass];
        saved = attributes;
        merge_tag_reference_attributes(symbol, &attributes);
        if (memcmp(&attributes, &saved, sizeof(saved))) return 0;
        for (i = 0; i < sizeof(storage); ++i)
            if (((BYTE *)storage)[i] != (i == 4 + 30 ? expected : 0xa5)) return 0;
    }
    attributes.extended_flags = 0;
    merge_tag_reference_attributes(NULL, &attributes);
    return 1;
}
extern int c1_test_deferred_literal_buffer(void);
const ReconTest type_tests[2] = {
    { "tag attributes update only their linkage bits and retain inputs, gaps and neighbors", 1, tag_attribute_write_extent },
    { "literal records use the deferred buffer without consuming pending ordinary initialization", 1, c1_test_deferred_literal_buffer }
};

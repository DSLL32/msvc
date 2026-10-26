#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include "legacy-symbols.h"
#include "legacy-types.h"
#include "pack.h"
#include <recon.h>

/* Common callback checks prove pointer identities. These are the remaining
 * scalar/ABI facts of the actual table at00412288..004122b8. */
static int base_symbol_layout(void)
{
    static const unsigned char kinds[3] = {4, 5, 0x0d};
    static const unsigned short narrow[3] = {0x100, 0x101, 0x1001};
    static const unsigned short wide[3] = {0x1006, 0x1007, 0x1001};
    unsigned i;
    if (sizeof(LegacyBaseSymbolConverter) != 16 ||
        offsetof(LegacyBaseSymbolConverter, narrow) != 4 ||
        offsetof(LegacyBaseSymbolConverter, wide) != 12) return 0;
    for (i = 0; i < 3; ++i)
        if (legacy_base_symbol_converters[i].kind != kinds[i] ||
            legacy_base_symbol_converters[i].narrow_kind != narrow[i] ||
            legacy_base_symbol_converters[i].wide_kind != wide[i]) return 0;
    return 1;
}
/* User-authorized native-only qualification of the two opaque converters.
 * This checks their actual target semantics, not CLI reach or index validity:
 * target8530/85c0 copy every payload byte without interpreting its parent.
 * Each original ownership class is exercised, including actual pool reuse. */
static int cobol_payload(void (__cdecl *convert)(TypeRecord *),
                         unsigned char leaf, unsigned short output_leaf,
                         const unsigned char *payload, unsigned payload_size)
{
    static const unsigned short ownership[4] = {0, 8, 0x10, 0x20};
    unsigned char borrowed[64], saved[64], *old, *recycled;
    TypeRecord record, expected;
    unsigned i;
    for (i = 0; i < 4; ++i) {
        if (!ownership[i]) old = borrowed;
        else if (ownership[i] == 0x10) old = allocate_small_type();
        else if (ownership[i] == 0x20) old = allocate_medium_type();
        else old = allocate_bytes(64);
        old[0] = 0;
        *(unsigned short *)(old + 1) = (unsigned short)(payload_size + 1);
        old[3] = leaf;
        memcpy(old + 4, payload, payload_size);
        memcpy(saved, old, payload_size + 4);
        memset(&record, 0x5a, sizeof(record));
        record.data = old;
        record.status = (unsigned short)(0x2000 | ownership[i]);
        expected = record;
        current_type_count = 0;
        convert(&record);
        if (current_type_count != 1 || record.data == old ||
            *(unsigned short *)record.data != payload_size + 2 ||
            *(unsigned short *)(record.data + 2) != output_leaf ||
            memcmp(record.data + 4, payload, payload_size)) return 0;
        expected.data = record.data;
        expected.status = 0x2408;
        if (memcmp(&record, &expected, sizeof(record))) return 0;
        if (!ownership[i] && memcmp(old, saved, payload_size + 4)) return 0;
        if (ownership[i] == 0x10 || ownership[i] == 0x20) {
            recycled = ownership[i] == 0x10 ? allocate_small_type() : allocate_medium_type();
            if (recycled != old) return 0;
            free(recycled);
        }
        free(record.data);
    }
    return 1;
}
static int cobol_parent_payload(void)
{
    /* Primary C6 encoded parent, level1, nonliteral alphanumeric size32.
     * The target copies this prefix unchanged; no parent widening is claimed. */
    static const unsigned char payload[] = {0x83, 1, 2, 1, 0xa0, 31};
    return cobol_payload(convert_legacy_cobol0, 0xa6, 0x100a,
                         payload, sizeof(payload));
}
static int cobol_root_payload(void)
{
    /* Level0, uninterpreted NameAlg WORD0, counted root name. */
    static const unsigned char payload[] = {0, 0, 0, 7, 'M', 'E', 'S', 'S', 'A', 'G', 'E'};
    return cobol_payload(convert_legacy_cobol1, 0xa7, 0x0c,
                         payload, sizeof(payload));
}
const ReconTest cvpack_pointer_tests[3] = {
    { "base-symbol-layout", 0, base_symbol_layout },
    { "cobol-parent-payload-native-only", 1, cobol_parent_payload },
    { "cobol-root-payload-native-only", 1, cobol_root_payload }
};

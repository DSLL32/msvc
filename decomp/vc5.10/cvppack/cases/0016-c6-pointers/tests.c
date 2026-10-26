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

/* Native-only qualification from target5630: WORD contribution count, 12-byte
 * stride, unsigned counted length, exact copy and one appended terminator. */
static int module_name_layout(void)
{
    static const unsigned short counts[4] = {0, 1, 3, 256};
    static const unsigned lengths[4] = {0, 9, 37, 255};
    ModuleInfo info, saved;
    unsigned char *data, *before, *name;
    const char *result;
    unsigned i, j, size;
    if (offsetof(ModuleInfo, data) != 12 || sizeof(ModuleCodeSegment) != 12) return 0;
    for (i = 0; i < 4; ++i) {
        size = 8 + counts[i] * 12 + 1 + lengths[i];
        data = malloc(size);
        before = malloc(size);
        if (!data || !before) return 0;
        memset(data, 0xa5, size);
        *(unsigned short *)(data + 4) = counts[i];
        name = data + 8 + counts[i] * 12;
        name[0] = (unsigned char)lengths[i];
        for (j = 0; j < lengths[i]; ++j) name[j + 1] = (unsigned char)('a' + j % 26);
        memcpy(before, data, size);
        memset(&info, 0x5a, sizeof(info));
        info.data = data;
        info.size = size;
        saved = info;
        result = module_name(&info);
        if (!result || memcmp(result, name + 1, lengths[i]) || result[lengths[i]] ||
            memcmp(data, before, size) || memcmp(&info, &saved, sizeof(info))) return 0;
        free(before);
        free(data);
    }
    return 1;
}
static int module_name_shared_buffer(void)
{
    unsigned char first[8 + 1 + 255], second[8 + 12 + 1 + 3], expected[256];
    ModuleInfo info;
    const char *before, *after;
    unsigned i;
    memset(first, 0, sizeof(first));
    first[8] = 255;
    for (i = 0; i < 255; ++i) first[9 + i] = (unsigned char)('A' + i % 26);
    memset(&info, 0, sizeof(info));
    info.data = first;
    before = module_name(&info);
    memcpy(expected, first + 9, 255);
    expected[255] = 0;
    if (memcmp(before, expected, sizeof(expected))) return 0;
    memset(second, 0, sizeof(second));
    *(unsigned short *)(second + 4) = 1;
    second[20] = 3;
    memcpy(second + 21, "end", 3);
    info.data = second;
    after = module_name(&info);
    memcpy(expected, "end", 3);
    expected[3] = 0;
    return before == after && !memcmp(after, expected, sizeof(expected));
}
const ReconTest cvpack_pointer_tests[5] = {
    { "base-symbol-layout", 0, base_symbol_layout },
    { "cobol-parent-payload-native-only", 1, cobol_parent_payload },
    { "cobol-root-payload-native-only", 1, cobol_root_payload },
    { "module-name-layout-native-only", 1, module_name_layout },
    { "module-name-shared-buffer-native-only", 1, module_name_shared_buffer }
};

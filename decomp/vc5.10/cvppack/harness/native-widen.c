/* Linux host-only, case-bound MSPDB responses. Never part of the target graph. */
#define RECON_SHIM_IMPL 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "native-widen-abi.h"
#include <recon.h>

typedef struct Response {
    unsigned factory_capacity, factory_flags, factory_result;
    unsigned input_size, output_capacity, fixup_capacity, output_size, fixup_count;
    unsigned char *input, *output, *fixups;
} Response;
static Response responses[21];
static unsigned cursor, stage, enabled;
static WidenTi opaque;
static WidenOutput *descriptor, *description_identity;
static const void *input_pointer, *input_identity;
static WidenTi **factory_identity;
static unsigned char *output_identity;
static unsigned long *fixup_identity;
static unsigned maximum_output, maximum_fixups;
static const WidenVTable vtable;

static void require(int condition, const char *what)
{
    if (!condition) recon_unsupported(what);
}
static unsigned word(FILE *file)
{
    unsigned char bytes[4];
    require(fread(bytes, 1, 4, file) == 4, "CVPACK dependency truncated word");
    return bytes[0] | (unsigned)bytes[1] << 8 | (unsigned)bytes[2] << 16 | (unsigned)bytes[3] << 24;
}
static unsigned char *block(FILE *file, unsigned size)
{
    unsigned char *value;
    require(size <= 65536, "CVPACK dependency oversized response");
    value = malloc(size ? size : 1);
    require(value != NULL && fread(value, 1, size, file) == size, "CVPACK dependency truncated block");
    return value;
}
static void finished(void)
{
    require(!enabled || (cursor == 21 && stage == 0), "CVPACK dependency incomplete lifetime sequence");
}
static void active(WidenTi *object, unsigned expected)
{
    require(enabled && cursor < 21 && stage == expected && object == &opaque,
            "CVPACK dependency object or sequence");
    require(object->vtable == &vtable, "CVPACK dependency opaque vtable identity");
    require(recon_GetLastError() == 0, "CVPACK dependency LastError input");
}
static int disjoint(const void *left, unsigned left_size, const void *right, unsigned right_size)
{
    uintptr_t a = (uintptr_t)left, b = (uintptr_t)right;
    return a <= UINTPTR_MAX - left_size && b <= UINTPTR_MAX - right_size &&
        (a + left_size <= b || b + right_size <= a);
}
static void RECON_THISCALL destroy(WidenTi *object)
{
    active(object, 3);
    stage = 0;
    ++cursor;
}
static void RECON_THISCALL unknown(WidenTi *object)
{
    recon_unsupported("CVPACK dependency unobserved vtable slot");
}
static int RECON_THISCALL record(WidenTi *object, void *output, const void *input, unsigned *size)
{
    recon_unsupported("CVPACK dependency unobserved slot0x0c");
    return 0;
}
static int RECON_THISCALL query(WidenTi *object, WidenOutput *out,
                            const void *input, unsigned size, unsigned flags)
{
    Response *response;
    active(object, 1);
    response = &responses[cursor];
    require(out && input && !flags && size == response->input_size &&
        !memcmp(input, response->input, size), "CVPACK dependency query input");
    require(!out->bytes && !out->fixups && !out->output && !out->records,
            "CVPACK dependency query descriptor");
    if (!description_identity) description_identity = out;
    if (!input_identity) input_identity = input;
    require(out == description_identity && input == input_identity,
            "CVPACK dependency cross-instance input aliases");
    require(disjoint(out, sizeof(*out), input, size), "CVPACK dependency query aliases");
    descriptor = out;
    input_pointer = input;
    out->bytes = response->output_capacity;
    out->fixups = response->fixup_capacity;
    stage = 2;
    return 1;
}
static int RECON_THISCALL emit(WidenTi *object, WidenOutput *out,
                           const void *input, unsigned size, unsigned flags)
{
    Response *response;
    active(object, 2);
    response = &responses[cursor];
    require(out == descriptor && input == input_pointer && !flags &&
        size == response->input_size && !memcmp(input, response->input, size),
        "CVPACK dependency emit input or aliases");
    require(out->bytes == response->output_capacity && out->fixups == response->fixup_capacity &&
        out->output && out->records, "CVPACK dependency emit descriptor");
    require(disjoint(input, size, out->output, out->bytes) &&
        disjoint(input, size, out->records, out->fixups * 8) &&
        disjoint(out, sizeof(*out), out->output, out->bytes) &&
        disjoint(out, sizeof(*out), out->records, out->fixups * 8) &&
        disjoint(out->output, out->bytes, out->records, out->fixups * 8),
        "CVPACK dependency output aliases");
    if (response->output_capacity <= maximum_output)
        require(out->output == output_identity, "CVPACK dependency unchanged output allocation identity");
    else maximum_output = response->output_capacity;
    if (response->fixup_capacity <= maximum_fixups)
        require(out->records == fixup_identity, "CVPACK dependency unchanged fixup allocation identity");
    else maximum_fixups = response->fixup_capacity;
    output_identity = out->output;
    fixup_identity = out->records;
    /* The actual counts do not cover capacity tails. Preserve THIS native
     * buffer's tails; original uninitialized capacity bytes are never replayed. */
    memcpy(out->output, response->output, response->output_size);
    memcpy(out->records, response->fixups, response->fixup_count * 8);
    out->bytes = response->output_size;
    out->fixups = response->fixup_count;
    stage = 3;
    return 1;
}
static const WidenVTable vtable = {
    destroy, unknown, unknown, record, unknown, unknown, unknown, unknown, query, emit
};

int __cdecl cvpack_widen_factory(WidenTi **out, unsigned capacity, int flags)
{
    Response *response;
    require(enabled && cursor < 21 && !stage, "CVPACK dependency unexpected factory");
    require(recon_GetLastError() == 0, "CVPACK dependency factory LastError");
    response = &responses[cursor];
    require(out && capacity == response->factory_capacity && (unsigned)flags == response->factory_flags,
            "CVPACK dependency factory arguments");
    if (!factory_identity) factory_identity = out;
    require(out == factory_identity, "CVPACK dependency factory output alias");
    opaque.vtable = &vtable;
    *out = &opaque;
    stage = 1;
    return response->factory_result;
}

void cvpack_widen_load(const char *filename, const char *input_name)
{
    FILE *file, *input;
    unsigned char magic[4], module[64];
    unsigned count, input_size, i;
    int saved_errno = errno;
    require(!enabled && filename && input_name, "CVPACK dependency duplicate initialization");
    file = fopen(filename, "rb");
    require(file && fread(magic, 1, 4, file) == 4 && !memcmp(magic, "RWP1", 4),
            "CVPACK dependency provider header");
    count = word(file);
    input_size = word(file);
    require(count == 21 && input_size == 33280 && fread(module, 1, 64, file) == 64 &&
        !memcmp(module, "cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33", 64),
        "CVPACK dependency provider identity");
    input = fopen(input_name, "rb");
    require(input != NULL, "CVPACK dependency input file missing");
    for (i = 0; i < input_size; ++i) {
        int expected = fgetc(file), actual = fgetc(input);
        require(expected != EOF && actual == expected, "CVPACK dependency complete input file");
    }
    require(fgetc(input) == EOF && !ferror(input), "CVPACK dependency input extent");
    fclose(input);
    for (i = 0; i < count; ++i) {
        Response *response = &responses[i];
        response->factory_capacity = word(file);
        response->factory_flags = word(file);
        response->factory_result = word(file);
        response->input_size = word(file);
        response->output_capacity = word(file);
        response->fixup_capacity = word(file);
        response->output_size = word(file);
        response->fixup_count = word(file);
        require(response->factory_capacity == 256 && !response->factory_flags && response->factory_result == 1 &&
            response->output_capacity <= 65536 && response->fixup_capacity <= 8192 &&
            response->output_size <= response->output_capacity && response->fixup_count <= response->fixup_capacity,
            "CVPACK dependency response bounds");
        response->input = block(file, response->input_size);
        response->output = block(file, response->output_size);
        response->fixups = block(file, response->fixup_count * 8);
    }
    require(fgetc(file) == EOF && !ferror(file), "CVPACK dependency trailing provider data");
    fclose(file);
    enabled = 1;
    require(atexit(finished) == 0, "CVPACK dependency completion check");
    errno = saved_errno;
}

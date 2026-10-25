/* Linux host-only, case-bound MSPDB responses. Never part of the target graph. */
#define RECON_SHIM_IMPL 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "native-widen-abi.h"
#include "pack.h"
#include <recon.h>

typedef struct RecordResponse {
    unsigned offset, input_size, before, after, result, output_size;
    unsigned char *output;
} RecordResponse;
typedef struct Response {
    unsigned module, kind, record_count;
    RecordResponse *records;
    unsigned factory_capacity, factory_flags, factory_result;
    unsigned input_size, output_capacity, fixup_capacity, output_size, fixup_count;
    unsigned char *input, *output, *fixups;
} Response;
static Response responses[21];
static unsigned cursor, stage, enabled, version, response_count, record_cursor;
static unsigned char *complete_input, *record_input_base, *record_output_base;
static unsigned *record_size_identity;
static unsigned record_input_position, record_output_position;
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
    require(!enabled || (cursor == response_count && stage == 0), "CVPACK dependency incomplete lifetime sequence");
}
static void active(WidenTi *object, unsigned expected)
{
    require(enabled && cursor < response_count && stage == expected && object == &opaque,
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
    if (version == 2 && responses[cursor].kind == 1) {
        active(object, 1);
        require(record_cursor == responses[cursor].record_count, "CVPACK dependency incomplete Record sequence");
    } else active(object, 3);
    stage = 0;
    ++cursor;
}
static void RECON_THISCALL unknown(WidenTi *object)
{
    recon_unsupported("CVPACK dependency unobserved vtable slot");
}
static int RECON_THISCALL record(WidenTi *object, void *output, const void *input, unsigned *size)
{
    Response *response;
    RecordResponse *item;
    active(object, 1);
    response = &responses[cursor];
    require(version == 2 && response->kind == 1 && record_cursor < response->record_count,
            "CVPACK dependency unobserved slot0x0c");
    item = &response->records[record_cursor];
    require(input && output && size && *size == item->before &&
        !memcmp(input, complete_input + item->offset, item->input_size),
        "CVPACK dependency Record input or capacity");
    require(disjoint(input, item->input_size, output, *size) &&
        disjoint(size, sizeof(*size), input, item->input_size) &&
        disjoint(size, sizeof(*size), output, *size), "CVPACK dependency Record aliases");
    if (!record_cursor) {
        record_input_base = (unsigned char *)input;
        record_output_base = output;
        record_size_identity = size;
    }
    require(input == record_input_base + record_input_position &&
        output == record_output_base + record_output_position && size == record_size_identity,
        "CVPACK dependency Record cursor identities");
    /* Only the actual successful caller-consumed prefix is a response. The
     * rest of this native allocation retains its own unwritten contents. */
    memcpy(output, item->output, item->output_size);
    *size = item->after;
    record_input_position += item->input_size;
    record_output_position += item->output_size;
    ++record_cursor;
    return item->result;
}
static int RECON_THISCALL query(WidenTi *object, WidenOutput *out,
                            const void *input, unsigned size, unsigned flags)
{
    Response *response;
    active(object, 1);
    response = &responses[cursor];
    require(version == 1 || response->kind == 2, "CVPACK dependency Query lifetime kind");
    require(out && input && !flags && size == response->input_size &&
        !memcmp(input, response->input, size), "CVPACK dependency query input");
    require(!out->bytes && !out->fixups && !out->output && !out->records,
            "CVPACK dependency query descriptor");
    if (version == 1) {
        if (!description_identity) description_identity = out;
        if (!input_identity) input_identity = input;
        require(out == description_identity && input == input_identity,
                "CVPACK dependency cross-instance input aliases");
    }
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
    if (version == 1 && response->output_capacity <= maximum_output)
        require(out->output == output_identity, "CVPACK dependency unchanged output allocation identity");
    else maximum_output = response->output_capacity;
    if (version == 1 && response->fixup_capacity <= maximum_fixups)
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
    require(enabled && cursor < response_count && !stage, "CVPACK dependency unexpected factory");
    require(recon_GetLastError() == 0, "CVPACK dependency factory LastError");
    response = &responses[cursor];
    require(out && capacity == response->factory_capacity && (unsigned)flags == response->factory_flags,
            "CVPACK dependency factory arguments");
    if (version == 1) {
        if (!factory_identity) factory_identity = out;
        require(out == factory_identity, "CVPACK dependency factory output alias");
    } else require(current_module_info && current_module_info->module == response->module,
                   "CVPACK dependency actual module sequence");
    record_cursor = record_input_position = record_output_position = 0;
    record_input_base = record_output_base = NULL;
    record_size_identity = NULL;
    opaque.vtable = &vtable;
    *out = &opaque;
    stage = 1;
    return response->factory_result;
}

void cvpack_widen_load(const char *filename, const char *input_name)
{
    FILE *file, *input;
    unsigned char magic[4], module[64];
    unsigned count, input_size, i, j;
    int saved_errno = errno;
    require(!enabled && filename && input_name, "CVPACK dependency duplicate initialization");
    file = fopen(filename, "rb");
    require(file && fread(magic, 1, 4, file) == 4 &&
        (!memcmp(magic, "RWP1", 4) || !memcmp(magic, "RWP2", 4)),
            "CVPACK dependency provider header");
    version = magic[3] - '0';
    count = version == 1 ? word(file) : 0;
    input_size = word(file);
    require(((version == 1 && count == 21 && input_size == 33280) ||
             (version == 2 && input_size == 5632)) && fread(module, 1, 64, file) == 64 &&
        !memcmp(module, "cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33", 64),
        "CVPACK dependency provider identity");
    input = fopen(input_name, "rb");
    require(input != NULL, "CVPACK dependency input file missing");
    complete_input = malloc(input_size);
    require(complete_input != NULL, "CVPACK dependency input allocation");
    for (i = 0; i < input_size; ++i) {
        int expected = fgetc(file), actual = fgetc(input);
        require(expected != EOF && actual == expected, "CVPACK dependency complete input file");
        complete_input[i] = expected;
    }
    require(fgetc(input) == EOF && !ferror(input), "CVPACK dependency input extent");
    fclose(input);
    if (version == 2) count = word(file);
    require(count == (version == 1 ? 21 : 4), "CVPACK dependency lifetime count");
    response_count = count;
    for (i = 0; i < count; ++i) {
        Response *response = &responses[i];
        if (version == 2) {
            response->module = word(file);
            response->kind = word(file);
            require(response->module == 2 + i / 2 && response->kind == 1 + i % 2,
                    "CVPACK dependency lifetime roles");
        }
        response->factory_capacity = word(file);
        response->factory_flags = word(file);
        response->factory_result = word(file);
        if (version == 2 && response->kind == 1) {
            unsigned position = response->module == 2 ? 3939 : 3519;
            unsigned end = position + (response->module == 2 ? 384 : 416);
            unsigned available = 65512;
            response->record_count = word(file);
            require(response->record_count == (response->module == 2 ? 16 : 19),
                    "CVPACK dependency actual Record count");
            response->records = calloc(response->record_count, sizeof(RecordResponse));
            require(response->records != NULL, "CVPACK dependency Record allocation");
            for (j = 0; j < response->record_count; ++j) {
                RecordResponse *item = &response->records[j];
                item->offset = word(file); item->input_size = word(file);
                item->before = word(file); item->after = word(file);
                item->result = word(file); item->output_size = word(file);
                require(item->offset == position && item->input_size >= 4 &&
                    item->input_size <= end - position && item->before == available &&
                    item->result == 1 && item->output_size >= 4 && item->output_size <= 4096 &&
                    item->output_size <= item->after && item->after <= item->before &&
                    item->input_size == 2 + complete_input[position] + (unsigned)complete_input[position + 1] * 256,
                    "CVPACK dependency Record bounds");
                item->output = block(file, item->output_size);
                require(item->output_size == 2 + item->output[0] + (unsigned)item->output[1] * 256,
                        "CVPACK dependency converted Record length");
                position += item->input_size;
                available = item->after - item->output_size;
            }
            require(position == end && response->factory_capacity == 256 &&
                !response->factory_flags && response->factory_result == 1,
                "CVPACK dependency full Record input extent");
            continue;
        }
        if (version == 2) {
            unsigned offset = word(file);
            response->input_size = word(file);
            require(offset == (response->module == 2 ? 3199 : 2844) &&
                response->input_size == (response->module == 2 ? 316 : 351) && word(file) == 1,
                "CVPACK dependency symbol input role");
            response->input = complete_input + offset;
        } else response->input_size = word(file);
        response->output_capacity = word(file);
        response->fixup_capacity = word(file);
        if (version == 2) require(word(file) == 1, "CVPACK dependency Emit result");
        response->output_size = word(file);
        response->fixup_count = word(file);
        require(response->factory_capacity == 256 && !response->factory_flags && response->factory_result == 1 &&
            response->output_capacity <= 65536 && response->fixup_capacity <= 8192 &&
            response->output_size <= response->output_capacity && response->fixup_count <= response->fixup_capacity,
            "CVPACK dependency response bounds");
        if (version == 1) response->input = block(file, response->input_size);
        response->output = block(file, response->output_size);
        response->fixups = block(file, response->fixup_count * 8);
    }
    require(fgetc(file) == EOF && !ferror(file), "CVPACK dependency trailing provider data");
    fclose(file);
    enabled = 1;
    require(atexit(finished) == 0, "CVPACK dependency completion check");
    errno = saved_errno;
}

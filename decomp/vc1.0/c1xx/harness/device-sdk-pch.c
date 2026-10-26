/* Shared fixture comparison for the frozen Device SDK PCH workflow.
   Native harness only; not compiled into the original Win32 reconstruction. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon-tool.h>
#include "device-sdk-pch.h"

static uint32_t device_word(const unsigned char *p)
{
    return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

unsigned char *device_pch_read(const char *root, const char *name)
{
    char path[4096];
    FILE *file;
    unsigned char *bytes;
    int n;
    if (!root) return NULL;
    n = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (n < 0 || (unsigned)n >= sizeof(path)) return NULL;
    file = fopen(path, "rb");
    if (!file) return NULL;
    bytes = malloc(DEVICE_PCH_BYTES);
    if (!bytes || fread(bytes, 1, DEVICE_PCH_BYTES, file) != DEVICE_PCH_BYTES ||
        fgetc(file) != EOF) {
        free(bytes);
        bytes = NULL;
    }
    fclose(file);
    return bytes;
}

/* A comparison only: raw PCH files and target state are never rewritten.
   The allocation/consumer inventory bounds every exception before comparing
   its whole pointer relation; generic output hashing checks the same remaining
   bytes, and this callback also checks them for independent narrow controls. */
int device_pch_compare(const unsigned char *actual, const unsigned char *oracle,
    size_t bytes)
{
    const uint32_t position = 0x5aba, used = 0xe3d8;
    const uint32_t originals[] = { 0x7e1000, 0x7fb640, 0x810660 };
    const uint16_t reserved[] = { 0xadf4, 0xba94, 0xc948, 0xc9b0 };
    uint32_t bases[3], capacities[] = { 0xeffc, 0x3ff4, 0x3ff4 };
    uint32_t value, expected, offset, anchor, previous_end = 0;
    unsigned counts[3] = { 0, 0, 0 }, owner, field, group, i, j;
    unsigned char *covered;
    int ok = 0;
    if (bytes != DEVICE_PCH_BYTES || memcmp(actual, "DTJPCH0", 8) ||
        device_word(actual + 48) != 1) return 0;
    anchor = device_word(actual + position + 0x95c8);
    if (anchor < 0x955c) return 0;
    bases[0] = anchor - 0x955c;
    anchor = device_word(actual + position + 0x9a44);
    if (anchor < 0x38) return 0;
    bases[1] = anchor - 0x38;
    anchor = device_word(actual + position + 0xb49c);
    if (anchor < 0xd4) return 0;
    bases[2] = anchor - 0xd4;
    if (bases[0] & 0xfff) return 0;
    for (i = 0; i < 3; ++i) {
        if (!bases[i] || bases[i] > UINT32_MAX - capacities[i]) return 0;
        for (j = 0; j < i; ++j)
            if (bases[i] < bases[j] + capacities[j] &&
                bases[j] < bases[i] + capacities[i]) return 0;
    }
    covered = calloc(DEVICE_PCH_BYTES, 1);
    if (!covered) return 0;
    for (owner = 0; owner < RECON_COUNT(device_pch_owners); ++owner) {
        const DevicePchOwner *record = &device_pch_owners[owner];
        if (record->offset < previous_end || record->offset + record->bytes > used)
            goto done;
        previous_end = record->offset + record->bytes;
        for (field = 0; field < 16; ++field) {
            if (!(record->fields & (1u << field))) continue;
            if (field * 4 + 4 > record->bytes) goto done;
            offset = position + record->offset + field * 4;
            value = device_word(actual + offset);
            expected = device_word(oracle + offset);
            for (group = 0; group < 3; ++group)
                if (expected >= originals[group] &&
                    expected - originals[group] < capacities[group]) break;
            if (group == 3 || value < bases[group] ||
                value - bases[group] != expected - originals[group]) goto done;
            ++counts[group];
            for (i = 0; i < 4; ++i) {
                if (covered[offset + i]) goto done;
                covered[offset + i] = 1;
            }
        }
    }
    for (i = 0; i < RECON_COUNT(reserved); ++i) {
        offset = position + reserved[i];
        for (j = 0; j < 8; ++j) {
            if (actual[offset + j] || covered[offset + j]) goto done;
            covered[offset + j] = 1;
        }
    }
    for (i = 0; i < DEVICE_PCH_BYTES; ++i)
        if (!covered[i] && actual[i] != oracle[i]) goto done;
    ok = counts[0] == 492 && counts[1] == 52 && counts[2] == 29;
done:
    free(covered);
    return ok;
}

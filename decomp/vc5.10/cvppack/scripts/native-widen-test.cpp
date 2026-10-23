/* Independent host exerciser for the frozen dependency contract, not target code. */
#define RECON_SHIM_IMPL 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widen.h"
#include <recon.h>
#undef main

extern "C" void cvpack_widen_load(const char *, const char *);
extern "C" int cvpack_widen_factory(WidenTi **, unsigned, int);
static unsigned word(FILE *file) { unsigned value; if (fread(&value, 4, 1, file) != 1) exit(2); return value; }
static void bytes(FILE *file, void *out, unsigned size) { if (fread(out, 1, size, file) != size) exit(2); }
static int mode(const char *actual, const char *expected) { return !strcmp(actual, expected); }
static void check(int condition) { if (!condition) exit(2); }

int main(int argc, char **argv)
{
    FILE *requests;
    char magic[4];
    unsigned i, count, input_size, output_capacity, fixup_capacity, output_size, fixup_count;
    unsigned max_output = 0, max_fixups = 0;
    unsigned char input[256], original[256], expected[256], fixup_expected[256];
    unsigned char *output = NULL;
    unsigned long *fixups = NULL;
    WidenTi *object;
    WidenOutput description;
    const char *test;
    check(argc == 4);
    test = argv[3];
    requests = fopen(argv[2], "rb");
    check(requests != NULL);
    bytes(requests, magic, 4); check(!memcmp(magic, "RWQ1", 4));
    count = word(requests); check(count == 21);
    cvpack_widen_load(argv[1], "hello.dll");
    for (i = 0; i < count; ++i) {
        input_size = word(requests); output_capacity = word(requests); fixup_capacity = word(requests);
        output_size = word(requests); fixup_count = word(requests);
        check(input_size <= 256 && output_size <= 256 && fixup_count * 8 <= 256);
        bytes(requests, input, input_size); memcpy(original, input, input_size);
        bytes(requests, expected, output_size); bytes(requests, fixup_expected, fixup_count * 8);
        recon_SetLastError(mode(test, "bad-last-error") ? 7 : 0);
        check(cvpack_widen_factory(&object, mode(test, "bad-factory-capacity") ? 257 : 256,
                                  mode(test, "bad-factory-flags") ? 1 : 0) == 1);
        memset(&description, 0, sizeof(description));
        if (mode(test, "unknown-slot")) object->Reserved1();
        if (mode(test, "unreached-record-slot")) object->Record(NULL, NULL, NULL);
        if (mode(test, "destroy-before-emit")) object->Release();
        if (mode(test, "emit-before-query")) object->Emit( &description, input, input_size, 0);
        if (mode(test, "bad-query-input")) input[0] ^= 1;
        if (mode(test, "bad-query-description")) description.bytes = 1;
        if (mode(test, "wrong-object")) {
            void *other_storage = *(void **)object;
            WidenTi *other = (WidenTi *)&other_storage;
            other->Query(&description, input, input_size, 0);
        }
        check(object->Query( &description, input, input_size,
                                   mode(test, "bad-query-flags") ? 1 : 0) == 1);
        check(description.bytes == output_capacity && description.fixups == fixup_capacity);
        if (mode(test, "query-twice")) object->Query( &description, input, input_size, 0);
        if (output_capacity > max_output) { output = (unsigned char *)realloc(output, output_capacity); max_output = output_capacity; }
        if (fixup_capacity > max_fixups) { fixups = (unsigned long *)realloc(fixups, fixup_capacity * 8); max_fixups = fixup_capacity; }
        check(output && fixups);
        memset(output, 0xa5, max_output); memset(fixups, 0xa5, max_fixups * 8);
        description.output = output; description.records = fixups;
        if (mode(test, "bad-emit-input")) input[0] ^= 1;
        if (mode(test, "bad-emit-capacity")) ++description.bytes;
        if (mode(test, "bad-output-alias")) description.output = input;
        check(object->Emit( &description, input,
                                  input_size + (mode(test, "bad-emit-size") ? 1 : 0),
                                  mode(test, "bad-emit-flags") ? 1 : 0) == 1);
        check(description.output == output && description.records == fixups &&
              description.bytes == output_size && description.fixups == fixup_count);
        check(!memcmp(output, expected, output_size) && !memcmp(fixups, fixup_expected, fixup_count * 8));
        check(!memcmp(input, original, input_size));
        { unsigned j; for (j = output_size; j < max_output; ++j) check(output[j] == 0xa5);
          for (j = fixup_count * 8; j < max_fixups * 8; ++j) check(((unsigned char *)fixups)[j] == 0xa5); }
        object->Release();
        if (mode(test, "duplicate-destroy")) object->Release();
        if (mode(test, "incomplete-sequence")) return 0;
    }
    check(fgetc(requests) == EOF); fclose(requests);
    if (mode(test, "extra-factory")) cvpack_widen_factory(&object, 256, 0);
    return 0;
}

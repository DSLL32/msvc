#ifndef CVPACK_REFERENCE_PADDING_H
#define CVPACK_REFERENCE_PADDING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon-output.h>

/* The original's S_PROCREF/S_DATAREF alignment WORD is unwritten. The
 * capture masks only these structurally selected tails; require our zeroes.
 * The canonical whole-file hash checks every other byte, including headers. */
static unsigned long reference_word(const unsigned char *p)
{
    return p[0] | (unsigned long)p[1]<<8;
}
static unsigned long reference_dword(const unsigned char *p)
{
    return reference_word(p) | reference_word(p+2)<<16;
}
static int check_reference_padding(const ReconOutputContext *context,
    const char *name, unsigned long size, unsigned long base,
    unsigned long directory_count, unsigned long reference_count)
{
    unsigned char *bytes;
    char path[4096];
    FILE *file;
    unsigned long directory, i, row, section, length, position, end, kind;
    unsigned long references = 0, sections = 0;
    int okay = 0;
    if (!context || context->struct_size != sizeof(*context) ||
        context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value ||
        !context->workspace_cwd || size < 24 || base > size-24)
        return 0;
    if (snprintf(path, sizeof(path), "%s/%s", context->workspace_cwd, name)
        >= (int)sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    bytes = malloc(size);
    if (!bytes) { fclose(file); return 0; }
    if (fread(bytes, 1, size, file) != size || fgetc(file) != EOF || ferror(file))
        goto finish;
    if (memcmp(bytes+base, "NB09", 4)) goto finish;
    directory = reference_dword(bytes+base+4);
    if (directory > size-base-16) goto finish;
    directory += base;
    if (reference_word(bytes+directory) != 16 || reference_word(bytes+directory+2) != 12 ||
        reference_dword(bytes+directory+4) != directory_count || size-directory-16 < directory_count*12)
        goto finish;
    for (i = 0; i < directory_count; ++i) {
        row = directory+16+i*12;
        kind = reference_word(bytes+row);
        if (kind != 0x129 && kind != 0x134) continue;
        section = reference_dword(bytes+row+4);
        length = reference_dword(bytes+row+8);
        if (reference_word(bytes+row+2) != 0xffff || section > size-base ||
            length < 16 || length > size-base-section) goto finish;
        section += base;
        if (reference_dword(bytes+section+4) > length-16) goto finish;
        length = reference_dword(bytes+section+4);
        if (length > size-section-16) goto finish;
        position = section+16;
        end = position+length;
        while (position < end) {
            if (end-position < 4) goto finish;
            length = reference_word(bytes+position)+2;
            kind = reference_word(bytes+position+2);
            if (length < 4 || length > end-position) goto finish;
            if (kind == 0x400 || kind == 0x401) {
                if (length != 16 || reference_word(bytes+position+14)) goto finish;
                ++references;
            }
            position += length;
        }
        ++sections;
    }
    okay = sections == 2 && references == reference_count;
finish:
    free(bytes);
    return fclose(file) == 0 && okay;
}
#endif

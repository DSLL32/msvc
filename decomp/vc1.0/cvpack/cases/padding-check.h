/* Only the two statically established sstFileIndex filename alignment tails.
 * The runner masks those original-uninitialized bytes on a comparison copy.
 * Require our deterministic zeroes and validate the derived raw PE checksum. */
#include <stdio.h>
#include <stdint.h>
#include <recon-output.h>

static uint32_t padding_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
           (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

static int check_padding(const ReconOutputContext *context, const char *name,
                         size_t size, size_t begin, size_t end, int checksum)
{
    char path[PATH_MAX];
    unsigned char *bytes;
    FILE *file;
    size_t i, pe, field;
    uint32_t sum = 0, word, stored;
    int okay = 0, closed;
    if (!context || context->struct_size != sizeof(*context) ||
        context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value ||
        !context->workspace_cwd || begin > end || end > size || size < 64)
        return 0;
    if (snprintf(path, sizeof(path), "%s/%s", context->workspace_cwd, name) >=
        (int)sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    bytes = malloc(size);
    if (!bytes) { fclose(file); return 0; }
    if (fread(bytes, 1, size, file) != size || fgetc(file) != EOF || ferror(file))
        goto finish;
    if (bytes[0] != 'M' || bytes[1] != 'Z') goto finish;
    pe = padding_u32(bytes+60);
    if (pe > size-64 || size-pe < 92 || memcmp(bytes+pe, "PE\0\0", 4))
        goto finish;
    field = pe+88;
    /* This field and the two padding intervals are fixed by these raw oracles. */
    if (field != 0xd8) goto finish;
    for (i = begin; i < end; ++i) if (bytes[i]) goto finish;
    stored = padding_u32(bytes+field);
    if (checksum) {
        for (i = 0; i < size; i += 2) {
            if (i == field || i == field+2) continue;
            word = bytes[i];
            if (i+1 < size) word |= (uint32_t)bytes[i+1]<<8;
            sum += word;
            sum = (sum & 0xffff)+(sum >> 16);
        }
        sum = (sum & 0xffff)+(sum >> 16);
        if (stored != sum+size) goto finish;
    } else if (stored) goto finish; /* No checksum requested by the mapped case. */
    okay = 1;
finish:
    free(bytes);
    closed = fclose(file);
    return okay && !closed;
}

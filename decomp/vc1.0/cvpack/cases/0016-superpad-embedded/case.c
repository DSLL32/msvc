/* Unchanged shipped MFC2.0 SUPERPAD sample, normal Z7/PCH debug build. */
#include <recon-tool.h>
#include <recon-output.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int main(int, char **, char **);
static int invoke(int argc, char **argv) { return main(argc, argv, (char **)0); }
static const uint32_t order[] = {
    0x7000,
    0x74c0,
    0x198a0,
    0x1a8e0,
    0x19a10,
    0x19cf0,
    0x19b20,
    0x19ac0,
    0x1a990,
    0x19db0,
    0x1a1f0,
    0x1a910,
    0x1a7f0,
    0x1a540,
    0x1a6a0,
    0x93a0,
    0xf060,
    0x12f20,
    0xf030,
    0xf250,
    0x1a2e0,
    0x1a400,
    0x1a350,
    0x1a430,
    0x10dd0,
    0x11540,
    0x1a5c0,
    0x1a120,
    0x113a0,
    0xfb60,
    0xff70,
    0x10010,
    0x76f0,
    0x7fd0,
    0x7e20,
    0x7d80,
    0x7b60,
    0x9e50,
    0xa270,
    0xe530,
    0xa2b0,
    0x160c0,
    0x164e0,
    0x16520,
    0xad40,
    0xaf10,
    0x172a0,
    0x9580,
    0xc1a0,
    0x9860,
    0x12f40,
    0x9770,
    0x162d0,
    0x17100,
    0x17b10,
    0xac80,
    0xc640,
    0xeac0,
    0x9ca0,
    0xc880,
    0x9c20,
    0x17c00,
    0xf0f0,
    0xcb40,
    0xf120,
    0xf1d0,
    0xf140,
    0xae40,
    0xc500,
    0xe780,
    0x152a0,
    0x14cc0,
    0x15000,
    0x155c0,
    0x15800,
    0xb500,
    0xafe0,
    0x9530,
    0x15570,
    0xbd60,
    0xd840,
    0x16280,
    0xb310,
    0x14fa0,
    0xbea0,
    0xbee0,
    0x157a0,
    0x15920,
    0x15480,
    0x154f0,
    0x15450,
    0x156b0,
    0x156f0,
    0xd790,
    0xa2e0,
    0xabf0,
    0x1a850,
    0x1a770,
    0x15980,
    0x17d80,
    0x94f0,
    0xbf10,
    0xefb0,
    0x100a0,
    0x11320,
    0x18090,
    0x10cb0,
    0x180f0,
    0x1a670,
    0x1a450,
    0xb6d0,
    0xdde0,
    0xcb90,
    0x13050,
    0xd070,
    0xd050,
    0x130e0,
    0x130f0,
    0xba10,
    0xdac0,
    0x10ec0,
    0xb240,
    0x181f0,
    0x19ba0,
    0x19c50,
    0x19ed0,
    0x19f50,

};
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase), "superpad-embedded", "Genuine shipped SUPERPAD debug executable; original Z7/PCH producer.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_EXIT, 0,
 NULL, invoke, NULL, NULL, 0
};

/* The original's S_PROCREF/S_DATAREF alignment WORD is unwritten. The
 * capture masks only these structurally selected tails; require our zeroes.
 * The canonical whole-file hash checks every other byte, including headers. */
static unsigned long word(const unsigned char *p)
{
    return p[0] | (unsigned long)p[1]<<8;
}
static unsigned long dword(const unsigned char *p)
{
    return word(p) | word(p+2)<<16;
}
static int check_reference_padding(const ReconOutputContext *context)
{
    unsigned char *bytes;
    char path[4096];
    FILE *file;
    const unsigned long size = 2558036, base = 0x139a6c;
    unsigned long directory, i, row, section, length, position, end, kind;
    unsigned long references = 0, sections = 0;
    int okay = 0;
    if (!context || context->struct_size != sizeof(*context) ||
        context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value)
        return 0;
    if (snprintf(path, sizeof(path), "%s/SUPERPAD.EXE", context->workspace_cwd)
        >= (int)sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    bytes = malloc(size);
    if (!bytes) { fclose(file); return 0; }
    if (fread(bytes, 1, size, file) != size || fgetc(file) != EOF || ferror(file))
        goto finish;
    if (memcmp(bytes+base, "NB09", 4)) goto finish;
    directory = dword(bytes+base+4);
    if (directory > size-base-16) goto finish;
    directory += base;
    if (word(bytes+directory) != 16 || word(bytes+directory+2) != 12 ||
        dword(bytes+directory+4) != 1645 || size-directory-16 < 1645*12)
        goto finish;
    for (i = 0; i < 1645; ++i) {
        row = directory+16+i*12;
        kind = word(bytes+row);
        if (kind != 0x129 && kind != 0x134) continue;
        section = dword(bytes+row+4);
        length = dword(bytes+row+8);
        if (word(bytes+row+2) != 0xffff || section > size-base ||
            length < 16 || length > size-base-section) goto finish;
        section += base;
        length = dword(bytes+section+4);
        if (length > size-section-16) goto finish;
        position = section+16;
        end = position+length;
        while (position < end) {
            if (end-position < 4) goto finish;
            length = word(bytes+position)+2;
            kind = word(bytes+position+2);
            if (length < 4 || length > end-position) goto finish;
            if (kind == 0x400 || kind == 0x401) {
                if (length != 16 || word(bytes+position+14)) goto finish;
                ++references;
            }
            position += length;
        }
        ++sections;
    }
    okay = sections == 2 && references == 3205;
finish:
    free(bytes);
    return fclose(file) == 0 && okay;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
    RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), check_reference_padding
};

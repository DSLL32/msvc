/* Versioned authored C6 workload; BARRAY control defect excluded. */
#include <limits.h>
#include <stdio.h>
#include <recon-output.h>
#include <recon-tool.h>
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
    0x10dd0,
    0x1a430,
    0x11400,
    0x112c0,
    0xfb60,
    0x10010,
    0x76f0,
    0x8090,
    0x7b60,
    0xa200,
    0xa270,
    0xe530,
    0xa2b0,
    0x7d80,
    0x7eb0,
    0x13100,
    0x13290,
    0xbea0,
    0x133e0,
    0x13360,
    0x13320,
    0x133a0,
    0x13200,
    0x131a0,
    0x131e0,
    0x13240,
    0x13380,
    0x13270,
    0x13300,
    0x134c0,
    0xbee0,
    0x13690,
    0x14250,
    0x14280,
    0x15980,
    0x159c0,
    0x11cf0,
    0x12610,
    0x17db0,
    0x17d80,
    0x172a0,
    0x9580,
    0xc1a0,
    0x9860,
    0x12f40,
    0x162d0,
    0x13a20,
    0x12c00,
    0x12c30,
    0x12c60,
    0xcb40,
    0x12cd0,
    0x12510,
    0xf170,
    0x12d60,
    0xf0a0,
    0xf0d0,
    0x11e80,
    0x17100,
    0x12e00,
    0x12e90,
    0x11dc0,
    0x12370,
    0x14670,
    0x147f0,
    0x140f0,
    0x13f20,
    0x14500,
    0x14970,
    0x13af0,
    0x17ea0,
    0x145b0,
    0x144e0,
    0xbd60,
    0x8430,
    0x8a60,
    0x9140,
    0x9100,
    0x13400,
    0x13340,
    0x133c0,
    0x13220,
    0x131c0,
    0x143b0,
    0x14730,
    0x148b0,
    0x141a0,
    0x14000,
    0x94f0,
    0xbf10,
    0xd440,
    0xd390,
    0xd350,
    0xd3f0,
    0x100a0,
    0x11320,
    0x18090,
    0x10cb0,
    0xb6d0,
    0xdde0,
    0x9530,
    0xcb90,
    0xd070,
    0xba10,
    0xdac0,
    0x10ec0,
    0xb240,
    0x130e0,
    0x130f0,
    0x181f0,
    0x1a670,
    0x1a450,
    0x19ba0,
    0x19c50,
    0x19ed0,
    0x19f50,
    0x1a850,
    0x1a120,

};
RECON_EXPORT const ReconCase case_descriptor = {
 RECON_ABI_VERSION, sizeof(ReconCase),
 "mixed-c6-no-barray", "Authored C6 type/symbol and multifile line workload; BARRAY excluded.",
 order, RECON_COUNT(order), NULL, 0, RECON_IO, RECON_EXIT, 0,
 NULL, invoke, NULL, NULL, 0
};

/* read_old_module left one byte after each counted filename unwritten.
 * capture.bin masks only these two original fields; require our zeroes. */
static const uint16_t module_padding[] = { 0x33, 0x5f
};
static int check_module_padding(const ReconOutputContext *context)
{
    char path[PATH_MAX];
    FILE *file;
    size_t i;
    int length, okay = 1;
    if (!context || context->struct_size != sizeof(*context) ||
        context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value ||
        !context->workspace_cwd) return 0;
    length = snprintf(path, sizeof(path), "%s/mixed-c6-no-barray.dbg", context->workspace_cwd);
    if (length < 0 || length >= (int)sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) || ftell(file) != 1744) okay = 0;
    for (i = 0; okay && i < RECON_COUNT(module_padding); ++i)
        if (fseek(file, module_padding[i], SEEK_SET) || fgetc(file) != 0)
            okay = 0;
    if (fclose(file)) okay = 0;
    return okay;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
    RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), check_module_padding
};

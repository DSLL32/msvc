#include <recon-tool.h>
#include <recon-output.h>
#include <stdio.h>
#include <string.h>
extern int main(int,char **,char **);
static int invoke(int argc,char **argv) {return main(argc,argv,(char **)0);}
static const uint32_t order[]={
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
 0xff70,
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
 0x13400,
 0x13340,
 0x133c0,
 0x13360,
 0x131e0,
 0x134c0,
 0xbee0,
 0x13690,
 0x14250,
 0x143b0,
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
 0x14730,
 0x148b0,
 0x13a20,
 0x11e80,
 0x83a0,
 0x13730,
 0x138a0,
 0x12d60,
 0xf0f0,
 0x17100,
 0xcb40,
 0xf120,
 0x139a0,
 0x13780,
 0x14500,
 0xbd60,
 0x8430,
 0x8a60,
 0x9140,
 0x9100,
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
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"wide-storage",
 "Format-authored two-module storage project; normal CLI, complete old-format container and record invariants.",
 order,RECON_COUNT(order),NULL,0,RECON_IO,RECON_EXIT,0,
 NULL,invoke,NULL,NULL,0
};

/* Only reserved attribute bit 0x0800 is unwritten by the original converter.
   The comparison masks its byte; require all eight native bits to be zero,
   independently preserving the seven defined bits as well as our zero rule. */
static int check_outputs(const ReconOutputContext *context)
{
    static const unsigned short bytes[] = {1409, 1441, 1473, 1505, 1537, 1569, 1601, 1633, 1665, 1697, 1729, 1761, 1793, 1825, 1857, 1889};
    unsigned char output[2152];
    char path[4096];
    unsigned i;
    FILE *file;
    int okay;
    if (!context || context->struct_size != sizeof(*context) ||
        context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value ||
        !context->workspace_cwd) return 0;
    if (snprintf(path, sizeof(path), "%s/storage-unpacked.dbg", context->workspace_cwd)
        >= (int)sizeof(path)) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    okay = fread(output, 1, sizeof(output), file) == sizeof(output) &&
           fgetc(file) == EOF && !ferror(file);
    if (fclose(file)) okay = 0;
    if (!okay || memcmp(output, "NB09", 4)) return 0;
    for (i = 0; i < RECON_COUNT(bytes); ++i)
        if (bytes[i] >= sizeof(output) || output[bytes[i]]) return 0;
    return 1;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
 RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), check_outputs
};

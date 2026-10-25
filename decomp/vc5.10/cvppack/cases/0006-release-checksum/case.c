#include <recon-tool.h>
#include <recon-output.h>
#include <stdio.h>
#include <stdlib.h>
extern int cvpack_native_setup(int,const char *);
extern int cvpack_main(int,char **,char **);
static int setup(void) { return cvpack_native_setup(0,"R:\\toolchains\\5.10\\bin\\CVPACK.EXE"); }
static int invoke(int argc,char **argv) { return cvpack_main(argc,argv,(char **)0); }
static const uint32_t order[] = {
    0x4620,
    0x7680,
    0x48c0,
    0x10a0,
    0x1f10,
    0x1010,
    0x1190,
    0x4a30,
    0x1310,
    0x1380,
    0x1470,
    0x1400,
    0x20a0,
    0x1070,
    0x2140,
    0x2480,
    0xaff0,
    0xfb90,
    0xff60,
    0xfb60,
    0x5d00,
    0x1910,
    0x2270,
    0x1980,
    0x22f0,
    0x1a80,
    0x5690,
    0x57b0,
    0x5760,
    0x5a10,
    0x5910,
    0x59a0,
    0x51e0,
    0x4c10,
    0x4b50,
    0x5130,
    0x5250,
    0xaa00,
    0x4ee0,
    0xc100,
    0xc720,
    0xc760,
    0x5070,
    0x53c0,
    0x3080,
    0x30a0,
    0x2ea0,
    0xd1f0,
    0xcf50,
    0xb1d0,
    0xa380,
    0xa730,
    0xdef0,
    0xa790,
    0xdf30,
    0xad00,
    0xae00,
    0xccb0,
    0xce70,
    0x3c20,
    0x3a90,
    0xcda0,
    0x4360,
    0xb210,
    0xe1a0,
    0xb490,
    0xff80,
    0xcc10,
    0xe640,
    0x8900,
    0xc080,
    0xe820,
    0xb890,
    0xb3c0,
    0xd430,
    0xacb0,
    0xaec0,
    0xaf80,
    0xac70,
    0xac30,
    0xabc0,
    0xdd60,
    0xf4a0,
    0xf400,
    0x5470,
    0x8650,
    0x2c20,
    0x5230,
    0xb140,
    0xdf60,
    0xf100,
    0xf0d0,
    0x69e0,
    0x7610,
    0x63f0,
    0x100e0,
    0x68a0,
    0xd600,
    0xf960,
    0xe920,
    0x10010,
    0xede0,
    0xedc0,
    0x100c0,
    0x100d0,
    0xd9a0,
    0xf5f0,
    0x6460,
    0xd130,
    0x10240,
    0x1cc0,
    0x2360,
    0x11b0,
    0x11f0,
    0x1940,
    0x2540,
    0x1680,
    0x23d0,
    0x1000,
};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="release-checksum",.description="Original /RELEASE debug DLL packing with a genuine nonzero PE checksum",
 .first_entry_order=order,.first_entry_count=RECON_COUNT(order),
 .execution=RECON_IO,.terminal=RECON_EXIT,.frontier_rva=0,.setup=setup,.io=invoke
};

/* Check the raw native checksum before masking the original's unwritten
 * file-index padding and its dependent checksum in ordinary byte comparison.
 * Every other byte remains strict. Partial frontiers precede this update. */
static int check_checksum(const ReconOutputContext *context)
{
    unsigned char *bytes;
    unsigned i;
    uint32_t sum = 0, stored;
    int result;
    FILE *file;
    if (context->terminal_kind != RECON_OUTPUT_EXIT || context->terminal_value)
        return 1;
    file = fopen("checksummed.dll", "rb");
    if (!file) return 0;
    bytes = (unsigned char *)malloc(18320);
    if (!bytes) { fclose(file); return 0; }
    result = fread(bytes, 1, 18320, file) == 18320 && fgetc(file) == EOF;
    fclose(file);
    if (!result) { free(bytes); return 0; }
    result = bytes[0] == 'M' && bytes[1] == 'Z' && bytes[60] == 176 &&
        !bytes[61] && !bytes[62] && !bytes[63] &&
        bytes[176] == 'P' && bytes[177] == 'E' && !bytes[178] && !bytes[179] &&
        bytes[18020] == 4 && !bytes[18021] && bytes[18022] == 3 && !bytes[18023] &&
        bytes[18077] == 11 && !bytes[18089] && !bytes[18090] && !bytes[18091];
    stored = (uint32_t)bytes[264] | ((uint32_t)bytes[265] << 8) |
        ((uint32_t)bytes[266] << 16) | ((uint32_t)bytes[267] << 24);
    for (i = 0; i < 18320; i += 2) {
        if (i == 264 || i == 266) continue;
        sum += bytes[i] | ((uint32_t)bytes[i + 1] << 8);
        sum = (sum & 65535) + (sum >> 16);
    }
    sum = (sum & 65535) + (sum >> 16);
    free(bytes);
    return result && stored == sum + 18320;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy = {
    RECON_OUTPUT_POLICY_VERSION, sizeof(ReconOutputPolicy), check_checksum
};

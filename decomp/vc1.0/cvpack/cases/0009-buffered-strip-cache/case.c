/* Fresh masked-original strip invocation qualified at the 187-App baseline. */
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
    0x1a430,
    0x10dd0,
    0x1a5c0,
    0x1a120,
    0x1a670,
    0x1a450,
    0x1a850,
    0x19ed0,
    0x19f50,
    0x19ba0,
    0x19c50,
};
RECON_EXPORT const ReconCase case_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconCase),
    "buffered-strip-cache", "Buffered debug extraction and input truncation.",
    order, RECON_COUNT(order), NULL, 0,
    RECON_IO, RECON_EXIT, 0,
    NULL, invoke, NULL, NULL, 0
};

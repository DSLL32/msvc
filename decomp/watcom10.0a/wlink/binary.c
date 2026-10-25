/* Original addresses are RVAs. Initial registry owns only the known pending
 * main body; unknown CRT startup is deliberately absent and remains masked. */
#include <recon-tool.h>
#include <recon.h>

static const ReconExtent main_code[] = { { 0x242e0, 0x243c8 } };
static const ReconFunction functions[] = {
    {
        .name = "main", .entry_rva = 0x242e0,
        .code = main_code, .code_count = RECON_COUNT(main_code),
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "main", .original_symbol = "main_"
    }
};
RECON_EXPORT const ReconBinary binary_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconBinary), "WLINK.EXE",
    "toolchains/watcom10.0a/.patched/WLINK.EXE",
    "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b",
    0x34656, functions, RECON_COUNT(functions), 0, 0
};

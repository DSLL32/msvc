/* Sole function registry. Extents are CF payload RVAs, not container/PE RVAs.
 * Pending App body: native scaffold only, zero application acceptance. */
#include <recon-tool.h>
#include <recon.h>

static const ReconExtent main_code[] = { { 0xcb28, 0xcb5d } };
static const ReconFunction functions[] = {
    {
        .name = "main",
        .entry_rva = 0xcb28,
        .code = main_code,
        .code_count = RECON_COUNT(main_code),
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "main",
        .original_symbol = "main_",
    },
};

RECON_EXPORT const ReconBinary binary_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconBinary),
    .name = "watcom10.0a/WASM.EXE",
    .original_path = "toolchains/watcom10.0a/.patched/binb/WASM.EXE",
    .original_sha256 = "f7189b9de3de853139d381816ab5ed9aefdd6a4fd988e29a023f1f5fbd640de9",
    .original_entry_rva = 0x1da16,
    .functions = functions,
    .function_count = RECON_COUNT(functions),
};

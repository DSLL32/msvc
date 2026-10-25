/* Sole function registry. Extents are CF payload RVAs, not container/PE RVAs.
 * One source candidate is implemented for static audit; zero fresh-pair
 * application acceptance. Six named callees remain pending traps. */
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
        .implementation = RECON_IMPLEMENTED,
        .native_symbol = "main",
        .original_symbol = "main_",
    },
    {
        .name = "main_init",
        .entry_rva = 0xcc0c,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "main_init",
        .original_symbol = "main_init_",
    },
    {
        .name = "do_init_stuff",
        .entry_rva = 0xca94,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "do_init_stuff",
        .original_symbol = "do_init_stuff_",
    },
    {
        .name = "WriteObjModule",
        .entry_rva = 0xf284,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "WriteObjModule",
        .original_symbol = "WriteObjModule_",
    },
    {
        .name = "PrintStats",
        .entry_rva = 0x164b0,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "PrintStats",
        .original_symbol = "PrintStats_",
    },
    {
        .name = "MsgFini",
        .entry_rva = 0x16844,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "MsgFini",
        .original_symbol = "MsgFini_",
    },
    {
        .name = "main_fini",
        .entry_rva = 0xcc40,
        .definition = RECON_APP,
        .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "main_fini",
        .original_symbol = "main_fini_",
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

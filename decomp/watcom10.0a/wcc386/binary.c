/* Standard registry. MemAlloc is a candidate under comparison, not accepted.
 * ABI2 registry metadata. Original addresses are canonical CF offsets/base0,
 * not PE RVAs and not the observed relocated heap addresses.
 * Admission and byte/fixup/ABI witnesses remain in the reviewed sidecar.
 */
#include <recon-tool.h>

static const ReconFunction functions[] = {
    {
        .name = "_cstart_",
        .entry_rva = 0x2fe00,
        .code = (const ReconExtent[]) { {0x2fe00, 0x2feb4} },
        .code_count = 1,
        .definition = RECON_CRT,
        .implementation = 0,
        .native_symbol = NULL,
        /* Source-correlated label; no original/rebuilt MAP exists. */
        .original_symbol = NULL,
    },
    {
        .name = "__InitRtns",
        .entry_rva = 0x48526,
        .code = (const ReconExtent[]) { {0x48526, 0x48571} },
        .code_count = 1,
        .definition = RECON_CRT,
        .implementation = 0,
        .native_symbol = NULL,
        /* Exact matched shipped inirt386 public name. */
        .original_symbol = "__InitRtns",
    },
    {
        .name = "_init_stk",
        .entry_rva = 0x482c6,
        .code = (const ReconExtent[]) { {0x482c6, 0x482ce} },
        .code_count = 1,
        .definition = RECON_CRT,
        .implementation = 0,
        .native_symbol = NULL,
        /* Local label in released stk386.asm, not an OMF public. */
        .original_symbol = NULL,
    },
    {
        .name = "__InitFiles_",
        .entry_rva = 0x4a1e5,
        .code = (const ReconExtent[]) { {0x4a1e5, 0x4a25d} },
        .code_count = 1,
        .definition = RECON_CRT,
        .implementation = 0,
        .native_symbol = NULL,
        /* Exact matched shipped initfile public name. */
        .original_symbol = "__InitFiles_",
    },
    {
        .name = "malloc_segment_wrapper",
        .entry_rva = 0x2f4cd,
        .code = (const ReconExtent[]) { {0x2f4cd, 0x2f4db} },
        .code_count = 1,
        .definition = RECON_CRT,
        .implementation = 0,
        .native_symbol = NULL,
        /* Matched initfile's _nmalloc_/malloc_ externals resolve here.
         * No public/MAP identity for this exact wrapper is available. */
        .original_symbol = NULL,
    },
    {
        /* Exact93-byte candidate body. Availability is not acceptance. */
        .name = "MemAlloc",
        .entry_rva = 0x47c77,
        .code = (const ReconExtent[]) { {0x47c77,0x47cd4} },
        .code_count = 1,
        .definition = RECON_APP,
        .implementation = RECON_IMPLEMENTED,
        .native_symbol = "MemAlloc",
        .original_symbol = "MemAlloc_",
    },
    {
        .name = "MemInit", .entry_rva = 0x47f15,
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "MemInit", .original_symbol = "MemInit_",
    },
    {
        .name = "SizeToClass", .entry_rva = 0x47b89,
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "SizeToClass", .original_symbol = "SizeToClass_",
    },
    {
        .name = "GetFromFrl", .entry_rva = 0x47bb1,
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "GetFromFrl", .original_symbol = "GetFromFrl_",
    },
    {
        .name = "MemFromSys", .entry_rva = 0x47e52,
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "MemFromSys", .original_symbol = "MemFromSys_",
    },
};

RECON_EXPORT const ReconBinary binary_descriptor = {
    .abi_version = RECON_ABI_VERSION,
    .struct_size = sizeof(ReconBinary),
    .name = "WCC386.EXE",
    .original_path = "toolchains/watcom10.0a/binb/WCC386.EXE",
    .original_sha256 = "c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
    .original_entry_rva = 0x2fe00,
    .functions = functions,
    .function_count = RECON_COUNT(functions),
    .exports = NULL,
    .export_count = 0,
};

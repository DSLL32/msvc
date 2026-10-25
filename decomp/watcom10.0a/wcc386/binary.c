/* Standard registry. Body availability is separate from fresh-pair acceptance;
 * case progress records the accepted prefix.
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
        /* Saved DS word written by startup; startup-evidence.json
         * records this cell separately from executable instructions. */
        .text_data = (const ReconExtent[]) { {0x2fdfe, 0x2fe00} },
        .text_data_count = 1,
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
        .code = (const ReconExtent[]) {{0x47f15,0x47f84},{0x47e86,0x47f15}},
        .code_count = 2,
        .definition = RECON_APP, .implementation = RECON_IMPLEMENTED,
        .native_symbol = "MemInit", .original_symbol = "MemInit_",
    },
    {
        .name = "SizeToClass", .entry_rva = 0x47b89,
        .code = (const ReconExtent[]) {{0x47b89,0x47bb1}}, .code_count = 1,
        .definition = RECON_APP, .implementation = RECON_IMPLEMENTED,
        .native_symbol = "SizeToClass", .original_symbol = "SizeToClass_",
    },
    {
        .name = "GetFromFrl", .entry_rva = 0x47bb1,
        .code = (const ReconExtent[]) {{0x47bb1,0x47c77}}, .code_count = 1,
        .definition = RECON_APP, .implementation = RECON_IMPLEMENTED,
        .native_symbol = "GetFromFrl", .original_symbol = "GetFromFrl_",
    },
    {
        .name = "MemFromSys", .entry_rva = 0x47e52,
        .definition = RECON_APP, .implementation = RECON_UNIMPLEMENTED,
        .native_symbol = "MemFromSys", .original_symbol = "MemFromSys_",
    },
    { .name="GetEnvVar", .entry_rva=0x66a2e, .definition=RECON_APP,
      .code=(const ReconExtent[]){{0x66a2e,0x66a42}}, .code_count=1,
      .implementation=RECON_IMPLEMENTED, .native_symbol="GetEnvVar", .original_symbol="GetEnvVar_" },
    { .name="myatoi", .entry_rva=0x47f84, .definition=RECON_APP,
      .implementation=RECON_UNIMPLEMENTED, .native_symbol="myatoi", .original_symbol="myatoi_" },
    { .name="myitoa", .entry_rva=0x47fa0, .definition=RECON_APP,
      .implementation=RECON_UNIMPLEMENTED, .native_symbol="myitoa", .original_symbol="myitoa_" },
    { .name="CopyStr", .entry_rva=0x3d9bb, .definition=RECON_APP,
      .implementation=RECON_UNIMPLEMENTED, .native_symbol="CopyStr", .original_symbol="CopyStr_" },
    { .name="Length", .entry_rva=0x3d9e0, .definition=RECON_APP,
      .implementation=RECON_UNIMPLEMENTED, .native_symbol="Length", .original_symbol="Length_" },
    /* Exact shipped CLIB3R members. Native calls shared getenv; no local
     * CRT bodies or runtime admission follows from these identities. */
    { .name="getenv", .entry_rva=0x2be1d,
      .code=(const ReconExtent[]){{0x2be1d,0x2be77}}, .code_count=1,
      .definition=RECON_CRT, .implementation=0,
      .native_symbol=NULL, .original_symbol="getenv_" },
    { .name="strnicmp", .entry_rva=0x328b9,
      .code=(const ReconExtent[]){{0x328b9,0x32910}}, .code_count=1,
      .definition=RECON_CRT, .implementation=0,
      .native_symbol=NULL, .original_symbol="strnicmp_" },
    /* External write alias known at this entry; exact wrapper classification
     * and native binding remain unqualified. Optional WCGMEMORY output holds. */
    { .name="write", .entry_rva=0x2f815, .definition=RECON_UNKNOWN,
      .implementation=0, .native_symbol=NULL, .original_symbol="write_" },
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

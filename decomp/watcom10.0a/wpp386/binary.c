/* Original CF image-offset registry, not PE RVAs or observed process addresses.
 * The five upstream original-only support owners have separate finite witnesses.
 * Their classification grants no application reconstruction credit.
 */
#include <recon-tool.h>

static const ReconFunction functions[] = {
    { .name="_cstart_", .entry_rva=0x664f4,
      .code=(const ReconExtent[]){{0x664f4,0x665a8}}, .code_count=1,
      .text_data=(const ReconExtent[]){{0x664f2,0x664f4},{0x665a8,0x665ac}}, .text_data_count=2,
      .definition=RECON_CRT },
    { .name="__InitRtns", .entry_rva=0x7035e,
      .code=(const ReconExtent[]){{0x7035e,0x703a9}}, .code_count=1,
      .definition=RECON_CRT, .original_symbol="__InitRtns" },
    { .name="_init_stk", .entry_rva=0x81a4c,
      .code=(const ReconExtent[]){{0x81a4c,0x81a54}}, .code_count=1,
      .definition=RECON_CRT },
    { .name="__InitFiles_", .entry_rva=0x84ffa,
      .code=(const ReconExtent[]){{0x84ffa,0x85072}}, .code_count=1,
      .definition=RECON_CRT, .original_symbol="__InitFiles_" },
    { .name="malloc_segment_wrapper", .entry_rva=0x6c6ef,
      .code=(const ReconExtent[]){{0x6c6ef,0x6c6fd}}, .code_count=1,
      .definition=RECON_CRT },
    { .name="MemAlloc", .entry_rva=0x81ba3,
      .definition=RECON_APP, .implementation=RECON_UNIMPLEMENTED,
      .native_symbol="MemAlloc", .original_symbol="MemAlloc_" },
    { .name="main", .entry_rva=0x43fef,
      .definition=RECON_APP, .implementation=RECON_UNIMPLEMENTED,
      .native_symbol="main", .original_symbol="main_" },
};

RECON_EXPORT const ReconBinary binary_descriptor = {
    .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconBinary),
    .name="WPP386.EXE",
    .original_path="toolchains/watcom10.0a/binb/WPP386.EXE",
    .original_sha256="647f4e754cd27fc76b3c44a47227ae3baa4ff2e2f253a8ec27216414653b1e6f",
    .original_entry_rva=0x664f4,
    .functions=functions, .function_count=RECON_COUNT(functions)
};

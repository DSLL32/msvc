/* Finished RC driver: two App bodies, 277 original code bytes.
 * Historical handwritten CRT support is separately classified, never App credit.
 * The unreached math helper remains CRT, without native availability credit. */
#include <recon-tool.h>

static const ReconFunction functions[] = {
    {
        .name = "_rc_main",
        .entry_rva = 0x11d2,
        .code = (const ReconExtent[]) { { 0x11d2, 0x1215 } },
        .code_count = 1,
        .definition = RECON_APP,
        .implementation = RECON_IMPLEMENTED,
        .native_symbol = "rc_main",
        .original_symbol = "_main"
    },
    {
        .name = "_rc_diagnostic@12",
        .entry_rva = 0x1100,
        .code = (const ReconExtent[]) { { 0x1100, 0x11d2 } },
        .code_count = 1,
        .text_data = (const ReconExtent[]) { { 0x10ec, 0x10ee } },
        .text_data_count = 1,
        .definition = RECON_APP,
        .implementation = RECON_IMPLEMENTED,
        .native_symbol = "rc_diagnostic"
    },
    {
        .name = "_mainCRTStartup",
        .entry_rva = 0x1220,
        .code = (const ReconExtent[]) { { 0x1220, 0x136a } },
        .code_count = 1,
        .text_data = (const ReconExtent[]) { { 0x10f0, 0x10fc } },
        .text_data_count = 1,
        .definition = RECON_CRT,
        .native_symbol = "mainCRTStartup"
    },
    {
        .name = "_set_default_precision",
        .entry_rva = 0x1380,
        .code = (const ReconExtent[]) { { 0x1380, 0x1393 } },
        .code_count = 1,
        .definition = RECON_CRT,
        .native_symbol = "set_default_precision"
    },
    {
        .name = "_math_error",
        .entry_rva = 0x13a0,
        .code = (const ReconExtent[]) { { 0x13a0, 0x13a3 } },
        .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_set_argv",
        .entry_rva = 0x13b0,
        .code = (const ReconExtent[]) { { 0x13b0, 0x13b1 } },
        .code_count = 1,
        .definition = RECON_CRT,
        .native_symbol = "set_argv"
    },
    {
        .name = "import:_XcptFilter",
        .entry_rva = 0x1370,
        .code = (const ReconExtent[]) { { 0x1370, 0x1376 } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "MSVCRT.dll",
        .import_name = "_XcptFilter"
    },
    {
        .name = "import:_initterm",
        .entry_rva = 0x1376,
        .code = (const ReconExtent[]) { { 0x1376, 0x137c } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "MSVCRT.dll",
        .import_name = "_initterm"
    },
    {
        .name = "import:_except_handler3",
        .entry_rva = 0x13b2,
        .code = (const ReconExtent[]) { { 0x13b2, 0x13b8 } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "MSVCRT.dll",
        .import_name = "_except_handler3"
    },
    {
        .name = "import:_controlfp",
        .entry_rva = 0x13b8,
        .code = (const ReconExtent[]) { { 0x13b8, 0x13be } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "MSVCRT.dll",
        .import_name = "_controlfp"
    },
    {
        .name = "import:RC",
        .entry_rva = 0x13be,
        .code = (const ReconExtent[]) { { 0x13be, 0x13c4 } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "RCDLL.dll",
        .import_name = "RC"
    },
    {
        .name = "import:Handler",
        .entry_rva = 0x13c4,
        .code = (const ReconExtent[]) { { 0x13c4, 0x13ca } },
        .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "RCDLL.dll",
        .import_name = "Handler"
    },
};
RECON_EXPORT const ReconBinary binary_descriptor = {
    .abi_version = RECON_ABI_VERSION, .struct_size = sizeof(ReconBinary),
    .name = "vc5.10/RC.EXE",
    .original_path = "toolchains/5.10/bin/RC.EXE",
    .original_sha256 = "c9f7e207bbae4be91baf6db564bd6b7f660d10287ea46302c792c3dd27b05399",
    .original_entry_rva = 0x1220,
    .functions = functions, .function_count = RECON_COUNT(functions)
};

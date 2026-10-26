/* Ordinary Linux startup only; target main and both CRT entries are unchanged. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <recon.h>

typedef int (WINAPI *DiagnosticCallback)(HWND, unsigned, char *);
extern int WINAPI rcdll_RC(HWND, unsigned, DiagnosticCallback, void *, int, char **);
extern BOOL WINAPI rcdll_Handler(DWORD);
extern BOOL WINAPI rcdll__DllMainCRTStartup(HINSTANCE, DWORD, LPVOID);
extern void mainCRTStartup(void);
extern unsigned char recon_module_rcdll_state_start[], recon_module_rcdll_state_end[];
extern const unsigned char recon_asset_rcdll_resources_start[], recon_asset_rcdll_resources_end[];

static int configure_modules(const char *program)
{
    ReconModuleSpec executable, library;
    ReconModuleExport exports[2];
    ReconModuleWritableRegion region;
    ReconStaticModuleSpec startup;
    HMODULE module;
    char *path, *base;

    base = strrchr(program, '\\');
    if (!base || !base[1]) return 0;
    executable.name = base + 1;
    executable.path = program;
    executable.sha256 = "c9f7e207bbae4be91baf6db564bd6b7f660d10287ea46302c792c3dd27b05399";
    executable.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    if (!recon_register_module(&executable)) return 0;
    path = malloc(strlen(program) + sizeof("RCDLL.DLL"));
    if (!path) return 0;
    strcpy(path, program);
    base = strrchr(path, '\\');
    if (!base) return 0;
    strcpy(base + 1, "RCDLL.DLL");
    library.name = "RCDLL.DLL";
    library.path = path;
    library.sha256 = "bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0";
    library.flags = RECON_MODULE_NO_TLS;
    module = recon_register_module(&library);
    if (!module) return 0;
    exports[0].name = "Handler";
    exports[0].ordinal = 1;
    exports[0].address = (FARPROC)rcdll_Handler;
    exports[1].name = "RC";
    exports[1].ordinal = 2;
    exports[1].address = (FARPROC)rcdll_RC;
    region.address = recon_module_rcdll_state_start;
    region.bytes = recon_module_rcdll_state_end - recon_module_rcdll_state_start;
    memset(&startup, 0, sizeof(startup));
    startup.exports = exports;
    startup.count = 2;
    startup.entry = rcdll__DllMainCRTStartup;
    startup.writable_region = &region;
    if (!recon_configure_static_module(module, &startup)) return 0;
    recon_load_module_resources_span(module, recon_asset_rcdll_resources_start,
        recon_asset_rcdll_resources_end - recon_asset_rcdll_resources_start);
    return recon_attach_static_module(module);
}

int main(int argc, char **argv, char **environment)
{
    ReconNativeEntry *host;
    host = recon_native_entry_initialize(argc, argv, environment);
    if (!host) return 125;
    /* RC and RCDLL originally import the same MSVCRT process state. */
    recon_fmode = 0;
    if (!configure_modules(host->program_path)) {
        fputs("RC: native module initialization failed\n", stderr);
        return 125;
    }
    mainCRTStartup();
    return 125;
}

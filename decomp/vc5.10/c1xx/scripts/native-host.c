/* Linux-only host; excluded from the target function graph. */
#include <stdio.h>
#include <float.h>
#include "c1xx.h"
#include <recon.h>


/* Host-only observation after the actual stdcall has returned. */
static void checkpoint(DWORD reason, BOOL status, HINSTANCE module,
                       const ReconModuleSpec *spec)
{
    const char *path = getenv("RECON_LIFECYCLE_STATE");
    FILE *file;
    unsigned first = onexit_begin ? (unsigned)*onexit_begin : 0;
    DWORD error = GetLastError();
    unsigned fp = _controlfp(0, 0);
    const char *cursor;
    if (!path) return;
    file = fopen(path, "ab");
    if (!file) exit(126);
    fprintf(file, "{\"reason\":%lu,\"status\":%d,\"module\":%u,"
                  "\"process_attached\":%d,\"raw_dll_entry\":%u,"
                  "\"dll_adjust_fdiv\":%d,\"dll_module\":%u,"
                  "\"onexit_begin\":%u,\"onexit_end\":%u,\"onexit_first\":%u,"
                  "\"initializer_words\":[%u,%u],\"last_error\":%lu,"
                  "\"floating_control\":%u,\"thread_calls_disabled\":%d,"
                  "\"registration\":{\"name\":\"%s\",\"sha256\":\"%s\",\"flags\":%lu,",
                  reason, status, (unsigned)module, process_attached, (unsigned)raw_dll_entry,
                  dll_adjust_fdiv, (unsigned)dll_module, (unsigned)onexit_begin, (unsigned)onexit_end,
                  first, (unsigned)crt_initializers[0], (unsigned)crt_initializers[1], error, fp,
                  recon_module_thread_calls_disabled(module), spec->name, spec->sha256, spec->flags);
    fputs("\"path\":\"", file);
    for (cursor = spec->path; *cursor; ++cursor) {
        if (*cursor == '\\' || *cursor == '"') fputc('\\', file);
        fputc(*cursor, file);
    }
    fputs("\"}}\n", file);
    if (fclose(file)) exit(126);
    SetLastError(error);
}

int main(void)
{
    ReconModuleSpec spec;
    HINSTANCE module;
    FARPROC abort_export, invoke_export;
    BOOL status;
    spec.name = "C1XX.DLL";
    spec.path = "R:\\toolchains\\5.10\\bin\\C1XX.DLL";
    spec.sha256 = "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9";
    spec.flags = RECON_MODULE_NO_TLS;
    module = recon_register_module(&spec);
    if (!module || recon_module_thread_calls_disabled(module)) return 126;
    /* Captured original loader context at this DLL's guarded first entry. */
    SetLastError(203);
    /* Captured caller x87 precision is 53 bits (CW 027f). */
    _controlfp(_PC_53, _MCW_PC);
    status = _DllMainCRTStartup(module, DLL_PROCESS_ATTACH, NULL);
    checkpoint(DLL_PROCESS_ATTACH, status, module, &spec);
    if (!status) return 3;
    puts("attach=1");
    /* The native adapter verifies this exact catalog against the rebuilt real
     * Win32 DLL before executing the host. These pointers are never called. */
    abort_export = (FARPROC)AbortCompilerPass;
    invoke_export = (FARPROC)InvokeCompilerPass;
    if (!abort_export || !invoke_export || abort_export == invoke_export) return 4;
    puts("exports=1");
    status = _DllMainCRTStartup(module, DLL_PROCESS_DETACH, NULL);
    checkpoint(DLL_PROCESS_DETACH, status, module, &spec);
    if (!status) return 5;
    puts("detach=1");
    return 0;
}

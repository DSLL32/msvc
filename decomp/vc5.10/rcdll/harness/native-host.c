/* Native DLL host contract, outside the reconstructed application graph. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rcdll.h"
#include "rcdll-crt.h"
#include <recon-tool.h>
#include <recon.h>

static int bind_target_environment(int count, char **vector)
{
    FILE *file;
    unsigned char raw[65536];
    char *text, **values;
    unsigned bytes, i, entries;
    const char *name = recon_host_getenv("RECON_RC_ENVIRONMENT");
    static char *empty[] = {NULL};
    if (!name) { recon_crt_bind_arguments(count, vector, empty); return 1; }
    file = fopen(name, "rb");
    if (!file) return 0;
    bytes = fread(raw, 1, sizeof(raw), file);
    if (ferror(file) || !feof(file) || bytes < 4 || (bytes & 1)) return 0;
    fclose(file);
    if (raw[bytes-1] || raw[bytes-2] || raw[bytes-3] || raw[bytes-4]) return 0;
    text = malloc(bytes/2);
    values = malloc((bytes/2+1)*sizeof(*values));
    if (!text || !values) return 0;
    for (i=0; i<bytes/2; ++i) {
        if (raw[2*i+1] || raw[2*i]>127) return 0;
        text[i] = raw[2*i];
    }
    for (entries=0, i=0; text[i];) {
        values[entries++] = text+i;
        while (text[i]) ++i;
        ++i;
    }
    if (i != bytes/2-1) return 0;
    values[entries] = NULL;
    recon_crt_bind_arguments(count, vector, values);
    return 1;
}

static int WINAPI host_diagnostic(DWORD first, DWORD second, char *message)
{
    unsigned length = strlen(message), count = 0;
    char *at, *escaped, *out;
    if (length >= 2 && ((message[length-1]=='\n' && message[length-2]=='\r') ||
        (message[length-1]=='\r' && message[length-2]=='\n')))
        message[length-2] = 0;
    if (*message) {
        if (strchr(message, '%')) {
            for (at=message; *at; ++at) if (*at=='%') ++count;
            escaped = malloc(strlen(message)+count+1);
            if (escaped) {
                out=escaped;
                for (at=message; *at; ++at) {
                    if (*at=='%') *out++='%';
                    *out++=*at;
                }
                *out=0;
                recon_printf(escaped);
            } else {
                while ((at=strchr(message,'%')) != NULL) *at=' ';
                recon_printf(message);
            }
        } else recon_printf(message);
    }
    recon_printf("\n");
    return 0;
}

RECON_EXPORT int rcdll_native_invoke(int count, char **vector)
{
    ReconModuleSpec spec;
    HINSTANCE module;
    DWORD opaque_startup = 0;
    int result;
    spec.name = "RCDLL.dll";
    spec.path = recon_host_getenv("RECON_RC_DLL_PATH");
    spec.sha256 = "bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0";
    spec.flags = RECON_MODULE_NO_TLS;
    if (!spec.path || count<1 || count>256 || !bind_target_environment(count,vector)) return 125;
    module = recon_register_module(&spec);
    if (!module || recon_module_thread_calls_disabled(module)) return 125;
    recon_load_module_resources(module, RECON_RESOURCE_FILE);
    if (!_DllMainCRTStartup(module,DLL_PROCESS_ATTACH,&opaque_startup)) return 126;
    if (!recon_module_thread_calls_disabled(module)) return 126;
    result = RC(NULL,0,host_diagnostic,NULL,count,vector);
    if (!_DllMainCRTStartup(module,DLL_PROCESS_DETACH,(LPVOID)1)) return 126;
    return result;
}


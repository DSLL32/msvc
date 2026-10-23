/* Linux-only fixture host. Target startup remains unchanged Win32 C. */
#define RECON_SHIM_IMPL
#include "c2.h"
#include <recon.h>

static char **read_environment(const char *path)
{
    FILE *file;
    char *bytes, **vector;
    long size;
    unsigned count, offset;
    if (!path) recon_unsupported("C2 fixture environment was not supplied");
    file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END)) recon_unsupported("C2 environment input");
    size = ftell(file);
    if (size < 2 || size > 65536 || fseek(file, 0, SEEK_SET))
        recon_unsupported("C2 environment input size");
    bytes = malloc(size);
    if (!bytes || fread(bytes, 1, size, file) != (unsigned long)size || fclose(file))
        recon_unsupported("C2 environment input read");
    if (bytes[size-1] || bytes[size-2]) recon_unsupported("C2 environment terminator");
    vector = malloc(4097 * sizeof(*vector));
    if (!vector) recon_unsupported("C2 environment vector allocation");
    count = offset = 0;
    while (offset < (unsigned long)size - 1 && bytes[offset]) {
        if (count == 4096) recon_unsupported("C2 environment vector too large");
        vector[count++] = bytes + offset;
        while (offset < (unsigned long)size && bytes[offset]) ++offset;
        if (++offset >= (unsigned long)size) recon_unsupported("C2 environment string");
    }
    if (offset != (unsigned long)size - 1) recon_unsupported("C2 environment trailing data");
    vector[count] = NULL;
    return vector;
}

int main(int argc, char **argv, char **host_environment)
{
    ReconModuleSpec module;
    HMODULE instance;
    char **environment;
    if (argc != 2 || strcmp(argv[1], "/?"))
        recon_unsupported("C2 host is released for the first /? case only");
    environment = read_environment(getenv("C2_ENVIRONMENT_FILE"));
    argv[0] = "R:\\toolchains\\5.10\\bin\\C2.EXE";
    recon_crt_bind_arguments(argc, argv, environment);
    module.name = "C2.EXE";
    module.path = argv[0];
    module.sha256 = "e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae";
    module.flags = RECON_MODULE_PRIMARY | RECON_MODULE_NO_TLS;
    instance = recon_register_module(&module);
    if (!instance) recon_unsupported("C2 module registration");
    recon_load_module_resources(instance, getenv("C2_RESOURCE_FILE"));
    /* Canonical MSVCRT DLL initialization precedes the target EXE startup. */
    recon_controlfp(0x10000, 0x30000);
    mainCRTStartup();
    return 0;
}

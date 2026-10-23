/* Native dependency host only; never compiled into the Win32 reconstruction. */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define RECON_SHIM_IMPL
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "../src/cl.h"
#include <recon.h>

static int dependency_fd = -1;
static unsigned char *configuration_bytes;
static size_t configuration_size;
static char response_hash[65];
static char **initial_environment;
static WCHAR *initial_os_environment;
static size_t initial_os_units;
static unsigned char profile_mbctype[257];
static unsigned short profile_ctype[257];
static unsigned invokes;
HMODULE cl_c1_module;

static int __stdcall invoke_compiler_pass(int, char **, BOOL);
static void __stdcall abort_compiler_pass(int);

static void fail(const char *message)
{
    recon_unsupported(message);
}

static void transfer(void *data, size_t length, int writing)
{
    unsigned char *bytes = data;
    while (length) {
        ssize_t count = writing ? send(dependency_fd, bytes, length, MSG_NOSIGNAL) :
                                  recv(dependency_fd, bytes, length, 0);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            fail("CL dependency transport closed or incomplete");
        bytes += count;
        length -= count;
    }
}

static void word(FILE *file, uint32_t value)
{
    if (fwrite(&value, 4, 1, file) != 1)
        fail("CL dependency request allocation");
}

static void text(FILE *file, const char *value)
{
    size_t length;
    if (!value || (length = strlen(value)) > 32767)
        fail("CL dependency unknown string input");
    word(file, (uint32_t)length);
    if (fwrite(value, 1, length, file) != length)
        fail("CL dependency request string");
}

static void vector(FILE *file, char **values)
{
    unsigned count;
    if (!values)
        fail("CL dependency unbound target CRT environment");
    for (count = 0; values[count]; ++count)
        if (count == 4096)
            fail("CL dependency vector is too large");
    word(file, (uint32_t)(uintptr_t)values);
    word(file, count);
    for (count = 0; values[count]; ++count) {
        word(file, (uint32_t)(uintptr_t)values[count]);
        text(file, values[count]);
    }
}

static void os_state(FILE *file)
{
    word(file, (uint32_t)(uintptr_t)recon_crt_os_environment);
    word(file, (uint32_t)recon_crt_os_environment_units);
    word(file, recon_crt_environment_updates);
    if (!recon_crt_os_environment || recon_crt_os_environment_units < 2 ||
        recon_crt_os_environment_units > 65536 ||
        fwrite(recon_crt_os_environment, 2, recon_crt_os_environment_units, file) != recon_crt_os_environment_units)
        fail("CL dependency actual bound OS environment observation");
}

static void reply(unsigned kind, const void *payload, size_t size, uint32_t result[5])
{
    uint32_t header[3], response[6], pid = (uint32_t)getpid();
    if (size > 2 * 1024 * 1024 - 68)
        fail("CL dependency request is too large");
    header[0] = 0x31444c43; /* CLD1 */
    header[1] = kind;
    header[2] = (uint32_t)size + 68;
    transfer(header, sizeof(header), 1);
    transfer(&pid, 4, 1);
    transfer(response_hash, 64, 1);
    transfer((void *)payload, size, 1);
    transfer(response, sizeof(response), 0);
    if (response[0] != 0x31524c43) /* CLR1 */
        fail("CL dependency unknown response framing");
    memcpy(result, response + 1, 20);
}

static void handle(FILE *file, long value)
{
    struct stat information;
    off_t position;
    if (value <= 0 || fstat((int)value - 1, &information))
        fail("CL dependency invalid stream handle");
    word(file, (uint32_t)value);
    word(file, S_ISREG(information.st_mode) ? 1 : S_ISCHR(information.st_mode) ? 2 : 0);
    word(file, (uint32_t)information.st_dev);
    word(file, (uint32_t)((uint64_t)information.st_dev >> 32));
    word(file, (uint32_t)information.st_ino);
    word(file, (uint32_t)((uint64_t)information.st_ino >> 32));
    position = lseek((int)value - 1, 0, SEEK_CUR);
    if (position < 0 && !S_ISCHR(information.st_mode))
        fail("CL dependency unobserved stream position");
    word(file, (uint32_t)position);
    word(file, (uint32_t)((uint64_t)position >> 32));
}

static void request(unsigned kind, int argc, char **argv, BOOL last, uint32_t result[5])
{
    int saved_errno = errno;
    unsigned long saved_dos = recon_doserrno;
    DWORD saved_error = recon_GetLastError();
    char *buffer = NULL, *cwd;
    size_t length = 0;
    FILE *file = open_memstream(&buffer, &length);
    unsigned i;
    CompilerPass *pass = active_pass;
    if (!file || pass != &compiler_passes[1])
        fail("CL dependency unknown active pass");
    word(file, (uint32_t)saved_errno);
    word(file, saved_dos);
    word(file, saved_error);
    word(file, recon_controlfp(0, 0)); /* Typed control flags; GDB reads raw CW independently. */
    word(file, recon_fmode);
    word(file, recon_commode);
    word(file, recon_mb_cur_max);
    word(file, recon_mb_codepage);
    word(file, recon_lc_codepage);
    for (i = 0; i < 6; ++i)
        word(file, recon_lc_handle[i]);
#ifdef RECON_CRT_LOCALE_1252_EN_US
    if (fwrite(recon_mbctype, 1, 257, file) != 257 ||
        fwrite(recon_pctype - 1, 2, 257, file) != 257)
        fail("CL dependency actual CRT table observation");
#else
    fail("CL dependency shared locale table interface is unavailable");
#endif
    word(file, interrupted);
    word(file, pass_running);
    word(file, pass->flags);
    word(file, pass == driver->current_pass);
    word(file, pass->module == cl_c1_module);
    word(file, pass->invoke == invoke_compiler_pass);
    word(file, pass->abort == abort_compiler_pass);
    word(file, recon_module_provider_state(cl_c1_module));
    word(file, recon_crt_environment == recon_crt_initenv);
    word(file, recon_crt_initenv == initial_environment);
    text(file, pass->name);
    text(file, pass->environment);
    word(file, argc);
    word(file, last);
    if (argv)
        vector(file, argv);
    else {
        word(file, 0);
        word(file, 0);
    }
    vector(file, recon_crt_environment);
    vector(file, initial_environment);
    os_state(file);
    if (fwrite(recon_iob, 32, 3, file) != 3)
        fail("CL dependency FILE observation");
    for (i = 0; i < 3; ++i) {
        ShimFILE *stream = &recon_iob[i];
        if (stream->_cnt || stream->_ptr != stream->_base || stream->_tmpfname)
            fail("CL dependency unobserved live FILE buffer");
        word(file, recon_osfile[i]);
        handle(file, recon_get_osfhandle(i));
        handle(file, (long)(intptr_t)recon_GetStdHandle(STD_INPUT_HANDLE - i));
    }
    {
        unsigned count = 0;
        if (recon_nhandle > 256)
            fail("CL dependency unknown descriptor-table bound");
        for (i = 0; i < recon_nhandle; ++i)
            if (recon_osfile[i] & 1) ++count;
        word(file, count);
        for (i = 0; i < recon_nhandle; ++i)
            if (recon_osfile[i] & 1) word(file, i);
    }
    cwd = getcwd(NULL, 0);
    if (!cwd)
        fail("CL dependency native working directory");
    text(file, cwd);
    free(cwd);
    if (fclose(file) || !buffer)
        fail("CL dependency request encoding");
    reply(kind, buffer, length, result);
    free(buffer);
    errno = saved_errno;
    recon_doserrno = saved_dos;
    recon_SetLastError(saved_error);
}

static void __cdecl lifecycle(HMODULE module, unsigned event, void *context)
{
    uint32_t result[5];
    if (module != cl_c1_module || context != &cl_c1_module ||
        (event != RECON_MODULE_EVENT_LOAD && event != RECON_MODULE_EVENT_FREE))
        fail("CL dependency unknown lifecycle event");
    request(event == RECON_MODULE_EVENT_LOAD ? 1 : 3, 0, NULL, 0, result);
    if (result[0] || result[1] || result[2] || result[3] || result[4])
        fail("CL dependency lifecycle state mutation");
}

static int __stdcall invoke_compiler_pass(int argc, char **argv, BOOL last)
{
    uint32_t result[5];
    if (recon_module_provider_state(cl_c1_module) != RECON_MODULE_PROVIDER_LOADED || invokes++)
        fail("CL dependency repeated or stale Invoke callback");
    if (argc != 26 || !argv || argv[argc] || last != TRUE ||
        errno || recon_doserrno != 2 || recon_GetLastError() != 2)
        fail("CL dependency unobserved Invoke ABI/error inputs");
    request(2, argc, argv, last, result);
    if (result[0] || result[1] != 1 || result[2] || result[3] != 2 || result[4])
        fail("CL dependency unobserved Invoke response");
#ifdef RECON_CRT_LOCALE_1252_EN_US
    {
        ReconCrtLocaleProfile profile;
        profile.provider_sha256 = "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
        profile.kind = RECON_CRT_LOCALE_1252_EN_US;
        profile.mbctype = profile_mbctype;
        profile.mbctype_count = 257;
        profile.ctype = profile_ctype;
        profile.ctype_count = 257;
        recon_crt_apply_locale_profile(&profile);
    }
#else
    fail("CL dependency shared locale state application is unavailable");
#endif
    /* This is the actual external C1 call's measured LastError2->0 effect.
     * Target globals, pass cache, argv and FILE state are never reseeded. */
    recon_SetLastError(0);
    return 0;
}

static void __stdcall abort_compiler_pass(int status)
{
    if (recon_module_provider_state(cl_c1_module) != RECON_MODULE_PROVIDER_LOADED)
        fail("CL dependency stale Abort callback");
    (void)status;
    fail("CL C1 Abort has no observed successful response");
}

static unsigned char *cursor, *limit;

static void *take(size_t count)
{
    unsigned char *value = cursor;
    if (count > (size_t)(limit - cursor))
        fail("CL dependency truncated configuration");
    cursor += count;
    return value;
}

static uint32_t number(void)
{
    uint32_t value;
    memcpy(&value, take(4), 4);
    return value;
}

static char *configuration_text(void)
{
    uint32_t size = number();
    char *value;
    if (size > 32767)
        fail("CL dependency configuration string is too large");
    value = malloc(size + 1);
    if (!value)
        fail("CL dependency configuration allocation");
    memcpy(value, take(size), size);
    if (memchr(value, 0, size))
        fail("CL dependency configuration embedded NUL");
    value[size] = 0;
    return value;
}

static void __attribute__((constructor)) configure_dependency(void)
{
    const char *descriptor = getenv("CL_DEPENDENCY_FD");
    const char *path = getenv("CL_DEPENDENCY_CONFIG");
    char *end, *module_path;
    long fd;
    FILE *file;
    long size;
    uint32_t result[5], count, i;
    ReconModuleSpec module;
    ReconModuleExportsSpec provider;
    ReconModuleExport exports[2];
    if (!descriptor && !path)
        return;
    if (!descriptor || !path)
        fail("CL dependency incomplete host configuration");
    errno = 0;
    fd = strtol(descriptor, &end, 10);
    if (!*descriptor || *end || errno || fd < 0 || fd > 0x7fffffffL)
        fail("CL dependency invalid socket descriptor");
    dependency_fd = (int)fd;
    file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 ||
        size > 2 * 1024 * 1024 || fseek(file, 0, SEEK_SET))
        fail("CL dependency cannot read host configuration");
    configuration_size = (size_t)size;
    configuration_bytes = malloc(configuration_size);
    if (!configuration_bytes || fread(configuration_bytes, 1, configuration_size, file) != configuration_size || fclose(file))
        fail("CL dependency incomplete host configuration bytes");
    cursor = configuration_bytes;
    limit = cursor + configuration_size;
    if (memcmp(take(4), "RCH1", 4) || number() != 1)
        fail("CL dependency unknown configuration format");
    memcpy(response_hash, take(64), 64);
    for (i = 0; i < 64; ++i)
        if (!strchr("0123456789abcdef", response_hash[i]))
            fail("CL dependency invalid response identity");
    /* The controller verifies the exact bytes actually read, not merely a
     * parallel expected hash. No target entry executes before this handshake. */
    reply(0, configuration_bytes, configuration_size, result);
    if (result[0] || result[1] || result[2] || result[3] || result[4])
        fail("CL dependency configuration was not approved");
    module_path = configuration_text();
    count = number();
    if (count > 4096)
        fail("CL dependency initial environment is too large");
    initial_environment = calloc(count + 1, sizeof(char *));
    if (!initial_environment)
        fail("CL dependency initial environment allocation");
    for (i = 0; i < count; ++i)
        initial_environment[i] = configuration_text();
    initial_os_units = number();
    if (initial_os_units < 2 || initial_os_units > 65536)
        fail("CL dependency OS environment size");
    initial_os_environment = malloc(initial_os_units * sizeof(WCHAR));
    if (!initial_os_environment)
        fail("CL dependency OS environment allocation");
    memcpy(initial_os_environment, take(initial_os_units * sizeof(WCHAR)), initial_os_units * sizeof(WCHAR));
    memcpy(profile_mbctype, take(sizeof(profile_mbctype)), sizeof(profile_mbctype));
    memcpy(profile_ctype, take(sizeof(profile_ctype)), sizeof(profile_ctype));
    if (cursor != limit)
        fail("CL dependency unused host configuration bytes");
    module.name = "C1.DLL";
    module.path = module_path;
    module.sha256 = "7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d";
    module.flags = RECON_MODULE_NO_TLS;
    cl_c1_module = recon_register_module(&module);
    exports[0].name = "_InvokeCompilerPass@12";
    exports[0].ordinal = 2;
    exports[0].address = (FARPROC)invoke_compiler_pass;
    exports[1].name = "_AbortCompilerPass@4";
    exports[1].ordinal = 1;
    exports[1].address = (FARPROC)abort_compiler_pass;
    provider.sha256 = module.sha256;
    provider.exports = exports;
    provider.count = 2;
    provider.lifecycle = lifecycle;
    provider.context = &cl_c1_module;
    recon_register_module_exports(cl_c1_module, &provider);
}

char **cl_dependency_bind_entry(int argc, char **argv, char **envp)
{
    if (dependency_fd < 0)
        return envp;
    recon_crt_bind_arguments(argc, argv, initial_environment);
    *recon_p_initenv() = initial_environment;
    recon_crt_bind_os_environment(initial_os_environment, initial_os_units,
        "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210");
    return initial_environment;
}

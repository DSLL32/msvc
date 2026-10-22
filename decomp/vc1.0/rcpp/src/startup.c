/* RCPP.EXE startup, original [01008d80,01008ed6). */
#include "rcpp.h"
char *rc_command_line, *rc_environment;
unsigned rc_os_build, rc_os_version, rc_os_major, rc_os_minor;
unsigned rc_base_build, rc_base_major, rc_base_minor;
unsigned rc_base_version, rc_major_version, rc_minor_version;
int rc_argc;
char **rc_argv, **rc_envp;

void rcpp_start(void)
{
    DWORD version;
    unsigned caught_code;
    rc_command_line = GetCommandLineA();
    rc_environment = GetEnvironmentStrings();
    version = GetVersion();
    rc_os_minor = (version >> 8) & 255;
    rc_os_major = version & 255;
    rc_os_version = (rc_os_major << 8) + rc_os_minor;
    rc_os_build = version >> 16;
    rc_base_build = rc_os_build;
    rc_base_minor = rc_os_build & 255;
    rc_base_major = (rc_os_build >> 8) & 255;
    rc_base_version = version & 65535;
    rc_major_version = rc_base_version & 255;
    rc_minor_version = (rc_base_version >> 8) & 255;
    rc_heap_initialize();
    _ioinit();
    rc_setargv();
    rc_setenvp();
    rc_initialize_c();
    __try {
        rc_exit(rc_main(rc_argc, rc_argv, rc_envp));
    } __except (caught_code = GetExceptionCode(),
                rc_exception_filter(caught_code, GetExceptionInformation())) {
        rc_fast_exit(caught_code);
    }
}

/* C/CRT initialization and termination reuse the identified CRT0DAT.OBJ.
 * Its private _initterm is mapped as the same logical CRT function. */

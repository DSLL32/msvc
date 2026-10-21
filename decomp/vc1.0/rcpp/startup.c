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
        rc_cexit(caught_code);
    }
}

RcInitializer rc_fp_initialize;
/* Four null slots occupy each original constructor range. RCPP is a C
 * program with no linked static C++ constructors or floating-point runtime. */
static RcInitializer cpp_initializers[4], c_initializers[4];
/* Original [01007ac0,01007af0). */
void rc_initialize_c(void)
{
    if (rc_fp_initialize) rc_fp_initialize();
    rc_initterm(cpp_initializers, cpp_initializers + 4);
    rc_initterm(c_initializers, c_initializers + 4);
}

/* Original [01007bd0,01007bf2). The original uses unsigned address ordering. */
void rc_initterm(RcInitializer *first, RcInitializer *last)
{
    while ((unsigned)first < (unsigned)last) {
        if (*first) (**first)();
        ++first;
    }
}

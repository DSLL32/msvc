/* Embedded CRT support is reconstructed separately from the application. */
#include "rc.h"
#include <recon.h>

void (__cdecl *crt_xc[2])(void);
void (__cdecl *crt_xi[2])(void);
int crt_wildcard;
int crt_newmode;
int crt_default_math_error = 1;
int crt_commit_mode;
int crt_file_mode;
void *crt_onexit_begin, *crt_onexit_end;
int crt_fdiv_adjust;

void mainCRTStartup(void)
{
    int argc;
    char **argv, **environment;
    struct _startupinfo information;
    unsigned long fault_code;

    __try {
        __set_app_type(1);
        crt_onexit_begin = (void *)-1;
        crt_onexit_end = crt_onexit_begin;
        *__p__fmode() = crt_file_mode;
        *__p__commode() = crt_commit_mode;
        crt_fdiv_adjust = _adjust_fdiv;
        set_argv();
        if (!crt_default_math_error)
            __setusermatherr(math_error);
        set_default_precision();
        _initterm(crt_xi, crt_xi + 1);
        information.newmode = crt_newmode;
        __getmainargs(&argc, &argv, &environment, crt_wildcard, &information);
        _initterm(crt_xc, crt_xc + 1);
        *__p___initenv() = environment;
        exit(rc_main(argc, argv, environment));
    } __except(_XcptFilter(fault_code = GetExceptionCode(), GetExceptionInformation())) {
        _exit(fault_code);
    }
}

void set_argv(void)
{
}

void set_default_precision(void)
{
    _controlfp(0x10000, 0x30000);
}

int math_error(struct _exception *exception)
{
    __asm { int 3 }
    return 0;
}

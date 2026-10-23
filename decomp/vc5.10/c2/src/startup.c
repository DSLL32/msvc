/* Embedded MSVCRT startup; kept separate from application coverage. */
#include "c2.h"
#include <recon.h>

C2Initializer *c2_onexit_begin;
C2Initializer *c2_onexit_end;
int c2_fmode = _O_BINARY;
int c2_commode;
int c2_adjust_fdiv;
int c2_default_math_error = 1;
int c2_new_mode;
int c2_wildcards;
C2Initializer c2_c_initializers[1];
C2Initializer c2_cpp_initializers[1];

void c2_crt_startup(void)
{
    int argc, result, fault_code;
    struct _startupinfo info;
    char **argv, **environment;

    __try {
        __set_app_type(1);
        c2_onexit_end = (C2Initializer *)-1;
        c2_onexit_begin = (C2Initializer *)-1;
        *__p__fmode() = c2_fmode;
        *__p__commode() = c2_commode;
        c2_adjust_fdiv = _adjust_fdiv;
        c2_set_argv();
        if (!c2_default_math_error)
            __setusermatherr(c2_math_error);
        c2_set_default_precision();
        _initterm(c2_c_initializers, c2_c_initializers + 1);
        info.newmode = c2_new_mode;
        __getmainargs(&argc, &argv, &environment, c2_wildcards, &info);
        _initterm(c2_cpp_initializers, c2_cpp_initializers + 1);
        *__p___initenv() = environment;
        result = c2_main(argc, argv, environment);
        exit(result);
    }
    __except (_XcptFilter(fault_code = GetExceptionCode(), GetExceptionInformation())) {
        _exit(fault_code);
    }
}

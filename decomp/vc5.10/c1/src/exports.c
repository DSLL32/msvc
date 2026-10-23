#include "c1.h"
#include <recon.h>

/* Original exported pass wrapper, RVA0x12229; PGO cold block0x30359. */
int c1_pass_mode;
int WINAPI InvokeCompilerPass(int argc, char **argv, int mode)
{
    int result;
    EXCEPTION_POINTERS *exception;

    result = initialize_compiler_pass();
    if (result)
        return result;
    c1_pass_mode = mode;
    __try {
        result = main(argc, argv);
    } __except ((exception = GetExceptionInformation())->ExceptionRecord->ExceptionCode
                == 0xe0005031UL) {
        result = (int)exception->ExceptionRecord->ExceptionInformation[0];
    }
    if (cleanup_compiler_pass())
        return 1;
    return result;
}

void WINAPI AbortCompilerPass(int reason)
{
    __asm { int 3 }
}

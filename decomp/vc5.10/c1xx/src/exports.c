#include "c1xx.h"
#include <recon.h>

int main(int argc, char **argv);

void WINAPI AbortCompilerPass(DWORD reason)
{
    __asm { int 3 }
}

int compiler_mode;

int WINAPI InvokeCompilerPass(int argc, char **argv, int mode)
{
    EXCEPTION_POINTERS *exception;
    int result = initialize_compiler_pass();
    if (result)
        return result;
    compiler_mode = mode;
    __try {
        result = main(argc, argv);
    } __except ((exception = GetExceptionInformation())->ExceptionRecord->ExceptionCode == 0xe0005031) {
        result = (int)exception->ExceptionRecord->ExceptionInformation[0];
    }
    if (cleanup_compiler_pass())
        return 1;
    return result;
}

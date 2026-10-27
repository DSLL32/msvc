#include <windows.h>
#include <objbase.h>
#include <recon.h>
typedef void (WINAPI *Uninitialize)(void);
typedef char Signature[__builtin_types_compatible_p(__typeof__(&CoUninitialize),Uninitialize)?1:-1];
int main(void)
{
    Uninitialize typed=CoUninitialize;
    typed();return 42;
}

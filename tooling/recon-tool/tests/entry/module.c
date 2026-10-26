#include <win32.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int shared_calls,shared_ctor_calls;
extern void entry_event(int,unsigned);
extern void WINAPI recon_SetLastError(DWORD);
int counter=17;
int *pointer=&counter;
unsigned char state[64] __attribute__((aligned(64)));
int constructor_count;
#ifdef __cplusplus
int common_counter;
#else
int common_counter __attribute__((common));
#endif
int internal(int n) { return n+1; }
int internal_alternative(int n) { return n+1; }
extern int MethodAlias(int);
extern int DataAlias;
int main(void) { return 999; }
int __attribute__((stdcall)) Export(int n,int (__attribute__((stdcall)) *callback)(int))
{
    ++shared_calls;
    counter+=callback(n);
    state[4]=91;
    recon_SetLastError((DWORD)*pointer);
    return *pointer;
}
int ReadCounter(void) { return pointer==&counter && MethodAlias(7)==8?DataAlias:-1000; }
int ReadConstructor(void) { return constructor_count; }
BOOL WINAPI DllEntry(HINSTANCE h,DWORD why,LPVOID reserved)
{
    (void)reserved;
    if (!h || pointer!=&counter) return FALSE;
    ++shared_calls;
    entry_event(MODULE_TAG,why);
    if (why==DLL_PROCESS_ATTACH) {
        if (!constructor_count && (counter!=17 || state[4] || common_counter)) return FALSE;
        ++counter;
        common_counter=1;
    } else {
        counter=-99;
        state[4]=77;
        common_counter=99;
    }
    return TRUE;
}

#ifdef __cplusplus
}
#endif

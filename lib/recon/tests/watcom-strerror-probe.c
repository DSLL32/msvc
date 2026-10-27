#ifdef TEST_SHIM
#define RECON_SHIM_IMPL
#endif
#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
#if !defined(TEST_SHIM) && !defined(TEST_UNSELECTED) && defined(RECON_CRT_PROFILE_WATCOM_100A)
    char *first,*again;int row;unsigned short cw,sw,after_cw,after_sw;
    int *host_errno=__errno_location();
#endif
#if defined(TEST_SHIM) || defined(TEST_UNSELECTED) || !defined(RECON_CRT_PROFILE_WATCOM_100A)
#ifdef strerror
#error The host/default strerror spelling must remain unmapped.
#endif
    (void)argc;(void)argv;assert(strerror!=recon_watcom_strerror);
#if !defined(TEST_SHIM) && !defined(TEST_UNSELECTED)
    recon_watcom_strerror(1);return 42;
#else
    return 0;
#endif
#else
    char *(__cdecl *typed)(int)=strerror;
    if(argc>1) {typed(atoi(argv[1]));return 42;}
    for(row=0;row<2;++row) {
        errno=row?111:0;*host_errno=row?EDOM:0;recon_doserrno=222;recon_SetLastError(333);
        __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        first=typed(1);again=strerror(1);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
        assert(first==again && !strcmp(first,"No such file or directory"));
        assert(errno==(row?111:0) && *host_errno==(row?EDOM:0) && recon_doserrno==222 && recon_GetLastError()==333);
        assert(cw==after_cw && sw==after_sw);
        __asm__ __volatile__("fstp %%st(0)":::"st");
    }
    return 0;
#endif
}

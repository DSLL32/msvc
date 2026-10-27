#ifdef TEST_SHIM
#define RECON_SHIM_IMPL
#endif
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <recon.h>
int main(int argc,char **argv)
{
    time_t clock=0;
#if !defined(RECON_CRT_PROFILE_MSVCRT_510) || defined(TEST_SHIM)
#ifdef ctime
#error Other CRT profiles and shim implementations must retain host ctime.
#endif
    (void)argc;(void)argv;
    assert(ctime!=recon_ctime);
    assert(setenv("TZ","UTC0",1)==0);tzset();
    assert(!strcmp(ctime(&clock),"Thu Jan  1 00:00:00 1970\n"));
#ifndef TEST_SHIM
    recon_ctime(&clock);return 42;
#else
    return 0;
#endif
#else
    static const struct {time_t stamp;const char *text;} rows[]={
        {0,"Thu Jan 01 00:00:00 1970\n"},
        {691200,"Fri Jan 09 00:00:00 1970\n"},
        {777600,"Sat Jan 10 00:00:00 1970\n"},
        {2592000,"Sat Jan 31 00:00:00 1970\n"},
        {951782400,"Tue Feb 29 00:00:00 2000\n"},
        {2147483647,"Tue Jan 19 03:14:07 2038\n"}};
    char *(__cdecl *typed)(const time_t *)=ctime;char *first=NULL,*text,saved[26];
    unsigned row,seed;unsigned short cw,sw,after_cw,after_sw;
    int *host_errno=__errno_location();void *page;
    assert(sizeof(time_t)==4);
    if(argc>1) {
        if(!strcmp(argv[1],"null"))typed(NULL);
        else typed((const time_t *)(uintptr_t)0xfffffffeUL);
        return 42;
    }
    assert(setenv("TZ","UTC0",1)==0);tzset();
    for(seed=0;seed<2;++seed)for(row=0;row<sizeof(rows)/sizeof(rows[0]);++row) {
        clock=rows[row].stamp;
        errno=seed?111:0;*host_errno=seed?EDOM:0;
        recon_doserrno=222;recon_SetLastError(333);
        __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
        text=typed(&clock);
        __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
        assert(text && !memcmp(text,rows[row].text,26));
        assert(strlen(text)==25 && text[24]=='\n' && !text[25]);
        if(first)assert(text==first);else first=text;
        assert(clock==rows[row].stamp);
        assert(errno==(seed?EDOM:0) && *host_errno==(seed?EDOM:0));
        assert(recon_doserrno==222 && recon_GetLastError()==333);
        assert(cw==after_cw && sw==after_sw);
        __asm__ __volatile__("fstp %%st(0)":::"st");
    }
    memcpy(saved,first,26);clock=-1;
    assert(!typed(&clock) && !memcmp(first,saved,26));
    assert(errno==EDOM && recon_doserrno==222 && recon_GetLastError()==333);
    /* Real read-only input proves the borrowed timestamp is only read. */
    page=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(page!=MAP_FAILED);*(time_t *)page=0;assert(!mprotect(page,4096,PROT_READ));
    assert(!memcmp(typed((const time_t *)page),rows[0].text,26));assert(!munmap(page,4096));
    assert(setenv("TZ","EST5",1)==0);tzset();clock=0;
    assert(!strcmp(typed(&clock),"Wed Dec 31 19:00:00 1969\n"));
    return 0;
#endif
}

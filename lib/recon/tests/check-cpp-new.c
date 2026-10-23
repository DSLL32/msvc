#include <assert.h>
#include <sys/resource.h>
#include <recon.h>
int main(int argc,char **argv)
{
    size_t sizes[]={12288,4000,4096,0}; unsigned n; void *p;
    struct rlimit limit; (void)argv;
    if (argc>1) {
        limit.rlim_cur=limit.rlim_max=32*1024*1024; assert(setrlimit(RLIMIT_AS,&limit)==0);
        crt_operator_new(256*1024*1024); return 1;
    }
    errno=EDOM; SetLastError(1234);
    for (n=0;n<sizeof(sizes)/sizeof(sizes[0]);++n) {
        p=crt_operator_new(sizes[n]); assert(p);
        assert(errno==EDOM && GetLastError()==1234);
        memset(p,0xa5,sizes[n]?sizes[n]:1); free(p);
    }
    assert(crt_operator_new(0xffffffffUL)==NULL);
    assert(errno==EDOM && GetLastError()==1234);
    return 0;
}

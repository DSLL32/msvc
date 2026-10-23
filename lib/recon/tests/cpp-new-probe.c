#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <recon.h>
extern void * __cdecl crt_operator_new(unsigned);
int main(void)
{
    unsigned sizes[]={12288,4000,4096,0,0xffffffffU}; unsigned i; void *p;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
        errno=123; SetLastError(456); p=crt_operator_new(sizes[i]);
        printf("%u %u %d %lu\n",sizes[i],p!=NULL,errno,GetLastError());
        if(p) free(p);
    }
    return 0;
}

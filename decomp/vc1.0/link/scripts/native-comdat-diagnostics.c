/* Explicit external caller; never instrumented or part of the target graph. */
#define RECON_SHIM_IMPL
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "../src/comdat-diagnostics.h"
#include <recon.h>
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
unsigned char *comdat_test_region;
int *comdat_test_errno;
int __wrap_main(int argc,char **argv)
{
    unsigned char original[4096],extra;
    unsigned long n=0;
    ssize_t got;
    int fd,result;
    unsigned char *occupied=NULL;
    if(argc!=2 && !(argc==3 && !strcmp(argv[2],"--occupied")))return 120;
    fd=open(argv[1],O_RDONLY);if(fd<0)return 121;
    while(n<sizeof(original)){
        got=read(fd,original+n,sizeof(original)-n);if(got<=0)return 121;n+=got;
    }
    if(read(fd,&extra,1)!=0 || close(fd))return 121;
    if(argc==3){
        occupied=mmap((void *)0x12000000,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
        if(occupied!=(void *)0x12000000)return 125;
        memset(occupied,0x5a,4096);
    }
    comdat_test_region=mmap((void *)0x12000000,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(occupied){
        if(comdat_test_region!=MAP_FAILED || errno!=EEXIST)return 125;
        for(n=0;n<4096;++n)if(occupied[n]!=0x5a)return 125;
        if(munmap(occupied,4096))return 125;
        return 122; /* Explicit unavailable-owned-address refusal; target not called. */
    }
    if(comdat_test_region!=(void *)0x12000000)return 122;
    memcpy(comdat_test_region,original,4096);
    comdat_test_errno=&errno;
    result=dump_comdat_graph((ComdatNode *)(comdat_test_region+0x40));
    if(result || memcmp(comdat_test_region,original,4096))return 123;
    if(munmap(comdat_test_region,4096))return 124;
    return 0;
}

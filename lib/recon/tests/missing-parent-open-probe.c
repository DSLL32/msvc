#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <recon.h>
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
static void api(const char *path,DWORD disposition,DWORD expected)
{
    unsigned char before[108],after[108];HANDLE handle;
    errno=111;recon_doserrno=222;recon_SetLastError(1234);
    fp(before);handle=recon_CreateFileA(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,disposition,FILE_ATTRIBUTE_NORMAL,NULL);fp(after);
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==expected);
    if(expected) assert(handle==INVALID_HANDLE_VALUE);
    else {assert(handle!=INVALID_HANDLE_VALUE);assert(recon_CloseHandle(handle));}
}
static void crt(const char *path,const char *mode,unsigned long expected)
{
    unsigned char before[108],after[108],osfile[256];ShimFILE streams[20];ShimFILE *result;
    memcpy(streams,recon_iob,sizeof(streams));memcpy(osfile,recon_osfile,sizeof(osfile));
    errno=0;recon_doserrno=0;recon_SetLastError(0);
    fp(before);result=recon_fopen(path,mode);fp(after);
    assert(!result && errno==ENOENT && recon_doserrno==expected && recon_GetLastError()==expected);
    assert(!memcmp(before,after,108) && !memcmp(streams,recon_iob,sizeof(streams)) && !memcmp(osfile,recon_osfile,sizeof(osfile)));
}
int main(int argc,char **argv)
{
    int fd;struct stat st;unsigned n;
    assert(!mkdir("parent",0700));fd=open("regular",O_CREAT|O_EXCL|O_RDWR,0600);assert(fd>=0);close(fd);
    if(argc>1) __asm__ __volatile__("fld1\n\tfldpi":::"st");
    api("parent/new",CREATE_ALWAYS,0);assert(!stat("parent/new",&st) && !st.st_size);
    api("parent/new",OPEN_EXISTING,0);
    api("parent/absent",OPEN_EXISTING,ERROR_FILE_NOT_FOUND);
    api("absent/child",CREATE_ALWAYS,ERROR_PATH_NOT_FOUND);
    api("regular/child",CREATE_ALWAYS,ERROR_PATH_NOT_FOUND);
    api("parent/new",999,ERROR_INVALID_PARAMETER);
    crt("absent/child","w",ERROR_PATH_NOT_FOUND);
    crt("parent/absent","r",ERROR_FILE_NOT_FOUND);
    crt("regular/child","w",ERROR_PATH_NOT_FOUND);
    for(n=0;n<3;++n) {
        const char *path=n==0?"absent":n==1?"parent/absent":"regular/child";
        assert(stat(path,&st)<0);
    }
    if(argc>1) __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    puts("missing-parent/leaf/nondirectory/success/API-CRT errors/full108FP/FILE state PASS");return 0;
}

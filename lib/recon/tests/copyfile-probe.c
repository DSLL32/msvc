#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int mode,reads,writes,closes,times_calls;
ssize_t __real_read(int,void *,size_t);
ssize_t __real_write(int,const void *,size_t);
int __real_close(int);
int __real_futimens(int,const struct timespec *);
ssize_t __wrap_read(int fd,void *data,size_t bytes)
{
    if(mode && ++reads==1) {
        if(mode==1) {errno=EINTR;return -1;}
        if(mode==2) {errno=EIO;return -1;}
    }
    return __real_read(fd,data,bytes);
}
ssize_t __wrap_write(int fd,const void *data,size_t bytes)
{
    if(mode) {
        ++writes;
        if(mode==1 && writes==1) {errno=EINTR;return -1;}
        if(mode==1 && bytes>1)bytes/=2;
        if(mode==3) {errno=ENOSPC;return -1;}
        if(mode==4)return 0;
    }
    return __real_write(fd,data,bytes);
}
int __wrap_close(int fd)
{
    int result=__real_close(fd);
    if(mode==6 && ++closes==1) {errno=EIO;return -1;}
    return result;
}
int __wrap_futimens(int fd,const struct timespec *times)
{
    ++times_calls;
    if(mode==5) {errno=EACCES;return -1;}
    return __real_futimens(fd,times);
}
static void file(const char *name,const void *data,size_t bytes)
{
    int fd=open(name,O_WRONLY|O_CREAT|O_TRUNC,0600);assert(fd>=0);
    assert(__real_write(fd,data,bytes)==(ssize_t)bytes);assert(!__real_close(fd));
}
static void contents(const char *name,const void *data,size_t bytes)
{
    char buffer[2048];size_t at=0;ssize_t got;int fd=open(name,O_RDONLY);assert(fd>=0);
    while((got=__real_read(fd,buffer,sizeof(buffer)))>0) {
        assert(at+got<=bytes && !memcmp(buffer,(const char *)data+at,got));at+=got;
    }
    assert(!got && at==bytes && !__real_close(fd));
}
static void errors(void)
{errno=E2BIG;recon_doserrno=919;recon_SetLastError(818);}
static void retained(void)
{assert(errno==E2BIG && recon_doserrno==919);}
static void deny(int which)
{
    int status;pid_t child=fork();assert(child>=0);
    if(!child) {
        int quiet=open("/dev/null",O_WRONLY);assert(quiet>=0 && dup2(quiet,2)>=0);__real_close(quiet);
        switch(which) {
        case 0:recon_CopyFileA(NULL,"destination.bin",FALSE);break;
        case 1:recon_CopyFileA("source.bin","destination.bin",TRUE);break;
        case 2:recon_CopyFileA("source.bin","nonempty.bin",FALSE);break;
        case 3:recon_CopyFileA("empty.bin","alias.bin",FALSE);break;
        case 4:recon_CopyFileA("source.bin","readonly.bin",FALSE);break;
        case 5:recon_CopyFileA("fifo","destination.bin",FALSE);break;
        case 6:recon_CopyFileA("source.bin","fifo",FALSE);break;
        case 7:recon_CopyFileA("\200","destination.bin",FALSE);break;
        case 8:{char path[MAX_PATH];memset(path,'a',sizeof(path));recon_CopyFileA(path,"destination.bin",FALSE);break;}
        case 9:recon_CopyFileA("*","destination.bin",FALSE);break;
        case 10:recon_CopyFileA("readonly.bin","destination.bin",FALSE);break;
        }
        _exit(42);
    }
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
}
int main(void)
{
    static unsigned char data[70001];struct stat before,after;struct timespec times[2];
    unsigned i;unsigned short cw,sw,newcw,newsw;BOOL result;
    BOOL (WINAPI *typed)(LPCSTR,LPCSTR,BOOL)=recon_CopyFileA;
    (void)typed;
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    recon_CopyFileA(NULL,NULL,FALSE);return 42;
#endif
    for(i=0;i<sizeof(data);++i)data[i]=(unsigned char)(i*31U+i/257);
    file("source.bin",data,sizeof(data));file("destination.bin","",0);
    times[0].tv_sec=946684800;times[0].tv_nsec=123456000;
    times[1].tv_sec=1700000000;times[1].tv_nsec=654321000;
    assert(!utimensat(AT_FDCWD,"source.bin",times,0));assert(!stat("source.bin",&before));
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));
    errors();mode=1;result=recon_CopyFileA("source.bin","destination.bin",FALSE);mode=0;
    assert(result);retained();assert(recon_GetLastError()==818 && reads>1 && writes>reads);
    __asm__ volatile("fnstcw %0;fnstsw %1":"=m"(newcw),"=m"(newsw));assert(cw==newcw && sw==newsw);
    assert(!stat("destination.bin",&after));
    assert(after.st_atim.tv_sec==before.st_atim.tv_sec && after.st_atim.tv_nsec==before.st_atim.tv_nsec);
    assert(after.st_mtim.tv_sec==before.st_mtim.tv_sec && after.st_mtim.tv_nsec==before.st_mtim.tv_nsec);
    contents("source.bin",data,sizeof(data));contents("destination.bin",data,sizeof(data));
    file("empty.bin","",0);file("empty-copy.bin","",0);errors();assert(recon_CopyFileA("empty.bin","empty-copy.bin",FALSE));retained();contents("empty-copy.bin","",0);
    assert(!link("empty.bin","alias.bin"));file("nonempty.bin","KEEP",4);file("readonly.bin","",0);assert(!chmod("readonly.bin",0444));assert(!mkfifo("fifo",0600));
    for(i=0;i<11;++i)deny(i);
    contents("nonempty.bin","KEEP",4);contents("readonly.bin","",0);contents("empty.bin","",0);
    errors();assert(!recon_CopyFileA("absent.bin","empty-copy.bin",FALSE));retained();assert(recon_GetLastError()==ERROR_FILE_NOT_FOUND);
    errors();assert(!recon_CopyFileA("source.bin","absent-destination.bin",FALSE));retained();assert(recon_GetLastError()==ERROR_FILE_NOT_FOUND);
    for(i=2;i<=6;++i) {
        file("destination.bin","",0);reads=writes=closes=times_calls=0;
        errors();mode=i;result=recon_CopyFileA("source.bin","destination.bin",FALSE);mode=0;
        assert(!result);retained();assert(recon_GetLastError()==(i==3?ERROR_DISK_FULL:i==5?ERROR_ACCESS_DENIED:ERROR_INVALID_DATA));
    }
    puts("CopyFileA content/empty/EINTR/short-write/times/error-state/prewrite/denials PASS");return 0;
}

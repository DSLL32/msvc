#define RECON_SHIM_IMPL
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <recon.h>
/* Private existing lease marker, observed only by this native-host probe. */
extern DWORD recon_fsopen_shares[256];
extern long recon_osfhnd[256];
static void save_fp(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(int argc,char **argv)
{
    unsigned char contents[25600],observed[25600],before[108],after[108];
    ShimFILE streams[3];unsigned char lowio[256];long handles[256];HANDLE stdhandles[3],handle,other;
    struct stat file,path;FILE *source;const char *mode;int fd,host,n,flags=0x8182,share=0x10,permission=0x180;
    assert(argc==3);mode=argv[2];source=fopen(argv[1],"rb");assert(source);
    assert(fread(contents,1,sizeof(contents),source)==sizeof(contents) && fgetc(source)==EOF);fclose(source);
    memcpy(streams,recon_iob,sizeof(streams));memcpy(lowio,recon_osfile,sizeof(lowio));memcpy(handles,recon_osfhnd,sizeof(handles));
    for(n=0;n<3;++n)stdhandles[n]=recon_GetStdHandle((DWORD)(-10-n));
    if(!strcmp(mode,"existing")) {host=open("lease.idb",O_CREAT|O_WRONLY|O_EXCL,0600);assert(host>=0);close(host);}
    if(!strcmp(mode,"dangling")) assert(!symlink("missing.idb","lease.idb"));
    if(!strcmp(mode,"directory")) assert(!mkdir("lease.idb",0700));
    if(!strcmp(mode,"flags")) flags|=8;
    if(!strcmp(mode,"access")) flags=0x8180;
    if(!strcmp(mode,"sharing")) share=0x40;
    if(!strcmp(mode,"permission")) permission=0x100;
    if(!strcmp(mode,"prior-mapping")) {
        host=open("other.bin",O_CREAT|O_RDWR|O_EXCL,0600);assert(host>=0);assert(!ftruncate(host,4096));close(host);
        other=recon_CreateFileA("other.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        assert(other!=INVALID_HANDLE_VALUE && recon_CreateFileMappingA(other,NULL,PAGE_READONLY,0,0,NULL));
    }
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    if(!strcmp(mode,"live"))__asm__ __volatile__("fld1":::"st");
    save_fp(before);fd=recon_sopen(!strcmp(mode,"null")?NULL:"lease.idb",flags,share,permission);save_fp(after);
    assert(!memcmp(before,after,108));assert(fd==3 && errno==111 && recon_doserrno==222 && recon_GetLastError()==0);
    assert(recon_osfile[fd]==0x11 && recon_fsopen_shares[fd]==1);
    handle=(HANDLE)recon_get_osfhandle(fd);host=(int)(uintptr_t)handle-1;
    assert(host>=0 && (fcntl(host,F_GETFD)&FD_CLOEXEC) && (fcntl(host,F_GETFL)&O_ACCMODE)==O_RDWR);
    assert(!fstat(host,&file) && !stat("lease.idb",&path) && S_ISREG(file.st_mode) && file.st_size==0);
    assert(file.st_dev==path.st_dev && file.st_ino==path.st_ino && lseek(host,0,SEEK_CUR)==0);
    assert(!memcmp(streams,recon_iob,sizeof(streams)));
    for(n=0;n<256;++n)if(n!=fd)assert(lowio[n]==recon_osfile[n] && handles[n]==recon_osfhnd[n]);
    for(n=0;n<3;++n)assert(stdhandles[n]==recon_GetStdHandle((DWORD)(-10-n)));
    if(!strcmp(mode,"alias")) {assert(!link("lease.idb","alias.idb"));recon_open("alias.idb",0x8002);return 42;}
    if(!strcmp(mode,"repeat")) {recon_sopen("lease.idb",0x8182,0x10,0x180);return 42;}
    if(!strcmp(mode,"dup")) {recon_dup(fd);return 42;}
    if(!strcmp(mode,"dup2")) {recon_dup2(fd,9);return 42;}
    if(!strcmp(mode,"mapping")) {recon_CreateFileMappingA(handle,NULL,PAGE_READWRITE,0,4096,NULL);return 42;}
    if(!strcmp(mode,"raw-alias")) {recon_CreateFileA("lease.idb",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);return 42;}
    save_fp(before);assert(recon_write(fd,contents,sizeof(contents))==sizeof(contents));assert(recon_lseek(fd,1024,SEEK_SET)==1024);save_fp(after);
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==0);
    assert(pread(host,observed,sizeof(observed),0)==sizeof(observed) && !memcmp(contents,observed,sizeof(contents)));
    assert(lseek(host,0,SEEK_CUR)==1024 && recon_osfile[fd]==0x11 && recon_fsopen_shares[fd]==1);
    assert(!memcmp(streams,recon_iob,sizeof(streams)));for(n=0;n<3;++n)assert(stdhandles[n]==recon_GetStdHandle((DWORD)(-10-n)));
    assert(!recon_close(fd) && !recon_osfile[fd] && !recon_fsopen_shares[fd]);
    fd=recon_open("lease.idb",0x8002);assert(fd==3 && recon_osfile[fd]==1 && !recon_fsopen_shares[fd]);assert(!recon_close(fd));
    if(!strcmp(mode,"live"))__asm__ __volatile__("fstp %%st(0)":::"st");
    puts("genuine exclusive binary noinherit lease/file/position/errors/FP/ownership pass");return 0;
}

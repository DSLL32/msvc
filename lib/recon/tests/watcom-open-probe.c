#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void seek_to(int fd,long offset,int origin,long expected,unsigned row)
{
    static const unsigned errors[][2]={{0,0},{2,2},{111,222}};
    unsigned char before[108],after[108];
    errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(333);
    fp(before);assert(recon_watcom_lseek(fd,offset,origin)==expected);fp(after);
    assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1]);
    assert(!recon_GetLastError() && lseek((int)recon_get_osfhandle(fd)-1,0,SEEK_CUR)==expected);
    fp(before);assert(recon_watcom_tell(fd)==expected);fp(after);
    assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1]);
    assert(!recon_GetLastError() && lseek((int)recon_get_osfhandle(fd)-1,0,SEEK_CUR)==expected);
}
int main(int argc,char **argv)
{
    static const unsigned errors[][2]={{0,0},{2,2},{111,222}};
    int fd,host,live=argc>1 && !strcmp(argv[1],"live");unsigned row,n;
    unsigned char before[108],after[108],payload[1024],files[sizeof(recon_iob)];
    struct stat information;
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"tell-invalid")){recon_watcom_tell(-1);return 42;}
        if(!strcmp(argv[1],"mode")){recon_watcom_open("binary.dat",0x8000);return 42;}
        if(!strcmp(argv[1],"null")){recon_watcom_open(NULL,0x0200);return 42;}
        if(!strcmp(argv[1],"empty")){recon_watcom_open("",0x0200);return 42;}
        if(!strcmp(argv[1],"wildcard")){recon_watcom_open("*.dat",0x0200);return 42;}
        if(!strcmp(argv[1],"nonascii")){recon_watcom_open("\200",0x0200);return 42;}
        if(!strcmp(argv[1],"directory")){recon_watcom_open(".",0x0200);return 42;}
        if(!strcmp(argv[1],"symlink")){recon_watcom_open("alias.dat",0x0200);return 42;}
        if(!strncmp(argv[1],"missing",7)) {
            const char *name=!strcmp(argv[1],"missing")?"missing.dat":!strcmp(argv[1],"missing-parent")?"absent/child":"binary.dat/child";
            unsigned char lowio[256];volatile int *target=recon_watcom_errno();
            *target=111;errno=777;recon_doserrno=222;recon_SetLastError(333);memcpy(lowio,recon_osfile,sizeof(lowio));
            fp(before);assert(recon_watcom_open(name,0x0200)==-1);fp(after);
            assert(*target==1 && target==recon_watcom_errno() && errno==777 && recon_doserrno==222 && recon_GetLastError()==333);
            assert(!memcmp(before,after,108) && !memcmp(lowio,recon_osfile,sizeof(lowio)));return 0;
        }
        if(!strcmp(argv[1],"tell-closed")) {
            fd=recon_watcom_open("binary.dat",0x0200);assert(fd>=0 && !recon_close(fd));
            recon_watcom_tell(fd);return 42;
        }
        if(!strncmp(argv[1],"seek-",5)) {
            fd=recon_watcom_open("binary.dat",0x0200);assert(fd>=0);
            if(!strcmp(argv[1],"seek-invalid")){recon_watcom_lseek(-1,0,SEEK_SET);return 42;}
            if(!strcmp(argv[1],"seek-origin")){recon_watcom_lseek(fd,0,3);return 42;}
            if(!strcmp(argv[1],"seek-negative")){recon_watcom_lseek(fd,-1,SEEK_SET);return 42;}
            assert(!strcmp(argv[1],"seek-closed") && !recon_close(fd));
            recon_watcom_lseek(fd,0,SEEK_SET);return 42;
        }
        assert(!strcmp(argv[1],"failure"));
        {struct rlimit limit={3,3};assert(!setrlimit(RLIMIT_NOFILE,&limit));}
        recon_watcom_open("binary.dat",0x0200);return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    memcpy(files,recon_iob,sizeof(files));
    for(row=0;row<3;++row) {
        *recon_watcom_errno()=456;errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(333);
        fp(before);fd=recon_watcom_open("binary.dat",0x0200);fp(after);
        assert(*recon_watcom_errno()==456);
        assert(fd>=0 && (unsigned)fd<recon_nhandle && (recon_osfile[fd]&0x81)==1);
        assert(!memcmp(before,after,108) && errno==(int)errors[row][0] && recon_doserrno==errors[row][1]);
        /* The reused CreateFileA backend clears LastError on success; original
         * Watcom per-call error state is not inferred from this native check. */
        assert(!recon_GetLastError());
        host=(int)recon_get_osfhandle(fd)-1;
        assert((fcntl(host,F_GETFL)&O_ACCMODE)==O_RDONLY && lseek(host,0,SEEK_CUR)==0);
        assert(!fstat(host,&information) && S_ISREG(information.st_mode) && information.st_size==1024);
        assert(recon_read(fd,payload,sizeof(payload))==sizeof(payload));
        for(n=0;n<sizeof(payload);++n)assert(payload[n]==(unsigned char)n);
        seek_to(fd,-24,SEEK_END,1000,row);
        assert(recon_read(fd,payload,24)==24);
        for(n=0;n<24;++n)assert(payload[n]==(unsigned char)(1000+n));
        seek_to(fd,7,SEEK_SET,7,row);seek_to(fd,3,SEEK_CUR,10,row);
        seek_to(fd,-10,SEEK_CUR,0,row);
        assert(!recon_close(fd) && !(recon_osfile[fd]&1));
        assert(!memcmp(files,recon_iob,sizeof(files)));
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}

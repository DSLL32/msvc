#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <recon.h>
int main(int argc,char **argv)
{
    const char *name="folder/binary.dat";int flags=0x0202,first,second,host;
    unsigned n;unsigned char bytes[1024],files[sizeof(recon_iob)];
    const unsigned char replacement[3]={13,10,26};unsigned short cw,sw,after_cw,after_sw;
    struct stat information;
    if(argc>1) {
        if(!strcmp(argv[1],"missing"))name="folder/absent.dat";
        else if(!strcmp(argv[1],"parent"))name="absent/binary.dat";
        else if(!strcmp(argv[1],"directory"))name="folder";
        else if(!strcmp(argv[1],"symlink"))name="alias.dat";
        else if(!strcmp(argv[1],"parent-symlink"))name="alias-folder/binary.dat";
        else if(!strcmp(argv[1],"absolute"))name="/binary.dat";
        else if(!strcmp(argv[1],"dotdot"))name="../binary.dat";
        else if(!strcmp(argv[1],"mode"))flags=0x0201;
        else if(!strcmp(argv[1],"wildcard"))name="folder/*.dat";
        else if(!strcmp(argv[1],"nonascii"))name="\200.dat";
        recon_watcom_open(name,flags);return 42;
    }
    memcpy(files,recon_iob,sizeof(files));
    errno=EDOM;*recon_watcom_errno()=111;recon_doserrno=222;recon_SetLastError(333);
    __asm__ __volatile__("fld1; fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
    first=recon_watcom_open(name,flags);
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
    assert(first>=0 && cw==after_cw && sw==after_sw);
    assert(errno==EDOM && *recon_watcom_errno()==111 && recon_doserrno==222);
    assert(!recon_GetLastError() && (recon_osfile[first]&0x81)==1);
    host=(int)recon_get_osfhandle(first)-1;
    assert((fcntl(host,F_GETFL)&O_ACCMODE)==O_RDWR && lseek(host,0,SEEK_CUR)==0);
    assert(!fstat(host,&information) && S_ISREG(information.st_mode) && information.st_size==1024);
    assert(recon_read(first,bytes,sizeof(bytes))==sizeof(bytes));
    for(n=0;n<sizeof(bytes);++n)assert(bytes[n]==(unsigned char)n);
    /* A supplied unused permission value also has no effect without CREATE. */
    second=recon_watcom_open(name,flags,0123);assert(second>=0 && second!=first);
    assert(recon_watcom_tell(second)==0 && recon_watcom_tell(first)==1024);
    assert(recon_watcom_lseek(first,17,SEEK_SET)==17 && recon_write(first,replacement,3)==3);
    assert(recon_watcom_tell(first)==20 && recon_watcom_tell(second)==0);
    assert(recon_watcom_lseek(second,17,SEEK_SET)==17 && recon_read(second,bytes,3)==3);
    assert(!memcmp(bytes,replacement,3));
    assert(!recon_close(first) && !(recon_osfile[first]&1));
    assert(recon_watcom_lseek(second,0,SEEK_SET)==0 && recon_read(second,bytes,sizeof(bytes))==sizeof(bytes));
    for(n=0;n<sizeof(bytes);++n)assert(bytes[n]==(n>=17 && n<20?replacement[n-17]:(unsigned char)n));
    assert(!recon_close(second) && !(recon_osfile[second]&1));assert(!memcmp(files,recon_iob,sizeof(files)));
    assert(*recon_watcom_errno()==111);
    __asm__ __volatile__("fstp %%st(0)":::"st");return 0;
}

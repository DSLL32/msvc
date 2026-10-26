#include <assert.h>
#include <fcntl.h>
#include <errno.h>
enum { NativeMissingLeaf=ENOENT };
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    char *(__cdecl *choose)(char *)=tmpnam;char output[21],names[16][13];
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    const char payload[]={ 'A',0,'B','\n' };char readback[4];struct stat information;
    unsigned n,prior,index;int fd;unsigned char before[108],after[108];
    int live=argc>1 && !strcmp(argv[1],"live"),names_only=argc>1 && !strcmp(argv[1],"names");
    assert(choose==recon_tmpnam);
    if(argc>1 && !strcmp(argv[1],"null"))choose(NULL);
    if(argc>1 && !strcmp(argv[1],"wrap"))choose((char *)0xfffffff8U);
    if(argc>1 && !strcmp(argv[1],"cwd")){assert(!mkdir("gone",0700) && !chdir("gone") && !rmdir("../gone"));choose(output+4);}
    if(argc>1 && !strcmp(argv[1],"permission"))recon_watcom_open("new.dat",0x0261,0600);
    if(argc>1 && !strcmp(argv[1],"flags"))recon_watcom_open("new.dat",0x0061,0666);
    if(argc>1 && !strcmp(argv[1],"directory"))recon_watcom_open("folder/new.dat",0x0261,0666);
    if(argc>1 && !strcmp(argv[1],"existing"))recon_watcom_open("existing.dat",0x0261,0666);
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(n=0;n<(names_only?1U:16U);++n) {
        memset(output,0x5a,sizeof(output));errno=111+n;*recon_errno()=211+n;
        recon_doserrno=311+n;SetLastError(411+n);fp(before);
        assert(choose(output+4)==output+4);fp(after);
        assert(errno==111+(int)n && *recon_errno()==211+(int)n && recon_doserrno==311+n && GetLastError()==411+n && !memcmp(before,after,108));
        assert(strlen(output+4)==8 && output[4]=='R' && output[5]=='C');
        for(index=0;index<4;++index)assert(output[index]==0x5a);
        for(index=13;index<sizeof(output);++index)assert(output[index]==0x5a);
        assert(lstat(output+4,&information)==-1 && *recon_errno()==NativeMissingLeaf);
        for(prior=0;prior<n;++prior)assert(strcmp(output+4,names[prior]));
        strcpy(names[n],output+4);
        if(!names_only) {
            fd=recon_watcom_open(names[n],O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0666);assert(fd>=0 && (recon_osfile[fd]&0x81)==1);
            assert(recon_lseek(fd,0,SEEK_CUR)==0 && recon_write(fd,payload,4)==4 && !recon_close(fd));
            fd=recon_watcom_open(names[n],O_BINARY);assert(fd>=0 && recon_read(fd,readback,4)==4);
            assert(!memcmp(payload,readback,4) && !recon_close(fd));
        }
    }
    if(!names_only)for(n=0;n<16;++n)assert(!recon_unlink(names[n]));
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
#else
    (void)argc;(void)argv;(void)names;(void)fp;choose(output+4);return 42;
#endif
}

#include <assert.h>
#include <fcntl.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *name="created.dat";int flags=0x0162,permission=0666,fd,binary=0;
    unsigned char before[108],after[108],data[8];unsigned row;
    static const unsigned char payload[]={ 'A','\n','B',0,'Z' };
    assert(argc>1);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(O_RDWR==2 && O_CREAT==0x20 && O_TRUNC==0x40 && O_TEXT==0x100 && O_BINARY==0x200);
    assert(_O_CREAT==0x100 && _O_TRUNC==0x200 && _O_TEXT==0x4000 && _O_BINARY==0x8000);
    flags=O_RDWR|O_CREAT|O_TRUNC|O_TEXT;
#else
    assert(O_CREAT==0x40 && O_TRUNC==0x200);
#endif
    if(!strncmp(argv[1],"binary",6)){binary=1;flags=0x0262;permission=!strcmp(argv[1],"binary600")?0600:0666;}
    if(argc>2 && !strcmp(argv[2],"old"))name="existing.dat";
    if(!strcmp(argv[1],"linux"))flags=0x0242;
    if(!strcmp(argv[1],"text600"))permission=0600;
    if(!strcmp(argv[1],"permission")){flags=0x0262;permission=0644;}
    if(!strcmp(argv[1],"directory"))name="folder";
    if(!strcmp(argv[1],"symlink"))name="alias.dat";
    if(!strcmp(argv[1],"path"))name="missing/new.dat";
    if(argc>2 && !strcmp(argv[2],"directory-new"))name="folder/created.dat";
    if(argc>2 && !strcmp(argv[2],"directory-old"))name="folder/existing.dat";
    if(argc>2 && !strcmp(argv[2],"backslash-new"))name="folder\\created.dat";
    if(argc>2 && !strcmp(argv[2],"backslash-old"))name="folder\\existing.dat";
    if(argc>2 && !strcmp(argv[2],"nested-new"))name="folder/sub/created.dat";
    if(argc>2 && !strcmp(argv[2],"nested-old"))name="folder/sub/existing.dat";
    if(!strcmp(argv[1],"parent-symlink"))name="aliasdir/new.dat";
    if(!strcmp(argv[1],"parent-file"))name="existing.dat/new.dat";
    if(!strcmp(argv[1],"traversal"))name="folder/../new.dat";
    if(!strcmp(argv[1],"dot"))name="./new.dat";
    if(!strcmp(argv[1],"double"))name="folder//new.dat";
    if(!strcmp(argv[1],"absolute"))name=argc>2?argv[2]:"/";
    if(!strcmp(argv[1],"drive"))name="C:new.dat";
    if(!strcmp(argv[1],"unc"))name="\\\\server\\new.dat";
    if(!strcmp(argv[1],"trailing-parent"))name="folder./new.dat";
    if(!strcmp(argv[1],"trailing-slash"))name="folder/";
    if(!strcmp(argv[1],"trailing"))name="trailing.";
    if(!strcmp(argv[1],"null"))name=NULL;
    if(argc>3)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row) {
#ifdef RECON_CRT_PROFILE_WATCOM_100A
        errno=111+row;*recon_errno()=777;recon_doserrno=222;recon_SetLastError(333);
#endif
        fp(before);fd=recon_watcom_open(name,flags,permission);fp(after);
        assert(fd>=0 && !memcmp(before,after,108));
        assert((recon_osfile[fd]&0x81)==(binary?1:0x81));
        assert(recon_lseek(fd,0,SEEK_END)==0 && recon_lseek(fd,0,SEEK_SET)==0);
        assert(recon_write(fd,payload,binary?5:3)==(binary?5:3));
        assert(recon_lseek(fd,0,SEEK_CUR)==(binary?5:4));
        if(binary) {
            assert(recon_lseek(fd,0,SEEK_SET)==0 && recon_read(fd,data,5)==5);
            assert(!memcmp(data,payload,5));
        }
        assert(!recon_close(fd));
#ifdef RECON_CRT_PROFILE_WATCOM_100A
        assert(errno==(int)(111+row));
#endif
    }
    if(argc>3)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}

#include <assert.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static int (__cdecl *set_binary)(int,int)=setmode;
#else
static int (__cdecl *set_binary)(int,int)=recon_watcom_setmode;
#endif
int main(int argc,char **argv)
{
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    int fd,descriptor,previous,live=argc>1 && !strcmp(argv[1],"live");
    unsigned row,old,n;unsigned char before[108],after[108],files[sizeof(recon_iob)],flags[256];
    long handles[3];
    if(argc>1 && !live) {
        if(!strcmp(argv[1],"invalid")){set_binary(-1,0x0200);return 42;}
        if(!strcmp(argv[1],"bound")){set_binary(recon_nhandle,0x0200);return 42;}
        if(!strcmp(argv[1],"text")){set_binary(0,0x0100);return 42;}
        if(!strcmp(argv[1],"ms")){set_binary(0,0x8000);return 42;}
        if(!strcmp(argv[1],"zero")){set_binary(0,0);return 42;}
        assert(!strcmp(argv[1],"closed"));
        fd=recon_open("binary.dat",0x8000);assert(fd>=0 && !recon_close(fd));
        set_binary(fd,0x0200);return 42;
    }
    fd=recon_open("binary.dat",0x8000);assert(fd>=0 && fd<256 && recon_nhandle<=256);
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    memcpy(files,recon_iob,sizeof(files));
    handles[0]=recon_get_osfhandle(0);handles[1]=recon_get_osfhandle(1);handles[2]=recon_get_osfhandle(fd);
    for(descriptor=0;descriptor<3;++descriptor)for(old=0;old<2;++old)for(row=0;row<3;++row) {
        n=descriptor==2?(unsigned)fd:(unsigned)descriptor;
        assert(recon_setmode(n,old?0x4000:0x8000)!=-1);
        memcpy(flags,recon_osfile,recon_nhandle);
        errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);previous=set_binary(n,0x0200);fp(after);
        assert(previous==(old?0x0100:0x0200) && !(recon_osfile[n]&0x80));
        flags[n]&=~0x80;
        assert(!memcmp(flags,recon_osfile,recon_nhandle) && !memcmp(files,recon_iob,sizeof(files)));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        assert(!memcmp(before,after,108));
        assert(recon_get_osfhandle(0)==handles[0] && recon_get_osfhandle(1)==handles[1] && recon_get_osfhandle(fd)==handles[2]);
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    assert(!recon_close(fd));return 0;
}

#include <conio.h>
#include <assert.h>
#include <recon.h>
extern int *__errno_location(void);
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    int (__cdecl *character)(void)=recon_watcom_getch;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    unsigned row;unsigned char before[108],after[108],lowio[256];ShimFILE streams[20];
    assert(getch==character);
    if(argc>1 && !strcmp(argv[1],"handle"))assert(SetStdHandle(STD_INPUT_HANDLE,(HANDLE)2));
    if(argc>1 && !strcmp(argv[1],"closed"))recon_osfile[0]=0;
    if(argc>1 && !strcmp(argv[1],"binary"))assert(recon_watcom_setmode(0,0x200)==0x100);
    if(argc>1 && !strcmp(argv[1],"flags"))recon_osfile[0]=1;
    if(argc>1 && !strcmp(argv[1],"live"))__asm__ __volatile__("fld1\n\tfldpi":::"st");
    memcpy(lowio,recon_osfile,256);memcpy(streams,recon_iob,sizeof(streams));
    for(row=0;row<3;++row) {
        *__errno_location()=113+row;errno=211+row;SetLastError(319+row);
        fp(before);assert(character()==0);fp(after);
        assert(!memcmp(before,after,108) && *__errno_location()==113+(int)row && errno==211+(int)row);
        assert(GetLastError()==0); /* Native ReadFile success, not an original-error claim. */
        assert(!memcmp(lowio,recon_osfile,256) && !memcmp(streams,recon_iob,sizeof(streams)));
    }
    if(argc>1 && !strcmp(argv[1],"live"))__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
#else
#ifdef getch
#error Watcom getch alias leaked
#endif
    (void)argc;(void)argv;character();return 42;
#endif
}

#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void *(__cdecl *fill)(void *,unsigned,unsigned)=__STOSD;
#else
#ifdef __STOSD
#error Watcom fill alias leaked
#endif
static void *(__cdecl *fill)(void *,unsigned,unsigned)=recon_watcom_fill_dwords;
#endif
int main(int argc,char **argv)
{
    static const unsigned patterns[]={0,4,0x12345678U,0xffffffffU};
    static const unsigned counts[]={6,11,7,0};
    unsigned char memory[80],before[108],after[108];unsigned row,n,byte;
    unsigned *errors;unsigned tuples[][3]={{0,0,0},{2,2,0},{111,222,333}};
    if(argc>1 && strcmp(argv[1],"live")) {
        if(!strcmp(argv[1],"null"))fill(NULL,4,1);
        if(!strcmp(argv[1],"wrap"))fill((void *)0xfffffff0U,4,8);
        if(!strcmp(argv[1],"count"))fill((void *)1,4,0x40000000U);
        return 42;
    }
    if(argc>1)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<3;++row)for(n=0;n<4;++n) {
        memset(memory,0xa5,sizeof(memory));errors=tuples[row];
        errno=errors[0];recon_doserrno=errors[1];recon_SetLastError(errors[2]);
        fp(before);assert(fill(memory+3,patterns[n],counts[n])==memory+3+4*counts[n]);fp(after);
        assert(!memcmp(before,after,108) && errno==(int)errors[0] && recon_doserrno==errors[1] && recon_GetLastError()==errors[2]);
        for(byte=0;byte<sizeof(memory);++byte)
            assert(memory[byte]==(byte>=3 && byte<3+4*counts[n]?((patterns[n]>>(8*((byte-3)%4)))&255):0xa5));
    }
    assert(fill(NULL,0,0)==NULL && fill((void *)1,0x12345678,0)==(void *)1);
    if(argc>1)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}

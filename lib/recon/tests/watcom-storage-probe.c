#include <assert.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static unsigned *policy=&_amblksiz;
static char *project(char __far *pointer){return pointer;}
static char *(*flat)(char *)=project;
#else
#ifdef _amblksiz
#error Watcom allocator alias leaked
#endif
#ifdef __far
#error Watcom far annotation leaked
#endif
#endif
int main(void)
{
    typedef char Unsigned32[(sizeof(unsigned)==4)?1:-1];
    (void)sizeof(Unsigned32);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char buffer[128];unsigned char before[108],after[108];
    assert(policy==&recon_watcom_amblksiz && _amblksiz==16 && flat(buffer)==buffer);
    errno=111;recon_doserrno=222;recon_SetLastError(333);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])before));
    _amblksiz=32;assert(recon_watcom_amblksiz==32);_amblksiz=16;
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])after));
    assert(!memcmp(before,after,108) && errno==111 && recon_doserrno==222 && recon_GetLastError()==333);
#endif
    return 0;
}

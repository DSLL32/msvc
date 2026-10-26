#include <assert.h>
#include <malloc.h>
#include <recon.h>
int main(void)
{
    unsigned char *bytes=alloca(63);unsigned n;
    assert(bytes);for(n=0;n<63;++n)bytes[n]=(unsigned char)(n+7);
    for(n=0;n<63;++n)assert(bytes[n]==(unsigned char)(n+7));
    return 0;
}

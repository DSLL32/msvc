#include <assert.h>
#include <recon.h>
int main(void)
{
    unsigned n;
    for(n=0;n<4;++n) {
        int crt=(n&1)?111:0; DWORD error=(n&2)?0x12345678:0;
        errno=crt;SetLastError(error);
        assert(GetACP()==1252 && errno==crt && GetLastError()==error);
    }
    return 0;
}

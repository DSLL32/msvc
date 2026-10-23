#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    HANDLE heap; unsigned char *block; unsigned n;
    if(argc>1 && !strcmp(argv[1],"create")) HeapCreate(0,0,0);
    if(argc>1 && !strcmp(argv[1],"message")) SendMessageA(NULL,0,0,0);
    for(n=0;n<4;++n) {
        DWORD error=(n&1)?0x12345678:0; int crt_errno=(n&2)?111:0;
        errno=crt_errno; SetLastError(error);
        heap=HeapCreate(1,0,0); assert(heap && errno==crt_errno && GetLastError()==error);
        if(argc>1 && !strcmp(argv[1],"owner")) HeapAlloc((HANDLE)1,9,528);
        if(argc>1 && !strcmp(argv[1],"flags")) HeapAlloc(heap,0,528);
        if(argc>1 && !strcmp(argv[1],"size")) HeapAlloc(heap,9,529);
        block=HeapAlloc(heap,9,528); assert(block && errno==crt_errno && GetLastError()==error);
        if(argc>1 && !strcmp(argv[1],"free")) HeapFree(heap,1,block);
        if(argc>1 && !strcmp(argv[1],"destroy")) HeapDestroy(heap);
        _setmode(1,0x8000); fwrite(block,1,528,stdout);
        memset(block,0xa5,528);
    }
    return 0;
}

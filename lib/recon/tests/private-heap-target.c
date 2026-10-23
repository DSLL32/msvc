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
        if(argc>1 && !strcmp(argv[1],"size")) HeapAlloc(heap,9,0);
        block=HeapAlloc(heap,9,528); assert(block && errno==crt_errno && GetLastError()==error);
        if(argc>1 && !strcmp(argv[1],"free")) HeapFree(heap,0,block);
        if(argc>1 && !strcmp(argv[1],"destroy")) HeapDestroy(heap);
        _setmode(1,0x8000); fwrite(block,1,528,stdout);
        memset(block,0xa5,528);
    }
    {
        static const DWORD sizes[]={16,268,528,582,128008};
        unsigned i,j; unsigned char *other;
        heap=HeapCreate(1,0,0); other=HeapAlloc(heap,9,528); memset(other,0x6b,10);
        for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
            block=HeapAlloc(heap,9,sizes[i]);
            for(j=0;j<sizes[i];++j) assert(block[j]==0);
            errno=111;SetLastError(0x12345678);
            assert(HeapFree(heap,1,block) && errno==111 && GetLastError()==0x12345678);
            for(j=0;j<10;++j) assert(other[j]==0x6b);
            if(argc>1 && !strcmp(argv[1],"double")) HeapFree(heap,1,block);
        }
        if(argc>1 && !strcmp(argv[1],"null")) HeapFree(heap,1,NULL);
        if(argc>1 && !strcmp(argv[1],"interior")) HeapFree(heap,1,other+1);
        if(argc>1 && !strcmp(argv[1],"wrongowner")) HeapFree(HeapCreate(1,0,0),1,other);
        if(argc>1 && !strcmp(argv[1],"overflow")) HeapAlloc(heap,9,0xffffffff);
        assert(HeapFree(heap,1,other));
    }
    return 0;
}

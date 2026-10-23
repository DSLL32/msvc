#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv)
{
    unsigned n,k; HLOCAL memory,result; int error,zero; DWORD last;
    for(n=0;n<4;++n) {
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        memory=LocalAlloc(argc>1 && !strcmp(argv[1],"flags")?2:LMEM_ZEROINIT,11);
        error=errno;last=GetLastError(); if(!memory) return 3;
        zero=1; for(k=0;k<11;++k) if(((unsigned char *)memory)[k]) zero=0;
        printf("alloc %u %d %d %lu\n",n,zero,error,last); strcpy((char *)memory,"client.exe");
        errno=(n&2)?111:0; SetLastError((n&1)?0x12345678:0);
        result=LocalFree(argc>1 && !strcmp(argv[1],"owner")?(HLOCAL)1:memory);
        printf("free %u %d %d %lu\n",n,result==NULL,errno,GetLastError());
#ifdef RECON_NATIVE_TEST
        if(argc>1 && !strcmp(argv[1],"twice")) LocalFree(memory);
#endif
    }
    return 0;
}

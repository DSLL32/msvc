/* Explicit original-only API observation; never a native fixture dependency. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>
int main(void)
{
    DWORD sentinels[2]={0,0x12345678},error;
    unsigned i,j,n;int saved_errno;
    HANDLE heap;unsigned char *block;FILE *file;char name[80],path[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA("MSVCRT.dll"),path,sizeof(path));
    printf("crt %s\n",path);
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        errno=j?111:0;SetLastError(sentinels[i]);
        heap=HeapCreate(1,0,0);error=GetLastError();saved_errno=errno;
        printf("create 1 0 0 %lu %d %p %lu %d\n",sentinels[i],j?111:0,heap,error,saved_errno);
        if(!heap)return 125;
        errno=j?111:0;SetLastError(sentinels[i]);
        block=HeapAlloc(heap,9,528);error=GetLastError();saved_errno=errno;
        printf("alloc 9 528 %lu %d %p %lu %d\n",sentinels[i],j?111:0,block,error,saved_errno);
        if(!block)return 126;
        for(n=0;n<528;++n)if(block[n])return 127;
        sprintf(name,"heap-%u-%u.bin",i,j);file=fopen(name,"wb");if(!file)return 128;
        if(fwrite(block,1,528,file)!=528)return 129;
        fclose(file);
    }
    return 0;
}

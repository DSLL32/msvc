/* Explicit original-only observation of APIs reached by resources-rich. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error,page;
    int values[2]={0,111},saved,ok; unsigned i,j;
    HANDLE heap; unsigned char *first,*second;
    char path[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA("MSVCRT.dll"),path,sizeof(path));
    printf("crt %s\n",path);
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        errno=values[j];SetLastError(errors[i]);
        page=GetACP();error=GetLastError();saved=errno;
        printf("GetACP before-error=%lu before-errno=%d result=%lu after-error=%lu after-errno=%d\n",errors[i],values[j],page,error,saved);
        heap=HeapCreate(1,0,0);if(!heap)return 125;
        first=HeapAlloc(heap,9,528);second=HeapAlloc(heap,9,528);
        if(!first||!second)return 126;
        memcpy(first+8,"include",8);memcpy(second+8,".;include",10);
        errno=values[j];SetLastError(errors[i]);
        ok=HeapFree(heap,1,first);error=GetLastError();saved=errno;
        printf("HeapFree flags=1 requested=528 before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",errors[i],values[j],ok,error,saved);
        if(!ok||memcmp(second+8,".;include",10))return 127;
        if(!HeapFree(heap,1,second)||!HeapDestroy(heap))return 128;
    }
    return 0;
}

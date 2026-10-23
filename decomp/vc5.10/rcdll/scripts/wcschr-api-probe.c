/* Explicit original-only canonical CRT boundary probe, outside target graph. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>

typedef WCHAR *(__cdecl *WideSearch)(const WCHAR *,WCHAR);
int main(void)
{
    WCHAR input[10]={L'\\',L':',L'/',0,L'.',L'e',L'w',0,0x1234,0xabcd};
    WCHAR needles[7]={L'\\',L':',L'/',L'.',L'e',L'w',0};
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,offset;
    unsigned i,j,k,n,control_before,status_before,control_after,status_after;
    HMODULE module=GetModuleHandleA("MSVCRT.dll");
    WideSearch search=(WideSearch)GetProcAddress(module,"wcschr");
    WCHAR *result;
    if(!module||!search)return 125;
    printf("provider-base=%08lx function=%08lx WCHAR-bytes=%u\n",(DWORD)module,(DWORD)search,sizeof(WCHAR));
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<7;++k){
        control_before=_control87(0,0);status_before=_statusfp();
        errno=values[j];SetLastError(errors[i]);
        result=search(input,needles[k]);error=GetLastError();saved=errno;
        control_after=_control87(0,0);status_after=_statusfp();
        offset=result?(int)(((DWORD)result-(DWORD)input)/2):-1;
        printf("wcschr input=%08lx needle=%u result=%08lx offset=%d before-errno=%d after-errno=%d before-error=%08lx after-error=%08lx control=%08x/%08x status=%08x/%08x bytes=",(DWORD)input,needles[k],(DWORD)result,offset,values[j],saved,errors[i],error,control_before,control_after,status_before,status_after);
        for(n=0;n<sizeof(input);++n)printf("%02x",((BYTE *)input)[n]);
        printf("\n");
    }
    return 0;
}

/* Explicit original-only ABI probe; outside reconstructed source and graph. */
#include <windows.h>
#include <stdio.h>
#include <errno.h>

typedef int (__cdecl *WideCompare)(const WCHAR *,const WCHAR *,unsigned);
int main(void)
{
    WCHAR left[16],right[16];
    const WCHAR actual[16]={L'0',L'x',L'0',L'0',L'0',L'0',L'0',L'0',L'8',L'0',L'L',0,L'D',0x1234,0xabcd,0};
    unsigned counts[4]={0,1,5,13};
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,result;
    unsigned i,j,k,c,n;
    unsigned char before[108],after[108];
    HMODULE module=GetModuleHandleA("MSVCRT.dll");
    WideCompare compare=(WideCompare)GetProcAddress(module,"wcsncmp");
    if(!module||!compare)return 125;
    printf("provider-base=%08lx function=%08lx WCHAR-bytes=%u\n",(DWORD)module,(DWORD)compare,sizeof(WCHAR));
    for(c=0;c<4;++c)for(i=0;i<2;++i)for(j=0;j<2;++j)for(k=0;k<4;++k){
        for(n=0;n<16;++n)left[n]=right[n]=actual[n];
        if(c==1)right[3]=0xfffe;
        if(c==2){left[0]=0x8000;right[0]=0x7fff;}
        if(c==3){left[0]=0;right[0]=L'X';}
        errno=values[j];SetLastError(errors[i]);
        /* Observe/restores actual x87 state without writing application storage. */
        __asm fnsave before
        __asm frstor before
        result=compare(left,right,counts[k]);
        __asm fnsave after
        __asm frstor after
        error=GetLastError();saved=errno;
        printf("wcsncmp case=%u count=%u result=%d before-errno=%d after-errno=%d before-error=%08lx after-error=%08lx left=",c,counts[k],result,values[j],saved,errors[i],error);
        for(n=0;n<sizeof(left);++n)printf("%02x",((BYTE *)left)[n]);
        printf(" right=");for(n=0;n<sizeof(right);++n)printf("%02x",((BYTE *)right)[n]);
        printf(" fp-before=");for(n=0;n<108;++n)printf("%02x",before[n]);
        printf(" fp-after=");for(n=0;n<108;++n)printf("%02x",after[n]);
        printf("\n");
    }
    return 0;
}

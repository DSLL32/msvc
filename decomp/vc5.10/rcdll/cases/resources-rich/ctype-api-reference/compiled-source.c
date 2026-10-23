/* Explicit original-only probe for the ASCII source scanner. */
#include <windows.h>
#include <stdio.h>
#include <wctype.h>
#include <errno.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,result;
    unsigned i,j,value,mask;
    for(i=0;i<2;++i)for(j=0;j<2;++j)
        for(value=0;value<128;++value)for(mask=4;mask<=8;mask+=4){
            errno=values[j];SetLastError(errors[i]);
            result=iswctype((wint_t)value,(wctype_t)mask);error=GetLastError();saved=errno;
            printf("ctype value=%u mask=%u before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",value,mask,errors[i],values[j],result,error,saved);
        }
    return 0;
}

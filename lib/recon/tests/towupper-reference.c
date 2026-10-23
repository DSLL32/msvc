/* Explicit original-only probe for the internal prefix matcher. */
#include <windows.h>
#include <stdio.h>
#include <wctype.h>
#include <errno.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved;
    wint_t result;
    unsigned i,j,value;
    for(i=0;i<2;++i)for(j=0;j<2;++j)for(value=0;value<128;++value){
        errno=values[j];SetLastError(errors[i]);
        result=towupper((wint_t)value);error=GetLastError();saved=errno;
        printf("towupper value=%u before-error=%lu before-errno=%d result=%u after-error=%lu after-errno=%d\n",value,errors[i],values[j],result,error,saved);
    }
    return 0;
}

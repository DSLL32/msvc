/* Explicit original-only probe for the ASCII source scanner. */
#include <windows.h>
#include <stdio.h>
#include <wctype.h>
#include <errno.h>
#include <recon.h>
int main(void)
{
    DWORD errors[2]={0,0x12345678},error;
    int values[2]={0,111},saved,result;
    unsigned i,j,value,mask;
#ifdef RECON_NATIVE_TEST
    {
        const char *mode=recon_host_getenv("RECON_WCTYPE_NEGATIVE");
        if(mode && !strcmp(mode,"nonascii"))iswctype(128,4);
        if(mode && !strcmp(mode,"mask"))iswctype('a',1);
        if(mode && !strcmp(mode,"locale"))recon_lc_handle[2]=1033;
    }
#endif
    for(i=0;i<2;++i)for(j=0;j<2;++j)
        for(value=0;value<128;++value)for(mask=4;mask<=8;mask+=4){
            errno=values[j];SetLastError(errors[i]);
            result=iswctype((wint_t)value,(wctype_t)mask);error=GetLastError();saved=errno;
            printf("ctype value=%u mask=%u before-error=%lu before-errno=%d result=%d after-error=%lu after-errno=%d\n",value,mask,errors[i],values[j],result,error,saved);
        }
    return 0;
}

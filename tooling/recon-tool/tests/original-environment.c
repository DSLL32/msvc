/* Native i386 tests use the exact pure-C merge routine compiled into the broker. */
#include <assert.h>
#include <stdio.h>
#include "../wine/environment.h"
int main(void)
{
    static const char base[]="Z=last\0Path=old\0WINEPREFIX=transport\0=C:=C:\\current\0KEEP=value\0EMPTY=before\0\0";
    static const char overlay[]="PATH=\0WINEPREFIX\0NEW=a=b\0empty\0\0";
    static const char expected[]="=C:=C:\\current\0KEEP=value\0NEW=a=b\0PATH=\0Z=last\0\0";
    static const char duplicate[]="PATH=x\0path\0\0";
    static const char invalid[]="=hidden=value\0\0";
    static const char trailing[]="A=x\0\0junk\0\0";
    static const char nonascii[]="A=\xff\0\0";
    const char empty[2]={0,0};
    char before[sizeof(base)],*result,*large;
    size_t n;
    memcpy(before,base,sizeof(base));
    result=original_environment_merge(base,sizeof(base)-1,overlay,sizeof(overlay)-1);
    assert(result && !memcmp(result,expected,sizeof(expected)-1));free(result);
    assert(!memcmp(before,base,sizeof(base))); /* Broker input stays unchanged. */
    result=original_environment_merge(empty,2,empty,2);
    assert(result && !result[0] && !result[1]);free(result);
    assert(!original_environment_merge(base,sizeof(base)-1,duplicate,sizeof(duplicate)-1));
    assert(!original_environment_merge(base,sizeof(base)-1,invalid,sizeof(invalid)-1));
    assert(!original_environment_merge(base,sizeof(base)-1,trailing,sizeof(trailing)-1));
    assert(!original_environment_merge(base,sizeof(base)-1,nonascii,sizeof(nonascii)-1));
    assert(!original_environment_merge(nonascii,sizeof(nonascii)-1,empty,2));
    assert(!original_environment_merge(base,sizeof(base)-2,empty,2));
    large=malloc(ORIGINAL_ENVIRONMENT_LIMIT);assert(large);
    large[0]='A';large[1]='=';
    for(n=2;n<ORIGINAL_ENVIRONMENT_LIMIT-2;++n)large[n]='x';
    large[n++]=0;large[n]=0;
    result=original_environment_merge(empty,2,large,ORIGINAL_ENVIRONMENT_LIMIT);
    assert(result);free(result);
    assert(!original_environment_merge(base,sizeof(base)-1,large,ORIGINAL_ENVIRONMENT_LIMIT));
    free(large);
    puts("original target environment: sets/removals/empty/hidden/sort/bounds/ASCII PASS");
    return 0;
}

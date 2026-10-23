#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <recon.h>
int main(void)
{
    char *value,*entry;int n,error;DWORD last;char **environment;
#ifdef RECON_NATIVE_TEST
    char text[]="RECON_ENV_TEST=Alpha tail "; char *variables[]={text,NULL};char *arguments[]={"probe",NULL};
    recon_crt_bind_arguments(1,arguments,variables);environment=variables;
#else
    _putenv("RECON_ENV_TEST=Alpha tail ");environment=_environ;
#endif
    for(n=0;environment[n];++n)if(!strncmp(environment[n],"RECON_ENV_TEST=",15))break;
    entry=environment[n];if(!entry)return 2;
    errno=111;SetLastError(123);value=getenv("recon_env_test");error=errno;last=GetLastError();
    printf("value=%s alias=%d errno=%d last=%lu\n",value,value==entry+15,error,last);
    value[0]='a';printf("mutation=%s missing=%d\n",getenv("RECON_ENV_TEST"),getenv("RECON_ENV_TEST_X")==NULL);
    return 0;
}

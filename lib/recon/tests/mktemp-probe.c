#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#ifdef RECON_NATIVE_TEST
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <recon.h>
int main(int argc,char **argv)
{
    char pattern[]=".\\XXXXXX",*result;int n=argc>1?atoi(argv[1]):0,error;DWORD last;unsigned long dos;
    errno=(n&1)?111:0;_doserrno=77;SetLastError((n&2)?123:0);
#ifdef RECON_NATIVE_TEST
    if(argc>2 && !strcmp(argv[2],"child")) {
        int status;pid_t child=fork();if(child<0)return 126;
        if(child>0){waitpid(child,&status,0);return WIFEXITED(status)?WEXITSTATUS(status):126;}
    }
#endif
    result=_mktemp(pattern);error=errno;last=GetLastError();dos=_doserrno;
    printf("alias=%d length=%u prefix=%d errno=%d dos=%lu last=%lu\n",result==pattern,(unsigned)strlen(pattern),!strncmp(pattern,".\\a",3),error,dos,last);
#ifdef RECON_NATIVE_TEST
    if(argc>2 && !strcmp(argv[2],"second")) {strcpy(pattern,".\\XXXXXX");_mktemp(pattern);}
#endif
    return 0;
}

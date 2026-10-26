#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
int main(int argc,char **argv,char **envp)
{
    assert(argc==2 && !strcmp(argv[1],"packed first second"));
    assert(fcntl(198,F_GETFD)==-1 && errno==EBADF);
    assert(envp[0] && !strcmp(envp[0],"MSC_CMD_FLAGS=only") && !envp[1]);
    puts("unmarked ordinary native target/no transport descriptor PASS");return 0;
}

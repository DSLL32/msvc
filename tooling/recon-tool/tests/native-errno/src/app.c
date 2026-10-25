/* Target-side errno observations; host metadata must not replace these values. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static int before_constructor;
__attribute__((constructor)) static void constructor(void)
{
    before_constructor=errno;
    errno=77;
}
static void at_exit(void)
{
    int actual=errno;
    int fd=open("errno.txt",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd>=0) { write(fd,actual==99?"99":"bad",actual==99?2:3); close(fd); }
}
int errno_app(int expected)
{
    int actual=errno;
    if(actual!=expected || before_constructor!=0)return 0;
    if(atexit(at_exit))return 0;
    errno=99;
    return 1;
}

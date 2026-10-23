#include <assert.h>
#include <errno.h>
#include <string.h>
#include <recon.h>
int main(int argc,char **argv)
{
    char *first,*second;
    errno=ENOENT;SetLastError(0x12345678);
    if(argc>1) {
        if(!strcmp(argv[1],"errno"))errno=EACCES;
        if(!strcmp(argv[1],"prefix"))_strerror("input");
    }
    first=_strerror(NULL);
    assert(first && !strcmp(first,"No such file or directory\n"));
    assert(errno==ENOENT && GetLastError()==0x12345678);
    second=_strerror("");assert(second==first);
    assert(!strcmp(first,"No such file or directory\n"));
    assert(errno==ENOENT && GetLastError()==0x12345678);
    return 0;
}

#include "recon-tool.h"
#include <errno.h>
#include <stdlib.h>
extern int errno_app(int);
static int setup(void)
{
    int actual=errno;
    if(actual!=77)return 0;
    if(getenv("ERRNO_KEEP"))errno=77;
    else errno=88;
    return 1;
}
static int io(int argc,char **argv)
{
    int actual=errno,expected=getenv("ERRNO_KEEP")?77:88;
    (void)argc;(void)argv;
    errno=actual;
    return errno_app(expected)?0:9;
}
static int direct(void)
{
    return errno_app(88);
}
static int first(void) { int ok=errno==88;errno=99;return ok; }
static int forked(void) { int ok=errno==88;errno=123;return ok; }
static int second(void) { int ok=errno==99;errno=111;return ok; }
static int third(void) { return errno==111; }
static const ReconTest tests[]={
    {"first",0,first},{"forked",1,forked},{"second",0,second},
    {"forked-again",1,forked},{"third",0,third}
};
static const uint32_t order[]={0x1000};

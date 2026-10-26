#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <malloc.h>
#include <recon.h>
static volatile int low_result;
static void normal(void)
{
    volatile unsigned char canary[32];unsigned char *bytes;unsigned n;int calls=0;int *host_errno=__errno_location();unsigned short cw,sw,after_cw,after_sw;
    for(n=0;n<sizeof(canary);++n)canary[n]=(unsigned char)(n+71);
    errno=17;recon_doserrno=73;recon_SetLastError(91);*host_errno=EDOM;
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));
    bytes=alloca((++calls,400));assert(bytes && calls==1);
    assert(errno==17 && *host_errno==EDOM && recon_doserrno==73 && recon_GetLastError()==91);
    __asm__ __volatile__("fnstcw %0; fnstsw %1":"=m"(after_cw),"=m"(after_sw));
    assert(cw==after_cw && sw==after_sw);
    for(n=0;n<400;++n)bytes[n]=(unsigned char)n;
    for(n=0;n<400;++n)assert(bytes[n]==(unsigned char)n);
    for(n=0;n<sizeof(canary);++n)assert(canary[n]==(unsigned char)(n+71));
}
static void *thread(void *unused)
{
    pthread_attr_t attributes;void *base;size_t length,guard,consume;
    volatile unsigned char marker[32],*burn;uintptr_t low;
    (void)unused;normal(); /* Cache this real thread's bounds with adequate stack. */
    assert(!pthread_getattr_np(pthread_self(),&attributes));
    assert(!pthread_attr_getstack(&attributes,&base,&length));
    assert(!pthread_attr_getguardsize(&attributes,&guard));
    assert(!pthread_attr_destroy(&attributes));
    low=(uintptr_t)base+guard;assert((uintptr_t)marker>low+8192);
    consume=(uintptr_t)marker-low-320;
    burn=__builtin_alloca(consume);burn[0]=7;burn[consume-1]=8;
    /* Only a shallow cached-bound call is made while this stack is low. */
    low_result=alloca(400)==NULL;
    return NULL;
}
int main(int argc,char **argv)
{
    pthread_attr_t attributes;pthread_t id;(void)argv;
#ifdef TEST_OTHER_PROFILE
    (void)argc;recon_watcom_alloca_available(400);return 9;
#endif
    if(argc>1){(void)alloca(401);return 9;}
    normal();
    assert(!pthread_attr_init(&attributes));assert(!pthread_attr_setstacksize(&attributes,0x20000));
    assert(!pthread_create(&id,&attributes,thread,NULL));assert(!pthread_attr_destroy(&attributes));
    assert(!pthread_join(id,NULL) && low_result==1);
    puts("caller-frame400 stack bytes/canaries/errors and real thread low-space rejection pass");
    return 0;
}

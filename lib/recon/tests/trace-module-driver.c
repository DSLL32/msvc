#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../include/recon-trace.h"
static void fp(unsigned char b[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])b)); }
int main(int argc,char **argv)
{
    void *module;void (*configure)(const ReconTraceEntry *,size_t,int),(*enable)(int),(*reset)(void),(*entries)(ReconTraceEntry *);
    int (*glue)(int),fd,second,status;pid_t child;ReconTraceEntry functions[4];
    unsigned char before[108],after[108];const char *mode;
    char label[16]="app_alpha";ReconTraceEntry original;
    assert(argc==5);mode=argv[4];module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(module);
    configure=dlsym(module,"recon_trace_configure");enable=dlsym(module,"recon_trace_enable");reset=dlsym(module,"recon_trace_reset");
    entries=dlsym(module,"module_entries");glue=dlsym(module,"module_glue");assert(configure && enable && reset && entries && glue);
    entries(functions);assert(glue(0)==4);
    fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0600);second=open(argv[3],O_CREAT|O_TRUNC|O_WRONLY,0600);assert(fd>=0 && second>=0);
    if(!strcmp(mode,"unconfigured")){enable(1);return 90;}
    if(!strcmp(mode,"address"))functions[1].address=functions[0].address;
    if(!strcmp(mode,"label"))functions[1].name=functions[0].name;
    if(!strcmp(mode,"newline"))functions[0].name="bad\nlabel";
    if(!strcmp(mode,"null-name"))functions[0].name=NULL;
    if(!strcmp(mode,"null-address"))functions[0].address=NULL;
    if(!strcmp(mode,"null-registry")){configure(NULL,1,fd);return 90;}
    if(!strcmp(mode,"large")){configure(functions,100001,fd);return 90;}
    if(!strcmp(mode,"fd")){configure(functions,4,-1);return 90;}
    if(!strcmp(mode,"readonly")){close(fd);fd=open(argv[2],O_RDONLY);assert(fd>=0);}
    original=functions[0];if(!strcmp(mode,"normal"))functions[0].name=label;
    errno=EDOM;__asm__ __volatile__("fld1\n\tfldpi":::"st");fp(before);
    configure(functions,4,fd);functions[0].address=NULL;label[0]='X';
    assert(errno==EDOM && glue(0)==4);enable(1);
    assert(glue(0)==4 && glue(0)==4 && errno==EDOM);
    fp(after);assert(!memcmp(before,after,108));
    child=fork();assert(child>=0);
    if(!child){fp(before);assert(glue(1)==5 && errno==EDOM);fp(after);assert(!memcmp(before,after,108));_exit(0);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    /* Linux may change inert saved CS metadata across fork. Observe a fresh
     * baseline; each tracer phase still compares every one of its108 bytes. */
    fp(before);assert(glue(1)==5);
    enable(0);assert(glue(0)==4);enable(1);assert(glue(0)==4);
    reset();assert(glue(1)==5);enable(1);assert(glue(1)==5 && glue(0)==4);
    enable(0);functions[0]=original;configure(functions,1,second);assert(glue(0)==4);enable(1);assert(glue(0)==4);
    configure(NULL,0,second);enable(1);assert(glue(0)==4);reset();
    assert(errno==EDOM);fp(after);
    assert(!memcmp(before,after,108));
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    close(fd);close(second);puts("DSO registry/filter/fork/reset/disable/reconfigure/errors/full108FP PASS");return 0;
}

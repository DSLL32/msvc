#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../include/recon-trace.h"
int main(int argc,char **argv)
{
    void *lib;void (*configure)(const ReconTraceEntry *,size_t,int,int),(*enable)(int),(*entries)(ReconTraceEntry *);
    ReconTraceEntry table[4];int (*glue)(int),out,state,status;pid_t child;char a[32],b[32];
    assert(argc==5 || argc==7);lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(lib);
    configure=dlsym(lib,"recon_trace_configure_shared");enable=dlsym(lib,"recon_trace_enable");
    entries=dlsym(lib,"module_entries");glue=dlsym(lib,"module_glue");assert(configure && enable && entries && glue);entries(table);
    if(argc==7){out=atoi(argv[5]);state=atoi(argv[6]);configure(table,4,out,state);enable(1);assert(glue(0)==4 && glue(1)==5);return 0;}
    out=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0600);state=open(argv[3],O_CREAT|O_TRUNC|O_RDWR,0600);assert(out>=0 && state>=0);
    assert(!ftruncate(state,!strcmp(argv[4],"size")?35:36));
    if(!strcmp(argv[4],"dirty")){assert(pwrite(state,"x",1,35)==1);}
    if(!strcmp(argv[4],"readonly")){close(state);state=open(argv[3],O_RDONLY);assert(state>=0);}
    configure(table,4,out,!strcmp(argv[4],"fd")?-1:state);
    if(!strcmp(argv[4],"hash")){table[0].name="different";configure(table,4,out,state);return 90;}
    if(!strcmp(argv[4],"count")){configure(table,3,out,state);return 90;}
    enable(1);assert(glue(0)==4);snprintf(a,sizeof(a),"%d",out);snprintf(b,sizeof(b),"%d",state);
    child=fork();assert(child>=0);
    if(!child){execl(argv[0],argv[0],argv[1],argv[2],argv[3],"child",a,b,(char *)NULL);_exit(99);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(glue(1)==5);configure(table,4,out,state);enable(1);assert(glue(0)==4 && glue(1)==5);
    puts("explicit RST1 exec attach preserves first encounters PASS");return 0;
}

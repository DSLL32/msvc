#include "recon-tool.h"
#include "recon-process.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern int self_main(int,char **),self_state;
__attribute__((constructor)) static void errno_constructor(void)
{
    if(getenv("SELF_ERRNO"))errno=77;
}
static int setup(void)
{
    int initial_errno=errno;
    char value[64];snprintf(value,sizeof(value),"%ld",(long)getpid());
    if(setenv("SELF_SETUP_PID",value,1))return 0;
    if(getenv("SELF_ERRNO")) {
        if(initial_errno!=77)return 0;
        errno=88;
    }
    return ++self_state==8;
}
static void errno_at_exit(void)
{
    int actual=errno;
    write(1,actual==99?"errno-exit:99\n":"errno-exit:bad\n",actual==99?14:15);
}
static int io(int argc,char **argv)
{
    int initial_errno=errno,result,check=getenv("SELF_ERRNO")!=NULL;
    if(check && (initial_errno!=88 || atexit(errno_at_exit)))return 90;
    result=self_main(argc,argv);
    if(check)errno=99;
    return result;
}
static int policy(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    uint32_t i;const char *mode=getenv("SELF_CASE");
    if(request->struct_size!=sizeof(*request) ||
       request->target_os_environment_count || request->target_os_environment ||
       request->protocol!=RECON_PROCESS_RSP1 || request->mode || request->index>1 ||
       strcmp(request->cwd,request->workspace_cwd) ||
       request->environment_count!=request->prepared_environment_count)return 0;
    for(i=0;i<request->environment_count;++i)
        if(strcmp(request->environment[i],request->prepared_environment[i]))return 0;
    if(!request->index) {
        if(strcmp(request->file,"T:\\self.exe") || request->argc!=3 ||
           strcmp(request->argv[0],request->file) || strcmp(request->argv[1],"child") ||
           strcmp(request->argv[2],"space arg"))return 0;
        response->action=RECON_PROCESS_SELF;
    } else {
        if(!mode || strcmp(mode,"captured") || strcmp(request->file,"captured.exe") ||
           request->argc!=2 || strcmp(request->argv[0],"captured.exe") || strcmp(request->argv[1],"x"))return 0;
        response->stdout_data="captured\n";response->stdout_size=9;
    }
    return 1;
}
static const uint32_t order[]={0x1000,0x1100,0x1200};
RECON_EXPORT const ReconProcessPolicy case_process_policy={RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),REQUEST_COUNT,policy};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"native-self","native synchronous self-child",order,RECON_COUNT(order),0,0,RECON_IO,RECON_EXIT,0,setup,io,0,0,0};

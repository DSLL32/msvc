#include "recon-tool.h"
#include "recon-process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern int fixture_main(int, char **);
static const uint32_t order[]={0x1000};
static int captured(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    uint32_t i;
    FILE *file;
    if(request->struct_size!=sizeof(*request) || request->index || request->mode ||
        strcmp(request->file,"captured.exe") || strcmp(request->cwd,request->workspace_cwd) ||
        request->argc!=2 || strcmp(request->argv[0],"captured.exe") || strcmp(request->argv[1],"x") ||
        request->environment_count!=request->prepared_environment_count || getenv("RECON_PROCESS_BAD"))return 0;
    for(i=0;i<request->environment_count;++i)
        if(strcmp(request->environment[i],request->prepared_environment[i]))return 0;
    if(getenv("RECON_PROCESS_HANG"))for(;;)pause();
    file=fopen("effect.txt","wb");if(!file)return 0;fputs("captured",file);fclose(file);
    response->action=RECON_PROCESS_CAPTURED;
    response->status=getenv("RECON_PROCESS_FAIL")?-1:0;
    response->failure_errno=response->status==-1?2:0;
    response->stdout_data="reply\n";response->stdout_size=6;
    response->stderr_data="diagnostic\n";response->stderr_size=11;
    return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,captured};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"0009-output","post-exit output fixture",order,1,0,0,RECON_IO,RECON_EXIT,0,0,fixture_main,0,0,0};

#include "recon-output.h"
extern int fixture_state;
static int check_outputs(const ReconOutputContext *context)
{
    char cwd[4096],bytes[128],expected[64];
    FILE *file; size_t count;
    if(context->struct_size!=sizeof(*context) || !context->executor_pid ||
       context->terminal_kind!=RECON_OUTPUT_EXIT || context->terminal_value ||
       !getcwd(cwd,sizeof(cwd)) || strcmp(cwd,context->workspace_cwd) ||
       fixture_state!=7 || getenv("RECON_OUTPUT_FALSE"))return 0;
    if(getenv("RECON_OUTPUT_HANG"))for(;;)pause();
    if(getenv("RECON_OUTPUT_DELAY"))usleep(30000);
    file=fopen("descendant.txt","rb");if(!file)return 0;
    count=fread(bytes,1,sizeof(bytes),file);fclose(file);
    if(count!=4 || memcmp(bytes,"done",4))return 0;
    file=fopen(context->stdout_raw,"rb");if(!file)return 0;
    count=fread(bytes,1,sizeof(bytes)-1,file);fclose(file);bytes[count]=0;
    snprintf(expected,sizeof(expected),"reply\npid:%u\n",context->executor_pid);
    if(strcmp(bytes,expected))return 0;
    file=fopen(context->stdout_compare,"wb");if(!file)return 0;
    fputs("reply\npid:EXPECTED\n",file);fclose(file);
    if(getenv("RECON_OUTPUT_RAW")) {
        file=fopen(context->stdout_raw,"wb");if(!file)return 0;fputs("changed",file);fclose(file);
    }
    puts("private checker diagnostic");
    return 1;
}
RECON_EXPORT const ReconOutputPolicy case_output_policy={RECON_OUTPUT_POLICY_VERSION,sizeof(ReconOutputPolicy),check_outputs};

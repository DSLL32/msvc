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
    if(getenv("RECON_PROCESS_BAD")) { fputs("fixture policy rejected prepared input\n",stderr); return 0; }
    if(request->struct_size!=sizeof(*request) || request->index || request->mode ||
        strcmp(request->file,"captured.exe") || strcmp(request->cwd,request->workspace_cwd) ||
        request->argc!=2 || strcmp(request->argv[0],"captured.exe") || strcmp(request->argv[1],"x") ||
        request->environment_count!=request->prepared_environment_count)return 0;
    for(i=0;i<request->environment_count;++i)
        if(strcmp(request->environment[i],request->prepared_environment[i]))return 0;
    if(getenv("RECON_PROCESS_HANG"))for(;;)pause();
    file=fopen("effect.txt","wb");if(!file)return 0;fputs("captured",file);fclose(file);
    response->action=RECON_PROCESS_CAPTURED;
    response->status=getenv("RECON_PROCESS_FAIL")?-1:getenv("RECON_PROCESS_SEVEN")?7:0;
    response->failure_errno=response->status==-1?2:0;
    response->stdout_data="reply\n";response->stdout_size=6;
    response->stderr_data="diagnostic\n";response->stderr_size=11;
    return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,captured};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"0008-process","captured process fixture",order,1,0,0,RECON_IO,RECON_EXIT,0,0,fixture_main,0,0,0};

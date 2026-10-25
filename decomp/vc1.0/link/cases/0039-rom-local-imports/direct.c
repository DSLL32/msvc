/* Original boundary manifest SHA256 d0c6886545231c3093698746f3c13d102299a9e0a05ed7cd9b1861a408ee7705. */
/* Only the two captured command-processor copy invocations are permitted.
 * No shell, Wine or guessed command execution is available in native fixtures. */
#define RECON_SHIM_IMPL
#include <string.h>
#include <recon-tool.h>
#include "../../harness/fixture-process.h"
#include <recon.h>
static uint32_t parent_pid;
int link_case_process_request(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    const char *const names[]={"provider.obj","client.obj"};
    const char *const references[]={".link-copy-provider.obj",".link-copy-client.obj"};
    const char *const outputs[]={"repro/provider.obj","repro/client.obj"};
    const char *const commands[]={"copy provider.obj S:\\vc1.0\\link\\rom-local-imports\\repro\\.",
        "copy client.obj S:\\vc1.0\\link\\rom-local-imports\\repro\\."};
    const char *argv[3];char source[1024],reference[1024],destination[1024];
    if(!link_fixture_process_context(request,"S:\\vc1.0\\link\\rom-local-imports",0)||request->index>1||
       !link_fixture_process_environment(request,"COMSPEC","C:\\windows\\system32\\cmd.exe")||
       !link_fixture_process_environment(request,"LINK_REPRO","S:\\vc1.0\\link\\rom-local-imports\\repro")||
       !link_fixture_process_environment(request,"COPYCMD",NULL)||
       strcmp(request->file,"C:\\windows\\system32\\cmd.exe"))return 0;
    if(request->index==1&&request->pid!=parent_pid)return 0;
    argv[0]="C:\\windows\\system32\\cmd.exe";argv[1]="/c";argv[2]=commands[request->index];
    if(!link_fixture_process_argv(request,argv,3)||
       !link_fixture_process_path(request,names[request->index],source,sizeof(source))||
       !link_fixture_process_path(request,references[request->index],reference,sizeof(reference))||
       !link_fixture_process_path(request,outputs[request->index],destination,sizeof(destination))||
       !link_fixture_copy(source,reference,destination))return 0;
    if(!request->index)parent_pid=request->pid;
    memset(response,0,sizeof(*response));response->struct_size=sizeof(*response);
    response->action=RECON_PROCESS_CAPTURED; /* Both actual child streams empty; exit0. */
    return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={
    RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),2,link_case_process_request
};

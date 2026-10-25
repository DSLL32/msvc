/* Original boundary manifest SHA256 88b8cc521db5c70f5e0f95e1f140a840b5dcd8c0c015e2fe190f72fbde31df81. */
/* Actual native LINK self-child followed by the original-backed CVPACK response.
 * The response helper verifies the entire current prepack input before writing. */
#define RECON_SHIM_IMPL
#include <string.h>
#include <recon-tool.h>
#include "../../harness/fixture-process.h"
#include "case.h"
#include <recon.h>
static uint32_t parent_pid;
int link_case_process_request(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    const char *const self[]={"R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","-lib","-def:hello-debug.def",
        "-out:hello-debug.lib","-name:hello-debug.dll","-machine:ix86","-nologo",
        "hello-debug.obj","helpers.lib","resources.res"};
    const char *const pack[]={"R:\\toolchains\\1.0-nt\\bin\\cvpack.exe","/nologo","/cvnew","hello-debug.dll"};
    char target[1024],before[1024],after[1024];
    if(!link_fixture_process_context(request,"S:\\vc1.0\\link\\hello-debug-dll",1))return 0;
    memset(response,0,sizeof(*response));response->struct_size=sizeof(*response);
    if(request->index==0) {
        if(strcmp(request->file,self[0])||!link_fixture_process_argv(request,self,RECON_COUNT(self)))return 0;
        parent_pid=request->pid;response->action=RECON_PROCESS_SELF;return 1;
    }
    if(request->index!=1||request->pid!=parent_pid||strcmp(request->file,pack[0])||
       !link_fixture_process_argv(request,pack,RECON_COUNT(pack))||
       !link_fixture_process_path(request,"hello-debug.dll",target,sizeof(target))||
       !link_fixture_process_path(request,".link-cvpack-before.dll",before,sizeof(before))||
       !link_fixture_process_path(request,".link-cvpack-after.dll",after,sizeof(after)))return 0;
    if(!link_fixture_cvpack(target,before,after,cvpack_clocks,RECON_COUNT(cvpack_clocks)))return 0;
    response->action=RECON_PROCESS_CAPTURED; /* Captured exit0, both child streams empty. */
    return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={
    RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),2,link_case_process_request
};

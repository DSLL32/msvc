/* Genuine LINK self-child only. No import archive is replayed by this policy. */
#define RECON_SHIM_IMPL
#include <string.h>
#include <recon-tool.h>
#include "../../harness/fixture-process.h"
#include <recon.h>
int link_case_process_request(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    const char *const argv[]={"R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","-lib","-def:",
        "-out:hello.lib","-name:hello.dll","-machine:ix86","-nologo","hello-dll.obj"};
    if(!link_fixture_process_context(request,"S:\\vc1.0\\link\\hello-dll",1)||request->index||
       strcmp(request->file,argv[0])||!link_fixture_process_argv(request,argv,RECON_COUNT(argv)))return 0;
    memset(response,0,sizeof(*response));response->struct_size=sizeof(*response);
    response->action=RECON_PROCESS_SELF;
    return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={
    RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,link_case_process_request
};

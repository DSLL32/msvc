/* Captured copy policy controls; no command processor is executed. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <recon-tool.h>
#include "../../harness/fixture-process.h"
#include <recon.h>
extern char **environ;
extern int link_case_process_request(const ReconProcessRequest *,ReconProcessResponse *);
static int prepare(ReconProcessRequest *request,char *cwd,size_t capacity)
{
    static const char *const argv[]={"C:\\windows\\system32\\cmd.exe","/c",
        "copy provider.obj S:\\vc1.0\\link\\rom-local-imports\\repro\\."};
    unsigned count=0;if(!getcwd(cwd,capacity))return 0;while(environ[count])++count;
    memset(request,0,sizeof(*request));request->struct_size=sizeof(*request);
    request->pid=getpid();request->protocol=RECON_PROCESS_RSP1;
    request->file=argv[0];request->argv=argv;request->argc=RECON_COUNT(argv);
    request->cwd=request->workspace_cwd=cwd;request->logical_cwd="S:\\vc1.0\\link\\rom-local-imports";
    request->environment=request->prepared_environment=(const char *const *)environ;
    request->environment_count=request->prepared_environment_count=count;
    return link_fixture_process_context(request,request->logical_cwd,0)&&
        link_fixture_process_environment(request,"COMSPEC",request->file)&&
        link_fixture_process_environment(request,"LINK_REPRO","S:\\vc1.0\\link\\rom-local-imports\\repro")&&
        link_fixture_process_environment(request,"COPYCMD",NULL);
}
int test_unknown_copy_command(void)
{
    ReconProcessRequest request;ReconProcessResponse response;char cwd[1024];
    const char *const argv[]={"C:\\windows\\system32\\cmd.exe","/c","copy other.obj repro"};
    if(!prepare(&request,cwd,sizeof(cwd)))return 0;
    request.argv=argv;request.argc=RECON_COUNT(argv);
    return !link_case_process_request(&request,&response)&&access("repro/provider.obj",F_OK)!=0;
}
int test_changed_copy_input(void)
{
    ReconProcessRequest request;ReconProcessResponse response;char cwd[1024];FILE *file;int byte;
    const char *const argv[]={"C:\\windows\\system32\\cmd.exe","/c",
        "copy provider.obj S:\\vc1.0\\link\\rom-local-imports\\repro\\."};
    if(!prepare(&request,cwd,sizeof(cwd)))return 0;
    file=fopen("provider.obj","r+b");if(!file)return 0;byte=fgetc(file);
    if(byte==EOF||fseek(file,0,SEEK_SET)||fputc(byte^1,file)==EOF||fclose(file))return 0;
    request.argv=argv;request.argc=RECON_COUNT(argv);
    return !link_case_process_request(&request,&response)&&access("repro/provider.obj",F_OK)!=0;
}

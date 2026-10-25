/* Host policy controls; no external program or application body is invoked. */
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
    static const char *const argv[]={"R:\\toolchains\\1.0-nt\\bin\\LINK.EXE","-lib","-def:hello-debug.def",
        "-out:hello-debug.lib","-name:hello-debug.dll","-machine:ix86","-nologo",
        "hello-debug.obj","helpers.lib","resources.res"};
    ReconProcessResponse response;unsigned count=0;
    if(!getcwd(cwd,capacity))return 0;
    while(environ[count])++count;
    memset(request,0,sizeof(*request));request->struct_size=sizeof(*request);
    request->pid=getpid();request->protocol=RECON_PROCESS_RSP1;
    request->file=argv[0];request->cwd=request->workspace_cwd=cwd;
    request->logical_cwd="S:\\vc1.0\\link\\hello-debug-dll";
    request->argc=RECON_COUNT(argv);request->argv=argv;
    request->environment=request->prepared_environment=(const char *const *)environ;
    request->environment_count=request->prepared_environment_count=count;
    return link_case_process_request(request,&response)&&response.action==RECON_PROCESS_SELF;
}
int test_bad_cvpack_request(void)
{
    ReconProcessRequest request;ReconProcessResponse response;char cwd[1024];
    const char *const argv[]={"R:\\toolchains\\1.0-nt\\bin\\cvpack.exe","/nologo","/wrong","hello-debug.dll"};
    if(!prepare(&request,cwd,sizeof(cwd)))return 0;
    request.index=1;request.file=argv[0];request.argv=argv;request.argc=RECON_COUNT(argv);
    return !link_case_process_request(&request,&response);
}
int test_changed_cvpack_input(void)
{
    ReconProcessRequest request;ReconProcessResponse response;char cwd[1024];FILE *input,*output;int byte,changed;
    const char *const argv[]={"R:\\toolchains\\1.0-nt\\bin\\cvpack.exe","/nologo","/cvnew","hello-debug.dll"};
    if(!prepare(&request,cwd,sizeof(cwd)))return 0;
    input=fopen(".link-cvpack-before.dll","rb");output=fopen("hello-debug.dll","wb");
    if(!input||!output)return 0;
    while((byte=fgetc(input))!=EOF)if(fputc(byte,output)==EOF)return 0;
    if(fclose(input)||fclose(output))return 0;
    output=fopen("hello-debug.dll","r+b");if(!output||fseek(output,0x1000,SEEK_SET))return 0;
    byte=fgetc(output);changed=byte^1;
    if(byte==EOF||fseek(output,0x1000,SEEK_SET)||fputc(changed,output)==EOF||fclose(output))return 0;
    request.index=1;request.file=argv[0];request.argv=argv;request.argc=RECON_COUNT(argv);
    if(link_case_process_request(&request,&response))return 0;
    output=fopen("hello-debug.dll","rb");if(!output||fseek(output,0x1000,SEEK_SET))return 0;
    byte=fgetc(output);fclose(output);return byte==changed;
}

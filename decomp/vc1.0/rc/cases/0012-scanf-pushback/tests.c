/* Narrow RC boundary tests: neither a changed prepared input nor an unknown
 * executable may obtain the frozen RCPP response. No child is launched. */
#include "rcpp.h"
extern char **environ;
static int reject_changed_request(int corrupt_input)
{
    ReconProcessRequest request;
    char cwd[PATH_MAX],input[16],options[4096];
    const char *argv[2];
    unsigned count,size;
    unsigned char *bytes;
    int passed=0;
    memset(&request,0,sizeof(request));
    request.struct_size=sizeof(request);
    request.protocol=RECON_PROCESS_RSP1;
    request.pid=(unsigned)getppid();
    request.file=rc_capture.application;
    request.cwd=getcwd(cwd,sizeof(cwd));
    request.workspace_cwd=recon_case_workspace_cwd();
    request.logical_cwd=recon_case_logical_cwd();
    request.assets_root=recon_case_assets();
    for(count=0;environ[count];++count) { }
    request.environment=(const char *const *)environ;
    request.environment_count=count;
    sprintf(input,"RC%06u",request.pid%1000000);
    snprintf(options,sizeof(options)," -f %s -g RD%06u%s",input,
             request.pid%1000000,rc_capture.options_tail);
    argv[0]="RCPP.EXE";argv[1]=options;
    request.argc=2;request.argv=argv;
    bytes=rc_read_asset("rcpp/input.bin",&size);
    if(!bytes || !size || !rc_write_file(input,bytes,size) ||
       !rc_request_matches(&request))goto done;
    if(corrupt_input) {
        bytes[0]^=1;
        if(!rc_write_file(input,bytes,size))goto done;
    } else request.file="R:\\uncaptured-child\\rcpp.exe";
    passed=!rc_request_matches(&request);
done:
    free(bytes);
    remove(input);
    return passed;
}
static int changed_input(void) { return reject_changed_request(1); }
static int unknown_child(void) { return reject_changed_request(0); }
const ReconTest rc_tests[2]={
    { "reject changed prepared RCPP input",1,changed_input },
    { "reject unknown RCPP executable",1,unknown_child }
};

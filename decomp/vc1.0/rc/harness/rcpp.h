/* RC's exact captured RCPP boundary; shared tooling owns transport/isolation. */
#ifndef RC_CAPTURED_PROCESS_H
#define RC_CAPTURED_PROCESS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <recon-tool.h>
#include <recon-process.h>
#include <recon-fixture.h>
#include <recon.h>

typedef struct RcRead {
    const char *path, *asset;
    unsigned temporary;             /* 1 = RC input, 2 = RD output, 0 = literal. */
} RcRead;
typedef struct RcCapture {
    const char *application, *logical_cwd, *options_tail, *captured_input;
    const char *program, *include, *winepath, *path;
    const RcRead *reads;
    unsigned read_count;
    int status;
} RcCapture;
extern const RcCapture rc_capture;
int rc_request_matches(const ReconProcessRequest *request);
unsigned char *rc_read_asset(const char *name, unsigned *size);
int rc_write_file(const char *name, const void *data, unsigned size);

#ifdef RC_PROCESS_IMPLEMENTATION
static int rc_reject(const char *reason)
{
    fprintf(stderr,"RC captured RCPP: %s\n",reason);
    return 0;
}
static unsigned char *rc_read_file(const char *name, unsigned *size)
{
    FILE *file;
    long length;
    unsigned char *data;
    file=fopen(name,"rb");
    if(!file)return 0;
    if(fseek(file,0,SEEK_END) || (length=ftell(file))<0 ||
       (unsigned long)length>0x1000000 || fseek(file,0,SEEK_SET)) {
        fclose(file);return 0;
    }
    data=malloc(length ? (unsigned)length : 1);
    if(!data || fread(data,1,(unsigned)length,file)!=(unsigned)length) {
        free(data);fclose(file);return 0;
    }
    if(fclose(file)) { free(data);return 0; }
    *size=(unsigned)length;
    return data;
}
unsigned char *rc_read_asset(const char *name, unsigned *size)
{
    char path[PATH_MAX];
    const char *root=recon_case_assets();
    if(!root || snprintf(path,sizeof(path),"%s/%s",root,name)>=(int)sizeof(path))
        return 0;
    return rc_read_file(path,size);
}
int rc_write_file(const char *name, const void *data, unsigned size)
{
    FILE *file=fopen(name,"wb");
    int passed;
    if(!file)return 0;
    passed=fwrite(data,1,size,file)==size;
    if(fclose(file))passed=0;
    return passed;
}
static int rc_file_matches(const char *name, const char *asset)
{
    char path[PATH_MAX];
    unsigned actual_size=0,expected_size=0;
    unsigned char *actual,*expected;
    int passed;
    if(recon_host_path(name,path,sizeof(path)))return 0;
    actual=rc_read_file(path,&actual_size);
    expected=rc_read_asset(asset,&expected_size);
    passed=actual && expected && actual_size==expected_size &&
           !memcmp(actual,expected,actual_size);
    free(actual);free(expected);
    return passed;
}
static int rc_environment_matches(const ReconProcessRequest *request,
                                  const char *name, const char *expected)
{
    unsigned i,count=0,length=strlen(name);
    const char *actual=0;
    for(i=0;i<request->environment_count;++i) {
        const char *entry=request->environment[i];
        if(!strncmp(entry,name,length) && entry[length]=='=') {
            actual=entry+length+1;++count;
        }
    }
    return count<=1 && ((!actual && !expected) ||
           (actual && expected && !strcmp(actual,expected)));
}
int rc_request_matches(const ReconProcessRequest *request)
{
    char options[4096],input[16],output[16],mapped[PATH_MAX],resolved[PATH_MAX];
    unsigned i;
    const RcCapture *capture=&rc_capture;
    if(request->struct_size!=sizeof(*request) || request->index ||
       request->protocol!=RECON_PROCESS_RSP1 || request->mode ||
       request->pid!=(unsigned)getppid())
        return rc_reject("protocol, caller or request count differs");
    if(!request->cwd || !request->workspace_cwd ||
       strcmp(request->cwd,request->workspace_cwd) || !request->logical_cwd ||
       strcmp(request->logical_cwd,capture->logical_cwd) ||
       recon_host_path(capture->logical_cwd,mapped,sizeof(mapped)) ||
       !realpath(mapped,resolved) || strcmp(resolved,request->workspace_cwd))
        return rc_reject("canonical/actual cwd differs");
    if(!request->file || strcmp(request->file,capture->application) ||
       !rc_file_matches(request->file,"rcpp/original-child.bin"))
        return rc_reject("executable spelling or complete bytes differ");
    if(!rc_environment_matches(request,"INCLUDE",capture->include) ||
       !rc_environment_matches(request,"WINEPATH",capture->winepath) ||
       !rc_environment_matches(request,"RECON_PROGRAM_PATH",capture->program) ||
       !rc_environment_matches(request,"TEMP",getenv("TEMP")) ||
       !rc_environment_matches(request,"TMP",getenv("TMP")) ||
       (capture->path && !rc_environment_matches(request,"PATH",capture->path)) ||
       !rc_environment_matches(request,"CL",0) ||
       !rc_environment_matches(request,"_CL_",0) ||
       !rc_environment_matches(request,"LINK",0) ||
       !rc_environment_matches(request,"_LINK_",0) ||
       !rc_environment_matches(request,"MSC_CMD_FLAGS",0))
        return rc_reject("relevant environment differs");
    sprintf(input,"RC%06u",request->pid%1000000);
    sprintf(output,"RD%06u",request->pid%1000000);
    if(snprintf(options,sizeof(options)," -f %s -g %s%s",input,output,
                capture->options_tail)>=(int)sizeof(options) ||
       request->argc!=2 || strcmp(request->argv[0],"RCPP.EXE") ||
       strcmp(request->argv[1],options))
        return rc_reject("complete argv differs");
    if(!rc_file_matches(input,"rcpp/input.bin"))
        return rc_reject("complete prepared input differs");
    for(i=0;i<capture->read_count;++i) {
        const RcRead *read=&capture->reads[i];
        const char *name=read->temporary==1 ? input :
                         read->temporary==2 ? output : read->path;
        struct stat information;
        if(recon_host_path(name,mapped,sizeof(mapped)))
            return rc_reject("recorded input path cannot resolve");
        if(read->asset) {
            if(!rc_file_matches(name,read->asset))
                return rc_reject("complete recorded read differs");
        } else if(!stat(mapped,&information) || (errno!=ENOENT && errno!=ENOTDIR)) {
            return rc_reject("recorded absent read exists");
        }
    }
    return 1;
}
static int rc_respond(const ReconProcessRequest *request,
                      ReconProcessResponse *response)
{
    unsigned size;
    unsigned char *output;
    char input[16],name[16];
    if(!rc_request_matches(request))return 0;
    output=rc_read_asset("rcpp/response.bin",&size);
    /* Each frozen response contains exactly this one recorded filename field. */
    if(!output || size<20 || memcmp(output,"#line 1 \"",9) ||
       memcmp(output+9,rc_capture.captured_input,8) || output[17]!='"') {
        free(output);return rc_reject("recorded #line token differs");
    }
    sprintf(input,"RC%06u",request->pid%1000000);
    memcpy(output+9,input,8);
    sprintf(name,"RD%06u",request->pid%1000000);
    if(!rc_write_file(name,output,size)) {
        free(output);return rc_reject("captured output write failed");
    }
    free(output);
    response->struct_size=sizeof(*response);
    response->action=RECON_PROCESS_CAPTURED;
    response->status=rc_capture.status;
    response->failure_errno=0;
    response->stdout_data=rc_read_asset("rcpp/stdout.bin",&response->stdout_size);
    response->stderr_data=rc_read_asset("rcpp/stderr.bin",&response->stderr_size);
    return response->stdout_data && response->stderr_data;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={
    RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,rc_respond
};
#endif
#endif

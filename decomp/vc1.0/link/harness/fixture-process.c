/* The three registered LINK process contracts share these exact context checks.
 * The common harness owns transport, process isolation and native selfreentry. */
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixture-process.h"
#include <recon.h>

int link_fixture_process_environment(const ReconProcessRequest *request,const char *name,const char *expected)
{
    unsigned i,count=0;size_t length=strlen(name);
    for(i=0;i<request->environment_count;++i) {
        const char *entry=request->environment[i];
        if(!entry)return 0;
        if(!strncmp(entry,name,length)&&entry[length]=='=') {
            if(!expected||strcmp(entry+length+1,expected))return 0;
            ++count;
        }
    }
    return count==(expected?1u:0u);
}
int link_fixture_process_context(const ReconProcessRequest *request,const char *logical,int self)
{
    static const char *inherited[]={"TMP","TEMP","RECON_TRACE_FD","RECON_TRACE_FILTER","RECON_TRACE_SEEN","RECON_TRACE_STATE_FD"};
    static const char *absent[]={"CL","_CL_","LINK","_LINK_","MSC_CMD_FLAGS","LINK32_DB"};
    unsigned i,j;
    if(!request||request->struct_size!=sizeof(*request)||request->mode!=0||
       request->protocol!=RECON_PROCESS_RSP1||!request->pid||!request->cwd||
       !request->workspace_cwd||strcmp(request->cwd,request->workspace_cwd)||
       !request->logical_cwd||strcmp(request->logical_cwd,logical)||
       !request->file||!request->argv||!request->environment||
       (request->prepared_environment_count&&!request->prepared_environment))return 0;
    if(!link_fixture_process_environment(request,"LIB","R:\\toolchains\\1.0-nt\\lib")||
       !link_fixture_process_environment(request,"RECON_PROGRAM_PATH","R:\\toolchains\\1.0-nt\\bin\\LINK.EXE")||
       !link_fixture_process_environment(request,"RECON_SELF_SPAWN",self?"1":NULL))return 0;
    for(i=0;i<sizeof(inherited)/sizeof(inherited[0]);++i) {
        const char *expected=NULL;unsigned matches=0;size_t length=strlen(inherited[i]);
        for(j=0;j<request->prepared_environment_count;++j) {
            const char *entry=request->prepared_environment[j];
            if(!entry)return 0;
            if(!strncmp(entry,inherited[i],length)&&entry[length]=='=') {
                expected=entry+length+1;++matches;
            }
        }
        if(matches>1||!link_fixture_process_environment(request,inherited[i],expected))return 0;
    }
    for(i=0;i<sizeof(absent)/sizeof(absent[0]);++i)
        if(!link_fixture_process_environment(request,absent[i],NULL))return 0;
    return 1;
}
int link_fixture_process_argv(const ReconProcessRequest *request,const char *const *argv,unsigned count)
{
    unsigned i;if(request->argc!=count)return 0;
    for(i=0;i<count;++i)if(!request->argv[i]||strcmp(request->argv[i],argv[i]))return 0;
    return 1;
}
int link_fixture_process_path(const ReconProcessRequest *request,const char *relative,char *path,size_t capacity)
{
    int length=snprintf(path,capacity,"%s/%s",request->workspace_cwd,relative);
    return length>=0&&(size_t)length<capacity;
}

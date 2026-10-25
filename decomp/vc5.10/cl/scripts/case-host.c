/* Native IO adapter support; target APIs and ABI remain in src/. */
#define RECON_SHIM_IMPL
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <recon.h>

static void completed(void)
{
    const char *count=getenv("CL_EXPECTED_MKTEMP_COUNT");
    ReconMktempObservation *r=&recon_mktemp_observation;
    const char *before=getenv("RECON_MKTEMP_TEMPLATE");
    const char *after=getenv("CL_EXPECTED_MKTEMP_RESULT");
    if (!count || (strcmp(count,"0") && strcmp(count,"1")) || r->count!=(unsigned)(count[0]-'0'))
        recon_unsupported("CL case missing/extra mktemp call");
    if (r->count && (r->magic!=0x31544d52 || r->version!=1 || r->owner!=(unsigned)getpid() || !before || !after || strcmp(before,r->before) || strcmp(after,r->after) ||
        r->argument!=r->result || r->before_errno || r->after_errno ||
        r->after_doserrno!=2 || r->after_last_error!=2))
        recon_unsupported("CL case mktemp response/alias/error effect differs");
}
static void prepare_owned_c_parents(void)
{
    const char *parents=getenv("CL_C_DRIVE_PARENTS");
    char root[PATH_MAX],path[PATH_MAX],*copy,*part,*end,*cursor;
    if (!parents) return;
    if (!getcwd(root,sizeof(root)) || strlen(root)>sizeof(root)-16) recon_unsupported("CL Cdrive root");
    strcat(root,"/.drive-c");
    if (mkdir(root,0700) && errno!=EEXIST) recon_unsupported("CL Cdrive mkdir");
    copy=strdup(parents);if(!copy) recon_unsupported("CL Cdrive allocation");
    for (part=copy;part;part=end) {
        end=strchr(part,';');if(end)*end++=0;
        if (strncmp(part,"C:\\",3) || strstr(part,"..") || strchr(part+3,':') || strlen(root)+strlen(part)>=sizeof(path))
            recon_unsupported("CL unknown Cdrive parent");
        strcpy(path,root);strcat(path,"/");strcat(path,part+3);
        for(cursor=path+strlen(root)+1;*cursor;++cursor) if(*cursor=='\\')*cursor='/';
        for(cursor=path+strlen(root)+1;;++cursor) if(*cursor=='/' || !*cursor) {
            int last=!*cursor;char saved=*cursor;*cursor=0;
            if(mkdir(path,0700) && errno!=EEXIST) recon_unsupported("CL Cdrive parent mkdir");
            *cursor=saved;if(last)break;
        }
    }
    free(copy);if(setenv("RECON_DRIVE_C",root,1))recon_unsupported("CL Cdrive binding");
}
extern void cl_start_provider(void);
extern int cl_case_main(int,char **,char **);
extern char **environ;
int cl_case_run(int argc,char **argv)
{
    prepare_owned_c_parents();
    cl_start_provider();
    if (atexit(completed)) recon_unsupported("CL case completion registration");
    return cl_case_main(argc,argv,environ);
}

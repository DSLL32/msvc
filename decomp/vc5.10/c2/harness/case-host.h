#ifndef C2_CASE_HOST_H
#define C2_CASE_HOST_H
#define RECON_SHIM_IMPL
#include "c2.h"
#include "register-sets.h"
#include <recon-tool.h>
#include <recon.h>

extern int final_multiply_table_initialized;
static char **target_environment;
static int setup(void)
{
    FILE *file;
    char *bytes;
    long size;
    unsigned count,offset;
    ReconModuleSpec module;
    HMODULE instance;
    final_multiply_table_initialized = 0;
    register_set_pool_three_initialized = 0;
    register_set_pool_four_initialized = 0;
    register_set_pool_six_initialized = 0;
    register_set_pool_twelve_initialized = 0;
    register_set_pool_thirteen_initialized = 0;
    register_set_pool_fourteen_initialized = 0;
    file=fopen("target-environment.bin","rb");
    if(!file || fseek(file,0,SEEK_END))return 0;
    size=ftell(file);
    if(size<2 || size>65536 || fseek(file,0,SEEK_SET))return 0;
    bytes=malloc(size);target_environment=malloc(4097*sizeof(*target_environment));
    if(!bytes || !target_environment || fread(bytes,1,size,file)!=(unsigned long)size || fclose(file) || bytes[size-1] || bytes[size-2])return 0;
    offset=count=0;
    while(offset<(unsigned long)size-1 && bytes[offset]) {
        if(count==4096)return 0;
        target_environment[count++]=bytes+offset;
        while(offset<(unsigned long)size && bytes[offset])++offset;
        if(++offset>=(unsigned long)size)return 0;
    }
    /* An empty environment is the two terminating NULs, with no string. */
    if(offset!=(unsigned long)size-1 && !(count==0 && size==2 && offset==0))return 0;
    target_environment[count]=NULL;
    module.name="C2.EXE";module.path="R:\\toolchains\\5.10\\bin\\C2.EXE";
    module.sha256=binary_descriptor.original_sha256;
    module.flags=RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS;
    instance=recon_register_module(&module);
    if(!instance)return 0;
    recon_load_module_resources(instance,"messages.rmr");
    recon_controlfp(0x10000,0x30000);
    return 1;
}
static int invoke(int argc,char **argv)
{
    recon_crt_bind_arguments(argc,argv,target_environment);
    mainCRTStartup();
    return 0;
}
#endif

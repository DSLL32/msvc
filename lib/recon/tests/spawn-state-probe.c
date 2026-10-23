#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static void profile(const char *directory)
{
    unsigned char mb[261];unsigned short ctype[257];char path[1024];FILE *file;
    ReconCrtLocaleProfile p={provider,RECON_CRT_LOCALE_1252_EN_US,mb+4,257,ctype,257};
    snprintf(path,sizeof(path),"%s/invoke-after-mb-state.bin",directory);file=fopen(path,"rb");
    assert(file && fread(mb,1,sizeof(mb),file)==sizeof(mb));fclose(file);
    snprintf(path,sizeof(path),"%s/invoke-after-ctype.bin",directory);file=fopen(path,"rb");
    assert(file && fread(ctype,1,sizeof(ctype),file)==sizeof(ctype));fclose(file);
    recon_crt_apply_locale_profile(&p);
}
int main(int argc,char **argv)
{
    char env0[]="PATH=CRT_ONLY",env1[]="VALUE=crt-value";
    char *environment[]={env0,env1,NULL},*target_argv[]={"parent.exe",NULL};
    static const char os_text[]="PATH=OS_ONLY\0VALUE=os-value\0";
    const char *child_argv[]={"C:\\captured\\child.exe",NULL};
    const char *explicit_env[]={"EXPLICIT=only",NULL};
    unsigned n;WCHAR os[sizeof(os_text)];int result,wait_mode=0;
    unsigned char before[108],after[108];const char *mode;
    assert(argc==3);mode=argv[2];
    for(n=0;n<sizeof(os_text);++n)os[n]=(unsigned char)os_text[n];
    if(strcmp(mode,"unbound")){
        recon_crt_bind_arguments(1,target_argv,environment);
        if(strcmp(mode,"missing-os"))recon_crt_bind_os_environment(os,sizeof(os_text),provider);
        if(strcmp(mode,"locale"))profile(argv[1]);
    }
    if(!strcmp(mode,"nonascii-os"))os[1]=128;
    if(!strcmp(mode,"early-os-nul"))os[1]=os[2]=0;
    if(!strcmp(mode,"mode"))wait_mode=1;
    errno=!strcmp(mode,"errno")?1:0;recon_doserrno=!strcmp(mode,"dos")?1:2;
    recon_SetLastError(!strcmp(mode,"last")?0:2);
    memset(before,0,sizeof(before));memset(after,0,sizeof(after));
    __asm__ __volatile__("fld1\n\tfnsave %0\n\tfrstor %0":"=m"(before));
    if(!strcmp(mode,"explicit") || !strcmp(mode,"rsp1-explicit"))
        result=recon_spawnve(wait_mode,child_argv[0],child_argv,explicit_env);
    else result=recon_spawnvp(wait_mode,child_argv[0],child_argv);
    __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(after));
    assert(!memcmp(before,after,sizeof(before)));
    __asm__ __volatile__("fstp %%st(0)":::"st");
    assert(result==0);
    if(!strcmp(mode,"positive"))assert(errno==0 && recon_doserrno==0 && recon_GetLastError()==2);
    else assert(errno==0 && recon_doserrno==2);
    return 0;
}

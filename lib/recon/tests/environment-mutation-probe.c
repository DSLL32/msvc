#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static const char provider[]="e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210";
static void errors(void) { errno=2;recon_doserrno=2;recon_SetLastError(2); }
static void check_errors(void) { assert(errno==2 && recon_doserrno==2 && recon_GetLastError()==2); }
static size_t wide_block(WCHAR *out,const char *text,size_t bytes)
{
    size_t n;for(n=0;n<bytes;++n)out[n]=(unsigned char)text[n];return bytes;
}
static void apply_profile(const char *directory)
{
    unsigned char mb[261];unsigned short ctype[257];char path[1024];FILE *file;
    ReconCrtLocaleProfile profile={provider,RECON_CRT_LOCALE_1252_EN_US,mb+4,257,ctype,257};
    snprintf(path,sizeof(path),"%s/invoke-after-mb-state.bin",directory);
    file=fopen(path,"rb");assert(file && fread(mb,1,sizeof(mb),file)==sizeof(mb));fclose(file);
    snprintf(path,sizeof(path),"%s/invoke-after-ctype.bin",directory);
    file=fopen(path,"rb");assert(file && fread(ctype,1,sizeof(ctype),file)==sizeof(ctype));fclose(file);
    recon_crt_apply_locale_profile(&profile);
}
int main(int argc,char **argv)
{
    char alpha[]="Alpha=original",temp[]="TEMP=target-temp",order[]="ORDER=last";
    char *initial[]={alpha,temp,order,NULL},*target_argv[]={"target.exe",NULL};
    char insertion[]="FLAGS=one two ",*copied[3];
    WCHAR os[128],saved_os[128],expected[128];
    static const char os_text[]="OS_ONLY=one\0Other=two\0";
    static const char after_os_text[]="OS_ONLY=one\0Other=two\0FLAGS=one two \0";
    const char *mode=argc>2?argv[2]:"";size_t units,n;char **copy_vector;
    assert(argc>=2);assert(!setenv("FLAGS","host-control",1));
    units=wide_block(os,os_text,sizeof(os_text));memcpy(saved_os,os,units*sizeof(*os));
    if(!strcmp(mode,"unbound")){errors();recon_putenv("FLAGS=");return 42;}
    if(strcmp(mode,"unbound-arguments"))recon_crt_bind_arguments(1,target_argv,initial);
    recon_crt_initenv=initial;
    if(!strcmp(mode,"bad-units"))units--;
    if(!strcmp(mode,"early-nul"))os[1]=os[2]=0;
    if(!strcmp(mode,"nonascii-os"))os[1]=128;
    if(!strcmp(mode,"empty-os")){units=2;os[0]=os[1]=0;}
    errors();
    recon_crt_bind_os_environment(os,units,!strcmp(mode,"provider")?"bad":provider);
    check_errors();
    if(!strcmp(mode,"rebind"))recon_crt_bind_os_environment(os,units,provider);
    assert(recon_crt_os_environment==os && recon_crt_os_environment_units==units);
    assert(recon_crt_environment_vectors[0].vector==initial && recon_crt_environment_vectors[0].count==3);
    assert(recon_crt_environment_vectors[0].borrowed && recon_crt_environment_vectors[0].live);
    if(!strcmp(mode,"initenv"))recon_crt_initenv=NULL;
    if(!strcmp(mode,"errno"))errno=0;
    if(!strcmp(mode,"doserrno"))recon_doserrno=0;
    if(!strcmp(mode,"last-error"))recon_SetLastError(0);
    if(!strcmp(mode,"first-profile"))apply_profile(argv[1]);
    if(!strcmp(mode,"first-table"))recon_pctype['A']^=1;
    if(!strcmp(mode,"first-mb"))recon_mbctype[1]^=1;
    if(!strcmp(mode,"first-nonempty"))recon_putenv("FLAGS=value");
    if(!strcmp(mode,"existing-crt"))recon_putenv("alpha=");
    if(!strcmp(mode,"existing-os"))recon_putenv("other=");
    if(!strcmp(mode,"no-equals"))recon_putenv("FLAGS");
    if(!strcmp(mode,"empty-name"))recon_putenv("=value");
    if(!strcmp(mode,"nonascii-assignment"))recon_putenv("FL\200GS=");
    if(!strcmp(mode,"control-assignment"))recon_putenv("FLAGS=\t");
    assert(!recon_putenv("FLAGS="));check_errors();
    copy_vector=recon_crt_environment;
    assert(copy_vector!=initial && recon_crt_initenv==initial);
    for(n=0;n<3;++n){copied[n]=copy_vector[n];assert(copied[n]!=initial[n] && !strcmp(copied[n],initial[n]));}
    assert(!copy_vector[3] && recon_crt_environment_updates==1);
    assert(recon_crt_environment_vectors[1].vector==copy_vector && recon_crt_environment_vectors[1].live);
    assert(!recon_crt_environment_vectors[1].borrowed && recon_crt_environment_vectors[1].count==3);
    assert(recon_crt_os_environment==os && recon_crt_os_environment_units==units && !memcmp(os,saved_os,units*sizeof(*os)));
    if(strcmp(mode,"no-profile"))apply_profile(argv[1]);
    if(!strcmp(mode,"second-name"))strcpy(insertion,"ALT=value");
    if(!strcmp(mode,"second-empty"))strcpy(insertion,"FLAGS=");
    if(!strcmp(mode,"second-table"))recon_pctype['A']^=1;
    if(!strcmp(mode,"second-handle"))recon_lc_handle[0]=1033;
    if(!strcmp(mode,"second-vector"))recon_crt_environment=initial;
    errors();assert(!recon_putenv(insertion));check_errors();
    assert(recon_crt_environment!=copy_vector && recon_crt_environment!=initial);
    for(n=0;n<3;++n)assert(recon_crt_environment[n]==copied[n] && !strcmp(copied[n],initial[n]));
    assert(recon_crt_environment[3]!=insertion && !strcmp(recon_crt_environment[3],insertion));
    assert(!recon_crt_environment[4] && recon_crt_environment_updates==2 && recon_crt_initenv==initial);
    assert(!recon_crt_environment_vectors[1].live && recon_crt_environment_vectors[1].vector==copy_vector);
    assert(recon_crt_environment_vectors[2].live && !recon_crt_environment_vectors[2].borrowed);
    assert(recon_crt_environment_vectors[2].vector==recon_crt_environment && recon_crt_environment_vectors[2].count==4);
    units=wide_block(expected,after_os_text,sizeof(after_os_text));
    assert(recon_crt_os_environment!=os && recon_crt_os_environment_units==units);
    assert(!memcmp(recon_crt_os_environment,expected,units*sizeof(*expected)));
    assert(!memcmp(os,saved_os,sizeof(os_text)*sizeof(*os)));
    memset(insertion,'x',sizeof(insertion)-1);assert(!strcmp(recon_crt_environment[3],"FLAGS=one two "));
    assert(!strcmp(recon_host_getenv("FLAGS"),"host-control") && !getenv("Alpha"));
    if(!strcmp(mode,"third"))recon_putenv("FLAGS2=other");
    puts("two namespace transitions/initial and current aliases/owned assignment/errors exact");return 0;
}

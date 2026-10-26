#define RECON_SHIM_IMPL
#include <assert.h>
#include <ctype.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal",*dir=NULL,*prefix="RCX1";char *args[]={"RC.EXE",NULL},*env[]={"TMP=.","KEEP=target",NULL};
    char *paths[5],host[128],prefix_buffer[5]="RCX1",data[4];unsigned n,k;struct stat st;ShimFILE *files[4];unsigned char before[108],after[108];
    if(!strcmp(mode,"legacy")){char *path=recon_tempnam("legacy","old");assert(path && !strncmp(path,"legacy\\old",10));recon_free(path);return 0;}
    assert(!setenv("TMP","host-invalid-dir",1) && !setenv("RECON_MKTEMP_SUFFIX","invalid fixed suffix",1));
    if(!strcmp(mode,"missing"))env[0]="OTHER=value";
    if(!strcmp(mode,"foreign-tmp"))env[0]="TMP=elsewhere";
    if(strcmp(mode,"unbound"))recon_crt_bind_arguments(1,args,env);
    if(!strcmp(mode,"dir"))dir=".";
    if(!strcmp(mode,"prefix"))prefix="OTHER";
    if(!strcmp(mode,"null-prefix"))prefix=NULL;
    if(!strcmp(mode,"low-prefix"))prefix=(const char *)1;
    if(!strcmp(mode,"wrap-prefix"))prefix=(const char *)0xfffffffeU;
    if(!strcmp(mode,"locale"))recon_lc_handle[1]=1033;
    if(!strcmp(mode,"mb"))recon_mb_codepage=1252;
    if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
    if(strcmp(mode,"normal")){recon_tempnam(dir,prefix);return 42;}
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(n=0;n<4;++n){prefix_buffer[3]='1'+n;errno=111+n;recon_doserrno=222+n;recon_SetLastError(333+n);fp(before);
        paths[n]=recon_tempnam(NULL,prefix_buffer);fp(after);
        assert(paths[n] && strlen(paths[n])==12 && !strncmp(paths[n],".\\RCX",5) && paths[n][5]=='1'+n);
        assert(errno==111+(int)n && recon_doserrno==222+n && recon_GetLastError()==333+n && !memcmp(before,after,108));
        for(k=6;k<12;++k)assert(isalnum((unsigned char)paths[n][k]));
        for(k=0;k<n;++k)assert(strcmp(paths[n],paths[k]));
        assert(!recon_host_path(paths[n],host,sizeof(host)) && lstat(host,&st)==-1 && errno==ENOENT);
    }
    for(n=0;n<4;++n){files[n]=recon_fopen(paths[n],"w+b");assert(files[n] && recon_fwrite("ABC",1,3,files[n])==3 && !recon_fflush(files[n]));}
    paths[4]=recon_tempnam(NULL,"RCX1");assert(paths[4] && strcmp(paths[4],paths[0]));
    assert(!recon_host_path(paths[4],host,sizeof(host)) && lstat(host,&st)==-1 && errno==ENOENT);recon_free(paths[4]);
    for(n=0;n<4;++n){assert(!recon_fseek(files[n],0,SEEK_SET) && recon_fread(data,1,3,files[n])==3 && !memcmp(data,"ABC",3));
        assert(!recon_fclose(files[n]));files[n]=recon_fopen(paths[n],"r+b");assert(files[n] && !recon_fclose(files[n]));
        assert(!recon_unlink(paths[n]));recon_free(paths[n]);}
    assert(!strcmp(getenv("TMP"),"host-invalid-dir") && !strcmp(recon_getenv("TMP"),".") && recon_crt_environment==env && !strcmp(env[1],"KEEP=target"));
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}

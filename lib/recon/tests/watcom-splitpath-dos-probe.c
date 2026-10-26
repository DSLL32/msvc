#include <assert.h>
#include <recon.h>
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_SPLITPATH_WATCOM_DOS)
static void (__cdecl *split)(const char *,char *,char *,char *,char *)=_splitpath;
#else
static void (__cdecl *split)(const char *,char *,char *,char *,char *)=recon_watcom_splitpath_dos;
#endif
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal",*path="transform.asm";char outputs[4][32],long_path[261];char *out[4];unsigned row,mask,n,k,pass;
    unsigned char before[108],after[108];
    static const char *paths[]={"transform.asm","foo","abcdefghi.abcdef",".profile","a.b.c","foo.",""};
    static const char *names[]={"transfor","foo","abcdefgh","","a.b","foo",""};
    static const char *extensions[]={".asm","",".abc",".pro",".c",".",""};
    for(n=0;n<4;++n)out[n]=outputs[n]+4;
    if(!strcmp(mode,"null"))path=NULL;
    if(!strcmp(mode,"low"))path=(const char *)1;
    if(!strcmp(mode,"wrap"))path=(const char *)0xfffffffeU;
    if(!strcmp(mode,"drive"))path="C:foo.obj";
    if(!strcmp(mode,"directory"))path="out/foo.obj";
    if(!strcmp(mode,"backslash"))path="out\\foo.obj";
    if(!strcmp(mode,"high"))path="a\200.obj";
    if(!strcmp(mode,"long")){memset(long_path,'a',260);long_path[260]=0;path=long_path;}
    if(!strcmp(mode,"output-wrap"))out[2]=(char *)0xfffffffeU;
    if(!strcmp(mode,"overlap"))out[3]=out[2]+1;
    if(!strcmp(mode,"input-alias")){strcpy(outputs[0],"transform.asm");path=outputs[0];out[2]=outputs[0];}
    if(strcmp(mode,"normal")){split(path,out[0],out[1],out[2],out[3]);return 42;}
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_SPLITPATH_WATCOM_DOS)
    assert(split==recon_watcom_splitpath_dos);
    for(pass=0;pass<2;++pass){if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<7;++row)for(mask=0;mask<16;++mask){
            memset(outputs,0xa5,sizeof(outputs));for(n=0;n<4;++n)out[n]=mask&(1U<<n)?outputs[n]+4:NULL;
            errno=111+row;*recon_errno()=211+row;recon_doserrno=311+row;SetLastError(411+row);fp(before);
            split(paths[row],out[0],out[1],out[2],out[3]);fp(after);
            assert(errno==111+(int)row && *recon_errno()==211+(int)row && recon_doserrno==311+row && GetLastError()==411+row && !memcmp(before,after,108));
            for(n=0;n<4;++n){const char *expected=n<2?"":n==2?names[row]:extensions[row];unsigned end=out[n]?4+strlen(expected)+1:4;
                if(out[n])assert(!strcmp(out[n],expected));
                for(k=0;k<4;++k)assert((unsigned char)outputs[n][k]==0xa5);
                for(k=end;k<32;++k)assert((unsigned char)outputs[n][k]==0xa5);
            }
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }return 0;
#else
    assert(_splitpath==recon_splitpath);(void)paths;(void)names;(void)extensions;(void)row;(void)mask;(void)k;(void)pass;(void)before;(void)after;(void)fp;
    split(path,out[0],out[1],out[2],out[3]);return 42;
#endif
}

#define RECON_SHIM_IMPL
#include <assert.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";unsigned char line[4096],reset[4096],saved[4096],before[108],after[108],*token;
    const unsigned char *delimiters=(const unsigned char *)" \t";unsigned pass,row,n;
    unsigned char *(__cdecl *tokenize)(unsigned char *,const unsigned char *)=recon_mbstok;
    if(!strcmp(mode,"unbound")){tokenize(NULL,delimiters);return 42;}
    memset(line,0xa5,sizeof(line));strcpy((char *)line,"_DllMain@12 @1 NONAME PRIVATE DATA\t ");
    assert(tokenize(line,delimiters)==line && !strcmp((char *)line,"_DllMain@12"));
    if(!strcmp(mode,"changed-nul"))line[13]=0;
    if(!strcmp(mode,"changed-end"))line[36]='X';
    if(!strcmp(mode,"high"))line[13]=128;
    if(!strcmp(mode,"delimiter"))delimiters=(const unsigned char *)",";
    if(!strcmp(mode,"overlap")){line[11]=' ';line[12]='\t';line[13]=0;delimiters=line+11;}
    if(!strcmp(mode,"mb"))recon_mb_codepage=1252;
    if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
    if(!strcmp(mode,"width"))recon_mb_cur_max=2;
    if(!strcmp(mode,"locale"))recon_lc_handle[5]=1033;
    if(strcmp(mode,"normal")){tokenize(NULL,delimiters);return 42;}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass){if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<3;++row){
            static const unsigned starts[]={0,12,15,22,30};
            static const unsigned ends[]={11,14,21,29,34};
            static const char *words[]={"_DllMain@12","@1","NONAME","PRIVATE","DATA"};
            memset(line,0xa5,sizeof(line));strcpy((char *)line,"_DllMain@12 @1 NONAME PRIVATE DATA\t ");memcpy(saved,line,sizeof(line));
            for(n=0;n<5;++n){
                errno=111+row;recon_doserrno=222+row;recon_SetLastError(333+row);fp(before);token=tokenize(n?NULL:line,delimiters);fp(after);
                saved[ends[n]]=0;assert(token==line+starts[n] && !strcmp((char *)token,words[n]) && !memcmp(line,saved,sizeof(line)));
                assert(errno==111+(int)row && recon_doserrno==222+row && recon_GetLastError()==333+row && !memcmp(before,after,108));
            }
            assert(!tokenize(NULL,delimiters) && !tokenize(NULL,delimiters));
            strcpy((char *)line,"OLD REMAINDER");assert(tokenize(line,delimiters)==line);
            memset(reset,0xa5,sizeof(reset));strcpy((char *)reset,"NEW\tFINAL");assert(tokenize(reset,delimiters)==reset && tokenize(NULL,delimiters)==reset+4 && !tokenize(NULL,delimiters));
            assert(line[3]==0 && !strcmp((char *)line+4,"REMAINDER"));
            reset[0]=0;assert(!tokenize(reset,delimiters) && !tokenize(NULL,delimiters));
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    return 0;
#else
    (void)reset;(void)saved;(void)before;(void)after;(void)token;(void)pass;(void)row;(void)n;(void)fp;return 42;
#endif
}

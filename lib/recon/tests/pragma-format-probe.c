#define RECON_SHIM_IMPL
#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void wide(WCHAR *out,const char *text){while((*out++=(unsigned char)*text++)!=0){}}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";WCHAR format[24],output[34],*destination=output+1;unsigned char before[108],after[108];
    int values[]={0,1,1252,-1,INT_MAX,INT_MIN},row,n,len;char expected[64];
    wide(format,"#pragma code_page %d\r\n");
    if(!strcmp(mode,"legacy")){
        wide(format,"#line %d\r\n");assert(recon_swprintf(output,format,1252)==12 && output[6]=='1');
        wide(format,"#line %d ");assert(recon_swprintf(output,format,1252)==11 && output[10]==' ');return 0;
    }
    if(!strcmp(mode,"format"))format[19]='u';
    if(!strcmp(mode,"suffix"))format[20]=' ';
    if(!strcmp(mode,"extra"))format[22]=' ';
    if(!strcmp(mode,"locale"))recon_lc_handle[2]=1033;
    if(!strcmp(mode,"codepage"))recon_lc_codepage=1252;
    if(!strcmp(mode,"mb"))recon_mb_codepage=1252;
    if(!strcmp(mode,"max"))recon_mb_cur_max=2;
    if(!strcmp(mode,"null"))destination=NULL;
    if(!strcmp(mode,"overlap"))destination=format;
    if(!strcmp(mode,"output-wrap"))destination=(WCHAR *)0xfffffff0U;
    if(!strcmp(mode,"unaligned"))destination=(WCHAR *)((char *)output+1);
    if(!strcmp(mode,"input-wrap")){recon_swprintf(output,(WCHAR *)0xfffffffeU,1252);return 42;}
    if(strcmp(mode,"normal") && strcmp(mode,"edge")){recon_swprintf(destination,format,1252);return 42;}
    if(!strcmp(mode,"edge")){
        char *source=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0),*dest;
        assert(source!=MAP_FAILED);wide((WCHAR *)(source+4096-sizeof(format)),"#pragma code_page %d\r\n");assert(!mprotect(source,4096,PROT_READ) && !mprotect(source+4096,4096,PROT_NONE));
        dest=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(dest!=MAP_FAILED && !mprotect(dest+4096,4096,PROT_NONE));
        assert(recon_swprintf((WCHAR *)(dest+4096-64),(WCHAR *)(source+4096-sizeof(format)),INT_MIN)==31);
        assert(!munmap(source,8192) && !munmap(dest,8192));return 0;
    }
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(row=0;row<6;++row){memset(output,0xa5,sizeof(output));errno=111;recon_doserrno=222;recon_SetLastError(333);fp(before);
        len=recon_swprintf(destination,format,values[row]);fp(after);assert(errno==111 && recon_doserrno==222 && recon_GetLastError()==333 && !memcmp(before,after,108));
        assert(len==snprintf(expected,sizeof(expected),"#pragma code_page %d\r\n",values[row]));
        for(n=0;n<=len;++n)assert(destination[n]==(unsigned char)expected[n]);
        assert(output[0]==0xa5a5 && output[len+2]==0xa5a5);
    }
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}

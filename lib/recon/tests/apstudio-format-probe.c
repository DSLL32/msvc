#define RECON_SHIM_IMPL
#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static const WCHAR format[]={'%','c','%','s','%','c','%','s','%','c','%','d','%','c','%','c','%','c',0};
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static int call(WCHAR *out,const WCHAR *pattern,const WCHAR *name,const WCHAR *text,int line,int flag,int marker)
{ return recon_swprintf(out,pattern,marker,name,0xe002,text,0xe002,line,0xe002,flag,0xe002); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";WCHAR storage[2050],name[2049],text[2049],expected[2048],changed[19];
    WCHAR *out=storage+1;const WCHAR *pattern=format,*nptr=name,*tptr=text;unsigned row,pass,n,pos,length;int flag=-128,marker=0xe000,line=0,result;char decimal[12];
    unsigned char before[108],after[108];memset(storage,0xa5,sizeof(storage));name[0]='!';name[1]=0;text[0]=0;memcpy(changed,format,sizeof(format));
    if(!strcmp(mode,"pattern")){changed[1]='q';pattern=changed;}
    if(!strcmp(mode,"marker"))marker=0xe001;
    if(!strcmp(mode,"flag"))flag=128;
    if(!strcmp(mode,"null"))out=NULL;
    if(!strcmp(mode,"output-wrap"))out=(WCHAR *)0xfffffff0U;
    if(!strcmp(mode,"unaligned"))out=(WCHAR *)((char *)storage+1);
    if(!strcmp(mode,"overlap-format")){out=changed;pattern=changed;}
    if(!strcmp(mode,"overlap-name")){out[0]='!';out[1]=0;nptr=out;}
    if(!strcmp(mode,"null-name"))nptr=NULL;
    if(!strcmp(mode,"name-wrap"))nptr=(WCHAR *)0xfffffffeU;
    if(!strcmp(mode,"high")){text[0]=128;text[1]=0;}
    if(!strcmp(mode,"unterminated"))for(n=0;n<2048;++n)name[n]='A';
    if(!strcmp(mode,"capacity")){for(n=0;n<2041;++n)name[n]='A';name[2041]=0;}
    if(!strcmp(mode,"mb"))recon_mb_codepage=1252;
    if(!strcmp(mode,"gate"))recon_mb_codepage_gate=1;
    if(!strcmp(mode,"width"))recon_mb_cur_max=2;
    if(!strcmp(mode,"locale"))recon_lc_codepage=1252;
    if(!strcmp(mode,"handle"))recon_lc_handle[0]=1033;
    if(strcmp(mode,"normal") && strcmp(mode,"boundary") && strcmp(mode,"prewrite")){
        call(out,pattern,nptr,tptr,line,flag,marker);return 42;
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    call(out,pattern,nptr,tptr,line,flag,marker);return 42;
#else
    if(!strcmp(mode,"prewrite")){
        WCHAR *shared=(WCHAR *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);pid_t child;int status;
        assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);text[0]='A';text[1]=128;text[2]=0;child=fork();assert(child>=0);
        if(!child){call(shared,format,name,text,44,-128,0xe000);_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<2048;++n)assert(shared[n]==0xa5a5);assert(!munmap(shared,4096));return 0;
    }
    if(!strcmp(mode,"boundary")){
        for(n=0;n<2040;++n)name[n]='A';name[2040]=0;memset(storage,0xa5,sizeof(storage));
        assert(call(out,format,name,text,0,-128,0xe000)==2047 && out[2047]==0 && out[2045]==0xff80 && storage[0]==0xa5a5 && storage[2049]==0xa5a5);return 0;
    }
    for(pass=0;pass<2;++pass){if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<6;++row){
            name[0]=row?'V':'!';name[1]=0;text[0]=row?'4':0;text[1]=row?'2':0;text[2]=0;
            line=row==0?0:row==1?44:row==2?INT_MIN:row==3?INT_MAX:row==4?-1:1;flag=row&1?-127:-128;
            pos=0;expected[pos++]=0xe000;expected[pos++]=name[0];expected[pos++]=0xe002;
            if(row){expected[pos++]='4';expected[pos++]='2';}expected[pos++]=0xe002;
            length=snprintf(decimal,sizeof(decimal),"%d",line);for(n=0;n<length;++n)expected[pos++]=(unsigned char)decimal[n];
            expected[pos++]=0xe002;expected[pos++]=row&1?0xff81:0xff80;expected[pos++]=0xe002;expected[pos]=0;
            memset(storage,0xa5,sizeof(storage));errno=111+row;recon_doserrno=222+row;recon_SetLastError(333+row);fp(before);
            result=call(out,format,name,text,line,flag,0xe000);fp(after);
            assert(result==(int)pos && !memcmp(out,expected,(pos+1)*2));
            assert(errno==111+(int)row && recon_doserrno==222+row && recon_GetLastError()==333+row && !memcmp(before,after,108));
            assert(storage[0]==0xa5a5);for(n=pos+2;n<2050;++n)assert(storage[n]==0xa5a5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }return 0;
#endif
}

#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int (__cdecl *format)(char *,const char *,...)=sprintf;
#else
#ifdef sprintf
#error sprintf remap leaked outside explicit510
#endif
static int (__cdecl *format)(char *,const char *,...)=recon_sprintf;
#endif
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(void)
{
    static const int lines[]={1,0,-1,INT_MIN,INT_MAX};
    WCHAR filename[257];char expected[512],*shared,*output;const char *pattern="%ws(%d) : ";
    unsigned row,index,pass,n,mode;unsigned char before[108],after[108];int count,status;pid_t child;
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);output=shared+16;
    assert(format==recon_sprintf);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(index=0;index<5;++index)for(row=0;row<3;++row) {
            char plain[257];strcpy(plain,"diagnostic-recovery.rc");
            if(row==1)plain[0]=0;
            if(row==2){memset(plain,'n',256);plain[256]=0;}
            for(n=0;n<=strlen(plain);++n)filename[n]=(WCHAR)(unsigned char)plain[n];
            snprintf(expected,sizeof(expected),"%s(%d) : ",plain,lines[index]);memset(shared,0xa5,4096);
            errno=111+row;recon_doserrno=211+row;SetLastError(311+row);fp(before);
            count=format(output,pattern,filename,lines[index]);fp(after);
            assert(count==(int)strlen(expected) && !memcmp(output,expected,(size_t)count+1) && !memcmp(before,after,108));
            assert(errno==111+(int)row && recon_doserrno==211+row && GetLastError()==311+row);
            for(n=0;n<16;++n)assert((unsigned char)shared[n]==0xa5);
            for(n=16+count+1;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    assert(format(output,"Preprocessed File Created in: %s\n","sample.i")==(int)sizeof("Preprocessed File Created in: sample.i\n")-1 && !strcmp(output,"Preprocessed File Created in: sample.i\n"));
    assert(format(output,"def:\".\\%s%s\"","sample",".def")==(int)sizeof("def:\".\\sample.def\"")-1 && !strcmp(output,"def:\".\\sample.def\""));
    assert(format(output,"%s/%d/%x/%%","x",-7,0xab)==9 && !strcmp(output,"x/-7/ab/%"));
    assert(format(output,"%%ws")==3 && !strcmp(output,"%ws"));
#else
    (void)lines;(void)expected;(void)row;(void)index;(void)pass;(void)before;(void)after;(void)count;(void)fp;
#endif
    for(mode=0;mode<26;++mode) {
        memset(shared,0xa5,4096);child=fork();assert(child>=0);
        if(!child) {
            const WCHAR *name=filename;const char *text=pattern;char *out=output;
            filename[0]='x';filename[1]=0;
            switch(mode) {
            case 0:out=NULL;break;
            case 1:out=(char *)1;break;
            case 2:out=(char *)(UINTPTR_MAX-5);break;
            case 3:name=NULL;break;
            case 4:name=(const WCHAR *)1;break;
            case 5:name=(const WCHAR *)((char *)filename+1);break;
            case 6:name=(const WCHAR *)(UINTPTR_MAX-3);break;
            case 7:filename[0]=0x100;break;
            case 8:for(n=0;n<257;++n)filename[n]='x';break;
            case 9:out=(char *)filename;break;
            case 10:text=output;strcpy(output,pattern);break;
            case 11:recon_lc_codepage=1252;break;
            case 12:recon_mb_cur_max=2;break;
            case 13:recon_mb_codepage_gate=1;break;
            case 14:recon_lc_handle[3]=1;break;
            case 15:text="%S";break;
            case 16:text="%ls";break;
            case 17:text="%wc";break;
            case 18:text="%.2S";break;
            case 19:text="%03lc";break;
            case 20:text="%5ws";break;
            case 21:text="%w%ls";break;
            case 22:text="%ws(%u) : ";break;
            case 23:text=NULL;break;
            case 24:text=(const char *)1;break;
            case 25:text=(const char *)(UINTPTR_MAX-3);break;
            }
            format(out,text,name,1);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n) {
            unsigned char expected_byte=0xa5;
            if(mode==10 && n>=16 && n<=16+strlen(pattern))expected_byte=(unsigned char)pattern[n-16];
            assert((unsigned char)shared[n]==expected_byte);
        }
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    child=fork();assert(child>=0);if(!child){format(output,pattern,(WCHAR[]){'x',0},1);_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,4096));return 0;
}

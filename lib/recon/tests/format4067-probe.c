#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static DWORD format(char *output,DWORD capacity,const char *text,...)
{
    va_list arguments;DWORD count;
    va_start(arguments,text);count=FormatMessageA(0x400,text,0,0,output,capacity,&arguments);va_end(arguments);return count;
}
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    ReconModuleSpec spec={"target.exe","S:\\target.exe","62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec);char text[1024],expected[512],*shared,*output;
    WCHAR directive[257];unsigned n,row,pass,error,mode;int status;pid_t child;DWORD count;size_t length;
    unsigned char before[108],after[108];
    assert(argc==2 && module);recon_load_module_resources(module,argv[1]);
    FormatMessageA(0xa00,module,4067,0,text,sizeof(text),NULL);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);output=shared+16;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<4;++row)for(error=0;error<3;++error) {
            char plain[257];strcpy(plain,"#undef");
            if(row==1)plain[0]=0;
            if(row==2){memset(plain,'x',256);plain[256]=0;}
            if(row==3){plain[0]=127;plain[1]=0;}
            for(n=0;n<=strlen(plain);++n)directive[n]=(WCHAR)(unsigned char)plain[n];
            snprintf(expected,sizeof(expected),"unexpected characters following '%s' directive; newline expected\r\n",plain);
            length=strlen(expected);memset(shared,0xa5,4096);errno=111+error;recon_doserrno=211+error;SetLastError(311+error);
            fp(before);count=format(output,(DWORD)length+1,text,directive);fp(after);
            assert(count==length && !memcmp(output,expected,length+1) && !memcmp(before,after,108));
            assert(errno==111+(int)error && recon_doserrno==211+error && GetLastError()==311+error);
            for(n=0;n<16;++n)assert((unsigned char)shared[n]==0xa5);
            for(n=16+length+1;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
#else
    (void)expected;(void)row;(void)pass;(void)error;(void)count;(void)length;(void)before;(void)after;(void)fp;
#endif
    for(mode=0;mode<18;++mode) {
        memset(shared,0xa5,4096);child=fork();assert(child>=0);
        if(!child) {
            const WCHAR *name=directive;const char *pattern=text;char *out=output;DWORD capacity=2048;
            directive[0]='x';directive[1]=0;
            switch(mode) {
            case 0:capacity=1;break;
            case 1:name=NULL;break;
            case 2:name=(const WCHAR *)1;break;
            case 3:name=(const WCHAR *)(UINTPTR_MAX-3);break;
            case 4:directive[0]=0x100;break;
            case 5:for(n=0;n<257;++n)directive[n]='x';break;
            case 6:out=NULL;break;
            case 7:out=(char *)1;break;
            case 8:out=(char *)(UINTPTR_MAX-3);break;
            case 9:out=(char *)directive;break;
            case 10:pattern=output;strcpy(output,text);break;
            case 11:recon_lc_codepage=1252;break;
            case 12:recon_mb_codepage=932;break;
            case 13:recon_mb_cur_max=2;break;
            case 14:recon_mb_codepage_gate=1;break;
            case 15:recon_lc_handle[5]=1;break;
            case 16:pattern="unregistered %1!ws!";break;
            case 17:capacity=(DWORD)strlen("unexpected characters following 'x' directive; newline expected\r\n");break;
            }
            format(out,capacity,pattern,name);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n) {
            unsigned char expected_byte=0xa5;
            if(mode==10 && n>=16 && n<=16+strlen(text))expected_byte=(unsigned char)text[n-16];
            assert((unsigned char)shared[n]==expected_byte);
        }
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    child=fork();assert(child>=0);if(!child){format(output,2048,text,(WCHAR[]){'#',0});_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,4096));return 0;
}

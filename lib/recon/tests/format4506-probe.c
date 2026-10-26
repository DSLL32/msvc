#include <assert.h>
#include <limits.h>
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
    HMODULE module=recon_register_module(&spec);char text[1024],filename[260],expected[1024];
    WCHAR name[257];char *shared,*output;unsigned n,index,pass,row,mode;int status;pid_t child;
    unsigned char before[108],after[108];DWORD count;size_t length;
    static const int lines[]={4,0,-1,INT_MIN,INT_MAX};
    assert(argc==2 && module);recon_load_module_resources(module,argv[1]);
    FormatMessageA(0xa00,module,4506,0,text,sizeof(text),NULL);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);output=shared+16;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(index=0;index<5;++index)for(row=0;row<3;++row) {
            strcpy(filename,"mac-cursor-compat.rc");for(n=0;n<17;++n)name[n]=(WCHAR)"assets/large.cur"[n];
            if(row==1){filename[0]=0;name[0]=0;}
            if(row==2){memset(filename,'f',259);filename[259]=0;for(n=0;n<256;++n)name[n]='n';name[256]=0;}
            snprintf(expected,sizeof(expected),"%s (%d): warning RC4506 : scaling cursor %s down to 16 by 16\r\n",filename,lines[index],row==0?"assets/large.cur":row==1?"":"");
            if(row==2){char plain[257];memset(plain,'n',256);plain[256]=0;snprintf(expected,sizeof(expected),"%s (%d): warning RC4506 : scaling cursor %s down to 16 by 16\r\n",filename,lines[index],plain);}
            length=strlen(expected);memset(shared,0xa5,4096);errno=111+row;recon_doserrno=211+row;SetLastError(311+row);
            fp(before);count=format(output,(DWORD)length+1,text,filename,lines[index],name);fp(after);
            assert(count==length && !memcmp(output,expected,length+1) && !memcmp(before,after,108));
            assert(errno==111+(int)row && recon_doserrno==211+row && GetLastError()==311+row);
            for(n=0;n<16;++n)assert((unsigned char)shared[n]==0xa5);
            for(n=16+length+1;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
#else
    (void)expected;(void)before;(void)after;(void)count;(void)length;(void)lines;(void)index;(void)pass;(void)row;(void)fp;
#endif
    for(mode=0;mode<17;++mode) {
        memset(shared,0xa5,4096);child=fork();assert(child>=0);
        if(!child) {
            const char *file=filename;const WCHAR *wide=name;char *out=output;DWORD capacity=2048;
            strcpy(filename,"mac-cursor-compat.rc");for(n=0;n<17;++n)name[n]=(WCHAR)"assets/large.cur"[n];
            switch(mode){
            case 0:capacity=1;break;
            case 1:file=NULL;break;
            case 2:wide=NULL;break;
            case 3:file=(const char *)1;break;
            case 4:wide=(const WCHAR *)1;break;
            case 5:wide=(const WCHAR *)(UINTPTR_MAX-3);break;
            case 6:filename[3]=(char)0x80;break;
            case 7:name[3]=0x100;break;
            case 8:memset(filename,'x',sizeof(filename));break;
            case 9:for(n=0;n<257;++n)name[n]='x';break;
            case 10:out=(char *)filename;break;
            case 11:out=(char *)name;break;
            case 12:out=text;break;
            case 13:recon_lc_codepage=1252;break;
            case 14:recon_mb_codepage_gate=1;break;
            case 15:recon_mb_cur_max=2;break;
            case 16:recon_lc_handle[4]=1;break;
            }
            format(out,capacity,text,file,4,wide);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    child=fork();assert(child>=0);if(!child){format(output,2048,text,"x",4,(WCHAR[]){'x',0});_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,4096));return 0;
}

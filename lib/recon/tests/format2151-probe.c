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
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(int argc,char **argv)
{
    ReconModuleSpec spec={"target.exe","S:\\target.exe","62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec);char text[1024],filename[260],expected[2048],*shared,*output;
    WCHAR names[2][256];unsigned n,row,pass,index,id_index,error,mode;int status;pid_t child;DWORD count;size_t length;
    unsigned char before[108],after[108];static const int lines[]={36,0,-1,INT_MIN,INT_MAX};static const unsigned ids[]={0,17,65535};
    assert(argc==2 && module);recon_load_module_resources(module,argv[1]);FormatMessageA(0xa00,module,2151,0,text,sizeof(text),NULL);
    shared=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);output=shared+16;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<4;++row)for(index=0;index<5;++index)for(id_index=0;id_index<3;++id_index)for(error=0;error<3;++error) {
            char first[256],second[256];strcpy(filename,"localized-version.rc");strcpy(first,"Original localized label");strcpy(second,"Replacement localized label");
            if(row==1){filename[0]=first[0]=second[0]=0;}
            if(row==2){memset(filename,'f',259);filename[259]=0;memset(first,'a',255);first[255]=0;memset(second,'b',255);second[255]=0;}
            if(row==3){strcpy(first,"%1!ws! \"quoted\" \\");strcpy(second,"literal%3!X!");}
            for(n=0;n<=strlen(first);++n)names[0][n]=(WCHAR)(unsigned char)first[n];
            for(n=0;n<=strlen(second);++n)names[1][n]=(WCHAR)(unsigned char)second[n];
            snprintf(expected,sizeof(expected),"%s (%d): error RC2151 : cannot reuse string constants, %d(0x%X) - \"%s\" already defined. Cannot add %d(0x%X) - \"%s\".\r\n",filename,lines[index],(int)ids[id_index],ids[id_index],first,(int)ids[id_index],ids[id_index],second);
            length=strlen(expected);memset(shared,0xa5,8192);errno=111+error;recon_doserrno=211+error;SetLastError(311+error);
            snapshot(before);count=format(output,error==2?2048:(DWORD)length+1,text,filename,lines[index],ids[id_index],names[0],names[1]);snapshot(after);
            assert(count==length && !memcmp(output,expected,length+1));
            assert(errno==111+(int)error && recon_doserrno==211+error && GetLastError()==311+error);
            assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
            if(pass)assert(!memcmp(before+28,after+28,20));
            for(n=0;n<16;++n)assert((unsigned char)shared[n]==0xa5);
            for(n=16+length+1;n<8192;++n)assert((unsigned char)shared[n]==0xa5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
#else
    (void)expected;(void)lines;(void)ids;(void)row;(void)pass;(void)index;(void)id_index;(void)error;(void)before;(void)after;(void)count;(void)length;(void)snapshot;
#endif
    for(mode=0;mode<27;++mode) {
        memset(shared,0xa5,8192);child=fork();assert(child>=0);
        if(!child) {
            const char *file=filename,*pattern=text;const WCHAR *first=names[0],*second=names[1];char *out=output;DWORD capacity=2048;unsigned id=17;
            strcpy(filename,"x");names[0][0]='a';names[0][1]=0;names[1][0]='b';names[1][1]=0;
            switch(mode) {
            case 0:id=65536;break;
            case 1:id=UINT_MAX;break;
            case 2:file=NULL;break;
            case 3:filename[0]=(char)0x80;break;
            case 4:memset(filename,'x',260);break;
            case 5:first=NULL;break;
            case 6:second=NULL;break;
            case 7:names[0][0]=0x100;break;
            case 8:names[1][0]=0x100;break;
            case 9:first=(const WCHAR *)1;break;
            case 10:second=(const WCHAR *)(UINTPTR_MAX-3);break;
            case 11:for(n=0;n<256;++n)names[0][n]='x';break;
            case 12:for(n=0;n<256;++n)names[1][n]='x';break;
            case 13:out=NULL;break;
            case 14:out=(char *)(UINTPTR_MAX-3);break;
            case 15:out=filename;break;
            case 16:out=(char *)names[0];break;
            case 17:out=(char *)names[1];break;
            case 18:pattern=output;strcpy(output,text);break;
            case 19:capacity=1;break;
            case 20:capacity=(DWORD)strlen("x (36): error RC2151 : cannot reuse string constants, 17(0x11) - \"a\" already defined. Cannot add 17(0x11) - \"b\".\r\n");break;
            case 21:recon_lc_codepage=1252;break;
            case 22:recon_mb_codepage=932;break;
            case 23:recon_mb_cur_max=2;break;
            case 24:recon_mb_codepage_gate=1;break;
            case 25:recon_lc_handle[5]=1;break;
            case 26:pattern="unregistered %1 %2!d! %3!d! %3!X! %4!ws! %5!ws!";break;
            }
            format(out,capacity,pattern,file,36,id,first,second);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<8192;++n) {
            unsigned char expected_byte=0xa5;
            if(mode==18 && n>=16 && n<=16+strlen(text))expected_byte=(unsigned char)text[n-16];
            assert((unsigned char)shared[n]==expected_byte);
        }
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    child=fork();assert(child>=0);if(!child){format(output,2048,text,"x",36,17U,(WCHAR[]){'a',0},(WCHAR[]){'b',0});_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,8192));return 0;
}

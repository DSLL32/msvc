#include <assert.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static DWORD format(char *output,DWORD capacity,const char *text,...)
{
    va_list args;DWORD count;va_start(args,text);
    count=FormatMessageA(0x400,text,0,0,output,capacity,&args);va_end(args);return count;
}
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(int argc,char **argv)
{
    ReconModuleSpec spec={"rcdll.dll","R:\\rcdll.dll","bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0",RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS};
    HMODULE module=recon_register_module(&spec),other;unsigned ids[]={2104,2175,2135,20101,1003};
    char patterns[5][256],expected[1024],filename[260],name[257],*shared,*out;
    WCHAR wide[257];unsigned n,index,row,edge,pass,mode;DWORD count;size_t length;
    int lines[]={0,11,-1,INT_MIN,INT_MAX};pid_t child;int status;
    unsigned char before[108],after[108];
    assert(argc==2);recon_load_module_resources(module,argv[1]);
    for(index=0;index<5;++index)FormatMessageA(0xa00,module,ids[index],0,patterns[index],256,NULL);
    spec.name="other.dll";spec.path="R:\\other.dll";spec.sha256="62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6";spec.flags=RECON_MODULE_NO_TLS;other=recon_register_module(&spec);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);out=shared+16;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(index=0;index<5;++index)for(row=0;row<3;++row)for(edge=0;edge<5;++edge) {
            strcpy(filename,"unit.rc");strcpy(name,index==0?"CAPTION":index==1?"typed.res":"missing.bmp");
            if(row==1)filename[0]=name[0]=0;
            if(row==2){memset(filename,'f',259);filename[259]=0;memset(name,'n',256);name[256]=0;}
            for(n=0;n<=strlen(name);++n)wide[n]=(unsigned char)name[n];
            switch(index) {
            case 0:snprintf(expected,sizeof(expected),"%s (%d): error RC2104 : undefined keyword or key name: %s\r\n",filename,lines[edge],name);break;
            case 1:snprintf(expected,sizeof(expected),"%s (%d): error RC2175 : resource file %s is not in 3.00 format\r\n",filename,lines[edge],name);break;
            case 2:snprintf(expected,sizeof(expected),"%s (%d): error RC2135 : file not found: %s\r\n",filename,lines[edge],name);break;
            case 3:strcpy(expected,"RC terminated by user\r\n");break;
            default:snprintf(expected,sizeof(expected),"error count exceeds %d; stopping compilation\r\n",lines[edge]);break;
            }
            length=strlen(expected);memset(shared,0xa5,4096);errno=111;*recon_errno()=111;recon_doserrno=211;SetLastError(311);
            snapshot(before);
            if(index==3)count=format(out,length+1,patterns[index]);
            else if(index==4)count=format(out,length+1,patterns[index],lines[edge]);
            else if(index==2)count=format(out,length+1,patterns[index],filename,lines[edge],name);
            else count=format(out,length+1,patterns[index],filename,lines[edge],wide);
            snapshot(after);
            assert(count==length && !memcmp(out,expected,length+1));
            assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
            if(pass)assert(!memcmp(before+28,after+28,20));
            assert(errno==111 && recon_doserrno==211 && GetLastError()==311);
            for(n=0;n<16;++n)assert((unsigned char)shared[n]==0xa5);
            for(n=16+length+1;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
        }
        memset(shared,0xa5,4096);errno=123;recon_doserrno=234;SetLastError(345);snapshot(before);
        assert(!FormatMessageA(0xa00,module,10056,0,shared,4096,NULL));snapshot(after);
        assert(errno==123 && recon_doserrno==234 && GetLastError()==317);
        assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
        for(n=0;n<4096;++n)assert((unsigned char)shared[n]==0xa5);
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
#else
    (void)expected;(void)row;(void)edge;(void)pass;(void)count;(void)length;(void)lines;(void)before;(void)after;(void)snapshot;
#endif
    for(mode=0;mode<22;++mode) {
        memset(shared,0xa5,4096);child=fork();assert(child>=0);if(!child) {
            const char *file=filename,*pattern=patterns[0];const WCHAR *third=wide;char *output=out;DWORD cap=2048;
            strcpy(filename,"unit.rc");wide[0]='x';wide[1]=0;
            switch(mode) {
            case 0:cap=1;break;case 1:file=NULL;break;case 2:file=(const char *)(UINTPTR_MAX-1);break;
            case 3:memset(filename,'a',260);break;case 4:filename[0]=(char)128;break;
            case 5:third=NULL;break;case 6:third=(const WCHAR *)1;break;
            case 7:third=(const WCHAR *)(UINTPTR_MAX-1);break;case 8:wide[0]=128;break;
            case 9:for(n=0;n<257;++n)wide[n]='a';break;
            case 10:output=NULL;break;case 11:output=(char *)(UINTPTR_MAX-2);break;
            case 12:output=filename;break;case 13:output=(char *)wide;break;
            case 14:pattern=out;strcpy(out,patterns[0]);break;
            case 15:recon_mb_cur_max=2;break;case 16:recon_mb_codepage=932;break;
            case 17:recon_lc_codepage=1252;break;case 18:recon_lc_handle[5]=1;break;
            case 19:recon_mb_codepage_gate=1;break;
            case 20:pattern="unregistered";break;
            case 21:format(out,2048,patterns[2],filename,13,(const char *)NULL);_exit(42);
            }
            format(output,cap,pattern,file,11,third);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert((unsigned char)shared[n]==(mode==14 && n>=16 && n<=16+strlen(patterns[0])?(unsigned char)patterns[0][n-16]:0xa5));
    }
    for(mode=0;mode<6;++mode) {
        child=fork();assert(child>=0);if(!child) {
            FormatMessageA(mode==5?0xa01:0xa00,mode==0?other:module,mode==1?10057:10056,
                mode==2?0x409:0,mode==3?NULL:shared,mode==4?4095:4096,NULL);_exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    for(index=0;index<5;++index) {
        child=fork();assert(child>=0);if(!child){format(out,2048,patterns[index],"unit.rc",11,(WCHAR[]){'x',0});_exit(42);}
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
    }
    child=fork();assert(child>=0);if(!child){FormatMessageA(0xa00,module,10056,0,shared,4096,NULL);_exit(42);}
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#endif
    assert(!munmap(shared,4096));return 0;
}

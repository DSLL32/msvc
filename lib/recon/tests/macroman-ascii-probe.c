#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
static void check(const WCHAR *input,int count)
{
    unsigned char output[132],before[108],after[108];int row,n;
    for(row=0;row<3;++row){
        memset(output,0xa5,sizeof(output));errno=111*row;recon_doserrno=222*row;SetLastError(333*row);
        fp(before);assert(IsValidCodePage(10000));
        assert(WideCharToMultiByte(10000,0,input,count,NULL,0,NULL,NULL)==count);
        assert(WideCharToMultiByte(10000,0,input,count,(char *)output+1,count,NULL,NULL)==count);fp(after);
        assert(errno==111*row && recon_doserrno==222U*row && GetLastError()==333U*row && !memcmp(before,after,108));
        assert(output[0]==0xa5 && output[count+1]==0xa5);
        for(n=0;n<count;++n)assert(output[n+1]==input[n]);
    }
}
int main(int argc,char **argv)
{
    static WCHAR actual[]={'&','F','i','l','e',0,0x80};
    static WCHAR controls[]={0,1,9,31,32,126,127};
    WCHAR high[]={65,66,0x80};BOOL used;char output[16];int live=argc>1&&!strcmp(argv[1],"live");
    if(argc>1&&!live){
        const char *mode=argv[1];
        if(!strcmp(mode,"legacy")){
            assert(WideCharToMultiByte(1252,0,actual,5,output,5,NULL,NULL)==5 && !memcmp(output,"&File",5));
            assert(!MultiByteToWideChar(10000,0,"a",1,actual,1) && GetLastError()==ERROR_NOT_SUPPORTED);return 0;
        }
        if(!strcmp(mode,"prewrite")){
            char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);pid_t child;int status;
            assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);child=fork();assert(child>=0);
            if(!child){WideCharToMultiByte(10000,0,high,3,shared,3,NULL,NULL);_exit(42);}
            assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
            assert((unsigned char)shared[0]==0xa5 && (unsigned char)shared[1]==0xa5 && (unsigned char)shared[2]==0xa5);
            assert(!munmap(shared,4096));return 0;
        }
        if(!strcmp(mode,"zero")){
            unsigned char before[108],after[108];char *blocked=mmap(NULL,4096,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);int row;
            assert(blocked!=MAP_FAILED);__asm__ __volatile__("fld1":::"st");
            for(row=0;row<3;++row){
                memset(output,0xa5,sizeof(output));errno=111*row;recon_doserrno=222*row;SetLastError(333*row);fp(before);
                assert(!WideCharToMultiByte(10000,0,(WCHAR *)blocked,0,NULL,0,NULL,NULL) && GetLastError()==ERROR_INVALID_PARAMETER);
                assert(!WideCharToMultiByte(10000,0,actual,0,output,0,NULL,NULL) && GetLastError()==ERROR_INVALID_PARAMETER);
                assert(!WideCharToMultiByte(10000,0,(WCHAR *)blocked,0,blocked,0,NULL,NULL));fp(after);
                assert(errno==111*row && recon_doserrno==222U*row && !memcmp(before,after,108));
                assert((unsigned char)output[0]==0xa5 && (unsigned char)output[15]==0xa5);
            }
            __asm__ __volatile__("fstp %%st(0)":::"st");assert(!munmap(blocked,4096));return 0;
        }
        if(!strcmp(mode,"valid1252")){
            unsigned char before[108],after[108];int row;
            __asm__ __volatile__("fld1":::"st");
            for(row=0;row<3;++row){errno=111*row;recon_doserrno=222*row;SetLastError(333*row);fp(before);
                assert(IsValidCodePage(1252));fp(after);
                assert(errno==111*row && recon_doserrno==222U*row && GetLastError()==333U*row && !memcmp(before,after,108));}
            __asm__ __volatile__("fstp %%st(0)":::"st");return 0;
        }
        if(!strcmp(mode,"other-page")){IsValidCodePage(1251);return 42;}
        if(!strcmp(mode,"null"))WideCharToMultiByte(10000,0,NULL,1,NULL,0,NULL,NULL);
        else if(!strcmp(mode,"negative"))WideCharToMultiByte(10000,0,actual,-1,NULL,0,NULL,NULL);
        else if(!strcmp(mode,"flags"))WideCharToMultiByte(10000,1,actual,5,NULL,0,NULL,NULL);
        else if(!strcmp(mode,"replacement"))WideCharToMultiByte(10000,0,actual,5,NULL,0,"?",NULL);
        else if(!strcmp(mode,"used"))WideCharToMultiByte(10000,0,actual,5,NULL,0,NULL,&used);
        else if(!strcmp(mode,"short"))WideCharToMultiByte(10000,0,actual,5,output,4,NULL,NULL);
        else if(!strcmp(mode,"query-output"))WideCharToMultiByte(10000,0,actual,5,output,0,NULL,NULL);
        else if(!strcmp(mode,"write-null"))WideCharToMultiByte(10000,0,actual,5,NULL,5,NULL,NULL);
        else if(!strcmp(mode,"overlap"))WideCharToMultiByte(10000,0,actual,5,(char *)actual,5,NULL,NULL);
        else if(!strcmp(mode,"input-wrap"))WideCharToMultiByte(10000,0,(WCHAR *)0xfffffffeU,1,NULL,0,NULL,NULL);
        else if(!strcmp(mode,"output-wrap"))WideCharToMultiByte(10000,0,actual,5,(char *)0xfffffffeU,5,NULL,NULL);
        else if(!strcmp(mode,"unaligned"))WideCharToMultiByte(10000,0,(WCHAR *)((char *)actual+1),1,NULL,0,NULL,NULL);
        else if(!strcmp(mode,"high"))WideCharToMultiByte(10000,0,high,3,output,3,NULL,NULL);
        else assert(0);
        return 42;
    }
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    check(actual,5);check(actual,6);check(controls,7);
    {
        char *memory=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);WCHAR *edge;
        assert(memory!=MAP_FAILED);assert(!mprotect(memory+4096,4096,PROT_NONE));edge=(WCHAR *)(memory+4096-10);
        memcpy(edge,actual,10);assert(!mprotect(memory,4096,PROT_READ));check(edge,5);assert(!munmap(memory,8192));
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");return 0;
}

#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
int main(void)
{
    char samples[4][4178];WCHAR output[4181],legacy[4];unsigned n,row,pass,error,mode;
    unsigned char before[108],after[108],*shared;int result,status;pid_t child;size_t length;
    strcpy(samples[0],"#define PRODUCT_PART \"Resource \"");samples[1][0]=0;
    for(n=1;n<128;++n)samples[2][n-1]=(char)n;samples[2][127]=0;samples[2][128]=(char)0x80;
    memset(samples[3],'x',4176);samples[3][4176]=0;
    shared=mmap(NULL,16384,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    for(pass=0;pass<2;++pass) {
        if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<4;++row)for(error=0;error<3;++error) {
            int capacity;length=strlen(samples[row])+1;capacity=error==2?4177:(int)length;
            memset(output,0xa5,sizeof(output));errno=111+error;recon_doserrno=211+error;SetLastError(311+error);
            snapshot(before);result=MultiByteToWideChar(10000,1,samples[row],-1,output+2,capacity);snapshot(after);
            assert(result==(int)length && errno==111+(int)error && recon_doserrno==211+error && GetLastError()==311+error);
            assert(!memcmp(before,after,2) && !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2));
            if(pass)assert(!memcmp(before+28,after+28,20));
            for(n=0;n<length;++n)assert(output[2+n]==(unsigned char)samples[row][n]);
            assert(output[0]==0xa5a5 && output[1]==0xa5a5);
            for(n=(unsigned)length+2;n<4181;++n)assert(output[n]==0xa5a5);
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    {
        char *edge=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(edge!=MAP_FAILED && !mprotect(edge+4096,4096,PROT_NONE));edge[4094]='x';edge[4095]=0;
        assert(!mprotect(edge,4096,PROT_READ));assert(MultiByteToWideChar(10000,1,edge+4094,-1,legacy,2)==2 && legacy[0]=='x' && legacy[1]==0);
        assert(!munmap(edge,8192));
    }
#else
    (void)output;(void)row;(void)pass;(void)error;(void)before;(void)after;(void)result;(void)length;(void)snapshot;
    assert(!MultiByteToWideChar(10000,1,samples[0],-1,legacy,33) && GetLastError()==ERROR_NOT_SUPPORTED);
#endif
    /* Unselected query/count/flag tuples retain their old failure boundary. */
    assert(!MultiByteToWideChar(10000,1,"x",-1,NULL,0) && GetLastError()==ERROR_NOT_SUPPORTED);
    assert(!MultiByteToWideChar(10000,0,"x",1,legacy,1) && GetLastError()==ERROR_NOT_SUPPORTED);
    assert(!MultiByteToWideChar(10000,1,"x",1,legacy,1) && GetLastError()==ERROR_NOT_SUPPORTED);
    assert(!MultiByteToWideChar(10000,1,"x",-1,legacy,-1) && GetLastError()==ERROR_NOT_SUPPORTED);
    assert(MultiByteToWideChar(1252,1,"x",-1,legacy,2)==2 && legacy[0]=='x' && legacy[1]==0);
    for(mode=0;mode<15;++mode) {
        memset(shared,0xa5,16384);if(mode==8){shared[0]='a';shared[1]=0;}
        child=fork();assert(child>=0);
        if(!child) {
            const char *input="ab";WCHAR *out=(WCHAR *)shared;int capacity=3;
            switch(mode) {
            case 0:input=NULL;break;
            case 1:input=(const char *)1;break;
            case 2:input=(const char *)(UINTPTR_MAX-1);break;
            case 3:out=NULL;break;
            case 4:out=(WCHAR *)(shared+1);break;
            case 5:out=(WCHAR *)(UINTPTR_MAX-1);break;
            case 6:input="a\200";break;
            case 7:capacity=2;break;
            case 8:input=(const char *)out;break;
            case 9:capacity=4178;break;
            case 10:recon_lc_codepage=1252;break;
            case 11:recon_mb_cur_max=2;break;
            case 12:recon_mb_codepage=932;break;
            case 13:recon_mb_codepage_gate=1;break;
            case 14:recon_lc_handle[5]=1;break;
            }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
            MultiByteToWideChar(10000,1,input,-1,out,capacity);_exit(42);
#else
            assert(!MultiByteToWideChar(10000,1,input,-1,out,capacity) && GetLastError()==ERROR_NOT_SUPPORTED);_exit(0);
#endif
        }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
#else
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
#endif
        for(n=0;n<16384;++n)assert(shared[n]==(mode==8 && n<2?(n?'\0':'a'):0xa5));
    }
    assert(!munmap(shared,16384));return 0;
}

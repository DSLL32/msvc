#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
static char *(__cdecl *convert)(long,char *,int)=_ltoa;
static char *(__cdecl *floating)(double,int,char *)=_gcvt;
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    static const long values[]={0,1,8,-1,2147483647L,(-2147483647L-1),-123456789L};
    static const char *expected[]={"0","1","8","-1","2147483647","-2147483648","-123456789"};
    static const unsigned errors[][3]={{0,0,0},{2,2,0},{111,222,333}};
    unsigned index,row,n,mode;int live=argc>1;char buffer[64],*output=buffer+9;
    unsigned char before[108],after[108],*shared;pid_t child;int status;
    typedef char Width32[(sizeof(long)==4 && sizeof(int)==4)?1:-1];(void)sizeof(Width32);
    shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);assert(shared!=MAP_FAILED);memset(shared,0xa5,4096);
    for(mode=0;mode<6;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            switch(mode) {
            case 0:convert(8,(char *)shared,8);break;
            case 1:convert(8,(char *)shared,16);break;
            case 2:convert(8,NULL,10);break;
            case 3:floating(1.25,16,(char *)shared);break;
            case 4:floating(0,0,NULL);break;
            case 5:floating(1.25,16,(char *)1);break;
            }
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(n=0;n<4096;++n)assert(shared[n]==0xa5);
    }
    assert(!munmap(shared,4096));
    if(live)__asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(index=0;index<7;++index)for(row=0;row<3;++row) {
        memset(buffer,0xa5,sizeof(buffer));errno=errors[row][0];recon_doserrno=errors[row][1];recon_SetLastError(errors[row][2]);
        fp(before);assert(convert(values[index],output,10)==output);fp(after);
        assert(!strcmp(output,expected[index]) && !memcmp(before,after,108));
        assert(errno==(int)errors[row][0] && recon_doserrno==errors[row][1] && recon_GetLastError()==errors[row][2]);
        for(n=0;n<9;++n)assert((unsigned char)buffer[n]==0xa5);
        for(n=9+strlen(expected[index])+1;n<sizeof(buffer);++n)assert((unsigned char)buffer[n]==0xa5);
    }
    if(live)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    return 0;
}

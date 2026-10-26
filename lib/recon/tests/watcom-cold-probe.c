#include <process.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static int (__cdecl *compare)(const void *,const void *,size_t)=memicmp;
static char *(__cdecl *command)(char *)=getcmd;
static int (__cdecl *character)(void)=getch;
static char *(__cdecl *number)(unsigned int,char *,int)=utoa;
static char *(__cdecl *long_number)(unsigned long,char *,int)=ultoa;
static char *(__cdecl *signed_number)(int,char *,int)=itoa;
#else
#ifdef memicmp
#error Watcom memicmp alias leaked
#endif
#ifdef getcmd
#error Watcom getcmd alias leaked
#endif
#ifdef getch
#error Watcom getch alias leaked
#endif
#ifdef utoa
#error Watcom utoa alias leaked
#endif
#ifdef ultoa
#error Watcom ultoa alias leaked
#endif
#ifdef itoa
#error Watcom itoa alias leaked
#endif
static int (__cdecl *compare)(const void *,const void *,size_t)=recon_watcom_memicmp;
static char *(__cdecl *command)(char *)=recon_watcom_getcmd;
static int (__cdecl *character)(void)=recon_watcom_getch;
static char *(__cdecl *number)(unsigned int,char *,int)=recon_watcom_utoa;
static char *(__cdecl *long_number)(unsigned long,char *,int)=recon_watcom_ultoa;
#endif
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static void snapshot(unsigned char bytes[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])bytes)); }
static void numeric(void)
{
    static const unsigned values[]={0,1,64,0xabcdef,0x80000000U,0xffffffffU,500};
    static const char *hexadecimal[]={"0","1","40","abcdef","80000000","ffffffff","1f4"};
    static const char *decimal[]={"0","1","64","11259375","2147483648","4294967295","500"};
    const char **expected;unsigned radix,pass,kind;
    unsigned index,row,n;char buffer[20],*output=buffer+4;
    unsigned char before[108],after[108];
    __asm__ __volatile__("fld1\n\tfldpi":::"st");
    for(kind=0;kind<2;++kind)for(pass=0;pass<2;++pass)for(index=0;index<7;++index)for(row=0;row<3;++row) {
        if(kind && !pass)continue;
        radix=pass?10:16;expected=pass?decimal:hexadecimal;
        memset(buffer,0x5a,sizeof(buffer));errno=111+row;*recon_errno()=211+row;
        recon_doserrno=311+row;SetLastError(411+row);
        snapshot(before);
        if(kind)assert(long_number(values[index],output,radix)==output);
        else assert(number(values[index],output,radix)==output);
        snapshot(after);
        assert(!strcmp(output,expected[index]) && !memcmp(before,after,2) &&
               !memcmp(before+4,after+4,2) && !memcmp(before+8,after+8,2) &&
               !memcmp(before+28,after+28,20));
        assert(errno==111+(int)row && *recon_errno()==211+(int)row &&
               recon_doserrno==311+row && GetLastError()==411+row);
        for(n=0;n<4;++n)assert(buffer[n]==0x5a);
        for(n=4+strlen(expected[index])+1;n<sizeof(buffer);++n)assert(buffer[n]==0x5a);
    }
    __asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
}
#endif
int main(void)
{
    unsigned char *memory;unsigned n,mode;pid_t child;int status,expected;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(signed_number==recon_itoa);numeric();
#endif
    memory=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(memory!=MAP_FAILED);memset(memory,0xa5,4096);
    for(mode=0;mode<19;++mode) {
        child=fork();assert(child>=0);
        if(!child) {
            switch(mode) {
            case 0:assert(!compare(NULL,NULL,0));_exit(0);
            case 1:compare((void *)1,(void *)1,(size_t)-1);break;
            case 2:assert(!compare(memory,memory+64,4));_exit(0);
            case 3:assert(!compare("ABC","abc",3));_exit(0);
            case 4:command(NULL);break;
            case 5:command((char *)1);break;
            case 6:command((char *)memory);break;
            case 7:assert(!compare(memory,memory,0));_exit(0);
            case 8:assert(SetStdHandle(STD_INPUT_HANDLE,(HANDLE)2));character();break;
            case 9:number(0,NULL,10);break;
            case 10:number(~0U,(char *)memory,8);break;
            case 11:number(1,(char *)1,1);break;
            case 12:number(1,NULL,16);break;
            case 13:number(1,(char *)0xfffffff8U,16);break;
            case 14:long_number(0,NULL,10);break;
            case 15:long_number(~0UL,(char *)1,16);break;
            case 16:long_number(500,(char *)memory,8);break;
            case 17:long_number(1,(char *)0xfffffff8U,10);break;
            case 18:long_number(1,(char *)memory,16);break;
            }
            _exit(42);
        }
        expected=125;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
        if(mode==0 || mode==2 || mode==3 || mode==7)expected=0;
#endif
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==expected);
        for(n=0;n<4096;++n)assert(memory[n]==0xa5);
    }
    assert(!munmap(memory,4096));return 0;
}

#include <assert.h>
#include <sys/mman.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
static int (__cdecl *compare)(const void *,const void *,size_t)=memicmp;
#else
#ifdef memicmp
#error Watcom memicmp alias leaked
#endif
static int (__cdecl *compare)(const void *,const void *,size_t)=recon_watcom_memicmp;
#endif
static void fp(unsigned char state[108])
{ __asm__ __volatile__("fnsave %0\n\tfrstor %0":"=m"(*(unsigned char (*)[108])state)); }
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"normal";unsigned char a[4]={'A',0,'X',255},b[4]={'a',0,'z',0},before[108],after[108];
    unsigned folded[256],x,y,row,pass;const char *upper="ABCDEFGHIJKLMNOPQRSTUVWXYZ",*lower="abcdefghijklmnopqrstuvwxyz";
    if(!strcmp(mode,"null"))compare(NULL,a,1);
    if(!strcmp(mode,"low"))compare((void *)1,a,1);
    if(!strcmp(mode,"wrap"))compare((void *)0xfffffffeU,a,4);
    if(strcmp(mode,"normal"))return 42;
#ifndef RECON_CRT_PROFILE_WATCOM_100A
    compare(a,b,1);return 42;
#else
    for(x=0;x<256;++x)folded[x]=x;for(x=0;x<26;++x)folded[(unsigned char)upper[x]]=(unsigned char)lower[x];
    for(x=0;x<256;++x)for(y=0;y<256;++y){a[0]=x;b[0]=y;assert(compare(a,b,1)==(int)folded[x]-(int)folded[y]);}
    a[0]='A';b[0]='a';assert(compare(a,b,1)==0 && compare(a,b,2)==0 && compare(a,b,3)==-2);
    assert(compare(a+3,b+3,1)==255 && compare(b+3,a+3,1)==-255);
    for(pass=0;pass<2;++pass){if(pass)__asm__ __volatile__("fld1\n\tfldpi":::"st");
        for(row=0;row<3;++row){errno=111+row;*recon_errno()=211+row;recon_doserrno=311+row;SetLastError(411+row);fp(before);
            assert(compare(a,b,3)==-2 && compare(NULL,NULL,0)==0 && compare((void *)1,(void *)1,0)==0);fp(after);
            assert(errno==111+(int)row && *recon_errno()==211+(int)row && recon_doserrno==311+row && GetLastError()==411+row && !memcmp(before,after,108));
        }
        if(pass)__asm__ __volatile__("fstp %%st(0)\n\tfstp %%st(0)":::"st");
    }
    {
        unsigned char *memory=(unsigned char *)mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        assert(memory!=MAP_FAILED);memory[4095]='C';assert(!mprotect(memory,4096,PROT_READ) && !mprotect(memory+4096,4096,PROT_NONE));
        assert(!compare(memory+4095,"c",1));assert(!munmap(memory,8192));
    }
    return 0;
#endif
}

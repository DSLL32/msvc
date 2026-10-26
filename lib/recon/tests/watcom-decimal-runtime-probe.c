#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <recon.h>
#ifndef RECON_CRT_PROFILE_WATCOM_100A
#ifdef __ZBuf2F
#error Watcom ZBuf2F alias leaked
#endif
#ifdef _Scale10V
#error Watcom Scale10V alias leaked
#endif
#endif
static uint64_t bits_of(double value)
{uint64_t bits;memcpy(&bits,&value,8);return bits;}
static double coefficient(char *text)
{
    struct {unsigned left;double value;unsigned right;} owner;
    owner.left=0x12345678;owner.right=0x87654321;
    recon_watcom_zbuf2f(text,&owner.value);
    assert(owner.left==0x12345678 && owner.right==0x87654321);
    return owner.value;
}
int main(void)
{
    unsigned i,j;pid_t child;int status;
    unsigned char *shared=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(shared!=MAP_FAILED);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    assert(__ZBuf2F==recon_watcom_zbuf2f && _Scale10V==recon_watcom_scale10v);
    errno=111;*recon_errno()=211;recon_doserrno=311;SetLastError(411);
    assert(bits_of(coefficient("0"))==0);
    assert(bits_of(coefficient("9007199254740991"))==UINT64_C(0x433fffffffffffff));
    assert(bits_of(coefficient("9007199254740992"))==UINT64_C(0x4340000000000000));
    assert(bits_of(coefficient("72057594037927936"))==UINT64_C(0x4370000000000000));
    assert(bits_of(_Scale10V(coefficient("5"),-1))==UINT64_C(0x3fe0000000000000));
    assert(bits_of(_Scale10V(coefficient("125"),-2))==UINT64_C(0x3ff4000000000000));
    assert(bits_of(_Scale10V(coefficient("2"),0))==UINT64_C(0x4000000000000000));
    assert(bits_of(_Scale10V(0,-17))==0);
    assert(bits_of(_Scale10V(coefficient("762939453125"),-17))==bits_of(1.0/131072));
    for(i=0;i<256;++i)for(j=0;j<=7;++j) {
        uint64_t integer=i;unsigned k;char text[32],before[32];double value,expected;
        for(k=0;k<j;++k)integer*=5;
        memset(text,0x5a,sizeof(text));snprintf(text,sizeof(text),"%llu",(unsigned long long)integer);
        memcpy(before,text,sizeof(text));value=coefficient(text);
        expected=(double)i/(double)(1U<<j);
        assert(bits_of(_Scale10V(value,-(int)j))==bits_of(expected));
        assert(!memcmp(text,before,sizeof(text)));
    }
    {
        unsigned short cw0,cw1,sw0,sw1;double value,pi,one;
        __asm__ __volatile__("fld1\n\tfldpi\n\tfnstcw %0\n\tfnstsw %1":"=m"(cw0),"=m"(sw0)::"st");
        value=_Scale10V(coefficient("125"),-2);assert(value==1.25);
        __asm__ __volatile__("fnstcw %0\n\tfnstsw %1":"=m"(cw1),"=m"(sw1));
        assert(cw0==cw1 && (sw0&0x383f)==(sw1&0x383f));
        __asm__ __volatile__("fstpl %0\n\tfstpl %1":"=m"(pi),"=m"(one)::"st");
        assert(pi>3 && pi<4 && one==1);
    }
    assert(errno==111 && *recon_errno()==211 && recon_doserrno==311 && GetLastError()==411);
#endif
    for(i=0;i<24;++i) {
        memset(shared,0x77,4096);child=fork();assert(child>=0);
        if(!child) {
            char *text="5";double *output=(double *)(shared+256);double value=5;int exponent=-1;
            if(i==0)text="";if(i==1)text=".5";if(i==2)text="+5";if(i==3)text="5 ";
            if(i==4)text="123456789012345678";if(i==5)text="9007199254740993";
            if(i==6)text=NULL;if(i==7)output=NULL;
            if(i==8)output=(double *)(UINTPTR_MAX-3);if(i==9)output=(double *)(shared+257);
            if(i==10){text=(char *)shared;strcpy(text,"5");output=(double *)shared;}
            if(i==11)recon_mb_codepage_gate=1;if(i==12)recon_lc_handle[3]=1;
            if(i<13)recon_watcom_zbuf2f(text,output);
            else {
                if(i==13)value=-0.0;if(i==14)value=-5;if(i==15)value=.5;
                if(i==16){uint64_t raw=UINT64_C(0x7ff0000000000000);memcpy(&value,&raw,8);}
                if(i==17)exponent=1;if(i==18)exponent=-18;
                if(i==19)value=1; /* inexact 0.1 */
                if(i==20)value=1e17;if(i==21)recon_mb_cur_max=2;
                if(i==22)recon_mb_codepage_gate=1;if(i==23)recon_lc_handle[0]=1;
                recon_watcom_scale10v(value,exponent);
            }
            _exit(42);
        }
        assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==125);
        for(j=0;j<4096;++j)assert(shared[j]==(i==10 && j<2?(j?'\0':'5'):0x77));
    }
    assert(!munmap(shared,4096));
    return 0;
}

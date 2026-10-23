/* Explicit original/native CRT probe; assembly only observes x87 ABI state. */
#include <stdio.h>
#include <string.h>
#include <float.h>
#ifdef RECON_NATIVE_TEST
#include <recon.h>
#endif
static void snapshot(unsigned char *out)
{
#ifdef RECON_NATIVE_TEST
    __asm__ volatile("fnsave (%0); frstor (%0)" : : "r"(out) : "memory");
#else
    __asm mov eax,out
    __asm fnsave [eax]
    __asm frstor [eax]
#endif
}
static void initialize(unsigned short control,int seeded)
{
#ifdef RECON_NATIVE_TEST
    __asm__ volatile("fninit; fldcw %0" : : "m"(control));
    if(seeded) __asm__ volatile("fldz; fldz; fcompp; fld1");
#else
    __asm fninit
    __asm fldcw control
    if(seeded) {
        __asm fldz
        __asm fldz
        __asm fcompp
        __asm fld1
    }
#endif
}
int main(void)
{
    static const double values[]={0.0,-0.0,1.25,-1.25,0.1,1e-20,1e20};
    unsigned char before[108],after[108];
    unsigned cw,state,i;int result;
    FILE *file=fopen("states.bin","wb");if(!file)return 126;
    for(cw=0;cw<2;++cw)for(state=0;state<2;++state)for(i=0;i<7;++i){
        initialize((unsigned short)(cw?0x37f:0x27f),state);
        snapshot(before);
        result=printf("; %g",values[i]);
        snapshot(after);
        initialize(0x37f,0);
        if(fwrite(before,1,108,file)!=108 || fwrite(after,1,108,file)!=108 || fwrite(&result,1,4,file)!=4)return 127;
    }
    fclose(file);return 0;
}

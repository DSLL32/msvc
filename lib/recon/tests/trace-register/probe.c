#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "trace-abi.h"
void *observed_function, *observed_caller;
unsigned hook_calls, exit_calls, output_length, output_sum, entry_ecx, entry_edx;
unsigned char entry_carry;
static unsigned char before[512] __attribute__((aligned(16)));
static unsigned char after[512] __attribute__((aligned(16)));
static double number=1.25;
RECON_TRACE_REGISTER_ABI void hash_register(void)
{
    const unsigned char *text;
    unsigned length=0,sum=0, captured_ecx,captured_edx;
    unsigned char captured_carry;
    __asm__ volatile("stc\n\tmovl $0x12345678,%%ecx\n\tmovl $0x87654321,%%edx" ::: "ecx","edx","cc");
    RECON_TRACE_REGISTER_ENTRY(hash_register)
    __asm__ volatile("movl %%esi,%0\n\tmovl %%ecx,%1\n\tmovl %%edx,%2\n\tsetc %3"
        : "=m"(text),"=m"(captured_ecx),"=m"(captured_edx),"=m"(captured_carry));
    entry_ecx=captured_ecx; entry_edx=captured_edx; entry_carry=captured_carry;
    while(*text) {sum+=*text++;++length;}
    output_length=length; output_sum=sum-length;
    __asm__ volatile("movl %0,%%ecx\n\tmovl %1,%%edx" : : "m"(output_length),"m"(output_sum):"ecx","edx");
}
RECON_TRACE_REGISTER_ABI int main(void)
{
    const char *text="Register ABI";
    unsigned length,sum,wanted=0,n,iteration;
    for(n=0;text[n];++n) wanted+=(unsigned char)text[n]-1;
    errno=37;
    __asm__ volatile("fninit\n\tfldl %0\n\tfxsave %1" : :"m"(number),"m"(before):"memory");
    for(iteration=0;iteration!=2;++iteration)
    __asm__ volatile("movl %2,%%esi\n\tcall hash_register\n\tmovl %%ecx,%0\n\tmovl %%edx,%1"
         : "=m"(length),"=m"(sum):"m"(text):"eax","ecx","edx","esi","cc","memory");
    __asm__ volatile("fxsave %0\n\tfninit":"=m"(after));
    if(length!=n || sum!=wanted || entry_ecx!=0x12345678 || entry_edx!=0x87654321 || !entry_carry || errno!=37) return 1;
    if(memcmp(before,after,6) || memcmp(before+24,after+24,8) || memcmp(before+32,after+32,128) || memcmp(before+160,after+160,128)) return 2;
#ifdef HOSTILE
    if(hook_calls!=2 || exit_calls!=0 || observed_function!=(void *)hash_register || observed_caller) return 3;
#endif
    puts("register ABI preserved"); return 0;
}

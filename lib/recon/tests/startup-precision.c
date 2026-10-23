#include <stdio.h>
#include <recon.h>
int main(void)
{
    unsigned short control,status;
    __asm__ volatile("fnstcw %0; fnstsw %1" : "=m"(control),"=m"(status));
    printf("%04x %04x\n",control,status);
    return 0;
}

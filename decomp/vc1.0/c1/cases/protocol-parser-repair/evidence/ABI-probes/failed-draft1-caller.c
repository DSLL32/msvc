#include <stdio.h>
#include "pair.h"
int main(void)
{
    Pair actual;
    unsigned before, after, low, high;
    actual = return_pair(0x11223344);
    printf("C %08x %08x\n",actual.words[0],actual.words[1]);
    __asm {
        mov before,esp
        push 11223344h
        call return_pair
        mov low,eax
        mov high,edx
        mov after,esp
    }
    printf("ABI %08x %08x stack-delta %d\n",low,high,after-before);
    return actual.words[0] != 0x11223344 || actual.words[1] != 0x55667788 ||
           low != 0x11223344 || high != 0x55667788 || after != before;
}

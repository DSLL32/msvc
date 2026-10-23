#include <stdio.h>
#include "pair.h"
int main(void)
{
    Pair actual;
    unsigned before, after, low_word, high_word;
    actual = return_pair(0x11223344);
    printf("C %08x %08x\n",actual.words[0],actual.words[1]);
    __asm {
        mov before,esp
        mov eax,11223344h
        push eax
        call return_pair
        mov low_word,eax
        mov high_word,edx
        mov after,esp
    }
    printf("ABI %08x %08x stack-delta %d\n",low_word,high_word,after-before);
    return actual.words[0] != 0x11223344 || actual.words[1] != 0x55667788 ||
           low_word != 0x11223344 || high_word != 0x55667788 || after != before;
}

/* Native analysis-only direct case; linked to the reconstructed C function. Assembly checks the ABI rather than implementing
 * the recovered helper: C cannot inspect ESP or seed all nonvolatile registers. */
#include "preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <recon.h>
typedef int (__stdcall *Contains)(const char *, int);

static int check_abi(Contains function)
{
    unsigned long stack_before, stack_after, b, s, d, p, p_after;
    int result;
    const char *text = "abc";
    __asm {
        push ebx
        push esi
        push edi
        mov stack_before, esp
        mov p, ebp
        mov ebx, 12345678h
        mov esi, 23456789h
        mov edi, 3456789ah
        mov eax, 62h
        push eax
        push text
        call function
        mov result, eax
        mov stack_after, esp
        mov b, ebx
        mov s, esi
        mov d, edi
        mov p_after, ebp
        mov esp, stack_before
        pop edi
        pop esi
        pop ebx
    }
    return result == 1 && stack_before == stack_after &&
        b == 0x12345678UL && s == 0x23456789UL && d == 0x3456789aUL &&
        p == p_after;
}

int main(int argc, char **argv)
{
    Contains function;
    static const char high[] = {'a', (char)0x80, (char)0xff, 0};
    struct Sample { const char *text; int character, result; };
    struct Sample samples[10];
    unsigned i;
    (void)argc; (void)argv;
    function = rc_option_contains_character;
    samples[0].text = NULL; samples[0].character = 'x'; samples[0].result = 0;
    samples[1].text = ""; samples[1].character = 0; samples[1].result = 0;
    samples[2].text = "abc"; samples[2].character = 'a'; samples[2].result = 1;
    samples[3].text = "abc"; samples[3].character = 'b'; samples[3].result = 1;
    samples[4].text = "abc"; samples[4].character = 'c'; samples[4].result = 1;
    samples[5].text = "abc"; samples[5].character = 'z'; samples[5].result = 0;
    samples[6].text = "abc"; samples[6].character = 0; samples[6].result = 0;
    samples[7].text = high; samples[7].character = 0x180; samples[7].result = 1;
    samples[8].text = high; samples[8].character = -1; samples[8].result = 1;
    samples[9].text = high; samples[9].character = 0x100; samples[9].result = 0;
    for (i = 0; i < 10; ++i)
        if (function(samples[i].text, samples[i].character) != samples[i].result)
            return 4;
    if (!check_abi(function)) return 5;
    puts("10 examples and stdcall ABI match");
    return 0;
}

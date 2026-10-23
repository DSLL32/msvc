/* Typed memory operands exercise floating and integer instruction forms. */
static int initial = 7, increment = 5, multiplier = 3;
static double half = 0.5, factor = 2.0;
static unsigned mask = 0x480;

int typed_checksum(void)
{
    int rounded, low, high, product;
    __asm {
        fild initial
        fiadd increment
        fadd half
        fmul factor
        fistp rounded
        bsf eax, mask
        mov low, eax
        bsr ecx, mask
        mov high, ecx
        mov edx, 17
        imul edx, multiplier
        mov product, edx
    }
    return rounded + low + high + product;
}

int main(void)
{
    if (typed_checksum() != 93) return 1;
    return 0;
}

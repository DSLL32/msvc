/* Check assembler constant expressions, directives and an explicit data read. */
unsigned assembly_checksum(const unsigned *input)
{
    unsigned result;
    __asm {
        ; NOP padding and alignment retain ordinary reachable control flow.
        _emit 090h
        align 4
        even
        mov eax, 101010b
        mov ebx, LOW 1234h
        mov ecx, HIGH 1234h
        shl ecx, 8
        or ebx, ecx
        add eax, ebx
        mov edx, NOT 0fffffff0h
        add eax, edx
        mov esi, input
        add eax, DWORD PTR ds:[esi]
        jmp SHORT checksum_done
        _emit 090h
checksum_done:
        xor eax, 110b
        mov result, eax
    }
    return result;
}

int main(void)
{
    unsigned values[3] = { 7, 19, 0x123 };
    unsigned expected, actual;
    int i;
    for (i = 0; i < 3; ++i) {
        expected = (42U + 0x1234U + 15U + values[i]) ^ 6U;
        actual = assembly_checksum(&values[i]);
        if (actual != expected) return i + 1;
    }
    return 0;
}

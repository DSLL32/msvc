int broken_assembly(int value)
{
    __asm {
        mov eax, [eax + ebx * 3]
        mov eax, [ax]
        add eax,
        mov eax,, 1
    }
    return value;
}

int recovered_tail(int value)
{
    return value * 3 + 7;
}

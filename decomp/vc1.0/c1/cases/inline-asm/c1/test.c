int mix(int first, int second)
{
    int result;
    __asm {
        mov eax, first
        mov ecx, second
        add eax, ecx
        cmp eax, 10
        jle asm_done
        sub eax, 1
    asm_done:
        mov result, eax
    }
    return result;
}

int main(void)
{
    return mix(7, 6) != 12;
}

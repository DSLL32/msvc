/* A damaged inline packet-decoder fragment, followed by ordinary C. */
int anchor;
int packet_fragment(int *input)
{
    int result = 0;
    __asm {
        mov eax input
        add eax,
        mov ecx, [eax
        mov edx, dword ptr eax
        lea eax, [esp + esp * 2]
        mov ax, SEG anchor
        mov result, eax
    }
    return result;
}
int recovered_tail(int value) { return value + 9; }

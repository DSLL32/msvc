/* Legal386 disassembly inputs only. Never execute these routines. */
void remaining_integer_forms(void)
{
    __asm {
        test dword ptr [esi],12345678h
        not dword ptr [esi]
        neg dword ptr [esi]
        mul dword ptr [esi]
        imul dword ptr [esi]
        div dword ptr [esi]
        idiv dword ptr [esi]
        mov al,ds:[12345678h]
        mov ds:[12345678h],al
        xchg [esi],eax
        bt [esi],eax
        bt dword ptr [esi],5
        bts dword ptr [esi],6
        btr dword ptr [esi],7
        btc dword ptr [esi],8
        imul eax,[esi],12345678h
        imul eax,[esi],7
        enter 20h,2
        int 3
        call 1234h:12345678h
        jmp 1234h:12345678h
    }
}

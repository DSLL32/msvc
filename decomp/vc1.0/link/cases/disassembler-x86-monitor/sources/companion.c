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
        /* VC1 rejects selector:offset inline syntax (C2415/C2400).
         * Fixture-only legal386 CALL/JMP ptr16:32, selector1234/offset12345678.
         * These7-byte encodings are disassembly inputs, never target code. */
        _emit 09ah
        _emit 078h
        _emit 056h
        _emit 034h
        _emit 012h
        _emit 034h
        _emit 012h
        _emit 0eah
        _emit 078h
        _emit 056h
        _emit 034h
        _emit 012h
        _emit 034h
        _emit 012h
    }
}

/* Original-toolchain fixture input; never reconstruction implementation.
 * Exercise representative i386 operand/addressing and x87 decoder paths. */
void memory_forms(void)
{
    __asm {
        mov eax,dword ptr [ebx+ecx*4+12]
        mov dword ptr [esp+8],eax
        mov edx,dword ptr [ebp-16]
        lea edi,[esi+edx*2-4]
        mov al,byte ptr [eax]
        mov ax,word ptr [eax+2]
        mov word ptr [ebx],dx
        mov eax,dword ptr ds:[12345678h]
        /* The32-bit compiler rejects16-bit address syntax. These fixture
         * input bytes encode address/operand overrides + MOV AX,[BX+SI+4]. */
        _emit 0x67
        _emit 0x66
        _emit 0x8b
        _emit 0x40
        _emit 0x04
    }
}
void branches_and_immediates(void)
{
    __asm {
        cmp eax,7
        jne nonzero
        add eax,12345678h
        jmp done
nonzero:
        xor edx,edx
        test ecx,ecx
        jz done
        loop nonzero
        call memory_forms
        sub esp,12
        add esp,12
done:
        nop
    }
}
void prefixes_and_shifts(void)
{
    __asm {
        lock add dword ptr [ebx],1
        rep movsb
        repne scasb
        mov eax,fs:[0]
        xchg eax,ecx
        ror eax,3
        shld eax,edx,4
        pushfd
        popfd
    }
}
void floating_forms(void)
{
    __asm {
        fld qword ptr [eax]
        fld dword ptr [ebx+4]
        fadd st,st(1)
        fmul qword ptr [esi+8]
        fstp dword ptr [edi]
        fild dword ptr [ecx]
        fistp dword ptr [edx]
        fld1
        fldz
        fchs
        fabs
        fsqrt
        fxch st(1)
        fcompp
        fnstsw ax
        fwait
        fninit
    }
}

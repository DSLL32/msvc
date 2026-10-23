/* Disassembly-only diagnostic-monitor corpus. Never execute these routines.
 * The ordinary inline assembler supplies the instruction bytes. Privileged
 * operations are legal encodings whose runtime preconditions are not exercised.
 */
void registers_and_groups(void)
{
    __asm {
        mov al,11h
        mov cl,22h
        mov dl,33h
        mov bl,44h
        mov ah,55h
        mov ch,66h
        mov dh,77h
        mov bh,88h
        mov [ebx],cl
        mov [ebx+1],dl
        mov [ebx+2],bh
        add al,3
        or al,4
        adc al,5
        sbb al,6
        and al,7
        sub al,8
        xor al,9
        cmp al,10
        add byte ptr [esi],3
        or byte ptr [esi],4
        adc byte ptr [esi],5
        sbb byte ptr [esi],6
        and byte ptr [esi],7
        sub byte ptr [esi],8
        xor byte ptr [esi],9
        cmp byte ptr [esi],10
        rol byte ptr [esi],1
        ror byte ptr [esi],cl
        rcl byte ptr [esi],3
        rcr word ptr [esi],1
        shl byte ptr [esi],cl
        shr byte ptr [esi],2
        sar byte ptr [esi],cl
        shld eax,edx,cl
        shrd eax,edx,cl
        not byte ptr [esi]
        neg byte ptr [esi]
        mul byte ptr [esi]
        imul byte ptr [esi]
        div byte ptr [esi]
        idiv byte ptr [esi]
        inc byte ptr [esi]
        dec byte ptr [esi]
        inc eax
        inc ecx
        inc edx
        inc ebx
        inc esp
        inc ebp
        inc esi
        inc edi
        dec eax
        dec ecx
        dec edx
        dec ebx
        dec esp
        dec ebp
        dec esi
        dec edi
        push edx
        push esp
        pop esp
        pop edx
        pusha
        popa
        aaa
        aas
        aam
        aad
        daa
        das
        cbw
        cwd
        lahf
        sahf
        xlat
    }
}

void selectors_and_system(void)
{
    __asm {
        push es
        push cs
        push ss
        push ds
        push fs
        push gs
        pop gs
        pop fs
        pop ds
        pop ss
        pop es
        mov ax,es
        mov ax,cs
        mov ax,ss
        mov ax,ds
        mov ax,fs
        mov ax,gs
        mov es,ax
        mov ss,ax
        mov ds,ax
        mov fs,ax
        mov gs,ax
        mov eax,es:[ebx]
        mov eax,cs:[ebx]
        mov eax,ss:[ebx]
        mov eax,ds:[ebx]
        mov eax,fs:[ebx]
        mov eax,gs:[ebx]
        arpl ax,bx
        bound eax,[ebx]
        les eax,[ebx]
        lds eax,[ebx]
        lss eax,[ebx]
        lfs eax,[ebx]
        lgs eax,[ebx]
        lar eax,ax
        lsl eax,ax
        sldt ax
        str ax
        lldt ax
        ltr ax
        verr ax
        verw ax
        sgdt [ebx]
        sidt [ebx]
        lgdt [ebx]
        lidt [ebx]
        smsw ax
        lmsw ax
        clts
        mov eax,cr0
        mov eax,cr2
        mov eax,cr3
        mov cr0,eax
        mov cr2,eax
        mov cr3,eax
        mov eax,dr0
        mov eax,dr1
        mov eax,dr2
        mov eax,dr3
        mov eax,dr6
        mov eax,dr7
        mov dr0,eax
        mov dr1,eax
        mov dr2,eax
        mov dr3,eax
        mov dr6,eax
        mov dr7,eax
        mov eax,tr6
        mov eax,tr7
        mov tr6,eax
        mov tr7,eax
        cli
        sti
        hlt
        int 20h
        into
        iret
    }
}

void ports_strings_and_far(void)
{
    __asm {
        in al,80h
        in ax,80h
        in eax,80h
        out 80h,al
        out 80h,ax
        out 80h,eax
        in al,dx
        in ax,dx
        in eax,dx
        out dx,al
        out dx,ax
        out dx,eax
        insb
        insw
        insd
        outsb
        outsw
        outsd
        movsb
        movsw
        movsd
        cmpsb
        cmpsw
        cmpsd
        scasb
        scasw
        scasd
        lodsb
        lodsw
        lodsd
        stosb
        stosw
        stosd
        rep movsw
        repne scasb
        call fword ptr [esi]
        jmp fword ptr [esi]
        retf 4
    }
}

void coprocessor_forms(void)
{
    __asm {
        fninit
        fclex
        fnstsw ax
        fnstsw word ptr [esi]
        fnstcw word ptr [esi]
        fldcw word ptr [esi]
        fldenv [esi]
        fnstenv [esi]
        frstor [esi]
        fnsave [esi]
        fld tbyte ptr [esi]
        fstp tbyte ptr [esi]
        fild word ptr [esi]
        fist word ptr [esi]
        fistp word ptr [esi]
        fild dword ptr [esi]
        fist dword ptr [esi]
        fistp dword ptr [esi]
        fild qword ptr [esi]
        fistp qword ptr [esi]
        fbld tbyte ptr [esi]
        fbstp tbyte ptr [esi]
        fiadd word ptr [esi]
        fimul word ptr [esi]
        ficom word ptr [esi]
        ficomp word ptr [esi]
        fisub word ptr [esi]
        fisubr word ptr [esi]
        fidiv word ptr [esi]
        fidivr word ptr [esi]
        fadd st(0),st(1)
        fadd st(1),st(0)
        faddp st(1),st(0)
        fmul st(0),st(1)
        fmulp st(1),st(0)
        fsub st(0),st(1)
        fsubr st(0),st(1)
        fsubp st(1),st(0)
        fsubrp st(1),st(0)
        fdiv st(0),st(1)
        fdivr st(0),st(1)
        fdivp st(1),st(0)
        fdivrp st(1),st(0)
        fcom st(1)
        fcomp st(1)
        fcompp
        fucom st(1)
        fucomp st(1)
        fucompp
        ffree st(1)
        fst st(1)
        fstp st(1)
        fld st(1)
        fxch st(1)
    }
}

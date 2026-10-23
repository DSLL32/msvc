/* Authored instruction-transfer fixture, not reconstructed toolchain source.
 * Build as a real no-CRT PE, then retain its initialized virtual image as CF.
 * The controller must stop before probe_unknown writes its marker.
 */
typedef void (__cdecl *ProbeRoutine)(void);

void __cdecl probe_leaf(void);
void __cdecl probe_unknown(void);

volatile unsigned long probe_marker = 0;
ProbeRoutine volatile probe_pointer = probe_leaf;

void __cdecl probe_leaf(void)
{
    ++probe_marker;
}

void __cdecl probe_unknown(void)
{
    probe_marker = 0xdeadbeefUL;
}

/* Naked helpers are used only to author exact near JMP/forged-RET probes.
 * Ordinary C does not specify a tail JMP or permit rewriting its return slot.
 * These helpers are capture-tool fixtures, never reconstructed application C.
 */
__declspec(naked) void __cdecl probe_jump_direct(void)
{
    __asm jmp probe_leaf
}

__declspec(naked) void __cdecl probe_jump_indirect(void)
{
    __asm jmp dword ptr [probe_pointer]
}

__declspec(naked) void __cdecl probe_forged_return(void)
{
    __asm {
        mov eax, offset probe_unknown
        mov dword ptr [esp], eax
        ret
    }
}

void __cdecl probe_prefix(void)
{
    probe_leaf();
    probe_pointer();
    probe_jump_direct();
    probe_jump_indirect();
    probe_unknown();
    for(;;) {}
}

void __cdecl probe_bad_return_prefix(void)
{
    probe_forged_return();
    probe_unknown();
    for(;;) {}
}

/* Public end anchor permits exact last-owner bounds from the actual MAP. */
void __cdecl probe_payload_end(void)
{
}

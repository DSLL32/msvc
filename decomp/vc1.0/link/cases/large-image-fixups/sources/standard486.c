/* Fixture input only, never executed. VC1 /G4 assembles CMPXCHG to obsolete
 * 0F06/0F07; use standard486 0FB0/0FB1 encodings explicitly for the dumper.
 * These are ordinary legal instructions, not reconstruction implementation. */
void cmpb(void)
{
    __asm {
        _emit 0x0f
        _emit 0xb0
        _emit 0x0e
    }
}
void cmpd(void)
{
    __asm {
        _emit 0x0f
        _emit 0xb1
        _emit 0x06
    }
}

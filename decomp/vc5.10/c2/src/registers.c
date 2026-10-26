#include "registers.h"
#include "function-records.h"
#include <recon.h>

/* The 75 flags have original zero initializers. Implicit zero storage in the
 * pending fields represents unavailable backing, not original data defaults.
 * Row zero also has the encountered original DWORD number initializer zero.
 */
/* Row one has its self pointer, number and bit range; rows two/three have only
 * their encountered numbers 2/3 (original DWORDs at 494814/494864).
 * Rows one/two/three have their encountered kind1 bytes; rows two/three
 * additionally have canonical self identities; row 66 has its
 * canonical identity, number and exact flag bit range. Row2 now has its
 * 32-bit range and row18 its canonical row2 identity and 8-bit range.
 * Rows1/2/3 have parameter-number WORDs 0/1/2; unmodeled range zeros
 * in row3 are only reserved backing, not original default claims.
 * Zeros in reserved backing retain the existing storage representation;
 * they do not assert original contents/defaults for any unknown payload. */
C2MachineRegisterStorage machine_register_storage = { {
    /* Row0: self root, width0 and NULL variant; a32-bit lookup falls back to root. */
    { {0}, 0, 0, {0}, (C2FunctionRegister *)&machine_registers[0], {{{0}, 0}}, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[1], {{{0}, 1}}, 32, 0, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[2], {{{0}, 2}}, 32, 0, 1 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[3], {{{0}, 3}}, 32, 0, 2 },
    /* Row4 has kind1, number4, its32-bit self view and the reached
     * x86 encodingWORD3 at original4948c0. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[4], {{{0}, 4}}, 32, 0, 3 },
    /* Row5 has kind1, canonical self,32-bit range and the reached encoding
     * WORD4 at494910. Its number and other payloads remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[5], {{{0}}}, 32, 0, 4 },
    /* Row6 has its number6, exact32-bit self view and encountered x86
     * encodingWORD5 (original494960). Other payload remains unavailable. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[6], {{{0}, 6}}, 32, 0, 5 },
    /* Row7 has its number, exact32-bit self view and the now-consumed
     * original kind1 byte and reached encodingWORD6. Other payloads remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[7], {{{0}, 7}}, 32, 0, 6 },
    /* Row8 has its encountered kind1, number and32-bit self view; rows9..17 remain unmodeled. */
    /* Encountered x86 encoding WORD at +40; neighboring fields remain partial. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[8], {{{0}, 8}}, 32, 0 , 7 },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} },
    /* Row18 is the kind1 low-byte alias of root row2; number remains unknown. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[2], {{{0}, 0}}, 8, 0 },
    /* Rows19..30 remain unmodeled backing. */
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    /* Rows31/32 have only the DWORD number column consumed by float cleanup. */
    { {0}, 0, 0, {0}, 0, {{{0}, 31}} },
    { {0}, 0, 0, {0}, 0, {{{0}, 32}} },
    /* Rows33..65 remain unmodeled backing. */
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} },
    /* Row66 is the kind1 flag-register root. Its definition pointer at+20
     * has the actual initialized NULL value; other rows' definition payloads
     * remain UNMODELED despite the reserved zero backing. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 66}}, 4, 0 },
    /* Row67: kind1 at495c4c, canonical row66 at495c50 and the reached
     * three-bit range at495c68/6c. Other payload remains UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}}}, 3, 0 }
    },
    /* Original495ecc is kind2;495ed0 points back to495ec8. The padding
     * remains UNMODELED; no76th row or range payload is introduced. */
    { {0}, 2, {0}, (C2FunctionRegister *)&machine_register_storage.sentinel }
};

void reset_register_marks(void)
{
    C2MachineRegister *reg;

    for (reg = machine_registers;
         reg < machine_registers + C2_MACHINE_REGISTER_COUNT; ++reg)
        reg->flags &= ~0x10;
}

C2FunctionRegister *__fastcall new_bitfield_register(C2FunctionRegister *source,
    unsigned bits, long displacement)
{
    C2FunctionRegister *root, *view;

    /* Row0 has its self pointer, width0 and NULL variant chain;
     * rows1/2/3/4/6/7/8 have exact32-bit self views.
     * Other views are valid original inputs; their chain data remains unknown. */
    if ((source != (C2FunctionRegister *)&machine_registers[0] &&
         source != (C2FunctionRegister *)&machine_registers[1] &&
         source != (C2FunctionRegister *)&machine_registers[2] &&
         source != (C2FunctionRegister *)&machine_registers[3] &&
         source != (C2FunctionRegister *)&machine_registers[4] &&
         source != (C2FunctionRegister *)&machine_registers[6] &&
         source != (C2FunctionRegister *)&machine_registers[7] &&
         source != (C2FunctionRegister *)&machine_registers[8]) ||
        bits != 32 || displacement != 0) { __asm { int 3 } }
    root = source->canonical;
    for (view = root; view; view = view->variant_next)
        if (view->value_bytes == bits && view->displacement == displacement)
            return view;
    return root;
}

void __fastcall mark_spill_register_used(unsigned number)
{
    if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
    machine_registers[number].flags |= 0x10;
}

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
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[1], {{{(C2FunctionRegister *)&machine_registers[9], {0}}, 1}}, 32, 0, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[2], {{{0}, 2}}, 32, 0, 1 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[3], {{{(C2FunctionRegister *)&machine_registers[11], {0}}, 3}}, 32, 0, 2 },
    /* Row4 has kind1, number4, its32-bit self view and the reached
     * x86 encodingWORD3 at original4948c0. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[4], {{{(C2FunctionRegister *)&machine_registers[12], {0}}, 4}}, 32, 0, 3 },
    /* Row5 has kind1, canonical self,32-bit range and the reached encoding
     * WORD4 at494910 and reached DWORD number5 at494904. Other payloads remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[5], {{{0}, 5}}, 32, 0, 4 },
    /* Row6 has its number6, exact32-bit self view and encountered x86
     * encodingWORD5 (original494960). Other payload remains unavailable. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[6], {{{0}, 6}}, 32, 0, 5 },
    /* Row7 has its number, exact32-bit self view and the now-consumed
     * original kind1 byte and reached encodingWORD6. Other payloads remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[7], {{{0}, 7}}, 32, 0, 6 },
    /* Row8 has its encountered kind1, number and32-bit self view; rows9/10 and12..17 remain unmodeled. */
    /* Encountered x86 encoding WORD at +40; neighboring fields remain partial. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[8], {{{0}, 8}}, 32, 0 , 7 },
    /* Row9: kind1, row17 link,16-bit/displacement0 range, canonical root1
     * and number9. FNSTSW AX additionally consumes encodingWORD0 at494a50.
     * Only these consumed fields are modeled. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[1], {{{(C2FunctionRegister *)&machine_registers[17], {0}}, 9}}, 16, 0 },
    { {0} },
    /* Row11 contributes only width16/displacement0 and its row19 link to
     * the reached root3 low-byte lookup. Its other payload is UNMODELED. */
    { {0}, 0, 0, {0}, 0, {{{(C2FunctionRegister *)&machine_registers[19], {0}}, 0}}, 16, 0 },
    /* Row12 supplies only its row20 link and16-bit/displacement0 range
     * for the reached root4 low-byte lookup. */
    { {0}, 0, 0, {0}, 0, {{{(C2FunctionRegister *)&machine_registers[20], {0}}, 0}}, 16, 0 },
    { {0} }, { {0} }, { {0} }, { {0} },
    /* Row17 has its reached kind1/root1 identity, number17 and eight-bit
     * range and reached x86 encoding WORD0. Its next link/other payload remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[1], {{{0}, 17}}, 8, 0, 0 },
    /* Row18 is the kind1 low-byte alias of root row2; number remains unknown. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[2], {{{0}, 0}}, 8, 0 },
    /* Row19 has its eight-bit range, root3 identity and reached x86
     * encoding WORD2 at+40. Kind/number/next and other payload remain UNMODELED. */
    { {0}, 0, 0, {0}, (C2FunctionRegister *)&machine_registers[3], {{{0}, 0}}, 8, 0, 2 },
    /* Row20 has its reached kind1/root4 identity, number20 and eight-bit
     * range and reached x86 encoding WORD3. Next/other payload and rows21..30 remain unavailable. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[4], {{{0}, 20}}, 8, 0, 3 },
    /* Row21 is the reached AH view: canonical row1, number21, width8,
     * displacement8 and encodingWORD4. These exact range DWORDs are consumed
     * by AX/AH scheduling overlap; the other reserved fields remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[1], {{{0}, 21}}, 8, 8, 4 },
    { {0} }, { {0} }, { {0} },
    { {0} }, { {0} }, { {0} }, { {0} }, { {0} }, { {0} },
    /* Row31 has the consumed number31 and canonical self identity. Its
     * kind1,80-bit range and encodingWORD0 are now modeled; other payload is unavailable.
     * Row32 likewise has kind1, canonical self and number32. Encoding WORDs
     * for rows32..35 are1..4, the actual x87 stack-register code column.
     * Their intervening range fields remain unmodeled storage representation. */
    /* Original495130 WORD0 is the reached x87 encoding. The reached scheduler range
     * is80 bits at displacement0 (original495128/49512c). Other payload remains unavailable. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[31], {{{0}, 31}}, 80, 0, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[32], {{{0}, 32}}, 0, 0, 1 },
    /* Rows33/34/35 have kind1, numbers33/34/35 and canonical self identities.
     * Their encoding WORDs are modeled; other fields and rows36..65 remain unmodeled backing. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[33], {{{0}, 33}}, 0, 0, 2 }, { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[34], {{{0}, 34}}, 0, 0, 3 }, { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[35], {{{0}, 35}}, 0, 0, 4 }, { {0} }, { {0} }, { {0} },
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
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 67}}, 3, 0 },
    /* Rows68..73 have the reached kind1 flag views, common row66 canonical
     * identity, number and exact bit range. Other payloads remain UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 68}}, 2, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 69}}, 2, 2 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 70}}, 1, 0 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 71}}, 1, 1 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 72}}, 1, 2 },
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[66], {{{0}, 73}}, 1, 3 },
    /* Row74 has kind1, canonical self and the encountered four-bit,
     * displacement-zero range. Only other payload remains UNMODELED. */
    { {0}, 1, 0, {0}, (C2FunctionRegister *)&machine_registers[74], {{{0}, 74}}, 4, 0 }
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
     * rows1/2/3/4/6/7/8 have exact32-bit self views. The only
     * additionally admitted searches are root1 -> row9 -> row17 and
     * root3 -> row11 -> row19 and root4 -> row12 -> row20 for width8/offset0;
     * each first matching row stops before its unknown next link.
     * The reached row9/width16 query follows root1 to that same row9.
     * Other views are valid original inputs; their chain data remains unknown. */
    if (!(source == (C2FunctionRegister *)&machine_registers[9] &&
          bits == 16 && displacement == 0) &&
        ((source != (C2FunctionRegister *)&machine_registers[0] &&
         source != (C2FunctionRegister *)&machine_registers[1] &&
         source != (C2FunctionRegister *)&machine_registers[2] &&
         source != (C2FunctionRegister *)&machine_registers[3] &&
         source != (C2FunctionRegister *)&machine_registers[4] &&
         source != (C2FunctionRegister *)&machine_registers[6] &&
         source != (C2FunctionRegister *)&machine_registers[7] &&
         source != (C2FunctionRegister *)&machine_registers[8]) ||
        bits != 32 || displacement != 0)) {
        if ((source != (C2FunctionRegister *)&machine_registers[3] &&
             source != (C2FunctionRegister *)&machine_registers[1] &&
             source != (C2FunctionRegister *)&machine_registers[4]) ||
            bits != 8 || displacement != 0) { __asm { int 3 } }
    }
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

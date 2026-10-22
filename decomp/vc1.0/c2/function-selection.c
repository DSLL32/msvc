#include "instruction-selection.h"
extern int codeview_debug;
/* Original [000193a0,000194c0), including its 31-entry control table.
   CodeView-only scope markers are filtered here; register lifetime and
   prebuilt machine records retain their separate constructors. */
void __fastcall select_directive(Node *node)
{
    short code = node->detail.control.code;
    switch (code) {
    case 0xf:
        emit_machine_directive(code, 0);
        break;
    case 0x10: case 0x11: case 0x12:
        if (codeview_debug) emit_machine_directive(code, 0);
        break;
    case 0x16:
        append_embedded_record(node);
        break;
    case 0x18:
        emit_machine_directive(code, node->left.word);
        break;
    case 0x24: case 0x25:
        emit_register_lifetime((unsigned short)node->left.value, node->attributes.words.type,
            code == 0x24, node->attributes.words.flags & 0x80);
        break;
    case 0x26: case 0x27: case 0x28: case 0x2a: case 0x2b: case 0x2c: case 0x2d:
        select_assembly_record(node);
        break;
    }
}
/* Original [00019560,000195a0). Emit saves in register order; the epilogue
   restores the same mask in reverse order. This is selection state, distinct
   from the final output saved-register mask at 0007e8a8. */
void emit_saved_register_pushes(void)
{
    int reg;
    MachineOperand *operand;
    for (reg = 0; reg < 8; ++reg) {
        if (selected_save_register_mask & (1U << reg)) {
            operand = new_operand(OPERAND_REGISTER, (unsigned short)reg, 0, (SymbolHeader *)0);
            emit_machine_operation(MC_EPUSH, 0x104, operand);
        }
    }
}

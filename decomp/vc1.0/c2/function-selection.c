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

/* Original [00019510,00019560). Restore the selected save set in the
   reverse order of its prologue pushes. */
void emit_saved_register_pops(void)
{
    int reg;
    MachineOperand *operand;
    for (reg = 7; reg >= 0; --reg) {
        if (selected_save_register_mask & (1U << reg)) {
            operand = new_operand(OPERAND_REGISTER, (unsigned short)reg, 0, (SymbolHeader *)0);
            emit_machine_operation(MC_EPOP, 0x104, operand);
        }
    }
}

/* Original [00026c80,00026d00), including the return-class table.
   Scalar integer/address results use EAX; floating results use ST(0). */
unsigned __fastcall return_value_register(FunctionSymbol *symbol)
{
    switch (symbol->field24 & 15) {
    case 0: case 7: return 0xffffffffU;
    case 1: case 2: case 3: case 6: case 8: case 9: return REG_EAX;
    case 5: return REG_ST0;
    default:
        compiler_internal_error("@(#)MDmisc.c:1.87", 0x228);
        return 0;
    }
}

/* Original [00019500,00019510). Kind 3 terminates the function stream. */
void finish_global_selection(void)
{
    append_machine_instruction(3);
}

/* Original [000194c0,00019500). Close any deferred stream with a block
   marker and link it back to the global selection header. */
void finish_function_selection(void)
{
    MachineBlockHeader *block;
    if (deferred_tail) {
        block = (MachineBlockHeader *)new_machine_instruction(7);
        block->definitions = selection_anchor;
        block->uses = selection_anchor;
        deferred_tail->next = (MachineInstruction *)block;
        block->previous = deferred_tail;
        deferred_tail = (MachineInstruction *)block;
        selection_global_header->previous = (MachineInstruction *)block;
        block->next = (MachineInstruction *)selection_global_header;
    }
}

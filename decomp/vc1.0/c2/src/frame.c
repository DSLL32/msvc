/* Stack-frame allocation state. */
#include "frame.h"
#include "functions.h"
#include "options.h"
extern int code_pointer_bytes;
/* Original [00027290,000272d0). Locals grow downward; unsigned arithmetic
   retains the original wrap/overflow diagnostic before returning the offset. */
long __fastcall reserve_local_storage(unsigned bytes)
{
    unsigned extent;
    local_stack_offset = (int)((unsigned)local_stack_offset - bytes);
    extent = align_stack_storage(0U - (unsigned)local_stack_offset, bytes);
    local_stack_offset = (int)(0U - extent);
    if (local_stack_offset > 0) compiler_report(0x7e, "2G");
    return local_stack_offset;
}
/* Original [000272d0,00027310). The procedure attributes determine the
   initial parameter-frame alignment; convention 0c supplies no frame. */
unsigned function_stack_alignment(void)
{
    unsigned short flags = current_symbol->symbol.field0e;
    if ((flags & 0x1c) == 0xc) return 0;
    if (!(flags & 2) && ((flags & 1) || code_pointer_bytes <= 8)) return 8;
    return 12;
}
unsigned stack_frame_extent; /* 00080688 */
/* Original [00026e30,00026e40). */
void reset_stack_frame(void)
{
    initialize_stack_alignment();
    stack_frame_extent = 0;
}

int local_stack_offset; /* 000806dc */
unsigned parameter_stack_offset = 8; /* 000806e0: next incoming parameter slot */
/* Original [00026e10,00026e30). */
void initialize_stack_alignment(void)
{
    local_stack_offset = 0;
    parameter_stack_offset = 8;
}

int dynamic_stack_allocation; /* 00083188: an alloca changes the epilogue stack restore */

#include "instruction-rewrite.h"
#include <recon.h>
extern int function_optimization, stack_probe_size;
SymbolHeader *__fastcall require_external_symbol(char *name);
/* Original [00022b20,00022c20). Resolve the frame pseudo operand, using
   __chkstk when required; empty ordinary frames lose their subtraction. */
unsigned __fastcall resolve_frame_allocation(MachineInstruction *instruction)
{
    unsigned bytes = calculate_stack_frame_size();
    MachineOperand destination, size;
    MachineInstruction *inserted;
    current_symbol->field30 = bytes;
    if (!bytes && !(current_symbol->symbol.field0e & 0x400)) {
        remove_machine_instruction(instruction);
        return 0;
    }
    if ((function_optimization || (unsigned)stack_probe_size <= bytes) &&
        !(current_symbol->symbol.field0e & 0x400)) {
        initialize_operand(&destination, OPERAND_REGISTER, REG_EAX, 0, 0);
        initialize_operand(&size, OPERAND_IMMEDIATE, 0x1a, bytes, 0);
        inserted = new_machine_record(0, MC_MOV, 4, &destination, &size);
        insert_machine_after(instruction->previous, inserted);
        initialize_operand(&destination, OPERAND_SYMBOL, 0x1a, 0, require_external_symbol("__chkstk"));
        inserted = new_machine_record(0, MC_CALL, 4, &destination, 0, 1);
        insert_machine_after(instruction->previous, inserted);
        remove_machine_instruction(instruction);
        return bytes;
    }
    initialize_operand(&instruction->operands[1], OPERAND_IMMEDIATE, 0x1a, bytes, 0);
    return bytes;
}

extern int default_segment_alignment;
/* Original [00026e60,00026e90). Byte/word temporaries may use narrower
   alignment than the default frame allocation unit. */
unsigned __fastcall align_stack_storage(unsigned offset, unsigned bytes)
{
    unsigned alignment = default_segment_alignment;
    if (bytes == 2 && alignment > 1) return (offset + 1) & ~1U;
    if (bytes == 1) return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}
/* Original [00026e40,00026e60). Locals grow downwards; round their extent
   to the same alignment used by argument and temporary allocation. */
unsigned calculate_stack_frame_size(void)
{
    unsigned alignment = (unsigned)default_segment_alignment;
    return (alignment - (unsigned)local_stack_offset - 1) & ~(alignment - 1);
}

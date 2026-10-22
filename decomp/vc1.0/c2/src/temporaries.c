#include "temporaries.h"
#include "tree-rewrite.h"
#include "expression-fields.h"
#include "frame.h"
extern int default_segment_alignment;
TemporaryRegister *temporary_registers;
/* Original [00058530,00058540). */
void reset_temporary_registers(void)
{
    temporary_registers = 0;
}

/* Original [000585a0,000585c0). The fifth cdecl argument preserves the
   native reference recipe, even though a non-null descriptor supplies type. */
Node *__fastcall new_temporary_reference(unsigned bytes, unsigned flags)
{
    TemporaryRegister *temporary = allocate_stack_temporary(bytes);
    return intern_leaf(0x26, 0, (SymbolHeader *)temporary, 0, 0x404);
}

/* Original [00058540,00058570). Align storage to the common argument/
   temporary alignment before reserving its stack offset. The allocator
   consumes only ECX; callers may leave an unrelated value in EDX. */
TemporaryRegister *__fastcall allocate_stack_temporary(unsigned bytes)
{
    unsigned alignment = (unsigned)default_segment_alignment;
    unsigned aligned = (bytes + alignment - 1) & ~(alignment - 1);
    TemporaryRegister *temporary = new_temporary_descriptor(aligned);
    temporary->number = (unsigned)reserve_local_storage(aligned);
    return temporary;
}

/* Original [00058570,000585a0). Kind 19 has its native packed 13-byte
   layout. The stack/register number is left to the descriptor's allocator. */
TemporaryRegister *__fastcall new_temporary_descriptor(unsigned bytes)
{
    TemporaryRegister *temporary = allocate_descriptor(19, 1);
    temporary->next = temporary_registers;
    temporary_registers = temporary;
    temporary->bytes = bytes;
    temporary->kind = 19;
    return temporary;
}

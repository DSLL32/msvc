/* Stack-frame allocation state. */
#include "frame.h"
#include "functions.h"
extern int code_pointer_bytes;
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
unsigned maximum_stack_alignment = 8; /* 000806e0 */
/* Original [00026e10,00026e30). */
void initialize_stack_alignment(void)
{
    local_stack_offset = 0;
    maximum_stack_alignment = 8;
}

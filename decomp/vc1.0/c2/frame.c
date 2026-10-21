/* Stack-frame allocation state. */
#include "frame.h"
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

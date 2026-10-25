#include "functions.h"
#include <recon.h>

int function_input_flags;
int function_analysis_available;

/* Size-indexed free lists. Allocation/release treats these as node pointers;
 * node payload and the remaining allocator are not reconstructed yet. */
C2SmallNode *free_small_nodes[49];

void clear_function_input_state(void)
{
    memset(free_small_nodes, 0, sizeof(free_small_nodes));
}

void begin_function_input(void)
{
    reset_register_marks();
    initialize_function_records();
    clear_function_input_state();
    code_generation_state = 1;
    /* Other writers store the full DWORD; this entry clears only its low byte. */
    *(unsigned char *)&function_input_flags = 0;
    current_function = NULL;
    if (c2_internal_listing)
        reset_function_listing();
    function_analysis_available = 0;
}

#include "c2.h"
#include <recon.h>

unsigned selected_save_register_mask, call_clobbered_register_mask;
int stack_alignment = 4;

void initialize_machine_registers(void)
{
    selected_save_register_mask = 0x1d0;
    call_clobbered_register_mask = 0x0e;
    if (c2_cpu_generation >= 4)
        stack_alignment = 16;
    if (c2_cpu_generation < 5)
        c2_cpu_generation = 5;
}

#include "registers.h"
#include <recon.h>

/* The 75 flags have original zero initializers. Implicit zero storage in the
 * pending fields represents unavailable backing, not original data defaults.
 */
C2MachineRegister machine_registers[C2_MACHINE_REGISTER_COUNT];

void reset_register_marks(void)
{
    C2MachineRegister *reg;

    for (reg = machine_registers;
         reg < machine_registers + C2_MACHINE_REGISTER_COUNT; ++reg)
        reg->flags &= ~0x10;
}

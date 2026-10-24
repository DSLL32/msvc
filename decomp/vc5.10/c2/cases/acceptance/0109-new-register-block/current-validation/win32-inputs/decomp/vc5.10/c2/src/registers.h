#ifndef C2_REGISTERS_H
#define C2_REGISTERS_H
#include "c2.h"
#include <recon.h>

#define C2_MACHINE_REGISTER_COUNT 75
/* Only flags has been encountered. The other bytes reserve proven stride,
 * with no original contents/default/type credit. Do not read or expose them
 * until their actual consumers and initial values have been reconstructed.
 */
typedef struct C2MachineRegister {
    unsigned char pending00[5];
    unsigned char flags;
    unsigned char pending06[74];
} C2MachineRegister;
typedef char C2MachineRegisterSizeCheck[sizeof(C2MachineRegister) == 80 ? 1 : -1];
typedef char C2MachineRegisterFlagsCheck[offsetof(C2MachineRegister, flags) == 5 ? 1 : -1];
extern C2MachineRegister machine_registers[C2_MACHINE_REGISTER_COUNT];
void reset_register_marks(void);
#endif

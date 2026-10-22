#ifndef MACHINE_TRANSFERS_H
#define MACHINE_TRANSFERS_H
#include "instruction-selection.h"
#include <recon.h>
enum MachineTransferFlags {
    MACHINE_TRANSFER_CALL_BOUNDARY = 0x400000,
    MACHINE_TRANSFER_PLACED = 0x800000
};
MachineInstruction *__fastcall find_transfer_call_boundary(MachineInstruction *transfer);
MachineInstruction *__fastcall find_transfer_use_boundary(MachineInstruction *transfer, MachineInstruction *start);
#endif

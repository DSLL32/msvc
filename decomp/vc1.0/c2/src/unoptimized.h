#ifndef UNOPTIMIZED_H
#define UNOPTIMIZED_H
#include "instruction-selection.h"
#include "bitsets.h"
typedef struct RegisterSlot {
    unsigned virtual_register;
    BitSet *conflicts;
} RegisterSlot;
typedef char RegisterSlotSizeCheck[sizeof(RegisterSlot) == 8 ? 1 : -1];
extern RegisterSlot unoptimized_registers[8];
extern unsigned unoptimized_live_mask;
extern BitSet *unoptimized_live_temporaries;
void __fastcall initialize_lowering_block(MachineBlockHeader *block);
unsigned __fastcall assign_physical_register(MachineInstruction *instruction, unsigned reg, int last_use);
void __fastcall mark_lowering_register_dead(unsigned reg);
void __fastcall mark_lowering_register_live(unsigned reg);
void __fastcall record_lowering_conflicts(unsigned reg);
void __fastcall record_byte_register_constraints(MachineInstruction *instruction, unsigned short reg);
#endif

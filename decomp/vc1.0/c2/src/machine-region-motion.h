#ifndef MACHINE_REGION_MOTION_H
#define MACHINE_REGION_MOTION_H
#include "machine-analysis.h"
#include <recon.h>
MachineInstruction *__fastcall move_region_block_operations(MachineBlockHeader *block,
    MachineInstruction *limit);
MachineInstruction *__fastcall move_region_instruction(MachineInstruction *instruction);
extern MachineBlockHeader *region_motion_block;
extern int region_motion_changed;
int __fastcall scale_region_increment(unsigned short number, MachineInstruction *increment, int apply, unsigned *changed);
MachineInstruction * __fastcall find_active_region_increment(unsigned short number);
void __fastcall replace_region_operand_uses(MachineOperand *operand, unsigned short number, MachineInstruction *limit);
unsigned short __fastcall hoist_region_definition(MachineInstruction *instruction, int retain);
int __fastcall region_register_is_constant(unsigned short number);
void __fastcall adjust_region_register_uses(MachineFlowRegion *region, unsigned short number, int delta);
void __fastcall remove_region_instruction(MachineFlowRegion *region, MachineInstruction *instruction);
void __fastcall replace_region_operand(MachineFlowRegion *region, MachineInstruction *first, MachineOperand *previous, MachineOperand *replacement);
unsigned short __fastcall new_region_address_register(MachineInstruction *instruction, unsigned base_scale, unsigned base, unsigned index, unsigned index_scale, SymbolHeader *symbol, unsigned displacement);
int __fastcall region_operand_replacement_safe(MachineInstruction *first, unsigned short number, MachineOperand *operand);
unsigned short __fastcall reserve_region_result_register(MachineInstruction *instruction, unsigned short register_flags);
int __fastcall region_operand_displacement(MachineOperand *operand, MachineOperand *base);
void __fastcall replace_region_operand_value(MachineOperand *source, MachineOperand *destination);
void __fastcall adjust_region_operand_uses(MachineFlowRegion *region, MachineOperand *operand, int delta);
#endif

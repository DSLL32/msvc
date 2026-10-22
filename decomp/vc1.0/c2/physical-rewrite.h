#ifndef PHYSICAL_REWRITE_H
#define PHYSICAL_REWRITE_H
#include "physical-allocation.h"
#include "machine-analysis.h"
/* Native cache objects at7c3a0..7c480 are eight28-byte records.
   The block reset starts at member+20 and clears two reference pointers;
   saved operand payloads remain live until their consumers replace them. */
typedef struct PhysicalRegisterCache {
    unsigned operation;
    MachineOperand saved[2];
    MachineOperand *reference[2];
} PhysicalRegisterCache;
typedef char PhysicalRegisterCacheSizeCheck[sizeof(PhysicalRegisterCache) == 28 ? 1 : -1];
/* Constructor2fb80 allocates eight bytes from arena3. */
typedef struct PhysicalRegisterAlias {
    struct PhysicalRegisterAlias *next;
    unsigned short number, reserved;
} PhysicalRegisterAlias;
typedef char PhysicalRegisterAliasSizeCheck[sizeof(PhysicalRegisterAlias) == 8 ? 1 : -1];
extern PhysicalRegisterCache register_value_cache[8];
extern PhysicalRegisterAlias *physical_register_aliases[8]; /*7c4a4..7c4c4*/
extern MachineFlowRegion *register_value_region; /*7c498*/
extern unsigned prior_live_physical_registers; /*7c49c*/
extern unsigned live_physical_registers; /*7c4a0*/
extern unsigned short previous_rewrite_register, latest_rewrite_register; /*7c490,494*/
void __fastcall add_physical_register_alias(unsigned short number);
MachineInstruction *__fastcall rewrite_register_block(MachineBlockHeader *block, MachineInstruction *limit);
void __fastcall rewrite_register_regions(MachineFlowRegion *regions);
void __fastcall record_written_register_aliases(MachineInstruction *instruction);
void __fastcall reuse_physical_register_values(MachineInstruction *instruction);
void __fastcall retire_physical_register_aliases(MachineInstruction *instruction);
int __fastcall choose_memory_split_register(MachineInstruction *instruction, unsigned short *number);
void __fastcall copy_machine_operand(MachineOperand *source, MachineOperand *destination);
extern unsigned short reusable_register_order[8]; /*7c480..7c490, built by30e90*/
unsigned __fastcall register_reuse_barriers(MachineInstruction *definition, MachineInstruction *use);
void __fastcall order_reusable_registers(MachineInstruction *definition, MachineInstruction *use, unsigned short color);
int __fastcall operand_ends_physical_register(MachineOperand *operand, unsigned short color);
void __fastcall remove_physical_register_alias(unsigned short number);
#endif

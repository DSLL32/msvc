#ifndef INSTRUCTION_REWRITE_H
#define INSTRUCTION_REWRITE_H
#include "instruction-selection.h"
unsigned short __fastcall new_value_register(unsigned short type);
void __fastcall release_machine_address(MachineAddress *address);
MachineInstruction *__fastcall insert_machine_after(MachineInstruction *previous, MachineInstruction *instruction);
/* Original 21790 reads ECX and one stack argument, then RET 4. The unused
   EDX slot expresses this private convention in ordinary original-compiler C. */
int __fastcall register_replacement_blocked(MachineInstruction *user, unsigned unused_edx, unsigned reg);
int __fastcall operand_changed_between(MachineInstruction *first, MachineInstruction *last,
    MachineOperand *operand, unsigned bytes);
int __fastcall narrow_result_safe(MachineInstruction *instruction);
int __fastcall expand_constant_multiply(MachineInstruction *instruction);
MachineInstruction *__fastcall find_flags_use(MachineInstruction *instruction);
int __fastcall instruction_mentions_register(MachineInstruction *instruction, unsigned short reg);
MachineInstruction *__fastcall find_carry_flag_use(MachineInstruction *instruction);
unsigned __fastcall integer_width_mask(unsigned bytes);
int __fastcall narrow_comparison_condition(unsigned condition);
int __fastcall instruction_operands_equal(MachineInstruction *left, unsigned left_index, MachineInstruction *right, unsigned right_index);
int __fastcall eliminate_redundant_compare(MachineInstruction *instruction, MachineInstruction **next);
unsigned __fastcall integer_sign_bit(unsigned bytes);
int __fastcall operand_mentions_register(MachineOperand *operand, unsigned short reg);
unsigned __fastcall integer_width_bits(unsigned bytes);
int __fastcall address_changed_between(MachineInstruction *first, MachineInstruction *last, MachineOperand *operand);
int __fastcall register_mentioned_between(MachineInstruction *first, MachineInstruction *last, unsigned short reg);
int __fastcall fold_lea_address(MachineInstruction *instruction, MachineOperand *operand, int apply);
MachineInstruction *__fastcall simplify_branch(MachineInstruction *instruction);
MachineInstruction *__fastcall clone_machine_record(MachineInstruction *instruction);
MachineInstruction *__fastcall replace_machine_operand(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply);
int __fastcall register_source_changed_between(MachineInstruction *first, MachineInstruction *last, unsigned short reg);
unsigned __fastcall machine_operand_bytes(MachineInstruction *instruction, int index);
unsigned __fastcall byte_register_available(unsigned short reg);
int __fastcall adjust_operand_label_references(MachineOperand *operand, int delta);
void __fastcall adjust_machine_symbol_references(MachineInstruction *instruction, int delta);
int __fastcall instruction_modifies_operand(MachineInstruction *instruction, MachineOperand *operand, unsigned bytes);
int __fastcall operand_replacement_allowed(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply);
void __fastcall release_operand_address(MachineOperand *operand);
int __fastcall branch_falls_through(MachineInstruction *instruction);
unsigned __fastcall resolve_frame_allocation(MachineInstruction *instruction);
void __fastcall advance_operand_byte(MachineOperand *operand);
int __fastcall expand_zero_extension(MachineInstruction *instruction);
void __fastcall optimize_sahf(MachineInstruction *instruction);
extern int conservative_flag_analysis;
MachineInstruction *__fastcall previous_machine_operation(MachineInstruction *instruction);
int __fastcall memory_displacements_overlap(MachineOperand *left, unsigned left_bytes, MachineOperand *right, unsigned right_bytes);
int __fastcall operands_overlap_storage(MachineOperand *left, unsigned left_bytes,
    MachineOperand *right, unsigned right_bytes);
#endif

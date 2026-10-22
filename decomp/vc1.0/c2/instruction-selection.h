#ifndef INSTRUCTION_SELECTION_H
#define INSTRUCTION_SELECTION_H
#include "machine.h"
extern BitSet *selection_anchor;
extern MachineBlockHeader *selection_global_header, *selection_end, *selected_block;
extern int temporary_operand_count;
extern int x87_stack_depth;
extern unsigned return_address_bytes;
extern unsigned selected_save_register_mask;
enum MachineRegister { REG_EAX = 0, REG_ECX = 1, REG_EDX = 2,
    REG_ESP = 4, REG_EBP = 5, REG_ST0 = 14 };
enum OperandKind { OPERAND_REGISTER = 1, OPERAND_IMMEDIATE = 2,
    OPERAND_BASED = 4, OPERAND_INDEXED = 5, OPERAND_LABEL_ADDRESS = 6, OPERAND_SYMBOL = 8,
    OPERAND_FRAME_SIZE = 9 };
typedef struct OpcodeMap { unsigned tree_opcode, selection; } OpcodeMap;
extern MachineOperand operand_eax, operand_ecx, operand_edx;
extern MachineOperand operand_esp, operand_ebp, operand_st0;
MachineInstruction *__fastcall new_machine_instruction(unsigned kind);
MachineInstruction *__cdecl new_machine_record(unsigned kind, ...);
MachineBlockHeader *__fastcall select_work_item(WorkItem *work);
MachineInstruction *__fastcall finish_selected_block(MachineBlockHeader *block, int mode);
void __fastcall peephole_selected_block(MachineBlockHeader *block, int global);
MachineBlockHeader *__fastcall new_selection_block(BitSet *definitions, BitSet *uses,
    int number, unsigned short flags);
void __fastcall select_expression(Node *node);
void __fastcall emit_source_marker(unsigned source_location);
MachineInstruction *__fastcall append_machine_instruction(unsigned kind);
MachineInstruction *__cdecl emit_machine_operation(unsigned opcode, unsigned type, ...);
MachineOperand *__fastcall operand_for_register(unsigned short reg);
unsigned short __fastcall select_value_register(Node *node);
MachineOperand *__fastcall new_operand(unsigned kind, unsigned short reg,
    unsigned displacement, SymbolHeader *symbol);
void __fastcall initialize_operand(MachineOperand *operand, unsigned kind,
    unsigned short reg, unsigned displacement, SymbolHeader *symbol);
MachineOperand *__fastcall duplicate_operand(MachineOperand *operand);
MachineOperand *__fastcall high_dword_operand(MachineOperand *operand);
MachineOperand *__fastcall select_lvalue_operand(Node *node);
void __fastcall select_condition(Node *node);
int __fastcall try_select_assignment(Node *node);
unsigned __fastcall lookup_opcode(const OpcodeMap *map, unsigned opcode);
void __fastcall emit_source_filename(char *filename);
void __fastcall select_directive(Node *node);
void finish_function_selection(void);
void finish_global_selection(void);
void emit_saved_register_pops(void);
void emit_saved_register_pushes(void);
MachineInstruction *__fastcall emit_call(unsigned type, MachineOperand *target,
    unsigned info0, unsigned info1, unsigned info2);
MachineInstruction *__fastcall emit_function_symbol(FunctionSymbol *symbol);
void emit_float_return(void);
void __fastcall emit_label(SymbolHeader *symbol);
void __fastcall select_assembly_record(Node *node);
unsigned __fastcall return_value_register(FunctionSymbol *symbol);
LabelSymbol *new_internal_label(void);
MachineInstruction *__fastcall next_machine_block(MachineInstruction *instruction);
void __fastcall analyze_selected_block(MachineBlockHeader *block);
MachineInstruction *__fastcall finish_machine_instruction(MachineInstruction *instruction, int mode);
void __fastcall remove_machine_instruction(MachineInstruction *instruction);
MachineAddress *new_machine_address(void);
#endif

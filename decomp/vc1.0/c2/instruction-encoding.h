#ifndef INSTRUCTION_ENCODING_H
#define INSTRUCTION_ENCODING_H
#include "machine.h"
unsigned __fastcall encode_machine_instruction(MachineInstruction *instruction,
    unsigned char *buffer, unsigned location, int emit);
extern unsigned encoding_location;
extern int encoding_emit_relocations;
extern unsigned char *encoding_buffer, *encoding_cursor;
void __fastcall encode_modrm(MachineInstruction *instruction, MachineOperand *operand, unsigned reg);
unsigned __fastcall encode_operand(MachineOperand *operand, unsigned bytes, int sign_extend);
void __fastcall encode_integer(unsigned value, unsigned bytes);
int __fastcall fits_signed_byte(int value);
unsigned __fastcall displacement_mode(MachineInstruction *instruction, MachineOperand *operand, unsigned *bytes);
#endif

#ifndef ENCODING_RECIPES_H
#define ENCODING_RECIPES_H
#include "machine.h"
typedef enum EncodingForm {
    ENC_INVALID, ENC_MODRM, ENC_RM_IMMEDIATE, ENC_ACCUMULATOR_ADDRESS,
    ENC_PUSH_IMMEDIATE, ENC_REGISTER_IMMEDIATE, ENC_REGISTER_OPCODE,
    ENC_FIXED, ENC_RELATIVE_BYTE, ENC_RELATIVE_NEAR, ENC_FAR_ADDRESS,
    ENC_SEGMENT_MOVE, ENC_SEGMENT_STACK, ENC_SPECIAL, ENC_X87_ARITHMETIC,
    ENC_X87_MEMORY, ENC_MEMORY_GROUP, ENC_FIXED_SECOND, ENC_SHIFT,
    ENC_MODRM_ALTERNATE, ENC_ACCUMULATOR_IMMEDIATE
} EncodingForm;
enum EncodingModifier { ENCODE_SIGN_EXTEND = 1, ENCODE_DIRECTION = 2, ENCODE_WIDTH = 4 };
typedef struct EncodingPattern {
    EncodingForm form;
    unsigned char opcode, modifiers;
} EncodingPattern;
typedef struct InstructionEncoding {
    MachineOpcode operation;
    /* Depending on form: ModR/M group bits, a fixed trailing opcode, or
       the x87 register-form opcode bits. */
    unsigned char extension;
    EncodingPattern patterns[6];
} InstructionEncoding;
extern const InstructionEncoding instruction_encodings[MC_OPCODE_COUNT];
#endif

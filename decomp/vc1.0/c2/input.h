#ifndef INPUT_H
#define INPUT_H
#include "c2.h"
#include "extended.h"
extern FILE *input_file;
unsigned char input_byte(void);
unsigned short input_word(void);
unsigned short input_type(void);
unsigned long input_dword(void);
unsigned long input_varuint(void);
long input_compact_signed(void);
short input_compact_short(void);
void __fastcall input_string(char *buffer, unsigned limit);
void __fastcall input_bytes(int count, unsigned char *buffer);
unsigned input_float_bits(void);
DoubleBits input_double_bits(void);
Extended __stdcall input_extended(void);
#endif

#ifndef INPUT_H
#define INPUT_H
#include "c2.h"
extern FILE *input_file;
unsigned char input_byte(void);
unsigned short input_word(void);
unsigned long input_dword(void);
#endif

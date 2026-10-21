#ifndef CONTROL_INPUT_H
#define CONTROL_INPUT_H
#include "expression-fields.h"
void __fastcall read_module_directive(int kind);
short __fastcall input_counted_string(char *buffer, unsigned limit);
char *__fastcall lookup_filename(unsigned identifier);
extern unsigned pragma_value, saved_pragma_value;
extern char *current_source_filename;
#endif

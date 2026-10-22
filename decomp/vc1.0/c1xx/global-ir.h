#ifndef GLOBAL_IR_H
#define GLOBAL_IR_H
#include "declarations.h"
extern unsigned char global_record_kind;
void __stdcall write_global_record(unsigned tag, CppSymbol *symbol);
#endif

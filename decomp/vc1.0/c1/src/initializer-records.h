#ifndef INITIALIZER_RECORDS_H
#define INITIALIZER_RECORDS_H
#include "expressions.h"
#include <recon.h>
extern FrontendSymbol *pending_initializer_symbol;
int __stdcall is_character_array_initializer(FrontendSymbol *symbol, IrToken *value);
int __stdcall find_initializer_address(IrToken *value, FrontendSymbol **symbol, int *offset);
unsigned __stdcall initializer_address_size(FrontendSymbol *symbol);
int extended_to_double(const Extended *value, double *result);
int extended_to_float(const Extended *value, float *result);
int extended_to_integer(const Extended *value, int *result);
void write_ir_float(float *value);
void write_ir_double(double *value);
void write_ir_short(short *value);
#endif

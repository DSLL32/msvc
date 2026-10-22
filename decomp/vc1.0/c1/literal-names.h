#ifndef LITERAL_NAMES_H
#define LITERAL_NAMES_H
#include "lexer.h"
#include "work-stack.h"
char *__stdcall string_pool_name(StringToken *string, int wide);
IrToken *__stdcall define_string_literal(IrToken *token, char *name, int pooled);
char *__stdcall encode_string_pool_name(char *destination, unsigned char *text, unsigned bytes, int wide);
char *__stdcall encode_name_number(char *destination, unsigned value);
char *__stdcall encode_name_character(char *destination, char value);
unsigned __stdcall checksum_bytes(unsigned char *text, unsigned bytes, unsigned seed);
void __stdcall assign_unpooled_literal_segment(FrontendSymbol *symbol, FrontendSymbol *segment);
void __stdcall emit_literal_initializer(FrontendSymbol *symbol, IrToken *token);
extern unsigned char initializer_tag;
void __stdcall write_literal_initializer(FrontendSymbol *symbol, IrToken *token, FILE *stream);
typedef WorkingBlock InitializerBlock;
typedef WorkingStack InitializerBuffer;
/* Source replay and initializer output reuse original 0x5f4f4. */
#define initializer_buffer compiler_stack
void __stdcall finish_initializer_buffer(InitializerBuffer *buffer);
unsigned __stdcall write_initializer_bytes(void *data, unsigned size, unsigned count, InitializerBuffer *buffer);
int __stdcall put_initializer_byte(int value, InitializerBuffer *buffer);
void write_ir_integer(int *value);
void __stdcall write_initializer_value(FrontendSymbol *symbol, IrToken *value, FILE *stream);
#endif

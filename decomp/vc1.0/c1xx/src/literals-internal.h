#ifndef LITERALS_INTERNAL_H
#define LITERALS_INTERNAL_H
#include "lexer.h"
#include <recon.h>
unsigned char *__stdcall check_literal_cursor(unsigned char *cursor);
unsigned char *__stdcall encode_literal_segment(unsigned char *segment, int wide, unsigned bytes);
void __stdcall finish_pascal_literal(ParserValue *literal);
unsigned __stdcall decode_literal_escape(unsigned char character, int wide);
#endif

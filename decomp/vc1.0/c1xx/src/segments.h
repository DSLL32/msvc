#ifndef SEGMENTS_H
#define SEGMENTS_H
#include "parser.h"
#include <recon.h>
CppSymbol *__stdcall find_name_in_chain(CppSymbol *symbol, char *name);
SegmentSymbol *__stdcall find_segment_symbol(char *name);
SegmentSymbol *__stdcall new_segment_symbol(char *name, char *class_name,
    unsigned index, unsigned char kind);
int __stdcall segment_name_has_suffix(char *name, char *suffix);
typedef char SegmentClassOffsetMustBe32[(offsetof(SegmentSymbol,detail) == 32) ? 1 : -1];
typedef char SegmentKindOffsetMustBe36[(offsetof(SegmentSymbol,auxiliary) == 36) ? 1 : -1];
#endif

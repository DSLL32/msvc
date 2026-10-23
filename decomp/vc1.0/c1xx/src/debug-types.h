#ifndef DEBUG_TYPES_H
#define DEBUG_TYPES_H
#include "types.h"
#include <recon.h>

/* Deferred CodeView records use sixteen-byte slots. Their subject may be
   a type or a declaration; each producer initializes its own record fields. */
typedef struct DebugTypeRecord {
    unsigned char kind, reserved1;
    unsigned short attributes;
    union { CType *type; CppSymbol *symbol; } subject;
    unsigned short related_type, reserved0a;
    union { unsigned count; void *pointer; } detail;
} DebugTypeRecord;
typedef char DebugTypeRecordSizeMustBe16[(sizeof(DebugTypeRecord) == 16) ? 1 : -1];
typedef char DebugTypeRecordSubjectOffsetMustBe4[(offsetof(DebugTypeRecord, subject) == 4) ? 1 : -1];
typedef char DebugTypeRecordRelatedOffsetMustBe8[(offsetof(DebugTypeRecord, related_type) == 8) ? 1 : -1];
extern DebugTypeRecord *debug_types;
extern unsigned short next_debug_type_index;
unsigned __stdcall reserve_debug_type(unsigned char kind);
unsigned short __stdcall primitive_pointer_debug_type(CType *type);
unsigned short __stdcall base_debug_type(CType *type);
unsigned short __stdcall primitive_debug_type(unsigned short flags);
void __stdcall reference_function_debug_type(CType *type);
unsigned short __cdecl reference_line_record_type(CType *type);
#endif

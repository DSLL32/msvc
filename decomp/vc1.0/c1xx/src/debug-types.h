#ifndef DEBUG_TYPES_H
#define DEBUG_TYPES_H
#include "types.h"
#include <recon.h>

/* Deferred CodeView records use sixteen-byte slots. Their subject may be
   a type or a declaration; each producer initializes its own record fields. */
typedef struct DebugTypeRecord {
    unsigned char kind, reserved1;
    unsigned short attributes;
    union { CType *type; CppSymbol *symbol; CppSymbol **members; FunctionParameters *parameters; } subject;
    unsigned short related_type, reserved0a;
    union { unsigned count; unsigned short field_list; int displacement, first_member; void *pointer; } detail;
} DebugTypeRecord;
typedef char DebugTypeRecordSizeMustBe16[(sizeof(DebugTypeRecord) == 16) ? 1 : -1];
typedef char DebugTypeRecordSubjectOffsetMustBe4[(offsetof(DebugTypeRecord, subject) == 4) ? 1 : -1];
typedef char DebugTypeRecordRelatedOffsetMustBe8[(offsetof(DebugTypeRecord, related_type) == 8) ? 1 : -1];
/* A substituted symbol reference splits the current serialized type leaf.
   Offsets are relative to the live debug buffer. */
typedef struct DebugTypeFixup {
    CppSymbol *symbol;
    unsigned offset;
    unsigned bytes;
    struct DebugTypeFixup *next;
} DebugTypeFixup;
typedef char DebugTypeFixupSizeMustBe16[(sizeof(DebugTypeFixup) == 16) ? 1 : -1];
extern DebugTypeFixup *debug_type_fixups;
extern unsigned debug_field_bytes;
unsigned pad_debug_field(unsigned char *destination, unsigned length);
int __stdcall emit_debug_base_fields(CppSymbol *tag, unsigned *member_index, unsigned first_member);
void split_debug_field_record(unsigned char kind, void *subject, unsigned first, unsigned tail);
struct OverloadSet;
unsigned __stdcall count_emitted_overloads(struct OverloadSet *overloads);
unsigned __stdcall debug_member_name_length(CppSymbol *symbol, short *conversion_text, int qualified);
void __stdcall write_debug_member_name(unsigned char *destination, CppSymbol *symbol,
    unsigned length, short conversion_text, int qualified);
unsigned __stdcall encode_debug_signed(unsigned char *destination, int value);
void __stdcall queue_debug_type_fixup(CppSymbol *symbol, unsigned char *location, unsigned bytes);
unsigned __stdcall encode_debug_based_symbol(unsigned char *destination, CppSymbol *symbol);
unsigned __stdcall update_checksum(unsigned char *bytes, unsigned count, unsigned checksum);
extern DebugTypeRecord *debug_types;
extern unsigned short next_debug_type_index;
extern unsigned debug_type_count;
extern unsigned debug_type_capacity;
extern unsigned short debug_field_properties;
unsigned debug_calling_convention(unsigned short attributes);
int compare_debug_symbol_indices(const void *left, const void *right);
unsigned __stdcall encode_debug_unsigned(unsigned char *destination, unsigned value);
unsigned char *__stdcall copy_debug_name(unsigned char *destination, char *name);
void __stdcall emit_debug_type_record(int length);
unsigned __stdcall reserve_debug_type(unsigned char kind);
unsigned short __stdcall primitive_pointer_debug_type(CType *type);
unsigned short __stdcall base_debug_type(CType *type);
unsigned short __stdcall primitive_debug_type(unsigned short flags);
void __stdcall reference_function_debug_type(CType *type);
unsigned short __cdecl reference_line_record_type(CType *type);
void reset_debug_type_records(void);
void __stdcall emit_debug_pointer_type(CType *type, unsigned short base_index);
void __stdcall emit_debug_array_type(CType *type, unsigned short base_index);
void __stdcall emit_debug_function_type(CType *type, unsigned short return_index,
    unsigned short argument_count);
void __stdcall emit_debug_member_function_type(CType *type, unsigned short return_index,
    unsigned short argument_count, unsigned short class_index, int this_displacement);
void __stdcall emit_debug_qualified_type(CType *type);
void __stdcall emit_debug_bit_field(CppSymbol *field);
void __stdcall emit_debug_class(CppSymbol *tag, unsigned short field_count,
    unsigned short field_list);
struct OverloadSet;
void __stdcall emit_debug_method_list(struct OverloadSet *overloads);
void __stdcall emit_debug_field_list(CppSymbol *tag, unsigned first_member,
    unsigned short vshape_index);
void __stdcall emit_debug_argument_list(FunctionParameters *parameters);
void __stdcall emit_debug_vshape(CppSymbol *tag);
void __stdcall emit_debug_field_list_chunk(CppSymbol **members, int first, int limit);
void __cdecl prepare_line_record_tag(CppSymbol *tag);
unsigned short __cdecl line_record_tag_type(CppSymbol *tag);
struct HashTable;
unsigned __stdcall hash_table_count(struct HashTable *table);
unsigned __stdcall count_debug_methods(struct OverloadSet *overloads,
    unsigned short *field_properties);
unsigned char __stdcall reference_parameter_debug_types(FunctionParameters *parameters);
#endif

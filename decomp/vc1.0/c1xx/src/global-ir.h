#ifndef GLOBAL_IR_H
#define GLOBAL_IR_H
#include "macros.h"
#include <recon.h>
enum GlobalField {
    GF_END = 0,
    GF_SIZE = 1,
    GF_FLAGS = 2,
    GF_RECORD_TAG = 3,
    GF_ARGUMENT_BYTES = 4,
    GF_SYMBOL_INDEX = 5,
    GF_DEBUG_TYPE = 6,
    GF_CONTEXT = 7,
    GF_REGISTER = 8,
    GF_NAME = 9,
    GF_COMMON_KIND = 10,
    GF_ZERO = 11,
    GF_DISTANCE = 12,
    GF_TYPE = 13,
    GF_REFERENCES = 14,
    GF_ENUM_VALUE = 15,
    GF_STORAGE = 16,
    GF_FUNCTION_STORAGE = 17,
    GF_RESERVED_18 = 18,
    GF_ARRAY_BOUND = 19,
    GF_FUNCTION_END = 20,
    GF_ALIAS_INDEX = 21,
    GF_TAG_INDEX = 22,
    GF_TAG_KIND = 23,
    GF_STREAM_POSITIONS = 24,
    GF_COMPLEXITY = 25,
    GF_OPTIMIZATION = 26,
    GF_TYPE_STACK = 27,
    GF_METHOD_ALIAS_INDEX = 28,
    GF_MACHINE = 29,
    GF_COMPILER_VERSION = 30,
    GF_SYMBOL_COUNT16 = 31,
    GF_RESERVED_BYTE = 32,
    GF_PRAGMA_VALUES = 33,
    GF_SOURCE_FILE = 34,
    GF_PARAMETER_COUNT = 35,
    GF_RESERVED_36 = 36,
    GF_SYMBOL_COUNT32 = 37
};
typedef struct PragmaValue {
    struct PragmaValue *next;
    unsigned kind;
    unsigned char *value;
} PragmaValue;
typedef struct FunctionIrState {
    unsigned complexity;
    unsigned short parameter_count, reserved;
    unsigned symbol_offset, expression_offset;
    int out_of_line_required;
} FunctionIrState;
extern unsigned char global_record_kind;
extern unsigned char *global_record_formats[19];
extern FILE *auxiliary_ir_stream;
extern FunctionIrState function_ir;
extern int hidden_frame_enabled, function_call_seen, inline_candidate, function_not_inlinable;
extern unsigned hidden_frame_bytes;
extern unsigned pragma_record_number;
extern char *pragma_text;
extern PragmaValue *pragma_values;
/* Original qc records rank two parameter slots per control region.
   The0xff0-byte allocator and16-byte index stride establish255 rows. */
typedef struct InlineRegion {
    unsigned short next_sibling, first_child;
    unsigned char parameter_count, flags, left_slot, right_slot;
    unsigned left_symbol_index, right_symbol_index;
} InlineRegion;
typedef char InlineRegionSizeMustBe16[(sizeof(InlineRegion) == 16) ? 1 : -1];
typedef char InlineRegionSymbolOffsetMustBe8[
    (offsetof(InlineRegion, left_symbol_index) == 8) ? 1 : -1];
typedef char InlineRegionRightSymbolOffsetMustBe12[
    (offsetof(InlineRegion, right_symbol_index) == 12) ? 1 : -1];
extern InlineRegion *inline_regions;
extern unsigned short inline_region_count;
extern unsigned *source_file_index, source_file_counter;
extern char last_record_source[256];
extern unsigned next_symbol_index;
CType *__stdcall drop_first_type_shape(CType *type);
CType *__stdcall make_class_pointer_type(CppSymbol *class_symbol, unsigned short attributes);
unsigned char __stdcall member_pointer_is_aggregate(CType *type);
CppSymbol *__stdcall method_alias(CppSymbol *symbol);
CppSymbol *__stdcall lookup_symbol_binding(CppSymbol *symbol, CppSymbol **value);
int __stdcall type_size(CType *type);
void __stdcall require_complete_type(CType *type);
int __stdcall symbol_element_size(CppSymbol *symbol);
unsigned short __stdcall type_alignment(CType *type);
void __stdcall emit_symbol_declaration(CppSymbol *symbol);
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol);
char *__stdcall write_decorated_name(FILE *stream, CppSymbol *symbol,
                                      CppSymbol *context, int flags);
void write_ir_filename(char *text);
void write_ir_string(char *text);
void write_ir_index(unsigned *index);
void write_ir_integer(int *value);
void write_ir_short(short *value);
void write_ir_float(float *value);
void write_ir_double(double *value);
void __stdcall write_global_record(unsigned tag, CppSymbol *symbol);
typedef char FunctionIrOutOfLineOffsetMustBe16[(offsetof(FunctionIrState, out_of_line_required) == 16) ? 1 : -1];
typedef char FunctionIrStateSizeMustBe20[(sizeof(FunctionIrState) == 20) ? 1 : -1];
#endif

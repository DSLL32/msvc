#ifndef GLOBAL_IR_H
#define GLOBAL_IR_H
#include "macros.h"
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
    GF_ZERO_19 = 19,
    GF_FUNCTION_END = 20,
    GF_ALIAS_INDEX = 21,
    GF_TAG_INDEX = 22,
    GF_TAG_KIND = 23,
    GF_STREAM_POSITIONS = 24,
    GF_COMPLEXITY = 25,
    GF_OPTIMIZATION = 26,
    GF_TYPE_STACK = 27,
    GF_MACHINE_LEGACY = 28,
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
    int no_inline;
} FunctionIrState;
extern unsigned char global_record_kind;
extern unsigned char *global_record_formats[19];
extern FILE *auxiliary_ir_stream;
extern FunctionIrState function_ir;
extern int hidden_frame_enabled, function_call_seen, inline_candidate, function_not_inlinable;
extern int default_inline_candidate;
extern unsigned hidden_frame_bytes;
extern int far_call_mode, prologue_mode;
extern unsigned pragma_record_number;
extern char *pragma_text;
extern PragmaValue *pragma_values;
extern CType *declaration_type_stack;
extern unsigned short declaration_type_index;
extern unsigned *source_file_index, source_file_counter;
extern char last_record_source[256];
int __stdcall type_size(CType *type);
void __stdcall require_complete_type(CType *type);
int __stdcall parameter_stack_bytes(FrontendSymbol *symbol);
unsigned short __stdcall type_alignment(CType *type);
void __stdcall emit_symbol_declaration(FrontendSymbol *symbol);
FrontendSymbol *__stdcall symbol_alias(FrontendSymbol *symbol);
void __stdcall write_decorated_name(FILE *stream, FrontendSymbol *symbol);
void write_ir_filename(char *text);
void write_ir_string(char *text);
void write_ir_index(unsigned *index);
void __stdcall write_global_record(unsigned tag, FrontendSymbol *symbol);
#endif

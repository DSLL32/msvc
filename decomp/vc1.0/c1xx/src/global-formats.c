/* Schemas consumed by the global intermediate record writer at 0x57190. */
#include "global-ir.h"
#include <recon.h>

static unsigned char stream_end_fields[] = { GF_RECORD_TAG, GF_END };
static unsigned char symbol_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_ZERO, GF_DISTANCE, GF_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_END };
static unsigned char array_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_ZERO, GF_DISTANCE, GF_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_ARGUMENT_BYTES, GF_END };
static unsigned char common_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_REFERENCES, GF_COMMON_KIND, GF_END };
static unsigned char function_fields[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_END };
static unsigned char tag_fields[] = { GF_RECORD_TAG, GF_TAG_INDEX, GF_SYMBOL_INDEX, GF_SIZE, GF_REGISTER, GF_DEBUG_TYPE, GF_END };
static unsigned char marker_fields[] = { GF_RECORD_TAG, GF_END };
static unsigned char alias_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_TYPE, GF_ZERO, GF_SIZE, GF_STORAGE, GF_ARRAY_BOUND, GF_DEBUG_TYPE, GF_ALIAS_INDEX, GF_ARGUMENT_BYTES, GF_END };
static unsigned char field_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_DEBUG_TYPE, GF_TYPE, GF_ZERO, GF_SIZE, GF_FUNCTION_END, GF_END };
static unsigned char enum_fields[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_NAME, GF_TAG_KIND, GF_ENUM_VALUE, GF_END };
static unsigned char pragma_fields[] = { GF_RECORD_TAG, GF_PRAGMA_VALUES, GF_END };
static unsigned char debug_symbol_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_ZERO, GF_DEBUG_TYPE, GF_END };
static unsigned char empty_fields[] = { GF_END };
static unsigned char context_fields[] = { GF_RECORD_TAG, GF_CONTEXT, GF_END };
static unsigned char function_body_fields[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_STREAM_POSITIONS, GF_COMPLEXITY, GF_OPTIMIZATION, GF_PARAMETER_COUNT, GF_END };
static unsigned char type_stack_fields[] = { GF_RECORD_TAG, GF_TYPE_STACK, GF_END };
static unsigned char method_fields[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_METHOD_ALIAS_INDEX, GF_END };
static unsigned char header_fields[] = { GF_RECORD_TAG, GF_MACHINE, GF_COMPILER_VERSION, GF_SYMBOL_COUNT32, GF_RESERVED_BYTE, GF_END };
static unsigned char source_file_fields[] = { GF_RECORD_TAG, GF_SOURCE_FILE, GF_END };

unsigned char *global_record_formats[19] = {
    stream_end_fields, symbol_fields, array_fields, common_fields, function_fields, tag_fields, marker_fields, alias_fields, field_fields, enum_fields, pragma_fields, debug_symbol_fields, empty_fields, context_fields, function_body_fields, type_stack_fields, method_fields, header_fields, source_file_fields
};

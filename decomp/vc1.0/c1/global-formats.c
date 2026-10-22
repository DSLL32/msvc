/* Schemas consumed by the global intermediate record writer at 0x36e00. */
#include "global-ir.h"

static unsigned char global_format_0[] = { GF_RECORD_TAG, GF_END };
static unsigned char global_format_1[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_ZERO, GF_DISTANCE, GF_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_END };
static unsigned char global_format_2[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_ZERO, GF_DISTANCE, GF_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_ARGUMENT_BYTES, GF_END };
static unsigned char global_format_3[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_REFERENCES, GF_COMMON_KIND, GF_END };
static unsigned char global_format_4[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_END };
static unsigned char global_format_5[] = { GF_RECORD_TAG, GF_TAG_INDEX, GF_SYMBOL_INDEX, GF_SIZE, GF_REGISTER, GF_DEBUG_TYPE, GF_END };
static unsigned char global_format_6[] = { GF_RECORD_TAG, GF_END };
static unsigned char global_format_7[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_TYPE, GF_ZERO, GF_SIZE, GF_STORAGE, GF_ZERO_19, GF_DEBUG_TYPE, GF_ALIAS_INDEX, GF_ARGUMENT_BYTES, GF_END };
static unsigned char global_format_8[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_DEBUG_TYPE, GF_TYPE, GF_ZERO, GF_SIZE, GF_FUNCTION_END, GF_END };
static unsigned char global_format_9[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_NAME, GF_TAG_KIND, GF_ENUM_VALUE, GF_END };
static unsigned char global_format_10[] = { GF_RECORD_TAG, GF_PRAGMA_VALUES, GF_END };
static unsigned char global_format_11[] = { GF_RECORD_TAG, GF_CONTEXT, GF_SYMBOL_INDEX, GF_NAME, GF_ZERO, GF_DEBUG_TYPE, GF_END };
static unsigned char global_format_12[] = { GF_END };
static unsigned char global_format_13[] = { GF_RECORD_TAG, GF_CONTEXT, GF_END };
static unsigned char global_format_14[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_STREAM_POSITIONS, GF_COMPLEXITY, GF_OPTIMIZATION, GF_PARAMETER_COUNT, GF_END };
static unsigned char global_format_15[] = { GF_RECORD_TAG, GF_TYPE_STACK, GF_END };
static unsigned char global_format_16[] = { GF_RECORD_TAG, GF_SYMBOL_INDEX, GF_REGISTER, GF_NAME, GF_TYPE, GF_FUNCTION_STORAGE, GF_SIZE, GF_FLAGS, GF_DEBUG_TYPE, GF_MACHINE_LEGACY, GF_END };
static unsigned char global_format_17[] = { GF_RECORD_TAG, GF_MACHINE, GF_COMPILER_VERSION, GF_SYMBOL_COUNT32, GF_RESERVED_BYTE, GF_END };
static unsigned char global_format_18[] = { GF_RECORD_TAG, GF_SOURCE_FILE, GF_END };

unsigned char *global_record_formats[19] = {
    global_format_0, global_format_1, global_format_2, global_format_3, global_format_4, global_format_5, global_format_6, global_format_7, global_format_8, global_format_9, global_format_10, global_format_11, global_format_12, global_format_13, global_format_14, global_format_15, global_format_16, global_format_17, global_format_18
};

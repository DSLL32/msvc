/* Expression record field schemas, original format pointers at 0x6e594. */
#include "ir.h"
#include <recon.h>

static unsigned char format_00[] = { IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_01[] = { IR_FIELD_NODE, IR_FIELD_END };
static unsigned char format_02[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_END };
static unsigned char format_03[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_04[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_NODE, IR_FIELD_ZERO_13, IR_FIELD_CALL_KIND, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_05[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_CALL_KIND, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_06[] = { IR_FIELD_NODE, IR_FIELD_ZERO_14, IR_FIELD_END };
static unsigned char format_07[] = { IR_FIELD_CONSTANT, IR_FIELD_END };
static unsigned char format_08[] = { IR_FIELD_NODE, IR_FIELD_SYMBOL_INDEX, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_09[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_TYPE_NUMBER, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_10[] = { IR_FIELD_NODE, IR_FIELD_STORAGE, IR_FIELD_END };
static unsigned char format_11[] = { IR_FIELD_STORAGE, IR_FIELD_END };
static unsigned char format_12[] = { IR_FIELD_NODE, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_13[] = { IR_FIELD_RESERVED_9, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_14[] = { IR_FIELD_RESERVED_4, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_15[] = { IR_FIELD_INTEGER, IR_FIELD_END };
static unsigned char format_16[] = { IR_FIELD_INTEGER, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_17[] = { IR_FIELD_NODE, IR_FIELD_ZERO, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_18[] = { IR_FIELD_RESERVED_12, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char format_19[] = { IR_FIELD_RESERVED_16, IR_FIELD_END };
static unsigned char format_20[] = { IR_FIELD_RESERVED_112, IR_FIELD_END };
static unsigned char format_21[] = { IR_FIELD_NODE, IR_FIELD_RESERVED_17, IR_FIELD_END };

unsigned char *ir_record_formats[191] = {
    format_00, /* record 0x00 */
    format_00, /* record 0x01 */
    0, /* record 0x02 */
    0, /* record 0x03 */
    0, /* record 0x04 */
    0, /* record 0x05 */
    0, /* record 0x06 */
    0, /* record 0x07 */
    0, /* record 0x08 */
    0, /* record 0x09 */
    0, /* record 0x0a */
    0, /* record 0x0b */
    0, /* record 0x0c */
    0, /* record 0x0d */
    0, /* record 0x0e */
    format_01, /* record 0x0f */
    format_01, /* record 0x10 */
    format_01, /* record 0x11 */
    format_01, /* record 0x12 */
    format_01, /* record 0x13 */
    format_01, /* record 0x14 */
    format_01, /* record 0x15 */
    format_01, /* record 0x16 */
    format_01, /* record 0x17 */
    format_01, /* record 0x18 */
    format_01, /* record 0x19 */
    0, /* record 0x1a */
    0, /* record 0x1b */
    0, /* record 0x1c */
    0, /* record 0x1d */
    0, /* record 0x1e */
    0, /* record 0x1f */
    0, /* record 0x20 */
    0, /* record 0x21 */
    0, /* record 0x22 */
    0, /* record 0x23 */
    0, /* record 0x24 */
    0, /* record 0x25 */
    format_02, /* record 0x26 */
    format_01, /* record 0x27 */
    format_02, /* record 0x28 */
    format_03, /* record 0x29 */
    format_04, /* record 0x2a */
    format_05, /* record 0x2b */
    format_06, /* record 0x2c */
    format_03, /* record 0x2d */
    0, /* record 0x2e */
    format_01, /* record 0x2f */
    format_01, /* record 0x30 */
    format_01, /* record 0x31 */
    format_01, /* record 0x32 */
    format_07, /* record 0x33 */
    format_06, /* record 0x34 */
    format_01, /* record 0x35 */
    format_01, /* record 0x36 */
    0, /* record 0x37 */
    format_03, /* record 0x38 */
    format_03, /* record 0x39 */
    format_03, /* record 0x3a */
    format_03, /* record 0x3b */
    format_08, /* record 0x3c */
    format_09, /* record 0x3d */
    format_10, /* record 0x3e */
    format_11, /* record 0x3f */
    format_01, /* record 0x40 */
    format_12, /* record 0x41 */
    0, /* record 0x42 */
    0, /* record 0x43 */
    0, /* record 0x44 */
    format_13, /* record 0x45 */
    format_00, /* record 0x46 */
    format_00, /* record 0x47 */
    format_00, /* record 0x48 */
    format_03, /* record 0x49 */
    format_00, /* record 0x4a */
    format_00, /* record 0x4b */
    format_00, /* record 0x4c */
    0, /* record 0x4d */
    0, /* record 0x4e */
    format_14, /* record 0x4f */
    format_15, /* record 0x50 */
    format_15, /* record 0x51 */
    format_16, /* record 0x52 */
    format_00, /* record 0x53 */
    format_16, /* record 0x54 */
    format_12, /* record 0x55 */
    format_00, /* record 0x56 */
    format_00, /* record 0x57 */
    format_01, /* record 0x58 */
    0, /* record 0x59 */
    format_01, /* record 0x5a */
    format_15, /* record 0x5b */
    0, /* record 0x5c */
    0, /* record 0x5d */
    0, /* record 0x5e */
    0, /* record 0x5f */
    0, /* record 0x60 */
    0, /* record 0x61 */
    0, /* record 0x62 */
    0, /* record 0x63 */
    0, /* record 0x64 */
    0, /* record 0x65 */
    0, /* record 0x66 */
    0, /* record 0x67 */
    0, /* record 0x68 */
    format_02, /* record 0x69 */
    0, /* record 0x6a */
    0, /* record 0x6b */
    0, /* record 0x6c */
    0, /* record 0x6d */
    0, /* record 0x6e */
    0, /* record 0x6f */
    0, /* record 0x70 */
    0, /* record 0x71 */
    0, /* record 0x72 */
    0, /* record 0x73 */
    0, /* record 0x74 */
    0, /* record 0x75 */
    0, /* record 0x76 */
    format_01, /* record 0x77 */
    format_17, /* record 0x78 */
    0, /* record 0x79 */
    0, /* record 0x7a */
    0, /* record 0x7b */
    0, /* record 0x7c */
    format_01, /* record 0x7d */
    format_10, /* record 0x7e */
    format_18, /* record 0x7f */
    0, /* record 0x80 */
    format_19, /* record 0x81 */
    format_19, /* record 0x82 */
    format_19, /* record 0x83 */
    0, /* record 0x84 */
    0, /* record 0x85 */
    0, /* record 0x86 */
    format_20, /* record 0x87 */
    0, /* record 0x88 */
    0, /* record 0x89 */
    0, /* record 0x8a */
    0, /* record 0x8b */
    0, /* record 0x8c */
    format_12, /* record 0x8d */
    format_00, /* record 0x8e */
    format_00, /* record 0x8f */
    format_00, /* record 0x90 */
    0, /* record 0x91 */
    0, /* record 0x92 */
    0, /* record 0x93 */
    0, /* record 0x94 */
    0, /* record 0x95 */
    0, /* record 0x96 */
    0, /* record 0x97 */
    0, /* record 0x98 */
    format_01, /* record 0x99 */
    format_01, /* record 0x9a */
    format_21, /* record 0x9b */
    format_01, /* record 0x9c */
    format_00, /* record 0x9d */
    0, /* record 0x9e */
    /* Remaining reserved records have no payload fields. */
};

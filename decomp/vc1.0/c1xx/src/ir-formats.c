/* Expression record field schemas, original format pointers at 0xa2b0c, stride eight in the original record table. */
#include "ir.h"
#include <recon.h>

static unsigned char annotation_fields[] = { IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char node_fields[] = { IR_FIELD_NODE, IR_FIELD_END };
static unsigned char symbol_fields[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_END };
static unsigned char symbol_annotation_fields[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char symbol_node_call_fields[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_NODE, IR_FIELD_ZERO_13, IR_FIELD_CALL_KIND, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char symbol_call_fields[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_CALL_KIND, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char node_zero14_fields[] = { IR_FIELD_NODE, IR_FIELD_ZERO_14, IR_FIELD_END };
static unsigned char constant_fields[] = { IR_FIELD_CONSTANT, IR_FIELD_END };
static unsigned char node_symbol_fields[] = { IR_FIELD_NODE, IR_FIELD_SYMBOL_INDEX, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char symbol_type_fields[] = { IR_FIELD_SYMBOL_INDEX, IR_FIELD_TYPE_NUMBER, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char node_storage_fields[] = { IR_FIELD_NODE, IR_FIELD_STORAGE, IR_FIELD_END };
static unsigned char storage_fields[] = { IR_FIELD_STORAGE, IR_FIELD_END };
static unsigned char node_annotation_fields[] = { IR_FIELD_NODE, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char annotation9_fields[] = { IR_FIELD_RESERVED_9, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char annotation4_fields[] = { IR_FIELD_RESERVED_4, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char integer_fields[] = { IR_FIELD_INTEGER, IR_FIELD_END };
static unsigned char integer_annotation_fields[] = { IR_FIELD_INTEGER, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char node_zero_fields[] = { IR_FIELD_NODE, IR_FIELD_ZERO, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char annotation12_fields[] = { IR_FIELD_RESERVED_12, IR_FIELD_RESERVED_15, IR_FIELD_END };
static unsigned char annotation16_fields[] = { IR_FIELD_RESERVED_16, IR_FIELD_END };
static unsigned char annotation112_fields[] = { IR_FIELD_RESERVED_112, IR_FIELD_END };
static unsigned char node_packed_fields[] = { IR_FIELD_NODE, IR_FIELD_PACKED_VALUE, IR_FIELD_END };

unsigned char *ir_record_formats[191] = {
    annotation_fields, /* record 0x00 */
    annotation_fields, /* record 0x01 */
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
    node_fields, /* record 0x0f */
    node_fields, /* record 0x10 */
    node_fields, /* record 0x11 */
    node_fields, /* record 0x12 */
    node_fields, /* record 0x13 */
    node_fields, /* record 0x14 */
    node_fields, /* record 0x15 */
    node_fields, /* record 0x16 */
    node_fields, /* record 0x17 */
    node_fields, /* record 0x18 */
    node_fields, /* record 0x19 */
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
    symbol_fields, /* record 0x26 */
    node_fields, /* record 0x27 */
    symbol_fields, /* record 0x28 */
    symbol_annotation_fields, /* record 0x29 */
    symbol_node_call_fields, /* record 0x2a */
    symbol_call_fields, /* record 0x2b */
    node_zero14_fields, /* record 0x2c */
    symbol_annotation_fields, /* record 0x2d */
    0, /* record 0x2e */
    node_fields, /* record 0x2f */
    node_fields, /* record 0x30 */
    node_fields, /* record 0x31 */
    node_fields, /* record 0x32 */
    constant_fields, /* record 0x33 */
    node_zero14_fields, /* record 0x34 */
    node_fields, /* record 0x35 */
    node_fields, /* record 0x36 */
    0, /* record 0x37 */
    symbol_annotation_fields, /* record 0x38 */
    symbol_annotation_fields, /* record 0x39 */
    symbol_annotation_fields, /* record 0x3a */
    symbol_annotation_fields, /* record 0x3b */
    node_symbol_fields, /* record 0x3c */
    symbol_type_fields, /* record 0x3d */
    node_storage_fields, /* record 0x3e */
    storage_fields, /* record 0x3f */
    node_fields, /* record 0x40 */
    node_annotation_fields, /* record 0x41 */
    0, /* record 0x42 */
    0, /* record 0x43 */
    0, /* record 0x44 */
    annotation9_fields, /* record 0x45 */
    annotation_fields, /* record 0x46 */
    annotation_fields, /* record 0x47 */
    annotation_fields, /* record 0x48 */
    symbol_annotation_fields, /* record 0x49 */
    annotation_fields, /* record 0x4a */
    annotation_fields, /* record 0x4b */
    annotation_fields, /* record 0x4c */
    0, /* record 0x4d */
    0, /* record 0x4e */
    annotation4_fields, /* record 0x4f */
    integer_fields, /* record 0x50 */
    integer_fields, /* record 0x51 */
    integer_annotation_fields, /* record 0x52 */
    annotation_fields, /* record 0x53 */
    integer_annotation_fields, /* record 0x54 */
    node_annotation_fields, /* record 0x55 */
    annotation_fields, /* record 0x56 */
    annotation_fields, /* record 0x57 */
    node_fields, /* record 0x58 */
    0, /* record 0x59 */
    node_fields, /* record 0x5a */
    integer_fields, /* record 0x5b */
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
    symbol_fields, /* record 0x69 */
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
    node_fields, /* record 0x77 */
    node_zero_fields, /* record 0x78 */
    0, /* record 0x79 */
    0, /* record 0x7a */
    0, /* record 0x7b */
    0, /* record 0x7c */
    node_fields, /* record 0x7d */
    node_storage_fields, /* record 0x7e */
    annotation12_fields, /* record 0x7f */
    0, /* record 0x80 */
    annotation16_fields, /* record 0x81 */
    annotation16_fields, /* record 0x82 */
    annotation16_fields, /* record 0x83 */
    0, /* record 0x84 */
    0, /* record 0x85 */
    0, /* record 0x86 */
    annotation112_fields, /* record 0x87 */
    0, /* record 0x88 */
    0, /* record 0x89 */
    0, /* record 0x8a */
    0, /* record 0x8b */
    0, /* record 0x8c */
    node_annotation_fields, /* record 0x8d */
    annotation_fields, /* record 0x8e */
    annotation_fields, /* record 0x8f */
    annotation_fields, /* record 0x90 */
    0, /* record 0x91 */
    0, /* record 0x92 */
    0, /* record 0x93 */
    0, /* record 0x94 */
    0, /* record 0x95 */
    0, /* record 0x96 */
    0, /* record 0x97 */
    0, /* record 0x98 */
    node_fields, /* record 0x99 */
    node_fields, /* record 0x9a */
    node_packed_fields, /* record 0x9b */
    node_fields, /* record 0x9c */
    annotation_fields, /* record 0x9d */
    0, /* record 0x9e */
    /* Remaining reserved records have no payload fields. */
};

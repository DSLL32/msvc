/* Typed descriptions of the attributes carried by expression records. */
#include "expression-fields.h"
#include "input.h"
#include "records.h"
#include "options.h"
static const ExpressionField no_fields[] = {FIELD_END};
static const ExpressionField type_fields[] = {FIELD_TYPE, FIELD_END};
static const ExpressionField symbol_fields[] = {FIELD_SYMBOL, FIELD_END};
static const ExpressionField qualified_symbol_fields[] = {
    FIELD_SYMBOL, FIELD_TYPE, FIELD_IGNORE_BYTE, FIELD_IGNORE_BYTE, FIELD_END
};
static const ExpressionField ignored_symbol_fields[] = {FIELD_SYMBOL, FIELD_IGNORE_BYTE, FIELD_END};
static const ExpressionField typed_byte_fields[] = {FIELD_TYPE, FIELD_IGNORE_BYTE, FIELD_END};
static const ExpressionField number_fields[] = {FIELD_NUMBER, FIELD_END};
static const ExpressionField typed_symbol_fields[] = {FIELD_TYPE, FIELD_SYMBOL, FIELD_END};
static const ExpressionField symbol_offset_fields[] = {FIELD_SYMBOL, FIELD_DUPLICATE_LONG, FIELD_END};
static const ExpressionField typed_control_fields[] = {FIELD_TYPE, FIELD_CONTROL_BYTE, FIELD_END};
static const ExpressionField control_byte_fields[] = {FIELD_CONTROL_BYTE, FIELD_END};
static const ExpressionField invalid_fields[] = {FIELD_INVALID, FIELD_END};
static const ExpressionField control_fields[] = {FIELD_CONTROL_WORD, FIELD_END};
static const ExpressionField long_fields[] = {FIELD_LONG, FIELD_END};
static const ExpressionField paired_symbol_fields[] = {FIELD_TYPE, FIELD_SYMBOL_PAIR, FIELD_END};
static const ExpressionField line_fields[] = {FIELD_LINE, FIELD_END};
static const ExpressionField typed_label_fields[] = {FIELD_TYPE, FIELD_VARUINT, FIELD_END};
int expression_line;

/* Original [0004c640,0004ca60), including its field dispatch table.
   The original pointer/byte recipes are expressed as semantic field lists.
   Recipes for unsupported attributes still diagnose once without consuming. */
Node *__fastcall decode_expression(unsigned short opcode, Function *function)
{
    const ExpressionField *fields;
    Node *node, *canonical;
    long value;
    unsigned type;
    Extended extended;
    DoubleBits double_bits;
    float single;
    switch (opcode) {
    case 0x00: case 0x01: case 0x46: case 0x47: case 0x48: case 0x4a:
    case 0x4b: case 0x4c: case 0x53: case 0x56: case 0x57: case 0x8e:
    case 0x8f: case 0x90: case 0x9d: fields = no_fields; break;
    case 0x0f: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
    case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x27:
    case 0x2f: case 0x30: case 0x31: case 0x32: case 0x35: case 0x36:
    case 0x40: case 0x41: case 0x55: case 0x58: case 0x5a: case 0x77:
    case 0x7d: case 0x8d: case 0x99: case 0x9a: case 0x9c:
        fields = type_fields; break;
    case 0x26: case 0x28: case 0x29: case 0x2d: case 0x38: case 0x39:
    case 0x3a: case 0x3b: case 0x49: case 0x69: fields = symbol_fields; break;
    case 0x2a: fields = qualified_symbol_fields; break;
    case 0x2b: fields = ignored_symbol_fields; break;
    case 0x2c: case 0x34: fields = typed_byte_fields; break;
    case 0x33: fields = number_fields; break;
    case 0x3c: fields = typed_symbol_fields; break;
    case 0x3d: fields = symbol_offset_fields; break;
    case 0x3e: case 0x7e: fields = typed_control_fields; break;
    case 0x3f: fields = control_byte_fields; break;
    case 0x45: case 0x81: case 0x82: case 0x83: case 0x87:
        fields = invalid_fields; break;
    case 0x4f: fields = control_fields; break;
    case 0x50: case 0x51: case 0x52: case 0x54: case 0x5b:
        fields = long_fields; break;
    case 0x78: fields = paired_symbol_fields; break;
    case 0x7f: fields = line_fields; break;
    case 0x9b: fields = typed_label_fields; break;
    default: fields = 0; break;
    }
    node = new_node(1);
    if (opcode == 0x5a) {
        opcode = 0x30;
        node->attributes.words.flags |= 0x40;
    }
    node->opcode = opcode;
    if (!fields) return node;
    while (*fields != FIELD_END) {
        switch (*fields++) {
        case FIELD_IGNORE_BYTE:
            input_byte();
            break;
        case FIELD_CONTROL_BYTE:
            node->detail.control.code = (signed char)input_byte();
            break;
        case FIELD_CONTROL_WORD:
            node->detail.control.code = input_compact_short();
            read_control_fields(node);
            break;
        case FIELD_DUPLICATE_LONG:
            node->left.value = node->detail.value = input_compact_signed();
            break;
        case FIELD_TYPE:
            read_node_type(node);
            break;
        case FIELD_SYMBOL_PAIR:
            node->right.symbol = lookup_symbol(function->scope, input_varuint());
            node->detail.symbol = lookup_symbol(function->scope, input_varuint());
            break;
        case FIELD_SYMBOL:
            node->right.symbol = lookup_symbol(function->scope, input_varuint());
            if (node->opcode == 0x26 && !node->right.symbol)
                compiler_internal_error("@(#)getattr.c:1.113", 0x95);
            break;
        case FIELD_NUMBER:
            read_node_type(node);
            type = node->attributes.words.type;
            if ((type & 0xff00) != 0x1000) {
                value = input_compact_signed();
                node->left.value = value;
                if (!(type & 0xff00)) type = 0x100;
                if (!(type & 0xff)) {
                    if ((type & 0x100) && value >= -128 && value <= 127) type = (type & 0xff01) | 1;
                    else if ((type & 0x100) && value >= -32767 && value <= 32767) type = (type & 0xff02) | 2;
                    else type = (type & 0xff04) | 4;
                }
                node->attributes.words.type = (unsigned short)type;
            } else {
                if (type == 0x100a) {
                    extended = input_extended();
                    type = (unsigned short)(extended_format(&extended) | 0x1000);
                    node->attributes.words.type = (unsigned short)type;
                    if (type == 0x1004) {
                        if (extended_to_float(&extended, &single)) compiler_warning(2, 0x2f4);
                        float_to_extended(&single, &extended);
                    } else {
                        if (extended_to_double(&extended, &double_bits)) compiler_warning(2, 0x2f4);
                        double_to_extended(&double_bits, &extended);
                    }
                } else {
                    compiler_internal_error("@(#)getattr.c:1.113", 0xb7);
                    double_bits = input_double_bits();
                    double_to_extended(&double_bits, &extended);
                }
                /* The leaf interner copies this temporary before we return. */
                node->left.pointer = &extended;
            }
            break;
        case FIELD_LINE:
            expression_line = input_compact_short();
            break;
        case FIELD_VARUINT:
            node->detail.value = input_varuint();
            break;
        case FIELD_LONG:
            node->left.value = input_compact_signed();
            break;
        default:
            compiler_report(1, "d:\\usr\\cuda\\src\\P2\\getattr.c", 0x116);
            break;
        }
    }
    switch (node->opcode) {
    case 0x26: case 0x69:
        canonical = intern_leaf(node->opcode, 0, node->right.symbol, 0);
        node->opcode = 1;
        release_node(node);
        return canonical;
    case 0x33:
        canonical = intern_leaf(0x33, node->attributes.words.flags,
                               node->attributes.words.type, node->left.value);
        node->opcode = 1;
        release_node(node);
        return canonical;
    case 0x4c:
        node->opcode = 1;
        release_node(node);
        return sentinel_node;
    case 0x9b:
        return intern_typed_label(function, node);
    }
    return node;
}

/* Original [0004ca60,0004cad0). The qualifier is retained on the
   operations whose original opcode class has the qualified-type bit. */
void __fastcall read_node_type(Node *node)
{
    unsigned short raw = input_type();
    signed char kind = (signed char)(raw & 15);
    unsigned bytes = (raw & 0x1f0) >> 4;
    if (raw & 0x1000) {
        switch (node->opcode) {
        case 0x0f: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
        case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x30:
        case 0x32: case 0x35: case 0x36: case 0x5a: case 0x7d: case 0x87:
            node->attributes.words.flags |= 0x800;
            break;
        }
    }
    if (kind == 6 && !bytes) bytes = input_compact_signed();
    if (node->opcode != 0x27) {
        node->attributes.words.type = encode_node_type(kind, bytes);
        if (node->attributes.words.type == 0x8000) node->info.value = bytes;
    }
}

extern unsigned char floating_point_used;
int default_integer_bytes = 4; /* 00083164: also the -isize option destination */
/* Original [0004cad0,0004cb70), including the kind dispatch table. */
unsigned __fastcall encode_node_type(signed char kind, unsigned bytes)
{
    switch (kind) {
    case 0: return 0;
    case 1: return bytes | 0x100;
    case 2: return bytes | 0x200;
    case 3: case 4: case 11: return bytes | 0x400;
    case 5:
        floating_point_used = 1;
        return bytes | 0x1000;
    case 6: return 0x8000;
    case 7: return default_integer_bytes | 0x100;
    case 10:
        floating_point_used = 1;
        return 0x100a;
    default:
        compiler_internal_error("@(#)getattr.c:1.113", 0x1dd);
        return 0;
    }
}

/* Original [0004cb70,0004cc00). Recover the frontend kind represented
   by a backend scalar/storage class; widths occupy the low byte. */
unsigned short __fastcall node_type_kind(unsigned type)
{
    switch (type & 0xff00) {
    case 0: return 0;
    case 0x100: return 1;
    case 0x200: return 2;
    case 0x400: return 3;
    case 0x1000: return 5;
    case 0x2000: return 8;
    case 0x4000: return 9;
    case 0x8000: return 6;
    default:
        compiler_internal_error("@(#)getattr.c:1.113", 0x19e);
        return 0;
    }
}

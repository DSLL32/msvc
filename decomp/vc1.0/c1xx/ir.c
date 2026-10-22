/* Expression-stream records. Buffer operations stay inside their original caller. */
#include "ir.h"

CppSymbol *function_symbol; /* original 0x8c140 */
CppSymbol *current_label_symbol; /* original 0x91384 */
int function_call_seen; /* original 0x9fc04; shared with global serialization */
int inline_candidate = 1; /* original 0x9fc00 */

#define PUT_IR_WORD(stream, value) do { \
    FILE *word_stream = (stream); \
    unsigned short word_value = (unsigned short)(value); \
    if ((word_stream->_cnt -= 2) >= 0) { \
        *(unsigned short *)word_stream->_ptr = word_value; \
        word_stream->_ptr += 2; \
    } else { \
        word_stream->_cnt += 2; \
        fwrite(&word_value, 2, 1, word_stream); \
    } \
} while (0)

#define PUT_IR_INTEGER(stream, value) do { \
    FILE *integer_stream = (stream); \
    long integer_value = (long)(value); \
    if (integer_value >= -127 && integer_value <= 127) \
        putc((signed char)integer_value, integer_stream); \
    else { \
        putc(0x80, integer_stream); \
        if ((integer_stream->_cnt -= 4) >= 0) { \
            *(long *)integer_stream->_ptr = integer_value; \
            integer_stream->_ptr += 4; \
        } else { \
            integer_stream->_cnt += 4; \
            fwrite(&integer_value, 4, 1, integer_stream); \
        } \
    } \
} while (0)

/* Original 0x59ea0..0x5a5e4, dispatch tables 0x5a5e4..0x5a68c. */
void __stdcall write_ir_record(unsigned tag, void *value)
{
    IrToken *node = value;
    unsigned char *format;
    FILE *file;
    CType *type;
    TypeShape *shape;
    CppSymbol *symbol;
    unsigned index, storage;
    unsigned short attributes;
    putc(tag, expression_file);
    ++expression_record_count;
    format = ir_record_formats[tag];
    if (!format)
        return;
    file = expression_file;
    for (; *format; ++format) {
        switch (*format) {
        case IR_FIELD_CALL_KIND:
            storage = ((((IrToken *)node->value.pointer)->kind & 0x1f) == 10);
            putc(storage, file);
            break;
        case IR_FIELD_INTEGER:
            PUT_IR_INTEGER(expression_file, node->value.integer);
            break;
        case IR_FIELD_STORAGE:
            /* Call records contain a callee expression, unlike symbol records. */
            shape = ((IrToken *)node->value.pointer)->type->related;
            if (shape && !(shape->flags & 0x77))
                shape = shape->related;
            attributes = shape->attributes;
            if ((attributes & 7) == 1) {
                function_call_seen = 1;
                inline_candidate &=
                    ((function_symbol->type->related->attributes & 7) == 1);
            }
            if (attributes & 0x100)
                storage = 12;
            else {
                switch (attributes & 0x70) {
                case 0x10: storage = 0; break;
                case 0x20:
                case 0x30: storage = 8; break;
                case 0x40: storage = 4; break;
                case 0x50: storage = 24; break;
                case 0x60:
                    if ((shape->flags & 0x77) != 4)
                        shape = shape->related;
                    storage = (shape->flags & 0x8000) ? 0 : 16;
                    break;
                case 0x70: storage = 20; break;
                }
            }
            attributes = shape->attributes;
            if (attributes & 0x200)
                storage |= 0x80;
            if ((attributes & 7) == 3)
                storage |= 3;
            putc(storage, file);
            break;
        case IR_FIELD_TYPE_NUMBER:
            PUT_IR_INTEGER(expression_file, (unsigned)node->type);
            break;
        case IR_FIELD_NODE:
            emit_record_node(tag, node, 2);
            break;
        case IR_FIELD_ZERO:
        case IR_FIELD_ZERO_13:
        case IR_FIELD_ZERO_14:
            putc(0, file);
            break;
        case IR_FIELD_SYMBOL_INDEX:
            index = 0;
            symbol = 0;
            switch (tag) {
            case 0x27:
            case 0x28:
                break;
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2d:
                index = ((CppSymbol *)node->value.pointer)->index;
                break;
            case 0x38:
            case 0x39:
            case 0x3a:
                symbol = node->value.pointer;
                reference_ir_symbol(symbol);
                break;
            case 0x3b:
            case 0x3c:
                symbol = current_label_symbol;
                ++symbol->references;
                reference_ir_symbol(symbol);
                break;
            case 0x3d:
                index = node->field08;
                break;
            default:
                symbol = node->value.pointer;
                break;
            }
            if (symbol)
                index = get_ir_symbol_index(symbol);
            if (index > 0x7fff) {
                PUT_IR_WORD(file, index | 0x8000);
                index >>= 15;
            }
            PUT_IR_WORD(file, index);
            break;
        case IR_FIELD_CONSTANT:
            if (node->type->related && (node->type->related->flags & 0x10))
                node->type = builtin_integer_type();
            emit_record_node(tag, node, 1);
            type = node->type;
            if (type->related || (type->flags & 0x240)) {
                PUT_IR_INTEGER(expression_file, node->value.integer);
            } else if (type->flags & 0x100) {
                fwrite(node->value.pointer, 10, 1, file);
                fwrite((char *)node->value.pointer + 10, 2, 1, file);
            } else if (type->flags & 0x80) {
                PUT_IR_INTEGER(expression_file, node->value.integer);
            }
            break;
        case IR_FIELD_PACKED_VALUE:
            index = node->value.integer;
            if (index > 0x7fff) {
                PUT_IR_WORD(file, index | 0x8000);
                index >>= 15;
            }
            PUT_IR_WORD(file, index);
            break;
        }
    }
}

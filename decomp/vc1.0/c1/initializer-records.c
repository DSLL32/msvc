/* Initializer-value records share the ordinary and buffered output encodings. */
#include "initializer-records.h"
#include "initializers.h"
#include "literal-names.h"
#include "global-ir.h"
#include "control-ir.h"

FrontendSymbol *pending_initializer_symbol; /* original 0x6c068 */

/* These macros keep the original writer's direct call edges and its
   choice between FILE output and the segmented initializer buffer. */
#define INITIALIZER_BYTE(value_) do { \
    signed char byte_ = (signed char)(value_); \
    if (auxiliary_ir_stream) putc(byte_, auxiliary_ir_stream); \
    else put_initializer_byte(byte_, initializer_buffer); \
} while (0)
#define INITIALIZER_INTEGER(value_) do { \
    int *integer_ = (value_); \
    if (auxiliary_ir_stream) write_ir_integer(integer_); \
    else if (*integer_ >= -127 && *integer_ <= 127) \
        put_initializer_byte((signed char)*integer_, initializer_buffer); \
    else { \
        put_initializer_byte(0x80, initializer_buffer); \
        write_initializer_bytes(integer_, 4, 1, initializer_buffer); \
    } \
} while (0)
#define INITIALIZER_SHORT(value_) do { \
    short *short_ = (value_); \
    if (auxiliary_ir_stream) write_ir_short(short_); \
    else if (*short_ >= -127 && *short_ <= 127) \
        put_initializer_byte((signed char)*short_, initializer_buffer); \
    else { \
        put_initializer_byte(0x80, initializer_buffer); \
        write_initializer_bytes(short_, 2, 1, initializer_buffer); \
    } \
} while (0)
#define INITIALIZER_INDEX(value_) do { \
    unsigned *index_ = (value_), word_; \
    if (auxiliary_ir_stream) write_ir_index(index_); \
    else if (*index_ <= 0x7fff) write_initializer_bytes(index_, 2, 1, initializer_buffer); \
    else { \
        word_ = *index_ | 0x8000; \
        write_initializer_bytes(&word_, 2, 1, initializer_buffer); \
        word_ = *index_ >> 15; \
        write_initializer_bytes(&word_, 2, 1, initializer_buffer); \
    } \
} while (0)

/* Original 0x3b3a0..0x3bd16, dispatch 0x3bd18..0x3bd6c. Classify
   each initializer, then emit its relocation, size and value fields. */
void __stdcall write_initializer_value(FrontendSymbol *symbol, IrToken *value, FILE *stream)
{
    unsigned kind = 0, index, word, width; /* width is established by 'z'. */
    int offset = 0, integer_value, number, unsigned_value, integral;
    short short_value, length;
    unsigned short representation = 0, flags, shape_flags;
    unsigned char byte_value;
    int was_string = 0;
    FrontendSymbol *reference = 0;
    TypeShape *shape;
    CType *type;
    char *format, *text;
    float single_value;
    double double_value;
    Extended extended_value;
    if (!value || (parser_flags & 0x20000000)) return;
    if (pending_initializer_symbol && !stream) {
        write_literal_initializer(pending_initializer_symbol, 0, 0);
        pending_initializer_symbol = 0;
        scanner_flags |= 0x100000;
    }
    if (!symbol) kind = 8;
    else if ((value->flags & 7) == 5) {
        was_string = 1;
        if (is_character_array_initializer(symbol, value)) kind = 3;
        else {
            value = materialize_string_literal(value);
            if (find_initializer_address(value, &reference, &offset)) kind = 2;
        }
    } else if ((value->flags & 7) == 3) kind = 1;
    else if (find_initializer_address(value, &reference, &offset)) {
        if (reference) kind = 2;
        else { value = make_integer_constant(offset); kind = 1; }
    }
    switch (kind) {
    case 1: format = "tpzv"; break;
    case 2: format = "tsfz"; break;
    case 3: format = "tl"; break;
    case 8: format = "tn"; break;
    default: format = "mdox"; break;
    }
    auxiliary_ir_stream = stream;
    while (*format) {
        switch (*format++) {
        case 'f': INITIALIZER_INTEGER(&offset); break;
        case 'i': INITIALIZER_BYTE(0); break;
        case 'l':
            text = value->value.text;
            length = (short)value->length;
            if (auxiliary_ir_stream) write_counted_ir(length, text);
            else {
                INITIALIZER_SHORT(&length);
                write_initializer_bytes(text, length, 1, initializer_buffer);
            }
            break;
        case 'n':
            integer_value = value->value.integer;
            INITIALIZER_INTEGER(&integer_value);
            break;
        case 'p':
            type = symbol->type;
            shape = type->related;
            if (symbol->attributes & 2) shape = shape->related;
            if (shape && (shape->flags & 1)) representation = 3;
            else {
                flags = type->flags;
                if ((flags & 0x4000) ||
                    (!options.extensions_enabled && options.option_J &&
                     (flags & 0x2fdf) == 0x41 && !(flags & 0x8000))) representation = 2;
                else if (flags & 0x40) representation = 1;
                else if (flags & 0x100) representation = 5;
                else if (flags & 0x800) representation = 2;
                else if (flags & 0x200) representation = 1;
            }
            INITIALIZER_BYTE(representation);
            break;
        case 's':
            index = get_ir_symbol_index(reference);
            INITIALIZER_INDEX(&index);
            break;
        case 't': INITIALIZER_BYTE(kind); break;
        case 'v':
            type = value->type;
            flags = type->flags;
            unsigned_value = 0;
            if ((flags & 0x40) || (type->related && (type->related->flags & 1)) ||
                (flags & 0x200)) {
                integral = 1;
                unsigned_value = (flags & 0x4000) ||
                    (!options.extensions_enabled && options.option_J &&
                     (flags & 0x2fdf) == 0x41 && !(flags & 0x8000));
                integer_value = value->value.integer;
            } else if (flags & 0x100) integral = 0;
            if (representation == 5) {
                if (integral) {
                    if (unsigned_value) number = unsigned_to_extended((unsigned *)&integer_value, &extended_value);
                    else number = integer_to_extended(&integer_value, &extended_value);
                    if (number) compiler_warning(2, 56);
                }
                if (width == 8) {
                    if (extended_to_double(integral ? &extended_value : value->value.pointer, &double_value))
                        compiler_warning(2, 56);
                    if (auxiliary_ir_stream) write_ir_double(&double_value);
                    else write_initializer_bytes(&double_value, 8, 1, initializer_buffer);
                } else if (width == 4) {
                    if (extended_to_float(integral ? &extended_value : value->value.pointer, &single_value))
                        compiler_warning(2, 56);
                    if (auxiliary_ir_stream) write_ir_float(&single_value);
                    else write_initializer_bytes(&single_value, 4, 1, initializer_buffer);
                } else {
                    if (!integral) extended_value = *(Extended *)value->value.pointer;
                    if (auxiliary_ir_stream) {
                        fwrite(&extended_value, 10, 1, auxiliary_ir_stream);
                        fwrite(&extended_value.format, 2, 1, auxiliary_ir_stream);
                    } else {
                        write_initializer_bytes(&extended_value, 10, 1, initializer_buffer);
                        write_initializer_bytes(&extended_value.format, 2, 1, initializer_buffer);
                    }
                }
            } else if (width == 1) {
                if (integral) byte_value = (unsigned char)integer_value;
                else {
                    if (extended_to_integer(value->value.pointer, &number)) compiler_warning(2, 56);
                    byte_value = (unsigned char)number;
                }
                INITIALIZER_BYTE(byte_value);
            } else if (width == 2) {
                if (integral) short_value = (short)integer_value;
                else {
                    if (extended_to_integer(value->value.pointer, &number)) compiler_warning(2, 56);
                    short_value = (short)number;
                }
                INITIALIZER_SHORT(&short_value);
            } else if (width == 4 || width == 8) {
                if (integral) number = integer_value;
                else if (extended_to_integer(value->value.pointer, &number)) compiler_warning(2, 56);
                INITIALIZER_INTEGER(&number);
            }
            break;
        case 'z':
            shape = symbol->type->related;
            if (shape && !(shape->flags & 7)) shape = shape->related;
            shape_flags = shape ? shape->flags : 0;
            if ((shape_flags & 7) == 2) width = symbol_element_size(symbol);
            else width = type_size(symbol->type);
            if (kind == 2 && (!was_string || width == 1) &&
                !(symbol->type->flags & 0x840) && !(shape_flags & 1)) {
                report_syntax_error(97);
                width = initializer_address_size(reference);
            }
            INITIALIZER_BYTE(width);
            break;
        }
    }
}

/* Original 0x30c60..0x30cee. The array-declaration attribute and the
   literal's character width determine whether its bytes can be copied. */
int __stdcall is_character_array_initializer(FrontendSymbol *symbol, IrToken *value)
{
    TypeShape *shape;
    unsigned short character;
    if (!value || value->kind != TOK_STRING || !(symbol->attributes & 0x400)) return 0;
    character = (value->auxiliary_flags & 1) ? 0x42 : 0x41;
    if ((symbol->type->flags & 0x2fdf) != character) return 0;
    shape = symbol->type->related;
    if (symbol->attributes & 2) shape = shape->related;
    return shape->related == 0;
}

/* Original 0x3bd70..0x3be58. Reduce constant address arithmetic to a
   symbol plus displacement, retaining the original in-place '+' exchange. */
int __stdcall find_initializer_address(IrToken *value, FrontendSymbol **symbol, int *offset)
{
    IrToken *left, *right;
    ExpressionNode *node;
    unsigned adjustment;
    for (;;) {
        value = normalize_initializer_expression(value);
        switch (value->flags & 7) {
        case 2:
            node = (ExpressionNode *)value;
            switch (value->kind) {
            case TOK_PLUS:
                left = node->value.children.left;
                if ((left->flags & 7) == 3) {
                    node->value.children.left = node->value.children.right;
                    node->value.children.right = left;
                }
                break;
            case TOK_LEFT_BRACKET: case TOK_MINUS: case TOK_MEMBER: case TOK_ARROW:
                break;
            default: return 0;
            }
            right = node->value.children.right;
            if ((right->flags & 7) != 3 || !(right->type->flags & 0x40)) return 0;
            adjustment = right->value.integer;
            if (value->kind == TOK_MINUS) adjustment = 0U - adjustment;
            *offset += adjustment;
            value = node->value.children.left;
            break;
        case 3:
            *offset += value->value.integer;
            return 1;
        case 5:
            value = materialize_string_literal(value);
            /* The materialized literal is an ordinary symbol reference. */
        case 4:
            *symbol = value->value.pointer;
            return 1;
        default: return 0;
        }
    }
}

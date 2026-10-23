/* Literal declarations and their segmented initializer records. */
#include "initializer-records.h"
#include "global-ir.h"
#include "class-members.h"
#include "numeric-conversion.h"
#include "control-ir.h"
#include "operators.h"
#include "source-input.h"
#include <recon.h>

/* These fields choose the ordinary FILE encoder or retained work-stack
   encoding without adding graph nodes for the original inline operations. */
#define INITIALIZER_BYTE(value_) do { \
    signed char byte_ = (signed char)(value_); \
    if (auxiliary_ir_stream) putc(byte_, auxiliary_ir_stream); \
    else write_work_character(byte_, initializer_record_stack); \
} while (0)
#define INITIALIZER_INTEGER(value_) do { \
    int *integer_ = (int *)(value_); \
    if (auxiliary_ir_stream) write_ir_integer(integer_); \
    else if (*integer_ >= -127 && *integer_ <= 127) \
        write_work_character((signed char)*integer_, initializer_record_stack); \
    else { \
        write_work_character(0x80, initializer_record_stack); \
        write_work_bytes(integer_, 4, 1, initializer_record_stack); \
    } \
} while (0)
#define INITIALIZER_SHORT(value_) do { \
    short *short_ = (value_); \
    if (auxiliary_ir_stream) write_ir_short(short_); \
    else if (*short_ >= -127 && *short_ <= 127) \
        write_work_character((signed char)*short_, initializer_record_stack); \
    else { \
        write_work_character(0x80, initializer_record_stack); \
        write_work_bytes(short_, 2, 1, initializer_record_stack); \
    } \
} while (0)
#define INITIALIZER_INDEX(value_) do { \
    unsigned *index_ = (value_), word_; \
    if (auxiliary_ir_stream) write_ir_index(index_); \
    else if (*index_ <= 0x7fff) \
        write_work_bytes(index_, 2, 1, initializer_record_stack); \
    else { \
        word_ = *index_ | 0x8000; \
        write_work_bytes(&word_, 2, 1, initializer_record_stack); \
        word_ = *index_ >> 15; \
        write_work_bytes(&word_, 2, 1, initializer_record_stack); \
    } \
} while (0)

/* Original 0x5bb30..0x5bb99, stdcall8. The terminating marker is
   suppressed under the same parser mode as the literal initializer. */
void __stdcall write_initializer_record(CppSymbol *symbol, IrToken *value)
{
    write_literal_initializer(symbol, value, line_file);
    if (!(cpp_parser_flags & 0x20000000UL)) {
        auxiliary_ir_stream = line_file;
        initializer_record_marker = 7;
        putc((signed char)initializer_record_marker, auxiliary_ir_stream);
    }
}

/* Original 0x5bba0..0x5bdc6, stdcall12. The immutable first record
   schema is marker, declaration index, zero offset, then initializer value.
   Keep its ordinary format literal here; its pointer identity is not used. */
void __stdcall write_literal_initializer(CppSymbol *symbol, IrToken *value,
    FILE *file)
{
    char *format = "mdox";
    unsigned index, word;
    int offset;
    if (cpp_parser_flags & 0x20000000UL) return;
    auxiliary_ir_stream = file;
    if (!format) return;
    while (*format) {
        switch (*format++) {
        case 'd':
            if (symbol->attributes & 0x40) {
                if ((symbol->kind & 31) == 3)
                    static_member_alias(symbol)->attributes |= 4;
                else
                    symbol_alias(symbol)->attributes |= 4;
                if ((symbol->kind & 31) == 3)
                    index = static_member_alias(symbol)->index;
                else
                    index = symbol_alias(symbol)->index;
            } else index = symbol->index;
            if (auxiliary_ir_stream) write_ir_index(&index);
            else if (index <= 0x7fff)
                write_work_bytes(&index, 2, 1, initializer_record_stack);
            else {
                word = index | 0x8000;
                write_work_bytes(&word, 2, 1, initializer_record_stack);
                word = index >> 15;
                write_work_bytes(&word, 2, 1, initializer_record_stack);
            }
            symbol->attributes |= 4;
            symbol->attributes |= 0x8000;
            break;
        case 'm':
            if (auxiliary_ir_stream) putc(0, auxiliary_ir_stream);
            else write_work_character(0, initializer_record_stack);
            break;
        case 'o':
            offset = 0;
            if (auxiliary_ir_stream) write_ir_integer(&offset);
            else if (offset >= -127 && offset <= 127)
                write_work_character((signed char)offset, initializer_record_stack);
            else {
                write_work_character(0x80, initializer_record_stack);
                write_work_bytes(&offset, 4, 1, initializer_record_stack);
            }
            break;
        case 'x':
            write_initializer_value(symbol, value, file);
            break;
        }
    }
}

/* Original 0x5be80..0x5c805, alphabet dispatch 0x5c808..0x5c85b.
   Recover relocation, string, integer and extended-value records on both
   output paths. The schema letters stay at their interpretation point.
   If address classification fails, the original leaves kind unwritten;
   width and the source numeric category are likewise established by their
   preceding schema/type branches, rather than synthesized defaults. */
void __stdcall write_initializer_value(CppSymbol *symbol, IrToken *value,
    FILE *file)
{
    unsigned kind, width, index;
    int offset = 0, was_string = 0, integral, unsigned_value, status;
    long integer_value, number;
    short length, short_value;
    unsigned short representation = 0, flags, shape_flags;
    unsigned char byte_value;
    CppSymbol *reference = 0;
    TypeShape *shape;
    CType *type;
    char *format, *text;
    float single_value;
    double double_value;
    Extended extended_value;
    if (!value || (cpp_parser_flags & 0x20000000UL)) return;
    if (pending_initializer_symbol && !file) {
        write_literal_initializer(pending_initializer_symbol, 0, 0);
        pending_initializer_symbol = 0;
        token_allocation_flags |= 0x100000;
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
        else {
            value = make_integer_constant(offset);
            kind = 1;
        }
    }
    /* Logical fields of all thirteen original shared schemas. No string
       address identity or row mutation is used by this interpreter. */
    switch (kind) {
    case 0: format = "mdox"; break;
    case 1: format = "tpzv"; break;
    case 2: format = "tsfz"; break;
    case 3: format = "tl"; break;
    case 4: format = "ti"; break;
    case 5: format = "tr"; break;
    case 6: case 7: format = "t"; break;
    case 8: format = "tn"; break;
    case 9: format = "td"; break;
    case 10: format = "mox"; break;
    case 11: format = "tb"; break;
    case 12: format = "tg"; break;
    }
    auxiliary_ir_stream = file;
    if (!format) return;
    while (*format) {
        switch (*format++) {
        case 'f': INITIALIZER_INTEGER(&offset); break;
        case 'i': INITIALIZER_BYTE(0); break;
        case 'l':
            text = value->value.text;
            length = (short)value->tail.string.length;
            if (auxiliary_ir_stream) write_counted_ir(length, text);
            else {
                INITIALIZER_SHORT(&length);
                write_work_bytes(text, length, 1, initializer_record_stack);
            }
            break;
        case 'n':
            number = (long)value->value.integer;
            INITIALIZER_INTEGER(&number);
            break;
        case 'p':
            type = symbol->type;
            shape = type->related;
            if (symbol->attributes & 2) shape = shape->related;
            if (shape && (shape->flags & 0x21)) representation = 3;
            else if (shape && (shape->flags & 0x10)) representation = 1;
            else {
                flags = type->flags;
                if ((flags & 0x4000) ||
                    (options.configuration.option_J && (flags & 0x2fdf) == 0x41 &&
                     !(flags & 0x8000))) representation = 2;
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
            if ((flags & 0x40) || (type->related && (type->related->flags & 0x11)) ||
                (flags & 0x200)) {
                integral = 1;
                unsigned_value = (flags & 0x4000) ||
                    (options.configuration.option_J && (flags & 0x2fdf) == 0x41 &&
                     !(flags & 0x8000));
                integer_value = (long)value->value.integer;
            } else if (flags & 0x100) integral = 0;
            if (representation == 5) {
                if (integral) {
                    if (unsigned_value)
                        status = unsigned_to_extended((unsigned long *)&integer_value, &extended_value);
                    else status = integer_to_extended(&integer_value, &extended_value);
                    if (status) compiler_warning(2, 56);
                }
                if (width == 8) {
                    if (extended_to_double(integral ? &extended_value : value->value.pointer,
                                           &double_value)) compiler_warning(2, 56);
                    if (auxiliary_ir_stream) write_ir_double(&double_value);
                    else write_work_bytes(&double_value, 8, 1, initializer_record_stack);
                } else if (width == 4) {
                    if (extended_to_float(integral ? &extended_value : value->value.pointer,
                                          &single_value)) compiler_warning(2, 56);
                    if (auxiliary_ir_stream) write_ir_float(&single_value);
                    else write_work_bytes(&single_value, 4, 1, initializer_record_stack);
                } else {
                    if (!integral) extended_value = *(Extended *)value->value.pointer;
                    if (auxiliary_ir_stream) {
                        fwrite(&extended_value, 10, 1, auxiliary_ir_stream);
                        fwrite(&extended_value.format, 2, 1, auxiliary_ir_stream);
                    } else {
                        write_work_bytes(&extended_value, 10, 1, initializer_record_stack);
                        write_work_bytes(&extended_value.format, 2, 1, initializer_record_stack);
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
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            shape_flags = shape ? shape->flags : 0;
            if ((shape_flags & 0x77) == 2) width = symbol_element_size(symbol);
            else width = type_size(symbol->type);
            if (kind == 2 && (!was_string || width == 1) &&
                !(symbol->type->flags & 0x840) && !(shape_flags & 0x71)) {
                report_syntax_error(97);
                width = initializer_address_size(reference);
            }
            INITIALIZER_BYTE(width);
            break;
        }
    }
}

/* Original 0x5c860..0x5c991, stdcall12. Reduce constant address
   arithmetic to its symbol and wrapping byte displacement. Preserve both
   the '+' operand exchange and internal null-based adjustment mutation. */
int __stdcall find_initializer_address(IrToken *value, CppSymbol **symbol,
    int *offset)
{
    IrToken *left, *right;
    unsigned adjustment;
    for (;;) {
        value = normalize_initializer_expression(value);
        switch (value->flags & 7) {
        case 2:
            switch (value->kind) {
            case OP_ADD:
                left = value->value.pointer;
                if ((left->flags & 7) == 3) {
                    value->value.pointer = value->tail.right;
                    value->tail.right = left;
                }
                break;
            case OP_SUBSCRIPT: case OP_SUBTRACT: case 0x59: case OP_ARROW:
                break;
            case 0xa9:
                left = value->value.pointer;
                right = value->tail.right;
                if ((left->flags & 7) == 3 && !left->value.integer &&
                    (right->flags & 7) == 3 && (right->type->flags & 0x40))
                    right->value.integer = 0;
                break;
            case 0x40:
            default:
                return 0;
            }
            right = value->tail.right;
            if ((right->flags & 7) != 3 || !(right->type->flags & 0x40)) return 0;
            adjustment = right->value.integer;
            if (value->kind == OP_SUBTRACT) adjustment = 0U - adjustment;
            *offset = (int)((unsigned)*offset + adjustment);
            value = value->value.pointer;
            break;
        case 3:
            *offset = (int)((unsigned)*offset + value->value.integer);
            return 1;
        case 5:
            value = materialize_string_literal(value);
            /* Materialization returns an ordinary symbol reference. */
        case 4:
            *symbol = value->value.pointer;
            return 1;
        default:
            return 0;
        }
    }
}

/* Original 0x91358: shared byte emitted after a complete initializer block. */
unsigned char initializer_record_marker;

/* Original 0x5ba40..0x5bb20, cdecl. Drain the segmented initializer stack
   into line records in payload-sized blocks, then restore its empty writer. */
void flush_initializer_records(void)
{
    char buffer[2040];
    unsigned bytes;
    if (!(cpp_parser_flags & 0x20000000UL)) {
        finish_work_stack(initializer_record_stack);
        seek_work_stack(initializer_record_stack, 0, 0);
        while ((bytes = read_work_stack(initializer_record_stack, buffer,
                sizeof(buffer))) == sizeof(buffer))
            fwrite(buffer, sizeof(buffer), 1, line_file);
        fwrite(buffer, bytes, 1, line_file);
        if (!(cpp_parser_flags & 0x20000000UL)) {
            auxiliary_ir_stream = line_file;
            initializer_record_marker = 7;
            putc((signed char)initializer_record_marker, auxiliary_ir_stream);
        }
    }
    initialize_work_stack(&initializer_record_stack);
}

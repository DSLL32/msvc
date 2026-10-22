#include "tree-rewrite.h"
#include "expression-fields.h"
#include "records.h"
#include "algebra.h"
extern int optimize_precision;
/* Original [00057850,000579e0). Scalar conversions retain the compiler's
   twelve-byte extended representation until the final leaf is interned. */
Node *__fastcall fold_constant_conversion(Node *node)
{
    Node *operand = node->left.node, *result;
    unsigned short target = node->attributes.words.type;
    unsigned short source = operand->attributes.words.type;
    union {
        unsigned integer;
        float single;
        DoubleBits double_bits;
    } scalar;
    Extended number;
    unsigned payload;
    void (*convert_integer)(const void *, Extended *);
    if (target & 0xf00) {
        if (source & 0xf00) scalar.integer = convert_integer_constant(node, operand);
        else if (extended_to_integer(operand->left.pointer, (int *)&scalar.integer))
            compiler_warning(2, 0x2f4);
        if (target == 0x408 && (source & 255) < 3 && !is_constant_value(operand, 0)) {
            lower_far_pointer_cast(node);
            return node;
        }
        payload = scalar.integer;
    } else {
        if (source & 0x300) {
            scalar.integer = operand->left.value;
            convert_integer = source == 0x104 ?
                (void (*)(const void *, Extended *))integer_to_extended :
                (void (*)(const void *, Extended *))unsigned_integer_to_extended;
            (*convert_integer)(&scalar.integer, &number);
        } else {
            number = *(Extended *)operand->left.pointer;
            if (optimize_precision) {
                if (target == 0x1004) {
                    if (extended_to_float(&number, &scalar.single)) compiler_warning(2, 0x2f4);
                    float_to_extended(&scalar.single, &number);
                } else if (target == 0x1008 && source == 0x100a) {
                    if (extended_to_double(&number, &scalar.double_bits)) compiler_warning(2, 0x2f4);
                    double_to_extended(&scalar.double_bits, &number);
                }
            }
        }
        payload = (unsigned)&number;
    }
    result = intern_leaf(0x33, 0, target, payload);
    release_tree(node);
    return result;
}

/* Original [000579e0,00057a50). Widen signed byte/word constants before
   applying the destination width. The original leaves its temporary value
   unset for a signed source wider than two bytes when widening further. */
unsigned __fastcall convert_integer_constant(Node *conversion, Node *operand)
{
    unsigned value;
    unsigned short source = operand->attributes.words.type;
    if ((source & 0x100) &&
        (conversion->attributes.words.type & 255) > (source & 255)) {
        switch (source & 255) {
        case 1: value = (int)(signed char)operand->left.value; break;
        case 2: value = (int)(short)operand->left.value; break;
        }
    } else value = operand->left.value;
    return normalize_integer_width(conversion, value);
}


/* Original [00057810,00057850). Integer constants keep their destination
   signedness when narrowing to the two sub-word widths. */
unsigned __fastcall normalize_integer_width(Node *node, unsigned value)
{
    switch (node->attributes.words.type) {
    case 0x101: return (int)(signed char)value;
    case 0x102: return (int)(short)value;
    case 0x201: return (unsigned char)value;
    case 0x202: return (unsigned short)value;
    default: return value;
    }
}

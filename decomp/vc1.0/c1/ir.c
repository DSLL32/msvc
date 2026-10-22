/* Expression-stream records. Buffer operations stay inside their original caller. */
#include "global-ir.h"
#include "parser.h"
#include "expression-records.h"

typedef struct IrRecordLink { struct IrRecordLink *next; } IrRecordLink;

unsigned string_record_count; /* original 0x60040 */
FrontendSymbol *current_label_symbol; /* original 0x60068 */

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

/* Original 0x395f0..0x39c68, dispatch tables 0x39c68..0x39d10. */
void __stdcall write_ir_record(unsigned tag, void *value)
{
    IrToken *node = value;
    unsigned char *format;
    FILE *file;
    CType *type;
    TypeShape *shape;
    FrontendSymbol *symbol;
    unsigned index, storage;
    unsigned short attributes;
    putc(tag, expression_file);
    ++function_ir.complexity;
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
            shape = ((FrontendSymbol *)node->value.pointer)->type->related;
            if (shape && !(shape->flags & 7))
                shape = shape->related;
            attributes = shape->attributes;
            if ((attributes & 7) == 1) {
                function_call_seen = 1;
                inline_candidate &= ((function_symbol->type->related->attributes & 7) == 1);
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
                    if ((shape->flags & 7) != 4)
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
                index = ((FrontendSymbol *)node->value.pointer)->index;
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
        }
    }
}

/* Original 0x393a0..0x395ef. Recursively serialize an expression, with
   explicit entry, call-argument, and switch-list record boundaries. */
void __stdcall emit_ir_expression(IrToken *expression)
{
    ExpressionNode *node;
    FrontendSymbol *symbol;
    IrRecordLink *link;
    unsigned record, rule;
    if (current_function) return;
    while (expression) {
        node = (ExpressionNode *)expression;
        switch (node->kind) {
        case TOK_CAST:
            if (node->type->flags == 0x80 && !node->type->related) {
                expression = node->value.children.left;
                if (current_function) return;
                continue;
            }
            break;
        case TOK_STRING:
            expression = emit_version_token(expression);
            WRITE_EXPRESSION_RECORD(expression);
            return;
        case TOK_BASED_REFERENCE:
            if (!node->value.children.right)
                node->value.children.right = make_typed_unary_expression(0x2a, type_4064_first, 0);
            break;
        case TOK_ENTRY:
            emit_ir_expression(node->value.children.left);
            symbol = (FrontendSymbol *)node->value.children.left->value.pointer;
            --symbol->references;
            WRITE_EXPRESSION_RECORD(expression);
            for (symbol = current_parameter; symbol; symbol = symbol->next)
                if (symbol->type && !(symbol->flags & 2) &&
                    ((symbol->kind & 0x1f) == 10 || (symbol->kind & 0x1f) == 11))
                    write_ir_record(0x2d, make_symbol_reference(symbol));
            write_ir_record(0x4c, 0);
            return;
        case TOK_INTRINSIC:
            if (emit_intrinsic_expression(expression)) return;
            /* An unexpanded intrinsic has the ordinary argument-list form. */
        case TOK_ARGUMENT_END:
            emit_ir_expression(node->value.children.left);
            WRITE_EXPRESSION_RECORD(expression);
            node = (ExpressionNode *)node->value.children.right;
            while (node) {
                emit_ir_expression(node->value.children.left);
                write_ir_record(0x55, node);
                node = (ExpressionNode *)node->value.children.right;
            }
            write_ir_record(0x4c, 0);
            return;
        case TOK_SWITCH:
            emit_ir_expression(node->value.children.left);
            WRITE_EXPRESSION_RECORD(expression);
            link = (IrRecordLink *)node->value.children.right;
            while (link) {
                write_ir_record(0x3d, link);
                link = link->next;
            }
            write_ir_record(0x4c, 0);
            return;
        }
        switch (node->flags & 7) {
        case 0:
            if (node->flags & 0x40) {
                report_syntax_error(124);
                return;
            }
            break;
        case 1:
            emit_ir_expression(node->value.children.left);
            break;
        case 2:
            emit_ir_expression(node->value.children.left);
            emit_ir_expression(node->value.children.right);
            break;
        }
        WRITE_EXPRESSION_RECORD(expression);
        return;
    }
}

/* Original 0x3a040..0x3a0a1. References account for aliases and disable
   inlining when a declaration's parameter context requires it. */
unsigned get_ir_symbol_index(FrontendSymbol *symbol)
{
    ++symbol->references;
    if (symbol->attributes & 0x40) {
        symbol = symbol_alias(symbol);
        ++symbol->references;
    }
    if ((symbol->attributes & 2) &&
        ((FrontendSymbol *)symbol->type->related->field08)->index == 5 &&
        function_symbol && !(function_symbol->storage[1] & 1)) {
        function_call_seen = 1;
        inline_candidate = 0;
    }
    return symbol->index;
}

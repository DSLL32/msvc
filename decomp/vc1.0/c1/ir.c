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
            /* This operand is the callee expression, not its symbol payload. */
            shape = ((IrToken *)node->value.pointer)->type->related;
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
    if (error_count) return;
    while (expression) {
        node = (ExpressionNode *)expression;
        switch (node->kind) {
        case TOK_CAST:
            if (node->type->flags == 0x80 && !node->type->related) {
                expression = node->value.children.left;
                if (error_count) return;
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

/* Original 0x32f10..0x33013. Check a return value, serialize it with the
   function's result type, and branch to the common exit label. */
void __stdcall emit_return_statement(IrToken *value, int explicit_return)
{
    CType *type;
    int special_function, void_result;
    if (!function_return_type) return;
    special_function = function_symbol->storage[2] & 1;
    if (special_function && explicit_return) {
        report_syntax_error(490, "return");
        return;
    }
    void_result = function_return_type->flags == 0x80 && !function_return_type->related;
    if (!value) {
        if (!void_result && !special_function) compiler_warning(1, 33, function_symbol->name);
    } else if (void_result) {
        compiler_warning(1, 98, function_symbol->name);
        emit_expression_statement(value);
    } else {
        type = function_return_type;
        check_assignment_conversion(TOK_RETURN, type, value);
        if ((scanner_flags & 0x800) && type->related && (type->related->attributes & 12))
            value = convert_based_pointer(type, value);
        emit_ir_expression(value);
        value->type = type;
        write_ir_record(0x41, value);
        parser_flags |= 0x8000;
    }
    write_ir_record(0x3a, function_exit_label);
}

/* Original 0x3e320..0x3e70e. Check assignment/return compatibility and
   issue the frontend's arithmetic precision and signedness diagnostics. */
int __stdcall check_assignment_conversion(unsigned char operation, CType *target, IrToken *value)
{
    int result, void_count, target_void;
    int float_integer = 0, narrower = 0, promoted = 0, source_unsigned;
    unsigned short destination_flags, source_flags, target_unsigned;
    unsigned constant;
    char *context_name;
    if ((value->flags & 0x80) && target->related && (target->related->flags & 3)) return 1;
    result = check_type_conversion(operation, target, value->type);
    if (target->related || value->type->related) return result;
    target_void = !!(target->flags & 0x80);
    void_count = target_void + !!(value->type->flags & 0x80);
    if (operation == TOK_COLON) {
        if (void_count == 1) compiler_warning(4, 53);
    } else if (void_count == 1 && operation == TOK_RETURN) {
        if (target_void) {
            if (options.z_a) compiler_warning(1, 98, function_symbol->name);
            else report_syntax_error(562, function_symbol->name);
        } else if (options.z_a) compiler_warning(1, 70, function_symbol->name);
        else if (!(function_symbol->storage[2] & 1)) report_syntax_error(561, function_symbol->name);
    } else if (void_count == 1 || void_count == 2) report_syntax_error(120);
    switch (operation) {
    case TOK_ASSIGN: context_name = "="; break;
    case TOK_DIVIDE_ASSIGN: context_name = "/="; break;
    case TOK_MINUS_ASSIGN: context_name = "-="; break;
    case TOK_MULTIPLY_ASSIGN: context_name = "*="; break;
    case TOK_PLUS_ASSIGN: context_name = "+="; break;
    case TOK_ARGUMENT_END: context_name = "argument"; break;
    case TOK_INITIALIZING: context_name = "initializing"; break;
    case TOK_RETURN: context_name = "return"; break;
    default: return result;
    }
    destination_flags = target->flags;
    source_flags = value->type->flags;
    if (!(destination_flags & 0x140) || !(source_flags & 0x140)) return result;
    if (source_flags & 0x100) {
        if (destination_flags & 0x40) float_integer = 1;
        else if ((source_flags & 15) > (destination_flags & 15)) narrower = 1;
        else if ((source_flags & 15) == (destination_flags & 15) &&
            (source_flags & 0x20) && (!(destination_flags & 0x100) || !(destination_flags & 0x20))) promoted = 1;
    } else if (destination_flags & 0x100) {
        if ((source_flags & 15) >= (destination_flags & 15)) float_integer = 1;
    } else if ((value->flags & 7) == 3) {
        constant = value->value.integer;
        target_unsigned = destination_flags & 0x4000;
        source_unsigned = (source_flags & 0x4000) ||
            (!options.z_a && options.option_J && (source_flags & 0x2fdf) == 0x41 && !(source_flags & 0x8000));
        if (target_unsigned && !source_unsigned && (int)constant < 0) compiler_warning(3, 18, context_name);
        if (source_flags != destination_flags && integer_conversion_overflow(
                (unsigned short)(destination_flags & 15), target_unsigned, (unsigned short)source_unsigned, constant))
            compiler_warning(2, 309, context_name);
    } else if ((source_flags & 15) > (destination_flags & 15)) {
        if ((source_flags & 0x20) && (source_flags & 0x40) &&
            (type_0064->flags & 15) >= (destination_flags & 15)) promoted = 1;
        else narrower = 1;
    } else if ((source_flags & 15) == (destination_flags & 15) &&
        !((source_flags & 0x20) && (source_flags & 0x40)) &&
        (destination_flags & 0x20) && (destination_flags & 0x40)) promoted = 1;
    if (float_integer) compiler_warning(2, 51);
    else if (narrower || promoted) compiler_warning(narrower ? 3 : 4, (destination_flags & 0x100) ? 136 : 135);
    return result;
}

/* Original 0x365f0..0x36955. Encode a node's base kind, width, alignment,
   and qualifiers; aggregates with no size nibble carry a separate size. */
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode)
{
    FILE *file = expression_file;
    IrToken *typed_node = tag == 0x27 ? node->value.pointer : node;
    CType *type;
    TypeShape *shape;
    unsigned short flags, attributes, kind, alignment, qualifiers, size_nibble, encoded;
    unsigned shifted_alignment;
    unsigned short alignment_value;
    int bytes;
    if (error_count) return;
    type = typed_node->type;
    shape = type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (shape) {
        attributes = shape->attributes;
        kind = (attributes & 7) == 3 ? IR_TYPE_FAR16_POINTER : IR_TYPE_POINTER;
        bytes = pointer_type_size(attributes);
        alignment_value = node_alignment(node);
    } else {
        attributes = type->attributes;
        flags = type->flags;
        alignment_value = node_alignment(node);
        if (flags & 0x40) {
            kind = (flags & 0x4000) || (!options.z_a && options.option_J &&
                (flags & 0x2fdf) == 0x41 && !(flags & 0x8000)) ? IR_TYPE_UNSIGNED : IR_TYPE_SIGNED;
            bytes = flags & 15;
        } else if (flags & 0x100) {
            kind = mode == 1 ? IR_TYPE_EXTENDED : IR_TYPE_FLOAT;
            bytes = mode == 1 ? 10 : flags & 15;
        } else if (flags & 0x400) {
            kind = IR_TYPE_AGGREGATE;
            bytes = tag == 0x27 ? 0 : complete_type_size(type);
        } else if (flags & 0x80) {
            kind = IR_TYPE_VOID;
            bytes = 0;
        } else if (flags & 0x800) {
            kind = IR_TYPE_UNSIGNED;
            bytes = 2;
        } else {
            kind = IR_TYPE_UNKNOWN;
            bytes = 0;
        }
    }
    shifted_alignment = alignment_value >> 1;
    alignment = 0;
    while (shifted_alignment) { ++alignment; shifted_alignment >>= 1; }
    if (++alignment > 5) alignment = 0;
    alignment <<= 9;
    qualifiers = ((attributes & 0x2000) ? 0x1000 : 0) | ((attributes & 0x1000) ? 0x2000 : 0);
    size_nibble = kind == IR_TYPE_AGGREGATE && (unsigned)bytes >= 32 ? 0 : (unsigned short)bytes;
    size_nibble <<= 4;
    encoded = kind | size_nibble | qualifiers | alignment;
    if (encoded > 0x7f) putc((encoded >> 8) | 0x80, file);
    putc((unsigned char)encoded, file);
    if (kind == IR_TYPE_AGGREGATE && !size_nibble) PUT_IR_INTEGER(expression_file, bytes);
}

/* Original 0x36540..0x365f0. Member/subscript expressions take the smaller
   child alignment; selected unary and assignment wrappers expose their child. */
unsigned short __stdcall node_alignment(IrToken *node)
{
    ExpressionNode *selected = (ExpressionNode *)node;
    unsigned char category = token_categories[node->kind];
    unsigned short left_alignment, right_alignment;
    switch (category) {
    case TOK_ARGUMENT: case TOK_ASSIGNMENT_OPERATOR: case TOK_ASSIGN:
    case TOK_DEREFERENCE: case TOK_INCREMENT_OPERATOR: case TOK_INITIALIZING:
        selected = (ExpressionNode *)selected->value.children.left;
        break;
    }
    if (token_categories[selected->kind] == TOK_BIT_FIELD)
        selected = (ExpressionNode *)selected->value.children.left;
    if (token_categories[selected->kind] == TOK_MEMBER_OPERATOR) {
        left_alignment = expression_type_alignment(selected->value.children.left->type);
        right_alignment = expression_type_alignment(selected->value.children.right->type);
        if (left_alignment >= right_alignment)
            return expression_type_alignment(selected->value.children.right->type);
        return expression_type_alignment(selected->value.children.left->type);
    }
    return expression_type_alignment(node->type);
}

/* Original 0x364c0..0x364fd. This entry is distinct from the identical
   global-record helper at 0x36500 and retains its own call-graph identity. */
unsigned short __stdcall expression_type_alignment(CType *type)
{
    if (type->related && (type->related->flags & 7) != 2) return 4;
    if (type->flags & 0x400)
        return (unsigned short)((((FrontendSymbol *)type->field08)->field24 >> 16) & 0xff);
    return type->flags & 15;
}

/* Original 0x6006c..0x60078: three structured-exception nesting depths.
   Their individual roles will be named as the corresponding entries are met. */
int exception_scope_depth[3];

/* Original 0x39ed0..0x39f63. Track the smallest nesting depth of a forward
   label reference and diagnose a branch into a protected declaration. */
void __stdcall reference_ir_symbol(FrontendSymbol *symbol)
{
    int depth;
    char *name;
    if (symbol->flags & 1) {
        if (symbol->attributes & 0x200) {
            if (symbol->detail.value == 19) name = "case";
            else if (symbol->detail.value == 20) name = "default";
            else name = symbol->name;
            if (!(parser_flags & 0x80000000U)) report_syntax_error(705, name);
        }
    } else {
        depth = exception_scope_depth[2] + exception_scope_depth[1] + exception_scope_depth[0];
        if (!symbol->references || depth < (int)((symbol->field24 >> 8) & 0xff))
            symbol->field24 = (symbol->field24 & 0xffff00ffU) | ((unsigned char)depth << 8);
    }
    if (symbol->flags & 1) symbol->attributes |= 0x400;
}

/* Original 0x39e60..0x39ec8. Define a label at the current statement and
   exception depths, checking the depths recorded by forward references. */
void __stdcall emit_code_label(IrToken *label)
{
    FrontendSymbol *symbol = label->value.pointer;
    int depth;
    symbol->flags |= 1;
    symbol->field24 = (symbol->field24 & 0xffffff00U) | parameter_nesting;
    depth = exception_scope_depth[2] + exception_scope_depth[1] + exception_scope_depth[0];
    if (symbol->references && depth > (int)((symbol->field24 >> 8) & 0xff))
        report_syntax_error(705, symbol->name);
    symbol->field24 = (symbol->field24 & 0xffff00ffU) | ((unsigned char)depth << 8);
    write_ir_record(0x29, label);
}

/* Function-directory blocks, original roots 0x6b63c/0x6b640 and index
   0x6b7e4. Each block holds twenty typed entries and one link. */
FunctionRecordBlock *function_records, *last_function_records;
unsigned function_record_index = 19;

/* Original 0x3a0b0..0x3a428. Retain the function's stream positions and
   optimization state for the final directory, and mark it in the line stream. */
void __stdcall record_function_definition(FrontendSymbol *symbol)
{
    FunctionRecordBlock *block;
    FunctionRecord *record;
    unsigned index;
    if (!symbol) return;
    function_record_index = (function_record_index + 1) % 20;
    if (!function_record_index) {
        block = allocate_permanent(sizeof(FunctionRecordBlock));
        if (!function_records) function_records = last_function_records = block;
        else { last_function_records->next = block; last_function_records = block; }
    }
    record = &last_function_records->records[function_record_index];
    record->symbol = symbol;
    if (pch_write_enabled && !pch_output_written) {
        record->flags |= 0x10;
        if (pch_read_enabled) {
            function_ir.expression_offset += pch_expression_bytes + 0x334;
            function_ir.symbol_offset += pch_symbol_bytes;
        }
    } else record->flags &= ~0x10;
    record->expression_offset = function_ir.expression_offset;
    record->symbol_offset = function_ir.symbol_offset;
    record->complexity = function_ir.complexity;
    record->flags = (record->flags & ~15) | (inline_candidate & 1) |
        ((function_call_seen << 1) & 2) | ((function_not_inlinable << 2) & 4) | ((normalized_Oq << 3) & 8);
    record->parameter_count = function_ir.parameter_count;
    function_ir.complexity = 0;
    if (parser_flags & 0x20000000) return;
    putc(9, line_file);
    if (function_symbol) {
        index = function_symbol->index;
        if (index > 0x7fff) { PUT_IR_WORD(line_file, index | 0x8000); index >>= 15; }
        PUT_IR_WORD(line_file, index);
    } else {
        putc(0, line_file);
        putc(0, line_file);
    }
}

/* Original 0x3a430..0x3a641. Emit retained function definitions after
   resolving their final linkage, reference counts, and inline attributes. */
void write_function_directory(void)
{
    unsigned saved_Oq = normalized_Oq;
    unsigned count, index;
    FunctionRecord *record;
    FrontendSymbol *symbol;
    while (function_records) {
        count = function_records == last_function_records ? function_record_index + 1 : 20;
        for (index = 0; index < count; ++index) {
            record = &function_records->records[index];
            symbol = record->symbol;
            function_ir.expression_offset = record->expression_offset;
            function_ir.symbol_offset = record->symbol_offset;
            function_in_pch = (record->flags >> 4) & 1;
            function_ir.complexity = record->complexity;
            function_ir.parameter_count = record->parameter_count;
            inline_candidate = (record->flags & 1) ? -1 : 0;
            function_call_seen = (record->flags & 2) || ((symbol->type->related->attributes & 7) == 1);
            function_not_inlinable = (record->flags & 4) ? -1 : 0;
            /* The original reads Oq from the last block at the current
               within-block index, even while emitting an earlier block. */
            normalized_Oq = (last_function_records->records[index].flags & 8) ? -1 : 0;
            function_ir.force_output = 0;
            if (symbol->name == name_main || symbol->name == name_winmain || symbol->name == name_dllmain) {
                if ((symbol->storage[0] & 7) == 3) {
                    compiler_warning(3, 8, symbol->name, "static");
                    symbol->storage[0] = (symbol->storage[0] & ~6) | 1;
                }
                if (symbol->storage[1] & 1) {
                    compiler_warning(3, 8, symbol->name, "__inline");
                    symbol->storage[1] &= ~1;
                }
                symbol->attributes |= 0x8000;
                function_ir.force_output = 1;
            } else if ((symbol->storage[0] & 7) != 3) {
                if (symbol->storage[1] & 1) {
                    symbol->storage[0] |= 0x40;
                    if ((symbol->storage[1] & 0x10) ||
                        ((symbol->storage[2] & 4) && (symbol->storage[2] & 8)))
                        if (!(symbol->storage[2] & 4) || (symbol->storage[2] & 8)) function_ir.force_output = 1;
                } else function_ir.force_output = 1;
            }
            if (symbol->storage[1] & 0x80) function_ir.force_output = 1;
            if (!function_ir.force_output && !symbol->references) compiler_warning(4, 505, symbol->name);
            else {
                if (options.debug_info) {
                    emit_debug_type(symbol->type);
                    flush_debug_types();
                }
                write_global_record(14, symbol);
            }
            symbol->flags |= 2;
        }
        function_records = function_records->next;
    }
    normalized_Oq = saved_Oq;
}

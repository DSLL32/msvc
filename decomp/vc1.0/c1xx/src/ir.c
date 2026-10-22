/* Expression-stream records. Buffer operations stay inside their original caller. */
#include "ir.h"
#include "parser.h"
#include "cpp-context.h"
#include "expression-records.h"
#include "type-sizes.h"
#include "function-finalization.h"

CppSymbol *function_symbol; /* original 0x8c140 */
CppSymbol *current_label_symbol; /* original 0x91384 */
int function_call_seen; /* original 0x9fc04; shared with global serialization */
int inline_candidate = 1; /* original 0x9fc00 */
int seh_try_depth; /* original 0x91388 */
int seh_except_depth; /* original 0x9138c */
int seh_finally_depth; /* original 0x91390 */

typedef char LabelScopeDepthOffsetMustBe36[
    (offsetof(CppSymbol, auxiliary) + offsetof(SymbolReferenceState, scope_depth) == 36) ? 1 : -1];
typedef char LabelSehDepthOffsetMustBe37[
    (offsetof(CppSymbol, auxiliary) + offsetof(SymbolReferenceState, seh_depth) == 37) ? 1 : -1];

/* Original 0x5a840..0x5a8a8. A definition records both lexical and SEH
   depth before serializing the label expression itself. */
void __stdcall emit_label(IrToken *label)
{
    CppSymbol *symbol = label->value.pointer;
    int depth;
    symbol->flags |= 4;
    symbol->auxiliary.reference.scope_depth = declaration_mode;
    depth = seh_try_depth + seh_except_depth + seh_finally_depth;
    if (symbol->references && depth > symbol->auxiliary.reference.seh_depth)
        report_syntax_error(705, symbol->name);
    symbol->auxiliary.reference.seh_depth = (unsigned char)depth;
    write_ir_record(0x29, label);
}

/* Original 0x5a8b0..0x5a943. Record the shallowest SEH context in which an
   ordinary symbol was referenced; labels carry their own use/definition bits. */
void __stdcall reference_ir_symbol(CppSymbol *symbol)
{
    char *name;
    int depth;
    if (symbol->flags & 4) {
        if (symbol->attributes & 0x200) {
            if (symbol->detail.value == 0x13) name = "case";
            else if (symbol->detail.value == 0x14) name = "default";
            else name = symbol->name;
            if (!(cpp_parser_flags & 0x80000000U)) report_syntax_error(705, name);
        }
    } else {
        depth = seh_try_depth + seh_except_depth + seh_finally_depth;
        if (!symbol->references || depth < symbol->auxiliary.reference.seh_depth)
            symbol->auxiliary.reference.seh_depth = (unsigned char)depth;
    }
    if (symbol->flags & 4) symbol->attributes |= 0x400;
}

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
                index = node->linkage.record_index;
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

/* Original 0x3b540..0x3b5c3. Labels and their reference tokens both live
   in function arena one, surviving expression-arena resets. */
IrToken *__stdcall new_branch_label(unsigned attributes)
{
    ArenaPool *pool = &compiler_arena.pools[1];
    CppSymbol *label;
    IrToken *reference;
    if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
    label = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    label->kind = 8;
    label->detail.value = attributes;
    label->type = type_0064;
    register_label(label);
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    reference = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    reference->flags = 4;
    reference->kind = 2;
    reference->value.pointer = label;
    reference->type = label->type;
    return reference;
}

CppSymbol *label_symbols; /* original 0x9fd18 */

/* Original 0x3b600..0x3b67c. Diagnose ordinary undefined or unused named
   labels and serialize definitions before releasing the per-function chain. */
void finish_label_symbols(void)
{
    CppSymbol *label;
    if (!error_count) {
        for (label = label_symbols; label; label = label->next) {
            if (!(label->flags & 4) && !label->detail.value)
                report_syntax_error(94, label->name);
            else {
                if (!label->references && label->name)
                    compiler_warning(3, 102, label->name);
                if (label->flags & 4) write_global_record(3, label);
            }
        }
    }
    label_symbols = 0;
}

/* Original 0x3b3d0..0x3b42e. Labels share the ordinary symbol index
   namespace and browser rollover, then join the frontend's label chain. */
void __stdcall register_label(CppSymbol *label)
{
    if (!label->index) {
        if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
        if (next_symbol_index >= browser_symbol_limit && browser_filename)
            browser_symbol_rollover();
        label->index = next_symbol_index++;
    }
    label->next = label_symbols;
    label_symbols = label;
}

/* Original 0x5aa20..0x5aad9, cdecl. Count the actual serialized declaration
   after alias selection, and release class-context suppression on methods. */
unsigned get_ir_symbol_index(CppSymbol *symbol)
{
    CppSymbol *context, *alias;
    if ((symbol->kind & 0x1f) == 7) {
        context = cpp_context_stack->items[symbol->auxiliary.declaring_class];
        while (context) {
            context->flags &= ~8;
            context = next_member_context(context);
        }
    }
    ++symbol->references;
    if (symbol->attributes & 0x40) {
        if ((symbol->kind & 0x1f) == 3) symbol = static_member_alias(symbol);
        else symbol = symbol_alias(symbol);
        ++symbol->references;
    } else if ((symbol->kind & 0x1f) == 7 &&
               (symbol->cpp.parts.member_qualifiers & 1)) {
        alias = method_alias(symbol);
        ++alias->references;
    }
    if ((symbol->attributes & 2) &&
        ((CppSymbol *)symbol->type->related->data.based_symbol)->index == 5 &&
        function_symbol && !(function_symbol->storage[1] & 1)) {
        function_call_seen = 1;
        inline_candidate = 0;
    }
    return symbol->index;
}

/* Original 0x566b0..0x56ae2. Serialize the expression's machine type,
   representation size, swapped qualifier bits and alignment exponent. */
void __stdcall emit_record_node(unsigned tag, IrToken *node, int mode)
{
    FILE *file = expression_file;
    IrToken *typed_node = tag == 0x27 ? node->value.pointer : node;
    TypeShape *shape;
    unsigned short attributes, flags, kind, alignment, alignment_value;
    unsigned short qualifiers, size_nibble, encoded;
    unsigned shifted_alignment, bytes;
    if (error_count) return;
    shape = typed_node->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (shape && (!(shape->flags & 0x30) ||
        ((shape->flags & 0x20) && member_pointer_inheritance_model(typed_node->type) == 1))) {
        attributes = shape->attributes;
        kind = (attributes & 7) == 3 ? IR_TYPE_FAR16_POINTER : IR_TYPE_POINTER;
        bytes = pointer_kind_size(attributes);
        alignment_value = node_alignment(node);
        shifted_alignment = alignment_value >> 1;
        alignment = 0;
        while (shifted_alignment) { ++alignment; shifted_alignment >>= 1; }
        if (++alignment > 5) alignment = 0;
        alignment <<= 9;
    } else if (shape && (shape->flags & 0x30) &&
               member_pointer_is_aggregate(typed_node->type)) {
        attributes = shape->attributes;
        alignment = 0;
        if (tag == 0x27 && ((IrToken *)node->value.pointer)->kind == 0xa7) {
            kind = IR_TYPE_POINTER;
            bytes = pointer_kind_size((unsigned short)data_pointer_kind);
        } else {
            kind = IR_TYPE_AGGREGATE;
            bytes = tag == 0x27 ? 0 : complete_type_size(typed_node->type);
        }
    } else {
        attributes = typed_node->type->attributes;
        shape = typed_node->type->related;
        if (shape && (shape->flags & 0x10)) node->type = builtin_integer_type();
        flags = typed_node->type->flags;
        alignment_value = node_alignment(node);
        shifted_alignment = alignment_value >> 1;
        alignment = 0;
        while (shifted_alignment) { ++alignment; shifted_alignment >>= 1; }
        if (++alignment > 5) alignment = 0;
        alignment <<= 9;
        if (flags & 0x40) {
            kind = (flags & 0x4000) || (options.configuration.option_J &&
                (flags & 0x2fdf) == 0x41 && !(flags & 0x8000)) ?
                IR_TYPE_UNSIGNED : IR_TYPE_SIGNED;
            bytes = flags & 15;
        } else if (flags & 0x200) {
            kind = IR_TYPE_SIGNED;
            bytes = 4;
        } else if (flags & 0x100) {
            kind = mode == 1 ? IR_TYPE_EXTENDED : IR_TYPE_FLOAT;
            bytes = mode == 1 ? 10 : flags & 15;
        } else if (flags & 0x400) {
            kind = IR_TYPE_AGGREGATE;
            bytes = tag == 0x27 ? 0 : complete_type_size(typed_node->type);
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
    qualifiers = ((attributes & 0x2000) ? 0x1000 : 0) |
                 ((attributes & 0x1000) ? 0x2000 : 0);
    size_nibble = kind == IR_TYPE_AGGREGATE && bytes >= 32 ? 0 : (unsigned short)bytes;
    size_nibble <<= 4;
    encoded = alignment | qualifiers | kind | size_nibble;
    if (encoded > 0x7f) putc((encoded >> 8) | 0x80, file);
    putc((unsigned char)encoded, file);
    if (kind == IR_TYPE_AGGREGATE && !size_nibble)
        PUT_IR_INTEGER(expression_file, bytes);
}

/* Original 0x56600..0x566b0. Selected wrappers expose their left operand;
   member expressions use the smaller alignment of the two child types. */
unsigned short __stdcall node_alignment(IrToken *node)
{
    IrToken *selected = node;
    unsigned char category = token_categories[node->kind];
    unsigned short left, right;
    switch (category) {
    case 0x6a: /* argument */
    case 0x6b: /* assignment operators */
    case 0x3d: /* assignment */
    case 0x46: /* dereference */
    case 0x7d: /* increment/decrement */
    case 0x7e: /* initialization */
        selected = selected->value.pointer;
        break;
    }
    if (token_categories[selected->kind] == 0x3e)
        selected = selected->value.pointer;
    if (token_categories[selected->kind] == 0x8e) {
        left = expression_type_alignment(((IrToken *)selected->value.pointer)->type);
        right = expression_type_alignment(selected->tail.right->type);
        if (left < right)
            return expression_type_alignment(((IrToken *)selected->value.pointer)->type);
        return expression_type_alignment(selected->tail.right->type);
    }
    return expression_type_alignment(node->type);
}

/* Original 0x56560..0x565a9. This expression entry is separate from the
   global-record alignment helper at 0x565b0, despite its identical body. */
typedef char ExpressionClassAlignmentOffsetMustBe50[
    (offsetof(ClassInfo, alignment_flags) == 50) ? 1 : -1];
unsigned short __stdcall expression_type_alignment(CType *type)
{
    CppSymbol *tag;
    if (type->related && (type->related->flags & 0x77) != 2) return 4;
    if (type->flags & 0x400) {
        tag = cpp_context_stack->items[type->field08];
        return tag->detail.class_info->alignment_flags & 0x3f;
    }
    return type->flags & 15;
}

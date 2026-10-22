/* Declaration and aggregate initializer state. */
#include "initializers.h"
#include "initializer-records.h"
#include "literal-names.h"
#include "global-ir.h"

FrontendSymbol *initializer_symbol; /* original 0x6023c */
unsigned initializer_offset; /* original 0x60244 */
int initializer_runtime_override; /* original 0x6045c */
unsigned initializer_reset_words[3]; /* original 0x6cd18..0x6cd24 */

/* Original 0x2fd30..0x2fda9. Automatic/register declarations select the
   runtime initializer path; static storage starts with constant output. */
FrontendSymbol *__stdcall prepare_initializer_symbol(FrontendSymbol *symbol)
{
    unsigned char storage;
    initializer_runtime_override = 0;
    scanner_flags &= ~0x100000UL;
    if (parameter_nesting) {
        storage = symbol->storage[0] & 7;
        if (storage >= 4 && storage <= 6) scanner_flags |= 0x200000;
        else scanner_flags &= ~0x200000UL;
    } else scanner_flags &= ~0x200000UL;
    initializer_symbol = symbol;
    initializer_offset = 0;
    initializer_reset_words[0] = 0;
    initializer_reset_words[1] = 0;
    initializer_reset_words[2] = 0;
    return symbol;
}

InitializerFrameStorage initializer_frame_storage; /* original 0x6024c..0x6045c */
int initializer_depth = -1; /* original 0x6cd14 */
CType *initializer_inferred_type; /* original 0x60230 */

/* Original 0x300c0..0x3019a. Start a fresh aggregate traversal and
   defer static initializer output until its first value is encountered. */
void __stdcall begin_aggregate_initializer(FrontendSymbol *symbol)
{
    int sized_array;
    unsigned char storage;
    if (symbol) {
        initializer_error_reported = 0;
        initializer_depth = -1;
        initializer_inferred_type = 0;
        sized_array = (symbol->attributes & 0x500) == 0x500;
        initializer_symbol = symbol;
        initializer_offset = 0;
        push_initializer_frame(symbol);
        initializer_frame->flags = 1;
        if (is_scalar_initializer_symbol(initializer_frame->symbol) || sized_array)
            initializer_frame->flags |= 4;
        storage = initializer_frame->symbol->storage[0] & 7;
        if (!parameter_nesting || storage < 4 || storage > 6) {
            pending_initializer_symbol = symbol;
            if (browser_filename) {
                browser_begin_function_scope();
                browser_function_parameters(pending_initializer_symbol);
            }
        }
    }
    scanner_flags &= ~0x100000UL;
}

/* Original 0x6c070: sizes for symbol kinds 0..19. Bitfields and enum
   constants have shorter records than the complete declaration variant. */
unsigned initializer_symbol_copy_sizes[20] = {
    44, 44, 40, 38, 44, 28, 36, 32, 40, 44,
    36, 44, 44, 32, 44, 24, 16, 16, 16, 44
};

/* Original 0x30d40..0x30e3d. Each level reuses a persistent symbol/type
   pair while copying only the active symbol variant's declared prefix. */
void __stdcall push_initializer_frame(FrontendSymbol *symbol)
{
    unsigned bytes;
    unsigned char *source, *destination;
    initializer_frame = &initializer_frames[++initializer_depth];
    if (initializer_depth >= 32) fatal_error(54);
    if (!initializer_frame->symbol)
        initializer_frame->symbol = allocate_permanent(sizeof(FrontendSymbol));
    bytes = initializer_symbol_copy_sizes[symbol->kind & 0x1f];
    source = (unsigned char *)symbol;
    destination = (unsigned char *)initializer_frame->symbol;
    while (bytes--) *destination++ = *source++;
    if (!initializer_frame->type) initializer_frame->type = allocate_permanent(sizeof(CType));
    *initializer_frame->type = *symbol->type;
    initializer_frame->symbol->type = initializer_frame->type;
    if (symbol->attributes & 2) {
        initializer_frame->symbol->type->related = initializer_frame->symbol->type->related->related;
        initializer_frame->symbol->attributes &= ~2;
    }
    initializer_frame->string_seen = 0;
    initializer_frame->offset = 0;
    initializer_frame->flags = 0;
}

typedef char InitializerFrame_must_be_16_bytes[sizeof(InitializerFrame) == 16 ? 1 : -1];

/* Original 0x30cf0..0x30d38. Pointer shapes and primitive scalar kinds
   accept a single value; array shapes require aggregate traversal. */
int __stdcall is_scalar_initializer_symbol(FrontendSymbol *symbol)
{
    CType *type = symbol->type;
    TypeShape *shape = type->related;
    if (symbol->attributes & 2) shape = shape->related;
    if (shape) {
        if (shape->flags & 1) return 1;
        if ((shape->flags & 7) == 2) return 0;
    }
    return (type->flags & 0xb40) != 0;
}

unsigned initializer_bitfield_offset; /* original 0x60248 */

/* Original 0x301a0..0x305a5. Emit one aggregate member, preserving the
   separate constant/runtime paths and bitfield storage-unit accumulation. */
void __stdcall emit_initializer(IrToken *initializer)
{
    IrToken *value, *left, *right;
    FrontendSymbol *symbol;
    CType *type;
    unsigned short flags;
    unsigned packed, count, character_bytes;
    int number, string_array;
    if (!initializer || initializer_depth < 0 || (initializer_frame->flags & 0x20)) return;
    value = normalize_initializer_expression(initializer);
    if (token_categories[value->kind] == TOK_MEMBER_OPERATOR) {
        left = ((ExpressionNode *)value)->value.children.left;
        if (left && (left->flags & 7) == 3) {
            right = ((ExpressionNode *)value)->value.children.right;
            value = make_integer_constant(right->value.integer + left->value.integer);
        }
    }
    if (!select_initializer_target(value)) {
        if (!initializer_error_reported) {
            initializer_error_reported = 1;
            report_syntax_error(78);
        }
        return;
    }
    if (scanner_flags & 0x200000) check_runtime_initializer(initializer_frame->symbol, value);
    else if (!check_static_initializer(initializer_frame->symbol, &value)) {
        report_syntax_error(99);
        return;
    }
    symbol = initializer_frame->symbol;
    if ((symbol->kind & 0x1f) == 2) {
        if (!initializer_bitfield_active) {
            initializer_bitfield_active = 1;
            initializer_bitfield_value = 0;
            initializer_bitfield_offset = symbol->detail.value;
        }
        flags = value->type->flags;
        if (!(flags & 0x140)) {
            report_syntax_error(77, initializer_frame->symbol->name);
            advance_initializer();
            return;
        }
        if ((value->flags & 7) == 3) {
            if (flags & 0x100) {
                if (extended_to_integer(value->value.pointer, &number)) compiler_warning(2, 56);
                value->value.integer = number;
            }
            symbol = initializer_frame->symbol;
            packed = ((1U << (symbol->metadata.bitfield.width & 31)) - 1) & value->value.integer;
            packed <<= symbol->metadata.bitfield.offset & 31;
            initializer_bitfield_value |= packed;
            advance_initializer();
            return;
        }
        if ((scanner_flags & 0x200000) &&
            (initializer_runtime_override || options.extensions_enabled)) {
            emit_runtime_initializer(initializer);
            advance_initializer();
            return;
        }
        advance_initializer();
        return;
    }
    string_array = is_character_array_initializer(symbol, value);
    if (string_array) value->length = fit_string_initializer(initializer_frame->symbol, value);
    if (scanner_flags & 0x200000)
        emit_runtime_symbol_initializer(initializer_frame->symbol, initializer);
    else write_initializer_value(initializer_frame->symbol, value, 0);
    if (string_array) {
        if (initializer_frame->offset) {
            if (!initializer_frame->string_seen && options.extensions_enabled)
                compiler_warning(4, 207);
            else if (!initializer_error_reported) {
                initializer_error_reported = 1;
                report_syntax_error(78);
            }
        }
        character_bytes = (value->auxiliary_flags & 1) ? 2 : 1;
        initializer_frame->offset += value->length / character_bytes;
        initializer_frame->string_seen = 1;
        count = initializer_frame->symbol->type->related->field08;
        if (count) {
            if (count <= initializer_frame->offset) {
                initializer_frame->flags |= 2;
                if (initializer_frame->offset > count && !initializer_error_reported) {
                    initializer_error_reported = 1;
                    report_syntax_error(78);
                }
            } else {
                constant_one = character_bytes * (count - initializer_frame->offset);
                emit_initializer_constant(0);
                initializer_frame->offset = count;
            }
        }
        if (!(scanner_flags & 0x200000)) {
            initializer_offset += value->length;
            advance_initializer();
            return;
        }
    } else if (!(scanner_flags & 0x200000)) {
        type = remove_type_prefix(initializer_frame->symbol->type);
        if (initializer->type->related && (initializer->type->related->flags & 7) == 2)
            initializer->type = decay_array_expression_type(initializer);
        check_assignment_conversion(TOK_INITIALIZING, type, initializer);
        initializer_offset += type_size(type);
    }
    advance_initializer();
}

/* Original 0x313f0..0x3147c. Strip casts and redundant segment/base
   wrappers while retaining a based reference whose base still has a shape. */
IrToken *__stdcall normalize_initializer_expression(IrToken *value)
{
    IrToken *left, *right, *reference;
    FrontendSymbol *symbol;
    TypeShape *shape;
    for (;;) {
        switch (value->kind) {
        case TOK_CAST:
            value = ((ExpressionNode *)value)->value.children.left;
            continue;
        case TOK_BASED_REFERENCE:
            left = ((ExpressionNode *)value)->value.children.left;
            right = ((ExpressionNode *)value)->value.children.right;
            if (!right) {
                value = left;
                continue;
            }
            if ((left->type->related->attributes & 7) != 5) return value;
            reference = token_categories[right->kind] == TOK_DEREFERENCE ?
                ((ExpressionNode *)right)->value.children.left : right;
            symbol = reference->value.pointer;
            shape = symbol->type->related;
            if (shape && !(shape->flags & 7)) shape = shape->related;
            if (shape) return value;
            value = left;
            continue;
        case TOK_SEGMENT:
            left = ((ExpressionNode *)value)->value.children.left;
            if (left->flags & 0x80) {
                value = left;
                continue;
            }
            break;
        }
        return value;
    }
}

/* Original 0x305b0..0x305f1. Descend aggregate levels until the current
   symbol can receive this scalar or character-array value. */
int __stdcall select_initializer_target(IrToken *value)
{
    while (descend_initializer_target(value)) {
        if (is_scalar_initializer_symbol(initializer_frame->symbol) ||
            is_character_array_initializer(initializer_frame->symbol, value)) return 1;
    }
    return 0;
}

/* Original 0x30600..0x308c3. Descend array elements or aggregate
   members, retaining mutable types for arrays whose bounds are inferred. */
int __stdcall descend_initializer_target(IrToken *value)
{
    FrontendSymbol *symbol, *tag, *member;
    CType *type;
    unsigned kind;
    int incomplete_array;
    char *name;
    if (initializer_depth < 0 || (initializer_frame->flags & 0x20)) return 0;
    symbol = initializer_frame->symbol;
    if (is_scalar_initializer_symbol(symbol)) return !(initializer_frame->flags & 8);
    if (is_character_array_initializer(symbol, value)) return 1;
    if (initializer_frame->flags & 2) return 0;
    if (symbol->attributes & 0x400) {
        incomplete_array = (symbol->attributes & 0x100) &&
            (initializer_frame->type->related->flags & 7) == 2 &&
            !initializer_frame->type->related->field08;
        push_initializer_frame(symbol);
        symbol = initializer_frame->symbol;
        *initializer_frame->type = *drop_first_type_shape(symbol->type);
        symbol->type = initializer_frame->type;
        initializer_inferred_type = incomplete_array ? initializer_frame->type : 0;
        if (symbol->type->related && (symbol->type->related->flags & 7) == 2)
            symbol->attributes |= 0x400;
        else symbol->attributes &= ~0x400;
        return 1;
    }
    type = symbol->type;
    switch (type->flags & 0x2fdf) {
    case 0x402: case 0x408:
        break;
    case 0x412: case 0x414: case 0x418:
        tag = (FrontendSymbol *)type->field08;
        if (!(tag->flags & 1)) {
            name = symbol->name;
            if (initializer_depth >= 0) {
                initializer_frame = &initializer_frames[--initializer_depth];
                if (initializer_depth >= 0)
                    report_syntax_error(79, initializer_frame->symbol->name, name);
            }
            initializer_depth = -1;
            return 0;
        }
        push_initializer_frame(tag);
        kind = symbol->kind & 0x1f;
        if (!(kind >= 1 && kind <= 13 && kind != 5 &&
              symbol->name == name_unnamed && (symbol->attributes & 0x800)))
            symbol = initializer_frame->symbol;
        break;
    default:
        initializer_frame->flags |= 0x20;
        return 0;
    }
    kind = symbol->kind & 0x1f;
    if (kind >= 1 && kind <= 13 && kind != 5 &&
        symbol->name == name_unnamed && (symbol->attributes & 0x800))
        member = symbol->next;
    else member = symbol->associated.members;
    member = next_initializer_member(0, member);
    if (!member) {
        initializer_frame->symbol->associated.members = 0;
        return advance_initializer();
    }
    initializer_frame->symbol->associated.members = member;
    push_initializer_frame(member);
    symbol = initializer_frame->symbol;
    if (is_character_array_initializer(symbol, value) && (symbol->attributes & 0x100))
        initializer_inferred_type = drop_first_type_shape(symbol->type);
    return 1;
}

/* Original 0x308d0..0x3096a. Skip unnamed padding members, flushing
   a pending bitfield unit before the padding record. */
FrontendSymbol *__stdcall next_initializer_member(FrontendSymbol *container, FrontendSymbol *member)
{
    unsigned kind;
    while (member) {
        kind = member->kind & 0x1f;
        if (kind < 1 || kind > 13 || kind == 5 || member->name != name_unnamed) return member;
        if (!(member->attributes & 0x1800)) {
            if (initializer_bitfield_active) {
                constant_one = initializer_bitfield_value;
                emit_initializer_constant(container);
                initializer_bitfield_active = 0;
            }
            emit_member_padding(member);
        } else {
            if (member->attributes & 0x1000) return 0;
            if (member->attributes & 0x800) return member;
        }
        member = member->next;
    }
    return 0;
}

/* Original 0x2f6e0..0x2f760. The Microsoft extension permits an
   automatic object's address in this initializer with a level-four warning. */
void __stdcall check_runtime_initializer(FrontendSymbol *symbol, IrToken *value)
{
    FrontendSymbol *reference = 0;
    int offset = 0;
    if (find_initializer_address(value, &reference, &offset) && reference &&
        (reference->storage[0] & 7) == 4) {
        if (options.extensions_enabled)
            compiler_warning(4, 221, symbol->name, reference->name);
        else report_syntax_error(93, symbol->name, reference->name);
    }
}

/* Original 0x2fab0..0x2fbb7. Scalar automatic initialization emits an
   assignment; strings choose an inline or call-form memory copy. */
void __stdcall emit_runtime_symbol_initializer(FrontendSymbol *symbol, IrToken *value)
{
    IrToken *target, *bytes;
    ArgumentNode *arguments;
    unsigned size;
    if (!check_static_initializer(symbol, &value)) report_declaration_context_error(204, 97);
    target = make_initializer_lvalue(symbol->type);
    if (is_character_array_initializer(symbol, value)) {
        value->length = fit_string_initializer(symbol, value);
        if (!options.option_qc && !(options.option_M & 4)) {
            bytes = make_integer_constant(value->length);
            arguments = append_argument(0, target, 1);
            arguments = append_argument(arguments, value, 1);
            arguments = append_argument(arguments, bytes, 1);
            emit_memory_initializer_call(172, arguments);
        } else {
            bytes = make_integer_constant(value->length);
            emit_inline_memory_initializer(172, target, value, bytes);
        }
        initializer_offset += value->length;
    } else {
        target->type = symbol->type;
        size = type_size(symbol->type);
        emit_initializer_assignment(target, value);
        initializer_offset += size;
    }
}

/* Original 0x2f540..0x2f6dc. Accept constant data and eligible symbol
   relocations, including the restricted integer form of a based pointer. */
int __stdcall check_static_initializer(FrontendSymbol *symbol, IrToken **value)
{
    FrontendSymbol *reference = 0, *base;
    TypeShape *shape, *base_shape;
    unsigned kind;
    int offset = 0;
    if (!*value) return 0;
    if ((*value)->type != symbol->type) {
        shape = symbol->type->related;
        if (shape && !(shape->flags & 7)) shape = shape->related;
        if (shape && (shape->flags & 1) && (shape->attributes & 7) == 5) {
            base = (FrontendSymbol *)shape->field08;
            base_shape = base->type->related;
            if (base_shape && !(base_shape->flags & 7)) base_shape = base_shape->related;
            if (base_shape && !((*value)->flags & 0x80) &&
                !(((*value)->flags & 7) == 3 && ((*value)->type->flags & 0x2fdf) == 0x64))
                return 0;
        }
    }
    kind = (*value)->flags & 7;
    if (kind == 5 || kind == 3) return 1;
    if (!find_initializer_address(*value, &reference, &offset)) return 0;
    if (!reference) return 1;
    kind = reference->kind & 0x1f;
    if (kind == 13 && reference->index == 6) return 0;
    shape = reference->type->related;
    if (shape && !(shape->flags & 7) &&
        (((FrontendSymbol *)shape->field08)->kind & 0x1f) != 13) return 0;
    if ((reference->storage[0] & 7) == 4 &&
        ((symbol->kind & 0x1f) == 3 || (symbol->kind & 0x1f) == 2 ||
         (symbol->storage[0] & 7) == 3)) return 0;
    if ((reference->storage[2] & 2) && kind != 7) return 0;
    if ((reference->storage[2] & 4) && !(reference->storage[2] & 8)) return 0;
    return 1;
}

/* Original 0x2f760..0x2f7a7. Aggregate members are addressed relative
   to the declaration currently being initialized and its byte offset. */
IrToken *__stdcall make_initializer_lvalue(CType *type)
{
    IrToken *offset, *object;
    if (is_scalar_initializer_symbol(initializer_symbol)) return make_symbol_reference(initializer_symbol);
    offset = make_integer_constant(initializer_offset);
    object = make_symbol_reference(initializer_symbol);
    return make_typed_ir_expression(TOK_MEMBER, object, offset, type);
}

/* Original 0x2fce0..0x2fcf2. Retain the separate assignment entry
   and its common expression-emission helper. */
int __stdcall emit_initializer_assignment(IrToken *target, IrToken *value)
{
    return emit_checked_initializer_assignment(target, value);
}

/* Original 0x2fd00..0x2fd25. Build the initializing assignment through
   the ordinary expression checks and emit it as a statement. */
int __stdcall emit_checked_initializer_assignment(IrToken *target, IrToken *value)
{
    target = make_indirection(target);
    emit_expression_statement(make_binary_expression(TOK_INITIALIZING, target, value));
    return 1;
}

typedef char InitializerFrameStorage_must_be_528_bytes[sizeof(InitializerFrameStorage) == 528 ? 1 : -1];

/* Original 0x30970..0x30c60, including the shared member-update tail.
   Pop implicit levels and advance the enclosing array or member chain. */
int advance_initializer(void)
{
    FrontendSymbol *symbol, *member, *next, *tag;
    CType *type;
    unsigned kind, bytes;
    int nesting;
    while (initializer_depth >= 0) {
        if (initializer_frame->flags & 0x20) {
            if (initializer_depth >= 0) initializer_frame = &initializer_frames[--initializer_depth];
            return 1;
        }
        if (initializer_frame->flags & 1) {
            initializer_frame->flags |= 8;
            return 0;
        }
        if (initializer_depth < 0) return 0;
        initializer_frame = &initializer_frames[--initializer_depth];
        if (initializer_depth < 0) return 0;
        symbol = initializer_frame->symbol;
        if (symbol->attributes & 0x400) {
            if (initializer_frame->string_seen && !initializer_error_reported) {
                initializer_error_reported = 1;
                report_syntax_error(78);
            }
            ++initializer_frame->offset;
            if (initializer_frame->symbol->type->related->field08 == initializer_frame->offset) {
                initializer_frame->flags |= 2;
                if (!(initializer_frame->flags & 1)) continue;
            }
            return 1;
        }
        switch (symbol->type->flags & 0x2fdf) {
        case 0x402: case 0x408:
            member = symbol->associated.members;
            kind = member->kind & 0x1f;
            if (kind >= 1 && kind <= 13 && kind != 5 && (member->attributes & 0x800)) {
                nesting = 1;
                do {
                    member = member->next;
                    kind = member->kind & 0x1f;
                    if (kind >= 1 && kind <= 13 && kind != 5 && member->name == name_unnamed) {
                        if (member->attributes & 0x800) ++nesting;
                        else if (member->attributes & 0x1000) --nesting;
                    }
                } while (nesting);
            }
            next = next_initializer_member(member, member->next);
            if (next) {
                if (initializer_bitfield_active && next->detail.value != initializer_bitfield_offset) {
                    constant_one = initializer_bitfield_value;
                    emit_initializer_constant(member);
                    initializer_bitfield_active = 0;
                }
                initializer_frame->symbol->associated.members = next;
                return 1;
            }
            if (initializer_bitfield_active) {
                constant_one = initializer_bitfield_value;
                emit_initializer_constant(member);
                initializer_bitfield_active = 0;
            }
            continue;
        case 0x404:
            if (initializer_depth >= 0) initializer_frame = &initializer_frames[--initializer_depth];
            if (initializer_bitfield_active) {
                constant_one = initializer_bitfield_value;
                emit_initializer_constant(initializer_frame->symbol->associated.members);
                initializer_bitfield_active = 0;
            }
            type = initializer_frame->symbol->type;
            bytes = type_size(type);
            tag = (FrontendSymbol *)type->field08;
            bytes -= type_size(tag->associated.members->type);
            constant_one = bytes;
            if (bytes) emit_initializer_constant(0);
            if (!(initializer_frame->flags & 1)) continue;
            initializer_frame->flags |= 2;
            return 1;
        case 0x412: case 0x418:
            if (!(initializer_frame->flags & 1)) continue;
            initializer_frame->flags |= 2;
            return 1;
        default: return 1;
        }
    }
    return 0;
}

/* Original 0x31200..0x31225. Restore declaration state after completion. */
void __stdcall finish_aggregate_initializer(FrontendSymbol *symbol)
{
    finalize_aggregate_initializer(symbol);
    if (parser_flags & 0x20000000UL) {
        parser_flags &= ~0x20000000UL;
        symbol->attributes &= ~4UL;
    }
}

/* Original 0x31230..0x3133f. Complete a braced initializer and fill in
   array bounds or aggregate size when the declaration left them open. */
void __stdcall finalize_aggregate_initializer(FrontendSymbol *symbol)
{
    int unsized_array, incomplete_tag;
    unsigned count, bytes;
    CType *type;
    TypeShape *shape;
    if (options.prototypes_only || !symbol) return;
    if (initializer_frame->flags & 0x20) {
        if (initializer_depth >= 0)
            initializer_frame = &initializer_frames[--initializer_depth];
        return;
    }
    finish_nested_initializer();
    unsized_array = (symbol->attributes & 0x400) && (symbol->attributes & 0x100);
    type = symbol->type;
    incomplete_tag = (type->flags & 0x400) &&
        (((FrontendSymbol *)type->field08)->attributes & 0x100);
    if (unsized_array) {
        symbol->attributes &= ~0x100UL;
        count = initializer_array_count();
        type = symbol->type;
        shape = type->related;
        if (symbol->attributes & 2) shape = shape->related;
        symbol->type = replace_outer_type_shape(type, shape->flags, shape->attributes, &count);
    } else if (incomplete_tag) {
        if (initializer_inferred_type) {
            bytes = type_size(initializer_inferred_type);
            bytes *= initializer_array_count();
            symbol->detail.value = bytes;
        } else symbol->detail.value = 0;
        initializer_inferred_type = 0;
    }
    finish_initializer_symbol(symbol);
}

/* Original 0x30ec0..0x30f47. Close the current braced level and emit
   its remaining zero bytes after restoring the enclosing position. */
void finish_nested_initializer(void)
{
    unsigned bytes;
    if (initializer_depth < 0) return;
    if (initializer_frame->flags & 0x20) {
        if (initializer_depth >= 0)
            initializer_frame = &initializer_frames[--initializer_depth];
        return;
    }
    bytes = finish_implicit_initializers();
    if (!(initializer_frame->flags & 6)) bytes += remaining_initializer_bytes();
    initializer_frame->flags = 0;
    advance_initializer();
    if (bytes) {
        constant_one = bytes;
        emit_initializer_constant(0);
    }
}

/* Original 0x30f50..0x30f8c. Collect zero fill from implicit aggregate
   levels until the enclosing explicit brace is reached. */
unsigned finish_implicit_initializers(void)
{
    unsigned bytes = 0;
    if (initializer_depth < 0) return 0;
    while (!(initializer_frame->flags & 1)) {
        bytes += remaining_initializer_bytes();
        advance_initializer();
    }
    return bytes;
}

/* Original 0x31030..0x3117c. Count uninitialized storage, consolidating
   bitfields sharing a unit and treating anonymous aggregates as one member. */
unsigned remaining_initializer_bytes(void)
{
    FrontendSymbol *symbol, *member, *next;
    CType *type;
    TypeShape *shape;
    unsigned count, bytes, size, kind;
    symbol = initializer_frame->symbol;
    if (initializer_depth < 0) return 0;
    type = symbol->type;
    shape = type->related;
    if (shape && (shape->flags & 7) == 2) {
        count = shape->field08;
        if (!count) return 0;
        size = array_element_size(type->flags, (void *)type->field08, shape);
        return size * (count - initializer_frame->offset);
    }
    if (!(type->flags & 0x400)) return type_size(type);
    if (initializer_bitfield_active) {
        constant_one = initializer_bitfield_value;
        emit_initializer_constant(symbol->associated.members);
    }
    bytes = 0;
    member = symbol->associated.members;
    while (member) {
        kind = member->kind & 0x1f;
        if (kind >= 1 && kind <= 13 && kind != 5) {
            if (member->attributes & 0x800) {
                bytes += type_size(member->type);
                member = skip_initializer_anonymous_members(member);
                break;
            }
            if (member->attributes & 0x1000) break;
        }
        if (kind == 2) {
            while ((next = next_shared_bitfield(member->detail.value, member->next)) != 0)
                member = next;
            if (initializer_bitfield_active) {
                initializer_bitfield_active = 0;
                size = 0;
            } else size = member->type->flags & 15;
        } else if (member->name != name_unnamed) size = type_size(member->type);
        else size = member->detail.value;
        bytes += size;
        member = member->next;
    }
    symbol->associated.members = 0;
    initializer_bitfield_active = 0;
    return bytes;
}

FrontendSymbol *initializer_temporary; /* original 0x60460 */
int initializer_temporary_assigned; /* original 0x60238 */

/* Original 0x31480..0x31512. Emit the shared integer workspace as a
   static value, runtime zero fill, or runtime member assignment. */
void __stdcall emit_initializer_constant(FrontendSymbol *symbol)
{
    IrToken *value, *target;
    FrontendSymbol *temporary;
    if (!(scanner_flags & 0x200000)) {
        value = make_constant_token(TOK_INT_CONSTANT, &constant_one);
        write_initializer_value(symbol, value, 0);
        return;
    }
    if (!symbol) {
        emit_runtime_zero_initializer();
        return;
    }
    temporary = initializer_temporary;
    value = make_constant_token(TOK_INT_CONSTANT, &constant_one);
    if (temporary) {
        target = make_symbol_reference(temporary);
        target = make_indirection(target);
        value = make_binary_expression(TOK_BIT_OR, target, value);
        emit_runtime_symbol_initializer(symbol, value);
        initializer_temporary_assigned = 1;
    } else emit_runtime_symbol_initializer(symbol, value);
}

/* Original 0x2f7b0..0x2f845. Zero uninitialized automatic storage through
   a call or an inline memory-operation record, according to optimization. */
void emit_runtime_zero_initializer(void)
{
    IrToken *bytes, *zero, *target;
    ArgumentNode *arguments;
    if (!options.option_qc && !(options.option_M & 4)) {
        bytes = make_constant_token(TOK_INT_CONSTANT, &constant_one);
        zero = make_integer_constant(0);
        target = make_initializer_lvalue(type_0064);
        arguments = append_argument(0, target, 1);
        arguments = append_argument(arguments, zero, 1);
        arguments = append_argument(arguments, bytes, 1);
        emit_memory_initializer_call(173, arguments);
    } else {
        bytes = make_constant_token(TOK_INT_CONSTANT, &constant_one);
        zero = make_integer_constant(0);
        target = make_initializer_lvalue(type_0064);
        emit_inline_memory_initializer(173, target, zero, bytes);
    }
    initializer_offset += constant_one;
}

/* Original 0x2f8c0..0x2f8e0. Intrinsic memory calls retain the numeric
   operation as the callee operand of an ordinary expression record. */
void __stdcall emit_memory_initializer_call(unsigned operation, ArgumentNode *arguments)
{
    IrToken *callee;
    callee = make_integer_constant(operation);
    emit_expression_statement(make_ir_expression(TOK_INTRINSIC, callee, (IrToken *)arguments));
}

IrToken *initializer_exit_label; /* original 0x60234 */

/* Original 0x311a0..0x311f4. The argument is retained by the original
   stdcall entry but completion uses the common initializer state. */
void __stdcall finish_initializer_symbol(FrontendSymbol *symbol)
{
    if (initializer_exit_label) {
        emit_code_label(initializer_exit_label);
        initializer_exit_label = 0;
    }
    pending_initializer_symbol = 0;
    if (!(scanner_flags & 0x200000) || (scanner_flags & 0x100000)) {
        if (browser_filename && (scanner_flags & 0x100000)) browser_finish_scope();
        flush_initializer_buffer();
    }
}

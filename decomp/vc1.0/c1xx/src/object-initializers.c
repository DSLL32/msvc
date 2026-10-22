/* Declaration-level object initialization and its frontend records. */
#include "lexer.h"
#include "parser-state.h"
#include "object-initializers.h"
#include "binary-expressions.h"
#include "class-members.h"
#include "source-input.h"
#include <recon.h>

int initializer_runtime_override; /* original 0x9157c */
CppSymbol *initializer_storage_symbol; /* original 0x91590 */
unsigned initializer_offset; /* original 0x91598 */
unsigned initializer_runtime_start; /* original 0x91594: offset before runtime fallback */
int initializer_value_error_reported; /* original 0xa0a84 */
unsigned initializer_constructor_offset; /* original 0xa0a88 */
unsigned initializer_constructor_count; /* original 0xa0a8c */
LocalStaticGuardState *active_local_static_guard = &local_static_guard; /* original 0x9fc88 */
IrToken *local_static_guard_label; /* original 0x91580 */

/* Original [0x482f0,0x4833a). A direct class object needs construction unless
   its constructor, copy constructor and destructor are all trivial. */
CppSymbol *__stdcall symbol_construction_class(CppSymbol *symbol)
{
    CppSymbol *tag;
    unsigned char trivial;
    if ((symbol->flags & 0x10) || (symbol->storage[0] & 7) == 7) return 0;
    tag = class_type_of_member(symbol);
    if (!tag) return 0;
    trivial = tag->detail.class_info->trivial_special_members;
    if ((trivial & 13) == 13) return 0;
    return tag;
}

/* Original [0x47ef0,0x47f80). The temporary unqualified type is restored
   after the constructor expression is emitted, before local-static cleanup. */
void __stdcall initialize_class_object(CppSymbol *symbol, IrToken *value)
{
    CppSymbol *tag = symbol_construction_class(symbol);
    CType *saved_type = symbol->type;
    IrToken *arguments, *object, *call;
    symbol->attributes &= ~4UL;
    begin_runtime_initializer(symbol);
    arguments = prepare_constructor_arguments(tag, value);
    symbol->type = remove_type_qualifiers(symbol->type, 0x3000);
    object = builtin_unary_expression(0x3a, resolve_symbol_expression(symbol));
    call = make_object_constructor_call(0, 0, 0, symbol, arguments, object, 0);
    emit_expression_statement(prepare_expression(call, 2));
    symbol->type = saved_type;
    if ((token_allocation_flags & 0x80000) && !(token_allocation_flags & 0x40) &&
        (symbol->storage[0] & 7) == 3)
        finish_local_static_initialization(symbol);
}

/* Original [0x47f80,0x47fca). Global initializers get a generated function;
   block statics use its separately represented guard setup instead. */
void __stdcall begin_runtime_initializer(CppSymbol *symbol)
{
    if (!(token_allocation_flags & 0x40) && !(token_allocation_flags & 0x80000))
        begin_initializer_function(0);
    else
        guard_local_static_initialization(symbol);
    token_allocation_flags |= 0x200000;
    if (token_allocation_flags & 0x40) record_line_information(1);
    symbol->declaration_flags |= 4;
}

/* Original [0x47fd0,0x480e2). The low byte of the shared state word packs
   eight object guards; its upper bytes retain the reset-only interpretation.
   A new guard symbol is allocated when all eight bits have been consumed. */
void __stdcall guard_local_static_initialization(CppSymbol *symbol)
{
    CppSymbol *guard;
    IrToken *value, *object, *label;
    unsigned char *mask;
    if ((token_allocation_flags & 0x80000) && (symbol->storage[0] & 7) == 3 &&
        !(token_allocation_flags & 0x40)) {
        active_local_static_guard = &local_static_guard;
        if (!local_static_guard.guard) {
            ++local_static_guard.count;
            guard = create_generated_object(3, type_4041_q1);
            active_local_static_guard->guard = guard;
            *(unsigned char *)&active_local_static_guard->state = 1;
        }
        local_static_guard_label = new_branch_label(0x1e);
        integer_constant_scratch = *(unsigned char *)&active_local_static_guard->state;
        label = local_static_guard_label;
        value = new_literal_constant(7, &integer_constant_scratch);
        object = symbol_reference(active_local_static_guard->guard);
        object = indirect_storage_expression(object);
        value = builtin_binary_expression(0x3a, object, value);
        emit_condition_to_label(value, label, 1);
        integer_constant_scratch = *(unsigned char *)&active_local_static_guard->state;
        value = new_literal_constant(7, &integer_constant_scratch);
        object = symbol_reference(active_local_static_guard->guard);
        object = indirect_storage_expression(object);
        value = builtin_binary_expression(0x57, object, value);
        emit_expression_statement(value);
        mask = (unsigned char *)&active_local_static_guard->state;
        *mask = (unsigned char)(*mask << 1);
        if (!*mask) active_local_static_guard->guard = 0;
    }
}

/* Original [0x3b060,0x3b0c1). Reuse arguments already packaged by a
   constructor expression for this class; otherwise form a one-item list. */
IrToken *__stdcall prepare_constructor_arguments(CppSymbol *tag, IrToken *value)
{
    IrToken *callee, *reference;
    CppSymbol *constructor;
    if (!value) return 0;
    if ((tag->declaration_flags & 0x1000) && value->kind != 0x6a) {
        if (value->kind == 0x7a) {
            callee = value->value.pointer;
            if (callee->kind == 0xa4) {
                reference = callee->value.pointer;
                constructor = reference->value.pointer;
                if ((constructor->declaration_flags & 0x1000) &&
                    tag == cpp_context_stack->items[constructor->auxiliary.declaring_class])
                    return value->tail.right;
            }
        }
        value = append_expression(0, value, 2);
    }
    return value;
}

/* Original 0x46160..0x461d9. The native EAX remains the incoming symbol
   through the final RET 4; its caller consumes that return value. */
CppSymbol *__stdcall prepare_initializer_symbol(CppSymbol *symbol)
{
    unsigned storage;
    initializer_runtime_override = 0;
    token_allocation_flags &= ~0x00100000UL;
    if (declaration_mode) {
        storage = symbol->storage[0] & 7;
        if (storage >= 4 && storage <= 6) token_allocation_flags |= 0x00200000UL;
        else token_allocation_flags &= ~0x00200000UL;
    } else token_allocation_flags &= ~0x00200000UL;
    initializer_storage_symbol = symbol;
    initializer_offset = 0;
    initializer_value_error_reported = 0;
    initializer_constructor_offset = 0;
    initializer_constructor_count = 0;
    return symbol;
}

/* Original 0x266a0..0x2670c. Recovery preserves a type/brace lookahead
   in token replay before inserting a semicolon. */
int __stdcall reject_function_initializer(CppSymbol *declaration)
{
    if (parser_lookahead == -1) parser_lookahead = read_parser_token();
    switch (parser_lookahead) {
    case 0x28:
    case 0x7f:
        unget_token((unsigned char)parser_lookahead);
        parser_lookahead = 0x8a;
        cpp_parser_flags |= 0x100;
        break;
    case 0x3f:
        break;
    default:
        return 0;
    }
    report_syntax_error(448, declaration->name);
    return 1;
}

/* Original 0x461e0..0x4628b. An initialized extern becomes a definition;
   prototype-only mode skips storage setup after the declaration checks. */
CppSymbol *__stdcall begin_object_initializer(CppSymbol *declaration)
{
    unsigned storage;
    if (!declaration) return 0;
    storage = declaration->storage[0] & 7;
    if (storage == 7) {
        report_syntax_error(71, declaration->name);
        return 0;
    }
    if (storage == 1) {
        if (declaration_mode) report_syntax_error(205, declaration->name);
        declaration->storage[0] = (declaration->storage[0] & 0xfa) | 2;
    }
    if (options.prototypes_only) return declaration;
    if ((declaration->storage[2] & 4) && !(declaration->storage[2] & 8))
        cpp_parser_flags |= 0x20000000UL;
    return prepare_initializer_symbol(declaration);
}

/* Original 0x46290..0x462ba. Complete DLL-import recovery after the
   separately represented initializer dispatcher has processed the value. */
void __stdcall initialize_object(CppSymbol *symbol, IrToken *value)
{
    initialize_object_value(symbol, value);
    if (cpp_parser_flags & 0x20000000UL) {
        cpp_parser_flags &= ~0x20000000UL;
        symbol->attributes &= ~4UL;
    }
}

/* Original [0x462c0,0x46589), including every early RET 8 and branch.
   The runtime-initialization bit belongs to token_allocation_flags at 0x9f790;
   cpp_parser_flags is the distinct word at 0x9f794. String length and width
   occupy the two shorts of the token's +0x10 union. */
void __stdcall initialize_object_value(CppSymbol *symbol, IrToken *value)
{
    int string_initializer;
    TypeShape *shape;
    unsigned short flags;
    IrToken *normalized;
    CType *target_type, *source_type;
    unsigned conversion;

    if (!symbol || (symbol->declaration_flags & 2)) return;
    if (symbol_construction_class(symbol)) {
        initialize_class_object(symbol, value);
        return;
    }
    if (!value) return;
    value = fold_symbol_constant(value);
    if (value->kind == 0x6a) {
        if (value->tail.right) report_syntax_error(78);
        value = value->value.pointer;
    }
    value = prepare_constant_initializer(symbol, value);
    if (!value) return;
    string_initializer = is_character_array_initializer(symbol, value);
    if (string_initializer)
        value->tail.string.length = fit_string_initializer(symbol, value);
    if (!(token_allocation_flags & 0x200000)) {
        if (!check_static_initializer(symbol, &value)) {
            if (symbol->storage[2] & 2) {
                report_syntax_error(482, format_symbol_name(symbol));
                return;
            }
            begin_runtime_initializer(symbol);
            initializer_runtime_override = 1;
            initializer_runtime_start = initializer_offset;
        }
    }
    if ((token_allocation_flags & 0x200000) && is_scalar_initializer_symbol(symbol)) {
        emit_initializer_assignment(symbol_reference(symbol), value);
        finish_initializer_symbol(symbol);
        return;
    }
    shape = symbol->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (shape) {
        if ((symbol->attributes & 0x400) && !string_initializer) {
            if (value->kind == 3 && !shape->related) {
                flags = symbol->type->flags;
                if ((value->tail.string.auxiliary_flags & 1) ?
                    (flags & 0x2fdf) == 0x41 : (flags & 0xefff) == 0x4042) {
                    report_syntax_error(53, symbol->name);
                    return;
                }
            }
            report_syntax_error(75, symbol->name);
            return;
        }
    } else if (symbol->type->flags & 0x400) {
        if (token_allocation_flags & 0x200000) {
            symbol->attributes &= ~4UL;
            emit_initializer_assignment(symbol_reference(symbol), value);
            finish_initializer_symbol(symbol);
            return;
        }
        report_syntax_error(74, type_kind_name(symbol->type->flags), symbol->name);
        return;
    }
    normalized = normalize_initializer_expression(value);
    if (token_allocation_flags & 0x200000)
        emit_runtime_symbol_initializer(symbol, value);
    else
        write_literal_initializer(symbol, normalized, 0);
    if (!(token_allocation_flags & 0x200000) &&
        !is_character_array_initializer(symbol, normalized)) {
        target_type = strip_based_shape(symbol->type);
        source_type = value->type;
        conversion = classify_initializer_conversion(target_type, value);
        if (conversion >= 0x400 && !report_operator_resolution_failure(
                0x7e, 2, conversion, target_type, source_type)) {
            if (!target_type || !source_type)
                report_type_error(592, target_type);
            else
                report_type_conversion(440, 0, 0x7e, source_type, target_type);
        }
    } else {
        integer_constant_scratch = remaining_string_initializer_bytes(symbol, normalized);
        if ((int)integer_constant_scratch > 0) emit_initializer_constant(0);
    }
    finish_initializer_symbol(symbol);
}

/* Declaration-level object initialization and its frontend records. */
#include "lexer.h"
#include "parser-state.h"
#include "object-initializers.h"
#include "binary-expressions.h"
#include "class-members.h"
#include "source-input.h"
#include "initialization.h"
#include "class-finalization.h"
#include "initializer-records.h"
#include "function-finalization.h"
#include "braced-initializers.h"
#include <recon.h>

char *__stdcall make_local_static_destructor_name(CppSymbol *object, CppSymbol *function);
void __stdcall emit_atexit_registration(CppSymbol *function);
void __stdcall defer_object_definition(CppSymbol *symbol);

IrToken *initializer_end_label; /* original 0x91588: optional final branch label */
CppSymbol *pending_initializer_symbol; /* original 0x9fcc4: deferred record header */
int initializer_runtime_override; /* original 0x9157c */
CppSymbol *initializer_storage_symbol; /* original 0x91590 */
unsigned initializer_offset; /* original 0x91598 */
unsigned initializer_runtime_start; /* original 0x91594: offset before runtime fallback */
int initializer_value_error_reported; /* original 0xa0a84 */
unsigned initializer_constructor_offset; /* original 0xa0a88 */
unsigned initializer_constructor_count; /* original 0xa0a8c */

/* Original 0x45830..0x45a8d, stdcall 8. Static data permits constants
   and representable symbol addresses, but rejects runtime-only storage and
   incompatible reference/member/based-pointer initialization. */
int __stdcall check_static_initializer(CppSymbol *symbol, IrToken **value)
{
    CppSymbol *address = 0, *tag, *base;
    TypeShape *shape, *base_shape;
    unsigned kind = symbol->kind & 0x1f, address_kind;
    int offset = 0, direct_constant = 0;
    if (((*value)->flags & 7) == 5) {
        if (kind < 1 || kind > 13 || kind == 5 || !symbol->type)
            direct_constant = 1;
        else {
            tag = cpp_context_stack->items[symbol->type->field08];
            if (!tag || !(tag->declaration_flags & 0x1000)) direct_constant = 1;
        }
    }
    if (!is_character_array_initializer(symbol, *value) &&
        can_initialize(strip_based_shape(symbol->type), *value))
        *value = convert_initializer(0x7e, symbol->type, *value);
    if (!*value) return 0;
    if ((*value)->type != symbol->type) {
        shape = symbol->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        if (shape) {
            if (shape->flags & 0x30) return 0;
            if (shape->flags & 0x40) return 0;
            if ((shape->flags & 1) && (shape->attributes & 7) == 5) {
                base = shape->data.based_symbol;
                base_shape = base->type->related;
                if (base_shape && !(base_shape->flags & 0x77))
                    base_shape = base_shape->related;
                if (base_shape && !((*value)->flags & 0x80) &&
                    (((*value)->flags & 7) != 3 ||
                     ((*value)->type->flags & 0x2fdf) != 0x64)) return 0;
            }
        }
    }
    if (direct_constant || ((*value)->flags & 7) == 3) return 1;
    if (!find_initializer_address(*value, &address, &offset)) return 0;
    if (address) {
        address_kind = address->kind & 0x1f;
        if (address_kind == 13 && address->index == 6) return 0;
        shape = address->type->related;
        if (shape && !(shape->flags & 0x77) &&
            (((CppSymbol *)shape->data.based_symbol)->kind & 0x1f) != 13)
            return 0;
        if ((address->storage[0] & 7) == 4 &&
            (kind == 3 || kind == 2 || (symbol->storage[0] & 7) == 3))
            return 0;
        if ((address->storage[2] & 2) && address_kind != 7) return 0;
        if ((address->storage[2] & 4) && !(address->storage[2] & 8)) return 0;
    }
    return 1;
}
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

/* Original 0x48270..0x482e9. Cache scalar const integer/enum values
   in the symbol before resolving the assignment. The unresolved member token
   receives its symbol-name diagnostic instead of an assignment conversion. */
IrToken *__stdcall prepare_constant_initializer(CppSymbol *symbol, IrToken *value)
{
    CType *type;
    TypeShape *shape;
    if (value) {
        if ((value->flags & 7) == 3) {
            type = symbol->type;
            if ((type->flags & 0x240) && (type->attributes & 0x1000)) {
                shape = type->related;
                if (shape && !(shape->flags & 0x77)) shape = shape->related;
                if (!shape) {
                    symbol->auxiliary.value = value->value.integer;
                    symbol->attributes |= 0x4000;
                }
            }
        }
        if (!(value->flags & 7) && value->kind == 0xa3) {
            report_syntax_error(275, format_symbol_name(value->value.pointer));
            return 0;
        }
    }
    return resolve_assignment_operand(symbol->type, value);
}

/* Original 0x47310..0x4739d. Match narrow/wide strings to an array's
   scalar element, skipping the symbol's optional outer qualifier shape. The
   array attribute establishes the shape prefix; retain its unchecked access. */
int __stdcall is_character_array_initializer(CppSymbol *symbol, IrToken *value)
{
    CType *type;
    TypeShape *shape;
    unsigned short character;
    if (!value || value->kind != 3) return 0;
    if (!(symbol->attributes & 0x400)) return 0;
    character = (value->tail.string.auxiliary_flags & 1) ? 0x42 : 0x41;
    type = symbol->type;
    if ((type->flags & 0x2fdf) != character) return 0;
    shape = type->related;
    if (symbol->attributes & 2) shape = shape->related;
    return shape->related ? 0 : 1;
}

/* Original 0x473a0..0x47407. Indirections and member pointers are
   scalar; a remaining array shape is not. Other shapes defer to base flags. */
int __stdcall is_scalar_initializer_symbol(CppSymbol *symbol)
{
    CType *type = symbol->type;
    TypeShape *shape = type->related;
    if (symbol->attributes & 2) shape = shape->related;
    if (shape) {
        if (shape->flags & 1) return 1;
        if (shape->flags & 0x40) return 1;
        if (shape->flags & 0x30) return 1;
        if ((shape->flags & 0x77) == 2) return 0;
    }
    return (type->flags & 0xb40) ? 1 : 0;
}

/* Original 0x46020..0x46031. Forward the target/value pair and retain
   the initializer helper's complete EAX result across this separate wrapper. */
int __stdcall emit_initializer_assignment(IrToken *target, IrToken *value)
{
    return initialize_lvalue(target, value);
}

/* Original 0x47ad0..0x47ba8. Complete a declaration's initializer, including
   deferred zero fill, local-static guard cleanup and binary record flushing. */
void __stdcall finish_initializer_symbol(CppSymbol *symbol)
{
    if (initializer_runtime_override) {
        integer_constant_scratch = initializer_offset - initializer_runtime_start;
        if (integer_constant_scratch) {
            token_allocation_flags &= ~0x200000UL;
            emit_initializer_constant(0);
            token_allocation_flags |= 0x200000UL;
        } else if (!initializer_offset) {
            symbol->attributes &= ~4UL;
        }
    }
    if ((token_allocation_flags & 0x80000) &&
        !(token_allocation_flags & 0x40) &&
        (symbol->storage[0] & 7) == 3 &&
        (token_allocation_flags & 0x200000))
        finish_local_static_initialization(symbol);
    if (initializer_end_label) {
        emit_label(initializer_end_label);
        initializer_end_label = 0;
    }
    pending_initializer_symbol = 0;
    if (!(token_allocation_flags & 0x200000) ||
        (token_allocation_flags & 0x100000)) {
        if (browser_filename && (token_allocation_flags & 0x100000))
            browser_end_tag();
        flush_initializer_records();
    }
}

/* Original 0x47dc0..0x47e4b, stdcall4. Discard conversion wrappers
   and based-pointer adjustments which do not contribute an address. */
IrToken *__stdcall normalize_initializer_expression(IrToken *value)
{
    IrToken *left, *base;
    CppSymbol *symbol;
    TypeShape *shape;
    for (;;) {
        switch (value->kind) {
        case 0x6e:
            value = value->value.pointer;
            continue;
        case 0x67:
            base = value->tail.right;
            left = value->value.pointer;
            if (!base) {
                value = left;
                continue;
            }
            if ((left->type->related->attributes & 7) != 5) return value;
            if (token_categories[base->kind] == 0x46)
                base = base->value.pointer;
            symbol = base->value.pointer;
            shape = symbol->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            if (shape) return value;
            value = left;
            continue;
        case 0x1a:
            left = value->value.pointer;
            if (left->flags & 0x80) {
                value = left;
                continue;
            }
            break;
        }
        return value;
    }
}

int initializer_data_used; /* original 0x9158c */

/* Original 0x47e50..0x47ee2. The shared integer payload is either a
   static initializer field, an explicit runtime field value, or a byte count
   for zero filling. A materialized data symbol supplies the runtime source. */
void __stdcall emit_initializer_constant(CppSymbol *target)
{
    IrToken *value;
    if (!(token_allocation_flags & 0x200000)) {
        write_initializer_value(target, new_literal_constant(7, &integer_constant_scratch), 0);
        return;
    }
    if (!target) {
        emit_runtime_zero_initializer();
        return;
    }
    value = new_literal_constant(7, &integer_constant_scratch);
    if (initializer_data_symbol) {
        value = builtin_binary_expression(0x56,
            indirect_storage_expression(symbol_reference(initializer_data_symbol)), value);
        emit_runtime_symbol_initializer(target, value);
        initializer_data_used = 1;
    } else emit_runtime_symbol_initializer(target, value);
}

/* Original 0x48340..0x48417, stdcall4. Static objects register an
   available destructor and queue its definition, then close the guard even
   when no destructor exists or a diagnostic was issued. */
void __stdcall finish_local_static_initialization(CppSymbol *symbol)
{
    CppSymbol *function, *object;
    unsigned char saved_mode;
    if (symbol_construction_class(symbol)) {
        if (!has_destructor_member_model(symbol->type))
            report_syntax_error(262, symbol->name);
        else {
            function = make_missing_function_declaration();
            if (function_symbol && (function_symbol->storage[1] & 1) &&
                ((function_symbol->storage[0] & 7) != 3 ||
                 (function_symbol->declaration_flags & 0x400))) {
                object = symbol_alias(symbol);
                function->name = make_local_static_destructor_name(object, function);
                function->declaration_flags =
                    (function->declaration_flags & ~0x100) | 0x200;
                function->storage[0] = (function->storage[0] & ~7) | 0x41;
                insert_scope_symbol(global_scope, function);
            }
            emit_atexit_registration(function);
            saved_mode = declaration_mode;
            declaration_mode = 0;
            object = symbol_alias(symbol);
            defer_object_definition(object);
            defer_object_definition(function);
            declaration_mode = saved_mode;
        }
    }
    emit_label(local_static_guard_label);
}

void __stdcall emit_runtime_initializer_call(unsigned operation, IrToken *destination,
    IrToken *source, IrToken *bytes);
void __stdcall emit_runtime_initializer_list(unsigned operation, IrToken *arguments);

/* Original 0x45df0..0x45ef6, stdcall8. Retain scalar assignment
   and string-copy forms separately, including the frontend-specific intrinsic
   route and the exact byte-offset advancement after emission. */
void __stdcall emit_runtime_symbol_initializer(CppSymbol *symbol, IrToken *value)
{
    IrToken *destination, *arguments, *length;
    unsigned bytes;
    if (!check_static_initializer(symbol, &value))
        report_declaration_diagnostic(204, 97);
    destination = initializer_object_expression(symbol->type);
    if (is_character_array_initializer(symbol, value)) {
        value->tail.string.length = fit_string_initializer(symbol, value);
        if (!options.configuration.option_qc && !(options.configuration.option_M & 4)) {
            length = make_integer_constant(value->tail.string.length);
            arguments = append_expression(0, destination, 1);
            arguments = append_expression(arguments, value, 1);
            arguments = append_expression(arguments, length, 1);
            emit_runtime_initializer_list(0xac, arguments);
        } else {
            length = make_integer_constant(value->tail.string.length);
            emit_runtime_initializer_call(0xac, destination, value, length);
        }
        initializer_offset += value->tail.string.length;
    } else {
        destination->type = symbol->type;
        bytes = type_size(symbol->type);
        emit_initializer_assignment(destination, value);
        initializer_offset += bytes;
    }
}

/* Original0x45bf0..0x45c13, stdcall8. The operation is represented
   by an integer node; preserve the supplied argument list for expression IR. */
void __stdcall emit_runtime_initializer_list(unsigned operation, IrToken *arguments)
{
    IrToken *selector = make_integer_constant(operation);
    emit_expression_statement(new_left_typed_expression(0x81,selector,arguments));
}

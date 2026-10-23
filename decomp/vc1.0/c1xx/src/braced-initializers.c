/* Aggregate initialization and its nested element contexts. */
#include "braced-initializers.h"
#include "lexer.h"
#include "cpp-context.h"
#include "constructor-initializers.h"
#include "binary-expressions.h"
#include "numeric-conversion.h"
#include "type-sizes.h"
#include <recon.h>

int initializer_excess_reported; /* original 0x915a8 */
int braced_initializer_depth = -1; /* original 0xa0a80 */
CType *initializer_selected_type; /* original 0x91584 */
BracedInitializer *current_braced_initializer; /* original 0x915ac */
BracedInitializer braced_initializers[32]; /* original 0x915b0..0x91830 */

/* Original 0x46590..0x466fc. Establish the root element context,
   select static/runtime handling and initialize browser ownership. */
void __stdcall begin_braced_initializer(CppSymbol *symbol)
{
    int force_static;
    CppSymbol *tag;
    unsigned storage;
    if (!symbol) {
        token_allocation_flags &= ~0x00100000UL;
        return;
    }
    initializer_excess_reported = 0;
    braced_initializer_depth = -1;
    initializer_selected_type = 0;
    force_static = (symbol->attributes & 0x500) == 0x500;
    initializer_storage_symbol = symbol;
    initializer_value_error_reported = 0;
    initializer_constructor_offset = 0;
    initializer_constructor_count = 0;
    initializer_offset = 0;
    push_braced_initializer(symbol);
    current_braced_initializer->flags = 1;
    if (symbol_construction_class(current_braced_initializer->symbol))
        begin_runtime_initializer(current_braced_initializer->symbol);
    if (is_unqualified_class_type(current_braced_initializer->symbol->type)) {
        tag = cpp_context_stack->items[current_braced_initializer->symbol->type->field08];
        if (!(tag->detail.class_info->flags2 & 1)) {
            report_syntax_error(552, current_braced_initializer->symbol->name);
            current_braced_initializer->flags |= 0x20;
        }
    }
    if (is_scalar_initializer_symbol(current_braced_initializer->symbol) || force_static)
        current_braced_initializer->flags |= 4;
    storage = current_braced_initializer->symbol->storage[0] & 7;
    if (!declaration_mode || storage < 4 || storage > 6) {
        pending_initializer_symbol = symbol;
        if (browser_filename) {
            browser_begin_tag();
            browser_enter_tag(pending_initializer_symbol);
        }
    }
    token_allocation_flags &= ~0x00100000UL;
}

/* Original 0x47480..0x475a2. Reuse each nesting slot's owned symbol,
   class-info and type copies, then reset its element cursor and state flags. */
void __stdcall push_braced_initializer(CppSymbol *symbol)
{
    unsigned char kind;
    ++braced_initializer_depth;
    current_braced_initializer = &braced_initializers[braced_initializer_depth];
    if (braced_initializer_depth >= 32) fatal_error(54);
    if (!current_braced_initializer->symbol) {
        current_braced_initializer->symbol = allocate_permanent(sizeof(CppSymbol));
        current_braced_initializer->class_info = allocate_permanent(sizeof(ClassInfo));
    }
    copy_initializer_symbol(current_braced_initializer->symbol,
        current_braced_initializer->class_info, symbol);
    if (!current_braced_initializer->type)
        current_braced_initializer->type = allocate_permanent(sizeof(CType));
    *current_braced_initializer->type = *symbol->type;
    current_braced_initializer->symbol->type = current_braced_initializer->type;
    if (symbol->attributes & 2) {
        current_braced_initializer->symbol->type->related =
            current_braced_initializer->symbol->type->related->related;
        current_braced_initializer->symbol->attributes &= ~2U;
    }
    current_braced_initializer->phase = 0;
    current_braced_initializer->offset = 0;
    current_braced_initializer->flags = 0;
    kind = symbol->kind & 31;
    if ((kind == 2 || kind == 3) && (symbol->flags & 3) != 3)
        report_syntax_error(557, symbol->name);
}

/* Exact meaningful declaration prefixes, indexed by original symbol kind.
   The zero terminator ends this table; following image strings are not rows. */
unsigned initializer_symbol_bytes[21] = {
    52,52,40,38,40,28,36,32,40,52,36,52,52,32,52,24,16,16,16,52,0
};

/* Original 0x47410..0x4747e. Copy only the declaration variant's
   prefix, and give a class tag its separately owned class-info copy. Other
   bytes in the destination declaration retain their prior contents. */
void __stdcall copy_initializer_symbol(CppSymbol *destination, ClassInfo *class_info,
    CppSymbol *source)
{
    unsigned kind = source->kind & 31;
    unsigned bytes = initializer_symbol_bytes[kind], index;
    unsigned char *to = (unsigned char *)destination;
    unsigned char *from = (unsigned char *)source;
    for (index = 0; index < bytes; ++index) to[index] = from[index];
    if (kind == 4) {
        destination->detail.class_info = class_info;
        *class_info = *source->detail.class_info;
    }
}

/* Original 0x475b0..0x47625. Enter another explicit brace level,
   preserving constructor batching and one excess-initializer diagnostic. */
void begin_nested_initializer(void)
{
    flush_initializer_constructors();
    if (braced_initializer_depth < 0) return;
    if (is_scalar_initializer_symbol(current_braced_initializer->symbol))
        push_braced_initializer(current_braced_initializer->symbol);
    else if (!advance_braced_initializer(0)) {
        if (!initializer_excess_reported) {
            ++initializer_excess_reported;
            report_syntax_error(78);
        }
        return;
    }
    current_braced_initializer->flags = 1;
    if (is_scalar_initializer_symbol(current_braced_initializer->symbol))
        current_braced_initializer->flags |= 4;
}

/* Original 0x480f0..0x48268. Emit a pending run of constructed
   array elements using an owned temporary symbol/type, then retain whether
   the emitted expression introduced a diagnostic. */
void flush_initializer_constructors(void)
{
    int saved_errors = error_count;
    IrToken *object, *offset;
    CppSymbol *temporary;
    char *name;
    ArenaPool *pool;
    unsigned count;
    if (initializer_value_error_reported || !initializer_constructor_count) return;
    offset = make_integer_constant(initializer_constructor_offset);
    object = new_ir_expression(0x59, symbol_reference(initializer_storage_symbol),
        offset, type_0000);
    name = make_anonymous_identifier();
    temporary = new_declarator(name);
    *temporary = *initializer_storage_symbol;
    temporary->name = name;
    pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
    temporary->type = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    *temporary->type = *initializer_storage_symbol->type;
    count = initializer_constructor_count;
    temporary->type->related = intern_type_shape(new_type_shape(2,
        initializer_storage_symbol->type->related->attributes, &count));
    if (initializer_storage_symbol->attributes & 2) {
        apply_symbol_based_type(temporary,
            initializer_storage_symbol->type->related->data.based_symbol);
        temporary->type->related = intern_type_shape(temporary->type->related);
    }
    temporary->type = remove_type_qualifiers(temporary->type, 0x3000);
    object->type = temporary->type;
    object->type = parameter_reference_type(object);
    emit_expression_statement(make_object_constructor_call(0, 0, 0, temporary,
        0, object, 0));
    initializer_constructor_count = 0;
    if (error_count > saved_errors) initializer_value_error_reported = 1;
}

/* An anonymous aggregate can expose its following declarations directly.
   Keep this local predicate inline rather than inventing an application call. */
#define INITIALIZER_ANONYMOUS_CHAIN(symbol_) \
    (((symbol_)->kind & 31) >= 1 && ((symbol_)->kind & 31) <= 13 && \
     ((symbol_)->kind & 31) != 5 && (symbol_)->name == name_unnamed && \
     ((symbol_)->attributes & 0x800))

/* Original 0x46c40..0x46f43. Descend into array elements and completed
   aggregate members while keeping each level's writable symbol/type copies.
   A character string or scalar can initialize a complete current element. */
int __stdcall advance_braced_initializer(IrToken *value)
{
    CppSymbol *symbol, *tag, *member, *first;
    CType *type;
    TypeShape *shape;
    unsigned kind;
    int unbounded;
    char *name;
    if (braced_initializer_depth < 0 || (current_braced_initializer->flags & 0x20))
        return 0;
    symbol = current_braced_initializer->symbol;
    if (is_scalar_initializer_symbol(symbol))
        return !(current_braced_initializer->flags & 8);
    if (is_character_array_initializer(symbol, value)) return 1;
    if (current_braced_initializer->flags & 2) return 0;
    if (symbol->attributes & 0x400) {
        shape = current_braced_initializer->type->related;
        unbounded = (symbol->attributes & 0x100) &&
            (shape->flags & 0x77) == 2 && !shape->data.array_bound;
        push_braced_initializer(symbol);
        symbol = current_braced_initializer->symbol;
        type = drop_first_type_shape(symbol->type);
        *current_braced_initializer->type = *type;
        symbol->type = current_braced_initializer->type;
        initializer_selected_type = unbounded ? current_braced_initializer->type : 0;
        shape = symbol->type->related;
        if (shape && (shape->flags & 0x77) == 2) symbol->attributes |= 0x400;
        else symbol->attributes &= ~0x400U;
        return 1;
    }
    kind = symbol->type->flags & 0x2fdf;
    if (kind != 0x402 && kind != 0x408) {
        if (kind != 0x412 && kind != 0x414 && kind != 0x418) {
            current_braced_initializer->flags |= 0x20;
            return 0;
        }
        tag = cpp_context_stack->items[symbol->type->field08];
        if (!(tag->flags & 4)) {
            name = symbol->name;
            if (braced_initializer_depth >= 0) {
                --braced_initializer_depth;
                current_braced_initializer = &braced_initializers[braced_initializer_depth];
                if (braced_initializer_depth >= 0)
                    report_syntax_error(79, current_braced_initializer->symbol->name, name);
            }
            braced_initializer_depth = -1;
            return 0;
        }
        tag = cpp_context_stack->items[symbol->type->field08];
        if (!(tag->detail.class_info->trivial_special_members & 1)) {
            current_braced_initializer->flags |= 0x10;
            return 1;
        }
        tag = cpp_context_stack->items[symbol->type->field08];
        push_braced_initializer(tag);
        if (!INITIALIZER_ANONYMOUS_CHAIN(symbol))
            symbol = current_braced_initializer->symbol;
    }
    first = INITIALIZER_ANONYMOUS_CHAIN(symbol) ? symbol->next : symbol->auxiliary.scope_members;
    member = find_next_initializer_member(0, first);
    if (!member) {
        current_braced_initializer->symbol->auxiliary.scope_members = 0;
        return finish_braced_initializer_element();
    }
    current_braced_initializer->symbol->auxiliary.scope_members = member;
    push_braced_initializer(member);
    symbol = current_braced_initializer->symbol;
    if (is_character_array_initializer(symbol, value) && (symbol->attributes & 0x100))
        initializer_selected_type = drop_first_type_shape(symbol->type);
    return 1;
}

#undef INITIALIZER_ANONYMOUS_CHAIN

/* A static bit-field unit accumulates adjacent members before emission. */
unsigned initializer_bit_field_offset; /* original 0x9159c */
unsigned initializer_bit_field_value; /* original 0x915a0 */
int initializer_bit_field_pending; /* original 0x915a4 */

/* Original 0x46700..0x46bcd. Initialize one braced element, preserving
   the separately normalized value, static-to-runtime transition, packed
   bit-field units, and the array/string cursor at the current nesting level. */
void __stdcall initialize_element(IrToken *value, int mode)
{
    IrToken *normalized;
    CppSymbol *symbol;
    CType *type;
    unsigned flags, width, bound;
    long converted;
    int string;
    if (!value || braced_initializer_depth < 0 ||
        (current_braced_initializer->flags & 0x20)) return;
    value = fold_symbol_constant(value);
    normalized = normalize_initializer_expression(value);
    if (!select_braced_initializer(normalized)) {
        if (!initializer_excess_reported) {
            initializer_excess_reported = 1;
            report_syntax_error(78);
        }
        return;
    }
    if (!value->type)
        value = resolve_assignment_operand(current_braced_initializer->type, normalized);
    if (!value) {
        report_expression_error(0x7e, normalized);
        return;
    }
    normalized = value;
    if (!(token_allocation_flags & 0x200000) &&
        !check_static_initializer(current_braced_initializer->symbol, &normalized)) {
        if (initializer_storage_symbol->storage[2] & 2) {
            report_syntax_error(482, format_symbol_name(initializer_storage_symbol));
            return;
        }
        begin_runtime_initializer(initializer_storage_symbol);
        initializer_runtime_override = 1;
        initializer_runtime_start = initializer_offset;
    }
    symbol = current_braced_initializer->symbol;
    if ((symbol->kind & 31) == 2) {
        if (!initializer_bit_field_pending) {
            initializer_bit_field_pending = 1;
            initializer_bit_field_value = 0;
            initializer_bit_field_offset = symbol->detail.value;
        }
        flags = normalized->type->flags;
        if (!(flags & 0x140)) {
            report_syntax_error(77, symbol->name);
            finish_braced_initializer_element();
            return;
        }
        if ((normalized->flags & 7) == 3) {
            if (flags & 0x100) {
                if (extended_to_integer(normalized->value.pointer, &converted))
                    compiler_warning(2, 56);
                normalized->value.integer = converted;
            }
            symbol = current_braced_initializer->symbol;
            initializer_bit_field_value |=
                (((1U << symbol->auxiliary.bit_field.width) - 1) &
                  normalized->value.integer) << symbol->auxiliary.bit_field.offset;
        } else if ((token_allocation_flags & 0x200000) &&
                   (initializer_runtime_override || options.configuration.extensions_enabled))
            emit_runtime_bit_field_initializer(value);
        finish_braced_initializer_element();
        return;
    }
    if (current_braced_initializer->flags & 0x10) {
        value = prepare_constructor_arguments(
            cpp_context_stack->items[symbol->type->field08], value);
        type = current_braced_initializer->symbol->type;
        if (!type->related) type->attributes &= ~0x3000;
        type = strip_based_shape(current_braced_initializer->symbol->type);
        emit_expression_statement(prepare_expression(make_object_constructor_call(
            0, 0, 0, current_braced_initializer->symbol, value,
            initializer_object_expression(make_pointer_to_type(0, type)), 0), 2));
        initializer_offset += type_size(type);
        current_braced_initializer->flags |= 2;
        finish_braced_initializer_element();
        return;
    }
    string = is_character_array_initializer(symbol, normalized);
    if (string)
        normalized->tail.string.length = fit_string_initializer(
            current_braced_initializer->symbol, normalized);
    if (token_allocation_flags & 0x200000)
        emit_runtime_symbol_initializer(current_braced_initializer->symbol,
            prepare_constant_initializer(current_braced_initializer->symbol, value));
    else {
        normalized = prepare_constant_initializer(current_braced_initializer->symbol, normalized);
        write_initializer_value(current_braced_initializer->symbol, normalized, 0);
    }
    if (string) {
        if (current_braced_initializer->offset) {
            if (!current_braced_initializer->phase && options.configuration.extensions_enabled)
                compiler_warning(4, 207);
            else if (!initializer_excess_reported) {
                initializer_excess_reported = 1;
                report_syntax_error(78);
            }
        }
        width = (normalized->tail.string.auxiliary_flags & 1) ? 2 : 1;
        current_braced_initializer->offset += normalized->tail.string.length / width;
        current_braced_initializer->phase = 1;
        bound = current_braced_initializer->symbol->type->related->data.array_bound;
        if (bound) {
            if (current_braced_initializer->offset >= bound) {
                current_braced_initializer->flags |= 2;
                if (current_braced_initializer->offset > bound && !initializer_excess_reported) {
                    initializer_excess_reported = 1;
                    report_syntax_error(78);
                }
            } else {
                width = (normalized->tail.string.auxiliary_flags & 1) ? 2 : 1;
                integer_constant_scratch = width * (bound - current_braced_initializer->offset);
                emit_initializer_constant(0);
                current_braced_initializer->offset = bound;
            }
        }
        if (!(token_allocation_flags & 0x200000))
            initializer_offset += normalized->tail.string.length;
    } else if (!(token_allocation_flags & 0x200000) && !mode) {
        type = strip_based_shape(current_braced_initializer->symbol->type);
        check_conversion_warning(0x7e, type, normalized);
        initializer_offset += type_size(type);
    }
    finish_braced_initializer_element();
}

/* Original 0x46bd0..0x46c32. Descend through implicit aggregate
   levels until the value has a scalar, string, or constructor destination. */
int __stdcall select_braced_initializer(IrToken *value)
{
    while (advance_braced_initializer(value)) {
        if (is_scalar_initializer_symbol(current_braced_initializer->symbol) ||
            is_character_array_initializer(current_braced_initializer->symbol, value) ||
            (current_braced_initializer->flags & 0x10)) return 1;
    }
    return 0;
}

/* Original 0x46f50..0x47000. Skip declarations that do not own an
   initializable data member. Anonymous markers delimit nested aggregate
   lists; padding records also terminate an accumulated bit-field unit. */
CppSymbol *__stdcall find_next_initializer_member(CppSymbol *owner, CppSymbol *member)
{
    unsigned kind;
    while (member) {
        kind = member->kind & 31;
        if (kind >= 1 && kind <= 13 && kind != 5 && member->name == name_unnamed) {
            if (!(member->attributes & 0x1800)) {
                if (initializer_bit_field_pending) {
                    integer_constant_scratch = initializer_bit_field_value;
                    emit_initializer_constant(owner);
                    initializer_bit_field_pending = 0;
                }
                emit_initializer_padding(member);
            } else {
                if (member->attributes & 0x1000) return 0;
                if (member->attributes & 0x800) return member;
            }
        } else if (is_initializer_data_member(member)) return member;
        member = member->next;
    }
    return 0;
}

/* Original 0x30af0..0x30b2d. Bit-fields and ordinary nonstatic data
   members are eligible; unresolved, inherited and static declarations are not. */
int __stdcall is_initializer_data_member(CppSymbol *symbol)
{
    unsigned char kind = symbol->kind & 31;
    if (kind == 2) return 1;
    if (kind != 3 || !symbol->type || (symbol->flags & 0x80) ||
        (symbol->attributes & 0x40)) return 0;
    return 1;
}

/* The cursor is allowed to retain the original one-slot-before-root
   address once depth becomes -1; no symbol is read at that depth. */
#define POP_BRACED_INITIALIZER() \
    (--braced_initializer_depth, \
     current_braced_initializer = braced_initializers + braced_initializer_depth)
#define INITIALIZER_DECLARATION(symbol_) \
    (((symbol_)->kind & 31) >= 1 && ((symbol_)->kind & 31) <= 13 && \
     ((symbol_)->kind & 31) != 5)

/* Original 0x47000..0x47310, including its shared final member block
   at 0x472d0. Finish implicit levels until an explicit brace, next element,
   or exhausted root is reached. */
int finish_braced_initializer_element(void)
{
    CppSymbol *symbol, *member, *next, *tag;
    unsigned kind, bound;
    int nesting;
    for (;;) {
        if (braced_initializer_depth < 0) return 0;
        if (current_braced_initializer->flags & 0x20) {
            if (braced_initializer_depth >= 0) POP_BRACED_INITIALIZER();
            return 1;
        }
        if (current_braced_initializer->flags & 1) {
            current_braced_initializer->flags |= 8;
            return 0;
        }
        if (braced_initializer_depth >= 0) POP_BRACED_INITIALIZER();
        if (braced_initializer_depth < 0) return 0;
        symbol = current_braced_initializer->symbol;
        if (symbol->attributes & 0x400) {
            if (current_braced_initializer->phase && !initializer_excess_reported) {
                initializer_excess_reported = 1;
                report_syntax_error(78);
            }
            ++current_braced_initializer->offset;
            bound = current_braced_initializer->symbol->type->related->data.array_bound;
            if (bound == current_braced_initializer->offset) {
                current_braced_initializer->flags |= 2;
                if (!(current_braced_initializer->flags & 1)) continue;
            }
            return 1;
        }
        kind = symbol->type->flags & 0x2fdf;
        if (kind == 0x402 || kind == 0x408) {
            member = symbol->auxiliary.scope_members;
            if (INITIALIZER_DECLARATION(member) && (member->attributes & 0x800)) {
                nesting = 1;
                do {
                    member = member->next;
                    if (INITIALIZER_DECLARATION(member) && member->name == name_unnamed) {
                        if (member->attributes & 0x800) ++nesting;
                        else if (member->attributes & 0x1000) --nesting;
                    }
                } while (nesting);
            }
            next = find_next_initializer_member(member, member->next);
            if (next) {
                if (initializer_bit_field_pending &&
                    next->detail.value != initializer_bit_field_offset) {
                    integer_constant_scratch = initializer_bit_field_value;
                    emit_initializer_constant(member);
                    initializer_bit_field_pending = 0;
                }
                current_braced_initializer->symbol->auxiliary.scope_members = next;
                return 1;
            }
            if (initializer_bit_field_pending) {
                integer_constant_scratch = initializer_bit_field_value;
                emit_initializer_constant(member);
                initializer_bit_field_pending = 0;
            }
            continue;
        }
        if (kind == 0x404) {
            if (braced_initializer_depth >= 0) POP_BRACED_INITIALIZER();
            if (initializer_bit_field_pending) {
                integer_constant_scratch = initializer_bit_field_value;
                emit_initializer_constant(current_braced_initializer->symbol->auxiliary.scope_members);
                initializer_bit_field_pending = 0;
            }
            tag = cpp_context_stack->items[current_braced_initializer->symbol->type->field08];
            integer_constant_scratch = type_size(current_braced_initializer->symbol->type) -
                type_size(tag->auxiliary.scope_members->type);
            if (integer_constant_scratch) emit_initializer_constant(0);
            if (!(current_braced_initializer->flags & 1)) continue;
            current_braced_initializer->flags |= 2;
            return 1;
        }
        if (kind == 0x412 || kind == 0x418) {
            if (!(current_braced_initializer->flags & 1)) continue;
            current_braced_initializer->flags |= 2;
            return 1;
        }
        return 1;
    }
}
#undef INITIALIZER_DECLARATION
#undef POP_BRACED_INITIALIZER

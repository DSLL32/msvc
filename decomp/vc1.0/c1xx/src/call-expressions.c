/* Call binding, overload selection and function-style conversion expressions. */
#include <stdlib.h>
#include <stdarg.h>
#include "call-expressions.h"
#include "constructor-initializers.h"
#include "class-lookup.h"
#include "member-expressions.h"
#include "operators.h"
#include "return-statements.h"
#include "unary-expressions.h"
#include "lexer.h"
#include "expression-preparation.h"
#include "numeric-conversion.h"
#include "special-members.h"
#include "frontend-records.h"
#include "function-definitions.h"
#include "type-sizes.h"
#include <recon.h>

/* Original 0x65500..0x65a76, stdcall 8. An explicit cast permits the
   scalar/pointer conversions below while diagnosing invalid function, array
   and member-pointer targets. Keep pointer-width warnings before conversion. */
IrToken *__stdcall cast_to_type(CType *type, IrToken *operand)
{
    TypeShape *target_shape, *source_shape;
    CType *source;
    CppSymbol *tag;
    QualifiedDeclarator *path;
    unsigned rank;
    unsigned short target_kind, source_kind, target_attributes, source_attributes;
    unsigned short target_flags, source_flags;
    int source_size, target_size;

    operand = validate_value_expression(operand);
    target_shape = type->related;
    if (target_shape && !(target_shape->flags & 0x77)) {
        type = drop_first_type_shape(type);
        target_shape = type->related;
    }
    target_kind = target_attributes = 0;
    if (target_shape) {
        target_kind = target_shape->flags;
        if ((target_kind & 0x77) == 4) {
            report_syntax_error(0x42);
            return 0;
        }
        if ((target_kind & 0x77) == 2) {
            report_syntax_error(0x43);
            return 0;
        }
        target_attributes = target_shape->attributes;
    }
    source = operand->type;
    if (!source) {
        operand = resolve_assignment_operand(type, operand);
        if (!operand || !(source = operand->type)) {
            report_syntax_error(0x223);
            return 0;
        }
    }
    if (target_kind & 0x40) return cast_to_reference(type, operand);
    if (is_unqualified_class_type(type)) {
        tag = cpp_context_stack->items[type->field08];
        operand = append_expression(0, operand, 2);
        return build_call_expression(identifier_expression(tag->name), operand);
    }
    rank = classify_expression_conversion(type, operand, 0, 0);
    if (rank < 0x400) return convert_initializer(0x6e, type, operand);
    if (rank & 0x1000) {
        report_type_error(0x249, type);
        return 0;
    }
    target_flags = type->flags;
    source_flags = source->flags;
    source_shape = source->related;
    source_kind = source_attributes = 0;
    if (source_shape) {
        source_kind = source_shape->flags;
        source_attributes = source_shape->attributes;
        if ((source_kind & 0x77) == 2)
            source_kind = (source_kind & 0xff89) | 1;
    }
    if (type == type_0080) goto convert;
    if (target_kind & 0x30) {
        if (!source_kind || !(source_kind & 0x30) ||
            ((source_kind ^ target_kind) & 0x20)) {
            report_syntax_error(0x282);
            return 0;
        }
        if (!member_pointer_cast_compatible(source, type, 1)) {
            if (options.configuration.extensions_enabled &&
                source_shape->data.word == target_shape->data.word) {
                compiler_warning(4, 0x86);
                return new_cast_expression(type, operand);
            }
            report_syntax_error(0x282);
            return 0;
        }
        goto convert;
    }
    if ((source_kind & 0x30) && !(target_kind & 0x40)) {
        report_syntax_error(0x283);
        return 0;
    }
    if ((source_flags & 0x80) && !source_shape) {
        report_syntax_error(0x45);
        goto convert;
    }
    if (!source_kind) {
        if (!target_kind) {
            if ((target_flags | source_flags) & 0x400) {
                report_type_conversion(0x1b8, 0, 0x6e, source, type);
                return 0;
            }
        } else {
            if (!(source_flags & 0x40)) {
                report_type_conversion(0x1b8, 0, 0x6e, source, type);
                return 0;
            }
            target_size = pointer_kind_size(target_attributes);
            if (target_size < (int)(source_flags & 15))
                report_type_conversion(0x131, 4, 0x6e, source, type);
            else if (target_size > (int)(source_flags & 15))
                report_type_conversion(0x132, 4, 0x6e, source, type);
        }
        goto convert;
    }
    if (!target_kind) {
        if (target_flags & 0x800)
            return new_typed_unary_expression(0x1a, type, operand);
        if (!(target_flags & 0x40))
            report_type_conversion(0x1b8, 0, 0x6e, source, type);
        if (pointer_kind_size(source_attributes) > (int)(target_flags & 15))
            report_type_conversion(0x131, 4, 0x6e, source, type);
        goto convert;
    }
    source_kind &= 0x77;
    target_kind &= 0x77;
    if ((source_kind == 1 || source_kind == 3) &&
        (target_kind == 1 || target_kind == 3)) {
        source_size = pointer_kind_size(source_attributes);
        target_size = pointer_kind_size(target_attributes);
        if (source_size > target_size)
            report_type_conversion(0x131, 4, 0x6e, source, type);
        else if (source_kind == 1 && target_kind == 1 &&
            (target_attributes & 7) == 5 && (operand->flags & 7) == 5)
            compiler_warning(1, 0x78);
        if (source_kind == 1 && target_kind == 1 &&
            !source_shape->related && !target_shape->related &&
            (source_flags & 0x400) && (target_flags & 0x400)) {
            tag = cpp_context_stack->items[type->field08];
            path = lookup_class_name(tag,
                ((CppSymbol *)cpp_context_stack->items[source->field08])->name,
                1, 0x182);
            if (!path) return new_cast_expression(type, operand);
            return convert_class_pointer(operand, type, source, path);
        }
    }
convert:
    return convert_scalar_initializer(0x6e, type, operand);
}

/* Original 0x39050..0x39583. Scalar/class conversions, callable class objects,
   direct/qualified member calls and indirect function calls share this entry. */
IrToken *__stdcall build_call_expression(IrToken *expression, IrToken *arguments)
{
    IrToken *object = 0, *argument, *result, *prefix;
    QualifiedDeclarator *qualified = 0, *conversion;
    CppSymbol *reference, *function;
    CType *type, *original_type;
    TypeShape *shape;
    unsigned count, rank, kind;
    unsigned short flags;
    int nonvirtual = 0, retain_reference;
    char *target_name, *source_name;

    if (!expression) return 0;
    if (expression->kind == 0xa3) {
        argument = 0;
        if (arguments && !arguments->tail.right) {
            kind = arguments->flags & 7;
            if (kind == 1 || kind == 2) argument = arguments->value.pointer;
            else if (!kind) argument = arguments;
        }
        if (!argument) {
            type = expression->type;
            if ((type->flags & 0x9c0) || type->related || (type->flags & 0x200)) {
                argument = make_integer_constant(0);
                type_warning(3, 0x130, type);
                cpp_parser_flags |= 2;
                result = cast_to_type(type, argument);
                cpp_parser_flags &= ~2U;
                return result;
            }
            if (!resolve_function_style_cast(expression)) return 0;
        } else if (!is_unqualified_class_type(argument->type)) {
            if (is_unqualified_class_type(expression->type)) {
                if (!resolve_function_style_cast(expression)) return 0;
            } else {
                cpp_parser_flags |= 2;
                result = cast_to_type(expression->type, argument);
                cpp_parser_flags &= ~2U;
                return result;
            }
        } else if (!is_unqualified_class_type(expression->type)) {
            cpp_parser_flags |= 2;
            result = cast_to_type(expression->type, argument);
            cpp_parser_flags &= ~2U;
            return result;
        } else {
            conversion = lookup_class_name(cpp_context_stack->items[argument->type->field08],
                encode_type_name(expression->type), 2, 0x181);
            if (!conversion) {
                if (!resolve_function_style_cast(expression)) return 0;
            } else {
                if (find_class_conversion(expression->type, argument->type, &rank)) {
                    target_name = ((CppSymbol *)cpp_context_stack->items[expression->type->field08])->name;
                    source_name = ((CppSymbol *)cpp_context_stack->items[argument->type->field08])->name;
                    report_syntax_error(0x292, target_name, source_name, source_name, target_name);
                    return 0;
                }
                object = builtin_unary_expression(0x3a, argument);
                arguments = 0;
                expression->kind = 2;
                expression->value.pointer = conversion;
            }
        }
    } else if (is_unqualified_class_type(expression->type)) {
        qualified = lookup_class_name(cpp_context_stack->items[expression->type->field08],
            name_call_operator, 2, 0);
        if (qualified) {
            count = count_call_arguments(arguments);
            object = builtin_unary_expression(0x3a, expression);
            function = resolve_function_overload((CppSymbol *)qualified->resolution->binding->target,
                object, arguments, count, 1);
            if (function) {
                bind_selected_overload(qualified, function);
                return make_resolved_call(function, object, arguments, count, 0);
            }
        }
    }

    count = count_call_arguments(arguments);
    if (expression->kind == 2) {
        reference = expression->value.pointer;
        function = reference;
        if ((reference->kind & 31) == 18) {
            qualified = (QualifiedDeclarator *)reference;
            function = (CppSymbol *)qualified->resolution->binding->target;
        }
        kind = function->kind & 31;
        if (kind != 16 && kind != 17 && kind != 6 && kind != 7) {
            report_syntax_error(0x40);
            return 0;
        }
        if (function->name == name_setjmp || function->name == name_catch) {
            if (!(function_symbol->storage[2] & 1))
                compiler_warning(4, 0x263, function->name);
            else report_syntax_error(0x1ea, format_symbol_name(function));
            function_symbol->attributes |= 0x200;
        }
    } else if (expression->kind == 0x5c) {
        nonvirtual = member_call_is_nonvirtual(expression);
        object = expression->value.pointer;
        reference = expression->tail.right->value.pointer;
        qualified = (QualifiedDeclarator *)reference;
        function = (CppSymbol *)qualified->resolution->binding->target;
    } else {
        prefix = 0;
        if (expression->kind == 0x9e)
            expression = lower_member_pointer_call(expression, &prefix);
        type = original_type = expression->type;
        flags = type->related ? type->related->flags : 0;
        if ((flags & 0x77) == 3 || (flags & 0x44) == 0x44 ||
            ((flags & 0x20) && (expression->kind == 0xa4 || expression->kind == 0xa5))) {
            type = drop_first_type_shape(type);
            flags = type->related->flags;
        }
        if ((flags & 0x77) != 4) {
            report_syntax_error(0x40);
            return 0;
        }
        shape = type->related;
        if (validate_call_arguments(original_type, shape, arguments, count) & 0x2000) return 0;
        type = drop_first_type_shape(type);
        result = convert_call_arguments(0, shape->data.parameters, arguments, count);
        result = new_ir_expression(0x7a, expression, result, type);
        if (prefix) result = new_right_typed_expression(0x40, prefix, result);
        return finish_call_result(result, 0, flags);
    }
    retain_reference = (reference->declaration_flags & 0x800) || nonvirtual;
    reference->declaration_flags &= ~0x800;
    function = resolve_function_overload(function, object, arguments, count, 1);
    if (qualified && function) bind_selected_overload(qualified, function);
    return make_resolved_call(function, object, arguments, count, retain_reference ? reference : 0);
}

/* Original 0x3ade0..0x3adff. List nodes use flags category two; a final
   unwrapped expression still contributes one argument. */
unsigned __stdcall count_call_arguments(IrToken *arguments)
{
    unsigned count = 0;
    while (arguments) {
        ++count;
        if ((arguments->flags & 7) != 2) break;
        arguments = arguments->tail.right;
    }
    return count;
}

/* Original 0x38fe0..0x39041. Known objects, and this inside constructors
   or destructors, select the direct rather than virtual member-call path. */
int __stdcall member_call_is_nonvirtual(IrToken *expression)
{
    IrToken *object = expression->value.pointer;
    IrToken *subject;
    if (!object) return 0;
    subject = object->kind == 0x5c ? object->value.pointer : object;
    if (find_expression_symbol(subject)) return 1;
    if (function_symbol && (function_symbol->declaration_flags & 0x3000) &&
        is_this_object_expression(object)) return 1;
    return 0;
}

/* Original 0x3aa60..0x3aae6. The search owns candidate storage until a
   selection succeeds; failure helpers retain the native cleanup policy. */
CppSymbol *__stdcall resolve_function_overload(CppSymbol *functions, IrToken *object,
    IrToken *arguments, unsigned count, unsigned char diagnose)
{
    OverloadSearch *search = new_overload_search(functions, count, diagnose);
    OverloadCandidate *candidate;
    CppSymbol *selected;
    if (!validate_overload_counts(search)) return 0;
    if (!rank_overload_arguments(search, object, arguments)) return 0;
    candidate = select_overload_candidate(search, 0, report_call_overload_failure);
    if (!candidate) return 0;
    selected = candidate->function;
    destroy_overload_search(search);
    if (browser_filename) browser_type_reference(selected);
    return selected;
}

/* Original 0x75eb0..0x75fbd. Explicit-candidate mode (flags bit one) reads
   one extra count argument; ordinary call searches supply only three arguments. */
OverloadSearch *new_overload_search(CppSymbol *functions, unsigned count,
    unsigned flags, ...)
{
    OverloadSearch *search = malloc(sizeof(OverloadSearch));
    unsigned *words, i, bytes;
    unsigned char kind;
    va_list extra;
    if (!search) return 0;
    words = (unsigned *)search;
    for (i = 0; i < sizeof(OverloadSearch) / sizeof(unsigned); ++i) words[i] = 0;
    search->operation = 1;
    search->functions = functions;
    search->flags = (unsigned char)flags;
    search->argument_count = count;
    if (!(flags & 2)) {
        kind = functions->kind & 31;
        if (kind == 16 || kind == 17)
            search->total_count = count_overload_functions(functions);
        else search->total_count = 1;
        if (kind == 17) ++search->argument_count;
    } else {
        va_start(extra, flags);
        search->total_count = va_arg(extra, unsigned);
        va_end(extra);
    }
    search->viable_count = search->total_count;
    bytes = (search->argument_count * sizeof(OverloadArgumentRank) +
        sizeof(OverloadCandidate)) * search->total_count;
    search->storage = malloc(bytes);
    if (!search->storage) {
        free(search);
        return 0;
    }
    words = (unsigned *)search->storage;
    for (i = 0; i < bytes / sizeof(unsigned); ++i) words[i] = 0;
    search->candidates = search->storage;
    search->rank_storage = (OverloadArgumentRank *)((char *)search->storage +
        search->total_count * sizeof(OverloadCandidate));
    search->current = 0;
    search->rank_cursor = 0;
    if (!(flags & 2) && !add_overload_candidates(search, search->functions, 0)) {
        free(search);
        return 0;
    }
    return search;
}

/* Original 0x30b30..0x30ba5. Overload containers expose their declaration
   list at +0xc. The native NULL-input path leaves the cursor unwritten. */
unsigned __stdcall count_overload_functions(CppSymbol *functions)
{
    CppSymbol *cursor;
    unsigned count = 0, kind;
    if (functions) {
        kind = functions->kind & 31;
        cursor = kind == 16 || kind == 17 ? ((OverloadSet *)functions)->functions : functions;
    }
    while (cursor) {
        kind = cursor->kind & 31;
        if (kind != 6 && kind != 7) break;
        cursor = cursor->next;
        ++count;
    }
    return count;
}

/* Original 0x76010..0x76197. Populate linked candidate slots and reserve
   their per-argument ranks, retaining the last occupied cursor for later adds. */
int __stdcall add_overload_candidates(OverloadSearch *search, CppSymbol *functions, QualifiedDeclarator *qualification)
{
    OverloadCandidate *candidate;
    OverloadArgumentRank *ranks;
    CppSymbol *cursor, *function;
    TypeShape *shape;
    FunctionParameters *parameters;
    unsigned kind;
    if (!functions) return 0;
    if (!search->functions) search->functions = functions;
    begin_overload_candidate_cursor(search, &candidate, &ranks);
    kind = functions->kind & 31;
    cursor = kind == 16 || kind == 17 ? ((OverloadSet *)functions)->functions : functions;
    while (cursor) {
        kind = cursor->kind & 31;
        if (kind != 6 && kind != 7) break;
        function = cursor;
        cursor = cursor->next;
        candidate = search->current;
        if (candidate->flags & 0x20) {
            candidate->next = candidate + 1;
            candidate->next->previous = candidate;
            candidate->ranks = search->rank_cursor;
            search->rank_cursor += search->argument_count;
            search->current = candidate->next;
        }
        candidate = search->current;
        candidate->flags |= 0x20;
        ranks = search->rank_cursor;
        candidate->ranks = ranks;
        candidate->function = function;
        shape = function->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        parameters = shape->data.parameters;
        candidate->parameter_cursor = parameters->declarations->types;
        candidate->qualification = qualification;
        if (!(function->declaration_flags & 0x400)) candidate->this_type = 0;
        else {
            candidate->flags |= 4;
            candidate->this_type = (function->storage[0] & 7) == 3 ? 0 : parameters->associated_type;
        }
    }
    save_overload_candidate_cursor(search, candidate, ranks);
    return 1;
}

/* Original 0x75fc0..0x75fea. Start at the storage heads only once. */
void __stdcall begin_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate **candidate, OverloadArgumentRank **ranks)
{
    if (!search->current) {
        search->current = search->storage;
        search->rank_cursor = search->rank_storage;
    }
    *candidate = search->current;
    *ranks = search->rank_cursor;
}

/* Original 0x75ff0..0x76004. */
void __stdcall save_overload_candidate_cursor(OverloadSearch *search,
    OverloadCandidate *candidate, OverloadArgumentRank *ranks)
{
    search->current = candidate;
    search->rank_cursor = ranks;
}

/* Original 0x761a0..0x7623a. Discard arity-incompatible declarations before
   considering individual argument conversions. */
int __stdcall validate_overload_counts(OverloadSearch *search)
{
    OverloadCandidate *candidate;
    TypeShape *shape;
    unsigned count, rank;
    set_overload_phase(search, 1);
    count = search->argument_count;
    for (candidate = search->candidates; candidate; candidate = candidate->next) {
        if (candidate->flags & 0x10) break;
        shape = candidate->function->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        rank = rank_call_argument_count(shape, count - ((candidate->flags & 4) != 0));
        if (rank & 0x2000) rank = 0x2000;
        if (rank == 0x2000 && reject_overload_candidate(candidate, search, 0)) return 0;
    }
    return 1;
}

/* Original 0x76860..0x768a3. The third argument is present only for
   phases three through five; other phase transitions consume two arguments. */
void set_overload_phase(OverloadSearch *search, unsigned phase, ...)
{
    unsigned byte_phase = phase & 255;
    va_list extra;
    search->phase = (unsigned char)byte_phase;
    if (byte_phase != 3 && (byte_phase < 4 || byte_phase > 5 || !search->field2c))
        search->field2c = search->viable_count;
    if (byte_phase >= 3 && byte_phase <= 5) {
        va_start(extra, phase);
        search->field28 = va_arg(extra, unsigned);
        va_end(extra);
    }
}

/* Original 0x77560..0x775f9. Exact/defaulted arities rank above ellipsis;
   mismatch codes distinguish excess and missing explicit arguments. */
unsigned __stdcall rank_call_argument_count(TypeShape *function, unsigned count)
{
    FunctionParameters *parameters;
    unsigned required;
    int variadic;
    if (function->flags & 0x4000) return count ? 0x200e : 1;
    parameters = function->data.parameters;
    required = parameters->declarations->count;
    variadic = (function->flags & 0x8000) != 0;
    if (variadic) --required;
    if (required == count) return 0x800;
    if (count > required) return variadic ? 0x400 : 0x200e;
    return (unsigned)parameters->default_count + count < required ? 0x200f : 0x800;
}

/* Original 0x76240..0x763fb. Rank the implicit object separately, then walk
   the circular argument list in lockstep with each candidate's formal types. */
int __stdcall rank_overload_arguments(OverloadSearch *search, IrToken *object, IrToken *arguments)
{
    OverloadCandidate *candidate;
    OverloadArgumentRank *entry;
    IrToken *argument;
    unsigned index, number, mode, rank;
    for (candidate = search->candidates; candidate; candidate = candidate->next) {
        number = 1;
        index = 0;
        entry = candidate->ranks;
        if (object) {
            set_overload_phase(search, 2);
            if (!candidate->this_type) entry->rank = 1;
            else {
                mode = (search->flags & 4) ? 3 : 2;
                entry->parameter_type = candidate->this_type;
                if ((object->type->attributes & 0x3000) && candidate->function &&
                    (candidate->function->declaration_flags & 0x3000))
                    object->type = remove_type_qualifiers(object->type, 0x3000);
                entry->rank = rank = classify_expression_conversion(candidate->this_type, object, mode, 0);
                if (rank & 0x2000) {
                    if (reject_overload_candidate(candidate, search, object)) return 0;
                    continue;
                }
            }
            index = 1;
            ++entry;
        } else if (candidate->flags & 4) {
            index = 1;
            ++entry;
        }
        argument = arguments;
        while (index < search->argument_count) {
            mode = (search->flags & 4) ? 3 : 2;
            argument = argument->linkage.list_head;
            if (!argument->value.pointer) return 0;
            set_overload_phase(search, 3, number);
            candidate->current_parameter = *candidate->parameter_cursor;
            entry->parameter_type = *candidate->parameter_cursor;
            entry->rank = rank = classify_expression_conversion(*candidate->parameter_cursor,
                argument->value.pointer, mode, 0);
            if (rank & 0x2000) {
                if (reject_overload_candidate(candidate, search, argument->value.pointer)) return 0;
                break;
            }
            if (rank == 0x400) {
                candidate->flags |= 0x40;
                break;
            }
            ++index;
            ++number;
            ++entry;
            ++candidate->parameter_cursor;
        }
    }
    return 1;
}

/* Original 0x76790..0x767f1. Unlink a rejected candidate without freeing
   its shared storage. Exhaustion diagnoses the failure and releases the search. */
int __stdcall reject_overload_candidate(OverloadCandidate *candidate,
    OverloadSearch *search, IrToken *argument)
{
    candidate->flags |= 8;
    if (search->candidates == candidate) search->candidates = candidate->next;
    if (candidate->next) candidate->next->previous = candidate->previous;
    if (candidate->previous) candidate->previous->next = candidate->next;
    if (!(candidate->flags & 1)) --search->viable_count;
    if (search->viable_count) return 0;
    diagnose_overload_failure(candidate, search, argument);
    destroy_overload_search(search);
    return 1;
}

/* Original 0x76700..0x76786. Select the remaining declaration or resolve
   ties, preserving the caller's diagnostic callback and failure cleanup. */
OverloadCandidate *__stdcall select_overload_candidate(OverloadSearch *search,
    unsigned *rank, OverloadFailure failure)
{
    OverloadCandidate *candidate;
    if (search->viable_count != 1) {
        if (!prune_overload_candidates(search, 4, 1)) {
            if (rank) *rank = 0x600c;
            return 0;
        }
        if (search->viable_count != 1) {
            candidate = resolve_overload_tie(search);
            if (candidate) {
                if (rank) *rank = 0x600d;
                return candidate;
            }
            candidate = failure(search);
            if (!candidate) {
                if (rank) *rank = 0x7000;
                destroy_overload_search(search);
            }
            return candidate;
        }
    }
    return first_active_overload(search->candidates);
}

/* Original 0x75cf0..0x75d0a. A selected list contains at least one
   candidate whose deferred-elimination bit is clear. */
OverloadCandidate *__stdcall first_active_overload(OverloadCandidate *candidate)
{
    while (candidate->flags & 1) candidate = candidate->next;
    return candidate;
}

/* Original 0x76840..0x7685d. Candidate and rank storage share one allocation. */
void __stdcall destroy_overload_search(OverloadSearch *search)
{
    free(search->storage);
    free(search);
}

/* Original 0x75e30..0x75e5a. A NULL context requests only the rank. */
void __stdcall record_user_conversion(OverloadCandidate *context, CppSymbol *conversion,
    CType *target, unsigned rank)
{
    OverloadArgumentRank *entry;
    if (context) {
        entry = &context->ranks[context->conversion_index];
        entry->conversion = conversion;
        entry->parameter_type = target;
        entry->rank = rank;
    }
}

/* Original 0x76400..0x7654d. Compare candidates one argument at a time,
   deferring weaker ranks and permanently rejecting ellipsis-only losers. */
int __stdcall prune_overload_candidates(OverloadSearch *search, unsigned char phase, int mode)
{
    OverloadCandidate *best, *candidate, *prior;
    unsigned index, best_rank, rank;
    int order;
    search->field2c = 0;
    for (index = 0; index < search->argument_count; ++index) {
        best = search->candidates;
        best_rank = candidate_argument_rank(best, index, mode);
        set_overload_phase(search, phase, index);
        for (candidate = best->next; candidate; candidate = candidate->next) {
            rank = candidate_argument_rank(candidate, index, mode);
            order = compare_candidate_ranks(rank, candidate, best_rank, best, index);
            if (order < 0) {
                if (rank == 0x400) {
                    if (reject_overload_candidate(candidate, search, 0)) return 0;
                } else if (!(candidate->flags & 1) && defer_overload_candidate(candidate, search))
                    return 0;
            } else if (order > 0) {
                for (prior = search->candidates; prior != candidate; prior = prior->next) {
                    if (best_rank == 0x400) {
                        if ((prior->flags & 0x40) && reject_overload_candidate(prior, search, 0))
                            return 0;
                    } else if (!(prior->flags & 1) && defer_overload_candidate(prior, search))
                        return 0;
                }
                best_rank = rank;
                best = candidate;
            }
        }
    }
    return search->viable_count != 0;
}

/* Original 0x76550..0x76570. First-pass comparison uses coarse rank groups. */
unsigned __stdcall candidate_argument_rank(OverloadCandidate *candidate, unsigned index, int mode)
{
    unsigned rank = candidate->ranks[index].rank;
    return mode ? coarse_conversion_rank(rank) : rank;
}

/* Original 0x75e60..0x75eaf. Standard adjustments share a broad group;
   distance/base conversions and user conversions have their own groups. */
unsigned __stdcall coarse_conversion_rank(unsigned rank)
{
    unsigned group = rank & ~1U;
    if (group == 2 || group == 4 || group == 8) return 1;
    group = rank & ~15U;
    if (group == 0x40 || group == 0x80 || group == 0x100) return 0x20;
    if (rank & 0x200) return 0x200;
    return rank;
}

/* Original 0x76580..0x7668f. Lower ranks are better. Equal derived/base
   ranks compare the formal classes to choose the more specific conversion. */
int __stdcall compare_candidate_ranks(unsigned left_rank, OverloadCandidate *left,
    unsigned right_rank, OverloadCandidate *right, unsigned index)
{
    CType *left_type, *right_type;
    CppSymbol *left_class, *right_class;
    if (left_rank < right_rank) return 1;
    if (right_rank < left_rank) return -1;
    if (!(left_rank & 0x40)) return 0;
    left_type = left->ranks[index].parameter_type;
    if (!left_type->field08) return 0;
    right_type = right->ranks[index].parameter_type;
    if (!right_type->field08 || right_type->field08 == left_type->field08) return 0;
    left_class = cpp_context_stack->items[left_type->field08];
    right_class = cpp_context_stack->items[right_type->field08];
    if (lookup_class_name(left_class, right_class->name, 1, 0)) return 1;
    if (lookup_class_name(right_class, left_class->name, 1, 0)) return -1;
    return 0;
}

/* Original 0x76800..0x7683d. Keep deferred candidates linked for the
   second pass, but remove them from the viable count exactly once. */
int __stdcall defer_overload_candidate(OverloadCandidate *candidate, OverloadSearch *search)
{
    if (!(candidate->flags & 1)) {
        candidate->flags |= 1;
        if (!--search->viable_count) {
            diagnose_overload_failure(candidate, search, 0);
            destroy_overload_search(search);
            return 1;
        }
    }
    return 0;
}

/* Original 0x32080..0x3209c. Rebind a qualified overload reference before
   applying the normal member-access check to the chosen declaration. */
void __stdcall bind_selected_overload(QualifiedDeclarator *reference, CppSymbol *function)
{
    if (reference) {
        reference->resolution->binding->target = (ClassReference *)function;
        check_qualified_member_access(reference);
    }
}
/* Original 0x39590..0x3999e. Build direct/intrinsic/member call records,
   preserving temporary construction, virtual dispatch and trivial-call elision. */
IrToken *__stdcall make_resolved_call(CppSymbol *symbol, IrToken *object,
    IrToken *arguments, unsigned count, CppSymbol *qualified)
{
    IrToken *call, *descriptor, *adjusted, *result, *this_argument, *temporary;
    CppSymbol *function, *dispatch_class;
    CType *temporary_type;
    TypeShape *shape;
    ArenaPool *pool = &compiler_arena.pools[2];
    unsigned index, kind;
    unsigned short operation, flags;
    int direct = qualified != 0;

    if (!symbol) return 0;
    if (symbol->declaration_flags & 2) {
        report_syntax_error(0x108, format_symbol_name(symbol));
        return 0;
    }
    if (token_allocation_flags & 0x20000020UL) call = allocate_permanent(sizeof(IrToken));
    else {
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        call = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    call->flags = 2;
    call->kind = 0x7a;
    kind = symbol->kind & 31;
    function = kind == 16 || kind == 17 ?
        first_vector_item((Vector *)((OverloadSet *)symbol)->functions) : symbol;
    call->type = function_return_type(function);
    shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    call->tail.right = convert_call_arguments(function, shape->data.parameters, arguments, count);
    if ((function->kind & 31) == 6) {
        operation = function->detail.intrinsic.operation;
        if ((operation == 0xd9 && !(token_allocation_flags & 0x04000000UL) && !seh_except_depth) ||
            (operation == 0xda && !(token_allocation_flags & 0x04000000UL)) ||
            (operation == 0xd7 && !seh_finally_depth))
            report_syntax_error(0x2c3, symbol->name);
        if (symbol->detail.intrinsic.operation == 0xd8 && (function_symbol->storage[2] & 1))
            report_syntax_error(0x1ea, format_symbol_name(symbol));
        call->kind = 0x81;
        call->value.pointer = make_integer_constant(operation);
    } else if (!(function->declaration_flags & 0x400)) {
        call->value.pointer = symbol_reference(symbol);
        defer_generated_member(function);
    } else if (!(function->cpp.parts.member_flags & 1))
        call->value.pointer = symbol_reference(symbol);
    else {
        if (!object) {
            if (!(function->declaration_flags & 0x1000)) {
                report_syntax_error(0x160,
                    ((CppSymbol *)cpp_context_stack->items[function->auxiliary.declaring_class])->name,
                    function->name);
                return 0;
            }
            shape = function->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            temporary_type = drop_first_type_shape(shape->data.parameters->associated_type);
            if (next_symbol_index >= 0x7fffffffUL) fatal_error(0x37);
            if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
            index = next_symbol_index++;
            temporary = new_temporary_reference(index, temporary_type);
            shape = function->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            object = new_cast_expression(shape->data.parameters->associated_type, temporary);
            register_temporary_destruction(object, temporary_type);
        }
        adjusted = prepare_member_call_object(object, function, qualified, &dispatch_class);
        if ((function->cpp.parts.member_flags & 0x20) && arguments) {
            result = try_elide_copy_construction(function, adjusted, arguments);
            if (result) return result;
        }
        result = simplify_trivial_member_call(function, adjusted, arguments);
        if (result) return result;
        if (token_allocation_flags & 0x20000020UL)
            descriptor = allocate_permanent(sizeof(IrToken));
        else {
            if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
            descriptor = (IrToken *)pool->next;
            pool->remaining -= sizeof(IrToken);
            pool->next += sizeof(IrToken);
        }
        if (adjusted->kind == 0x6e) adjusted = adjusted->value.pointer;
        shape = function->type->related;
        if (shape && !(shape->flags & 0x77)) shape = shape->related;
        this_argument = new_typed_unary_expression(0x46, shape->data.parameters->associated_type, adjusted);
        if (!(function->storage[1] & 2) || direct) {
            defer_generated_member(function);
            descriptor->kind = 0xa4;
            descriptor->value.pointer = symbol_reference(symbol);
        } else {
            descriptor->kind = 0xa5;
            descriptor->value.pointer = make_integer_constant(function->detail.virtual_table_offset);
            this_argument = new_typed_unary_expression(0x46,
                dispatch_class->detail.class_info->virtual_dispatch_pointer->type, this_argument);
        }
        if (function->declaration_flags & 0x1000) {
            shape = function->type->related;
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            call->type = shape->data.parameters->associated_type;
        }
        descriptor->flags = 2;
        descriptor->tail.right = this_argument;
        descriptor->type = function->type;
        call->value.pointer = descriptor;
    }
    shape = function->type->related;
    flags = shape->flags;
    if ((flags & 0x77) != 4) flags = shape->related->flags;
    return finish_call_result(call, function, flags);
}
/* Original 0x3aaf0..0x3ad78. Convert explicit arguments against the formal
   list, retain ellipsis operands, then materialize and splice default arguments. */
IrToken *__stdcall convert_call_arguments(CppSymbol *function, FunctionParameters *parameters,
    IrToken *arguments, int count)
{
    CType **formal = parameters->declarations->types;
    CType *type;
    CppSymbol *overload, *selected;
    IrToken *node = arguments, *value, *defaults_list, *tail;
    DefaultArgument *argument;
    int remaining = parameters->declarations->count;
    int failed = 0, member_form, skip, i;

    if (arguments) {
        node = arguments->linkage.list_head;
        while (count > 0) {
            --count;
            value = node->value.pointer;
            if (!value || (value->flags & 8)) return 0;
            type = *formal;
            if (!type) {
                type = type_0000;
                if (!node->type) {
                    member_form = value->kind != 2;
                    if (member_form) value = value->tail.right;
                    overload = value->value.pointer;
                    if ((overload->kind & 31) == 18)
                        overload = (CppSymbol *)((QualifiedDeclarator *)overload)->resolution->binding->target;
                    selected = ((OverloadSet *)overload)->functions;
                    value->value.pointer = selected;
                    value->type = selected->type;
                    node->type = selected->type;
                    if (!has_single_function(overload))
                        report_call_argument_error(0x1fa, ((OverloadSet *)overload)->functions, 0);
                    else if (member_form) {
                        selected = value->value.pointer;
                        if ((selected->kind & 31) == 7 && (selected->declaration_flags & 0x400) &&
                            (selected->storage[0] & 7) != 3) {
                            report_syntax_error(0x114, "... (argument)");
                            return 0;
                        }
                    }
                    type = type_0000;
                }
            } else {
                if (!node->type) {
                    value = resolve_assignment_operand(type, value);
                    node->value.pointer = value;
                }
                ++formal;
                --remaining;
            }
            node->type = type;
            if (type == type_0000) node = node->linkage.list_head;
            else {
                value = convert_initializer(0x7a, type, value);
                node->value.pointer = value;
                value = prepare_reference_initializer(type, value);
                node->value.pointer = value;
                if (value) {
                    node->type = type;
                    node = node->linkage.list_head;
                } else failed = 1;
            }
        }
    }
    if (!*formal || !remaining || (remaining == 1 && (!*formal || *formal == type_0080)))
        return arguments;
    argument = parameters->defaults;
    skip = remaining - 1;
    while (skip > 0 && argument->next) {
        --skip;
        argument = argument->next;
    }
    if (failed) return 0;
    defaults_list = 0;
    for (i = remaining; i > 0; --i) {
        if (((IrToken *)argument->expression)->kind == 0xc3) {
            if (function) report_syntax_error(0x1b3, format_symbol_name(function));
            else report_syntax_error(0x1b9);
            return 0;
        }
        defaults_list = append_default_call_argument(defaults_list, argument->expression);
        argument = argument->following;
    }
    if (node) {
        tail = defaults_list->linkage.list_head;
        tail->tail.right = arguments;
        defaults_list->linkage.list_head = arguments->linkage.list_head;
        node->linkage.list_head = tail;
    }
    return defaults_list;
}

/* Original 0x67760..0x67818. Compose an explicit qualification with the
   member's inherited path, then adjust the object to its declaring subobject. */
IrToken *__stdcall prepare_member_call_object(IrToken *object, CppSymbol *function,
    CppSymbol *qualified, CppSymbol **dispatch_class)
{
    QualifierBinding *binding, *prefix;
    TypeShape *shape;
    if (!qualified) {
        binding = find_member_call_binding(1, cpp_context_stack->items[object->type->field08],
            function, dispatch_class);
        if (!binding) return object;
    } else {
        binding = ((QualifiedDeclarator *)qualified)->resolution->binding;
        prefix = find_member_call_binding(0,
            cpp_context_stack->items[function->auxiliary.declaring_class], function, dispatch_class);
        if (prefix) binding = attach_qualifier_parent(prefix, binding->next);
    }
    shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    object = adjust_member_binding_object(object, binding, 0, 1, shape->data.parameters->associated_type);
    if (qualified) object = make_member_at_offset(object, function->field28, object->type);
    return object;
}

/* Original 0x78560..0x785ae. Explicit qualifications use the declaring
   class; implicit virtual calls may instead use the virtual-table class. */
QualifierBinding *__stdcall find_member_call_binding(int implicit, CppSymbol *tag,
    CppSymbol *function, CppSymbol **dispatch_class)
{
    short index = implicit && (function->storage[1] & 2) ?
        function->auxiliary.virtual_function.table_class : function->auxiliary.declaring_class;
    CppSymbol *owner = cpp_context_stack->items[index];
    if (dispatch_class) *dispatch_class = owner;
    if (owner == tag) return 0;
    return find_named_class_binding(tag, owner->name, 1);
}

/* Original 0x39bf0..0x39c8a. Trivial default construction/destruction
   preserves the object; trivial copy and assignment become a plain assignment. */
IrToken *__stdcall simplify_trivial_member_call(CppSymbol *function, IrToken *object, IrToken *arguments)
{
    TypeShape *shape = function->type->related;
    CppSymbol *tag;
    IrToken *source, *target;
    unsigned char members = function->cpp.parts.member_flags, trivial;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    tag = cpp_context_stack->items[shape->data.parameters->associated_type->field08];
    trivial = tag->detail.class_info->trivial_special_members;
    if (((members & 0x10) && (trivial & 1)) ||
        ((function->declaration_flags & 0x2000) && (trivial & 8))) return object;
    if (((members & 0x20) && (trivial & 2)) || ((members & 0x40) && (trivial & 4))) {
        source = builtin_unary_expression(OP_DEREFERENCE, arguments->value.pointer);
        target = builtin_unary_expression(OP_DEREFERENCE, object);
        return builtin_binary_expression(OP_ASSIGN, target, source);
    }
    return 0;
}

/* Original 0x399a0..0x39ba7. Supply hidden class-result storage, recover
   reference results, and retain constructor-specific virtual-base arguments. */
IrToken *__stdcall finish_call_result(IrToken *call, CppSymbol *function, unsigned short flags)
{
    int constructor = function && (function->declaration_flags & 0x1000);
    CType *result_type = call->type, *pointer_type;
    TypeShape *shape;
    CppSymbol *tag, *temporary;
    IrToken *actual_call, *object, *argument;
    unsigned index;
    unsigned char storage;

    if (flags & 0x200) {
        tag = class_tag_for_type(result_type);
        actual_call = call->kind == 0x40 ? call->tail.right : call;
        pointer_type = make_class_pointer_type(tag, 0);
        if (initializing_symbol) {
            if (member_initializer_active) {
                if (options.configuration.extensions_enabled)
                    report_syntax_error(0x162, initializing_symbol->name);
                else compiler_warning(1, 0x162, initializing_symbol->name);
            }
            storage = member_initializer_active ? 4 : (initializing_symbol->storage[0] & 7);
            temporary = create_initializer_temporary(storage, result_type);
            object = resolve_symbol_expression(temporary);
            object = builtin_unary_expression(0x3b, object);
            record_initialized_temporary(temporary, result_type);
        } else {
            if (next_symbol_index >= 0x7fffffffUL) fatal_error(0x37);
            if (next_symbol_index >= browser_symbol_limit && browser_filename)
                browser_symbol_rollover();
            index = next_symbol_index++;
            object = new_temporary_reference(index, result_type);
            register_temporary_destruction(object, result_type);
        }
        argument = append_expression(0, object, 2);
        argument->type = pointer_type;
        if (!actual_call->tail.right) actual_call->tail.right = argument;
        else {
            argument->linkage.list_head = actual_call->tail.right->linkage.list_head;
            actual_call->tail.right->linkage.list_head->tail.right = argument;
            actual_call->tail.right->linkage.list_head = argument;
        }
        actual_call->type = pointer_type;
        call->type = pointer_type;
    }
    if (token_allocation_flags & 0x800) call = qualify_based_storage(call);
    if (!call) return 0;
    shape = call->type ? call->type->related : 0;
    if (shape && (shape->flags & 0x40)) {
        call = builtin_unary_expression(0x46, call);
        call->flags |= 0x10;
    } else if (constructor || (flags & 0x200))
        call = builtin_unary_expression(0x46, call);
    else if (flags & 0x100) call = materialize_call_result(call);
    if (constructor)
        call = finish_constructor_result(call,
            cpp_context_stack->items[function->auxiliary.declaring_class]);
    return call;
}

/* Original 0x39bb0..0x39bed. A complete-object constructor with virtual
   bases receives the hidden true argument after its explicit argument list. */
IrToken *__stdcall finish_constructor_result(IrToken *call, CppSymbol *tag)
{
    IrToken *constructor_call, *argument;
    if (!call) return 0;
    if (tag->detail.class_info->flags & 0x10) {
        constructor_call = call->value.pointer;
        argument = make_integer_constant(1);
        constructor_call->tail.right = append_expression(constructor_call->tail.right, argument, 2);
    }
    return call;
}

/* Original 0x4f7c0..0x4f85c. Resolve an operator-new call's allocation
   model while retaining the surrounding expression wrapper, when present. */
IrToken *__stdcall finish_call_expression(IrToken *expression, unsigned short flags)
{
    IrToken *wrapper = 0, *callee;
    CppSymbol *function;
    if (!expression) return 0;
    if (flags & 4) flags = 4;
    if (expression->kind == 0x67) {
        wrapper = expression;
        expression = expression->value.pointer;
    }
    if (expression->kind == 0x7a) {
        callee = expression->value.pointer;
        if ((callee->flags & 7) == 4 &&
            ((CppSymbol *)callee->value.pointer)->name == name_new) {
            function = select_allocation_function(flags, callee->value.pointer);
            if (function->declaration_flags & 0x200) expression->flags |= 8;
            callee->value.pointer = function;
            callee->type = strip_based_shape(function->type);
            expression->type = drop_first_type_shape(callee->type);
        }
    }
    if (wrapper) {
        wrapper->value.pointer = expression;
        wrapper->type = expression->type;
        return wrapper;
    }
    return expression;
}

/* Original 0x38f80..0x38fda. Recognize the unary reference to the
   interned this parameter, excluding the special member-reference token. */
int __stdcall is_this_object_expression(IrToken *expression)
{
    IrToken *operand;
    if ((expression->flags & 7) != 1) return 0;
    operand = expression->value.pointer;
    if (!operand || (operand->flags & 7) != 4 || operand->kind == 0xa7) return 0;
    if (((CppSymbol *)operand->value.pointer)->name != name_this) return 0;
    return 1;
}

/* Original 0x785b0..0x785ee. Prefer the restricted path search, then
   relax one category flag. Each attempt retains its own original call edge. */
QualifierBinding *__stdcall find_named_class_binding(CppSymbol *tag, char *name,
    unsigned categories)
{
    QualifierResolution *resolution = find_class_name_paths(tag, name, categories | 12);
    if (!resolution) resolution = find_class_name_paths(tag, name, categories | 8);
    return resolution ? resolution->binding : 0;
}

/* Original 0x38ee0..0x38f7d, stdcall 4. Resolve the class name
   to its constructor overload set while retaining an explicit qualifier. */
int __stdcall resolve_function_style_cast(IrToken *expression)
{
    CppSymbol *symbol = expression->value.pointer;
    CppSymbol *constructors;
    QualifiedDeclarator *qualified = 0;
    if ((symbol->kind & 31) == 18) {
        qualified = (QualifiedDeclarator *)symbol;
        symbol = (CppSymbol *)qualified->resolution->binding->target;
    }
    if ((symbol->storage[0] & 7) == 7 && is_unqualified_class_type(symbol->type))
        symbol = cpp_context_stack->items[symbol->type->field08];
    if (!(symbol->declaration_flags & 0x1000)) {
        report_syntax_error(514, symbol->name);
        return 0;
    }
    expression->kind = 2;
    constructors = find_constructor_overloads(symbol);
    if (qualified)
        qualified->resolution = append_qualifier_reference(0,
            qualified->resolution->binding, constructors);
    else expression->value.pointer = constructors;
    return 1;
}

/* Original 0x3dcb0..0x3dd6e, stdcall 8. Nontrivial class-result
   temporaries receive an expression-owned destruction record. */
void __stdcall register_temporary_destruction(IrToken *object, CType *type)
{
    TemporaryFrontendRecord record;
    CppSymbol *tag;
    record.header.kind = 13;
    record.header.flags = 0;
    record.header.field02 = 0;
    record.header.sequence = 0;
    record.header.next = record.header.previous = 0;
    record.object = 0;
    record.field14 = 0;
    if (ir_records_suppressed || !declaration_mode || allow_incomplete_class || !type)
        return;
    tag = cpp_context_stack->items[type->field08];
    if ((tag->kind & 31) != 4 || !(tag->declaration_flags & 0x2000) ||
        (tag->detail.class_info->trivial_special_members & 8)) return;
    record.header.kind &= ~0x40;
    record.header.kind &= ~0x80;
    record.object = object;
    append_frontend_record(&record.header, sizeof(record));
}

typedef char ElisionParameterDefaultCountOffsetMustBe4[
    (offsetof(FunctionParameters, default_count) == 4) ? 1 : -1];
/* Original 0x39c90..0x39d22, stdcall 12. Reuse a constructor's
   temporary storage only when the destination expression is repeatable and
   the constructor has no default arguments. */
IrToken *__stdcall try_elide_copy_construction(CppSymbol *function, IrToken *object,
    IrToken *arguments)
{
    IrToken *call = arguments->value.pointer;
    IrToken *reference, *result;
    TypeShape *shape;
    CppSymbol *tag;
    if (!expression_can_be_repeated(object)) return 0;
    shape = function->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (shape->data.parameters->default_count) return 0;
    if (!find_constructor_result_reference(call, &reference)) return 0;
    cancel_temporary_cleanup(reference->value.integer);
    result = replace_temporary_reference(call, reference->value.integer, object);
    tag = cpp_context_stack->items[function->auxiliary.declaring_class];
    return new_typed_unary_expression(OP_DEREFERENCE, tag->type, result);
}

/* Original 0x39d30..0x39db9, stdcall 8. Constructors carry their
   object in the member designator; class-valued functions carry result storage
   at the argument ring's saved tail. */
int __stdcall find_constructor_result_reference(IrToken *call, IrToken **reference)
{
    IrToken *callee, *symbol_reference, *argument, *temporary;
    CppSymbol *function;
    if (!call || call->kind != 0x7a) return 0;
    callee = call->value.pointer;
    if (callee->kind == 0xa4) {
        symbol_reference = callee->value.pointer;
        if (symbol_reference->kind == 2) {
            function = symbol_reference->value.pointer;
            if ((function->kind & 31) == 7 && (function->declaration_flags & 0x1000)) {
                argument = callee->tail.right;
                goto found_argument;
            }
        }
    }
    if (!class_tag_for_result_type(drop_first_type_shape(strip_based_shape(callee->type))))
        return 0;
    argument = call->tail.right->linkage.list_head;
found_argument:
    temporary = argument->value.pointer;
    if (temporary->kind != 0xa7) return 0;
    *reference = temporary;
    return 1;
}

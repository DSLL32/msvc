/* Call binding, overload selection and function-style conversion expressions. */
#include <stdlib.h>
#include <stdarg.h>
#include "call-expressions.h"
#include "class-lookup.h"
#include "member-expressions.h"
#include "operators.h"
#include "return-statements.h"
#include "unary-expressions.h"
#include "lexer.h"
#include <recon.h>

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
    search->field0c = 1;
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
int __stdcall add_overload_candidates(OverloadSearch *search, CppSymbol *functions, unsigned origin)
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
        candidate->origin = origin;
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

/* Validation of C++ overloaded operator declarations. */
#include "declarations.h"
#include "parameters.h"
#include "cpp-context.h"
#include "operators.h"
#include <recon.h>

TypeShape *__stdcall skip_reference_shape(TypeShape *shape);

#define SPECIAL_ERROR(code_) \
    do { report_syntax_error(code_); symbol->declaration_flags |= 2; } while (0)
#define OPERATOR_ERROR(code_) \
    do { report_syntax_error(code_, token_spellings[symbol->cpp.parts.token]); \
         symbol->declaration_flags |= 2; } while (0)

/* Original code 0x50ed0..0x517bc; three switch tables 0x517bc..0x518b0. */
void __stdcall validate_special_function(CppSymbol *symbol, int definition)
{
    unsigned token;
    unsigned default_count = 0;
    int count = 0, classes = 0, has_ellipsis = 0, remaining;
    CType **types, **argument;
    CType *type;
    TypeShape *function, *result, *stripped;
    FunctionParameters *parameters;
    DeclarationParameterInfo *info;
    unsigned short flags;
    int member;
    if ((symbol->kind & 31) != 6 && (symbol->kind & 31) != 7) return;
    if (symbol->declaration_flags & 0x3000) return;
    token = symbol->cpp.parts.token;
    if (token == 1) return;
    if (definition && (symbol->declaration_flags & 0x400)) return;
    switch (token) {
    case OP_NEGATE: case OP_UNARY_PLUS:
    case OP_DEREFERENCE: case OP_PRE_DECREMENT: case OP_PRE_INCREMENT:
        return;
    case OP_ASSIGN: case OP_SUBSCRIPT: case OP_ARROW: case OP_CALL: case 0x9a:
        if (!(symbol->declaration_flags & 0x400)) {
            report_syntax_error(801, token_spellings[token], "");
            symbol->declaration_flags |= 2;
            return;
        }
        if (!(symbol->cpp.flags & 0x10000)) {
            report_syntax_error(801, token_spellings[token], " non-static");
            symbol->declaration_flags |= 2;
            return;
        }
        break;
    case 0x3f: case 0x59: case 0x5f: case 0x68: case 0x8c: case 0x9d: case 0xa0:
        OPERATOR_ERROR(800);
        return;
    }
    function = symbol->type->related;
    if (function && !(function->flags & 0x77)) function = function->related;
    parameters = function->data.parameters;
    info = parameters->declarations;
    types = info->types;
    if (types) {
        count = info->count;
        default_count = parameters->default_count;
    }
    if (count == 1 && !types[0]->related && (types[0]->flags & 0x80)) --count;
    remaining = count;
    argument = types;
    while (argument && remaining) {
        if (!*argument) has_ellipsis = 1;
        else if (is_class_name_type(*argument)) ++classes;
        ++argument;
        --remaining;
    }
    /* Reacquire the shared descriptor after class-type classification. */
    function = symbol->type->related;
    if (function && !(function->flags & 0x77)) function = function->related;
    info = function->data.parameters->declarations;
    types = info->types;
    argument = types;
    member = (symbol->declaration_flags & 0x400) && (symbol->cpp.flags & 0x10000);
    if (member) { ++classes; ++count; }
    if (!count) {
        if (!(symbol->declaration_flags & 0x400)) OPERATOR_ERROR(809);
        else if (!(symbol->cpp.flags & 0x10000)) OPERATOR_ERROR(802);
        return;
    }
    if (token != 0x97 && token != 0x98 && !classes) OPERATOR_ERROR(803);
    if (has_ellipsis && token != 0x97 && token != OP_CALL) OPERATOR_ERROR(829);
    switch (token) {
    case 2:
        return;
    case OP_BIT_AND: case OP_SUBTRACT: case OP_MULTIPLY: case OP_ADD:
        if (count == 1) {
            if (token == OP_SUBTRACT) symbol->cpp.parts.token = OP_NEGATE;
            else if (token == OP_MULTIPLY) symbol->cpp.parts.token = OP_DEREFERENCE;
            else if (token == OP_ADD) symbol->cpp.parts.token = OP_UNARY_PLUS;
            break;
        }
        /* The binary forms share the ordinary two-operand checks. */
    case OP_LOGICAL_AND: case OP_AND_ASSIGN: case 0x40:
    case OP_DIVIDE: case OP_DIVIDE_ASSIGN: case OP_EQUAL:
    case OP_GREATER: case OP_GREATER_EQUAL: case OP_SUBSCRIPT:
    case OP_SHIFT_LEFT_ASSIGN: case OP_SHIFT_LEFT: case OP_LESS: case OP_LESS_EQUAL:
    case OP_SUBTRACT_ASSIGN: case OP_REMAINDER: case OP_REMAINDER_ASSIGN:
    case OP_MULTIPLY_ASSIGN: case OP_NOT_EQUAL: case OP_BIT_OR: case OP_OR_ASSIGN:
    case OP_LOGICAL_OR: case OP_ADD_ASSIGN: case OP_SHIFT_RIGHT_ASSIGN:
    case OP_SHIFT_RIGHT: case OP_BIT_XOR: case OP_XOR_ASSIGN: case OP_MEMBER_POINTER:
        if (count > 2) OPERATOR_ERROR(804);
        else if (count != 2) OPERATOR_ERROR(805);
        break;
    case OP_ASSIGN:
        if (count > 2 && !(symbol->flags & 0x80)) OPERATOR_ERROR(804);
        else if (count < 2) OPERATOR_ERROR(805);
        break;
    case OP_POST_DECREMENT: case OP_POST_INCREMENT:
        if (count > 2) OPERATOR_ERROR(806);
        else if (count == 2) {
            if (!member) ++argument;
            type = *argument;
            flags = type->flags;
            if (!(flags & 0x40) || !(flags & 0x20) ||
                type->related || (flags & 0x4000)) OPERATOR_ERROR(807);
        } else symbol->cpp.parts.token = token == OP_POST_DECREMENT ?
            OP_PRE_DECREMENT : OP_PRE_INCREMENT;
        break;
    case OP_ARROW:
        type = symbol->type;
        flags = type->flags;
        if ((flags & 0x408) == 0x408 || (flags & 0x402) == 0x402 ||
            (flags & 0x404) == 0x404) {
            result = type->related->related;
            stripped = skip_reference_shape(result);
            if (!stripped) {
                if (cpp_context_stack->items[type->field08] ==
                    cpp_context_stack->items[(short)symbol->auxiliary.value]) SPECIAL_ERROR(819);
            } else if ((result->flags & 0x77) != 1 || result->related) SPECIAL_ERROR(818);
        } else SPECIAL_ERROR(818);
        /* Arrow and conversion operators also obey the unary limit. */
    case OP_NOT: case OP_COMPLEMENT: case 0x9a:
        if (count > 1) {
            if (token == 0x9a) {
                report_syntax_error(835, format_symbol_name(symbol));
                symbol->declaration_flags |= 2;
            } else OPERATOR_ERROR(808);
        }
        break;
    case 0x97: /* operator new */
        type = *argument;
        if ((type->flags & 0x800) && !type->related) {
            type = symbol->type;
            result = type->related;
            if (!(type->flags & 0x80) || !result || (result->flags & 0x77) != 4 ||
                !result->related || (result->related->flags & 0x40) || result->related->related ||
                !(result->related->attributes & 12) || (result->related->attributes & 7) != 4)
                SPECIAL_ERROR(823);
            if (count < 2) {
                SPECIAL_ERROR(820);
                break;
            }
            ++argument;
            if ((int)(count - default_count) >= 2) ++default_count;
            if (symbol->declaration_flags & 0x400) {
                report_syntax_error(832, token_spellings[51], token_spellings[0x97]);
                symbol->declaration_flags |= 2;
            }
        } else {
            type = symbol->type;
            result = type->related;
            if (!(type->flags & 0x80) || !result || (result->flags & 0x77) != 4 ||
                !result->related || (result->related->flags & 0x40) || result->related->related ||
                (result->related->attributes & 12)) SPECIAL_ERROR(824);
        }
        if ((*argument)->flags != 0x4064 || (*argument)->related) {
            if (argument == types) SPECIAL_ERROR(821);
            else SPECIAL_ERROR(822);
        }
        break;
    case 0x98: /* operator delete */
        type = symbol->type;
        result = type->related;
        if (!(type->flags & 0x80) || !result || result->related || (result->flags & 0x77) != 4)
            SPECIAL_ERROR(817);
        type = *argument;
        if ((type->flags & 0x800) && !type->related) {
            if (count > 3) SPECIAL_ERROR(811);
            else if (count == 1) {
                SPECIAL_ERROR(812);
                break;
            }
            ++argument;
            type = *argument;
            result = type->related;
            if (!(type->flags & 0x80) || (type->attributes & 0x3000) || !result ||
                (result->flags & 0x40) || result->related ||
                !(result->attributes & 12) || (result->attributes & 7) != 4) SPECIAL_ERROR(814);
            if (symbol->declaration_flags & 0x400) {
                report_syntax_error(832, token_spellings[51], token_spellings[0x98]);
                symbol->declaration_flags |= 2;
            }
            if (count < 3) break;
        } else {
            if (count > 2) SPECIAL_ERROR(813);
            type = *argument;
            result = type->related;
            if (!(type->flags & 0x80) || (type->attributes & 0x3000) || !result ||
                (result->flags & 0x40) || result->related || (result->attributes & 12))
                SPECIAL_ERROR(815);
            if (count < 2) break;
        }
        if (!(symbol->declaration_flags & 0x400)) SPECIAL_ERROR(816);
        if (argument[1]->flags != 0x4064 || argument[1]->related) SPECIAL_ERROR(810);
        break;
    }
    token = symbol->cpp.parts.token;
    if (token != OP_CALL) {
        if (token == 0x97) {
            if ((int)(count - default_count) < 1) SPECIAL_ERROR(830);
        } else if (default_count) OPERATOR_ERROR(831);
    }
}
#undef SPECIAL_ERROR
#undef OPERATOR_ERROR

#include <stdio.h>
#include "frontend-init.h"
#include "parser-actions.h"
#include <recon.h>

extern "C" {
void *__cdecl make_integer_constant(int);
void __cdecl write_ir_record(int, const void *);
void __cdecl prepare_expression_input(const char *);
int __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl open_expression_output(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3]; /* c0360 */
    if (!frontend_data.options.pch_create) {
        buffer->Open(frontend_data.options.il_prefix, "ex", "w+b");
    } else if (!(frontend_data.options.pch_create_file || frontend_data.options.pch_file) ||
               frontend_data.options.pch_use) {
        buffer->Open(frontend_data.options.il_prefix, "ex", "w+b");
    } else {
        prepare_expression_input(frontend_data.options.pch_create_file);
        buffer->Open(frontend_data.options.pch_file, "", "wb");
        frontend_data.pch_expression_file = 1;
    }
    if (frontend_data.options.pch_use)
        return;
    write_ir_record(0x5b, make_integer_constant(0x484));
    buffer->Flush();
    fflush(buffer->file);
    frontend_bss.expression_seek_result = fseek(buffer->file, 0x483, SEEK_SET);
    if (frontend_bss.expression_seek_result)
        report_response_error(86, 339, NULL);
    buffer->ok &= buffer->cursor + 2 > buffer->data + buffer->capacity ?
        buffer->Grow(2) : 1;
    if (buffer->ok) {
        *buffer->cursor = 0;
        ++buffer->cursor;
    }
    buffer->Flush();
}

extern "C" void __cdecl prepare_expression_input(const char *name) { __asm { int 3 } }

/* The encountered CF770 flag and CF390 record-code columns. Other kinds
 * remain semantic frontiers, not implicit zero rows in a larger table. */
struct ExpressionEmissionKind { BYTE kind, flags; unsigned record; };
static const ExpressionEmissionKind expression_emission_kinds[] = {
    { 6, 0x2c, 38 }, { 7, 0xa5, 51 },
    { 52, 0x65, 50 }, /* assignment */
    { 61, 0xe5, 48 }, { 81, 0x56, 2 }, /* addition */
    { 83, 0x65, 39 }, /* member address */
    { 93, 0xa5, 104 }, { 100, 0xa5, 44 }, { 101, 0x2d, 51 },
    { 108, 0xa5, 70 }, { 112, 0xa5, 62 }, { 119, 0xa5, 64 },
    { 132, 0xac, 60 }, { 153, 0x25, 153 }, { 157, 0xa5, 157 },
    { 166, 0x65, 90 }, /* implicit-this dereference: CF816 / CF628 */
    { 199, 0xa5, 92 }, { 200, 0xa5, 93 }, { 201, 0xa5, 94 }
};
/* This is a partial view, not the case-range allocation size. */
struct CompilerCaseRange {
    CompilerCaseRange *next;
    BYTE unmodeled_04_07[4];
    __int64 value;
};
extern "C" {
CompilerExpression *__cdecl prepare_ir_expression(CompilerExpression *); /*2bd4b*/
int __cdecl emit_special_ir_expression(CompilerExpression *); /*44153*/
unsigned __cdecl expression_control_kind(CompilerExpression *); /*21408*/
CompilerExpression *__cdecl make_immediate_expression(unsigned, CompilerType *, CompilerExpression *); /*10187*/
CompilerExpression *__cdecl make_sized_integer_constant(__int64, int); /*5df6*/
CompilerExpression *__cdecl resolve_constant_reference(CompilerExpression *); /*7a5d*/
}
/* Inline semantic lookup preserves the original function graph. Unknown
 * rows stop before reading either original semantic column in both builds. */
#define EMISSION_KIND(node, info) do { \
    unsigned emission_row; \
    for (emission_row = 0; emission_row < sizeof(expression_emission_kinds) / sizeof(expression_emission_kinds[0]); ++emission_row) \
        if (expression_emission_kinds[emission_row].kind == (node)->kind) break; \
    if (emission_row == sizeof(expression_emission_kinds) / sizeof(expression_emission_kinds[0])) { __asm { int 3 } } \
    (info) = &expression_emission_kinds[emission_row]; \
} while (0)
#define EMIT_EXPRESSION_RECORD(node) do { \
    const ExpressionEmissionKind *record_info; \
    EMISSION_KIND(node, record_info); \
    write_ir_record(record_info->record, (node)); \
} while (0)

extern "C" void __cdecl emit_ir_expression(CompilerExpression *expression)
{
    const ExpressionEmissionKind *info;
    CompilerExpression *item, *constant, *classification;
    CompilerRuntimeFunction *function;
    CompilerParameterSet *parameters;
    CompilerSymbolLink *link;
    CompilerParameterSymbol *parameter;
    CompilerSymbol *marked;
    CompilerType *type;
    CompilerCaseRange *range;
    unsigned attributes, kind, control;
    WORD control_flags, type_flags;
    int saved_mark = 0, bytes, classification_bytes;
    if (frontend_bss.error_count) return;
restart:
    if (!expression) return;
    EMISSION_KIND(expression, info);
    if (info->flags & 0x80) {
        switch (expression->kind) {
        case 7:
            item = prepare_ir_expression(expression);
            EMIT_EXPRESSION_RECORD(item);
            return;
        case 61:
            expression = resolve_constant_reference(expression);
            goto ordinary;
        case 93:
            if (!expression->value.operands.second)
                expression->value.operands.second = make_immediate_expression(
                    23, frontend_data.builtin_types.type_4063_first, 0);
            goto ordinary;
        case 100:
            type = expression->type;
            if ((type->shape && (type->shape->flags & 0x30)) ||
                (!type->shape && (type->flags & 0x2fdf) == 0x80)) {
                expression = expression->value.operands.first;
                goto restart;
            }
            goto ordinary;
        case 119:
            if (emit_special_ir_expression(expression)) return;
            /* Otherwise use the same list sequence as kind112. */
        case 112: case 157:
            emit_ir_expression(expression->value.operands.first);
            EMIT_EXPRESSION_RECORD(expression);
            item = expression->value.operands.second;
            while (item) {
                emit_ir_expression(item->value.operands.first);
                write_ir_record(0x55, item);
                item = item->value.operands.second;
            }
            write_ir_record(0x4c, NULL);
            return;
        case 132:
            emit_ir_expression(expression->value.operands.first);
            EMIT_EXPRESSION_RECORD(expression);
            range = (CompilerCaseRange *)expression->value.operands.second;
            while (range) {
                type_flags = expression->type->flags;
                attributes = type_flags & 0x340;
                kind = type_flags & 15;
                bytes = (!attributes && (type_flags & ~15U)) ?
                    kind : compiler_primitive_type_bytes[kind];
                classification_bytes = attributes || !(type_flags & ~15U) ?
                    compiler_primitive_type_bytes[kind] : kind;
                constant = make_sized_integer_constant(range->value, bytes);
                classification = make_sized_integer_constant(range->value, classification_bytes);
                EMISSION_KIND(classification, info);
                write_ir_record(info->record, constant);
                write_ir_record(0x3d, range);
                range = range->next;
            }
            write_ir_record(0x4c, NULL);
            return;
        case 108:
            item = expression->value.operands.first;
            if (item->kind == 0x99) item = item->value.operands.first;
            function = (CompilerRuntimeFunction *)item->value.symbol;
            attributes = function->emission_state & 3;
            emit_ir_expression(expression->value.operands.first);
            function->emission_state = (function->emission_state & ~3U) | attributes;
            EMIT_EXPRESSION_RECORD(expression);
            marked = NULL;
            parameters = frontend_bss.active_parameters;
            if ((function->type_flags & 4) && (function->builtin_flags & 1)) {
                link = parameters ? parameters->head : NULL;
                if (link) {
                    marked = (CompilerSymbol *)((BYTE *)link - 8);
                    saved_mark = (marked->definition_flags >> 3) & 1;
                    marked->definition_flags |= 8;
                }
            }
            /* The producer may publish a new active list during the call. */
            parameters = frontend_bss.active_parameters;
            link = parameters ? parameters->tail : NULL;
            while (link) {
                parameter = (CompilerParameterSymbol *)((BYTE *)link - 8);
                if (parameter->type && !(parameter->definition_flags & 8) &&
                    (parameter->kind == 11 || parameter->kind == 12 || parameter->kind == 22))
                    write_ir_record(0x2d, symbol_expression(parameter, 0));
                link = parameter->previous_parameter;
            }
            if (marked) {
                if (saved_mark) marked->definition_flags |= 8;
                else marked->definition_flags &= ~8U;
            }
            write_ir_record(0x4c, NULL);
            return;
        case 199:
            function = (CompilerRuntimeFunction *)frontend_bss.current_function;
            function->queue_flags |= 8;
            function = (CompilerRuntimeFunction *)frontend_bss.current_function;
            function->queue_flags = (function->queue_flags & ~2U) |
                ((frontend_data.options.exception_handling & 1) << 1);
            item = expression->value.operands.second;
            emit_ir_expression(item->value.operands.first);
            item = expression->value.operands.second;
            emit_ir_expression(item->value.operands.second);
            item = expression->value.operands.first;
            control = expression_control_kind(item->value.operands.first);
            control_flags = item->value.control.flags;
            goto control_kind;
        case 200: case 201:
            control = expression_control_kind(expression->value.operands.first);
            control_flags = expression->value.control.flags;
control_kind:
            switch (control) {
            case 1: case 2: case 3: case 7: control_flags |= 1; break;
            case 4: control_flags |= 2; break;
            case 5: control_flags |= 9; break;
            case 6: control_flags |= 3; break;
            case 8: control_flags |= 4; break;
            }
            if (!frontend_data.options.exception_handling) control_flags |= 0x10;
            expression->value.control.flags = control_flags;
            goto record;
        }
    }
ordinary:
    switch (expression->flags & 7) {
    case 0:
        if (expression->flags & 0x40) { compiler_warning(0x7c); return; }
        break;
    case 1:
        emit_ir_expression(expression->value.operands.first);
        break;
    case 2:
        emit_ir_expression(expression->value.operands.first);
        emit_ir_expression(expression->value.operands.second);
        break;
    }
record:
    EMIT_EXPRESSION_RECORD(expression);
}
#undef EMIT_EXPRESSION_RECORD
#undef EMISSION_KIND

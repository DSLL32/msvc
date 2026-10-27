#include "code-records.h"
#include "output-callbacks.h"
#include "code-input.h"
#include "small-nodes.h"
#include "flow.h"
#include "functions.h"
#include "coff-output.h"
#include <recon.h>

unsigned record_source_line;
unsigned short record_line_delta;
extern int c2_internal_listing, c2_basic_mode, c2_exception_mode;
extern int function_input_flags;
extern char *c2_output_name;

void __fastcall link_code_nodes(C2Record *first, C2Record *last)
{
    C2Record *old = first->next;
    first->next = last;
    last->previous = first;
    if (old) {
        while (last->next)
            last = last->next;
        last->next = old;
        old->previous = last;
    }
}

C2DebugLink *__fastcall prepare_function_records(C2Function *function)
{
    C2FunctionSymbol *owner;
    C2Record *record;
    C2Expression *expression;
    C2Symbol *symbol;
    C2InputSymbol *declaration;
    C2DebugLink *head, *link, *debug;
    C2Section *section;
    int side;

    owner = function ? function->symbol : NULL;
    record = function ? function->flow->last->records : NULL;
    for (; record; record = record->next) {
        if (record->line) {
            record_line_delta = record->line;
            record_source_line = function->source_line_base + record->line;
        }
        switch (record->kind) {
        case 11: case 12: case 13: case 14: case 15:
        case 16: case 17: case 18: case 19: case 20:
            for (side = 2; side > 0; --side) {
                expression = side == 2 ? record->results : record->value.arguments;
                for (; expression; expression = expression->next) {
                    switch (expression->kind) {
                    case 2: case 3:
                        symbol = expression->value.reference->reference->symbol;
                        break;
                    case 4:
                        symbol = expression->value.symbol;
                        if (record->opcode != 187 && symbol->kind == 3 &&
                            expression->opcode == 298)
                            symbol = NULL;
                        break;
                    case 5: case 6:
                        symbol = expression->tail.indirect ?
                            expression->tail.indirect->reference->symbol : NULL;
                        break;
                    default:
                        symbol = NULL;
                        break;
                    }
                    if (symbol) {
                        queue_symbol_debug(symbol);
                        if (symbol->kind == 4 &&
                            (((C2FunctionSymbol *)symbol)->attributes & 0x4000) &&
                            !(((C2FunctionSymbol *)symbol)->attributes & 0x1000))
                            ((C2FunctionSymbol *)symbol)->additional_flags |= 0x20;
                    }
                }
            }
            break;
        case 22:
            queue_symbol_debug(record->owner.reference->symbol);
            break;
        case 25:
            declaration = record->value.declaration;
            if (c2_codeview_types) {
                if (declaration->symbol.source_kind != 0) {
                    if (declaration->symbol.source_kind != 2 &&
                        !declaration->detail.variable.flags)
                        break;
                    debug = c2_pool_allocate(1, sizeof(*debug));
                    debug->symbol = &declaration->symbol;
                    debug->next = function->debug_scope->records;
                    function->debug_scope->records = debug;
                }
                queue_symbol_debug(&declaration->symbol);
            } else if (declaration->detail.variable.flags)
                queue_symbol_debug(&declaration->symbol);
            break;
        }
    }
    head = prepare_global_records();
    for (link = head; link; link = link->next) {
        if (owner && (c2_internal_listing || (owner->symbol.flags & 0x20))) {
            symbol = link->symbol;
            if (has_local_symbol(symbol->serial)) {
                section = select_symbol_section(owner, symbol);
                symbol->association.section_serial = section->symbol.serial;
            }
        }
        reserve_symbol_storage(link->symbol);
    }
    return head;
}

C2InitRecord *global_initializers;
int global_records_processing;
extern int c2_zero_alignment;

C2DebugLink *prepare_global_records(void)
{
    C2InitRecord *record, *value;
    C2Symbol *symbol;
    C2DebugLink *head = NULL, **tail = &head, *link;
    unsigned attributes, flags;

    global_records_processing = 1;
    for (record = global_initializers; record; record = record->list_next) {
        if (record->emitted)
            continue;
        symbol = record->symbol;
        if (!symbol) {
            symbol = lookup_global_symbol(record->symbol_serial);
            if (!symbol)
                continue;
            record->symbol = symbol;
        }
        if ((symbol->kind == 4 || symbol->kind == 1) &&
            (((C2InputSymbol *)symbol)->attributes & 0x1000))
            symbol = resolve_alias_symbol(symbol);
        if (symbol->output_flags & 3)
            continue;
        if (symbol->output_flags & 4) {
            record_symbol_references(symbol);
            continue;
        }
        if ((symbol->flags & 0x60) == 0x20)
            continue;
        symbol->output_flags |= 4;
        if (symbol_requires_references(symbol)) {
            symbol->output_flags &= ~4;
            record_symbol_references(symbol);
        } else
            symbol->output_flags &= ~4;
    }
    for (record = global_initializers; record; record = record->list_next) {
        if (record->emitted)
            continue;
        symbol = record->symbol;
        if (!symbol) {
            symbol = lookup_global_symbol(record->symbol_serial);
            if (!symbol)
                continue;
        }
        if ((symbol->kind == 4 || symbol->kind == 1) &&
            (((C2InputSymbol *)symbol)->attributes & 0x1000))
            symbol = resolve_alias_symbol(symbol);
        if (!(symbol->output_flags & 4) || (symbol->output_flags & 3))
            continue;
        attributes = ((C2InputSymbol *)symbol)->attributes;
        if ((attributes & 0x780) != 0x80 && (attributes & 0x780) != 0x200)
            goto append;
        if ((attributes & 0x70) == 0x40)
            goto append;
        flags = symbol->flags;
        if ((flags & 0x1200) || c2_zero_alignment)
            goto append;
        for (value = record->next; value; value = value->next) {
            switch (value->kind) {
            case 4: case 8:
                break;
            case 1: case 16:
                if (initializer_is_static(value))
                    break;
                goto append;
            default:
                goto append;
            }
        }
        symbol->flags = flags & ~0x480U;
        symbol->association.section_serial = 0;
        update_symbol_storage(symbol);
        reserve_symbol_storage(symbol);
        record->emitted = 1;
        continue;

append:
        link = new_symbol_link();
        link->next = NULL;
        link->symbol = symbol;
        *tail = link;
        tail = &link->next;
    }
    /* The original shares this single RET0 body with startup's _setargv.
       This reviewed use supplies the link head in ECX, with no stack arguments
       or result. Other unresolved callback views stay stores-only. */
    ((void (__fastcall *)(C2DebugLink *)) c2_set_argv)(head);
    global_records_processing = 0;
    return head;
}

C2DebugLink *pending_debug_symbols;

void __fastcall process_records(C2DebugLink *records)
{
    if (!c2_zero_alignment)
        align_pending_debug_symbols(pending_debug_symbols);
    write_pending_debug_symbols();
    write_assembly_symbols();
    write_record_initializers(records);
    write_assembly_records(records);
}

void __fastcall align_pending_debug_symbols(C2DebugLink *symbols)
{
    C2DebugLink *next;
    C2Symbol *symbol;
    long value, half;

    if (!symbols || !is_known_symbol(symbols->symbol->serial))
        return;
    for (; symbols; symbols = next) {
        next = symbols->next;
        if (!next || symbols->symbol->value < 4096)
            continue;
        symbol = next->symbol;
        value = symbol->value;
        half = (value % 8192) / 2;
        if (half / 32 == 0 && value >= 4096)
            symbol->value = value - half + 32;
    }
}

void write_pending_debug_symbols(void)
{
    while (pending_debug_symbols) {
        output_slot_497cec.target(pending_debug_symbols->symbol);
        pending_debug_symbols = pending_debug_symbols->next;
    }
}

void __fastcall write_record_initializers(C2DebugLink *records)
{
    if (c2_output_name) {
        for (; records; records = records->next)
            emit_initializer(((C2InputSymbol *)records->symbol)->initializer);
    }
}

C2Expression *__fastcall new_control_expression(unsigned opcode)
{
    C2Expression *record = (C2Expression *)allocate_small_node(21);
    record->opcode = opcode;
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_function_entry(C2Function *function)
{
    C2Expression *record = (C2Expression *)allocate_small_node(22);
    record->value.function = function;
    record->opcode = 0x17a;
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_typed_register_expression(unsigned opcode,
                               unsigned short type, C2FunctionRegister *reg)
{
    C2Expression *tree = (C2Expression *)allocate_small_node(2);
    tree->opcode = opcode;
    tree->type = type;
    tree->value.reg = reg;
    return tree;
}

C2Expression *__fastcall new_register_expression(unsigned opcode, C2FunctionRegister *reg)
{
    C2Expression *tree = (C2Expression *)allocate_small_node(3);
    tree->opcode = opcode;
    tree->value.reg = reg;
    tree->type = input_value_type;
    return tree;
}

C2Expression *__fastcall new_integer_expression(unsigned short type, unsigned __int64 value)
{
    C2Expression *tree = (C2Expression *)allocate_small_node(7);
    union { unsigned __int64 value; C2IntegerWords words; } integer;
    tree->opcode = 0x11d;
    tree->type = type;
    integer.value = normalize_integer_value(type, value);
    tree->integer.low = integer.words.low;
    tree->integer.high = integer.words.high;
    return tree;
}

unsigned __int64 __fastcall normalize_integer_value(unsigned short type, unsigned __int64 value)
{
    /* Compiler helpers receive the count in CL, including its byte truncation.
     * Spell out their >=64 result so native C never performs an undefined shift. */
    unsigned char shift = (unsigned char)((8 - (type & 0xfff)) * 8);
    if (shift >= 64) return 0;
    if ((type & 0xf000) == 0x2000)
        return value & (~(unsigned __int64)0 >> shift);
    return (unsigned __int64)((__int64)(value << shift) >> shift);
}

/* Construct a two-input operation with a separate temporary result operand. */
C2Expression *__fastcall new_folded_binary_expression(unsigned opcode, unsigned short type,
    C2Expression *left, C2Expression *right, C2InputLink link, C2Record *prior)
{
    C2Expression *result = new_temporary_operand(type), *record, *value;
    record = (C2Expression *)allocate_small_node(11);
    record->type = type;
    record->opcode = opcode;
    record->flags |= 1;
    value = result;
    if ((1U << (value->kind & 31)) & 0x7fe) {
        if (value->location.expression.properties & 0x10)
            value = copy_input_expression(value);
        value->location.expression.properties |= 0x10;
    }
    record->tail.right = value;
    if ((1U << (left->kind & 31)) & 0x7fe) {
        if (left->location.expression.properties & 0x10)
            left = copy_input_expression(left);
        left->location.expression.properties |= 0x10;
    }
    record->left = left;
    if ((1U << (right->kind & 31)) & 0x7fe) {
        if (right->location.expression.properties & 0x10)
            right = copy_input_expression(right);
        right->location.expression.properties |= 0x10;
    }
    left->next = right;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    result->value.reg->value = record;
    return record;
}

C2Expression *__fastcall new_temporary_operand(unsigned short type)
{
    C2FunctionRegister *reg = new_temporary_register(type);
    return new_temporary_register_expression(0x11f, type, reg, reg);
}

C2Expression *__fastcall new_temporary_register_expression(unsigned opcode,
    unsigned short type, C2FunctionRegister *reg, C2FunctionRegister *original)
{
    C2Expression *expression = (C2Expression *)allocate_small_node(1);
    expression->opcode = opcode;
    expression->type = type;
    expression->value.reg = reg;
    expression->original_register = original;
    return expression;
}

/* Aggregate operands carry separate component uses in both input/result lists. */
void __fastcall retain_aggregate_operands(C2Expression *expression)
{
    C2Expression *value;
    for (value = expression->left; value; value = value->next) {
        if (((1U << (value->kind & 31)) & 0x60) &&
            !(value->location.expression.comparison & 0x20)) {
            if (value->aggregate_value) {
                value->aggregate_value->location.expression.properties &= ~0x10;
                copy_input_operand(expression, value->aggregate_value);
            }
            if (value->aggregate_second) {
                value->aggregate_second->location.expression.properties &= ~0x10;
                copy_input_operand(expression, value->aggregate_second);
            }
        }
    }
    for (value = expression->tail.right; value; value = value->next) {
        if (((1U << (value->kind & 31)) & 0x60) &&
            !(value->location.expression.comparison & 0x20)) {
            if (value->aggregate_value) {
                value->aggregate_value->location.expression.properties &= ~0x10;
                copy_input_operand(expression, value->aggregate_value);
            }
            if (value->aggregate_second) {
                value->aggregate_second->location.expression.properties &= ~0x10;
                copy_input_operand(expression, value->aggregate_second);
            }
        }
    }
}

C2Expression *__fastcall copy_input_expression(C2Expression *expression)
{
    unsigned char kind = expression->kind;
    C2Expression *copy = (C2Expression *)allocate_small_node(kind);
    C2Expression *source, *tail;
    int retained_components = 0;
    /* allocate_small_node guards unknown size rows before this shared lookup. */
    memcpy(copy, expression, small_node_sizes.values[kind]);
    copy->next = NULL;
    switch (kind) {
    case 1: case 2:
        copy->location.expression.properties &= ~0x30;
        copy->location.expression.comparison &= ~0x10;
        return copy;
    case 5: case 6:
        if (expression->aggregate_value) {
            copy->aggregate_value = copy_input_expression(expression->aggregate_value);
            copy->aggregate_value->location.expression.properties |= 0x30;
        }
        if (expression->aggregate_second) {
            copy->aggregate_second = copy_input_expression(expression->aggregate_second);
            copy->aggregate_second->location.expression.properties |= 0x30;
        }
        /* Fall through. */
    case 3: case 4: case 7: case 8: case 9: case 10:
        copy->location.expression.properties &= ~0x10;
        return copy;
    case 20:
        copy->detail.data = copy_input_expression_list(expression->detail.data);
        /* Fall through to ordinary operand/result list copying. */
    case 11: case 12: case 13: case 14: case 15: case 16:
    case 17: case 18: case 19:
        if (kind == 19 && expression->opcode == 0x16d)
            copy_input_assembly_context(expression, copy);
        source = expression->left;
        if (source) {
            tail = copy_input_expression(source);
            copy->left = tail;
            if ((1U << (tail->kind & 31)) & 0x7fe)
                tail->location.expression.properties |= 0x10;
            for (source = source->next; source; source = source->next) {
                if (((1U << (source->kind & 31)) & 0x7fe) &&
                    (source->location.expression.properties & 0x20)) {
                    retained_components = 1;
                    continue;
                }
                tail->next = copy_input_expression(source);
                tail = tail->next;
                if ((1U << (tail->kind & 31)) & 0x7fe)
                    tail->location.expression.properties |= 0x10;
            }
        }
        source = expression->tail.right;
        if (source) {
            tail = copy_input_expression(source);
            copy->tail.right = tail;
            if ((1U << (tail->kind & 31)) & 0x7fe)
                tail->location.expression.properties |= 0x10;
            for (source = source->next; source; source = source->next) {
                tail->next = copy_input_expression(source);
                tail = tail->next;
                if ((1U << (tail->kind & 31)) & 0x7fe)
                    tail->location.expression.properties |= 0x10;
            }
        }
        if (retained_components) retain_aggregate_operands(copy);
        /* Fall through. */
    case 21: case 22: case 23: case 24: case 25: case 26:
        copy->previous = NULL;
        break;
    }
    return copy;
}

C2Expression *__fastcall new_binary_expression(unsigned opcode, unsigned short type,
    C2Expression *right, C2Expression *left, C2InputLink link, C2Record *prior)
{
    C2Expression *record = (C2Expression *)allocate_small_node(11);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if ((1U << (right->kind & 31)) & 0x7fe) {
        if (right->location.expression.properties & 0x10)
            right = copy_input_expression(right);
        right->location.expression.properties |= 0x10;
    }
    record->tail.right = right;
    if ((1U << (left->kind & 31)) & 0x7fe) {
        if (left->location.expression.properties & 0x10)
            left = copy_input_expression(left);
        left->location.expression.properties |= 0x10;
    }
    record->left = left;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_block_copy_expression(unsigned opcode, unsigned short type,
    C2Expression *destination, C2Expression *source, C2Expression *bytes,
    C2InputLink link, C2Record *prior)
{
    C2Expression *record = (C2Expression *)allocate_small_node(12);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if ((1U << (destination->kind & 31)) & 0x7fe) {
        if (destination->location.expression.properties & 0x10)
            destination = copy_input_expression(destination);
        destination->location.expression.properties |= 0x10;
    }
    record->tail.right = destination;
    if ((1U << (source->kind & 31)) & 0x7fe) {
        if (source->location.expression.properties & 0x10)
            source = copy_input_expression(source);
        source->location.expression.properties |= 0x10;
    }
    record->left = source;
    /* This constructor discards the unattached size expression; the record's
     * aggregate type carries the copy width into later passes. */
    if (!(bytes->location.expression.properties & 0x10)) release_small_node(bytes);
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}

C2Expression *__fastcall new_aggregate_expression(unsigned opcode, unsigned short type,
    C2FunctionRegister *address, long displacement, C2Expression *source,
    C2Expression *destination, unsigned attributes)
{
    C2Expression *record = (C2Expression *)allocate_small_node(6);
    record->opcode = opcode;
    record->type = type;
    record->tail.aggregate_register = address;
    record->detail.aggregate_displacement = displacement;
    record->immediate = attributes;
    if (source) {
        if ((1U << (source->kind & 31)) & 0x7fe) {
            if (source->location.expression.properties & 0x10)
                source = copy_input_expression(source);
            source->location.expression.properties |= 0x10;
        }
        record->aggregate_value = source;
        source->location.expression.properties |= 0x20;
    }
    if (destination) {
        if ((1U << (destination->kind & 31)) & 0x7fe) {
            if (destination->location.expression.properties & 0x10)
                destination = copy_input_expression(destination);
            destination->location.expression.properties |= 0x10;
        }
        record->aggregate_second = destination;
        destination->location.expression.properties |= 0x20;
    }
    return record;
}

C2Expression *__fastcall new_comparison_expression(unsigned opcode, unsigned short type,
    C2Expression *left, C2Expression *right, C2InputLink link, C2Record *prior)
{
    C2Expression *temporary = new_temporary_operand(0x8000), *output = temporary;
    C2Expression *record = (C2Expression *)allocate_small_node(11);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if ((1U << (output->kind & 31)) & 0x7fe) {
        if (output->location.expression.properties & 0x10)
            output = copy_input_expression(output);
        output->location.expression.properties |= 0x10;
    }
    record->tail.right = output;
    if ((1U << (left->kind & 31)) & 0x7fe) {
        if (left->location.expression.properties & 0x10)
            left = copy_input_expression(left);
        left->location.expression.properties |= 0x10;
    }
    record->left = left;
    if ((1U << (right->kind & 31)) & 0x7fe) {
        if (right->location.expression.properties & 0x10)
            right = copy_input_expression(right);
        right->location.expression.properties |= 0x10;
    }
    left->next = right;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    temporary->value.reg->value = record;
    return record;
}

C2Expression *__fastcall new_select_expression(unsigned opcode, unsigned short type,
    C2Expression *condition, C2Expression *left, C2Expression *right,
    unsigned char comparison, C2InputLink link, C2Record *prior)
{
    C2Expression *temporary, *output, *record;
    temporary = new_temporary_operand(type);
    output = temporary;
    record = (C2Expression *)allocate_small_node(15);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if ((1U << (output->kind & 31)) & 0x7fe) {
        if (output->location.expression.properties & 0x10)
            output = copy_input_expression(output);
        output->location.expression.properties |= 0x10;
    }
    record->tail.right = output;
    if ((1U << (condition->kind & 31)) & 0x7fe) {
        if (condition->location.expression.properties & 0x10)
            condition = copy_input_expression(condition);
        condition->location.expression.properties |= 0x10;
    }
    record->left = condition;
    if ((1U << (left->kind & 31)) & 0x7fe) {
        if (left->location.expression.properties & 0x10)
            left = copy_input_expression(left);
        left->location.expression.properties |= 0x10;
    }
    condition->next = left;
    if ((1U << (right->kind & 31)) & 0x7fe) {
        if (right->location.expression.properties & 0x10)
            right = copy_input_expression(right);
        right->location.expression.properties |= 0x10;
    }
    left->next = right;
    record->detail.condition = comparison;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    temporary->value.reg->value = record;
    return record;
}

C2Expression *__fastcall new_transfer_expression(unsigned opcode, unsigned short type,
    C2Expression *right, C2Expression *left, C2InputLink link, C2Record *prior)
{
    C2Expression *record = (C2Expression *)allocate_small_node(14);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    if (right) {
        if ((1U << (right->kind & 31)) & 0x7fe) {
            if (right->location.expression.properties & 0x10)
                right = copy_input_expression(right);
            right->location.expression.properties |= 0x10;
        }
        record->tail.right = right;
    }
    if (left) {
        if ((1U << (left->kind & 31)) & 0x7fe) {
            if (left->location.expression.properties & 0x10)
                left = copy_input_expression(left);
            left->location.expression.properties |= 0x10;
        }
        record->left = left;
    }
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}

int __fastcall input_has_side_effects(C2Expression *tree)
{
    C2Expression *child;
    unsigned intrinsic;
    switch (tree->kind) {
    case 1: case 2: case 3: case 4: case 6:
        return (tree->location.expression.properties & 0x40) ||
               (tree->location.expression.comparison & 8);
    case 11:
        if (c2_basic_mode && input_basic_has_side_effects(tree)) return 1;
        if (tree->opcode == 0x16c) return 1;
        if (tree->opcode && tree->opcode < 0x11b)
            return input_instruction_has_side_effects(tree);
        /* Fall through to both operand lists. */
    case 12: case 15:
        for (child = tree->tail.right; child; child = child->next)
            if (input_has_side_effects(child)) return 1;
        for (child = tree->left; child; child = child->next)
            if (input_has_side_effects(child)) return 1;
        return 0;
    case 13: case 14: case 16: case 18: case 20:
        return 1;
    case 17:
        intrinsic = tree->detail.intrinsic;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        if (intrinsic >= C2_INTRINSIC_COUNT)
            recon_unsupported("unmodeled C2 intrinsic side-effect row");
#endif
        if (intrinsics[intrinsic].flags & 0x20) return 0;
        if (intrinsic <= 18)
            return !(intrinsic >= 13 || intrinsic == 3 || intrinsic == 5);
        if (intrinsic <= 23) return intrinsic != 23 && intrinsic != 20;
        if (intrinsic <= 33) return intrinsic != 33 && intrinsic != 28;
        if (intrinsic <= 79) {
            if (intrinsic >= 78 || intrinsic == 71) return 0;
            if (intrinsic <= 72) return 1;
            return intrinsic > 74;
        }
        if (intrinsic >= 88 && intrinsic <= 139) return 0;
        switch (intrinsic) {
        case 165: case 167: case 170: case 174:
        case 217: case 218: case 222: case 229:
        case 236: case 237: case 339: case 340:
            return 0;
        default:
            return 1;
        }
    case 19:
        return tree->opcode == 0x175 ? !c2_exception_mode : 1;
    default:
        return 0;
    }
}

C2Expression *__fastcall symbol_input_expression(C2InputSymbol *symbol)
{
    C2Expression *expression = (C2Expression *)allocate_small_node(25);
    expression->declaration = symbol;
    symbol->input_operand = expression;
    expression->opcode = 0x178;
    expression->location.line = 0;
    return expression;
}

C2Expression *__fastcall new_input_call_expression(unsigned opcode, unsigned short type,
    C2Expression *target, unsigned char convention, C2InputLink link, C2Record *prior)
{
    C2Expression *temporary = new_temporary_operand(type), *output = temporary;
    C2Expression *record = (C2Expression *)allocate_small_node(13);
    record->opcode = opcode;
    record->type = type;
    record->flags |= 1;
    /* Kind13 stores its convention at+36, distinct from the kind16
     * branch/call convention view at+10. */
    record->call_convention = convention;
    if (output) {
        if ((1U << (output->kind & 31)) & 0x7fe) {
            if (output->location.expression.properties & 0x10)
                output = copy_input_expression(output);
            output->location.expression.properties |= 0x10;
        }
        record->tail.right = output;
    }
    if ((1U << (target->kind & 31)) & 0x7fe) {
        if (target->location.expression.properties & 0x10)
            target = copy_input_expression(target);
        target->location.expression.properties |= 0x10;
    }
    record->left = target;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    temporary->value.reg->value = record;
    return record;
}

void __fastcall attach_call_result(C2Expression *call, C2Expression *result)
{
    C2StatementLink *link = (C2StatementLink *)allocate_small_node(29);
    link->record = result;
    link->next = call->detail.statement_links;
    call->detail.statement_links = link;
}

C2Expression *__fastcall new_call_expression(unsigned opcode, unsigned char convention,
    C2Expression *target, C2Expression *arguments, C2Expression *result,
    C2InputLink link, C2Record *prior)
{
    C2Expression *record = (C2Expression *)allocate_small_node(16);
    record->opcode = opcode;
    record->flags |= 1;
    record->convention = convention;
    if (target->kind == 25) {
        bind_symbol_input_use(target, record);
        target = resolve_symbol_input_operand(target, 0);
    }
    if ((1U << (target->kind & 31)) & 0x7fe) {
        if (target->location.expression.properties & 0x10)
            target = copy_input_expression(target);
        target->location.expression.properties |= 0x10;
    }
    record->left = target;
    if (arguments) {
        if ((1U << (arguments->kind & 31)) & 0x7fe) {
            if (arguments->location.expression.properties & 0x10)
                arguments = copy_input_expression(arguments);
            arguments->location.expression.properties |= 0x10;
        }
        target->next = arguments;
        attach_call_result(record, result);
    }
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    return record;
}

void __fastcall bind_symbol_input_use(C2Expression *symbol, C2Expression *use)
{
    C2SymbolInputUse *entry = (C2SymbolInputUse *)allocate_small_node(27);
    entry->next = symbol->tail.symbol_uses;
    symbol->tail.symbol_uses = entry;
    entry->expression = use;
    symbol->declaration->storage.input_references++;
}

C2Expression *__fastcall resolve_symbol_input_operand(C2Expression *symbol, int adjustment)
{
    C2Expression *operand = (C2Expression *)allocate_small_node(4);
    C2InputSymbol *owner = symbol->declaration;
    operand->opcode = 0x12a;
    operand->value.symbol = &owner->symbol;
    owner->storage.input_references += adjustment;
    return operand;
}

C2Expression *__fastcall new_function_exit(C2Function *function)
{
    C2Expression *record = (C2Expression *)allocate_small_node(23);
    record->value.function = function;
    record->opcode = 0x17b;
    record->location.line = record_line_delta;
    return record;
}

/* Replace the front end's generic pointer types before machine analysis. */
void __fastcall validate_function_code(C2Record *first)
{
    C2Expression *record = (C2Expression *)first, *operand;
    C2FunctionRegister *reg;
    C2RegisterBlock *block;
    unsigned short type;
    while (record) {
        if (record->flags & 1) {
            type = 0x1004;
            if (record->opcode == 0x151) type = 0x2004;
            else if (record->opcode == 0x16d)
                validate_function_code(*(C2Record **)record->detail.data);
            if ((record->type & 0xf000) == 0x3000) record->type = type;
            else if ((record->type & 0xf000) == 0x4000) function_input_flags = 1;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x3000) continue;
                operand->type = type;
                if (!((1U << (operand->kind & 31)) & 6U)) continue;
                reg = operand->value.reg;
                if (reg->type == 0x3004) {
                    reg->type = type;
                    reg->flags |= 8;
                } else if (!register_types_match(reg->type, type))
                    operand->value.reg = new_address_register(reg, type, 0, 0);
            }
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x3000) continue;
                operand->type = operand->location.expression.properties & 0x20 ? 0x1004 : type;
                if (!((1U << (operand->kind & 31)) & 6U)) continue;
                reg = operand->value.reg;
                if (reg->type == 0x3004) {
                    reg->type = operand->type;
                    reg->flags |= 8;
                }
            }
        } else if (record->kind == 22) {
            for (operand = record->left; operand; operand = operand->next)
                if ((operand->type & 0xf000) == 0x3000) operand->type = 0x1004;
        }
        record = record->next;
    }
    for (block = first_function_register_block; block; block = block->next)
        for (reg = block->records; reg < block->cursor; ++reg)
            if (reg->kind != 0x10 && reg->type == 0x3004) reg->type = 0x2004;
}

void __fastcall unlink_and_release_input_record(C2Record *record)
{
    C2Record *previous = record->previous, *next = record->next;
    previous->next = next;
    next->previous = previous;
    release_small_node(record);
}

void __fastcall group_symbol_records(C2Function *function)
{
    C2Expression *record = (C2Expression *)function->flow->last->records;
    C2Expression *first = NULL, *last = NULL, *anchor = NULL, *next;
    int active = 0, have_boundary = 0;
    while (record) {
        next = record->next;
        if (record->kind != 21) {
            if (record->kind == 25) {
                if (!active) {
                    active = 1;
                    have_boundary = symbol_range_boundary(record);
                    first = anchor = record;
                    last = NULL;
                } else if (symbol_range_boundary(record)) {
                    if (!have_boundary) {
                        have_boundary = 1;
                        anchor = record;
                        last = record;
                    } else {
                        if (last) splice_symbol_record_range(first, last, anchor);
                        first = anchor = record;
                        last = NULL;
                    }
                } else
                    last = record;
            } else if (active) {
                if (last) splice_symbol_record_range(first, last, anchor);
                active = 0;
            }
        }
        record = next;
    }
}

/* Original DWORD48e0dc starts at1; later optimizer passes replace it with
 * their current mode. Preserve its full value, rather than normalizing BOOL. */
int group_optimized_symbols = 1;

int __fastcall symbol_range_boundary(C2Expression *record)
{
    C2InputSymbol *symbol = record->declaration;
    if (symbol->detail.variable.flags) return 1;
    switch ((signed char)symbol->symbol.source_kind) {
    case 0:
        return 1;
    case 5: case 6: case 7: case 12:
        return group_optimized_symbols;
    default:
        return 0;
    }
}

void __fastcall simplify_symbol_records(C2Function *function)
{
    C2Record *record, *next, *last;
    last = function->flow->first->last_record;
    record = function->flow->last->records->next;
    while (record != last) {
        next = record->next;
        if (record->kind == 25) simplify_symbol_record((C2Expression *)record);
        record = next;
    }
}

int __fastcall simplify_symbol_record(C2Expression *record)
{
    C2Expression *value, *previous;
    value = record->tail.right;
    if (!value) {
        if (record->declaration->detail.variable.flags) return 0;
        remove_symbol_record(record);
        return 1;
    }
    previous = value->previous;
    if (!previous || value->next || previous != record->previous ||
        previous->opcode != 0x15a) return 0;
    unlink_input_record((C2Record *)previous);
    release_small_node(previous);
    if (!record->declaration->detail.variable.flags)
        remove_symbol_record(record);
    return 1;
}

void __fastcall partition_function_blocks(C2Function *function)
{
    C2BasicBlock *block = function->flow->last;
    C2Record *record = block->records->next;
    C2Record *last = function->flow->first->last_record;
    C2Record *next;
    int split = 0;
    unsigned short depth = 0;
    while (record != last) {
        next = record->next;
        if (record->kind == 21) {
            normalize_line_record(record);
        } else {
            if (split || record->kind == 25 || record->kind == 23) {
                split = 0;
                block = split_function_block(function, block, record);
                block->scope_depth = depth;
            }
            switch (record->kind) {
            case 16: case 18: case 22:
                split = 1;
                if (record->opcode == 0x15b) block->flags |= 0x01000000;
                break;
            case 19:
                switch (record->opcode) {
                case 0x161: case 0x171:
                    ++depth;
                    block->scope_depth = depth;
                    break;
                case 0x163: case 0x165:
                    --depth;
                    break;
                case 0x172:
                    if (record_closes_scope(record)) --depth;
                    break;
                }
                break;
            case 25:
                record->owner.block = block;
                break;
            }
        }
        record = next;
    }
}

void __fastcall normalize_line_record(C2Record *record)
{
    C2Expression *previous;
    switch (record->opcode) {
    case 0x17e: case 0x17f:
        if (!record->owner.reference) {
            unlink_and_release_input_record(record);
            return;
        }
        previous = (C2Expression *)record->previous;
        if ((1U << (previous->kind & 31)) & 0x50000U)
            move_input_record_before((C2Record *)previous, record);
        break;
    case 0x180: case 0x181:
        unlink_and_release_input_record(record);
        break;
    case 0x184:
        previous = (C2Expression *)record->previous;
        if ((1U << (previous->kind & 31)) & 0x50000U) {
            if (previous->kind == 16 && previous->detail.data)
                previous = ((C2Expression *)previous->left->next->value.pointer)->value.pointer;
            move_input_record_before((C2Record *)previous, record);
        }
        break;
    }
}

void __fastcall unlink_input_use(C2InputUse *use, C2Expression *owner)
{
    C2SymbolInputUse *node = use->tail.symbol_uses, *previous;
    if (node->expression == owner) {
        use->tail.symbol_uses = node->next;
        release_small_node(node);
        --use->declaration->storage.input_references;
        return;
    }
    previous = node;
    while (previous->next->expression != owner) previous = previous->next;
    node = previous->next;
    previous->next = node->next;
    release_small_node(node);
    --use->declaration->storage.input_references;
}

void __fastcall remove_symbol_record(C2Expression *record)
{
    C2InputSymbol *symbol = record->declaration;
    C2Expression *previous, *next;
    if (symbol->detail.variable.flags) {
        switch (symbol->symbol.source_kind) {
        case 'S':
            clear_function_symbol_references(current_function, symbol);
            break;
        case 'V':
            remove_pending_symbol_references(symbol);
            break;
        case 'X':
            symbol->symbol.association = current_symbol->association;
            break;
        }
    }
    previous = record->previous;
    next = record->next;
    previous->next = next;
    next->previous = previous;
    release_small_node(record);
}

C2Expression *__fastcall replace_input_result(C2Expression *owner, C2Expression *old,
                                             C2Expression *replacement)
{
    if ((1U << (replacement->kind & 31)) & 0x7fe) {
        if (replacement->location.expression.properties & 0x10)
            replacement = copy_input_expression(replacement);
        replacement->location.expression.properties |= 0x10;
    }
    replacement->next = old->next;
    old->next = replacement;
    remove_input_result(owner, old);
    if (((1U << (replacement->kind & 31)) & 0x60) &&
        !(replacement->location.expression.comparison & 0x20)) {
        if (replacement->aggregate_value) {
            replacement->aggregate_value->location.expression.properties &= ~0x10;
            copy_input_operand(owner, replacement->aggregate_value);
        }
        if (replacement->aggregate_second) {
            replacement->aggregate_second->location.expression.properties &= ~0x10;
            copy_input_operand(owner, replacement->aggregate_second);
        }
    }
    return replacement;
}

void __fastcall release_result_aggregate_operands(C2Expression *owner, C2Expression *result);

void __fastcall remove_input_result(C2Expression *owner, C2Expression *result)
{
    C2Expression *current = owner->tail.right, *previous = NULL;
    while (current && current != result) {
        previous = current;
        current = current->next;
    }
    if (previous) previous->next = result->next;
    else owner->tail.right = result->next;
    if ((1U << (result->kind & 31)) & 0x60)
        release_result_aggregate_operands(owner, result);
    release_small_node(result);
}

C2Expression *__fastcall copy_input_operand(C2Expression *owner, C2Expression *value)
{
    C2Expression *last;
    if ((1U << (value->kind & 31)) & 0x7fe) {
        if (value->location.expression.properties & 0x10)
            value = copy_input_expression(value);
        value->location.expression.properties |= 0x10;
    }
    last = owner->left;
    if (last) {
        while (last->next) last = last->next;
        last->next = value;
    } else {
        owner->left = value;
    }
    if (((1U << (value->kind & 31)) & 0x60) &&
        !(value->location.expression.comparison & 0x20)) {
        if (value->aggregate_value) {
            value->aggregate_value->location.expression.properties &= ~0x10;
            copy_input_operand(owner, value->aggregate_value);
        }
        if (value->aggregate_second) {
            value->aggregate_second->location.expression.properties &= ~0x10;
            copy_input_operand(owner, value->aggregate_second);
        }
    }
    return value;
}

C2Expression *__fastcall retain_input_result(C2Expression *expression)
{
    if ((1U << (expression->kind & 31)) & 0x7fe) {
        if (expression->location.expression.properties & 0x10)
            expression = copy_input_expression(expression);
        expression->location.expression.properties |= 0x10;
    }
    return expression;
}

C2Expression *__fastcall new_assignment_expression(unsigned opcode, unsigned short type,
    C2Expression *operand, C2InputLink link, C2Record *prior)
{
    C2Expression *temporary = new_temporary_operand(type);
    C2Expression *result = temporary;
    C2Expression *record = (C2Expression *)allocate_small_node(11);

    record->type = type;
    record->opcode = opcode;
    record->flags |= 1;
    if ((1U << (result->kind & 31)) & 0x7fe) {
        if (result->location.expression.properties & 0x10)
            result = copy_input_expression(result);
        result->location.expression.properties |= 0x10;
    }
    record->tail.right = result;
    if ((1U << (operand->kind & 31)) & 0x7fe) {
        if (operand->location.expression.properties & 0x10)
            operand = copy_input_expression(operand);
        operand->location.expression.properties |= 0x10;
    }
    record->left = operand;
    retain_aggregate_operands(record);
    if (link) link(prior, (C2Record *)record);
    record->location.line = record_line_delta;
    temporary->value.reg->value = record;
    return record;
}

C2Expression *__fastcall new_input_expression(unsigned opcode, unsigned short type)
{
    C2Expression *record = (C2Expression *)allocate_small_node(19);
    record->opcode = opcode;
    record->flags |= 1;
    record->type = type;
    record->location.line = record_line_delta;
    return record;
}

void __fastcall validate_symbol_function_owner(C2Symbol *symbol) { __asm { int 3 } }
void __fastcall queue_symbol_debug(C2Symbol *symbol)
{
    C2DebugLink *link;
    if (c2_il_extended_header && symbol->association.function_owner != current_symbol && current_symbol)
        validate_symbol_function_owner(symbol);
    if (symbol->output_flags & 5) return;
    symbol->output_flags |= 4;
    link = c2_pool_allocate(1, sizeof(*link));
    link->symbol = symbol;
    link->next = pending_debug_symbols;
    pending_debug_symbols = link;
}

C2FunctionRegister *__fastcall new_spill_temporary_register(unsigned short type);

C2Expression *__fastcall new_input_temporary(unsigned short type)
{
    return new_typed_register_expression(0x120, type, new_spill_temporary_register(type));
}

C2Expression *__fastcall new_final_address_expression(unsigned opcode, unsigned short type,
    C2FunctionRegister *symbol, long displacement, C2Expression *base, C2Expression *index)
{
    C2Expression *record = (C2Expression *)allocate_small_node(5);
    record->tail.aggregate_register = symbol;
    record->opcode = opcode;
    record->type = type;
    record->detail.aggregate_displacement = displacement;
    if (base) {
        if ((1U << (base->kind & 31)) & 0x7fe) {
            if (base->location.expression.properties & 0x10)
                base = copy_input_expression(base);
            base->location.expression.properties |= 0x10;
        }
        record->aggregate_value = base;
        base->location.expression.properties |= 0x20;
    }
    if (index) {
        if ((1U << (index->kind & 31)) & 0x7fe) {
            if (index->location.expression.properties & 0x10)
                index = copy_input_expression(index);
            index->location.expression.properties |= 0x10;
        }
        record->aggregate_second = index;
        index->location.expression.properties |= 0x20;
    }
    return record;
}

C2Expression *__fastcall new_register_set_expression(C2RegisterSet *set)
{
    C2Expression *record = (C2Expression *)allocate_small_node(9);
    record->value.registers = set;
    record->opcode = 0x12e;
    return record;
}

/* Original497f48 is the initializer free chain, initially NULL. Recycled
 * links use+8, distinct from the initializer value chain at+4. */
C2InitRecord *free_initializers;
C2InitRecord *__fastcall new_initializer(int kind)
{
    C2InitRecord *record = free_initializers;

    if (record) {
        free_initializers = record->list_next;
        memset(record, 0, sizeof(*record));
    } else record = (C2InitRecord *)c2_pool_allocate(0, sizeof(*record));
    record->kind = kind;
    return record;
}

/* Reuse the497df4 tail already owned by the output cleanup. It belongs
 * to the same initializer chain as global_initializers/497d98. */
extern C2InitRecord *last_global_initializer;
C2InitRecord *__fastcall queue_initializer(C2InitRecord *record)
{
    C2InputSymbol *symbol = (C2InputSymbol *)lookup_global_symbol(record->symbol_serial);
    C2InitRecord **slot, *existing, *next_root, *last;
    int before;

    if (symbol && symbol->initializer) {
        slot = &global_initializers;
        while (*slot && (*slot)->symbol_serial != record->symbol_serial)
            slot = &(*slot)->list_next;
    } else if (last_global_initializer) slot = &last_global_initializer->list_next;
    else slot = &global_initializers;
    existing = *slot;
    if (existing) {
        before = record->offset < existing->offset;
        next_root = existing->list_next;
        while (*slot && record->offset >= (*slot)->offset) {
            slot = &(*slot)->next;
            while (*slot && (*slot)->kind) slot = &(*slot)->next;
        }
        existing = *slot;
        if (existing) {
            last = record->next;
            while (last->next) last = last->next;
            last->next = existing;
        }
        if (before) {
            record->list_next = next_root;
            existing->list_next = NULL;
        }
    }
    *slot = record;
    last_global_initializer = record;
    while (last_global_initializer->list_next)
        last_global_initializer = last_global_initializer->list_next;
    if (symbol) symbol->initializer = record;
    return record;
}

/* Follow the symbol references in an initializer's value chain. The root's
 * +8 list link and a kind2 leaf's +8 referenced serial share storage. */
void __fastcall record_symbol_references(C2Symbol *symbol)
{
    C2InputSymbol *input = (C2InputSymbol *)symbol;
    C2InitRecord *initializer, *value;
    C2Symbol *target;

    if (symbol->output_flags & 3)
        return;
    queue_symbol_debug(symbol);
    if (symbol->kind == 1) {
        if (!(symbol->flags & 0x480))
            return;
        initializer = input->initializer;
        if (!initializer) {
            initializer = lookup_initializer(symbol->serial);
            input->initializer = initializer;
        }
        for (value = initializer->next; value; value = value->next) {
            if (value->kind != 2)
                continue;
            target = lookup_global_symbol(value->referenced_serial);
            if ((target->kind == 4 || target->kind == 1) &&
                (((C2InputSymbol *)target)->attributes & 0x1000))
                target = resolve_alias_symbol(target);
            if (!(target->output_flags & 4))
                record_symbol_references(target);
        }
    } else if (symbol->kind == 4 && (input->attributes & 0x4000) &&
               !(input->attributes & 0x1000))
        input->additional_flags |= 0x20;
}

C2DebugLink *new_symbol_link(void)
{
    return (C2DebugLink *)c2_pool_allocate(4, sizeof(C2DebugLink));
}

/* Original499a0c is reset before each emission; no pre-reset value or
 * wider neighboring storage is inferred. */
unsigned initializer_output_state;
void __fastcall emit_initializer(C2InitRecord *record)
{
    C2Symbol *symbol = record->symbol;
    C2Section *section;
    C2InitRecord *value;
    unsigned saved_size, bytes;

    if (!symbol) symbol = lookup_global_symbol(record->symbol_serial);
    if (symbol && (symbol->kind == 4 || symbol->kind == 1) &&
        (((C2InputSymbol *)symbol)->attributes & 0x1000))
        symbol = resolve_alias_symbol(symbol);
    if (!record->kind)
        section = (C2Section *)lookup_global_symbol(symbol->association.section_serial);
    else if (record->kind == 10)
        section = debug_types_section;
    else {
        c2_report_internal("E:\\utc\\src\\\\P2\\emit.c", 0x201);
        return;
    }
    saved_size = section->symbol.section_size;
    initializer_output_state = 0;
    for (value = record; value; value = value->next) {
        if (!value->kind)
            section->symbol.section_size = symbol->parameter_offset + value->offset;
        else if (value->kind != 10) {
            bytes = output_slot_497f50.target(value, symbol, section);
            section->symbol.section_size += bytes;
        }
    }
    if (section != debug_types_section)
        section->symbol.section_size = saved_size;
    record->emitted = 1;
}

/* Initializer leaf variants share the root's28-byte allocation. Only each
 * selected kind's actual scalar/pointer payload is interpreted here. */
unsigned __fastcall output_callback_44a79d(C2InitRecord *value,
    C2Symbol *symbol, C2Section *section)
{
    unsigned total = 0, displacement;
    int count, bytes;
    C2Symbol *target;

    select_output_section(section);
    switch (value->kind) {
    case 1:
        count = value->repeat_count;
        while (count > 0) {
            if (value->value_kind == 6)
                write_output_bytes(value->value_data, value->value_bytes);
            else switch (value->value_bytes) {
            case 1:
                write_output_fill((unsigned char)value->value_low, count);
                if (count > 1) total += (unsigned)(count - 1) * value->value_bytes;
                count = 1;
                break;
            case 2:
                write_output_word((unsigned short)value->value_low);
                break;
            case 4:
                if (value->value_kind == 5) write_output_bytes(&value->offset, 4);
                else write_output_long(value->value_low);
                break;
            case 8:
                if (value->value_kind == 5) write_output_bytes(&value->offset, 8);
                else {
                    write_output_long(value->value_low);
                    write_output_long(value->value_high);
                }
                break;
            case 10:
                write_output_bytes(&value->offset, 10);
                break;
            default:
                c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x49e);
                break;
            }
            total += value->value_bytes;
            --count;
        }
        return total;
    case 2:
        bytes = value->reference_bytes;
        total = (unsigned)bytes;
        target = lookup_global_symbol(value->referenced_serial);
        if ((target->kind == 4 || target->kind == 1) &&
            (((C2InputSymbol *)target)->attributes & 0x1000))
            target = resolve_alias_symbol(target);
        switch (target->kind) {
        case 1: displacement = value->value_low; break;
        case 3:
            if (!((C2InputSymbol *)target)->detail.variable.flags)
                c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x4d7);
            /* Fall through: branch and function symbols have no addend. */
        case 4: displacement = 0; break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x4eb);
            return 0;
        }
        if (bytes < 4) write_output_fill(0, total);
        else {
            output_callback_43696a(0, target);
            write_output_long(displacement);
            if (bytes > 4) write_output_fill(0, total - 4);
        }
        return total;
    case 4:
        total = query_coff_position(coff_default_flags) % (unsigned)(int)value->alignment;
        /* Original defect: writes one byte for a nonzero remainder, while
         * reporting the remainder as its byte count. */
        if (total) write_output_fill(0, 1);
        return total;
    case 8:
        total = value->referenced_serial;
        write_output_fill(0, total);
        return total;
    case 16:
        total = (unsigned)(int)value->value_bytes;
        switch (value->value_bytes) {
        case 1: write_output_fill((unsigned char)value->value_low, 1); break;
        case 2: write_output_word((unsigned short)value->value_low); break;
        case 4: write_output_long(value->value_low); break;
        case 8:
            write_output_long(value->value_low);
            write_output_long(value->value_high);
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x4c0);
            break;
        }
        return total;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x532);
        return total;
    }
}

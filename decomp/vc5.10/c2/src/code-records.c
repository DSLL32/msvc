#include "code-records.h"
#include "output-callbacks.h"
#include "code-input.h"
#include "small-nodes.h"
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

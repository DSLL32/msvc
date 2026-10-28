#include "functions.h"
#include "code-input.h"
#include "global-optimizer.h"
#include "inlining.h"
#include "optimizer-translate.h"
#include "registers.h"
#include <recon.h>

extern int c2_global_optimize, c2_favor_speed, c2_basic_mode, c2_stack_probe_limit;
extern unsigned input_option_high_bit;
extern C2Symbol *frame_reserved_symbol;
/* Initialized file DWORD0; all earlier writers are pending paths. No pointed
 * object or later nonzero mode is inferred from this Boolean consumer. */
int prologue_has_reserved_storage; /* 4911b4 */
/* Actual later writers store a record identity here (42e6d4/4764c5).
 * The earlier reset-only scalar interpretation is superseded. */
C2Expression *prologue_special_stack_record; /* 4911b8 */
C2Expression *stack_probe_record; /* 4911bc, reset by this pass. */
unsigned prologue_unwind_state; /* 497f34, only the explicit clear is modeled. */

C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
C2Expression *__fastcall find_next_record_kind(C2Record *prior, unsigned char kind);
C2Expression *__fastcall new_final_address_expression(unsigned opcode, unsigned short type,
    C2FunctionRegister *symbol, long displacement, C2Expression *base, C2Expression *index);
int __fastcall function_needs_frame_setup(C2Record *records) { __asm { int 3 } return 0; }
C2Expression *__fastcall push_function_register(unsigned number, C2Expression *prior)
{
    C2Expression *stack, *value, *result;
    stack = new_fixed_register_operand(5, 0x1004);
    value = new_fixed_register_operand(number, 0x1004);
    result = new_fixed_register_operand(5, 0x1004);
    return new_global_ternary_expression(13, 0x1004, result, value, stack,
        insert_input_record, (C2Record *)prior);
}
C2Expression *__fastcall pop_function_register(unsigned number, C2Expression *prior)
{
    C2Expression *stack, *value, *record;
    stack = new_fixed_register_operand(5, 0x1004);
    value = new_fixed_register_operand(number, 0x1004);
    record = new_binary_expression(12, 0x1004, value, stack,
        insert_input_record, (C2Record *)prior);
    attach_optimizer_output(record, new_fixed_register_operand(5, 0x1004));
    return record;
}
C2Expression *__fastcall save_function_registers(C2Expression *prior)
{
    unsigned number;
    for (number = 4; number <= 8; ++number) {
        if ((selected_save_register_mask & (1U << number)) &&
            (machine_registers[number].flags & 0x10)) {
            prior = push_function_register(number, prior);
            prior->opcode = 0xba;
            ++current_function->saved_register_count;
        }
    }
    return prior;
}
C2Expression *__fastcall restore_function_registers(C2Expression *prior, unsigned skip)
{
    int number;
    unsigned mask;
    for (number = 8; number >= 4; --number) {
        mask = 1U << number;
        if ((selected_save_register_mask & mask) &&
            (machine_registers[number].flags & 0x10) && !(skip & mask)) {
            prior = pop_function_register(number, prior);
            prior->opcode = 0xb9;
        }
    }
    return prior;
}
C2BasicBlock *__fastcall find_register_save_region(C2BasicBlock *first,
    C2BasicBlock *last, C2BasicBlock **end)
{
    C2FlowEdge *branch, *other, *edge;
    C2BasicBlock *start, *finish, *block, *next, *source;
    short number, lower, upper;

    branch = first->outgoing;
    if (!branch || first == last || first->incoming->next_incoming ||
        !last->incoming->next_incoming) return NULL;
    other = branch->next_outgoing;
    if (!other || other->next_outgoing) return NULL;
    finish = first->previous;
    if (branch->destination == finish) start = other->destination;
    else if (other->destination == finish) start = branch->destination;
    else return NULL;
    for (edge = last->incoming; edge; edge = edge->next_incoming)
        if (edge->source == start->next) break;
    if (!edge) return NULL;
    if (start != last) {
        if (start->incoming->source != first || start->incoming->next_incoming)
            return NULL;
    } else {
        while (finish != start && !finish->incoming->next_incoming) {
            branch = finish->outgoing;
            other = branch->next_outgoing;
            if (!other || other->next_outgoing) break;
            next = finish->previous;
            if ((next != branch->destination && next != other->destination) ||
                (start != branch->destination && start != other->destination)) break;
            finish = next;
        }
    }
    for (edge = last->incoming; edge; edge = edge->next_incoming) {
        source = edge->source;
        if (source == last->next || source == start->next) continue;
        for (block = first; block != finish; block = block->previous)
            if (block == source) break;
        if (block == finish) return NULL;
    }
    if (start != last) {
        if ((short)first->number84 == 1) {
            number = 1;
            for (block = first; block != last; block = block->previous) {
                block->number84 = number;
                number = (short)((unsigned short)number + 1);
            }
        }
        lower = (short)start->number84;
        upper = (short)last->next->number84;
        for (block = start->previous; block != last; block = block->previous) {
            for (edge = block->incoming; edge; edge = edge->next_incoming) {
                number = (short)edge->source->number84;
                if (number < lower || number > upper) return NULL;
            }
        }
        lower = (short)start->previous->number84;
        for (block = start; block != last; block = block->previous) {
            for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                number = (short)edge->destination->number84;
                if (number < lower) return NULL;
                if (number > upper &&
                    (block->previous != last || edge->destination != last)) return NULL;
            }
        }
    }
    *end = finish;
    return start;
}
int __fastcall region_uses_saved_register(C2MachineRegister *reg,
    C2Expression *first, C2Expression *last)
{
    C2Expression *operand;
    C2FunctionRegister *source;

    for (; first != last; first = first->next) {
        if (!(first->flags & 1)) continue;
        for (operand = first->tail.right; operand; operand = operand->next) {
            if (operand->kind == 1) {
                source = operand->original_register;
                /* Only the modeled static canonical identities are available. */
                if ((unsigned long)source >= (unsigned long)machine_registers &&
                    (unsigned long)source <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                    source != (C2FunctionRegister *)&machine_registers[0] &&
                    source != (C2FunctionRegister *)&machine_registers[1] &&
                    source != (C2FunctionRegister *)&machine_registers[2] &&
                    source != (C2FunctionRegister *)&machine_registers[3] &&
                    source != (C2FunctionRegister *)&machine_registers[4] &&
                    source != (C2FunctionRegister *)&machine_registers[5] &&
                    source != (C2FunctionRegister *)&machine_registers[6] &&
                    source != (C2FunctionRegister *)&machine_registers[7] &&
                    source != (C2FunctionRegister *)&machine_registers[8] &&
                    source != (C2FunctionRegister *)&machine_registers[18] &&
                    source != (C2FunctionRegister *)&machine_registers[19] &&
                    source != (C2FunctionRegister *)&machine_registers[66] &&
                    source != (C2FunctionRegister *)&machine_registers[67] &&
                    source != (C2FunctionRegister *)&machine_registers[68] &&
                    source != (C2FunctionRegister *)&machine_registers[69] &&
                    source != (C2FunctionRegister *)&machine_registers[70] &&
                    source != (C2FunctionRegister *)&machine_registers[71] &&
                    source != (C2FunctionRegister *)&machine_registers[72] &&
                    source != (C2FunctionRegister *)&machine_registers[73]) { __asm { int 3 } }
                if (source->canonical == (C2FunctionRegister *)reg) return 1;
            } else if (operand->kind == 9) {
                if (reg != &machine_registers[4] && reg != &machine_registers[5] &&
                    reg != &machine_registers[6] && reg != &machine_registers[7] &&
                    reg != &machine_registers[8]) { __asm { int 3 } }
                if (optimizer_set_contains(operand->value.registers, reg->number)) return 1;
            }
        }
    }
    return 0;
}
C2Expression *__fastcall push_region_register(C2MachineRegister *reg,
    C2Expression *position)
{
    C2Expression *record;

    if (reg != &machine_registers[4] && reg != &machine_registers[5] &&
        reg != &machine_registers[6] && reg != &machine_registers[7] &&
        reg != &machine_registers[8]) { __asm { int 3 } }
    record = push_function_register(reg->number, position);
    record->opcode = 0xba;
    return record;
}
C2Expression *__fastcall pop_region_register(C2MachineRegister *reg,
    C2Expression *position)
{
    C2Expression *record;

    if (reg != &machine_registers[4] && reg != &machine_registers[5] &&
        reg != &machine_registers[6] && reg != &machine_registers[7] &&
        reg != &machine_registers[8]) { __asm { int 3 } }
    record = pop_function_register(reg->number, position);
    record->opcode = 0xb9;
    return record;
}

int __fastcall save_register_in_region(C2MachineRegister *reg, C2BasicBlock *first,
    C2BasicBlock *last, C2BasicBlock *start, C2BasicBlock *end)
{
    C2Expression *position;
    int changes = 0;

    if (region_uses_saved_register(reg, (C2Expression *)first->records,
        (C2Expression *)end->records)) return 0;
    for (;;) {
        if (region_uses_saved_register(reg, (C2Expression *)last->records,
            (C2Expression *)last->last_record)) break;
        if (last == start) {
            last = last->next;
            first = end;
            start = find_register_save_region(first, last, &end);
        } else if (!region_uses_saved_register(reg, (C2Expression *)start->records,
            (C2Expression *)last->records)) {
            last = start->next;
            first = end;
            start = find_register_save_region(first, last, &end);
        } else {
            if (region_uses_saved_register(reg, (C2Expression *)end->records,
                (C2Expression *)start->records)) break;
            last = last->next;
            first = start;
            start = find_register_save_region(first, last, &end);
        }
        ++changes;
        if (!start || region_uses_saved_register(reg, (C2Expression *)first->records,
            (C2Expression *)end->records)) break;
    }
    if (!changes) return 0;
    position = (C2Expression *)first->records;
    if (position->next->kind == 25) position = position->next;
    push_region_register(reg, position);
    position = ((C2Expression *)last->last_record)->previous;
    if (position->kind == 16 && !position->detail.data && position->opcode != 0x15c)
        position = position->previous;
    pop_region_register(reg, position);
    return 1;
}
unsigned __fastcall count_saved_function_registers(C2Function *function)
{ __asm { int 3 } return 0; }
C2Expression *__fastcall build_exception_prologue(C2Expression *prior, int opening)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall build_funclet_prologue(C2Expression *prior, int opening)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall build_exception_epilogue(C2Expression *prior)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall build_funclet_epilogue(C2Expression *prior)
{ __asm { int 3 } return NULL; }
/* Compiler runtime-symbol cache at499bc8:128 initialized NULL pointers.
 * This is application symbol bookkeeping, not a CRT implementation. */
C2FunctionSymbol *runtime_function_cache[128];
unsigned __fastcall hash_runtime_function_name(char *name);
extern int output_enabled;
C2Symbol *__fastcall find_runtime_function(char *name)
{
    unsigned first = hash_runtime_function_name(name) & 127;
    unsigned index = first;
    C2FunctionSymbol *function;
    char *left, *right;

    for (;;) {
        function = runtime_function_cache[index];
        if (!function) {
            function = (C2FunctionSymbol *)c2_allocate_node(4, 0);
            runtime_function_cache[index] = function;
            function->symbol.kind = 4;
            function->symbol.serial = c2_internal_listing ? index - 0x81U : next_symbol_serial();
            function->symbol.name = arena_strdup(name, 0);
            function->symbol.output_flags &= ~3;
            function->symbol.flags = 0;
            function->attributes = (function->attributes & ~0xffU) | 0x47;
            queue_symbol_debug(&function->symbol);
            function->attributes = (function->attributes & ~0x600U) | 0x100;
            insert_symbol(&function->symbol, 0);
            return &function->symbol;
        }
        /* The original inlines byte comparison; no external strcmp edge. */
        left = function->symbol.name;
        right = name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) {
            if (c2_internal_listing) queue_symbol_debug(&function->symbol);
            return &function->symbol;
        }
        index = (index + 1) & 127;
        if (index == first)
            c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 0x73c);
    }
}
unsigned __fastcall hash_runtime_function_name(char *name)
{
    unsigned hash = 0;
    for (; *name; ++name) hash += (signed char)*name;
    return hash;
}
C2Expression *__fastcall new_runtime_call(unsigned opcode, unsigned short type,
    C2Expression *result, C2Expression *target, unsigned char convention,
    C2InputLink link, C2Record *prior)
{ __asm { int 3 } return NULL; }

void __fastcall build_function_prologue(C2Function *function)
{
    C2FunctionSymbol *symbol = (C2FunctionSymbol *)current_symbol;
    C2FlowGraph *flow;
    C2Expression *record, *first, *second, *constant, *target, *entry;
    C2BasicBlock *first_block, *last_block, *start, *end;
    C2Symbol *runtime;
    unsigned extra = function->frame_layout_state, bytes, flags, skipped = 0;
    unsigned number, mask, saved_bytes;
    int emit_frame_setup = 1;

    prologue_special_stack_record = NULL;
    stack_probe_record = NULL;
    if (symbol->symbol.flags & 0x400) return;
    flags = function->flags;
    if (!(flags & 0x10) && (int)extra <= 0 && c2_global_optimize && (flags & 1))
        emit_frame_setup = function_needs_frame_setup(function->flow->last->records);
    flow = function->flow;
    record = find_next_record_kind(flow->last->records, 22);
    if (record->location.line) {
        record_line_delta = record->location.line;
        record_source_line = function->source_line_base + record_line_delta;
    }
    symbol = (C2FunctionSymbol *)current_symbol;
    if ((flags & 0x300000) && !symbol->frame_stack_bytes) {
        function->flags &= ~0x300000U;
        if (!(machine_registers[6].flags & 0x10)) function->flags |= 0x10;
    }
    flags = function->flags;
    if (!c2_global_optimize ||
        ((!(flags & 0x10) || (flags & 0x300000)) &&
         (symbol->frame_stack_bytes || symbol->argument_stack_bytes ||
          (flags & 0xc0) || (flags & 0x80000)))) {
        if (flags & 0x300000) prologue_unwind_state = 0;
        if ((c2_favor_speed || (!(current_function->flags & 0x8000) &&
             !(current_function->symbol->additional_flags & 0x200))) && emit_frame_setup) {
            record = push_function_register(6, record);
            first = new_fixed_register_operand(5, 0x1004);
            if ((int)extra > 0)
                first = new_final_address_expression(0x122, 0x1004, NULL,
                    (long)(0U - extra), first, NULL);
            second = new_fixed_register_operand(6, 0x1004);
            record = new_binary_expression((int)extra > 0 ? 0x12 : 1,
                0x1004, second, first, insert_input_record, (C2Record *)record);
        }
        if (flags & 0x300000) {
            constant = new_integer_expression(0x1004, (unsigned __int64)0xfffffff8U);
            first = new_fixed_register_operand(5, 0x1004);
            second = new_fixed_register_operand(5, 0x1004);
            record = new_global_ternary_expression(0x2e, 0x1004, second, first,
                constant, insert_input_record, (C2Record *)record);
            attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
        }
    } else function->flags |= 0x100;

    function->flags = (function->flags & ~0x20U) |
        ((machine_registers[6].flags & 0x10) << 1);
    if (current_function->symbol->symbol.flags & 0x1000)
        record = build_exception_prologue(record, 1);
    if ((current_function->flags & 0x8000) || (current_function->symbol->additional_flags & 0x200))
        record = build_funclet_prologue(record, 1);
    symbol = (C2FunctionSymbol *)current_symbol;
    bytes = symbol->frame_stack_bytes;
    if (prologue_has_reserved_storage) bytes -= (unsigned)frame_reserved_symbol->value;
    if (bytes) {
        if (!input_option_high_bit && bytes < (unsigned)c2_stack_probe_limit) {
            if (function->flags & 0x300000) {
                saved_bytes = 0;
                for (number = 4; number <= 8; ++number)
                    if ((selected_save_register_mask & (1U << number)) &&
                        (machine_registers[number].flags & 0x10)) saved_bytes += 4;
                if ((saved_bytes + bytes) & 7) bytes += 4;
            }
            constant = new_integer_expression(0x1004, bytes);
            first = new_fixed_register_operand(5, 0x1004);
            second = new_fixed_register_operand(5, 0x1004);
            record = new_global_ternary_expression(0x32, 0x1004, second, first,
                constant, insert_input_record, (C2Record *)record);
            attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
        } else {
            constant = new_integer_expression(0x1004, bytes);
            first = new_fixed_register_operand(1, 0x1004);
            record = new_binary_expression(1, 0x1004, first, constant,
                insert_input_record, (C2Record *)record);
            runtime = find_runtime_function(c2_basic_mode ? "___vbaChkstk" : "__chkstk");
            target = new_scalar_expression(0x129, (unsigned)runtime);
            first = new_fixed_register_operand(1, 0x1004);
            record = new_runtime_call(14, 0x1004, first, target, 4,
                insert_input_record, (C2Record *)record);
            copy_input_operand(record, new_fixed_register_operand(1, 0x1004));
            copy_input_operand(record, new_fixed_register_operand(5, 0x1004));
            attach_optimizer_output(record, new_fixed_register_operand(5, 0x1004));
            attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
            stack_probe_record = record;
        }
    }
    if ((function->symbol->symbol.flags & 0x1000) ||
        (function->symbol->additional_flags & 0x200)) {
        machine_registers[4].flags |= 0x10;
        machine_registers[7].flags |= 0x10;
        machine_registers[8].flags |= 0x10;
    }
    if (!c2_global_optimize || (function->flags & 0x300000)) {
        record = save_function_registers(record);
    } else {
        first_block = flow->last->previous;
        last_block = flow->first->next;
        start = find_register_save_region(first_block, last_block, &end);
        if (!start || (function->symbol->symbol.flags & 0x1000) ||
            (function->symbol->additional_flags & 0x200) || (function->flags & 0x80)) {
            record = save_function_registers(record);
        } else {
            for (number = 4; number <= 8; ++number) {
                mask = 1U << number;
                if (!(selected_save_register_mask & mask) ||
                    !(machine_registers[number].flags & 0x10)) continue;
                if (!save_register_in_region(machine_registers + number,
                        first_block, last_block, start, end)) {
                    record = push_function_register(number, record);
                    record->opcode = 0xba;
                } else skipped |= mask;
                ++function->saved_register_count;
            }
        }
    }
    if (current_function->symbol->symbol.flags & 0x1000)
        build_exception_prologue(record, 0);
    if ((current_function->flags & 0x8000) || (current_function->symbol->additional_flags & 0x200))
        build_funclet_prologue(record, 0);
    if (prologue_has_reserved_storage) bytes += (unsigned)frame_reserved_symbol->value;
    entry = (C2Expression *)get_symbol_branch_target((C2BranchTarget *)flow->first->last_record, 23);
    if (entry->location.line) {
        record_line_delta = entry->location.line;
        record_source_line = function->source_line_base + record_line_delta;
    }
    record = entry->previous;
    if (function->flags & 0x80) {
        number = count_saved_function_registers(function);
        first = new_fixed_register_operand(6, 0x1004);
        first = new_final_address_expression(0x122, 0x1004, NULL,
            (long)(0U - number * 4 - bytes), first, NULL);
        second = new_fixed_register_operand(5, 0x1004);
        record = new_binary_expression(0x12, 0x1004, second, first,
            insert_input_record, (C2Record *)record);
    }
    if (current_function->symbol->symbol.flags & 0x1000)
        record = build_exception_epilogue(record);
    if ((current_function->flags & 0x8000) || (current_function->symbol->additional_flags & 0x200))
        record = build_funclet_epilogue(record);
    record = restore_function_registers(record, skipped);
    symbol = (C2FunctionSymbol *)current_symbol;
    flags = function->flags;
    if (!c2_global_optimize ||
        ((!(flags & 0x10) || (flags & 0x300000)) &&
         (symbol->frame_stack_bytes || symbol->argument_stack_bytes ||
          (flags & 0xc0) || (flags & 0x80000)))) {
        if ((int)extra > 0) {
            constant = new_integer_expression(0x1004, (int)extra);
            first = new_fixed_register_operand(6, 0x1004);
            second = new_fixed_register_operand(6, 0x1004);
            record = new_global_ternary_expression(0x2d, 0x1004, second, first,
                constant, insert_input_record, (C2Record *)record);
            attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
        }
        if (bytes) {
            if (!c2_favor_speed) {
                first = new_fixed_register_operand(5, 0x1004);
                second = new_fixed_register_operand(6, 0x1004);
                target = new_fixed_register_operand(5, 0x1004);
                record = new_global_ternary_expression(0xa7, 0x1004, target,
                    second, first, insert_input_record, (C2Record *)record);
                attach_optimizer_output(record, new_fixed_register_operand(6, 0x1004));
                goto return_record;
            }
            first = new_fixed_register_operand(6, 0x1004);
            second = new_fixed_register_operand(5, 0x1004);
            record = new_binary_expression(1, 0x1004, second, first,
                insert_input_record, (C2Record *)record);
        } else if (!emit_frame_setup) goto return_record;
        record = pop_function_register(6, record);
    } else if (!c2_favor_speed && ((current_function->flags & 0x8000) ||
                (current_function->symbol->additional_flags & 0x200))) {
        first = new_fixed_register_operand(5, 0x1004);
        second = new_fixed_register_operand(6, 0x1004);
        target = new_fixed_register_operand(5, 0x1004);
        record = new_global_ternary_expression(0xa7, 0x1004, target,
            second, first, insert_input_record, (C2Record *)record);
        attach_optimizer_output(record, new_fixed_register_operand(6, 0x1004));
        machine_registers[6].flags |= 0x10;
        function->flags |= 0x20;
    } else if (bytes) {
        constant = new_integer_expression(0x1004, bytes);
        first = new_fixed_register_operand(5, 0x1004);
        second = new_fixed_register_operand(5, 0x1004);
        record = new_global_ternary_expression(0x2d, 0x1004, second, first,
            constant, insert_input_record, (C2Record *)record);
        attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
    }
return_record:
    symbol = (C2FunctionSymbol *)current_symbol;
    bytes = symbol->symbol.flags & 0x1c ? symbol->argument_stack_bytes : 0;
    constant = new_integer_expression(0x1004, bytes);
    record = new_transfer_expression(8, 0x1004, NULL, constant,
        insert_input_record, (C2Record *)record);
    attach_optimizer_output(record, new_fixed_register_operand(66, 0x8000));
}

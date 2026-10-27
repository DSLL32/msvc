#include "functions.h"
#include "function-records.h"
#include "code-input.h"
#include "flow.h"
#include "final-schedule.h"
#include "inlining.h"
#include "registers.h"
#include "optimizer.h"
#include "global-optimizer.h"
#include "optimizer-translate.h"
#include <recon.h>

C2ScheduleNodeHook schedule_input_mode;             /* 493060, set by the constructor */
C2ScheduleEdgeHook schedule_output_mode;            /* 4930a0, set by the constructor */
extern int c2_favor_speed, extra_function_passes;
int schedule_final_mode;                 /* 49321c */
C2Expression *schedule_output_tail;       /* 493220 */
C2Expression *schedule_first_record;      /* 493224 */
C2Expression *schedule_last_record;       /* 493228 */
C2ScheduleGraph *schedule_graph;          /* 49322c */

int __fastcall final_scheduling_is_enabled(C2Function *function, int mode)
{
    if (!mode) return 1;
    if (!c2_favor_speed || !(function->flags & 8)) return 0;
    if (c2_cpu_generation == 4 || c2_cpu_generation == 6 ||
        extra_function_passes || (function->flags & 1)) return 0;
    return 1;
}
C2ScheduleGraph *__fastcall new_schedule_graph(C2ScheduleNodeHook input_mode, C2ScheduleEdgeHook output_mode)
{
    C2ScheduleGraph *graph = c2_pool_allocate(4, sizeof(C2ScheduleGraph));
    initialize_flow_pool(&graph->flow, 4, 60, 24);
    schedule_output_mode = output_mode;
    schedule_input_mode = input_mode;
    return graph;
}
/* CPU-specific source columns are contextual scheduling inputs, not a copy of
 * the image. Only the first score/delay fields have reached named consumers.
 * Reserved backing bytes below are UNMODELED, not initialized-value claims. */
static C2ScheduleWeights early_schedule_weights[4] = {
    {-1}, {-1}, {-1, 13, -5, -1, 0, 15, 0}, {23}
}; /*492288,16B stride. Only Pentium's reached priority columns extend
    * the fixed-resource field; pair_weight and pending0e remain UNMODELED. */
static C2ScheduleWeights late_schedule_weights[4] = {
    {-1}, {-1}, {-1, 13, -5, -1, -2, 16, 16}, {23}
}; /*493018: only Pentium's reached priority columns extend the first field.
    * pending0e and other profiles' remaining fields are UNMODELED. */
static C2ScheduleOpcodeCost schedule_costs_386 = {0}; /*49b900 row0 delay only. */
static C2ScheduleOpcodeCost schedule_costs_486 = {0}; /*49c650 row0 delay only. */
static C2ScheduleOpcodeCost schedule_costs_pentium = {0}; /*4922c8 row0 delay only. */
short __fastcall select_pentium_schedule_node(C2ScheduleNode *, C2ScheduleNode **, int);
short __fastcall select_486_schedule_node(C2ScheduleNode *, C2ScheduleNode **, int);
short __fastcall select_p6_schedule_node(C2ScheduleNode *, C2ScheduleNode **, int);
void __fastcall prepare_early_schedule_costs(void);
C2ScheduleWeights *schedule_weights;       /*493070 */
C2ScheduleSelector schedule_select_node;   /*493074,three actual RET4 targets */
short schedule_issue_width;               /*493078 */
int schedule_function_flag8;              /*493090 */
int schedule_aggregate_return;            /*493094 */
short schedule_cpu_index;                 /*49309c,only the actual WORD writes */
C2ScheduleOpcodeCost *schedule_opcode_costs; /*4930a4 */
C2ScheduleResource schedule_resources[4];  /*4930e0,only initialized member span */
short schedule_resource_count;            /*493138 */
unsigned short schedule_parameter_4921f8;  /*written here, no consumer inferred */
int schedule_function_flag16;             /*4921fc */

void __fastcall initialize_schedule_function(C2Function *function)
{
    unsigned flags = function->flags;
    int cpu, i;
    short capacity, spacing;

    schedule_function_flag8 = (flags >> 3) & 1;
    schedule_cpu_index = (short)(c2_cpu_generation - 3);
    if (schedule_cpu_index < 0) schedule_cpu_index = 0;
    cpu = schedule_cpu_index;
    /* Higher CPU selections read data not yet reconstructed, in either build. */
    if (cpu > 3) { __asm { int 3 } }
    schedule_parameter_4921f8 = 0;
    switch (cpu) {
    case 0: schedule_issue_width = 2; schedule_resource_count = 4; break;
    case 1: schedule_issue_width = 1; schedule_resource_count = 2; break;
    case 2: schedule_issue_width = 2; schedule_resource_count = 4; break;
    default: schedule_issue_width = 3; schedule_resource_count = 4; break;
    }
    schedule_weights = schedule_final_mode ? &early_schedule_weights[cpu] : &late_schedule_weights[cpu];
    for (i = 0; i < schedule_resource_count; ++i) {
        capacity = cpu == 3 ? 3 : 1;
        if (cpu == 3) spacing = i == 1 ? 3 : i == 2 ? 2 : 1;
        else spacing = (cpu != 1 && i == 2) ? 3 : 1;
        schedule_resources[i].capacity = capacity;
        schedule_resources[i].spacing = spacing;
        schedule_resources[i].issued = 0;
        schedule_resources[i].occupied = 0;
    }
    switch (cpu) {
    case 0:
        schedule_opcode_costs = &schedule_costs_386;
        schedule_select_node = select_pentium_schedule_node;
        break;
    case 1:
        schedule_opcode_costs = &schedule_costs_486;
        schedule_select_node = select_486_schedule_node;
        break;
    case 2:
        schedule_opcode_costs = &schedule_costs_pentium;
        schedule_select_node = select_pentium_schedule_node;
        break;
    case 3:
        schedule_opcode_costs = &schedule_costs_pentium;
        schedule_select_node = select_p6_schedule_node;
        break;
    }
    schedule_function_flag16 = (flags >> 4) & 1;
    schedule_aggregate_return = schedule_function_flag16 &&
        (((current_symbol->flags & 0x1c) == 4) ||
         ((current_symbol->flags & 0x1c) == 0x10));
    if (schedule_final_mode) prepare_early_schedule_costs();
}
int schedule_opcode46_cooldown; /*493084, initialized0; issue writes2, cycle advance decrements. */
int __fastcall schedule_resource_available(int resource)
{
    if ((unsigned)resource >= 4) { __asm { int 3 } }
    return schedule_resources[resource].occupied < schedule_resources[resource].capacity;
}
int __fastcall final_float_stack_effect(C2Expression *record);
extern short schedule_early_delta;
int __fastcall early_schedule_node_is_busy(C2ScheduleNode *node)
{
    if (schedule_early_delta <= 6) return 0;
    return final_float_stack_effect(node->record) + (int)schedule_early_delta > 8;
}
int __fastcall can_pair_schedule_nodes(C2ScheduleNode *first, C2ScheduleNode *second);
short __fastcall select_pentium_schedule_node(C2ScheduleNode *node, C2ScheduleNode **selected, int count)
{
    unsigned resource = node->properties & 7;
    int i;
    if (node->record->opcode == 0x46 && schedule_opcode46_cooldown > 0) return -1;
    if (schedule_final_mode && early_schedule_node_is_busy(node)) return -1;
    if ((int)resource >= schedule_resource_count) {
        for (i = 0; i < schedule_resource_count; ++i)
            if (!schedule_resource_available(i)) return -1;
        return 0;
    }
    if (!count) {
        if (schedule_resource_available(resource) && schedule_resource_available(resource + 1))
            return (short)resource;
    } else if (can_pair_schedule_nodes(selected[0], node)) return (short)(resource + 1);
    return -1;
}
short __fastcall select_486_schedule_node(C2ScheduleNode *node, C2ScheduleNode **selected, int count)
{ __asm { int 3 } return 0; }
short __fastcall select_p6_schedule_node(C2ScheduleNode *node, C2ScheduleNode **selected, int count)
{ __asm { int 3 } return 0; }
/* Original499fa0 holds eight canonical floating-register pointers. The
 * schedule mapper at46333c compares/stores actual register canonical roots;
 * only this reset establishes all eight NULL entries. The adjacent49a000
 * signed WORD is the stack's first occupied slot, not a ninth entry. */
C2FunctionRegister *early_float_registers[8];
short early_float_stack_base;
extern short schedule_early_base, schedule_early_delta;
void __fastcall prepare_early_schedule_costs(void)
{
    early_float_stack_base = 8;
    schedule_early_base = 2;
    memset(early_float_registers, 0, sizeof(early_float_registers));
}
int __fastcall schedule_range_has_memory_output(C2Expression *first);
C2Expression *schedule_entry_records[10];   /*492260,only the first count slots live */
unsigned schedule_entry_record_count;       /*492218 */
C2Expression *schedule_entry_tail;           /*493130 */
C2Expression *schedule_exit_head;            /*493088 */
void __fastcall prepare_schedule_registers(C2Function *function)
{
    C2Expression *boundary, *record, *marker, *scan, *operand;
    int memory_output;

    boundary = (C2Expression *)function->flow->last->records;
    while (boundary->kind != 22) boundary = boundary->next;
    record = boundary->next;
    memory_output = schedule_range_has_memory_output(record);
    marker = NULL;
    for (; record->opcode != 0x17e; record = record->next) {
        switch (record->opcode) {
        case 4:
            boundary = record;
            break;
        case 1:
            operand = record->tail.right;
            if (operand->kind == 1 && operand->original_register == (C2FunctionRegister *)&machine_registers[6]) {
                if (!(function->flags & 0x10) ||
                    (record->left->kind == 1 &&
                     record->left->original_register == (C2FunctionRegister *)&machine_registers[5]))
                    boundary = record;
            }
            break;
        case 0xd: case 0xba:
            if (record->left->kind == 1 &&
                record->left->original_register == (C2FunctionRegister *)&machine_registers[2])
                boundary = record;
            break;
        case 0x2e:
            if (record->tail.right->kind == 1 &&
                record->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5] &&
                record->left->next->kind == 7 && record->left->next->integer.low == (unsigned)-8)
                boundary = record;
            break;
        case 0x32:
            if (record->tail.right->kind == 1 &&
                record->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5] &&
                (!(function->flags & 0x10) || memory_output)) boundary = record;
            break;
        case 0x180:
            marker = record;
            break;
        }
    }
    scan = marker ? marker->previous : record->previous;
    schedule_entry_tail = scan;
    schedule_entry_record_count = 0;
    while (scan != boundary && schedule_entry_record_count < 10) {
        schedule_entry_records[schedule_entry_record_count++] = scan;
        scan = scan->previous;
    }
    if (schedule_entry_record_count == 10) boundary = scan;
    unlink_input_record((C2Record *)record);
    insert_input_record((C2Record *)boundary, (C2Record *)record);
    if (marker) {
        unlink_input_record((C2Record *)marker);
        insert_input_record((C2Record *)boundary, (C2Record *)marker);
    }

    boundary = (C2Expression *)function->flow->first->last_record;
    while (boundary->kind != 23) boundary = boundary->previous;
    marker = NULL;
    for (record = boundary->previous; record->opcode != 0x17f; record = record->previous) {
        switch (record->opcode) {
        case 1:
            if (record->left->kind == 1 &&
                record->left->original_register == (C2FunctionRegister *)&machine_registers[6] &&
                !(function->flags & 0x10)) boundary = record;
            break;
        case 0xc: case 0xb9:
            operand = record->tail.right;
            if (operand->kind == 1 &&
                (operand->original_register == (C2FunctionRegister *)&machine_registers[2] ||
                 (!(function->flags & 0x10) &&
                  operand->original_register == (C2FunctionRegister *)&machine_registers[6])))
                boundary = record;
            break;
        case 0x2d:
            if (record->tail.right->kind == 1 &&
                record->tail.right->original_register == (C2FunctionRegister *)&machine_registers[5])
                boundary = record;
            break;
        case 0xa7:
            boundary = record;
            break;
        case 0x181:
            marker = record;
            break;
        }
    }
    schedule_exit_head = marker ? marker->next : record->next;
    unlink_input_record((C2Record *)record);
    prepend_input_record((C2Record *)boundary, (C2Record *)record);
    if (marker) {
        unlink_input_record((C2Record *)marker);
        prepend_input_record((C2Record *)boundary, (C2Record *)marker);
    }
}
int __fastcall schedule_range_has_memory_output(C2Expression *first)
{
    C2Expression *output;
    C2FunctionRegister *reg;
    unsigned kind;
    for (; first; first = first->next) {
        switch (first->kind) {
        case 16: case 18: case 23: case 25:
            return 0;
        }
        if (!(first->flags & 1)) continue;
        output = first->tail.right;
        if (!output || output->kind != 2) continue;
        reg = output->value.reg;
        if ((unsigned long)reg >= (unsigned long)machine_registers &&
            (unsigned long)reg < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
            reg != (C2FunctionRegister *)&machine_registers[1] &&
            reg != (C2FunctionRegister *)&machine_registers[2] &&
            reg != (C2FunctionRegister *)&machine_registers[3] &&
            reg != (C2FunctionRegister *)&machine_registers[6] &&
            reg != (C2FunctionRegister *)&machine_registers[7] &&
            reg != (C2FunctionRegister *)&machine_registers[18] &&
            reg != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
        kind = reg == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ?
            machine_register_storage.sentinel.kind : reg->kind;
        if (kind == 3 || kind == 4 || kind == 6) return 1;
    }
    return 0;
}
C2Expression *__fastcall find_early_schedule_range_end(C2Expression *first);
C2Expression *__fastcall find_schedule_range_end(C2Expression *first)
{
    C2Expression *record;
    int count;
    if (first->opcode == 0x17f) first = first->next;
    record = first;
    for (count = 0; record && count <= 80; ++count, record = record->next) {
        switch (record->kind) {
        case 16: case 18: case 25:
            return record;
        case 21:
            if (record->opcode != 0x17f) break;
            /* The closing transfer belongs to the next scheduling range. */
        case 23:
            return record->previous;
        }
    }
    if (schedule_final_mode) return find_early_schedule_range_end(first);
    return record->previous;
}
C2Expression *__fastcall find_early_schedule_range_end(C2Expression *first)
{ __asm { int 3 } return NULL; }
int __fastcall early_schedule_range_is_usable(C2Expression *first, C2Expression *last);
int __fastcall schedule_range_has_work(C2Expression *first, C2Expression *last)
{
    C2Expression *record;
    int count = 0;
    if (schedule_final_mode && !early_schedule_range_is_usable(first, last)) return 0;
    if (first->opcode == 0x17f) return 0;
    for (record = first; record != last; record = record->next)
        if ((record->flags & 1) && ++count >= 2) return 1;
    return 0;
}
int __fastcall early_schedule_range_is_usable(C2Expression *first, C2Expression *last)
{
    C2Expression *record;
    int total = 0, floating = 0, overlapped = 0;
    unsigned opcode;

    for (record = first; record != last; record = record->next) {
        if (!(record->flags & 1)) continue;
        ++total;
        if ((record->type & 0xf000) != 0x4000) continue;
        ++floating;
        opcode = record->opcode;
        if (opcode < 0x11b) {
            if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
            /* This sole consumer needs only signed column+4 < delay from
             * the12-byte Pentium rows. Other column values remain unmodeled. */
            if ((opcode >= 0x45 && opcode <= 0x48) ||
                (opcode >= 0x4b && opcode <= 0x4e) ||
                (opcode >= 0x51 && opcode <= 0x53) ||
                (opcode >= 0x56 && opcode <= 0x59) ||
                (opcode >= 0x5c && opcode <= 0x5e) ||
                (opcode >= 0x80 && opcode <= 0x84) || opcode == 0x8d ||
                (opcode >= 0xd1 && opcode <= 0xd3)) ++overlapped;
        }
    }
    /* Original48d3e0 is the exact double0.5; all integer counts convert
     * exactly, so the half-density comparison requires no private x87 ABI. */
    return overlapped > 0 && (double)floating > (double)total * 0.5;
}
void __fastcall reset_schedule_graph(C2ScheduleGraph *graph)
{
    C2FlowGraph *flow = &graph->flow;
    C2BasicBlock *last = flow->last, *first = flow->first, *block;
    C2FlowEdge *edge;
    /* Scheduling nodes use this same allocator/link prefix. None of the
     * ordinary flow block's larger analysis payload is accessed here. */
    for (block = first; block != last; block = block->next)
        for (edge = block->incoming; edge; edge = edge->next_incoming)
            release_flow_edge(flow, edge);
    for (block = first; block != last; block = block->next)
        recycle_flow_block(flow, block);
    if (last) recycle_flow_block(flow, last);
    flow->last = NULL;
    flow->first = NULL;
    flow->state08 = 0;
    flow->state0c = 0;
}
void __fastcall prepare_schedule_range(C2Expression *first, C2Expression *last);
void __fastcall initialize_schedule_graph_state(C2ScheduleGraph *graph);
C2ScheduleNode *__fastcall new_schedule_node(C2Expression *record);
void __fastcall fence_schedule_node(C2ScheduleNode *node);
int __fastcall schedule_record_is_serial(C2Expression *record);
void __fastcall add_schedule_record_dependencies(C2ScheduleNode *node, C2Expression *record);
void __fastcall add_schedule_control_dependencies(C2ScheduleNode *node, C2Expression *record, C2Expression *last);
C2ScheduleRegisterUse *__fastcall record_schedule_register_use(C2FunctionRegister *reg, C2ScheduleNode *node);
C2ScheduleRegisterUse *__fastcall record_schedule_register_write(C2FunctionRegister *reg, C2ScheduleNode *node);
C2ScheduleMemory *__fastcall record_schedule_memory_dependency(C2ScheduleMemory **head, C2ScheduleMemory **tail,
    C2Expression *operand, C2ScheduleNode *node);
C2ScheduleDependency *__fastcall new_schedule_dependency(C2ScheduleNode *source, C2ScheduleNode *destination,
    unsigned flags);
void __fastcall annotate_schedule_dependency(C2ScheduleDependency *edge);
void __fastcall note_volatile_schedule_node(C2ScheduleNode *node);
C2ScheduleRegisterAlias *__fastcall find_schedule_register_alias(C2FunctionRegister *reg);
C2ScheduleGraph *active_schedule_graph;       /*49308c */
C2ScheduleNode *schedule_fence, *schedule_first_node, *schedule_last_node; /*493110/3160/3164 */
C2ScheduleNode *schedule_volatile_node;        /*493148 */
C2ScheduleMemory *schedule_read_head, *schedule_read_tail;   /*49313c/3140 */
C2ScheduleMemory *schedule_write_head, *schedule_write_tail; /*493064/3080 */
unsigned schedule_node_count, schedule_edge_count;          /*493068/30a8 */
extern int optimizer_input_changed;

/* Only these initialized static kind bytes are known. No table padding may
 * silently stand in for a register class during scheduling. */
#define SCHEDULE_REGISTER_KIND(reg, result) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
    (reg) != (C2FunctionRegister *)&machine_registers[4] && \
    (reg) != (C2FunctionRegister *)&machine_registers[5] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
    (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[67] && \
        !((unsigned long)(reg) >= (unsigned long)&machine_registers[68] && \
          (unsigned long)(reg) < (unsigned long)&machine_registers[74])) { __asm { int 3 } } \
    (result) = (reg) == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ? \
        machine_register_storage.sentinel.kind : (reg)->kind; \
} while (0)

int __fastcall final_float_stack_effect(C2Expression *record);
void __fastcall prepare_schedule_range(C2Expression *first, C2Expression *last)
{
    C2Expression *record, *next, *end = last->next, *operand;
    C2FunctionRegister *reg, *original;
    int depth = 0, phase;
    unsigned kind;

    prepare_early_schedule_costs();
    for (record = first; record != end; record = next) {
        next = record->next;
        if (!(record->flags & 1) ||
            ((record->type & 0xf000) != 0x4000 && record->kind != 13)) continue;
        if (record->opcode == 0x5f) {
            unlink_input_record((C2Record *)record);
            release_small_node((C2SmallNode *)record);
            continue;
        }
        for (phase = 0; phase != 2; ++phase)
            for (operand = phase ? record->tail.right : record->left;
                 operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                reg = operand->value.reg;
                if (reg == (C2FunctionRegister *)&machine_registers[9] ||
                    reg == (C2FunctionRegister *)&machine_registers[17] ||
                    reg == (C2FunctionRegister *)&machine_registers[20] ||
                    reg == (C2FunctionRegister *)&machine_registers[31] ||
                    reg == (C2FunctionRegister *)&machine_registers[32] ||
                    reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
                else SCHEDULE_REGISTER_KIND(reg, kind);
                if (kind != 3 || !reg->value) continue;
                original = operand->original_register;
                if (original == (C2FunctionRegister *)&machine_registers[9] ||
                    original == (C2FunctionRegister *)&machine_registers[17] ||
                    original == (C2FunctionRegister *)&machine_registers[20] ||
                    original == (C2FunctionRegister *)&machine_registers[31] ||
                    original == (C2FunctionRegister *)&machine_registers[32] ||
                    original == (C2FunctionRegister *)&machine_registers[74]) kind = original->kind;
                else SCHEDULE_REGISTER_KIND(original, kind);
                if (kind != 1 || (operand->type & 0xf000) != 0x4000) continue;
                /* Only ST0/ST1 number fields are encountered in this view;
                 * other original rows are valid but unmodeled here. */
                if ((unsigned long)original >= (unsigned long)machine_registers &&
                    (unsigned long)original <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                    original != (C2FunctionRegister *)&machine_registers[31] &&
                    original != (C2FunctionRegister *)&machine_registers[32]) { __asm { int 3 } }
                operand->float_stack_index = (long)original->number - 31;
                operand->original_register = reg;
            }
        depth = (int)((unsigned)depth + (unsigned)final_float_stack_effect(record));
        if (depth > (int)schedule_early_base) schedule_early_base = (short)depth;
    }
    schedule_early_delta = (short)(8 - early_float_stack_base);
}

#define SCHEDULE_REGISTER_ROOT(reg, result) do { \
    if ((unsigned long)reg >= (unsigned long)machine_registers && \
    (unsigned long)reg < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
    reg != (C2FunctionRegister *)&machine_registers[0] && \
    reg != (C2FunctionRegister *)&machine_registers[1] && \
    reg != (C2FunctionRegister *)&machine_registers[2] && \
    reg != (C2FunctionRegister *)&machine_registers[3] && \
    reg != (C2FunctionRegister *)&machine_registers[4] && \
    reg != (C2FunctionRegister *)&machine_registers[5] && \
    reg != (C2FunctionRegister *)&machine_registers[6] && \
    reg != (C2FunctionRegister *)&machine_registers[7] && \
    reg != (C2FunctionRegister *)&machine_registers[8] && \
    reg != (C2FunctionRegister *)&machine_registers[18] && \
    reg != (C2FunctionRegister *)&machine_registers[66] && \
    reg != (C2FunctionRegister *)&machine_registers[67] && \
    !((unsigned long)reg >= (unsigned long)&machine_registers[68] && \
      (unsigned long)reg < (unsigned long)&machine_registers[74])) { __asm { int 3 } } \
    (result) = reg == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ? \
    machine_register_storage.sentinel.canonical : reg->canonical; \
} while (0)

#define SCHEDULE_REGISTER_RANGE(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[5] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[67] && \
        !((unsigned long)(reg) >= (unsigned long)&machine_registers[68] && \
          (unsigned long)(reg) < (unsigned long)&machine_registers[74])) { __asm { int 3 } } \
} while (0)

void __fastcall build_schedule_graph(C2ScheduleGraph *graph, C2Expression *first, C2Expression *last)
{
    C2Expression *record, *operand;
    C2FunctionRegister *reg;
    C2ScheduleNode *node;
    unsigned kind, number, volatile_access;

    initialize_schedule_graph_state(graph);
    schedule_fence = schedule_first_node = new_schedule_node(NULL);
    schedule_first_node->flags3a |= 1;
    schedule_volatile_node = NULL;
    for (record = first; record != last->next; record = record->next) {
        node = new_schedule_node(record);
        switch (record->kind) {
        case 14: case 16: case 18: case 19: case 20: case 21:
        case 22: case 23: case 24: case 25:
            fence_schedule_node(node);
        }
        if (record->opcode == 0x13c || record->opcode == 0x9c || record->opcode == 0xb2) {
            fence_schedule_node(node);
        } else if (record->flags & 1) {
            if (schedule_record_is_serial(record)) add_schedule_record_dependencies(node, record);
            add_schedule_control_dependencies(node, record, last);
            if (record->kind == 13) fence_schedule_node(node);
            volatile_access = 0;
            for (operand = record->left; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    reg = operand->original_register;
                    if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                        reg == (C2FunctionRegister *)&machine_registers[20] ||
                        reg == (C2FunctionRegister *)&machine_registers[9] ||
                        reg == (C2FunctionRegister *)&machine_registers[31] ||
                        reg == (C2FunctionRegister *)&machine_registers[32] ||
                        reg == (C2FunctionRegister *)&machine_registers[21] ||
                        reg == (C2FunctionRegister *)&machine_registers[33] ||
                        reg == (C2FunctionRegister *)&machine_registers[34] ||
                        reg == (C2FunctionRegister *)&machine_registers[35] ||
                        reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
                    else SCHEDULE_REGISTER_KIND(reg, kind);
                    if (kind == 2) goto input_memory;
                    record_schedule_register_use(reg, node);
                    break;
                case 2:
input_memory:
                    reg = operand->value.reg;
                    if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                        reg == (C2FunctionRegister *)&machine_registers[20] ||
                        reg == (C2FunctionRegister *)&machine_registers[9] ||
                        reg == (C2FunctionRegister *)&machine_registers[31] ||
                        reg == (C2FunctionRegister *)&machine_registers[32] ||
                        reg == (C2FunctionRegister *)&machine_registers[21] ||
                        reg == (C2FunctionRegister *)&machine_registers[33] ||
                        reg == (C2FunctionRegister *)&machine_registers[34] ||
                        reg == (C2FunctionRegister *)&machine_registers[35] ||
                        reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
                    else SCHEDULE_REGISTER_KIND(reg, kind);
                    volatile_access = (operand->location.expression.properties & 0x40) >> 6;
                    if ((kind == 3 || kind == 4 || kind == 5) && !(reg->view_flags & 2) &&
                        (!optimizer_input_changed || !find_schedule_register_alias(reg))) {
                        record_schedule_register_use(reg, node);
                        break;
                    }
                    goto input_unknown_memory;
                case 6: case 10:
input_unknown_memory:
                    volatile_access = operand->kind == 6 && (operand->location.expression.properties & 0x40);
                    record_schedule_memory_dependency(&schedule_read_head, &schedule_read_tail, operand, node);
                    break;
                case 9:
                    number = next_optimizer_symbol(operand->value.registers);
                    while (number != (unsigned)-1) {
                        if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
                        record_schedule_register_use((C2FunctionRegister *)&machine_registers[number], node);
                        number = next_optimizer_symbol(NULL);
                    }
                    break;
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    reg = operand->original_register;
                    if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                        reg == (C2FunctionRegister *)&machine_registers[20] ||
                        reg == (C2FunctionRegister *)&machine_registers[9] ||
                        reg == (C2FunctionRegister *)&machine_registers[31] ||
                        reg == (C2FunctionRegister *)&machine_registers[32] ||
                        reg == (C2FunctionRegister *)&machine_registers[21] ||
                        reg == (C2FunctionRegister *)&machine_registers[33] ||
                        reg == (C2FunctionRegister *)&machine_registers[34] ||
                        reg == (C2FunctionRegister *)&machine_registers[35] ||
                        reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
                    else SCHEDULE_REGISTER_KIND(reg, kind);
                    if (kind == 2) goto output_memory;
                    record_schedule_register_write(reg, node);
                    break;
                case 2:
output_memory:
                    reg = operand->value.reg;
                    if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                        reg == (C2FunctionRegister *)&machine_registers[20] ||
                        reg == (C2FunctionRegister *)&machine_registers[9] ||
                        reg == (C2FunctionRegister *)&machine_registers[31] ||
                        reg == (C2FunctionRegister *)&machine_registers[32] ||
                        reg == (C2FunctionRegister *)&machine_registers[21] ||
                        reg == (C2FunctionRegister *)&machine_registers[33] ||
                        reg == (C2FunctionRegister *)&machine_registers[34] ||
                        reg == (C2FunctionRegister *)&machine_registers[35] ||
                        reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
                    else SCHEDULE_REGISTER_KIND(reg, kind);
                    volatile_access = (operand->location.expression.properties & 0x40) >> 6;
                    if ((kind == 3 || kind == 4 || kind == 5) && !(reg->view_flags & 2) &&
                        (!optimizer_input_changed || !find_schedule_register_alias(reg))) {
                        record_schedule_register_write(reg, node);
                        break;
                    }
                    goto output_unknown_memory;
                case 6: case 10:
output_unknown_memory:
                    volatile_access = operand->kind == 6 && (operand->location.expression.properties & 0x40);
                    record_schedule_memory_dependency(&schedule_write_head, &schedule_write_tail, operand, node);
                    break;
                case 9:
                    number = next_optimizer_symbol(operand->value.registers);
                    while (number != (unsigned)-1) {
                        if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
                        record_schedule_register_write((C2FunctionRegister *)&machine_registers[number], node);
                        number = next_optimizer_symbol(NULL);
                    }
                    break;
                }
            }
            if (!node->incoming)
                annotate_schedule_dependency(new_schedule_dependency(schedule_fence, node, 0x80000));
            if (volatile_access) note_volatile_schedule_node(node);
        }
    }
    schedule_last_node = new_schedule_node(NULL);
    fence_schedule_node(schedule_last_node);
    active_schedule_graph->flow.last = (C2BasicBlock *)schedule_first_node;
    active_schedule_graph->flow.first = (C2BasicBlock *)schedule_last_node;
    active_schedule_graph->node_count = schedule_node_count;
    active_schedule_graph->edge_count = schedule_edge_count;
}
C2ScheduleRegisterUse *schedule_register_reads, *schedule_register_writes; /*49312c/307c */
unsigned schedule_state_493158, schedule_state_49315c; /*Actual DWORD resets; later role unclaimed. */
unsigned schedule_next_number;               /*493128 */
void __fastcall initialize_schedule_graph_state(C2ScheduleGraph *graph)
{
    active_schedule_graph = graph;
    schedule_register_writes = NULL;
    schedule_state_493158 = 0;
    schedule_register_reads = NULL;
    schedule_state_49315c = 0;
    schedule_next_number = 0;
    schedule_node_count = 0;
    schedule_edge_count = 0;
}
int __fastcall schedule_record_reads_memory(C2Expression *record);
int __fastcall schedule_record_writes_memory(C2Expression *record);
unsigned __fastcall schedule_record_resource_class(C2Expression *record);
unsigned __fastcall schedule_node_cost(C2ScheduleNode *node);
C2ScheduleNode *__fastcall new_schedule_node(C2Expression *record)
{
    C2ScheduleNode *node = (C2ScheduleNode *)prepend_flow_block(&active_schedule_graph->flow);
    node->number = (unsigned short)schedule_next_number++;
    node->record = record;
    if (record && (record->flags & 1)) {
        node->properties = (node->properties & ~0x20) | ((schedule_record_reads_memory(record) & 1) << 5);
        node->properties = (node->properties & ~0x40) | ((schedule_record_writes_memory(record) & 1) << 6);
        node->properties = (node->properties & ~7) | (schedule_record_resource_class(record) & 7);
        node->cost = (unsigned char)schedule_node_cost(node);
        node->properties &= 0x7f;
    } else node->cost = 0;
    node->outgoing_count = 0;
    node->incoming_count = 0;
    node->remaining_incoming = 0;
    if (schedule_input_mode) schedule_input_mode(node);
    ++schedule_node_count;
    return node;
}
int __fastcall schedule_record_reads_memory(C2Expression *record)
{
    C2Expression *operand;
    for (operand = record->left; operand; operand = operand->next)
        if (operand->kind == 6 || operand->kind == 2) return 1;
    return 0;
}
int __fastcall schedule_record_writes_memory(C2Expression *record)
{
    C2Expression *operand;
    for (operand = record->tail.right; operand; operand = operand->next)
        if (operand->kind == 6 || operand->kind == 2) return 1;
    return 0;
}
unsigned __fastcall schedule_record_resource_class(C2Expression *record)
{
    unsigned opcode = record->opcode;
    if (opcode >= 0x11b) return 0;
    /* Only encountered Pentium resource-class bytes are modeled; other
     * in-range rows and the neighboring property fields remain unavailable. */
    if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
    switch (opcode) {
    case 0x29: case 0x32: case 0x33: case 0x2d: case 0x21: /* Reached arithmetic: Pentium delay1/class0. */
    case 0x20: case 0xdf: /* Pentium full property0x300, low BYTE0. */
    case 1: return 0;
    case 0xf: return 0;
    case 0x30: return 0;
    case 0x34: return 0;
    case 0x1f: return 0;
    case 0x10: return 0;
    case 0xd: return 0;
    case 0xe: return 0;
    case 0x1e: return 0;
    case 0xc1: return 4;
    case 0x12: return 0;
    case 0x2f: return 0;
    case 0x2e: return 0;
    case 0x2b: return 0;
    case 0x2a: return 0;
    case 0xb9: return 0;
    case 0x8: return 0;
    case 0xba: return 0;
    case 0xfe: return 0; /* Reached Pentium row: low property BYTE0. */
    case 0x56: case 0x47: case 0x62: case 0x63: case 0x60: return 2;
    case 0x46: case 0x4b: case 0x4c: case 0x4d: case 0x5d: case 0x6e: return 2;
    case 0xae: return 0;
    case 0x45: return 2;
    case 0x77: return 2;
    case 0x4a: case 0x4f: return 2;
    case 0x5c: return 2;
    case 0x5f: return 2;
    default: __asm { int 3 }
    }
    return 0;
}
unsigned __fastcall schedule_node_operand_cost(C2ScheduleNode *node);
unsigned __fastcall schedule_node_cost(C2ScheduleNode *node)
{
    unsigned cost, opcode;
    if (!node->record) return 0;
    cost = schedule_node_operand_cost(node);
    opcode = node->record->opcode;
    if (opcode >= 0x11b) return cost;
    /* Base delays from the encountered Pentium rows only. Other in-range
     * rows remain unavailable in both reconstruction builds. */
    if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
    switch (opcode) {
    case 0x20: return cost + 22;
    case 0xdf: return cost + 2;
    case 0x29: case 0x32: case 0x33: case 0x2d: case 0x21: /* Reached arithmetic: Pentium delay1/class0. */
    case 1: return cost + 1;
    case 0xf: return cost + 1;
    case 0x30: return cost + 1;
    case 0x34: return cost + 1;
    case 0x1f: return cost + 17;
    case 0x10: return cost + 2;
    case 0xd: return cost + 1;
    case 0xe: return cost + 0;
    case 0x1e: return cost + 1;
    case 0xc1: return cost + 10;
    case 0x12: return cost + 1;
    case 0x2f: return cost + 1;
    case 0x2e: return cost + 1;
    case 0x2b: return cost + 1;
    case 0x2a: return cost + 1;
    case 0xb9: return cost + 1;
    case 0x8: return cost + 3;
    case 0xba: return cost + 1;
    case 0xfe: return cost + 13;
    case 0x56: case 0x47: return cost + 3;
    case 0x62: case 0x63: case 0x60: return cost + 1;
    case 0x46: case 0x4b: case 0x4c: case 0x4d: return cost + 3;
    case 0x5d: return cost + 4;
    case 0x6e: case 0xae: return cost + 2;
    case 0x45: return cost + 3;
    case 0x77: return cost + 1;
    case 0x4a: case 0x4f: return cost + 39;
    case 0x5c: return cost + 4;
    case 0x5f: return cost + 1;
    default: __asm { int 3 }
    }
    return cost;
}
unsigned __fastcall schedule_node_operand_cost(C2ScheduleNode *node)
{
    C2Expression *record = node->record, *operand;
    unsigned opcode = record->opcode, cost = 0, bytes;
    if (opcode >= 0x10b && opcode <= 0x11a)
        return (node->properties & 0x40) != 0;
    switch (opcode) {
    case 0x1f: case 0x20:
        bytes = record->tail.right->type & 0xfff;
        if (bytes == 2) return 8;
        if (bytes == 4) return 24;
        return 0;
    case 0xc: case 0xd: case 0x1e: case 0x21: case 0x23: case 0x24:
    case 0x2c: case 0x2d: case 0x2e: case 0x2f: case 0x30: case 0x31:
    case 0x32: case 0x33: case 0x34: case 0xb9: case 0xba:
        if (node->properties & 0x40) return 2;
        return (node->properties & 0x20) != 0;
    case 0x25: case 0x26:
        cost = node->properties & 0x40 ? 2 : 0;
        operand = record->left->next;
        if (operand->kind == 1) return cost + 6;
        if (operand->integer.low != 1 || operand->integer.high) return cost + 7;
        return cost;
    case 0x27: case 0x28: case 0x29: case 0x2a: case 0x2b:
        if (record->left->next->kind == 1) return 3;
        return node->properties & 0x40 ? 2 : 0;
    case 0x62: case 0x63: case 0xd9:
        return (node->properties & 0x40) != 0;
    case 0xcc:
        if ((node->properties & 0x40) && record->left->next->kind == 1) return 5;
        return 0;
    case 0xcd: case 0xce: case 0xcf:
        cost = (node->properties & 0x40) != 0;
        if (record->left->next->kind == 1) return cost + 5;
        return cost;
    }
    return 0;
}
extern C2ScheduleRegisterUse *free_schedule_register_uses;
void __fastcall release_schedule_register_use(C2ScheduleRegisterUse *use)
{
    C2ScheduleRegisterUse *previous = free_schedule_register_uses;
    free_schedule_register_uses = use;
    use->next = previous;
}
void __fastcall clear_schedule_register_uses(void)
{
    C2ScheduleRegisterUse *use, *next;
    for (use = schedule_register_reads; use; use = next) {
        next = use->next;
        release_schedule_register_use(use);
    }
    schedule_register_reads = NULL;
    for (use = schedule_register_writes; use; use = next) {
        next = use->next;
        release_schedule_register_use(use);
    }
    schedule_register_writes = NULL;
}
C2ScheduleMemory *free_schedule_memory_uses; /*493098, NULL initialized free head. */
void __fastcall release_schedule_memory_uses(C2ScheduleMemory *head, C2ScheduleMemory *tail)
{
    C2ScheduleMemory *previous;
    if (!head) return;
    previous = free_schedule_memory_uses;
    free_schedule_memory_uses = head;
    tail->next = previous;
}
void __fastcall clear_schedule_memory_uses(void)
{
    release_schedule_memory_uses(schedule_read_head, schedule_read_tail);
    schedule_read_tail = NULL;
    schedule_read_head = NULL;
    release_schedule_memory_uses(schedule_write_head, schedule_write_tail);
    schedule_write_tail = NULL;
    schedule_write_head = NULL;
}
void __fastcall fence_schedule_node(C2ScheduleNode *node)
{
    C2ScheduleNode *previous;
    for (previous = schedule_fence; previous; previous = previous->previous)
        if (!previous->outgoing && previous != node)
            new_schedule_dependency(previous, node, 0x80000);
    schedule_fence = node;
    node->flags3a |= 1;
    clear_schedule_register_uses();
    clear_schedule_memory_uses();
}
int __fastcall schedule_record_is_serial(C2Expression *record)
{
    C2Expression *input;
    if (record->opcode == 0x5f) return 0;
    if (record->opcode != 0x34) return 1;
    input = record->left;
    if (input->kind == 1 && input->next->kind == 1 &&
        input->original_register == input->next->original_register &&
        input->original_register != (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT))
        return 0;
    return 1;
}
void __fastcall add_schedule_register_dependencies(C2FunctionRegister *reg, C2ScheduleNode *node, unsigned flags);
void __fastcall add_schedule_memory_dependencies(C2Expression *operand, C2ScheduleNode *node, unsigned flags);
void __fastcall add_schedule_record_dependencies(C2ScheduleNode *node, C2Expression *record)
{
    C2Expression *operand;
    C2FunctionRegister *reg;
    unsigned kind, number;
    for (operand = record->left; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1:
            reg = operand->original_register;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
            else SCHEDULE_REGISTER_KIND(reg, kind);
            if (kind == 2) goto memory;
            add_schedule_register_dependencies(reg, node, 1);
            break;
        case 2:
memory:
            reg = operand->value.reg;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
            else SCHEDULE_REGISTER_KIND(reg, kind);
            if ((kind == 3 || kind == 4 || kind == 5) && !(reg->view_flags & 2) &&
                (!optimizer_input_changed || !find_schedule_register_alias(reg))) {
                add_schedule_register_dependencies(reg, node, 0x20);
                break;
            }
            /* Unknown-address memory shares the same genuine dependency path. */
        case 6: case 10:
            add_schedule_memory_dependencies(operand, node, 0x20);
            break;
        case 9:
            number = next_optimizer_symbol(operand->value.registers);
            while (number != (unsigned)-1) {
                if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
                add_schedule_register_dependencies((C2FunctionRegister *)&machine_registers[number], node, 1);
                number = next_optimizer_symbol(NULL);
            }
            break;
        }
    }
}
C2ScheduleDependency *__fastcall find_or_add_schedule_dependency(C2ScheduleNode *source,
    C2ScheduleNode *destination, unsigned flags);
void __fastcall set_schedule_register_overlap(C2ScheduleDependency *edge, C2FunctionRegister *written,
    C2FunctionRegister *used);
void __fastcall set_schedule_register_delay(C2FunctionRegister *reg, C2ScheduleDependency *edge);
int __fastcall schedule_register_fully_covers(C2FunctionRegister *written, C2FunctionRegister *used);
void __fastcall add_schedule_register_dependencies(C2FunctionRegister *reg, C2ScheduleNode *node, unsigned flags)
{
    C2ScheduleRegisterUse *use;
    C2ScheduleDependency *edge;
    C2FunctionRegister *root;
    for (use = schedule_register_writes; use; use = use->next) {
        if (!function_registers_overlap(use->reg, reg)) continue;
        if (flags == 1) {
            edge = find_or_add_schedule_dependency(use->node, node, 1);
            set_schedule_register_overlap(edge, use->reg, reg);
        } else if (flags == 4) {
            edge = find_or_add_schedule_dependency(use->node, node, 4);
            set_schedule_register_delay(reg, edge);
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) root = reg->canonical;
            else SCHEDULE_REGISTER_ROOT(reg, root);
            if (root == (C2FunctionRegister *)&machine_registers[66]) continue;
        } else if (flags == 0x20) {
            edge = find_or_add_schedule_dependency(use->node, node, 0x20);
            edge->delay = 1;
        } else edge = find_or_add_schedule_dependency(use->node, node, flags);
        annotate_schedule_dependency(edge);
        if (schedule_register_fully_covers(use->reg, reg)) return;
    }
}
C2ScheduleDependency *__fastcall find_schedule_dependency(C2ScheduleNode *source, C2ScheduleNode *destination);
C2ScheduleDependency *__fastcall find_or_add_schedule_dependency(C2ScheduleNode *source,
    C2ScheduleNode *destination, unsigned flags)
{
    C2ScheduleDependency *edge = find_schedule_dependency(source, destination);
    if (edge) {
        edge->flags |= flags;
        return edge;
    }
    return new_schedule_dependency(source, destination, flags);
}
C2ScheduleDependency *__fastcall find_schedule_dependency(C2ScheduleNode *source, C2ScheduleNode *destination)
{
    C2ScheduleDependency *edge;
    for (edge = destination->incoming; edge; edge = edge->next_incoming)
        if (edge->source == source) break;
    return edge;
}
int __fastcall schedule_dependency_is_zero_latency(C2FunctionRegister *written, C2ScheduleDependency *edge);
unsigned __fastcall schedule_dependency_operand_penalty(C2ScheduleDependency *edge, C2FunctionRegister *used);
unsigned __fastcall schedule_pentium_pair_penalty(C2ScheduleDependency *edge);
unsigned __fastcall schedule_partial_register_penalty(C2FunctionRegister *written, C2FunctionRegister *used);
void __fastcall set_schedule_register_overlap(C2ScheduleDependency *edge, C2FunctionRegister *written,
    C2FunctionRegister *used)
{
    unsigned cost = edge->source->cost;
    switch (schedule_cpu_index) {
    case 0: case 1:
        cost += schedule_dependency_operand_penalty(edge, used);
        cost += schedule_partial_register_penalty(written, used);
        break;
    case 2:
        if (schedule_dependency_is_zero_latency(written, edge)) cost = 0;
        else {
            cost += schedule_dependency_operand_penalty(edge, used);
            cost += schedule_pentium_pair_penalty(edge);
        }
        break;
    case 3:
        cost += schedule_partial_register_penalty(written, used);
        break;
    }
    if (edge->source->record->opcode == 0x138) edge->delay = 0;
    else if ((int)cost > (int)edge->delay) edge->delay = (unsigned short)cost;
}
int __fastcall schedule_dependency_is_zero_latency(C2FunctionRegister *written, C2ScheduleDependency *edge)
{
    C2ScheduleNode *source = edge->source, *destination = edge->destination;
    unsigned first, second;
    if (written != (C2FunctionRegister *)&machine_registers[5] || schedule_cpu_index == 3) return 0;
    first = source->record->opcode;
    second = destination->record->opcode;
    if ((first == 0xd || first == 0xba) && (second == 0xd || second == 0xba) &&
        !(source->properties & 0x20) && !(destination->properties & 0x20)) return 1;
    if ((first == 0xc || first == 0xb9) && (second == 0xc || second == 0xb9) &&
        !(source->properties & 0x40) && !(destination->properties & 0x40)) return 1;
    return 0;
}
unsigned __fastcall schedule_dependency_operand_penalty(C2ScheduleDependency *edge, C2FunctionRegister *used)
{
    C2Expression *destination = edge->destination->record;
    C2Expression *source, *operand;
    unsigned opcode;
    for (operand = destination->left; operand; operand = operand->next)
        if (operand->kind == 6 || operand->kind == 5) break;
    if (!operand)
        for (operand = destination->tail.right; operand; operand = operand->next)
            if (operand->kind == 6) break;
    if (operand) {
        if (operand->aggregate_second && operand->aggregate_second->original_register == used) return 1;
        return operand->aggregate_value && operand->aggregate_value->original_register == used;
    }
    source = edge->source->record;
    if (used != (C2FunctionRegister *)&machine_registers[5]) return 0;
    opcode = source->opcode;
    if (opcode == 0xd || opcode == 0xc || opcode == 0xba || opcode == 0xb9) return 0;
    opcode = destination->opcode;
    if (opcode != 0xd && opcode != 0xc && opcode != 0xba && opcode != 0xb9) return 0;
    if ((current_symbol->flags & 0x1c) == 4 && source->kind == 13) return 2;
    return 1;
}
unsigned __fastcall schedule_pentium_pair_penalty(C2ScheduleDependency *edge)
{
    unsigned first = edge->source->record->opcode;
    unsigned second = edge->destination->record->opcode;
    if (first == 0x5f || second == 0x5f) return 0;
    if (second != 0x62 && second != 0x63) return 0;
    return first == 0x45 || first == 0x47 || first == 0x46 || first == 0x60;
}
unsigned __fastcall schedule_partial_register_penalty(C2FunctionRegister *written, C2FunctionRegister *used)
{ __asm { int 3 } return 0; }
void __fastcall set_schedule_register_delay(C2FunctionRegister *reg, C2ScheduleDependency *edge)
{
    C2Expression *source, *destination;
    C2FunctionRegister *root;
    int minimum;
    /* The sole consumed signed-WORD column is1 for CPUs386/486/Pentium,
     * and0 for P6. No neighboring data or wider CPU domain is modeled. */
    if ((unsigned)schedule_cpu_index > 3) { __asm { int 3 } }
    minimum = schedule_cpu_index == 3 ? 0 : 1;
    source = edge->source->record;
    if (source->opcode == 0xe) return;
    destination = edge->destination->record;
    if (destination->opcode == 0x5f) return;
    if (reg == (C2FunctionRegister *)&machine_registers[17] ||
        reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) root = reg->canonical;
    else SCHEDULE_REGISTER_ROOT(reg, root);
    if (root == (C2FunctionRegister *)&machine_registers[66] || source->opcode == 0x138 ||
        schedule_dependency_is_zero_latency(reg, edge)) return;
    if (minimum > edge->delay) edge->delay = (unsigned short)minimum;
}
int __fastcall schedule_register_fully_covers(C2FunctionRegister *written, C2FunctionRegister *used)
{
    C2FunctionRegister *written_root, *used_root;
    unsigned written_kind, used_kind;
    long written_base, used_base, written_start, used_start;
    if (used == (C2FunctionRegister *)&machine_registers[17] ||
        used == (C2FunctionRegister *)&machine_registers[20] ||
                used == (C2FunctionRegister *)&machine_registers[9] ||
                used == (C2FunctionRegister *)&machine_registers[31] ||
                used == (C2FunctionRegister *)&machine_registers[32] ||
                used == (C2FunctionRegister *)&machine_registers[21] ||
                used == (C2FunctionRegister *)&machine_registers[33] ||
                used == (C2FunctionRegister *)&machine_registers[34] ||
                used == (C2FunctionRegister *)&machine_registers[35] ||
                used == (C2FunctionRegister *)&machine_registers[74]) used_root = used->canonical;
    else SCHEDULE_REGISTER_ROOT(used, used_root);
    if (written == (C2FunctionRegister *)&machine_registers[17] ||
        written == (C2FunctionRegister *)&machine_registers[20] ||
                written == (C2FunctionRegister *)&machine_registers[9] ||
                written == (C2FunctionRegister *)&machine_registers[31] ||
                written == (C2FunctionRegister *)&machine_registers[32] ||
                written == (C2FunctionRegister *)&machine_registers[21] ||
                written == (C2FunctionRegister *)&machine_registers[33] ||
                written == (C2FunctionRegister *)&machine_registers[34] ||
                written == (C2FunctionRegister *)&machine_registers[35] ||
                written == (C2FunctionRegister *)&machine_registers[74]) written_root = written->canonical;
    else SCHEDULE_REGISTER_ROOT(written, written_root);
    if (written_root == used_root) {
        if (written != (C2FunctionRegister *)&machine_registers[17] &&
            written != (C2FunctionRegister *)&machine_registers[20] &&
            written != (C2FunctionRegister *)&machine_registers[9] &&
            written != (C2FunctionRegister *)&machine_registers[31] &&
            written != (C2FunctionRegister *)&machine_registers[74]) SCHEDULE_REGISTER_RANGE(written);
        if (used != (C2FunctionRegister *)&machine_registers[17] &&
            used != (C2FunctionRegister *)&machine_registers[20] &&
            used != (C2FunctionRegister *)&machine_registers[9] &&
            used != (C2FunctionRegister *)&machine_registers[31] &&
            used != (C2FunctionRegister *)&machine_registers[74]) SCHEDULE_REGISTER_RANGE(used);
        return written->displacement <= used->displacement &&
            (long)(used->value_bytes + (unsigned)used->displacement) <=
            (long)(written->value_bytes + (unsigned)written->displacement);
    }
    if (!optimizer_input_changed) return 0;
    if (written == (C2FunctionRegister *)&machine_registers[17] ||
        written == (C2FunctionRegister *)&machine_registers[20] ||
                written == (C2FunctionRegister *)&machine_registers[9] ||
                written == (C2FunctionRegister *)&machine_registers[31] ||
                written == (C2FunctionRegister *)&machine_registers[32] ||
                written == (C2FunctionRegister *)&machine_registers[21] ||
                written == (C2FunctionRegister *)&machine_registers[33] ||
                written == (C2FunctionRegister *)&machine_registers[34] ||
                written == (C2FunctionRegister *)&machine_registers[35] ||
                written == (C2FunctionRegister *)&machine_registers[74]) written_kind = written->kind;
    else SCHEDULE_REGISTER_KIND(written, written_kind);
    if (written_kind < 3 || written_kind > 6) return 0;
    if (used == (C2FunctionRegister *)&machine_registers[17] ||
        used == (C2FunctionRegister *)&machine_registers[20] ||
                used == (C2FunctionRegister *)&machine_registers[9] ||
                used == (C2FunctionRegister *)&machine_registers[31] ||
                used == (C2FunctionRegister *)&machine_registers[32] ||
                used == (C2FunctionRegister *)&machine_registers[21] ||
                used == (C2FunctionRegister *)&machine_registers[33] ||
                used == (C2FunctionRegister *)&machine_registers[34] ||
                used == (C2FunctionRegister *)&machine_registers[35] ||
                used == (C2FunctionRegister *)&machine_registers[74]) used_kind = used->kind;
    else SCHEDULE_REGISTER_KIND(used, used_kind);
    if (used_kind < 3 || used_kind > 6) return 0;
    /* Address-register roots below use actual dynamic symbol/frame fields.
     * Static machine-row payloads for that distinct role remain unavailable. */
    if (((unsigned long)written_root >= (unsigned long)machine_registers &&
         (unsigned long)written_root <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) ||
        ((unsigned long)used_root >= (unsigned long)machine_registers &&
         (unsigned long)used_root <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))) { __asm { int 3 } }
    written_base = written_kind == 3 ? written_root->frame_displacement : written_root->symbol->parameter_offset;
    written_start = (long)((unsigned)written_base + (unsigned)written->displacement -
        (unsigned)written_root->displacement);
    used_base = used_kind == 3 ? used_root->frame_displacement : used_root->symbol->parameter_offset;
    used_start = (long)((unsigned)used_base + (unsigned)used->displacement - (unsigned)used_root->displacement);
    return written_start <= used_start &&
        (long)(used->value_bytes + (unsigned)used_start) <= (long)(written->value_bytes + (unsigned)written_start);
}
extern unsigned optimizer_original_limit;
int __fastcall schedule_memory_operands_match(C2Expression *written, C2Expression *used)
{ __asm { int 3 } return 0; }
int __fastcall schedule_operand_fully_covers(C2Expression *written, C2Expression *used)
{
    C2FunctionRegister *first, *second, *first_root, *second_root;
    unsigned first_kind, second_kind;
    if (written == used) return 1;
    switch (written->kind) {
    case 1:
        first = written->original_register;
        if (first == (C2FunctionRegister *)&machine_registers[9] ||
            first == (C2FunctionRegister *)&machine_registers[17] ||
            first == (C2FunctionRegister *)&machine_registers[20] ||
            first == (C2FunctionRegister *)&machine_registers[31] ||
            first == (C2FunctionRegister *)&machine_registers[32] ||
            first == (C2FunctionRegister *)&machine_registers[74]) first_kind = first->kind;
        else SCHEDULE_REGISTER_KIND(first, first_kind);
        if (first_kind == 2) break;
        if (used->kind != 1) return 0;
        second = used->original_register;
        if (first == (C2FunctionRegister *)&machine_registers[9] ||
            first == (C2FunctionRegister *)&machine_registers[17] ||
            first == (C2FunctionRegister *)&machine_registers[20] ||
            first == (C2FunctionRegister *)&machine_registers[31] ||
            first == (C2FunctionRegister *)&machine_registers[32] ||
            first == (C2FunctionRegister *)&machine_registers[74]) first_root = first->canonical;
        else SCHEDULE_REGISTER_ROOT(first, first_root);
        if (second == (C2FunctionRegister *)&machine_registers[9] ||
            second == (C2FunctionRegister *)&machine_registers[17] ||
            second == (C2FunctionRegister *)&machine_registers[20] ||
            second == (C2FunctionRegister *)&machine_registers[31] ||
            second == (C2FunctionRegister *)&machine_registers[32] ||
            second == (C2FunctionRegister *)&machine_registers[74]) second_root = second->canonical;
        else SCHEDULE_REGISTER_ROOT(second, second_root);
        if (first_root != second_root) return 0;
        if (first != (C2FunctionRegister *)&machine_registers[9] &&
            first != (C2FunctionRegister *)&machine_registers[17] &&
            first != (C2FunctionRegister *)&machine_registers[20] &&
            first != (C2FunctionRegister *)&machine_registers[74]) SCHEDULE_REGISTER_RANGE(first);
        if (second != (C2FunctionRegister *)&machine_registers[9] &&
            second != (C2FunctionRegister *)&machine_registers[17] &&
            second != (C2FunctionRegister *)&machine_registers[20] &&
            second != (C2FunctionRegister *)&machine_registers[74]) SCHEDULE_REGISTER_RANGE(second);
        return first->displacement <= second->displacement &&
            (long)((unsigned)first->displacement + first->value_bytes) >=
            (long)((unsigned)second->displacement + second->value_bytes);
    case 2:
        break;
    case 6:
        if (used->kind != 6 || written->immediate != used->immediate ||
            written->immediate < optimizer_original_limit) return 0;
        return schedule_memory_operands_match(written, used);
    default:
        return 0;
    }
    if (used->kind != 2) {
        if (used->kind != 1) return 0;
        second = used->original_register;
        SCHEDULE_REGISTER_KIND(second, second_kind);
        if (second_kind != 2) return 0;
    }
    return schedule_register_fully_covers(written->value.reg, used->value.reg);
}
void __fastcall add_schedule_memory_dependencies(C2Expression *operand, C2ScheduleNode *node, unsigned flags)
{
    C2ScheduleMemory *use;
    for (use = schedule_write_head; use; use = use->next) {
        if (operand->kind == 2 && schedule_operand_fully_covers(use->operand, operand)) {
            find_or_add_schedule_dependency(use->node, node, flags);
            return;
        }
        if (input_operands_may_alias(use->operand, operand))
            find_or_add_schedule_dependency(use->node, node, flags);
    }
}
int __fastcall output_dependency_is_redundant(C2Expression *operand, C2Expression *record, C2Expression *last);
void __fastcall add_schedule_anti_dependencies(C2FunctionRegister *reg, C2ScheduleNode *node, unsigned flags);
void __fastcall add_schedule_memory_anti_dependencies(C2Expression *operand, C2ScheduleNode *node, unsigned flags);
void __fastcall add_schedule_control_dependencies(C2ScheduleNode *node, C2Expression *record, C2Expression *last)
{
    C2Expression *operand;
    C2FunctionRegister *reg;
    unsigned kind, number;
    for (operand = record->tail.right; operand; operand = operand->next) {
        switch (operand->kind) {
        case 1:
            reg = operand->original_register;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
            else SCHEDULE_REGISTER_KIND(reg, kind);
            if (kind == 2) goto memory;
            if (!output_dependency_is_redundant(operand, record, last))
                add_schedule_register_dependencies(reg, node, 4);
            add_schedule_anti_dependencies(reg, node, 2);
            break;
        case 2:
memory:
            reg = operand->value.reg;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) kind = reg->kind;
            else SCHEDULE_REGISTER_KIND(reg, kind);
            if ((kind == 3 || kind == 4 || kind == 5) && !(reg->view_flags & 2) &&
                (!optimizer_input_changed || !find_schedule_register_alias(reg))) {
                add_schedule_register_dependencies(reg, node, 0x80);
                add_schedule_anti_dependencies(reg, node, 0x40);
                break;
            }
        case 6: case 10:
            add_schedule_memory_dependencies(operand, node, 0x80);
            add_schedule_memory_anti_dependencies(operand, node, 0x40);
            break;
        case 9:
            number = next_optimizer_symbol(operand->value.registers);
            while (number != (unsigned)-1) {
                if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
                reg = (C2FunctionRegister *)&machine_registers[number];
                add_schedule_register_dependencies(reg, node, 4);
                add_schedule_anti_dependencies(reg, node, 2);
                number = next_optimizer_symbol(NULL);
            }
            break;
        }
    }
}
int __fastcall output_dependency_is_redundant(C2Expression *operand, C2Expression *record, C2Expression *last)
{
    C2Expression *scan, *value;
    C2FunctionRegister *reg, *root;
    C2FunctionRegister *flags = (C2FunctionRegister *)&machine_registers[66];
    if (operand->original_register != flags || operand->value.reg != flags) return 0;
    for (scan = record->next; scan; scan = scan->next) {
        if (scan == last->next || scan->kind == 16 || scan->kind == 24 || scan->kind == 25)
            return 0;
        for (value = scan->left; value; value = value->next) {
            if (value->kind != 1) continue;
            reg = value->original_register;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) root = reg->canonical;
            else SCHEDULE_REGISTER_ROOT(reg, root);
            if (root == flags) return 0;
        }
        for (value = scan->tail.right; value; value = value->next) {
            if (value->kind != 1) continue;
            reg = value->original_register;
            if (reg == (C2FunctionRegister *)&machine_registers[17] ||
                reg == (C2FunctionRegister *)&machine_registers[20] ||
                reg == (C2FunctionRegister *)&machine_registers[9] ||
                reg == (C2FunctionRegister *)&machine_registers[31] ||
                reg == (C2FunctionRegister *)&machine_registers[32] ||
                reg == (C2FunctionRegister *)&machine_registers[21] ||
                reg == (C2FunctionRegister *)&machine_registers[33] ||
                reg == (C2FunctionRegister *)&machine_registers[34] ||
                reg == (C2FunctionRegister *)&machine_registers[35] ||
                reg == (C2FunctionRegister *)&machine_registers[74]) root = reg->canonical;
            else SCHEDULE_REGISTER_ROOT(reg, root);
            if (root == flags) return 1;
        }
    }
    return 0;
}
void __fastcall add_schedule_anti_dependencies(C2FunctionRegister *reg, C2ScheduleNode *node, unsigned flags)
{
    C2ScheduleRegisterUse *use;
    for (use = schedule_register_reads; use; use = use->next)
        if (function_registers_overlap(use->reg, reg))
            annotate_schedule_dependency(find_or_add_schedule_dependency(use->node, node, flags));
}
void __fastcall add_schedule_memory_anti_dependencies(C2Expression *operand, C2ScheduleNode *node, unsigned flags)
{
    C2ScheduleMemory *use;
    for (use = schedule_read_head; use; use = use->next)
        if (input_operands_may_alias(use->operand, operand))
            find_or_add_schedule_dependency(use->node, node, flags);
}
C2ScheduleRegisterUse *__fastcall prepend_schedule_register_use(C2ScheduleRegisterUse **head,
    C2FunctionRegister *reg, C2ScheduleNode *node);
C2ScheduleRegisterUse *__fastcall record_schedule_register_use(C2FunctionRegister *reg, C2ScheduleNode *node)
{
    return prepend_schedule_register_use(&schedule_register_reads, reg, node);
}
C2ScheduleRegisterUse *__fastcall new_schedule_register_use(C2FunctionRegister *reg, C2ScheduleNode *node);
C2ScheduleRegisterUse *__fastcall prepend_schedule_register_use(C2ScheduleRegisterUse **head,
    C2FunctionRegister *reg, C2ScheduleNode *node)
{
    C2ScheduleRegisterUse *use = new_schedule_register_use(reg, node);
    C2ScheduleRegisterUse *previous = *head;
    *head = use;
    use->next = previous;
    return use;
}
/*493118: actual initialized NULL free list, shared only with the genuine
 * register-use cleanup. Reuse writes all three fields of the proved12B node. */
C2ScheduleRegisterUse *free_schedule_register_uses;
C2ScheduleRegisterUse *__fastcall new_schedule_register_use(C2FunctionRegister *reg, C2ScheduleNode *node)
{
    C2ScheduleRegisterUse *use = free_schedule_register_uses;
    if (use) free_schedule_register_uses = use->next;
    else use = c2_pool_allocate(0, sizeof(C2ScheduleRegisterUse));
    use->reg = reg;
    use->node = node;
    use->next = NULL;
    return use;
}
C2ScheduleRegisterUse *__fastcall record_schedule_register_write(C2FunctionRegister *reg, C2ScheduleNode *node)
{
    return prepend_schedule_register_use(&schedule_register_writes, reg, node);
}
C2ScheduleMemory *__fastcall new_schedule_memory_use(C2Expression *operand, C2ScheduleNode *node)
{
    C2ScheduleMemory *use = free_schedule_memory_uses;
    if (use) free_schedule_memory_uses = use->next;
    else use = c2_pool_allocate(0, sizeof(C2ScheduleMemory));
    use->operand = operand;
    use->node = node;
    use->next = NULL;
    return use;
}
C2ScheduleMemory *__fastcall record_schedule_memory_dependency(C2ScheduleMemory **head, C2ScheduleMemory **tail,
    C2Expression *operand, C2ScheduleNode *node)
{
    C2ScheduleMemory *use = new_schedule_memory_use(operand, node), *previous = *head;
    *head = use;
    use->next = previous;
    if (!*tail) *tail = use;
    return use;
}
C2ScheduleDependency *__fastcall new_schedule_dependency(C2ScheduleNode *source, C2ScheduleNode *destination,
    unsigned flags)
{
    C2ScheduleDependency *edge = (C2ScheduleDependency *)new_flow_edge(&active_schedule_graph->flow,
        (C2BasicBlock *)source, (C2BasicBlock *)destination);
    edge->flags = flags;
    ++source->outgoing_count;
    ++destination->incoming_count;
    edge->properties &= 0xe0;
    edge->delay = 0;
    ++schedule_edge_count;
    if (schedule_output_mode) schedule_output_mode(source, destination, edge, flags);
    return edge;
}
void __fastcall annotate_schedule_dependency(C2ScheduleDependency *edge)
{
    C2Expression *destination = edge->destination->record;
    C2Expression *source = edge->source->record, *operand;
    C2FunctionRegister *reg;
    unsigned kind;
    if (!schedule_aggregate_return || schedule_cpu_index == 3 || edge->delay >= 2 ||
        !source || source->kind != 13 || !source->detail.data) return;
    for (operand = destination->left; operand; operand = operand->next) {
        if (operand->kind != 2) continue;
        reg = operand->value.reg;
        SCHEDULE_REGISTER_KIND(reg, kind);
        if (kind >= 3 && kind <= 6) goto delay;
    }
    for (operand = destination->tail.right; operand; operand = operand->next) {
        if (operand->kind != 2) continue;
        reg = operand->value.reg;
        SCHEDULE_REGISTER_KIND(reg, kind);
        if (kind >= 3 && kind <= 6) goto delay;
    }
    return;
delay:
    edge->delay = 2;
}
void __fastcall note_volatile_schedule_node(C2ScheduleNode *node)
{ __asm { int 3 } }
C2ScheduleRegisterAlias *__fastcall find_schedule_register_alias(C2FunctionRegister *reg)
{
    C2FunctionRegister *root = reg->canonical;
    C2FrameSlot *slot;
    long offset = root->kind == 3 ? root->frame_displacement : root->symbol->parameter_offset;
    unsigned bytes = root->value_bytes;

    for (slot = frame_slots; slot; slot = slot->next)
        if (offset < (long)((unsigned)slot->offset + slot->bytes) &&
            slot->offset < (long)((unsigned)offset + bytes)) return slot->uses;
    return NULL;
}
/* A frame-slot alias may name another register with the requested use. */
int __fastcall optimizer_register_may_alias_index(C2FunctionRegister *reg, unsigned index)
{
    C2ScheduleRegisterAlias *alias;
    if (register_may_alias_index(reg, index)) return 1;
    switch (reg->kind) {
    case 3: case 4: case 5: case 6:
        for (alias = find_schedule_register_alias(reg); alias; alias = alias->next)
            if (register_may_alias_index(alias->reg, index)) return 1;
        break;
    }
    return 0;
}
/* This path walker consumes only these independently recovered canonical
 * pointers. Kind, range and other columns retain their separate guards. */
#define PATH_REGISTER_ROOT(reg, result) do { \
    if ((reg) == (C2FunctionRegister *)&machine_registers[9] || \
        (reg) == (C2FunctionRegister *)&machine_registers[17] || \
        (reg) == (C2FunctionRegister *)&machine_registers[19] || \
        (reg) == (C2FunctionRegister *)&machine_registers[20] || \
        (reg) == (C2FunctionRegister *)&machine_registers[21] || \
        (reg) == (C2FunctionRegister *)&machine_registers[31] || \
        (reg) == (C2FunctionRegister *)&machine_registers[32] || \
        (reg) == (C2FunctionRegister *)&machine_registers[33] || \
        (reg) == (C2FunctionRegister *)&machine_registers[34] || \
        (reg) == (C2FunctionRegister *)&machine_registers[35] || \
        (reg) == (C2FunctionRegister *)&machine_registers[74]) (result) = (reg)->canonical; \
    else SCHEDULE_REGISTER_ROOT(reg, result); \
} while (0)
void __fastcall prepare_schedule_paths(C2ScheduleNode *last, C2ScheduleNode *first)
{
    C2ScheduleNode *node = first->next, *source;
    C2Expression *record = node->record, *input, *output;
    C2ScheduleDependency *edge;
    C2FunctionRegister *input_root, *output_root;
    unsigned flags;
    switch (record->opcode) {
    case 0xf: case 0x16: case 0x102: case 0x10a:
        break;
    default:
        return;
    }
    for (edge = node->incoming; edge; edge = edge->next_incoming) {
        flags = edge->flags;
        if (flags & 1) {
            edge->source->properties |= 0x10;
            continue;
        }
        if (!(flags & 0x80000)) continue;
        source = edge->source;
        input = record->left->next;
        for (output = source->record->tail.right; output; output = output->next) {
            if (input->kind == 1 && output->kind == 1) {
                PATH_REGISTER_ROOT(input->original_register, input_root);
                PATH_REGISTER_ROOT(output->original_register, output_root);
                if (input_root != output_root) continue;
            } else if (input->kind == 2 && output->kind == 2) {
                PATH_REGISTER_ROOT(input->value.reg, input_root);
                PATH_REGISTER_ROOT(output->value.reg, output_root);
                if (input_root != output_root) continue;
            } else continue;
            edge->delay = 1;
            source->properties |= 0x10;
            edge->flags = flags | 1;
            break;
        }
    }
}

void __fastcall remove_schedule_dependency(C2ScheduleDependency *edge);
void __fastcall prepare_schedule_move_dependency(C2ScheduleNode *node, C2FunctionRegister *reg)
{
    C2ScheduleDependency *edge, *next, *incoming;
    C2ScheduleNode *destination;
    C2Expression *record, *input;

    for (edge = node->outgoing; edge; edge = next) {
        destination = edge->destination;
        next = edge->next_outgoing;
        record = destination->record;
        if (!record || record->kind == 13 || (record->type & 0xf000) != 0x4000)
            continue;
        input = record->left;
        if (!input) continue;
        if (input->kind == 1 && input->value.reg == reg) continue;
        input = input->next;
        if (input && input->kind == 1 && input->value.reg == reg) continue;
        remove_schedule_dependency(edge);
        for (incoming = node->incoming; incoming; incoming = incoming->next_incoming)
            find_or_add_schedule_dependency(incoming->source, destination, incoming->flags);
    }
}
void __fastcall prepare_schedule_dependencies(C2ScheduleNode *last, C2ScheduleNode *first)
{
    C2ScheduleNode *node, *following;
    C2Expression *record, *output, *input;
    C2FunctionRegister *root, *input_root;
    C2ScheduleDependency *edge;
    int found;
    if (schedule_final_mode || !schedule_function_flag8) return;
    for (node = last; node != first; node = node->previous) {
        record = node->record;
        if (!record || !(record->flags & 1)) continue;
        output = record->tail.right;
        if (!output || (output->type & 0xf000) != 0x4000 || output->kind != 1 || record->opcode == 0xe) continue;
        PATH_REGISTER_ROOT(output->value.reg, root);
        if (record->opcode == 0x138) {
            prepare_schedule_move_dependency(node, root);
            continue;
        }
        found = 0;
        for (following = node->previous; following != first; following = following->previous) {
            record = following->record;
            if ((record->type & 0xf000) != 0x4000 && record->kind != 13) continue;
            for (input = record->left; input; input = input->next) {
                if (input->kind != 1) continue;
                PATH_REGISTER_ROOT(input->value.reg, input_root);
                if (input_root != root) continue;
                edge = find_or_add_schedule_dependency(node, following, 1);
                set_schedule_register_overlap(edge, root, root);
                found = 1;
                break;
            }
            if (found) break;
        }
    }
}

C2Expression *__fastcall find_schedule_register_address(C2Expression *record,
    C2FunctionRegister *reg);
C2Expression *__fastcall find_schedule_address_operand(C2Expression *record, C2Expression *other,
    C2FunctionRegister **reg)
{
    C2Expression *output, *address, *input, *scan;
    C2FunctionRegister *root;
    C2FunctionRegister *flags = (C2FunctionRegister *)&machine_registers[66];

    *reg = NULL;
    for (output = record->tail.right; output; output = output->next) {
        if (output->kind == 6) return NULL;
        if (output->kind != 1) continue;
        if (output->original_register == (C2FunctionRegister *)&machine_registers[17] ||
            output->original_register == (C2FunctionRegister *)&machine_registers[20]) root = output->original_register->canonical;
        else PATH_REGISTER_ROOT(output->original_register, root);
        if (root != flags) {
            *reg = output->original_register;
            break;
        }
    }
    if (!*reg) return NULL;
    address = find_schedule_register_address(other, *reg);
    if (!address) return NULL;
    input = other->left;
    for (scan = input; scan; scan = scan->next) {
        if (scan->kind != 1) continue;
        if (scan->original_register == (C2FunctionRegister *)&machine_registers[17] ||
            scan->original_register == (C2FunctionRegister *)&machine_registers[20]) root = scan->original_register->canonical;
        else PATH_REGISTER_ROOT(scan->original_register, root);
        if (root == flags) return NULL;
    }
    if (input->kind == 1) {
        if (input->original_register == (C2FunctionRegister *)&machine_registers[17] ||
            input->original_register == (C2FunctionRegister *)&machine_registers[20]) root = input->original_register->canonical;
        else PATH_REGISTER_ROOT(input->original_register, root);
        if (root == *reg) return NULL;
    }
    input = input->next;
    if (input->kind != 1) return address;
    if (input->original_register == (C2FunctionRegister *)&machine_registers[17] ||
            input->original_register == (C2FunctionRegister *)&machine_registers[20]) root = input->original_register->canonical;
        else PATH_REGISTER_ROOT(input->original_register, root);
    if (root != *reg || input == address->aggregate_value ||
        input == address->aggregate_second) return address;
    return NULL;
}
#undef PATH_REGISTER_ROOT

C2Expression *__fastcall find_schedule_register_address(C2Expression *record,
    C2FunctionRegister *reg)
{
    C2Expression *address;
    for (address = record->left; address; address = address->next)
        if (address->kind == 6) break;
    if (!address) {
        for (address = record->tail.right; address; address = address->next)
            if (address->kind == 6) break;
    }
    if (!address) return NULL;
    if (address->aggregate_second && address->aggregate_second->original_register == reg)
        return address;
    if (address->aggregate_value && address->aggregate_value->original_register == reg)
        return address;
    return NULL;
}
/* The original target is a defined false leaf: XOR EAX,EAX; RET8.
 * Its four-argument fastcall role retains the two stack slots at each caller. */
int __fastcall test_schedule_dependency(C2ScheduleNode *source, C2ScheduleDependency *edge,
    C2FunctionRegister *reg, C2FunctionRegister *flags)
{
    return 0;
}
#define SCHEDULE_REGISTER_NUMBER(reg) do { \
    if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } \
} while (0)
int __fastcall adjust_schedule_dependency(C2ScheduleDependency *edge)
{
    C2ScheduleNode *source = edge->source, *destination = edge->destination;
    C2Expression *first = source->record, *second = destination->record;
    C2Expression *address, *operand, *literal;
    C2FunctionRegister *reg, *flags;
    C2ScheduleDependency *incoming, *added;
    unsigned value, shift, properties, marker;
    long displacement;
    int result;
    if ((source->flags3a & 1) || (destination->flags3a & 1)) return 0;
    if (!(edge->flags & 1)) goto anti_dependency;
    flags = (C2FunctionRegister *)&machine_registers[66];
    switch (first->opcode) {
    case 0x2d:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address) return 0;
        literal = first->left->next;
        if (literal->kind == 7) edge->properties = (edge->properties & 0xe1) | 1;
        else {
            if (literal->kind != 1) return 0;
            if (address->opcode != 0x122 &&
                (address->opcode != 0x125 || (address->location.expression.properties & 15))) return 0;
            edge->properties = (edge->properties & 0xe2) | 2;
        }
        break;
    case 0x32:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address) return 0;
        operand = first->tail.right;
        if (operand->kind == 1) {
            SCHEDULE_REGISTER_NUMBER(operand->original_register);
            if (operand->original_register->number == 5) return 0;
        }
        if (first->left->next->kind != 7) return 0;
        edge->properties = (edge->properties & 0xe3) | 3;
        break;
    case 0x21:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address) return 0;
        edge->properties = (edge->properties & 0xeb) | 0xb;
        break;
    case 0x1e:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address) return 0;
        operand = first->tail.right;
        if (operand->kind == 1) {
            SCHEDULE_REGISTER_NUMBER(operand->original_register);
            if (operand->original_register->number == 5) return 0;
        }
        edge->properties = (edge->properties & 0xec) | 0xc;
        break;
    case 0x22:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address || first->left->next->kind != 7) return 0;
        value = first->left->next->integer.low;
        if (!address->aggregate_second && (value == 1 || value == 2 || value == 4 || value == 8))
            edge->properties = (edge->properties & 0xe4) | 4;
        else {
            if (address->aggregate_value) return 0;
            value *= 1U << (address->location.expression.properties & 15);
            if (value == 1 || value == 2 || value == 4 || value == 8)
                edge->properties = (edge->properties & 0xe4) | 4;
        }
        break;
    case 0x12:
        address = find_schedule_address_operand(first, second, &reg);
        if (!address || address->tail.right) return 0;
        for (operand = first->left; operand; operand = operand->next)
            if (operand->kind == 5) break;
        if (!operand) return 0;
        if (address->opcode == 0x122 ||
            (address->opcode == 0x125 && !(address->location.expression.properties & 15))) {
            edge->properties = (edge->properties & 0xe5) | 5;
        } else if (address->aggregate_value && address->aggregate_value->original_register == reg) {
            if (operand->opcode != 0x122 &&
                (operand->opcode != 0x125 || (operand->location.expression.properties & 15))) return 0;
            edge->properties = (edge->properties & 0xe7) | 7;
        } else {
            if (operand->opcode == 0x122) shift = 0;
            else if (operand->opcode == 0x125) shift = operand->location.expression.properties & 15;
            else shift = 99;
            shift += address->location.expression.properties & 15;
            if (shift > 3) return 0;
            edge->properties = (edge->properties & 0xe9) | 9;
        }
        flags = NULL;
        break;
    default:
        return 0;
    }
    result = test_schedule_dependency(source, edge, reg, flags);
    properties = edge->properties;
    marker = ((unsigned)result & 1) << 5;
    properties = (properties & 0xdf) | marker;
    edge->properties = (unsigned char)properties;
    if (!result && (edge->flags & 6)) {
        if (first->opcode == 0x12) {
            operand = first->left;
            if ((!operand->aggregate_value || operand->aggregate_value->original_register != reg) &&
                (!operand->aggregate_second || operand->aggregate_second->original_register != reg)) {
                added = new_schedule_dependency(source, destination, edge->flags & ~1U);
                if (added->flags & 4) added->delay = 1;
                ++edge->destination->remaining_incoming;
                edge->flags = 1;
                edge->properties = (unsigned char)(((properties + 1U ^ properties) & 31) ^ properties);
                return 1;
            }
        }
        edge->properties = (unsigned char)(properties & 0xe0);
        return 0;
    }
    for (incoming = source->incoming; incoming; incoming = incoming->next_incoming) {
        if (incoming->flags & 1) {
            if (!find_schedule_dependency(incoming->source, destination)) {
                added = new_schedule_dependency(incoming->source, destination, 1);
                added->delay = edge->delay;
                continue;
            }
        }
        if (incoming->flags & 0x80000)
            find_or_add_schedule_dependency(incoming->source, destination, 0x80000);
    }
    ++edge->destination->remaining_incoming;
    return 1;

anti_dependency:
    if (!(edge->flags & 2)) return 0;
    switch (second->opcode) {
    case 0x2d:
        address = find_schedule_address_operand(second, first, &reg);
        if (!address) return 0;
        operand = second->tail.right;
        if (operand->kind == 1) {
            SCHEDULE_REGISTER_NUMBER(operand->original_register);
            if (operand->original_register->number == 5) return 0;
        }
        if (second->left->next->kind != 7) return 0;
        displacement = (long)(0U - second->left->next->integer.low);
        edge->properties = (edge->properties & 0xf0) | 0x10;
        break;
    case 0x32:
        address = find_schedule_address_operand(second, first, &reg);
        if (!address || second->left->next->kind != 7) return 0;
        displacement = (long)second->left->next->integer.low;
        edge->properties = (edge->properties & 0xf1) | 0x11;
        break;
    case 0x12:
        address = find_schedule_address_operand(second, first, &reg);
        if (!address) return 0;
        for (operand = second->left; operand; operand = operand->next)
            if (operand->kind == 5) break;
        if (!operand || operand->tail.right) return 0;
        if (operand->opcode == 0x122 && operand->aggregate_value->original_register == reg) {
            displacement = (long)(0U - (unsigned)operand->detail.aggregate_displacement);
            edge->properties = (edge->properties & 0xf3) | 0x13;
        } else {
            if (operand->opcode != 0x125 || operand->aggregate_second->original_register != reg ||
                (operand->location.expression.properties & 15)) return 0;
            displacement = (long)(0U - (unsigned)operand->detail.aggregate_displacement);
            edge->properties = (edge->properties & 0xf4) | 0x14;
        }
        break;
    case 0x21:
        address = find_schedule_address_operand(second, first, &reg);
        if (!address) return 0;
        operand = second->tail.right;
        if (operand->kind == 1) {
            SCHEDULE_REGISTER_NUMBER(operand->original_register);
            if (operand->original_register->number == 5) return 0;
        }
        displacement = -1;
        edge->properties = (edge->properties & 0xf5) | 0x15;
        break;
    case 0x1e:
        address = find_schedule_address_operand(second, first, &reg);
        if (!address) return 0;
        displacement = 1;
        edge->properties = (edge->properties & 0xf6) | 0x16;
        break;
    default:
        return 0;
    }
    if (reg == (C2FunctionRegister *)&machine_registers[5] &&
        (long)((unsigned)address->detail.aggregate_displacement + (unsigned)displacement) < 0) return 0;
    if (!c2_favor_speed && (((edge->properties & 31) == 0x12) ||
        (!address->tail.right && !address->detail.aggregate_displacement && displacement))) {
        edge->properties &= 0xe0;
        return 0;
    }
    if (edge->flags & 4) {
        edge->properties &= 0xe0;
        return 0;
    }
    ++destination->remaining_incoming;
    edge->properties &= 0xdf;
    return 1;
}

void __fastcall prepare_schedule_ready_node(C2ScheduleNode *node)
{
    C2ScheduleNode *previous = node->previous;
    if (!previous) {
        c2_report_internal("E:\\utc\\src\\\\P2\\dag.c", 0x34f);
        return;
    }
    do {
        if (previous->flags3a & 1) {
            find_or_add_schedule_dependency(previous, node, 0x80000);
            return;
        }
        previous = previous->previous;
    } while (previous);
    c2_report_internal("E:\\utc\\src\\\\P2\\dag.c", 0x34f);
}
unsigned __fastcall scale_schedule_priority(unsigned value, int shift)
{
    if (shift < 0) return value >> ((0U - (unsigned)shift) & 31);
    return value << ((unsigned)shift & 31);
}
void __fastcall order_schedule_graph(C2ScheduleGraph *graph)
{
    C2ScheduleNode *last = (C2ScheduleNode *)graph->flow.last;
    C2ScheduleNode *first = (C2ScheduleNode *)graph->flow.first, *node;
    C2ScheduleDependency *edge;
    unsigned length, next, priority;
    unsigned char properties;
    prepare_schedule_paths(last, first);
    prepare_schedule_dependencies(last, first);
    first->path_length = 0;
    for (node = first->next; node != last->next; node = node->next) {
        length = 0;
        for (edge = node->outgoing; edge; edge = edge->next_outgoing) {
            next = (unsigned)edge->delay + edge->destination->path_length;
            if (length < next) length = next;
        }
        node->path_length = (unsigned short)(length + 1);
    }
    last->properties |= 8;
    for (node = last; node != first; node = node->previous) {
        if (!schedule_final_mode)
            for (edge = node->outgoing; edge; edge = edge->next_outgoing)
                adjust_schedule_dependency(edge);
        if (node != last && node->incoming_count == node->remaining_incoming)
            prepare_schedule_ready_node(node);
        properties = node->properties;
        if (properties & 8)
            for (edge = node->outgoing; edge; edge = edge->next_outgoing)
                if ((unsigned)node->path_length == (unsigned)edge->delay + 1 + edge->destination->path_length)
                    edge->destination->properties |= 8;
        /* Only the reached early/late Pentium priority columns are modeled. */
        if (schedule_weights != &late_schedule_weights[2] &&
            schedule_weights != &early_schedule_weights[2]) { __asm { int 3 } }
        priority = scale_schedule_priority(node->path_length, schedule_weights->path_weight);
        priority += scale_schedule_priority(node->outgoing_count, schedule_weights->outgoing_weight);
        priority += scale_schedule_priority((properties >> 5) & 1, schedule_weights->read_weight);
        priority += scale_schedule_priority((properties >> 3) & 1, schedule_weights->critical_weight);
        priority += scale_schedule_priority((properties >> 4) & 1, schedule_weights->fixed_resource_weight);
        node->priority = priority;
        if (node->record && (node->record->type & 0xf000) == 0x4000)
            node->priority += scale_schedule_priority((properties >> 6) & 1, schedule_weights->write_weight);
        node->current_priority = node->priority;
    }
}
C2ScheduleNode *schedule_active_last, *schedule_active_first; /*493120/311c */
C2ScheduleNode *schedule_ready_head, *schedule_ready_tail; /*493134/30c4 */
unsigned schedule_ready_count; /*493124, real full-DWORD count; prior reset-only pointer view superseded. */
C2ScheduleNode *schedule_last_issued; /*493144 */
unsigned schedule_emitted_count; /*493114, actual DWORD increment; previous reset-only pointer view superseded. */
unsigned schedule_state_4930c0;
int schedule_cycle; /*4930ac */
/* The supported processors issue at most three nodes in one cycle. The
 * adjacent fourth DWORD and following scalar are not part of this view. */
C2ScheduleNode *schedule_selected[3]; /*4930b0 */
void __fastcall clear_schedule_resource_busy(void)
{
    int i;
    if (schedule_resource_count > 4) { __asm { int 3 } }
    for (i = 0; i < schedule_resource_count; ++i) schedule_resources[i].occupied = 0;
}
void __fastcall enqueue_schedule_node(C2ScheduleNode *node)
{
    C2ScheduleNode *following;
    ++schedule_ready_count;
    if (!schedule_ready_head) {
        schedule_ready_tail = schedule_ready_head = node;
        node->ready_previous = NULL;
        node->ready_next = NULL;
        return;
    }
    for (following = schedule_ready_head; following; following = following->ready_next)
        if (node->current_priority > following->current_priority ||
            (node->current_priority == following->current_priority && node->number < following->number)) break;
    if (!following) {
        node->ready_previous = schedule_ready_tail;
        schedule_ready_tail->ready_next = node;
        schedule_ready_tail = node;
        node->ready_next = NULL;
        return;
    }
    node->ready_next = following;
    node->ready_previous = following->ready_previous;
    if (following->ready_previous) following->ready_previous->ready_next = node;
    else schedule_ready_head = node;
    following->ready_previous = node;
}
short schedule_early_base, schedule_early_delta; /*49306c/306e, initialized signed-WORD counters. */
int __fastcall can_pair_schedule_nodes(C2ScheduleNode *first, C2ScheduleNode *second)
{
    C2Expression *source = first->record, *destination = second->record;
    unsigned first_class, second_class, opcode;
    int first_ok, second_ok;
    if (!destination) return 0;
    opcode = source->opcode;
    if (opcode >= 0x11b) first_class = 0x300;
    else {
        if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
        /* Only full property DWORDs for this reached scheduling range. The
         * earlier resource-class consumer uses just the low BYTE. */
        switch (opcode) {
        case 0x12: case 0x2f: case 0x32: case 0x2d: case 0x21: case 1: case 0x2e: case 0x30: case 0x34: case 0xb9: case 0xba:
            first_class = 0;
            break;
        case 0x33: case 0x29: case 0x20: case 0xdf: case 8: case 0x1f: case 0x2a: case 0x2b: case 0xfe:
            first_class = 0x300;
            break;
        case 0x10: first_class = 0x300; break;
        case 0xd: first_class = 0x0; break;
        case 0xe: first_class = 0x300; break;
        case 0x1e: first_class = 0x0; break;
        case 0xc1: first_class = 0x304; break;
        case 0xf: first_class = 0x200; break;
        case 0x56: case 0x62: case 0x63: first_class = 0x302; break;
        case 0x5c: first_class = 0x102; break;
        case 0x5f: first_class = 0x202; break;
        case 0x47: case 0x60: first_class = 0x102; break;
        case 0x46: case 0x4b: case 0x4c: case 0x4d: case 0x5d: first_class = 0x102; break;
        case 0x6e: first_class = 0x302; break;
        case 0xae: first_class = 0x300; break;
        case 0x45: first_class = 0x102; break;
        case 0x77: first_class = 0x102; break;
        case 0x4a: case 0x4f: first_class = 0x102; break;
        default: __asm { int 3 } first_class = 0;
        }
    }
    if ((source->type & 0xf000) == 0x4000) {
        first_ok = first_class == 0x102;
        second_ok = destination->opcode == 0x5f;
    } else {
        first_ok = first_class == 0 || first_class == 0x100;
        opcode = destination->opcode;
        if (opcode >= 0x11b) second_class = 0x300;
        else {
            if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
            switch (opcode) {
            case 0x12: case 0x2f: case 0x32: case 0x2d: case 0x21: case 1: case 0x2e: case 0x30: case 0x34: case 0xb9: case 0xba:
                second_class = 0;
                break;
            case 0x33: case 0x29: case 0x20: case 0xdf: case 8: case 0x1f: case 0x2a: case 0x2b: case 0xfe:
                second_class = 0x300;
                break;
            case 0x10: second_class = 0x300; break;
            case 0xd: second_class = 0x0; break;
            case 0xe: second_class = 0x300; break;
            case 0x1e: second_class = 0x0; break;
            case 0xc1: second_class = 0x304; break;
            case 0xf: second_class = 0x200; break;
            case 0x56: case 0x62: case 0x63: second_class = 0x302; break;
            case 0x5c: second_class = 0x102; break;
            case 0x5f: second_class = 0x202; break;
            case 0x47: case 0x60: second_class = 0x102; break;
            case 0x46: case 0x4b: case 0x4c: case 0x4d: case 0x5d: second_class = 0x102; break;
            case 0x6e: second_class = 0x302; break;
            case 0xae: second_class = 0x300; break;
            case 0x45: second_class = 0x102; break;
            case 0x77: second_class = 0x102; break;
            case 0x4a: case 0x4f: second_class = 0x102; break;
            default: __asm { int 3 } second_class = 0;
            }
        }
        second_ok = second_class == 0 || second_class == 0x200;
    }
    return first_ok && second_ok;
}
void __fastcall update_schedule_ready_priorities(C2ScheduleNode *head, unsigned cycle)
{
    C2ScheduleNode *node, *other;
    C2ScheduleDependency *edge;
    int found, i;
    if (!head || !head->ready_next) return;
    if (!schedule_final_mode) {
        if (schedule_weights != &late_schedule_weights[2]) { __asm { int 3 } }
        if (schedule_weights->pair_weight < 0) return;
    }
    if (schedule_cpu_index == 3 && !schedule_final_mode) return;
    if (!schedule_final_mode) {
        for (node = head; node; node = node->ready_next) {
            if (!node->record) continue;
            node->current_priority = node->priority;
            if (node->ready_cycle > cycle) continue;
            found = 0;
            for (other = head; other; other = other->ready_next)
                if (other->record && other->ready_cycle <= cycle && other != node &&
                    can_pair_schedule_nodes(node, other)) {
                    found = 1;
                    break;
                }
            if (!found)
                for (edge = node->outgoing; edge; edge = edge->next_outgoing)
                    if (edge->destination->incoming_count == 1 && !edge->delay &&
                        edge->destination->ready_cycle <= cycle && can_pair_schedule_nodes(node, edge->destination)) {
                        found = 1;
                        break;
                    }
            if (found) node->current_priority = node->priority + scale_schedule_priority(1, schedule_weights->pair_weight);
        }
    } else if ((int)schedule_early_base + schedule_early_delta < 8 || !head->current_priority) {
        if (!head->current_priority && schedule_early_delta + 2 < 8)
            for (node = head; node; node = node->ready_next)
                if (node->record) node->current_priority = node->priority;
    } else {
        for (node = head; node; node = node->ready_next)
            if (node->record) {
                node->current_priority = 0;
                node->ready_cycle = cycle;
            }
        if (schedule_resource_count > 4) { __asm { int 3 } }
        for (i = 0; i < schedule_resource_count; ++i) schedule_resources[i].occupied = 0;
    }
}
void __fastcall rebuild_schedule_ready_queue(void)
{
    C2ScheduleNode *node = schedule_ready_head, *next;
    if ((int)schedule_ready_count <= 1) return;
    schedule_ready_tail = NULL;
    schedule_ready_head = NULL;
    schedule_ready_count = 0;
    while (node) {
        next = node->ready_next;
        enqueue_schedule_node(node);
        node = next;
    }
}
int __fastcall schedule_node_is_busy(C2ScheduleNode *node, unsigned cycle)
{
    C2ScheduleDependency *edge;
    C2ScheduleNode *source;
    if (!node->incoming_count || schedule_cpu_index == 3) return 0;
    for (edge = node->incoming; edge; edge = edge->next_incoming) {
        if ((edge->properties & 31) < 0x10) continue;
        source = edge->source;
        if (source->properties & 0x80) continue;
        if (source->incoming_count != source->remaining_incoming || cycle + 1U < source->ready_cycle) continue;
        return 1;
    }
    return 0;
}
int __fastcall delay_schedule_node(C2ScheduleNode *node, unsigned cycle, C2ScheduleNode *head,
    C2Expression *output)
{
    C2ScheduleNode *other;
    unsigned type;
    if (!schedule_final_mode) {
        if (output->opcode != 0x5f &&
            (output->opcode != 0x138 || output->previous->opcode != 0x5f)) return 0;
        if (schedule_cpu_index != 2) return 0;
        type = node->record->type;
        if (!((1U << (type >> 12)) & 0xe)) return 0;
        for (other = head; other; other = other->ready_next)
            if (other->record && (other->record->type & 0xf000) == 0x4000 &&
                schedule_resources[2].occupied <= 1 && other->ready_cycle <= cycle + 1U) return 1;
    } else {
        for (other = node->ready_next; other; other = other->ready_next)
            if (!other->record && other->number < node->number) {
                /* Original46e214 reads WORD address0xa after proving this
                 * record NULL. This is the recovered original defect, not an
                 * unknown semantic domain; preserve the genuine C read. */
                type = other->record->type;
                if (((1U << (type >> 12)) & 0xe) && (node->record->type & 0xf000) == 0x4000) return 1;
                if ((type & 0xf000) == 0x4000 && ((1U << (node->record->type >> 12)) & 0xe)) return 1;
            }
    }
    return 0;
}
void __fastcall remove_schedule_ready_node(C2ScheduleNode *node)
{
    --schedule_ready_count;
    if (schedule_ready_head == node) schedule_ready_head = node->ready_next;
    else node->ready_previous->ready_next = node->ready_next;
    if (schedule_ready_tail == node) schedule_ready_tail = node->ready_previous;
    else node->ready_next->ready_previous = node->ready_previous;
    node->ready_previous = NULL;
    node->ready_next = NULL;
}
C2ScheduleDependency *__fastcall find_schedule_coalescing_dependency(C2ScheduleNode *source, C2ScheduleNode *destination)
{
    C2ScheduleDependency *edge;
    unsigned kind;
    if (schedule_cpu_index == 3) return NULL;
    for (edge = source->outgoing; edge; edge = edge->next_outgoing) {
        if (edge->destination != destination) continue;
        kind = edge->properties & 31;
        if (kind == 6 || kind == 8 || kind == 10) return edge;
    }
    return NULL;
}
void __fastcall detach_input_operand(C2Expression *record, C2Expression *operand);
void __fastcall remove_schedule_dependency(C2ScheduleDependency *edge)
{
    --edge->source->outgoing_count;
    --edge->destination->incoming_count;
    if (edge->properties & 31) --edge->destination->remaining_incoming;
    release_flow_edge(&active_schedule_graph->flow, (C2FlowEdge *)edge);
    --schedule_edge_count;
}
void __fastcall prepare_schedule_terminal_node(C2ScheduleNode *node)
{ __asm { int 3 } }
void __fastcall discard_schedule_node_dependencies(C2ScheduleNode *node)
{ __asm { int 3 } }

/* Apply the actual address-rewrite class selected on this dependency. The
 * shared completion path remains part of this owner, including cold cases. */
int __fastcall apply_schedule_dependency(C2ScheduleDependency *edge)
{
    C2ScheduleNode *source_node = edge->source;
    C2Expression *source = source_node->record;
    C2Expression *destination = edge->destination->record;
    C2Expression *owner, *address, *form, *operand, *component, *copy;
    unsigned action = edge->properties & 31, bytes, scale, shift;

    owner = action >= 16 ? source : destination;
    for (address = owner->left; address && address->kind != 6; address = address->next) ;
    if (!address)
        for (address = owner->tail.right; address && address->kind != 6; address = address->next) ;

    switch (action) {
    case 1: case 3: case 16: case 17:
        owner = action < 16 ? source : destination;
        bytes = owner->left->next->integer.low;
        scale = 1U << (address->location.expression.properties & 15);
        if (address->aggregate_value && owner->tail.right->original_register ==
            address->aggregate_value->original_register) scale = 1;
        if (action == 3 || action == 16)
            address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement - scale * bytes);
        else
            address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement + scale * bytes);
        break;
    case 2: case 18:
        owner = action == 2 ? source : destination;
        copy = copy_input_expression(owner->left->next);
        copy_input_operand(action == 2 ? destination : source, copy);
        if (address->aggregate_second) address->aggregate_value = copy;
        else {
            address->aggregate_second = copy;
            address->location.expression.properties &= 0xf0;
        }
        address->opcode = 0x123;
        break;
    case 4:
        bytes = source->left->next->integer.low;
        if (!address->aggregate_second) {
            address->aggregate_second = address->aggregate_value;
            address->aggregate_value = NULL;
        } else bytes *= 1U << (address->location.expression.properties & 15);
        shift = bytes == 1 ? 0 : bytes == 2 ? 1 : bytes == 4 ? 2 : 3;
        address->location.expression.properties = (address->location.expression.properties & 0xf0) | shift;
        /* This class shares the authentic kind-5 address replacement below. */
    case 5: case 6:
        for (form = source->left; form && form->kind != 5; form = form->next) ;
        operand = address->aggregate_value;
        if (operand) {
            operand->location.expression.properties &= 0xdf;
            detach_input_operand(destination, operand);
            release_small_node(address->aggregate_value);
            address->aggregate_value = NULL;
        }
        operand = address->aggregate_second;
        if (operand) {
            operand->location.expression.properties &= 0xdf;
            detach_input_operand(destination, operand);
            release_small_node(address->aggregate_second);
            address->aggregate_second = NULL;
        }
        if (form->aggregate_value) {
            address->aggregate_value = copy_input_expression(form->aggregate_value);
            copy_input_operand(destination, address->aggregate_value);
            address->aggregate_value->location.expression.properties |= 0x10;
        }
        if (form->aggregate_second) {
            address->aggregate_second = copy_input_expression(form->aggregate_second);
            copy_input_operand(destination, address->aggregate_second);
            address->aggregate_second->location.expression.properties |= 0x10;
        }
        address->opcode = form->opcode;
        address->tail.aggregate_register = form->tail.aggregate_register;
        address->location.expression.properties = (address->location.expression.properties & 0xf0) |
            (form->location.expression.properties & 15);
        address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
            (unsigned)form->detail.aggregate_displacement);
        break;
    case 7: case 8:
        for (form = source->left; form && form->kind != 5; form = form->next) ;
        component = form->aggregate_value ? form->aggregate_value : form->aggregate_second;
        address->aggregate_value->value.reg = component->value.reg;
        address->aggregate_value->original_register = component->original_register;
        address->tail.aggregate_register = form->tail.aggregate_register;
        address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
            (unsigned)form->detail.aggregate_displacement);
        break;
    case 9: case 10:
        for (form = source->left; form && form->kind != 5; form = form->next) ;
        address->tail.aggregate_register = form->tail.aggregate_register;
        scale = 1U << (address->location.expression.properties & 15);
        address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
            scale * (unsigned)form->detail.aggregate_displacement);
        component = form->aggregate_value ? form->aggregate_value : form->aggregate_second;
        address->aggregate_second->value.reg = component->value.reg;
        address->aggregate_second->original_register = component->original_register;
        if (!form->aggregate_value)
            address->location.expression.properties = (address->location.expression.properties & 0xf0) |
                ((form->location.expression.properties + address->location.expression.properties) & 15);
        break;
    case 11: case 12: case 21: case 22:
        owner = action < 16 ? source : destination;
        scale = 1U << (address->location.expression.properties & 15);
        if (address->aggregate_value && owner->tail.right->original_register ==
            address->aggregate_value->original_register) scale = 1;
        if (action == 12 || action == 21)
            address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement - scale);
        else
            address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement + scale);
        break;
    case 19: case 20:
        for (form = destination->left; form && form->kind != 5; form = form->next) ;
        scale = action == 19 ? 1 : 1U << (address->location.expression.properties & 15);
        address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement -
            scale * (unsigned)form->detail.aggregate_displacement);
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\schedmd.c", 0x728);
        break;
    }
    remove_schedule_dependency(edge);
    if (edge->properties & 0x20) discard_schedule_node_dependencies(source_node);
    else if (!source_node->outgoing_count) prepare_schedule_terminal_node(source_node);
    destination->flags &= 0xfb;
    source->flags &= 0xfb;
    return 1;
}
void __fastcall update_schedule_dependent(C2ScheduleDependency *edge)
{
    C2ScheduleNode *node = edge->destination;
    unsigned short before = node->incoming_count, remaining = node->remaining_incoming;
    if (edge->properties & 31) --node->remaining_incoming;
    node->incoming_count = (unsigned short)(before - 1);
    if (before != remaining && node->incoming_count == node->remaining_incoming)
        enqueue_schedule_node(node);
}
int __fastcall select_schedule_nodes(unsigned cycle)
{
    C2ScheduleNode *node;
    C2ScheduleDependency *edge, *following;
    unsigned occupied, available;
    unsigned short incoming_count;
    short resource;
    int count = 0;
    update_schedule_ready_priorities(schedule_ready_head, cycle);
    rebuild_schedule_ready_queue();
    if (schedule_issue_width > 3) { __asm { int 3 } }
    while (count < schedule_issue_width) {
        for (node = schedule_ready_head; node; node = node->ready_next) {
            if (node == schedule_active_first) return count;
            resource = schedule_select_node(node, schedule_selected, count);
            if (resource < 0 || schedule_node_is_busy(node, cycle)) continue;
            if (c2_favor_speed && delay_schedule_node(node, cycle, schedule_ready_head, schedule_output_tail)) continue;
            if (resource >= 4) { __asm { int 3 } }
            occupied = schedule_resources[resource].occupied > 0 ? schedule_resources[resource].occupied : 0;
            if (node->ready_cycle > occupied + cycle) continue;
            break;
        }
        if (!node) return count;
        remove_schedule_ready_node(node);
        incoming_count = node->incoming_count;
        schedule_selected[count] = node;
        node->properties = (node->properties & ~7) | (resource & 7) | 0x80;
        if (!incoming_count) {
            if (schedule_last_issued && schedule_last_issued->record->opcode == 0x12) {
                edge = find_schedule_coalescing_dependency(schedule_last_issued, node);
                if (edge) {
                    ++schedule_last_issued->outgoing_count;
                    ++node->incoming_count;
                    ++node->remaining_incoming;
                    apply_schedule_dependency(edge);
                }
            }
        } else {
            for (edge = node->incoming; edge; edge = following) {
                following = edge->next_incoming;
                if (edge->source->properties & 0x80) continue;
                apply_schedule_dependency(edge);
                if (!incoming_count) break;
            }
        }
        for (edge = node->outgoing; edge; edge = edge->next_outgoing) {
            available = (unsigned)edge->delay + occupied + cycle;
            if (edge->destination->ready_cycle < available) edge->destination->ready_cycle = available;
            update_schedule_dependent(edge);
        }
        ++count;
        schedule_last_issued = node;
    }
    return count;
}
void __fastcall append_scheduled_record(C2Expression *record)
{
    record->previous = schedule_output_tail;
    schedule_output_tail->next = record;
    if (record->location.line < schedule_output_tail->location.line)
        record->location.line = schedule_output_tail->location.line;
    schedule_output_tail = record;
    ++schedule_emitted_count;
}
int __fastcall schedule_node_resource_cost(C2ScheduleNode *node)
{
    unsigned opcode;
    if (!node->record) return 0;
    opcode = node->record->opcode;
    if (opcode >= 0x11b) return 0;
    if (schedule_opcode_costs != &schedule_costs_pentium) { __asm { int 3 } }
    /* The second DWORD is the resource occupancy increment, distinct from
     * the result delay and the full pairing/property DWORD. */
    switch (opcode) {
    case 0x12: case 0x2f: case 0xf: case 0x33: case 0x29: case 0x32: case 0x2d: case 0x21: case 1: case 0x2a: case 0x2b: case 0x2e: case 0x30: case 0x34: case 0xb9: case 0xba:
        return 1;
    case 0x10: return 2;
    case 0xd: return 1;
    case 0xe: return 0;
    case 0x1e: return 1;
    case 0xc1: return 10;
    case 0x20: return 22;
    case 0xdf: return 2;
    case 8: return 3;
    case 0x1f: return 17;
    case 0xfe: return 13;
    case 0x56: case 0x47: case 0x62: case 0x63: case 0x60: return 1;
    case 0x46: case 0x4b: case 0x4c: case 0x4d: case 0x5d: return 1;
    case 0x6e: case 0xae: return 2;
    case 0x45: return 1;
    case 0x77: return 1;
    case 0x4a: case 0x4f: return 39;
    case 0x5c: return 1;
    case 0x5f: return 1;
    default: __asm { int 3 }
    }
    return 0;
}
void __fastcall issue_schedule_node(C2ScheduleNode *node)
{
    unsigned resource = node->properties & 7;
    int cost = schedule_node_resource_cost(node);
    if (node->record->opcode == 0x46) schedule_opcode46_cooldown = 2;
    if (resource >= 4) { __asm { int 3 } }
    if (schedule_resources[resource].occupied < 0) schedule_resources[resource].occupied = 0;
    schedule_resources[resource].occupied = (int)((unsigned)schedule_resources[resource].occupied + (unsigned)cost);
}
int __fastcall final_float_stack_effect(C2Expression *record);
void __fastcall complete_schedule_node(C2ScheduleNode *node)
{
    if (schedule_final_mode)
        schedule_early_delta = (short)((unsigned short)schedule_early_delta +
            (unsigned short)final_float_stack_effect(node->record));
}
void __fastcall advance_schedule_cycle(void)
{
    int i;
    if (schedule_resource_count > 4) { __asm { int 3 } }
    for (i = 0; i < schedule_resource_count; ++i)
        schedule_resources[i].occupied = (int)((unsigned)schedule_resources[i].occupied - 1U);
    schedule_opcode46_cooldown = (int)((unsigned)schedule_opcode46_cooldown - 1U);
}
void __fastcall emit_schedule_graph(C2ScheduleGraph *graph)
{
    C2ScheduleDependency *edge;
    C2ScheduleNode *node;
    int count, i;
    schedule_active_last = (C2ScheduleNode *)graph->flow.last;
    schedule_active_first = (C2ScheduleNode *)graph->flow.first;
    schedule_emitted_count = 0;
    schedule_state_4930c0 = 0;
    schedule_last_issued = NULL;
    clear_schedule_resource_busy();
    schedule_ready_tail = NULL;
    schedule_ready_head = NULL;
    schedule_ready_count = 0;
    schedule_active_last->properties |= 0x80;
    for (edge = schedule_active_last->outgoing; edge; edge = edge->next_outgoing) {
        node = edge->destination;
        --node->incoming_count;
        if (node->incoming_count == node->remaining_incoming) enqueue_schedule_node(node);
    }
    schedule_cycle = 0;
    while (schedule_ready_head && schedule_ready_head != schedule_active_first) {
        count = select_schedule_nodes(schedule_cycle);
        if (count > 3) { __asm { int 3 } }
        if (count > 1)
            for (i = 0; i < count; ++i) schedule_selected[i]->record->flags &= 0xfb;
        for (i = 0; i < count; ++i) {
            node = schedule_selected[i];
            append_scheduled_record(node->record);
            issue_schedule_node(node);
            complete_schedule_node(node);
        }
        schedule_cycle = (int)((unsigned)schedule_cycle + 1U);
        advance_schedule_cycle();
    }
}
int __fastcall schedule_float_operation_is_ordered(C2Expression *record);
void __fastcall move_scheduled_float_slot(C2Expression *record, int from, int to);
void __fastcall exchange_scheduled_float_top(C2Expression *record, int slot);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);

/* These guards reuse only already established columns of the partial machine
 * owner. They do not give the remaining x87 rows kind/root/payload defaults. */
#define EARLY_FLOAT_KIND(reg, value) do { \
    if ((reg) == (C2FunctionRegister *)&machine_registers[9] || \
        (reg) == (C2FunctionRegister *)&machine_registers[17] || \
        (reg) == (C2FunctionRegister *)&machine_registers[20] || \
        (reg) == (C2FunctionRegister *)&machine_registers[31] || \
        (reg) == (C2FunctionRegister *)&machine_registers[32] || \
        (reg) == (C2FunctionRegister *)&machine_registers[74]) (value) = (reg)->kind; \
    else SCHEDULE_REGISTER_KIND(reg, value); \
} while (0)
#define EARLY_FLOAT_ROOT(reg, value) do { \
    if ((reg) == (C2FunctionRegister *)&machine_registers[9] || \
        (reg) == (C2FunctionRegister *)&machine_registers[17] || \
        (reg) == (C2FunctionRegister *)&machine_registers[20] || \
        (reg) == (C2FunctionRegister *)&machine_registers[31] || \
        (reg) == (C2FunctionRegister *)&machine_registers[32] || \
        (reg) == (C2FunctionRegister *)&machine_registers[74]) (value) = (reg)->canonical; \
    else SCHEDULE_REGISTER_ROOT(reg, value); \
} while (0)
#define EARLY_FLOAT_INDEX(index) do { if ((unsigned)(index) >= 8) { __asm { int 3 } } } while (0)
/* The original error continuation retains the corresponding prior stack local
 * if the internal-error routine returns. Do not seed an invented index. */
#define FIND_EARLY_FLOAT(reg, index) do { \
    EARLY_FLOAT_ROOT(reg, root); \
    if (early_float_stack_base < 0) { __asm { int 3 } } \
    for (position = early_float_stack_base; position < 8; ++position) \
        if (early_float_registers[position] == root) break; \
    if (position >= 8) c2_report_internal("E:\\utc\\src\\\\P2\\x86\\schedmd.c", 0x9e3); \
    else (index) = position - early_float_stack_base; \
} while (0)

void __fastcall finish_schedule_output(C2Expression *first)
{
    C2Expression *record, *input, *second, *output, *operand, *pending;
    C2FunctionRegister *reg, *root;
    int effect, ordered, live, position, outputs;
    int index, assignment_index, result_index; /* Genuine carried locals. */
    short count, top;
    unsigned kind;

    for (record = first; record; record = record->next) {
        if (!(record->flags & 1) ||
            ((record->type & 0xf000) != 0x4000 && record->kind != 13)) continue;
        input = record->left;
        output = record->tail.right;
        effect = final_float_stack_effect(record);
        if (record->opcode == 0x138) {
            reg = input->value.reg;
            FIND_EARLY_FLOAT(reg, index);
            EARLY_FLOAT_INDEX(index);
            input->fixed_register = &machine_registers[31 + index];
            FIND_EARLY_FLOAT(reg, assignment_index);
            EARLY_FLOAT_INDEX(early_float_stack_base + assignment_index);
            EARLY_FLOAT_ROOT(output->value.reg, root);
            early_float_registers[early_float_stack_base + assignment_index] = root;
            output->fixed_register = &machine_registers[31 + index];
            continue;
        }
        ordered = record->kind == 13;
        if (!ordered) ordered = schedule_float_operation_is_ordered(record);
        second = NULL;
        if (!ordered && input && input->kind == 1) {
            reg = input->value.reg;
            EARLY_FLOAT_KIND(reg, kind);
            if (kind == 3 && reg->value) {
                second = input->next;
                if (second && second->kind == 1) {
                    reg = second->value.reg;
                    EARLY_FLOAT_KIND(reg, kind);
                    if (kind != 3 || !reg->value || (second->type & 0xf000) != 0x4000)
                        second = NULL;
                } else second = NULL;
            }
        }
        if (second) {
            FIND_EARLY_FLOAT(input->value.reg, index);
            if (!index && effect < 0 && (output->type & 0xf000) == 0x4000) {
                move_scheduled_float_slot(record, 0, 1);
                index = 1;
            }
            EARLY_FLOAT_INDEX(index);
            input->fixed_register = &machine_registers[31 + index];
            if ((output->type & 0xf000) == 0x4000) {
                output->fixed_register = &machine_registers[31 + index];
                FIND_EARLY_FLOAT(input->value.reg, result_index);
                EARLY_FLOAT_INDEX(early_float_stack_base + result_index);
                EARLY_FLOAT_ROOT(output->value.reg, root);
                early_float_registers[early_float_stack_base + result_index] = root;
            }
            if (!index) {
                FIND_EARLY_FLOAT(second->value.reg, index);
                EARLY_FLOAT_INDEX(index);
                second->fixed_register = &machine_registers[31 + index];
            } else {
                FIND_EARLY_FLOAT(second->value.reg, index);
                exchange_scheduled_float_top(record, index);
                second->fixed_register = &machine_registers[31];
            }
            early_float_stack_base = (short)((unsigned short)early_float_stack_base -
                (unsigned short)effect);
            if (effect < 0) {
                if (output->fixed_register != &machine_registers[31] &&
                    output->fixed_register != &machine_registers[32]) { __asm { int 3 } }
                if (output->fixed_register->number != 31)
                    attach_optimizer_output(record, new_fixed_register_operand(31, output->type));
            }
            continue;
        }
        count = 0;
        pending = NULL;
        for (operand = record->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            reg = operand->value.reg;
            EARLY_FLOAT_KIND(reg, kind);
            if (kind != 3 || !reg->value || (operand->type & 0xf000) != 0x4000) continue;
            if (reg->value->opcode == 0x16c) {
                EARLY_FLOAT_INDEX(operand->float_stack_index);
                operand->fixed_register = &machine_registers[31 + operand->float_stack_index];
            } else {
                count = (short)((unsigned short)count + 1);
                if (!operand->float_stack_index) pending = operand;
                else {
                    FIND_EARLY_FLOAT(reg, index);
                    move_scheduled_float_slot(record, index, operand->float_stack_index);
                    EARLY_FLOAT_INDEX(operand->float_stack_index);
                    operand->fixed_register = &machine_registers[31 + operand->float_stack_index];
                }
            }
        }
        if (pending) {
            FIND_EARLY_FLOAT(pending->value.reg, index);
            move_scheduled_float_slot(record, index, 0);
            pending->fixed_register = &machine_registers[31];
        }
        top = (short)((unsigned short)early_float_stack_base + (unsigned short)count);
        outputs = 0;
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            reg = operand->value.reg;
            EARLY_FLOAT_KIND(reg, kind);
            live = kind == 3 && reg->value && (operand->type & 0xf000) == 0x4000;
            if (!live && record->opcode != 0x60) continue;
            ++outputs;
            if (record->opcode != 0x16c) {
                top = (short)((unsigned short)top - 1);
                EARLY_FLOAT_INDEX(top);
                EARLY_FLOAT_ROOT(reg, root);
                early_float_registers[top] = root;
            }
            EARLY_FLOAT_INDEX(operand->float_stack_index);
            operand->fixed_register = &machine_registers[31 + operand->float_stack_index];
        }
        early_float_stack_base = top;
        if (outputs > 1) {
            output = record->tail.right;
            output->fixed_register = &machine_registers[32];
            EARLY_FLOAT_INDEX(top + 1);
            EARLY_FLOAT_ROOT(output->value.reg, root);
            early_float_registers[top + 1] = root;
            second = output->next;
            second->fixed_register = &machine_registers[31];
            EARLY_FLOAT_INDEX(top);
            EARLY_FLOAT_ROOT(second->value.reg, root);
            early_float_registers[top] = root;
        }
    }
}
#undef FIND_EARLY_FLOAT
#undef EARLY_FLOAT_INDEX
#undef EARLY_FLOAT_ROOT
#undef EARLY_FLOAT_KIND

int __fastcall schedule_float_operation_is_ordered(C2Expression *record)
{
    switch (record->opcode) {
    case 0x47: case 0x49: case 0x4a: case 0x4d: case 0x4f: case 0x50:
    case 0x5c: case 0x5d: case 0x5e: case 0xd1: case 0xd2: case 0xd3:
        return record->left->float_stack_index == 0;
    case 0x86: case 0x87: case 0x8a: case 0x91: case 0x92: case 0xd4:
        return 1;
    default:
        return 0;
    }
}
void __fastcall move_scheduled_float_slot(C2Expression *record, int from, int to)
{
    if (from == to) return;
    exchange_scheduled_float_top(record, from);
    exchange_scheduled_float_top(record, to);
}
void __fastcall swap_scheduled_float_registers(int first, int second);
void __fastcall exchange_scheduled_float_top(C2Expression *record, int slot)
{
    C2Expression *scan, *source, *top;

    if (!slot) return;
    swap_scheduled_float_registers(slot, 0);
    for (scan = record->previous; scan; scan = scan->previous) {
        if ((scan->type & 0xf000) != 0x4000 && scan->kind != 13) continue;
        if (scan->opcode == 0x138) continue;
        source = new_fixed_register_operand(31 + slot, 0x400a);
        top = new_fixed_register_operand(31, 0x400a);
        new_binary_expression(0x5f, 0x400a, top, source,
            insert_input_record, (C2Record *)scan);
        return;
    }
    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\schedmd.c", 0x999);
}
void __fastcall swap_scheduled_float_registers(int first, int second)
{
    unsigned a = (unsigned)(int)early_float_stack_base + (unsigned)first;
    unsigned b = (unsigned)(int)early_float_stack_base + (unsigned)second;
    C2FunctionRegister *temporary;

    /* The encountered stack has eight pointer slots; surrounding globals
     * are not an extension of this array in either reconstructed build. */
    if (a >= 8 || b >= 8) { __asm { int 3 } }
    temporary = early_float_registers[b];
    early_float_registers[b] = early_float_registers[a];
    early_float_registers[a] = temporary;
}

void __fastcall finish_schedule_registers(C2Function *function)
{
    C2Expression *first = ((C2Expression *)function->flow->last->records)->next;
    C2Expression *record = first, *marker = NULL, *before;
    int count = 0;
    unsigned i;
    while (record->opcode != 0x17e) {
        if (record->opcode == 0x180) marker = record;
        record = record->next;
    }
    for (before = first; before != schedule_entry_tail; before = before->next) ++count;
    if (count < 40) {
        unlink_input_record((C2Record *)record);
        insert_input_record((C2Record *)schedule_entry_tail, (C2Record *)record);
        if (marker) {
            unlink_input_record((C2Record *)marker);
            insert_input_record((C2Record *)schedule_entry_tail, (C2Record *)marker);
        }
    } else {
        before = marker ? marker->previous : record->previous;
        if (schedule_entry_record_count > 10) { __asm { int 3 } }
        for (i = 0; i < schedule_entry_record_count; ++i) {
            unlink_input_record((C2Record *)schedule_entry_records[i]);
            insert_input_record((C2Record *)before, (C2Record *)schedule_entry_records[i]);
        }
    }
    marker = NULL;
    record = ((C2Expression *)function->flow->first->last_record)->previous;
    while (record->opcode != 0x17f) {
        if (record->opcode == 0x181) marker = record;
        record = record->previous;
    }
    unlink_input_record((C2Record *)record);
    prepend_input_record((C2Record *)schedule_exit_head, (C2Record *)record);
    if (marker) {
        unlink_input_record((C2Record *)marker);
        prepend_input_record((C2Record *)schedule_exit_head, (C2Record *)marker);
    }
}

void __fastcall schedule_final_function(C2Function *function, int mode)
{
    C2Expression *record, *before;

    if (!final_scheduling_is_enabled(function, mode)) return;
    schedule_final_mode = mode;
    schedule_graph = new_schedule_graph(0, 0);
    initialize_schedule_function(function);
    if (!mode) prepare_schedule_registers(function);
    schedule_output_tail = function->flow->last->records;
    while (schedule_output_tail->opcode != 0x17e)
        schedule_output_tail = schedule_output_tail->next;
    record = schedule_output_tail->next;
    while (record->opcode != 0x17b) {
        schedule_first_record = record;
        schedule_last_record = find_schedule_range_end(record);
        record = schedule_last_record->next;
        if (!schedule_range_has_work(schedule_first_record, schedule_last_record)) {
            schedule_output_tail->next = schedule_first_record;
            schedule_first_record->previous = schedule_output_tail;
            schedule_output_tail = schedule_last_record;
        } else {
            reset_schedule_graph(schedule_graph);
            if (mode) {
                before = schedule_output_tail;
                prepare_schedule_range(schedule_first_record, schedule_last_record);
            }
            build_schedule_graph(schedule_graph, schedule_first_record, schedule_last_record);
            order_schedule_graph(schedule_graph);
            emit_schedule_graph(schedule_graph);
            if (mode) {
                schedule_output_tail->next = NULL;
                finish_schedule_output(before->next);
            }
        }
    }
    schedule_output_tail->next = record;
    record->previous = schedule_output_tail;
    if (!mode) finish_schedule_registers(function);
    reset_input_pool(4);
}

/* Register liveness and interference while visiting selected blocks. */
#include "physical-allocation.h"
#include "bitsets.h"
#include "register-use.h"
#include "instruction-rewrite.h"
extern int cpu_model;
int allocation_live_register_count;
RegisterConflict *allocation_live_registers;

/* Original [0002bbd0,0002bcd0). Start with each block's live-in registers
   and visit its selected instructions in stream order. Arena3 holds the
   temporary live list; the visitor owns persistent conflict edges. */
void __fastcall build_register_conflicts(WorkItem *work, RegisterConflictVisitor visitor)
{
    MachineInstruction *instruction;
    unsigned kind;
    int depth;
    for (; work; work = work->next) {
        instruction = work->body.block->next;
        while ((instruction->opcode & 15) == 4) instruction = instruction->next;
        if ((instruction->opcode & 15) == 7) continue;
        allocation_live_register_count = 0;
        allocation_live_registers = 0;
        reset_arena(3);
        initialize_allocation_live_registers(work);
        release_bitset(work->flow_sets[0]);
        work->flow_sets[0] = 0;
        depth = (short)work->region_depth;
        for (; (instruction->opcode & 15) != 7; instruction = instruction->next) {
            kind = instruction->opcode & 15;
            if (kind == 0 || kind == 8 || kind == 9) {
                record_operand_conflicts(work, instruction, depth, visitor);
                record_special_register_conflicts(instruction, visitor);
                record_implicit_register_conflicts(instruction, visitor);
            } else if (kind == 10 && (instruction->operands[1].flags & 0x400)) {
                allocation_live_register_count -=
                    remove_allocation_live_register(&allocation_live_registers,
                        instruction->operands[1].value.reg.words.number);
            }
        }
    }
    reset_arena(3);
}

/* Original [0002db50,0002dbb0). Iterate a private copy of the block's
   live-in set. Untagged members above31 denote tree virtual registers. */
void __fastcall initialize_allocation_live_registers(WorkItem *work)
{
    BitSet *live;
    int member;
    unsigned short number;
    live = assign_bitset(0, work->pass.selection.uses);
    for (member = next_bitset_member(live); member != -1; member = next_bitset_member(0)) {
        number = (unsigned short)member;
        if (member > 31) number |= 0x4000;
        allocation_live_register_count += add_allocation_live_register(&allocation_live_registers, number);
    }
    release_bitset(live);
}

/* Original [0002ddc0,0002dfb0). Visit operands in reverse order so a
   definition meets all still-live sources, except a coalescible MOV source.
   Address base/index words carry the same signed allocation annotation as
   direct registers. Last-use bits remove those registers from the live list. */
void __fastcall record_operand_conflicts(WorkItem *work, MachineInstruction *instruction,
    int depth, RegisterConflictVisitor visitor)
{
    MachineOperand *operand;
    MachineAddress *address;
    unsigned short number, exclude;
    int count;
    for (count = machine_operand_counts[(instruction->opcode >> 8) & 255]; count > 0; --count) {
        operand = &instruction->operands[count - 1];
        switch (operand->flags & 15) {
        case OPERAND_REGISTER:
            number = operand->value.reg.words.number;
            work->flow_sets[0] = bitset_include(work->flow_sets[0], number);
            record_allocation_operand(instruction, number, (short)operand->value.reg.words.flags,
                count - 1, depth);
            if (count == 1 && writes_first_operand(instruction)) {
                exclude = 0xffff;
                if (((instruction->opcode >> 8) & 255) == MC_MOV &&
                    (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
                    ((instruction->opcode & 0xf0) > 0x10 ||
                     !(((instruction->opcode >> 19) ^ (instruction->opcode >> 20)) & 1)))
                    exclude = instruction->operands[1].value.reg.words.number;
                conflict_with_live_registers(number, exclude, visitor);
                allocation_live_register_count +=
                    add_allocation_live_register(&allocation_live_registers, number);
            }
            if (operand->flags & 0x400)
                allocation_live_register_count -=
                    remove_allocation_live_register(&allocation_live_registers, number);
            break;
        case OPERAND_INDEXED:
            address = operand->value.address;
            number = address->index.words.number;
            work->flow_sets[0] = bitset_include(work->flow_sets[0], number);
            record_allocation_operand(instruction, number, (short)address->index.words.flags,
                count - 1, depth);
            if (operand->flags & 0x800)
                allocation_live_register_count -=
                    remove_allocation_live_register(&allocation_live_registers, number);
            if (address->base.words.number == 0x1b) break;
            /* An indexed address may also have an ordinary base. */
        case OPERAND_BASED:
            address = operand->value.address;
            number = address->base.words.number;
            work->flow_sets[0] = bitset_include(work->flow_sets[0], number);
            record_allocation_operand(instruction, number, (short)address->base.words.flags,
                count - 1, depth);
            if (operand->flags & 0x400)
                allocation_live_register_count -=
                    remove_allocation_live_register(&allocation_live_registers, number);
            break;
        }
    }
}

/* Original [0002bf40,0002bff0). Physical registers need no spill cost.
   Weight each virtual operand's cost by loop depth, saturating at30000.
   The i486 LEA-pair exceptions avoid charging a foldable address twice. */
void __fastcall record_allocation_operand(MachineInstruction *instruction, unsigned short number,
    int annotation, int operand_index, int depth)
{
    RegisterVariable *variable;
    int cost;
    if (!(number & 0xc000)) return;
    variable = lookup_register_variable(number, 0);
    if (!variable) variable = create_register_variable(number);
    if (cpu_model == 4 && (register_precedes_lea(instruction, number) ||
        register_follows_lea(instruction, number))) return;
    if (variable->spill_cost == 30000) return;
    if (variable->flags & 0x40) {
        variable->spill_cost = 30000;
        return;
    }
    cost = allocation_operand_cost(instruction, variable, annotation, operand_index, 0, 0, 0);
    cost = loop_depth_weight((unsigned)depth) * cost + variable->spill_cost;
    variable->spill_cost = (short)(cost > 30000 ? 30000 : cost);
}

/* Original [0002dfb0,0002e070). A virtual byte operand cannot be colored
   ESI, EDI or an available EBP, since those registers have no byte names. */
void __fastcall record_special_register_conflicts(MachineInstruction *instruction,
    RegisterConflictVisitor visitor)
{
    RegisterVariable *variable;
    MachineOperand *operand;
    int index;
    for (index = 0; index < machine_operand_counts[(instruction->opcode >> 8) & 255]; ++index) {
        operand = &instruction->operands[index];
        if ((operand->flags & 15) != OPERAND_REGISTER ||
            !(operand->value.reg.words.number & 0xc000) ||
            machine_operand_bytes(instruction, index) != 1) continue;
        variable = lookup_register_variable(operand->value.reg.words.number, 1);
        visitor(variable, lookup_register_variable(REG_ESI, 1));
        visitor(variable, lookup_register_variable(REG_EDI, 1));
        if (frame_register_available) visitor(variable, lookup_register_variable(REG_EBP, 1));
    }
}

/* Original [0002dc30,0002dcf0). Implicitly modified registers conflict
   with the current live list and receive maximum spill cost. The record's
   second operand word marks the implicit values whose lifetime ends here. */
void __fastcall record_implicit_register_conflicts(MachineInstruction *instruction,
    RegisterConflictVisitor visitor)
{
    unsigned mask, bit;
    unsigned short number;
    mask = implicit_register_mask(instruction, 2);
    /* Native property0x40 is represented by the contextual implicit bit. */
    if (!(machine_selection_properties[(instruction->opcode >> 8) & 255] & MC_IMPLICIT_REGISTERS)) return;
    for (number = 0; number < 31; ++number) {
        bit = 1U << number;
        if (!(mask & bit)) continue;
        conflict_with_live_registers(number, 0xffff, visitor);
        allocation_live_register_count += add_allocation_live_register(&allocation_live_registers, number);
        physical_register_costs[number] = 30000;
    }
    for (number = 0; number < 31; ++number) {
        if (instruction->operands[1].flags & (1U << number))
            allocation_live_register_count -= remove_allocation_live_register(&allocation_live_registers, number);
    }
}

/* Original [0002dd40,0002ddc0). Fixed stack/frame and noninteger physical
   registers are excluded. Two fixed physical registers need no graph edge;
   the optional excluded register is the source of a coalescible transfer. */
void __fastcall conflict_with_live_registers(unsigned short number, unsigned short exclude,
    RegisterConflictVisitor visitor)
{
    RegisterVariable *variable;
    RegisterConflict *live;
    unsigned short other;
    if ((number > 7 && number <= 31) || number == REG_EBP || number == REG_ESP) return;
    variable = lookup_register_variable(number, 1);
    for (live = allocation_live_registers; live; live = live->next) {
        other = live->variable->number;
        if (!(number & 0xc000) && !(other & 0xc000)) continue;
        if (number != other && exclude != other) visitor(variable, live->variable);
    }
}

/* Original [0002dbb0,0002dc30). Keep one arena3 live-list node per
   allocatable integer register. The result contributes to the live count. */
int __fastcall add_allocation_live_register(RegisterConflict **list, unsigned short number)
{
    RegisterConflict *live, *head;
    if ((number > 7 && number <= 31) || number == REG_EBP || number == REG_ESP) return 0;
    head = *list;
    for (live = head; live; live = live->next)
        if (live->variable->number == number) return 0;
    live = arena_alloc(3, sizeof(RegisterConflict));
    live->variable = lookup_register_variable(number, 1);
    live->next = head;
    *list = live;
    return 1;
}

/* Original [0002dcf0,0002dd40). Unlink a last use without individually
   freeing its arena-owned node. Return whether the live count decreases. */
int __fastcall remove_allocation_live_register(RegisterConflict **list, unsigned short number)
{
    RegisterConflict *live;
    if ((number > 7 && number <= 31) || number == REG_EBP || number == REG_ESP) return 0;
    for (; (live = *list) != 0; list = &live->next) {
        if (live->variable->number == number) {
            *list = live->next;
            return 1;
        }
    }
    return 0;
}

/* Original [000592d0,000592f0). Four contextual depth weights at836f8:
   1,5,25,125. Beyond depth3 the native schedule grows linearly from125. */
int __fastcall loop_depth_weight(unsigned depth)
{
    static const int weights[4] = { 1, 5, 25, 125 };
    return depth < 4 ? weights[depth] : (int)(125U + depth);
}

/* Original [0002bcd0,0002bd30). Preserve one edge in each direction and
   each endpoint's short degree; existing pointer-identical edges are ignored. */
int __fastcall add_register_conflict(RegisterVariable *left, RegisterVariable *right)
{
    RegisterConflict *edge, *left_edges;
    left_edges = left->conflicts;
    for (edge = left_edges; edge; edge = edge->next)
        if (edge->variable == right) return 0;
    edge = new_register_conflict(4);
    edge->variable = left;
    edge->next = right->conflicts;
    right->conflicts = edge;
    ++right->degree;
    edge = new_register_conflict(4);
    edge->variable = right;
    edge->next = left_edges;
    left->conflicts = edge;
    ++left->degree;
    return 1;
}

/* Original [0002e0d0,0002e0e0). The native leaf tail-calls the shared
   allocator for one8-byte conflict link in the requested arena. */
RegisterConflict *__fastcall new_register_conflict(unsigned arena)
{
    return arena_alloc(arena, sizeof(RegisterConflict));
}

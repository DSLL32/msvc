/* Copy affinities and interference-preserving register coalescence. */
#include "physical-allocation.h"
RegisterAffinity *register_affinities, *free_register_affinities;

/* Original [0002bb10,0002bb50). Build copy affinities once, then sort
   and coalesce one admissible pair at a time until no merge remains. */
void build_allocation_constraints(void)
{
    register_affinities = 0;
    free_register_affinities = 0;
    collect_register_affinities(current_work);
    do {
        sort_register_affinities();
    } while (coalesce_register_affinities(register_affinities));
}

/* Original [0002e2c0,0002e390). Prefer eliminating register MOVs and
   simple LEAs. Zero displacement has weight8, magnitude1 has weight4,
   and larger displacements have weight1, all scaled by block nesting. */
void __fastcall collect_register_affinities(WorkItem *work)
{
    MachineInstruction *instruction;
    MachineAddress *address;
    unsigned operation, magnitude;
    int weight, scale;
    for (; work; work = work->next) {
        weight = loop_depth_weight((unsigned)(int)(short)work->region_depth);
        for (instruction = work->body.block->next; (instruction->opcode & 15) != 7;
             instruction = instruction->next) {
            if (instruction->opcode & 15) continue;
            operation = (instruction->opcode >> 8) & 255;
            if (operation == MC_MOV &&
                (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                (instruction->operands[1].flags & 15) == OPERAND_REGISTER) {
                add_register_affinity(instruction->operands[0].value.reg.words.number,
                    instruction->operands[1].value.reg.words.number, weight * 8);
            } else if (operation == MC_LEA &&
                (instruction->operands[1].flags & 15) == OPERAND_BASED) {
                address = instruction->operands[1].value.address;
                if (address->symbol) continue;
                magnitude = (unsigned)address->first.displacement;
                if (address->first.displacement < 0) magnitude = 0U - magnitude;
                scale = magnitude == 0 ? 8 : magnitude == 1 ? 4 : 1;
                add_register_affinity(instruction->operands[0].value.reg.words.number,
                    address->base.words.number, weight * scale);
            }
        }
    }
}

/* Original [0002e480,0002e550). Resolve current representatives, reject
   fixed or already interfering pairs, and accumulate their copy benefit. */
void __fastcall add_register_affinity(unsigned short left, unsigned short right, int benefit)
{
    RegisterVariable *left_variable, *right_variable;
    RegisterAffinity *affinity;
    left = lookup_register_variable(left, 1)->representative;
    if ((left > 7 && left <= 31) || left == REG_EBP || left == REG_ESP) return;
    right = lookup_register_variable(right, 1)->representative;
    if ((right > 7 && right <= 31) || right == REG_EBP || right == REG_ESP || right == left) return;
    if (!(left & 0xc000) && !(right & 0xc000)) return;
    left_variable = lookup_register_variable(left, 1);
    if (left_variable->flags & 0x400) return;
    right_variable = lookup_register_variable(right, 1);
    if (right_variable->flags & 0x400) return;
    if (registers_conflict(left_variable, right_variable)) return;
    affinity = new_register_affinity(left_variable, right_variable);
    affinity->benefit = benefit;
    merge_register_affinity(&register_affinities, affinity);
}

/* Original [0002e600,0002e640). Search the shorter adjacency list,
   comparing canonical register numbers just as the native predicate does. */
int __fastcall registers_conflict(RegisterVariable *left, RegisterVariable *right)
{
    RegisterConflict *edge;
    unsigned short number;
    if (right->degree < left->degree) {
        edge = right->conflicts;
        number = left->number;
    } else {
        edge = left->conflicts;
        number = right->number;
    }
    for (; edge; edge = edge->next)
        if (edge->variable->number == number) return 1;
    return 0;
}

/* Original [0002e5a0,0002e600). Reuse a16-byte affinity record, canonicalize
   its endpoint order by unsigned register number, and clear its list/weight. */
RegisterAffinity *__fastcall new_register_affinity(RegisterVariable *left, RegisterVariable *right)
{
    RegisterAffinity *affinity;
    if (free_register_affinities) {
        affinity = free_register_affinities;
        free_register_affinities = affinity->next;
    } else affinity = arena_alloc(4, sizeof(RegisterAffinity));
    if (right->number < left->number) {
        affinity->left = right;
        affinity->right = left;
    } else {
        affinity->left = left;
        affinity->right = right;
    }
    affinity->benefit = 0;
    affinity->next = 0;
    return affinity;
}

/* Original [0002e550,0002e5a0). Repeated canonical endpoint pairs share
   their benefit and recycle the redundant node; new pairs prepend once. */
RegisterAffinity *__fastcall merge_register_affinity(RegisterAffinity **list, RegisterAffinity *affinity)
{
    RegisterAffinity *existing, *head;
    head = *list;
    for (existing = head; existing; existing = existing->next) {
        if (existing->left == affinity->left && existing->right == affinity->right) {
            existing->benefit += affinity->benefit;
            release_register_affinity(affinity);
            return existing;
        }
    }
    affinity->next = head;
    *list = affinity;
    return affinity;
}

/* Original [0002e280,0002e2c0). Rebuild a descending benefit list and
   recycle entries that have lost their benefit after a failed merge. */
void sort_register_affinities(void)
{
    RegisterAffinity *affinity, *next;
    affinity = register_affinities;
    register_affinities = 0;
    for (; affinity; affinity = next) {
        next = affinity->next;
        if (affinity->benefit > 0) insert_register_affinity(&register_affinities, affinity);
        else release_register_affinity(affinity);
    }
}

/* Original [0002e680,0002e6a0). Descending signed benefit insertion,
   placing a new entry before existing entries of equal benefit. */
void __fastcall insert_register_affinity(RegisterAffinity **list, RegisterAffinity *affinity)
{
    RegisterAffinity *existing;
    for (; (existing = *list) != 0; list = &existing->next)
        if (existing->benefit <= affinity->benefit) break;
    affinity->next = *list;
    *list = affinity;
}

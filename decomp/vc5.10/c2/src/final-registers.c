#include "functions.h"
#include "function-records.h"
#include "code-input.h"
#include "optimizer.h"
#include "global-optimizer.h"
#include "optimizer-sets.h"
#include "register-sets.h"
#include "registers.h"
#include "inlining.h"
#include <recon.h>

extern C2FunctionRegister *optimizer_memory_register;

typedef struct C2AllocationRegister C2AllocationRegister;
typedef struct C2AllocationPreference {
    struct C2AllocationPreference *next;
    C2FunctionRegister *physical;
    unsigned count;
} C2AllocationPreference;
typedef char C2AllocationPreferenceSizeCheck[sizeof(C2AllocationPreference) == 12 ? 1 : -1];
struct C2AllocationRegister {
    C2FunctionRegister *source;
    unsigned char kind, flags, attributes, coalesce_rank;
    C2AllocationRegister *canonical;
    union { unsigned char pending0c[16];
        struct { union { unsigned pending0c_word; int weighted_pressure; };
            union { unsigned pending10_word, spill_mask; C2FunctionRegister *pinned_register; };
            union { C2AllocationRegister *next_active, *float_next; C2Expression *last_use; };
            union { C2AllocationRegister *previous_active, *float_previous; int reference_cost; }; }; };
    unsigned number;
    C2RegisterSet *available;
    union { unsigned char pending24[8]; struct { int use_count; union { int degree; C2BasicBlock *rewrite_block, *first_block; }; }; };
    C2AllocationRegister *next_hash;
    union {
        unsigned char pending30[20];
        struct {
            union { C2BasicBlock *last_block; int remaining_degree; };
            union { unsigned pending34_word; C2AllocationPreference *preferences; struct C2AllocationMoveLink *float_definitions; };
            union { unsigned pending38_word; C2Expression *definition_record; struct C2AllocationMoveLink *float_uses; };
            int priority;
            unsigned definition_sequence;
        };
    };
};
typedef char C2AllocationRegisterSizeCheck[sizeof(C2AllocationRegister) == 68 ? 1 : -1];
typedef char C2AllocationRegisterNumberCheck[offsetof(C2AllocationRegister, number) == 28 ? 1 : -1];
typedef char C2AllocationRegisterAvailableCheck[offsetof(C2AllocationRegister, available) == 32 ? 1 : -1];
typedef char C2AllocationRegisterHashCheck[offsetof(C2AllocationRegister, next_hash) == 44 ? 1 : -1];
C2RegisterSet *allocation_register_classes[2]; /* Original494240/44. */
#define allocation_integer_registers allocation_register_classes[0]
#define allocation_float_registers allocation_register_classes[1]
C2AllocationRegister *free_allocation_registers; /* 493230 */
unsigned next_allocation_register_number;       /* 494238 */
unsigned last_allocation_register_number;       /* 495fb8 */
C2FlowRegion *allocation_region_chain;          /* 49424c */
int allocation_machine_initialized;             /* 495fbc: original DWORD zero */

/* These two register classes and call-preservation sets are initialized by
 * the original routine; no neighboring static table cells are represented. */
C2RegisterSet *allocation_class_registers[2];     /* 494380/84 */
C2RegisterSet *allocation_call_clobbered;         /* 4931c0 */
C2RegisterSet *allocation_call_preserved;         /* 4931c8 */

void initialize_allocation_machine(void)
{
    C2RegisterSet *integer_registers = new_register_set(0);

    add_register_to_set(integer_registers, 1);
    add_register_to_set(integer_registers, 2);
    add_register_to_set(integer_registers, 3);
    add_register_to_set(integer_registers, 4);
    add_register_to_set(integer_registers, 6);
    add_register_to_set(integer_registers, 7);
    add_register_to_set(integer_registers, 8);
    allocation_class_registers[0] = integer_registers;
    allocation_class_registers[1] = new_register_set(0);
    allocation_call_clobbered = new_register_set(0);
    add_register_to_set(allocation_call_clobbered, 1);
    add_register_to_set(allocation_call_clobbered, 2);
    add_register_to_set(allocation_call_clobbered, 3);
    add_register_to_set(allocation_call_clobbered, 4);
    allocation_call_preserved = new_register_set(0);
    add_register_to_set(allocation_call_preserved, 6);
    add_register_to_set(allocation_call_preserved, 7);
    add_register_to_set(allocation_call_preserved, 8);
}
/* The complete 1024-head span is established by the real reset, not by
 * an inferred static NULL default. Nodes remain opaque until their factory. */
C2AllocationRegister *allocation_register_hash[1024]; /* 493238..494238 */
void reset_allocation_register_hash(void)
{
    memset(allocation_register_hash, 0, sizeof(allocation_register_hash));
}
C2FunctionRegister *allocation_class_heads[64]; /* 494250..494350 */
extern int c2_favor_speed;
C2Expression *__fastcall find_final_register_use(C2FunctionRegister *reg,
    C2Expression *owner);
C2Expression *__fastcall final_register_has_other_use(C2FunctionRegister *reg,
    C2Expression *owner)
{
    C2Expression *operand;

    /* Return the actual matching input, not a normalized Boolean. */
    for (operand = owner->left; operand; operand = operand->next)
        if (operand->kind == 1 && operand->original_register == reg) break;
    return operand;
}
/* Static machine rows are only partially reconstructed. This guard precedes
 * the newly reached kind-column reads, rather than treating reserved backing
 * as known zero. Dynamic function registers have their real initialized kinds. */
#define ALLOCATION_KNOWN_KIND(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[67]) { __asm { int 3 } } } while (0)
#define ALLOCATION_KNOWN_NUMBER(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)



int __fastcall final_record_needs_allocation(C2Expression *record)
{
    C2Expression *input, *output;

    switch (record->opcode) {
    case 1:
        input = record->left;
        output = record->tail.right;
        if (input->kind != 7 || input->original_register || output->kind != 1)
            return 1;
        ALLOCATION_KNOWN_KIND(output->original_register);
        return output->original_register->kind == 2;
    case 0x2f:
        input = record->left;
        if (input->next->kind != 7 || input->next->original_register || input->kind != 1)
            return 1;
        ALLOCATION_KNOWN_KIND(input->original_register);
        return input->original_register->kind == 2;
    case 0x2d: case 0x32:
        output = record->tail.right;
        if (output->kind != 1) return 1;
        return output->original_register != (C2FunctionRegister *)&machine_registers[6] &&
            output->original_register != (C2FunctionRegister *)&machine_registers[5];
    case 4: case 6: case 8: case 0xa: case 0xb:
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a: case 0x2b:
    case 0xa3: case 0xc1: case 0xca: case 0xcb: case 0x109: case 0x13c: case 0x14c:
        return 0;
    default:
        return 1;
    }
}
typedef struct C2AllocationClass {
    struct C2AllocationClass *next;
    C2RegisterSet *numbers;
} C2AllocationClass;
typedef char C2AllocationClassSizeCheck[sizeof(C2AllocationClass) == 8 ? 1 : -1];
struct C2AllocationInfo {
    unsigned number;
    C2RegisterSet *pinned_registers;
    unsigned definition_count;
    union { C2AllocationRegister *assigned_register; C2AllocationClass *classes; };
    union { C2Expression *last_definition; C2FunctionRegister *active_next; };
    unsigned class_number;
};
typedef char C2AllocationInfoSizeCheck[sizeof(struct C2AllocationInfo) == 24 ? 1 : -1];
typedef char C2AllocationInfoAssignedCheck[offsetof(struct C2AllocationInfo, assigned_register) == 12 ? 1 : -1];
void __fastcall insert_allocation_register(C2AllocationRegister *reg)
{
    unsigned bucket = reg->number & 1023;
    C2AllocationRegister *previous = allocation_register_hash[bucket];

    allocation_register_hash[bucket] = reg;
    reg->next_hash = previous;
}
C2AllocationRegister *__fastcall get_allocation_register(C2FunctionRegister *source)
{
    C2AllocationRegister *reg;
    unsigned allocation_class;

    if (source->allocation_root) source = source->allocation_root;
    if (free_allocation_registers) {
        reg = free_allocation_registers;
        free_allocation_registers = reg->next_hash;
    } else {
        reg = c2_pool_allocate(4, sizeof(C2AllocationRegister));
        reg->number = next_allocation_register_number++;
    }
    reg->canonical = reg;
    reg->kind = 2;
    reg->source = source;
    reg->available = new_register_set(6);
    reg->flags |= 1;
    /* Original494350 DWORD rows1/2 select integer class0; row4 selects
     * floating class1. Other original classes remain unmodeled. */
    switch (source->type >> 12) {
    case 1: case 2: allocation_class = 0; break;
    case 4: allocation_class = 1; break;
    default: __asm { int 3 } return NULL;
    }
    merge_optimizer_symbol_sets(reg->available, allocation_class_registers[allocation_class]);
    add_register_to_set(allocation_register_classes[allocation_class], reg->number);
    insert_allocation_register(reg);
    return reg;
}

C2Expression *__fastcall number_allocation_operand(C2Expression *record,
    C2Expression *operand, unsigned *count)
{
    C2FunctionRegister *reg;
    C2Expression *prior, *replacement;
    unsigned bucket;

    if (operand->kind == 7) bucket = operand->integer.low & 63;
    else if (operand->kind == 3)
        bucket = operand->value.reg->canonical->symbol->serial & 63;
    else bucket = operand->value.symbol->serial & 63;
    for (reg = allocation_class_heads[bucket]; reg; reg = reg->allocation_next) {
        prior = reg->allocation_operand;
        if (prior->kind != operand->kind) continue;
        if (operand->kind == 7) {
            if (prior->integer.low == operand->integer.low) break;
        } else if (operand->kind == 3) {
            if (prior->value.reg == operand->value.reg) break;
        } else if (prior->value.symbol == operand->value.symbol) break;
    }
    if (reg) reg->view_flags |= 0x10;
    else {
        reg = c2_pool_allocate(1, sizeof(C2FunctionRegister));
        reg->kind = 13;
        reg->type = 0x1004;
        reg->allocation_operand = copy_input_expression(operand);
        reg->canonical = reg;
        reg->value_bytes = 4;
        reg->allocation = c2_pool_allocate(4, sizeof(struct C2AllocationInfo));
        reg->allocation->number = (*count)++;
        reg->allocation_next = allocation_class_heads[bucket];
        allocation_class_heads[bucket] = reg;
        reg->allocation->assigned_register = get_allocation_register(reg);
    }
    replacement = new_temporary_register_expression(0x11f, operand->type, reg,
        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT));
    return attach_input_expression(record, operand, replacement);
}


unsigned __fastcall number_allocation_registers(C2Expression *first)
{
    C2Expression *record, *output, *input, *address, *use;
    C2FunctionRegister *reg, *root, *original;
    unsigned count = 1;

    memset(allocation_class_heads, 0, sizeof(allocation_class_heads));
    for (record = first; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        output = record->tail.right;
        for (use = output; use; use = use->next) {
            if ((use->type & 0xfff) == 4) continue;
            if (use->kind == 1) {
                /* These fixed rows have independently proved kind1 bytes.
                 * This comparison consumes no allocation/range payload. */
                if (use->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    use->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    use->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    use->original_register != (C2FunctionRegister *)&machine_registers[74])
                    ALLOCATION_KNOWN_KIND(use->original_register);
                if (use->original_register->kind != 2) continue;
            } else if (use->kind != 2) continue;
            reg = use->value.reg;
            root = reg->canonical;
            if ((unsigned long)root >= (unsigned long)machine_registers &&
                (unsigned long)root < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
                root != (C2FunctionRegister *)&machine_registers[1] &&
                root != (C2FunctionRegister *)&machine_registers[2] &&
                root != (C2FunctionRegister *)&machine_registers[18] &&
                root != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
            ALLOCATION_KNOWN_KIND(reg);
            if ((long)(use->type & 0xfff) < (long)root->value_bytes && reg->kind != 3)
                root->view_flags |= 0x80;
        }
        if (record->opcode == 0x12) {
            /* Reached ESI/EDI outputs have their already proved kind1 bytes.
             * Admit only this local kind comparison; other guarded consumers
             * and all unrelated static-row fields keep their own limits. */
            if (output->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                output->original_register != (C2FunctionRegister *)&machine_registers[8])
                ALLOCATION_KNOWN_KIND(output->original_register);
            if (output->original_register->kind != 2) continue;
            input = record->left;
            if (input->opcode == 0x122) address = input->aggregate_value;
            else if (c2_favor_speed && input->opcode == 0x125)
                address = input->aggregate_second;
            else continue;
            if (!((1U << (address->kind & 31)) & 6)) continue;
            reg = address->value.reg;
            if (!reg->value) continue;
            original = address->original_register;
            ALLOCATION_KNOWN_KIND(original);
            if (original->kind != 3 || find_final_register_use(original, record) ||
                final_register_has_other_use(original, reg->value)) continue;
            root = reg->canonical;
            /* The static rows' variant-link payload is not yet reconstructed. */
            if ((unsigned long)root >= (unsigned long)machine_registers &&
                (unsigned long)root < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                { __asm { int 3 } }
            if (root->variant_next) continue;
            use = find_final_register_use(original, reg->value);
            address = input->opcode == 0x122 ? input->aggregate_value : input->aggregate_second;
            use->original_register = (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT);
            reg->value = NULL;
            address->original_register = (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT);
        } else {
            for (input = record->left; input; input = input->next) {
                if (input->kind == 7) {
                    if (final_record_needs_allocation(record))
                        input = number_allocation_operand(record, input, &count);
                } else if (((1U << (input->kind & 31)) & 0x18) &&
                    final_record_needs_allocation(record) &&
                    record->kind != 13 && record->kind != 16) {
                    input = number_allocation_operand(record, input, &count);
                }
            }
        }
    }
    return count;
}
C2FunctionRegister **allocation_registers_by_number; /* 494248 */
extern int c2_float_precision;

unsigned __fastcall prepare_allocation_registers(unsigned count)
{
    C2RegisterBlock *block;
    C2FunctionRegister *root, *view, *candidate, *best;
    C2RegisterSet *pinned_registers = NULL;
    unsigned attributes;
    long candidate_end, view_end;
    int pinned;

    for (block = first_function_register_block; block; block = block->next) {
        for (root = block->records; root < block->cursor; ++root) {
            if (root->kind == 16 || root->canonical != root) continue;
            pinned = 0;
            switch (root->kind) {
            case 3:
                if (root->value) goto clear_chain;
                root->flags &= ~2;
                break;
            case 4:
                root->allocation_next = NULL;
                /* FALLTHROUGH */
            case 5:
                if (root->view_flags & 2) pinned = 1;
                break;
            case 7: case 8:
                pinned = 1;
                break;
            case 10:
                attributes = ((C2InputSymbol *)root->symbol)->attributes;
                if (!(attributes & 0x1000) || (attributes & 0x4000)) goto clear_chain;
                for (view = root; view; view = view->variant_next) {
                    if (view->value_bytes == 4) {
                        view->allocation = c2_pool_allocate(4, sizeof(struct C2AllocationInfo));
                        view->allocation->number = count++;
                    } else view->allocation = NULL;
                    view->allocation_root = NULL;
                }
                continue;
            default:
clear_chain:
                for (view = root; view; view = view->variant_next) {
                    view->allocation = NULL;
                    view->allocation_root = NULL;
                }
                continue;
            }
            for (view = root; view; view = view->variant_next) {
                view->allocation_root = NULL;
                if (!global_type_has_register_class(view->type) ||
                    ((view->type & 0xf000) == 0x4000 && c2_float_precision)) {
                    view->allocation = NULL;
                    continue;
                }
                if ((long)view->value_bytes < 4 &&
                    ((long)view->value_bytes < (long)root->value_bytes || view->kind == 3) &&
                    !(root->view_flags & 0x80)) {
                    best = NULL;
                    for (candidate = root; candidate; candidate = candidate->variant_next) {
                        if (candidate == view || !global_type_has_register_class(candidate->type) ||
                            (candidate->type & 0xf000) == 0x4000) continue;
                        candidate_end = (long)((unsigned)candidate->displacement + candidate->value_bytes);
                        view_end = (long)((unsigned)view->displacement + view->value_bytes);
                        if (candidate->displacement <= view->displacement &&
                            view_end <= candidate_end &&
                            (long)view->value_bytes < (long)candidate->value_bytes &&
                            (view->displacement == candidate->displacement ||
                             (view->displacement == (long)((unsigned)candidate->displacement + 1) &&
                              view->value_bytes == 1)) &&
                            (!best || (long)candidate->value_bytes < (long)best->value_bytes))
                            best = candidate;
                    }
                    if (best) {
                        view->allocation = NULL;
                        view->allocation_root = best;
                        continue;
                    }
                }
                if (pinned) {
                    if (!pinned_registers) pinned_registers = new_register_set(6);
                    add_register_to_set(pinned_registers, count);
                }
                view->allocation = c2_pool_allocate(4, sizeof(struct C2AllocationInfo));
                view->allocation->number = count++;
            }
            if (pinned_registers) {
                if (!root->allocation)
                    root->allocation = c2_pool_allocate(4, sizeof(struct C2AllocationInfo));
                root->allocation->pinned_registers = pinned_registers;
                pinned_registers = NULL;
            }
            root->view_flags &= ~0x80;
        }
    }
    allocation_registers_by_number = c2_pool_allocate(4,
        count * sizeof(*allocation_registers_by_number));
    return count;
}
/* Consumed bit0x40 in the same encountered opcode-property column. */
#define ALLOCATION_PAIRED_OPCODE(op) \
    ((op) == 9 || (op) == 0x1e || (op) == 0x21 || \
     ((op) >= 0x23 && (op) <= 0x2e) || ((op) >= 0x30 && (op) <= 0x32) || \
     (op) == 0x34 || ((op) >= 0x45 && (op) <= 0x51) || \
     ((op) >= 0x54 && (op) <= 0x55) || ((op) >= 0x57 && (op) <= 0x59) || \
     ((op) >= 0x76 && (op) <= 0x77) || (op) == 0x8c || (op) == 0x9f || \
     (op) == 0xc1 || ((op) >= 0xca && (op) <= 0xcb) || \
     ((op) >= 0xd6 && (op) <= 0xd7) || ((op) >= 0xd9 && (op) <= 0xda))

void __fastcall finish_allocation_record(C2Expression *record)
{
    C2Expression *first, *second, *output;

    if (!ALLOCATION_PAIRED_OPCODE(record->opcode)) return;
    first = record->left;
    if (first->kind == 1) {
        /* The reached stack-register row5 already has its proved kind1. */
        if (first->original_register != (C2FunctionRegister *)&machine_registers[5] &&
            first->original_register != (C2FunctionRegister *)&machine_registers[31] &&
            first->original_register != (C2FunctionRegister *)&machine_registers[32])
            ALLOCATION_KNOWN_KIND(first->original_register);
        if (first->original_register->kind == 2) {
            output = record->tail.right;
            output->location.expression.properties =
                (output->location.expression.properties & 0xf1) | 1;
            first->location.expression.properties =
                (first->location.expression.properties & 0xf1) | 1;
        }
    }
    if (record->opcode == 9 || record->opcode == 0xd9) {
        second = first->next;
        if (second->original_register != (C2FunctionRegister *)&machine_registers[31] &&
            second->original_register != (C2FunctionRegister *)&machine_registers[32])
            ALLOCATION_KNOWN_KIND(second->original_register);
        if (second->original_register->kind == 2) {
            output = record->tail.right->next;
            output->location.expression.properties =
                (output->location.expression.properties & 0xf2) | 2;
            second->location.expression.properties =
                (second->location.expression.properties & 0xf2) | 2;
        }
    }
}
C2Expression *__fastcall materialize_allocation_input(C2Expression *record,
    C2Expression *operand, C2FunctionRegister *reg)
{
    C2Expression *source = reg->allocation_operand, *replacement;

    if (source->kind == 7)
        replacement = new_integer_expression(operand->type, source->integer.value);
    else
        replacement = copy_input_expression(source);
    return attach_input_expression(record, operand, replacement);
}

/* The already recovered bit0 column is consumed only at opcodes1..0x11a.
 * Express its two semantic ranges rather than copying the packed WORD table. */
#define ALLOCATION_DESTRUCTIVE_OPCODE(op) \
    (((op) >= 0x45 && (op) <= 0x94) || ((op) >= 0xd1 && (op) <= 0xd7))
#define ALLOCATION_RECORD_CONSTRAINED(record) \
    (((record)->type & 0xf000) == 0x4000 || ALLOCATION_DESTRUCTIVE_OPCODE((record)->opcode))

void __fastcall collect_allocation_uses(C2BasicBlock *block, unsigned count)
{
    C2RegisterSet *live_in, *uses, *definitions;
    C2Expression *record, *operand, *output;
    C2FunctionRegister *reg, *alias;
    C2OptimizerUse *use, *saved;
    unsigned number;
    unsigned char properties;
    long reg_end, alias_end;

    for (; block; block = block->previous) {
        live_in = new_register_set(6);
        uses = new_register_set(6);
        definitions = new_register_set(6);
        for (record = block->last_record; record != block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            if (record->kind == 19 || record->opcode == 0x15b) {
                switch (record->opcode) {
                case 0x15b: case 0x163: case 0x165: case 0x167:
                case 0x169: case 0x16b: case 0x172:
                    fill_global_available_set(definitions, count + 1);
                    clear_register_set(uses);
                    break;
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    operand->location.expression.properties &= 0xf0;
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) break;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    number = reg->allocation->number;
                    allocation_registers_by_number[number] = reg;
                    if (ALLOCATION_RECORD_CONSTRAINED(record)) {
                        make_global_register_operand(operand);
                        add_register_to_set(definitions, number);
                        if (reg->kind == 3) reg->flags |= 2;
                    } else if (!optimizer_set_contains(definitions, number)) {
                        add_register_to_set(live_in, number);
                        add_register_to_set(definitions, number);
                    }
                    remove_optimizer_symbol(uses, number);
                    goto output_aliases;
                case 2:
                    reg = operand->value.reg;
                    if (reg->allocation && reg->allocation->number) {
                        number = reg->allocation->number;
                        if ((operand->location.expression.properties & 0x40) ||
                            (ALLOCATION_RECORD_CONSTRAINED(record) &&
                             (reg->type & 0xf000) != 0x4000)) {
                            add_register_to_set(definitions, number);
                            remove_optimizer_symbol(uses, number);
                        } else {
                            materialize_global_register_operand(operand,
                                (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT));
                            operand->location.expression.properties &= 0xf0;
                            number = reg->allocation->number;
                            allocation_registers_by_number[number] = reg;
                            if (!optimizer_set_contains(definitions, number)) {
                                add_register_to_set(live_in, number);
                                add_register_to_set(definitions, number);
                            }
                            remove_optimizer_symbol(uses, number);
                        }
                    }
output_aliases:
                    alias = reg->canonical;
                    /* Static variant links remain unmodeled. */
                    if ((unsigned long)alias >= (unsigned long)machine_registers &&
                        (unsigned long)alias < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                        { __asm { int 3 } }
                    if (alias->variant_next) {
                        for (; alias; alias = alias->variant_next) {
                            if (alias == reg) continue;
                            reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                            alias_end = (long)((unsigned)alias->displacement + alias->value_bytes);
                            if (reg->displacement < alias_end && alias->displacement < reg_end &&
                                alias->allocation && alias->allocation->number) {
                                number = alias->allocation->number;
                                add_register_to_set(definitions, number);
                                remove_optimizer_symbol(uses, number);
                            }
                        }
                    }
                    break;
                case 6: case 10:
                    use = get_global_index_uses(operand->kind == 6 ?
                        operand->immediate : operand->value.optimizer_index);
                    saved = NULL;
                    if (use) {
next_index_use:
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            use = reg->optimizer_uses;
                            if (!use) {
                                use = saved->next;
                                goto resume_index_uses;
                            }
                            reg = use->reg;
                        }
                        if (reg->allocation && reg->allocation->pinned_registers) {
                            merge_optimizer_symbol_sets(definitions, reg->allocation->pinned_registers);
                            subtract_optimizer_symbol_sets(uses, reg->allocation->pinned_registers);
                        }
                        use = use->next;
                        if (use) goto next_index_use;
                        if (saved) {
                            use = saved->next;
                            saved = NULL;
                        }
resume_index_uses:
                        if (use) goto next_index_use;
                    }
                    break;
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    properties = operand->location.expression.properties;
                    operand->location.expression.properties &= 0xf0;
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) continue;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    number = reg->allocation->number;
                    allocation_registers_by_number[number] = reg;
                    if (reg->kind == 13 && !(reg->view_flags & 0x10) &&
                        (!block->block_scope || !c2_favor_speed)) {
                        operand = materialize_allocation_input(record, operand, reg);
                    } else if (ALLOCATION_RECORD_CONSTRAINED(record) && !(properties & 0x20)) {
                        make_global_register_operand(operand);
                        if (reg->kind == 3) reg->flags |= 2;
                    } else {
                        if (!optimizer_set_contains(definitions, number))
                            add_register_to_set(live_in, number);
                        add_register_to_set(uses, number);
                    }
                } else if (operand->kind == 2) {
                    reg = operand->value.reg;
                    if (!reg->allocation || !reg->allocation->number ||
                        (operand->location.expression.properties & 0x40)) continue;
                    if (ALLOCATION_RECORD_CONSTRAINED(record) &&
                        !(operand->location.expression.properties & 0x20) &&
                        (reg->type & 0xf000) != 0x4000) {
                        if (reg->kind == 3) reg->flags |= 2;
                    } else {
                        output = record->tail.right;
                        if (output && output->kind == 2 &&
                            (output->location.expression.properties & 0x40) &&
                            output->value.reg == reg) continue;
                        materialize_global_register_operand(operand,
                            (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT));
                        operand->location.expression.properties &= 0xf0;
                        number = reg->allocation->number;
                        allocation_registers_by_number[number] = reg;
                        if (!optimizer_set_contains(definitions, number))
                            add_register_to_set(live_in, number);
                        add_register_to_set(uses, number);
                    }
                }
            }
            finish_allocation_record(record);
        }
        block->allocation_uses = uses;
        block->allocation_live_in = live_in;
        block->allocation_definitions = definitions;
    }
}
void __fastcall intersect_block_allocation_sets(C2BasicBlock *block)
{
    for (; block; block = block->previous) {
        intersect_optimizer_symbol_sets(block->allocation_live_out, block->alias_in);
        intersect_optimizer_symbol_sets(block->allocation_live_in_after, block->alias_out);
    }
}
C2RegisterSet *__fastcall clone_optimizer_dense_set(C2RegisterSet *source);
unsigned __fastcall next_allocation_dense_member(C2RegisterSet *set);
int __fastcall allocation_spill_path_is_available(unsigned number, C2BasicBlock *from,
    C2BasicBlock *to, C2RegisterSet *visited)
{
    C2FlowEdge *edge;
    C2BasicBlock *predecessor;
    unsigned block_number;

    for (edge = from->incoming; edge; edge = edge->next_incoming) {
        predecessor = edge->source;
        if (predecessor == to) continue;
        block_number = (unsigned)(int)(short)predecessor->number84;
        if (optimizer_set_contains(visited, block_number)) continue;
        if (optimizer_set_contains(predecessor->allocation_definitions, number)) return 0;
        add_register_to_set(visited, block_number);
        if (!allocation_spill_path_is_available(number, predecessor, to, visited)) return 0;
    }
    return 1;
}

int __fastcall allocation_spill_is_profitable(unsigned number, C2BasicBlock *from,
    C2BasicBlock *to)
{
    C2RegisterSet *visited = new_register_set(6);
    int available = allocation_spill_path_is_available(number, from, to, visited);
    release_optimizer_symbol_set(visited);
    return available;
}
/* The provisional new_allocation_reload name was an insertion-point
 * selector, not a constructor. EAX retains this exact selected record on
 * every original return path, while ECX supplies only the line source. */
C2Expression *__fastcall find_allocation_reload_position(C2BasicBlock *block,
    C2FunctionRegister *reg, C2Function *function)
{
    C2Expression *position = (C2Expression *)block->last_record->previous;
    C2Expression *scan, *output;
    unsigned short line;

    while (position->kind == 26) position = position->previous;
    if (position->kind != 14) {
        if (!((1U << (position->kind & 31)) & 0x50000)) position = position->next;
        else if (reg->kind == 13) {
            scan = position->previous;
            while (scan->opcode == 0x139) scan = scan->previous;
            output = scan->tail.right;
            if ((scan->flags & 1) && output && !output->next && output->kind == 1 &&
                output->value.reg->kind == 3 && output->value.reg->value) position = scan;
        }
    }
    if (position->flags & 1) line = position->location.line;
    else {
        scan = position->previous;
        while (scan && !(scan->flags & 1)) scan = scan->previous;
        if (!scan) {
            scan = position->next;
            while (!(scan->flags & 1)) scan = scan->next;
        }
        line = scan->location.line;
    }
    if (line) {
        record_line_delta = line;
        record_source_line = function->source_line_base + line;
    }
    return position;
}
C2Expression *__fastcall bind_allocation_spill(C2FunctionRegister *reg,
    C2FunctionRegister *original, C2Expression *replacement)
{
    C2Expression *operand, *temporary;
    unsigned opcode;
    unsigned short type;

    if (reg->kind == 13) {
        operand = reg->allocation_operand;
        if (operand->kind == 7) operand = new_integer_expression(0x1004, operand->integer.value);
        else operand = copy_input_expression(operand);
        type = 0x1004;
    } else {
        if (reg->allocation_root) reg = reg->allocation_root;
        type = reg->type;
        if (type == 0x3004) type = 0x1004;
        opcode = reg->kind >= 3 && reg->kind <= 6 ? 0x120 : 0x121;
        operand = new_typed_register_expression(opcode, type, reg);
    }
    temporary = new_temporary_register_expression(0x11f, type, reg, original);
    return new_binary_expression(0x139, type, temporary, operand,
        prepend_input_record, (C2Record *)replacement);
}
void __fastcall propagate_allocation_spill_edges(C2RegisterSet *spills,
    C2BasicBlock *from, C2BasicBlock *to, C2RegisterSet *visited)
{
    C2FlowEdge *edge;
    C2BasicBlock *predecessor;
    unsigned number;

    for (edge = from->incoming; edge; edge = edge->next_incoming) {
        predecessor = edge->source;
        if (predecessor == to) continue;
        number = (unsigned)(int)(short)predecessor->number84;
        if (optimizer_set_contains(visited, number)) continue;
        merge_optimizer_symbol_sets(predecessor->allocation_live_out, spills);
        merge_optimizer_symbol_sets(predecessor->allocation_live_in_after, spills);
        add_register_to_set(visited, number);
        propagate_allocation_spill_edges(spills, predecessor, to, visited);
    }
}

void __fastcall rewrite_allocation_spill_edges(C2RegisterSet *spills,
    C2BasicBlock *from, C2BasicBlock *to)
{
    C2RegisterSet *visited = new_register_set(6);
    propagate_allocation_spill_edges(spills, from, to, visited);
    release_optimizer_symbol_set(visited);
}

void __fastcall prepare_function_register_spills(C2Function *function)
{
    C2BasicBlock *block, *predecessor, *target;
    C2FlowEdge *edge;
    C2Expression *record, *replacement;
    C2FunctionRegister *reg;
    C2RegisterSet *needed, *dominators = NULL, *spills;
    unsigned number, member, common;
    int common_defined = 0;

    /* The original first empty enumeration would consume an unwritten local.
     * Later empty enumerations carry its previously written value. Do not
     * invent a default; deny only the first unwritten use in both builds. */
    needed = new_register_set(6);
    for (block = function->flow->first; block; block = block->next) {
        edge = block->incoming;
        if (!edge || !edge->next_incoming) continue;
        for (; edge; edge = edge->next_incoming) {
            predecessor = edge->source;
            if ((short)predecessor->number84 >= (short)block->number84) continue;
            copy_optimizer_symbol_set_into(needed, block->allocation_live_out);
            subtract_optimizer_symbol_sets(needed, predecessor->allocation_live_in_after);
            if (!needed->links.first) continue;
            if (!dominators) dominators = clone_optimizer_dense_set(block->dominators);
            else copy_optimizer_dense_set(dominators, block->dominators);
            intersect_optimizer_dense_sets(dominators, predecessor->dominators);
            member = next_allocation_dense_member(dominators);
            while (member != ~0U) {
                common = member;
                common_defined = 1;
                member = next_allocation_dense_member(NULL);
            }
            if (!common_defined) { __asm { int 3 } }
            if ((short)common >= (short)block->number84) continue;
            for (target = block->next; target && target->number84 != (unsigned short)common;
                 target = target->next) {
            }
            record = (C2Expression *)target->last_record->previous;
            if (record->kind == 16 && record->previous->kind == 24 && record->left->next)
                target = target->next;
            spills = NULL;
            for (number = next_optimizer_symbol(needed); number != ~0U;
                 number = next_optimizer_symbol(NULL)) {
                reg = allocation_registers_by_number[number];
                if (reg->kind != 13 && !allocation_spill_is_profitable(number, block, target))
                    continue;
                if (!spills) spills = new_register_set(6);
                add_register_to_set(spills, number);
                if (!optimizer_set_contains(target->allocation_live_in_after, number)) {
                    add_register_to_set(target->allocation_live_in_after, number);
                    add_register_to_set(target->allocation_definitions, number);
                    add_register_to_set(target->allocation_live_in, number);
                    replacement = find_allocation_reload_position(target, reg, function);
                    bind_allocation_spill(reg,
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT),
                        replacement);
                }
            }
            if (spills) {
                rewrite_allocation_spill_edges(spills, block, target);
                release_optimizer_symbol_set(spills);
            }
        }
    }
    release_optimizer_symbol_set(needed);
}
C2AllocationRegister *__fastcall lookup_allocation_register(unsigned number)
{
    C2AllocationRegister *reg;

    for (reg = allocation_register_hash[number & 1023]; reg; reg = reg->next_hash)
        if (reg->number == number) return reg;
    c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0x1587);
    return NULL;
}
void __fastcall release_function_flow_edge(C2Function *function, C2FlowEdge *edge);
C2Record *__fastcall new_block_jump(C2Record *label, C2Record *prior);
C2Expression *__fastcall find_control_target(C2Expression *record, C2Expression *target);
C2InputSymbol *new_flow_label(void);
C2Record *__fastcall prepend_flow_label(C2InputSymbol *symbol, C2BasicBlock *block);
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Record *label, int mode);
C2BasicBlock *__fastcall split_allocation_edge(C2Function *function,
    C2FlowEdge *edge, C2BasicBlock *predecessor, C2BasicBlock *destination)
{
    C2Record *boundary, *label, *new_label;
    C2BasicBlock *split;
    C2Expression *branch, *target, *scan;

    release_function_flow_edge(function, edge);
    boundary = new_statement_boundary();
    prepend_input_record(destination->records, boundary);
    split = create_function_block(function, destination, boundary, destination->records);
    split->number86 = destination->number86;
    split->number88 = destination->number88;
    new_function_edge(function, predecessor, split);
    new_function_edge(function, split, destination);
    for (label = destination->records; label && label->kind != 25; label = label->next) {
    }
    for (branch = (C2Expression *)predecessor->last_record; branch &&
         !((1U << (branch->kind & 31)) & 0x50000); branch = branch->previous) {
    }
    /* Defined original fallback: if no branch search runs, the predecessor
     * object pointer is carried as the target. It is not an unwritten local
     * or a NULL default. The original also assumes this scan finds a record. */
    target = (C2Expression *)predecessor;
    while ((1U << (branch->kind & 31)) & 0x50000) {
        target = find_control_target(branch, (C2Expression *)label);
        if (target) break;
        branch = branch->previous;
        while (branch->kind == 26) branch = branch->previous;
    }
    if (target) {
        new_label = prepend_flow_label(new_flow_label(), split);
        redirect_allocation_branch(target, new_label, 1);
        new_block_jump(label, split->last_record);
    }
    if (split->records != predecessor->last_record) {
        scan = (C2Expression *)split->records;
        while (scan && !(scan->flags & 1) && scan->kind != 25) scan = scan->previous;
        if (!scan || scan->kind != 16 || scan->detail.data || scan->opcode == 0x15c)
            new_block_jump(label, split->records);
    }
    return split;
}
C2Record *__fastcall prepend_flow_label(C2InputSymbol *symbol, C2BasicBlock *block)
{
    C2Record *label = (C2Record *)symbol_input_expression(symbol);

    insert_input_record(block->records, label);
    label->owner.block = block;
    return label;
}
void __fastcall redirect_switch_branch(C2Expression *branch, C2Record *label, int mode);
void __fastcall redirect_allocation_branch(C2Expression *branch,
    C2Record *label, int mode)
{
    C2Expression *prior, *target = (C2Expression *)label;
    if (branch->kind == 16) {
        prior = branch->left;
        if (prior->kind == 4)
            unlink_input_use(((C2InputSymbol *)prior->value.symbol)->input_operand, branch);
        if (target->kind == 25) {
            bind_symbol_input_use(target, branch);
            target = resolve_symbol_input_operand(target, 0);
        }
        attach_input_expression(branch, prior, target);
    } else redirect_switch_branch(branch, label, mode);
}
C2Expression *__fastcall find_control_target(C2Expression *record, C2Expression *target);
C2InputSymbol *new_flow_label(void);
void __fastcall move_flow_edge_destination(C2FlowEdge *edge,
    C2BasicBlock *destination, C2Function *function);

void __fastcall initialize_block_register_regions(C2Function *function,
    C2BasicBlock *block, unsigned mode)
{
    C2FlowEdge *edge, *first, *next;
    C2BasicBlock *predecessor, *split = NULL, *destination;
    C2RegisterSet *needed, *available, *written;
    C2AllocationRegister *assigned;
    C2FunctionRegister *reg;
    C2Expression *record, *replacement, *branch;
    C2Record *label, *first_record;
    unsigned number;
    int branch_defined = 0;

    first = block->incoming;
    if (!first || !first->next_incoming) return;
    needed = new_register_set(6);
    available = block->allocation_live_out;
    for (edge = first; edge; edge = next) {
        next = edge->next_incoming;
        copy_optimizer_symbol_set_into(needed, available);
        predecessor = edge->source;
        subtract_optimizer_symbol_sets(needed, predecessor->allocation_live_in_after);
        if (!needed->links.first) continue;
        destination = predecessor;
        if (predecessor->outgoing->next_outgoing || (predecessor->flags & 0x1000000)) {
            if (split) {
                written = new_register_set(6);
                for (record = (C2Expression *)split->records;
                     record != (C2Expression *)split->last_record; record = record->next) {
                    if (record->opcode != 0x139) continue;
                    if (mode)
                        number = record->tail.right->original_register->number;
                    else
                        number = record->tail.right->value.reg->allocation->number;
                    add_register_to_set(written, number);
                }
                if (optimizer_symbol_sets_equal(needed, written)) {
                    release_optimizer_symbol_set(written);
                    first_record = block->records->next;
                    while (first_record->kind == 26) first_record = first_record->next;
                    for (record = (C2Expression *)predecessor->last_record->previous;
                         record; record = record->previous) {
                        if ((1U << (record->kind & 31)) & 0x50000U) {
                            branch = find_control_target(record, (C2Expression *)first_record);
                            branch_defined = 1;
                            if (branch) break;
                        } else if (record->kind != 26) break;
                    }
                    /* The machine local carries a prior search result. With no
                     * first search it is genuinely unwritten, not a NULL default. */
                    if (!branch_defined) { __asm { int 3 } }
                    label = split->records->next;
                    while (label->kind == 26) label = label->next;
                    if (label->kind != 25) label = prepend_flow_label(new_flow_label(), split);
                    redirect_allocation_branch(branch, label, 1);
                    move_flow_edge_destination(edge, split, function);
                    continue;
                }
                release_optimizer_symbol_set(written);
            }
            split = split_allocation_edge(function, edge, predecessor, block);
            available = block->allocation_live_out;
            split->allocation_live_out = copy_optimizer_symbol_set(available);
            intersect_optimizer_symbol_sets(split->allocation_live_out,
                predecessor->allocation_live_in_after);
            split->allocation_live_in_after = copy_optimizer_symbol_set(split->allocation_live_out);
            split->number86 = block->number86;
            split->block_scope = block->block_scope;
            destination = split;
            if (mode) {
                split->allocation_region_uses = copy_optimizer_symbol_set(split->allocation_live_out);
                split->allocation_edge_uses = copy_optimizer_symbol_set(block->allocation_edge_uses);
                intersect_optimizer_symbol_sets(split->allocation_edge_uses,
                    predecessor->allocation_edge_definitions);
                split->allocation_edge_definitions = copy_optimizer_symbol_set(split->allocation_edge_uses);
                split->allocation_region_input = new_register_set(6);
                copy_optimizer_symbol_set_into(split->allocation_region_input, split->allocation_edge_uses);
            }
        }
        for (number = next_optimizer_symbol(needed); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            if (mode) {
                assigned = lookup_allocation_register(number);
                reg = assigned->source;
                replacement = find_allocation_reload_position(destination, reg, function);
                record = bind_allocation_spill(reg, (C2FunctionRegister *)assigned, replacement);
                if ((record->type & 0xfff) == 1) assigned->flags |= 0x80;
            } else {
                reg = allocation_registers_by_number[number];
                replacement = find_allocation_reload_position(destination, reg, function);
                record = bind_allocation_spill(reg,
                    (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT), replacement);
            }
            record->tail.right->location.expression.use_flags |= 0x80;
        }
        merge_optimizer_symbol_sets(destination->allocation_live_in_after, needed);
        if (mode) merge_optimizer_symbol_sets(destination->allocation_region_uses, needed);
    }
    release_optimizer_symbol_set(needed);
}
void __fastcall prepare_final_record_operands(C2Expression *record);
C2Expression *__fastcall finish_final_integer_record(C2Expression *record);
C2Expression *__fastcall materialize_allocation_input(C2Expression *record,
    C2Expression *operand, C2FunctionRegister *reg);
int __fastcall can_drop_allocation_definition(C2Expression *record)
{ __asm { int 3 } return 0; }

void __fastcall ensure_allocation_definition(C2Function *function,
    C2FunctionRegister *reg, C2Expression *limit)
{
    C2Expression *definition = reg->allocation->last_definition;
    C2Expression *record, *operand, *first;
    unsigned short line;

    if (definition->opcode == 0x139) {
        record = definition->next;
        if (!record) operand = (C2Expression *)limit;
        else {
            for (; record; record = record->next) {
                for (operand = record->left; operand; operand = operand->next) {
                    if (operand->kind == 1 &&
                        (operand->value.reg == reg || operand->value.reg->allocation_root == reg))
                        goto found_allocation_use;
                }
            }
        }
found_allocation_use:
        if (record == limit && !(operand->location.expression.properties & 0x20)) return;
        unlink_and_release_input_record((C2Record *)definition);
    } else {
        record = definition;
        first = record->tail.right;
        for (operand = first; operand; operand = operand->next) {
            if (operand->kind == 1 &&
                (operand->value.reg == reg || operand->value.reg->allocation_root == reg)) break;
        }
        /* The original dereferences NULL if the defining output is absent. */
        if (!operand) { __asm { int 3 } }
        if (operand->location.expression.properties & 0xf) return;
        --reg->allocation->definition_count;
        if (reg->kind == 3 && !reg->value && !reg->canonical->variant_next &&
            operand == first && (operand->type & 0xf000) != 0x4000 && !operand->next &&
            !(reg->flags & 2) && can_drop_allocation_definition(record)) {
            unlink_and_release_input_record((C2Record *)record);
            return;
        }
    }
    line = record->location.line;
    if (line) {
        record_line_delta = line;
        record_source_line = function->source_line_base + line;
    }
    if (reg->kind == 13) {
        materialize_allocation_input(record, operand, reg);
        return;
    }
    make_global_register_operand(operand);
    if ((operand->type & 0xf000) != 0x4000) {
        if (operand->location.expression.properties & 0x20) prepare_final_record_operands(record);
        else finish_final_integer_record(record);
    }
}
int __fastcall allocation_record_stores_operand(C2Expression *record,
    C2Expression *operand)
{
    return (operand->location.expression.properties & 0x20) && record->opcode == 0x12;
}

void __fastcall build_function_register_regions(C2Function *function)
{
    C2RegisterSet *seen = new_register_set(6), *available = new_register_set(6);
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2FunctionRegister *reg, *alias;
    C2OptimizerUse *use, *saved;
    unsigned number;
    unsigned short line;
    long reg_end, alias_end;

    for (block = function->flow->last; block; block = block->previous) {
        initialize_block_register_regions(function, block, 0);
        copy_optimizer_symbol_set_into(available, block->allocation_live_out);
        for (record = block->records; record != block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            if (record->kind == 19 || record->opcode == 0x15b) {
                switch (record->opcode) {
                case 0x15b: case 0x163: case 0x165: case 0x167:
                case 0x169: case 0x16b: case 0x172:
                    clear_register_set(available);
                    break;
                }
            }
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1 || operand->original_register !=
                    (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) continue;
                reg = operand->value.reg;
                if (reg->allocation_root) reg = reg->allocation_root;
                number = reg->allocation->number;
                if (!optimizer_set_contains(available, number)) {
                    if (!optimizer_set_contains(seen, number)) add_register_to_set(seen, number);
                    else {
                        ensure_allocation_definition(function, reg, record);
                        line = record->location.line;
                        if (line) {
                            record_source_line = function->source_line_base + line;
                            record_line_delta = line;
                        }
                    }
                    if (allocation_record_stores_operand(record, operand))
                        remove_optimizer_symbol(seen, reg->allocation->number);
                    add_register_to_set(available, reg->allocation->number);
                    reg->allocation->last_definition = bind_allocation_spill(reg,
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT), record);
                } else remove_optimizer_symbol(seen, number);
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) break;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    number = reg->allocation->number;
                    if (!optimizer_set_contains(seen, number)) add_register_to_set(seen, number);
                    else {
                        ensure_allocation_definition(function, reg, record);
                        line = record->location.line;
                        if (line) {
                            record_source_line = function->source_line_base + line;
                            record_line_delta = line;
                        }
                    }
                    add_register_to_set(available, reg->allocation->number);
                    reg->allocation->last_definition = record;
                    if (record->opcode != 0x139) ++reg->allocation->definition_count;
                    goto region_output_aliases;
                case 2:
                    reg = operand->value.reg;
                    if (reg->allocation && reg->allocation->number)
                        remove_optimizer_symbol(available, reg->allocation->number);
region_output_aliases:
                    alias = reg->canonical;
                    if ((unsigned long)alias >= (unsigned long)machine_registers &&
                        (unsigned long)alias < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                        { __asm { int 3 } }
                    if (alias->variant_next) {
                        for (; alias; alias = alias->variant_next) {
                            if (alias == reg) continue;
                            reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                            alias_end = (long)((unsigned)alias->displacement + alias->value_bytes);
                            if (reg->displacement < alias_end && alias->displacement < reg_end &&
                                alias->allocation && alias->allocation->number)
                                remove_optimizer_symbol(available, alias->allocation->number);
                        }
                    }
                    break;
                case 6: case 10:
                    use = get_global_index_uses(operand->kind == 6 ?
                        operand->immediate : operand->value.optimizer_index);
                    saved = NULL;
                    if (use) {
next_region_use:
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            use = reg->optimizer_uses;
                            if (!use) {
                                use = saved->next;
                                goto resume_region_uses;
                            }
                            reg = use->reg;
                        }
                        if (reg->allocation && reg->allocation->pinned_registers)
                            subtract_optimizer_symbol_sets(available, reg->allocation->pinned_registers);
                        use = use->next;
                        if (use) goto next_region_use;
                        if (saved) { use = saved->next; saved = NULL; }
resume_region_uses:
                        if (use) goto next_region_use;
                    }
                    break;
                }
            }
        }
        subtract_optimizer_symbol_sets(seen, block->allocation_live_in_after);
        if (seen->links.first) {
            for (number = next_optimizer_symbol(seen); number != ~0U;
                 number = next_optimizer_symbol(NULL))
                ensure_allocation_definition(function, allocation_registers_by_number[number], NULL);
            clear_register_set(seen);
            record = record->previous;
            while (!(record->flags & 1) && record->kind != 24) record = record->previous;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
        }
        block->allocation_state24 = block->allocation_state28 = 0;
    }
    release_optimizer_symbol_set(seen);
    release_optimizer_symbol_set(available);
}
int __fastcall region_shares_ancestor_exit(C2FlowRegion *region)
{
    C2FlowRegion *ancestor;
    C2FlowBlockLink *outer, *inner;

    for (ancestor = region->parent; ancestor; ancestor = ancestor->parent)
        for (outer = ancestor->exits; outer; outer = outer->next)
            for (inner = region->exits; inner; inner = inner->next)
                if (outer->block == inner->block) return 1;
    return 0;
}
int __fastcall region_has_early_exit(C2FlowRegion *region)
{
    C2FlowBlockLink *exit;
    C2FlowEdge *edge;
    short number;

    for (exit = region->exits; exit; exit = exit->next) {
        number = exit->block->number84;
        if (number < 0) return 1;
        for (edge = exit->block->outgoing; edge; edge = edge->next_outgoing)
            if (edge->destination->block_scope != region &&
                (short)edge->destination->number84 < number) return 1;
    }
    return 0;
}

void __fastcall allocate_region_registers(C2FlowRegion *region, unsigned count)
{
    C2RegisterSet *temporary = new_register_set(6), *eligible;
    C2FlowBlockLink *link;
    C2BasicBlock *block;
    C2Expression *record, *output, *address;
    unsigned number;

    for (; region; region = region->next) {
        eligible = new_register_set(6);
        region->allocation_eligible = eligible;
        fill_global_available_set(eligible, count);
        if (!(region->flags & 1) && !region_shares_ancestor_exit(region) &&
            !region_has_early_exit(region)) {
            block = region->header;
            copy_optimizer_symbol_set_into(temporary, block->allocation_live_out);
            for (link = region->exits; link; link = link->next)
                intersect_optimizer_symbol_sets(temporary, link->block->allocation_live_in_after);
            subtract_optimizer_symbol_sets(eligible, temporary);
            if (block == region->last) {
                for (record = (C2Expression *)block->records;
                     record != (C2Expression *)block->last_record; record = record->next) {
                    if (!(record->flags & 1)) continue;
                    output = record->tail.right;
                    if (!output || output->kind != 6 || output->opcode != 0x122) continue;
                    address = output->aggregate_value;
                    if (address->original_register->kind != 2) continue;
                    number = address->value.reg->allocation->number;
                    if (!optimizer_set_contains(block->allocation_definitions, number) &&
                        optimizer_set_contains(block->allocation_live_out, number)) break;
                }
                if (record != (C2Expression *)block->last_record) {
                    region->allocation_next = allocation_region_chain;
                    allocation_region_chain = region;
                }
            }
        }
        allocate_region_registers(region->children, count);
    }
    release_optimizer_symbol_set(temporary);
}
unsigned __fastcall find_optimizer_set_intersection(C2RegisterSet *first,
    C2RegisterSet *second);
C2FunctionRegister *__fastcall seed_allocation_classes(C2FunctionRegister *active,
    C2RegisterSet *numbers, C2RegisterSet *predecessor_numbers, unsigned *count)
{
    C2FunctionRegister *reg;
    struct C2AllocationInfo *info;
    C2AllocationClass *part;
    unsigned number, class_number, next_number = *count;

    for (number = next_optimizer_symbol(numbers); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = allocation_registers_by_number[number];
        reg->allocation_next = active;
        if (active) active->allocation->active_next = reg;
        info = reg->allocation;
        info->active_next = NULL;
        part = predecessor_numbers->links.first ? info->classes : NULL;
        for (; part; part = part->next) {
            class_number = find_optimizer_set_intersection(part->numbers,
                predecessor_numbers);
            if (class_number != ~0U) break;
        }
        if (part) info->class_number = class_number;
        else {
            add_register_to_set(predecessor_numbers, next_number);
            part = c2_pool_allocate(4, sizeof(*part));
            part->numbers = new_register_set(6);
            add_register_to_set(part->numbers, next_number);
            part->next = info->classes;
            info->classes = part;
            info->class_number = next_number++;
        }
        active = reg;
    }
    *count = next_number;
    return active;
}

unsigned __fastcall split_allocation_registers(C2BasicBlock *first, unsigned count)
{
    C2BasicBlock *block, *next, *predecessor;
    C2RegisterBlock *page;
    C2FunctionRegister *reg, *view, *active = NULL, *after, *before;
    C2RegisterSet *universe, *temporary, *defined, *used, *region_numbers, *outgoing, *prior_numbers;
    C2AllocationClass *part, *matched, **link, *removed;
    struct C2AllocationInfo *info;
    C2Expression *record, *operand;
    C2OptimizerUse *use, *saved;
    C2FlowEdge *edge;
    unsigned next_number = 1, number;
    long reg_end, view_end;
    int follows;

    universe = new_register_set(6);
    fill_global_available_set(universe, count);
    for (block = first; block; block = block->next) {
        if (!block->alias_in) {
            block->alias_in = new_register_set(6);
            block->alias_out = new_register_set(6);
            block->allocation_uses = new_register_set(6);
            block->allocation_definitions = new_register_set(6);
        } else {
            release_optimizer_symbol_set(block->allocation_live_in);
            block->allocation_live_in = NULL;
            clear_register_set(block->allocation_uses);
            clear_register_set(block->allocation_definitions);
        }
        copy_optimizer_symbol_set_into(block->alias_in, block->allocation_live_out);
        subtract_optimizer_symbol_sets(block->alias_in, universe);
        intersect_optimizer_symbol_sets(block->allocation_live_out, universe);
        copy_optimizer_symbol_set_into(block->alias_out, block->allocation_live_in_after);
        subtract_optimizer_symbol_sets(block->alias_out, universe);
        intersect_optimizer_symbol_sets(block->allocation_live_in_after, universe);
    }
    release_optimizer_symbol_set(universe);
    for (page = first_function_register_block; page; page = page->next) {
        for (reg = page->records; reg < page->cursor; ++reg) {
            if (reg->kind == 16 || !reg->allocation || !reg->allocation->pinned_registers) continue;
            if ((unsigned long)reg->canonical >= (unsigned long)machine_registers &&
                (unsigned long)reg->canonical <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                { __asm { int 3 } }
            for (view = reg->canonical; view; view = view->variant_next) {
                if (view->allocation && view->allocation->number && !view->allocation->definition_count)
                    remove_optimizer_symbol(reg->allocation->pinned_registers, view->allocation->number);
            }
            if (!reg->allocation->pinned_registers->links.first) {
                release_optimizer_symbol_set(reg->allocation->pinned_registers);
                reg->allocation->pinned_registers = NULL;
            }
        }
    }
    temporary = new_register_set(6);
    first->allocation_edge_definitions = new_register_set(6);
    for (block = first; block; block = next) {
        defined = block->allocation_uses;
        used = block->allocation_definitions;
        region_numbers = block->block_scope ? block->block_scope->allocation_eligible : NULL;
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) continue;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    info = reg->allocation;
                    if (reg->kind == 13) {
                        operand->allocation_class_number = 0;
                        operand->original_register = (C2FunctionRegister *)info->assigned_register;
                        continue;
                    }
                    if (!info->class_number) {
                        part = c2_pool_allocate(4, sizeof(*part));
                        part->numbers = new_register_set(6);
                        add_register_to_set(part->numbers, next_number);
                        part->next = info->classes;
                        info->classes = part;
                        info->class_number = next_number++;
                        reg->allocation_next = active;
                        if (active) active->allocation->active_next = reg;
                        info->active_next = NULL;
                        active = reg;
                    }
                    operand->allocation_class_number = info->class_number;
                    if (!(operand->location.expression.properties & 0xf)) {
                        info->class_number = 0;
                        if (reg == active) {
                            active = reg->allocation_next;
                            if (active) active->allocation->active_next = NULL;
                        } else {
                            after = info->active_next;
                            before = reg->allocation_next;
                            after->allocation_next = before;
                            if (before) before->allocation->active_next = after;
                        }
                    }
                    remove_optimizer_symbol(defined, info->number);
                    add_register_to_set(used, info->number);
                } else if (operand->kind == 2) {
                    reg = operand->value.reg;
                    if (reg->allocation && reg->allocation->number) {
                        remove_optimizer_symbol(defined, reg->allocation->number);
                        add_register_to_set(used, reg->allocation->number);
                    }
                } else continue;
                view = reg->canonical;
                if ((unsigned long)view >= (unsigned long)machine_registers &&
                    (unsigned long)view <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                    { __asm { int 3 } }
                if (!view->variant_next) continue;
                for (; view; view = view->variant_next) {
                    if (view == reg) continue;
                    reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                    view_end = (long)((unsigned)view->displacement + view->value_bytes);
                    if (reg->displacement <= view->displacement && view_end <= reg_end &&
                        view->allocation && view->allocation->number) {
                        remove_optimizer_symbol(defined, view->allocation->number);
                        add_register_to_set(used, view->allocation->number);
                    }
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) break;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    info = reg->allocation;
                    if (reg->kind == 13) {
                        operand->allocation_class_number = 0;
                        ++info->class_number;
                        operand->original_register = (C2FunctionRegister *)info->assigned_register;
                    } else {
                        if (!info->class_number) {
                            part = c2_pool_allocate(4, sizeof(*part));
                            part->numbers = new_register_set(6);
                            add_register_to_set(part->numbers, next_number);
                            part->next = info->classes;
                            info->classes = part;
                            info->class_number = next_number++;
                            reg->allocation_next = active;
                            if (active) active->allocation->active_next = reg;
                            info->active_next = NULL;
                            active = reg;
                        }
                        operand->allocation_class_number = info->class_number;
                    }
                    break;
                case 2:
                    reg = operand->value.reg;
                    if ((unsigned long)reg->canonical >= (unsigned long)machine_registers &&
                        (unsigned long)reg->canonical <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                        { __asm { int 3 } }
                    for (view = reg->canonical; view; view = view->variant_next) {
                        reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                        view_end = (long)((unsigned)view->displacement + view->value_bytes);
                        if (view != reg && !(reg->displacement < view_end && view->displacement < reg_end)) continue;
                        if (view->allocation && view->allocation->number &&
                            (int)view->allocation->definition_count > 0) {
                            add_register_to_set(defined, view->allocation->number);
                            if (region_numbers) add_register_to_set(region_numbers, view->allocation->number);
                        }
                    }
                    break;
                case 6: case 10:
                    use = get_global_index_uses(operand->kind == 6 ?
                        operand->immediate : operand->value.optimizer_index);
                    saved = NULL;
                    if (use) {
next_split_use:
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            use = reg->optimizer_uses;
                            if (!use) { use = saved->next; goto resume_split_uses; }
                            reg = use->reg;
                        }
                        if (reg->allocation && reg->allocation->pinned_registers) {
                            merge_optimizer_symbol_sets(defined, reg->allocation->pinned_registers);
                            if (region_numbers) merge_optimizer_symbol_sets(region_numbers, reg->allocation->pinned_registers);
                        }
                        use = use->next;
                        if (use) goto next_split_use;
                        if (saved) { use = saved->next; saved = NULL; }
resume_split_uses:
                        if (use) goto next_split_use;
                    }
                }
            }
        }
        outgoing = new_register_set(6);
        block->allocation_edge_uses = outgoing;
        for (reg = active; reg; reg = reg->allocation_next)
            add_register_to_set(outgoing, reg->allocation->class_number);
        next = block->next;
        follows = 0;
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            predecessor = edge->source;
            if (predecessor == next) follows = 1;
            prior_numbers = predecessor->allocation_edge_definitions;
            if (!prior_numbers) {
                predecessor->allocation_edge_definitions = copy_optimizer_symbol_set(outgoing);
                continue;
            }
            for (reg = active; reg; reg = reg->allocation_next) {
                info = reg->allocation;
                for (matched = info->classes; matched; matched = matched->next)
                    if (optimizer_symbol_sets_intersect(matched->numbers, prior_numbers)) break;
                if (!matched) add_register_to_set(prior_numbers, info->class_number);
                else if (!optimizer_set_contains(matched->numbers, info->class_number)) {
                    for (link = &info->classes; !optimizer_set_contains((*link)->numbers,
                         info->class_number); link = &(*link)->next) {
                    }
                    removed = *link;
                    merge_optimizer_symbol_sets(matched->numbers, removed->numbers);
                    *link = removed->next;
                }
            }
        }
        if (!next) break;
        if (follows) {
            if (!optimizer_symbol_sets_equal(block->alias_in, next->alias_out)) {
                copy_optimizer_symbol_set_into(temporary, next->alias_out);
                subtract_optimizer_symbol_sets(temporary, block->alias_in);
                active = seed_allocation_classes(active, temporary,
                    next->allocation_edge_definitions, &next_number);
            }
        } else {
            while (active) {
                reg = active;
                active = active->allocation_next;
                reg->allocation->class_number = 0;
            }
            if (!next->allocation_edge_definitions) next->allocation_edge_definitions = new_register_set(6);
            active = seed_allocation_classes(NULL, next->alias_out,
                next->allocation_edge_definitions, &next_number);
        }
    }
    release_optimizer_symbol_set(temporary);
    for (use = optimizer_memory_register->optimizer_uses; use; use = use->next) {
        if (use->reg->allocation && use->reg->allocation->pinned_registers)
            merge_optimizer_symbol_sets(first->allocation_uses, use->reg->allocation->pinned_registers);
    }
    for (block = first; block; block = block->next) {
        release_optimizer_symbol_set(block->alias_in);
        block->alias_in = block->allocation_live_out;
        block->allocation_live_out = NULL;
        release_optimizer_symbol_set(block->alias_out);
        block->alias_out = block->allocation_live_in_after;
        block->allocation_live_in_after = NULL;
    }
    return next_number;
}
void __fastcall finish_region_register_allocation(C2FlowRegion *region)
{
    for (; region; region = region->next) {
        finish_region_register_allocation(region->children);
        if (region->parent)
            merge_optimizer_symbol_sets(region->parent->allocation_eligible, region->allocation_eligible);
    }
}
/* Original4910d4 is the initialized NULL active-list head. Its later nonNULL
 * writers and list-removal child are still pending; no node payload is claimed. */
C2AllocationRegister *active_allocation_registers;
C2AllocationRegister *__fastcall unlink_active_allocation_register(C2AllocationRegister *reg,
    C2AllocationRegister *head)
{
    C2AllocationRegister *previous = reg->previous_active, *next;

    if (!previous) {
        if (head == reg) {
            head = reg->next_active;
            if (head) head->previous_active = NULL;
        }
    } else {
        next = reg->next_active;
        previous->next_active = next;
        if (next) next->previous_active = previous;
    }
    return head;
}

void __fastcall release_allocation_register(C2AllocationRegister *reg, int remove)
{
    C2AllocationRegister **link;
    unsigned number;

    release_optimizer_symbol_set(reg->available);
    if (remove) {
        if (active_allocation_registers)
            active_allocation_registers = unlink_active_allocation_register(reg,
                active_allocation_registers);
    }
    number = reg->number;
    for (link = &allocation_register_hash[number & 1023]; *link && *link != reg;
         link = &(*link)->next_hash) {
    }
    if (!*link) {
        c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0x1578);
        return;
    }
    *link = reg->next_hash;
    memset(reg, 0, sizeof(*reg));
    reg->next_hash = free_allocation_registers;
    free_allocation_registers = reg;
    reg->number = number;
}

void __fastcall build_allocation_register_map(C2AllocationRegister **map)
{
    C2FunctionRegister *reg;
    C2AllocationRegister *assigned;
    C2AllocationClass *part;
    C2RegisterBlock *page;
    C2RegisterSet *numbers;
    unsigned bucket, number;

    for (bucket = 0; bucket < 64; ++bucket) {
        for (reg = allocation_class_heads[bucket]; reg; reg = reg->allocation_next) {
            if ((int)reg->allocation->class_number < 1) {
                assigned = reg->allocation->assigned_register;
                reg->allocation->assigned_register = NULL;
                release_allocation_register(assigned, 0);
                remove_optimizer_symbol(allocation_integer_registers, assigned->number);
            }
        }
    }
    for (page = first_function_register_block; page; page = page->next) {
        for (reg = page->records; reg < page->cursor; ++reg) {
            if (reg->kind == 16 || !reg->allocation) continue;
            for (part = reg->allocation->classes; part; part = part->next) {
                assigned = get_allocation_register(reg);
                numbers = part->numbers;
                for (number = next_optimizer_symbol(numbers); number != ~0U;
                     number = next_optimizer_symbol(NULL)) map[number] = assigned;
                release_optimizer_symbol_set(numbers);
            }
        }
    }
}
C2Expression *__fastcall spill_allocation_result(C2FunctionRegister *reg,
    C2AllocationRegister *assigned, C2Expression *record)
{
    C2Expression *temporary, *value;
    unsigned short type;
    unsigned opcode;

    /* Static machine rows do not yet have a modeled function type at+16. */
    if ((unsigned)reg >= (unsigned)machine_registers &&
        (unsigned)reg <= (unsigned)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
    type = reg->type == 0x3004 ? 0x1004 : reg->type;
    opcode = reg->kind == 3 || reg->kind == 4 || reg->kind == 5 || reg->kind == 6 ? 0x120 : 0x121;
    temporary = new_temporary_register_expression(0x11f, type, reg,
        (C2FunctionRegister *)assigned);
    value = new_typed_register_expression(opcode, type, reg);
    return new_binary_expression(0x13a, type, value, temporary,
        insert_input_record, (C2Record *)record);
}

void __fastcall rewrite_allocated_function(C2Function *function,
    C2AllocationRegister **map)
{
    C2BasicBlock *block;
    C2RegisterSet *live, *classes, *converted;
    C2FunctionRegister *reg, *view;
    C2AllocationRegister *assigned;
    C2Expression *record, *operand, *inserted;
    C2OptimizerUse *use, *saved;
    unsigned number, sequence = 0;
    unsigned short line;
    long reg_end, view_end;

    for (block = function->flow->last; block; block = block->previous) {
        release_optimizer_symbol_set(block->allocation_uses);
        block->allocation_uses = NULL;
        release_optimizer_symbol_set(block->allocation_definitions);
        block->allocation_definitions = NULL;
        release_optimizer_symbol_set(block->allocation_live_out);
        live = block->allocation_live_in_after;
        classes = block->allocation_edge_definitions;
        converted = block->alias_out;
        block->alias_out = NULL;
        block->allocation_live_in_after = converted;
        block->allocation_edge_definitions = NULL;
        for (number = next_optimizer_symbol(classes); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            assigned = map[number];
            assigned->last_block = block;
            add_register_to_set(converted, assigned->number);
        }
        release_optimizer_symbol_set(classes);
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
            ++sequence;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    if (operand->original_register !=
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) continue;
                    reg = operand->value.reg;
                    if (reg->allocation_root) reg = reg->allocation_root;
                    assigned = map[operand->allocation_class_number];
                    operand->original_register = (C2FunctionRegister *)assigned;
                    assigned->definition_sequence = sequence;
                    if (record->opcode == 0x139) operand->allocation_class_number = 0;
                    else {
                        if ((unsigned long)reg->canonical >= (unsigned long)machine_registers &&
                            (unsigned long)reg->canonical <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                            { __asm { int 3 } }
                        if (optimizer_set_contains(live, reg->allocation->number) ||
                            (operand->location.expression.use_flags & 8) || (reg->canonical->flags & 1)) {
                            operand->allocation_class_number = 0;
                            inserted = spill_allocation_result(reg, assigned, record);
                            if ((operand->location.expression.use_flags & 8) || (reg->canonical->flags & 1))
                                inserted->tail.right->location.expression.use_flags |= 8;
                        }
                    }
                    remove_optimizer_symbol(live, reg->allocation->number);
                } else if (operand->kind == 2) {
                    reg = operand->value.reg;
                    if (reg->allocation && reg->allocation->number)
                        remove_optimizer_symbol(live, reg->allocation->number);
                } else continue;
                view = reg->canonical;
                if ((unsigned long)view >= (unsigned long)machine_registers &&
                    (unsigned long)view <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                    { __asm { int 3 } }
                if (!view->variant_next) continue;
                for (; view; view = view->variant_next) {
                    reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                    view_end = (long)((unsigned)view->displacement + view->value_bytes);
                    if (view != reg && reg->displacement <= view->displacement && view_end <= reg_end &&
                        view->allocation && view->allocation->number)
                        remove_optimizer_symbol(live, view->allocation->number);
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                switch (operand->kind) {
                case 1:
                    if (operand->original_register ==
                        (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)) {
                        number = operand->allocation_class_number;
                        operand->allocation_class_number = 0;
                        assigned = map[number];
                        operand->original_register = (C2FunctionRegister *)assigned;
                        assigned->last_block = block;
                    }
                    break;
                case 2:
                    reg = operand->value.reg;
                    if ((unsigned long)reg->canonical >= (unsigned long)machine_registers &&
                        (unsigned long)reg->canonical <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                        { __asm { int 3 } }
                    for (view = reg->canonical; view; view = view->variant_next) {
                        reg_end = (long)((unsigned)reg->displacement + reg->value_bytes);
                        view_end = (long)((unsigned)view->displacement + view->value_bytes);
                        if (view != reg && !(reg->displacement < view_end && view->displacement < reg_end)) continue;
                        if (view->allocation && view->allocation->number &&
                            (int)view->allocation->definition_count > 0)
                            add_register_to_set(live, view->allocation->number);
                    }
                    break;
                case 6: case 10:
                    use = get_global_index_uses(operand->kind == 6 ?
                        operand->immediate : operand->value.optimizer_index);
                    saved = NULL;
                    if (use) {
next_rewrite_allocation_use:
                        reg = use->reg;
                        if (reg->kind == 12) {
                            saved = use;
                            use = reg->optimizer_uses;
                            if (!use) { use = saved->next; goto resume_rewrite_allocation_uses; }
                            reg = use->reg;
                        }
                        if (reg->allocation && reg->allocation->pinned_registers)
                            merge_optimizer_symbol_sets(live, reg->allocation->pinned_registers);
                        use = use->next;
                        if (use) goto next_rewrite_allocation_use;
                        if (saved) { use = saved->next; saved = NULL; }
resume_rewrite_allocation_uses:
                        if (use) goto next_rewrite_allocation_use;
                    }
                }
            }
        }
        release_optimizer_symbol_set(live);
        classes = block->allocation_edge_uses;
        converted = block->alias_in;
        block->alias_in = NULL;
        block->allocation_live_out = converted;
        block->allocation_edge_uses = NULL;
        for (number = next_optimizer_symbol(classes); number != ~0U;
             number = next_optimizer_symbol(NULL)) add_register_to_set(converted, map[number]->number);
        release_optimizer_symbol_set(classes);
        block->flags &= ~1U;
    }
}

void __fastcall allocate_final_function_registers(C2Function *function)
{
    C2BasicBlock *first;
    C2FlowGraph *flow;
    C2FlowRegion *regions;
    C2AllocationRegister **map;
    unsigned count, prepared_count;

    reset_input_pool(4);
    reset_input_pool(6);
    allocation_integer_registers = new_register_set(6);
    allocation_float_registers = new_register_set(6);
    free_allocation_registers = NULL;
    next_allocation_register_number = 1;
    if (!allocation_machine_initialized) {
        allocation_machine_initialized = 1;
        initialize_allocation_machine();
    }
    reset_allocation_register_hash();
    first = function->flow->last;
    count = number_allocation_registers(first->records);
    last_allocation_register_number = count - 1;
    prepared_count = prepare_allocation_registers(count);
    collect_allocation_uses(first, prepared_count);
    solve_allocation_liveness(function->flow->first);
    solve_optimizer_alias_sets(first, 1);
    intersect_block_allocation_sets(first);
    if (!(function->symbol->additional_flags & 0x200) &&
        !(function->symbol->symbol.flags & 0x1000))
        prepare_function_register_spills(function);
    build_function_register_regions(function);
    regions = function->flow_regions;
    allocation_region_chain = NULL;
    allocate_region_registers(regions, prepared_count);
    flow = function->flow;
    count = split_allocation_registers(flow->first, count);
    finish_region_register_allocation(regions);
    map = c2_pool_allocate(4, count * sizeof(*map));
    build_allocation_register_map(map);
    solve_allocation_liveness(flow->first);
    rewrite_allocated_function(function, map);
}

void __fastcall solve_allocation_liveness(C2BasicBlock *last)
{
    C2BasicBlock *block, *first;
    C2FlowEdge *edge;
    C2RegisterSet *temporary;
    int changed;

    for (block = last; block; block = block->next) {
        block->allocation_live_out = copy_optimizer_symbol_set(block->allocation_uses);
        block->allocation_live_in_after = new_register_set(6);
    }
    temporary = new_register_set(6);
    first = last->next;
    do {
        changed = 0;
        for (block = first; block; block = block->next) {
            clear_register_set(temporary);
            for (edge = block->outgoing; edge; edge = edge->next_outgoing)
                merge_optimizer_symbol_sets(temporary, edge->destination->allocation_live_out);
            if (!optimizer_symbol_sets_equal(temporary, block->allocation_live_in_after)) {
                copy_optimizer_symbol_set_into(block->allocation_live_in_after, temporary);
                update_optimizer_alias_set(block->allocation_live_out, temporary,
                    block->allocation_definitions, block->allocation_uses);
                changed = 1;
            }
        }
    } while (changed);
    release_optimizer_symbol_set(temporary);
}

extern int c2_frame_optimization, c2_global_optimize, c2_favor_speed, c2_float_precision;
extern unsigned final_lowering_state;
int frame_has_parameter_reference; /* Original4910f0: reset by this pass. */
unsigned frame_reference_cost;     /* Original4910f4: reset by this pass. */
int __fastcall frame_reference_weight(int base, int depth)
{
    unsigned weight = 1;
    if (depth < 0) return 0;
    while (depth-- > 0) weight *= (unsigned)base;
    return (int)weight;
}

void __fastcall plan_function_frame_pointer(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *record, *operand, *base, *transfer;
    C2FunctionRegister *canonical;
    unsigned cost = 0, double_stores = final_lowering_state, score;
    unsigned kind, canonical_kind;
    int floating_base;

    frame_has_parameter_reference = 0;
    frame_reference_cost = 0;
    function->flags &= ~0x300010U;
    if (!c2_frame_optimization || (function->flags & 0xc1) ||
        (function->symbol->symbol.flags & 0x1000) ||
        (function->symbol->additional_flags & 0x200)) return;
    function->flags |= 0x10;
    if (!c2_global_optimize || !c2_favor_speed ||
        (function->flags & 1) || !(function->flags & 8)) return;
    for (block = function->flow->last; block; block = block->previous) {
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                kind = operand->kind;
                base = NULL;
                floating_base = 0;
                if (kind == 2 || kind == 3) base = operand;
                else if (kind == 5 || kind == 6) base = operand->tail.right;
                if (base) {
                    canonical = base->value.reg->canonical;
                    ALLOCATION_KNOWN_KIND(canonical);
                    canonical_kind = canonical->kind;
                    if (canonical_kind == 5) {
                        frame_has_parameter_reference = 1;
                        if (kind == 2 || kind == 3) continue;
                    } else if (canonical_kind == 7 || canonical_kind == 8) continue;
                    else if (canonical_kind == 4 && canonical->type == 0x4008) floating_base = 1;
                }
                if (floating_base || (operand->type == 0x4008 && (kind == 2 || kind == 6)))
                    cost += frame_reference_weight(5, (short)block->number86);
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                kind = operand->kind;
                base = NULL;
                floating_base = 0;
                if (kind == 2 || kind == 3) base = operand;
                else if (kind == 5 || kind == 6) base = operand->tail.right;
                if (base) {
                    canonical = base->value.reg->canonical;
                    ALLOCATION_KNOWN_KIND(canonical);
                    canonical_kind = canonical->kind;
                    if (canonical_kind == 5) { frame_has_parameter_reference = 1; continue; }
                    if (canonical_kind == 7 || canonical_kind == 8) continue;
                    if (canonical_kind == 4 && canonical->type == 0x4008) floating_base = 1;
                }
                if (floating_base || (operand->type == 0x4008 && (kind == 2 || kind == 6))) {
                    transfer = floating_base && (kind == 2 || kind == 3) ? record : operand;
                    if (transfer->opcode == 0x122 && transfer->aggregate_value->kind == 1 &&
                        transfer->aggregate_value->fixed_register == machine_registers + 5)
                        double_stores += frame_reference_weight(5, (short)block->number86);
                    else cost += frame_reference_weight(5, (short)block->number86);
                }
            }
        }
    }
    score = cost + double_stores * 3;
    if ((int)score >= 250 || (!frame_has_parameter_reference && (int)score > 15)) {
        function->flags |= 0x300000;
        if (((C2FunctionSymbol *)current_symbol)->argument_stack_bytes && frame_has_parameter_reference)
            function->flags &= ~0x10U;
    } else if ((int)score >= 15) frame_reference_cost = score;
}

/* Scalar reset owners: no pointer/default interpretation is inferred for the
 * five counters until their producers/consumers are reached. */
typedef struct C2AllocationMoveLink {
    struct C2AllocationMoveLink *next;
    union { C2Expression *record; C2RegisterSet *registers; };
} C2AllocationMoveLink;
typedef char C2AllocationMoveSizeCheck[sizeof(C2AllocationMoveLink) == 8 ? 1 : -1];

C2AllocationMoveLink *free_allocation_moves; /* Original4910dc, reset before use. */
unsigned allocation_block_count;
int allocation_is_empty;                /* Original4910ec. */
C2RegisterSet *allocation_deferred_registers; /* Original4910cc. */
C2RegisterSet *allocation_spilled_registers;  /* Original4910d0. */
unsigned allocation_class_limits[2];    /* Original4910f8/fc, f8 written below. */
int allocation_frame_register_available; /* Original491080, written by limit helper. */

C2AllocationMoveLink *new_allocation_move(void)
{
    C2AllocationMoveLink *move = free_allocation_moves;

    if (move) free_allocation_moves = move->next;
    else move = c2_pool_allocate(4, sizeof(C2AllocationMoveLink));
    return move;
}
void __fastcall release_allocation_moves(C2AllocationMoveLink *head)
{
    C2AllocationMoveLink *last = head, *old;

    while (last->next) last = last->next;
    old = free_allocation_moves;
    free_allocation_moves = head;
    last->next = old;
}
int __fastcall allocation_move_is_blocked(C2FunctionRegister *source, C2Expression *move,
    C2Expression *last, C2AllocationRegister *assigned)
{
    C2Expression *record, *operand;
    C2FunctionRegister *root, *other, *other_root;
    C2BasicBlock *block;
    long source_end, other_end;
    int live = 1;

    if (move == last || last->previous == move) return 0;
    /* Rows7/8's kind1, canonical self and32-bit ranges are already modeled.
     * Admit that known view locally; other allocation consumers retain their
     * own narrower data domains. */
    if (source != (C2FunctionRegister *)&machine_registers[7] &&
        source != (C2FunctionRegister *)&machine_registers[8])
        ALLOCATION_KNOWN_KIND(source);
    root = source == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)
        ? machine_register_storage.sentinel.canonical : source->canonical;
    for (record = last->previous; record != move; record = record->previous) {
        if (record->flags & 1) {
            if (!live) {
                for (operand = record->left; operand; operand = operand->next)
                    if (operand->kind == 1 &&
                        (C2AllocationRegister *)operand->original_register == assigned) live = 1;
            } else {
                for (operand = record->tail.right; operand; operand = operand->next) {
                    switch (operand->kind) {
                    case 1:
                        if ((C2AllocationRegister *)operand->original_register == assigned) {
                            live = 0;
                            break;
                        }
                        /* Other register outputs share the range check. */
                    case 2:
                        other = operand->value.reg;
                        if (other != (C2FunctionRegister *)&machine_registers[7] &&
                            other != (C2FunctionRegister *)&machine_registers[8])
                            ALLOCATION_KNOWN_KIND(other);
                        other_root = other == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT)
                            ? machine_register_storage.sentinel.canonical : other->canonical;
                        if (other_root != root) break;
                        if (other == source) return 1;
                        /* The sentinel has only a typed12-byte prefix; row3's
                         * range is outside this consumer's recovered domain. */
                        if (source == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ||
                            other == (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT) ||
                            source == (C2FunctionRegister *)&machine_registers[3] ||
                            other == (C2FunctionRegister *)&machine_registers[3]) { __asm { int 3 } }
                        other_end = (long)(other->value_bytes + (unsigned)other->displacement);
                        source_end = (long)(source->value_bytes + (unsigned)source->displacement);
                        if (source->displacement < other_end && other->displacement < source_end)
                            return 1;
                        break;
                    case 6:
                        if (register_may_alias_index(root, operand->allocation_alias_index)) return 1;
                        break;
                    case 10:
                        if (register_may_alias_index(root, operand->value.optimizer_index)) return 1;
                        break;
                    }
                }
            }
        } else if (record->kind == 24 && ((C2Record *)record)->owner.block) {
            block = ((C2Record *)record)->owner.block->next;
            live = optimizer_set_contains(block->allocation_live_in_after, assigned->number);
        }
    }
    return 0;
}
C2BasicBlock *__fastcall allocation_block_for_record(C2Expression *record)
{
    while (record->kind != 24 || !((C2Record *)record)->owner.block)
        record = record->previous;
    return ((C2Record *)record)->owner.block;
}

void __fastcall coalesce_single_use_registers(C2Function *function, C2RegisterSet **classes)
{
    C2Expression *record, *operand, *input, *output, *last, *cursor, *previous;
    C2AllocationMoveLink *moves = NULL, *link;
    C2AllocationRegister *assigned, *replacement, *reg;
    C2FunctionRegister *source, *original, *view;
    C2BasicBlock *block, *last_block, *search;
    C2RegisterSet *set;
    unsigned bucket, number, type_class;
    long displacement;

    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        output = record->tail.right;
        for (operand = output; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            assigned = (C2AllocationRegister *)operand->original_register;
            if (assigned->kind != 2) continue;
            ++assigned->use_count;
            input = record->left;
            if (record->opcode == 1 && operand->allocation_class_number && input->kind == 1 &&
                ((C2AllocationRegister *)input->original_register)->kind == 2 &&
                input->value.reg->kind != 13 && !input->next && !output->next) {
                link = new_allocation_move();
                link->record = record;
                link->next = moves;
                moves = link;
            }
        }
    }
    for (link = moves; link; link = link->next) {
        record = link->record;
        output = record->tail.right;
        input = record->left;
        assigned = (C2AllocationRegister *)output->original_register;
        if (assigned->use_count == 1) {
            original = output->value.reg;
            replacement = (C2AllocationRegister *)input->original_register;
            source = input->value.reg;
        } else {
            assigned = (C2AllocationRegister *)input->original_register;
            previous = record->previous;
            if (assigned->use_count != 1 || !(previous->flags & 1) || previous->opcode == 0x139)
                continue;
            operand = previous->tail.right;
            if (!operand || operand->kind != 1 ||
                (C2AllocationRegister *)operand->original_register != assigned ||
                ((operand->type ^ record->type) & 0xfff)) continue;
            original = input->value.reg;
            source = output->value.reg;
            replacement = (C2AllocationRegister *)output->original_register;
        }
        last_block = assigned->last_block;
        last = (C2Expression *)last_block->last_record;
        if (!optimizer_set_contains(last_block->allocation_live_in_after, assigned->number)) {
            for (;;) {
                if (last->flags & 1) {
                    for (operand = last->left; operand; operand = operand->next)
                        if (operand->kind == 1 &&
                            (C2AllocationRegister *)operand->original_register == assigned) goto last_coalesced_use;
                }
                last = last->previous;
            }
        }
last_coalesced_use:
        if (allocation_move_is_blocked(source, record, last, assigned)) continue;
        if (last->flags & 1) last = last->next;
        if (original->allocation_root) original = original->allocation_root;
        for (cursor = record->next; cursor != last; cursor = cursor->next) {
            if (!(cursor->flags & 1)) continue;
            for (operand = cursor->left; operand; operand = operand->next) {
                if (operand->kind != 1 || (C2AllocationRegister *)operand->original_register != assigned) continue;
                operand->original_register = (C2FunctionRegister *)replacement;
                if (operand->value.reg == original) operand->value.reg = source;
                else {
                    displacement = (long)((unsigned)operand->value.reg->displacement -
                        (unsigned)original->displacement);
                    view = new_address_register(source, operand->type, operand->type & 0xfff, displacement);
                    operand->value.reg = view;
                    view->allocation_root = source->allocation_root ? source->allocation_root : source;
                }
            }
        }
        block = allocation_block_for_record(record);
        number = assigned->number;
        for (;;) {
            set = block->allocation_live_out;
            if (optimizer_set_contains(set, number)) {
                remove_optimizer_symbol(set, number);
                add_register_to_set(set, replacement->number);
            }
            set = block->allocation_live_in_after;
            if (optimizer_set_contains(set, number)) {
                remove_optimizer_symbol(set, number);
                add_register_to_set(set, replacement->number);
            }
            if (block == last_block) break;
            block = block->previous;
        }
        for (search = replacement->last_block; search; search = search->previous) {
            if (search == block) { replacement->last_block = block; break; }
        }
        type_class = assigned->source->type >> 12;
        /* Reuse only the same two established integer-class rows. */
        if (type_class != 1 && type_class != 2) { __asm { int 3 } }
        remove_optimizer_symbol(classes[0], number);
        release_allocation_register(assigned, 0);
        record = link->record;
        if ((C2AllocationRegister *)record->tail.right->original_register == replacement) {
            operand = record->previous->tail.right;
            operand->value.reg = source;
            operand->original_register = (C2FunctionRegister *)replacement;
        }
        unlink_and_release_input_record((C2Record *)record);
    }
    for (bucket = 0; bucket < 1024; ++bucket)
        for (reg = allocation_register_hash[bucket]; reg; reg = reg->next_hash) reg->use_count = 0;
    if (moves) release_allocation_moves(moves);
}
int __fastcall compare_expression_trees(C2Expression *left, C2Expression *right);
int __fastcall allocation_record_is_rematerializable(C2Expression *record)
{
    C2Expression *input, *first, *second;

    if (record->tail.right->next || record->opcode != 0x12) return 0;
    input = record->left;
    if (input->kind != 5) return 0;
    switch (input->opcode) {
    case 0x122:
        first = input->aggregate_value;
        ALLOCATION_KNOWN_KIND(first->original_register);
        return first->original_register->kind == 2;
    case 0x125:
        if (!c2_favor_speed) return 0;
        second = input->aggregate_second;
        ALLOCATION_KNOWN_KIND(second->original_register);
        return second->original_register->kind == 2;
    case 0x123:
        if (!c2_favor_speed) return 0;
        first = input->aggregate_value;
        ALLOCATION_KNOWN_KIND(first->original_register);
        if (first->original_register->kind != 2) return 0;
        second = input->aggregate_second;
        ALLOCATION_KNOWN_KIND(second->original_register);
        return second->original_register->kind == 2;
    }
    return 0;
}
int __fastcall allocation_use_accepts_definition(C2Expression *record, C2Expression *operand)
{
    C2AllocationRegister *reg = (C2AllocationRegister *)operand->original_register;
    C2Expression *definition = reg->definition_record->left;
    C2Expression *owner = record, *address;

    if (definition->opcode == 0x122) return 1;
    address = find_aggregate_operand_owner(record, operand, &owner);
    if (address->opcode != 0x122) return 0;
    if (definition->tail.right && address->tail.right) return 0;
    return 1;
}
C2Expression *__fastcall allocation_definition_operands(C2Expression *record)
{
    C2Expression *address = record->left, *first;

    if (address->opcode == 0x122) {
        first = address->aggregate_value;
        first->allocation_operand_next = NULL;
    } else if (address->opcode == 0x125) {
        first = address->aggregate_second;
        first->allocation_operand_next = NULL;
    } else {
        first = address->aggregate_value;
        first->allocation_operand_next = address->aggregate_second;
    }
    return first;
}
int __fastcall allocation_rematerialization_is_profitable(C2AllocationRegister *reg,
    C2Expression *definition, C2Expression *last_use) { __asm { int 3 } return 0; }
extern C2AllocationMoveLink *allocation_current_choices;
void __fastcall clear_allocation_choice_record(C2AllocationMoveLink *choices,
    C2Expression *record) { __asm { int 3 } }
void __fastcall remove_input_operand(C2Expression *owner, C2Expression *operand);
void __fastcall rewrite_allocation_deferred_operand(C2Expression *record,
    C2Expression *operand, C2Expression *definition)
{
    C2Expression *first = record, *address, *source, *copy, *prior_definition;
    C2AllocationRegister *assigned;
    unsigned scale;

    address = find_aggregate_operand_owner(record, operand, &first);
    operand->location.expression.properties &= ~0x20;
    remove_input_operand(record, operand);
    source = definition->left;
    if (source->aggregate_value) {
        if (address->aggregate_value == operand) {
            copy = copy_input_expression(source->aggregate_value);
            address->aggregate_value = copy;
            copy->location.expression.properties =
                (copy->location.expression.properties & ~0x10) | 0x20;
            copy_input_operand(record, copy);
        } else {
            scale = address->location.expression.properties & 15;
            if (scale)
                address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
                    ((1U << scale) - 1) * (unsigned)source->detail.aggregate_displacement);
            copy = copy_input_expression(source->aggregate_value);
            address->aggregate_second = copy;
            copy->location.expression.properties =
                (copy->location.expression.properties & ~0x10) | 0x20;
            copy_input_operand(record, copy);
        }
    } else if (address->aggregate_value == operand) address->aggregate_value = NULL;
    source = definition->left;
    if (source->aggregate_second) {
        copy = address->aggregate_second;
        if (copy && copy != operand) address->aggregate_value = copy;
        copy = copy_input_expression(source->aggregate_second);
        address->aggregate_second = copy;
        copy->location.expression.properties =
            (copy->location.expression.properties & ~0x10) | 0x20;
        copy_input_operand(record, copy);
        source = definition->left;
        address->location.expression.properties = (address->location.expression.properties & ~15) |
            (source->location.expression.properties & 15);
    }
    address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
        (unsigned)source->detail.aggregate_displacement);
    if (source->tail.aggregate_register) address->tail.aggregate_register = source->tail.aggregate_register;
    if (!address->aggregate_value) {
        if (address->tail.aggregate_register &&
            (address->tail.aggregate_register->kind == 3 || address->tail.aggregate_register->kind == 4 ||
             address->tail.aggregate_register->kind == 5 || address->tail.aggregate_register->kind == 6))
            address->opcode = 0x124;
        else address->opcode = 0x125;
        assigned = (C2AllocationRegister *)address->aggregate_second->original_register;
        prior_definition = assigned->definition_record;
        if (prior_definition && prior_definition->opcode == 0x12 && prior_definition->left->opcode != 0x122)
            assigned->flags |= 4;
    } else if (!address->aggregate_second) address->opcode = 0x122;
    else {
        address->opcode = 0x123;
        assigned = (C2AllocationRegister *)address->aggregate_value->original_register;
        prior_definition = assigned->definition_record;
        if (prior_definition && prior_definition->opcode == 0x12 && prior_definition->left->opcode != 0x122)
            assigned->flags |= 4;
        assigned = (C2AllocationRegister *)address->aggregate_second->original_register;
        prior_definition = assigned->definition_record;
        if (prior_definition && prior_definition->opcode == 0x12 && prior_definition->left->opcode != 0x122)
            assigned->flags |= 4;
    }
}

void __fastcall rewrite_allocation_definition(C2Expression *definition,
    C2AllocationRegister *reg, C2Expression *operands)
{
    C2AllocationRegister *first = NULL, *second = NULL;
    C2BasicBlock *block, *cursor;
    C2Expression *record, *next, *operand, *after, *copy, *result;
    C2FunctionRegister *output;

    if (operands) {
        first = (C2AllocationRegister *)operands->original_register;
        for (cursor = first->last_block; cursor; cursor = cursor->previous)
            if (cursor == reg->last_block) break;
        if (cursor) first->last_block = reg->last_block;
        if (operands->allocation_operand_next) {
            second = (C2AllocationRegister *)operands->allocation_operand_next->original_register;
            for (cursor = second->last_block; cursor; cursor = cursor->previous)
                if (cursor == reg->last_block) break;
            if (cursor) second->last_block = reg->last_block;
        }
    }
    for (block = reg->first_block; ; block = block->previous) {
        if (!first) remove_optimizer_symbol(block->allocation_region_uses, reg->number);
        else {
            if (optimizer_set_contains(block->allocation_live_out, reg->number)) {
                add_register_to_set(block->allocation_live_out, first->number);
                if (second) add_register_to_set(block->allocation_live_out, second->number);
            }
            if (optimizer_set_contains(block->allocation_live_in_after, reg->number)) {
                add_register_to_set(block->allocation_live_in_after, first->number);
                if (second) add_register_to_set(block->allocation_live_in_after, second->number);
            }
        }
        remove_optimizer_symbol(block->allocation_live_out, reg->number);
        remove_optimizer_symbol(block->allocation_live_in_after, reg->number);
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            if (!(record->flags & 1)) {
                if (record->kind == 26)
                    remove_optimizer_symbol(((C2Record *)record)->owner.registers, reg->number);
                continue;
            }
            if (record != definition && find_final_register_use((C2FunctionRegister *)reg, record)) {
                if (allocation_current_choices) clear_allocation_choice_record(allocation_current_choices, record);
                if (first && !(first->flags & 4) && first->last_use == record) first->last_use = next;
                if (second && !(second->flags & 4) && second->last_use == record) second->last_use = next;
                unlink_and_release_input_record((C2Record *)record);
                continue;
            }
            for (operand = record->left; operand; operand = after) {
                after = operand->next;
                if (operand->kind != 1 || (C2AllocationRegister *)operand->original_register != reg) continue;
                if (operand->location.expression.properties & 0x20) {
                    rewrite_allocation_deferred_operand(record, operand, definition);
                } else {
                    copy = copy_input_expression(definition);
                    replace_input_result(copy, copy->tail.right, record->tail.right);
                    result = copy->tail.right;
                    if ((1U << (result->kind & 31)) & 6) {
                        output = result->value.reg;
                        if (output->value) {
                            if (!output->canonical->variant_next) output->value = copy;
                            else bind_input_temporary(output, copy);
                        }
                    }
                    prepend_input_record((C2Record *)record, (C2Record *)copy);
                    unlink_and_release_input_record((C2Record *)record);
                    break;
                }
            }
        }
        if (block == reg->last_block) break;
    }
    if (first && !(first->flags & 4)) {
        for (record = first->last_use; record; record = record->next)
            if (record == reg->last_use) break;
        if (record) first->last_use = record;
    }
    if (second && !(second->flags & 4)) {
        for (record = second->last_use; record; record = record->next)
            if (record == reg->last_use) break;
        if (record) second->last_use = record;
    }
    unlink_and_release_input_record((C2Record *)definition);
}

void __fastcall build_allocation_preferences(C2Function *function, C2RegisterSet *registers)
{
    C2Expression *record, *operand, *input, *output, *definition, *previous, *value;
    C2AllocationRegister *reg, *next;
    unsigned bucket;
    unsigned char inputs;

    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        for (operand = record->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            reg = (C2AllocationRegister *)operand->original_register;
            if (reg->kind != 2 || (reg->flags & 4)) continue;
            if (operand->location.expression.properties & 0x20) {
                if (!reg->definition_record) reg->flags |= 4;
                else if (!allocation_use_accepts_definition(record, operand)) reg->flags |= 4;
                else reg->last_use = record;
            } else {
                output = record->tail.right;
                if (record->opcode == 1 && output->kind == 1 && !output->next &&
                    !record->left->next && c2_favor_speed && reg->definition_record &&
                    final_register_has_other_use(output->original_register, reg->definition_record))
                    reg->last_use = record;
                else reg->flags |= 4;
            }
        }
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            reg = (C2AllocationRegister *)operand->original_register;
            if (reg->kind != 2) continue;
            definition = reg->definition_record;
            if (!definition) {
                input = record->left;
                if (record->opcode == 1 && ((1U << (input->kind & 31)) & 6) &&
                    input->value.reg->value && input->value.reg->value == record->previous &&
                    allocation_record_is_rematerializable(record->previous)) {
                    previous = record->previous;
                    if (!((record->type ^ previous->type) & 0xfff) && !record->tail.right->next) {
                        replace_input_result(previous, previous->tail.right, record->tail.right);
                        unlink_and_release_input_record((C2Record *)record);
                        record = previous;
                        operand = previous->tail.right;
                        reg = (C2AllocationRegister *)operand->original_register;
                    }
                }
                reg->definition_record = record;
                if (!allocation_record_is_rematerializable(record)) reg->flags |= 4;
                else {
                    inputs = 0;
                    for (input = record->left; input; input = input->next)
                        if (input->kind == 1) ++inputs;
                    record->control_flags = inputs;
                }
            } else if (!(reg->flags & 4)) {
                if (definition->opcode != record->opcode || compare_expression_trees(definition, record))
                    reg->flags |= 4;
            }
        }
    }
    for (bucket = 0; bucket < 1024; ++bucket) {
        for (reg = allocation_register_hash[bucket]; reg; reg = next) {
            next = reg->next_hash;
            if (reg->flags & 4) continue;
            if (optimizer_set_contains(reg->last_block->allocation_live_in_after, reg->number))
                reg->last_use = (C2Expression *)reg->last_block->last_record;
            definition = reg->definition_record;
            value = allocation_definition_operands(definition);
            if (allocation_move_is_blocked(value->value.reg, definition, reg->last_use, reg)) continue;
            if (value->previous &&
                (allocation_move_is_blocked(value->previous->value.reg, definition, reg->last_use, reg) ||
                 definition->control_flags == 1)) continue;
            if (!c2_favor_speed &&
                !allocation_rematerialization_is_profitable(reg, definition, reg->last_use)) continue;
            reg->rewrite_block = allocation_block_for_record(definition);
            rewrite_allocation_definition(definition, reg, value);
            remove_optimizer_symbol(registers, reg->number);
            release_allocation_register(reg, 0);
        }
    }
    for (bucket = 0; bucket < 1024; ++bucket) {
        for (reg = allocation_register_hash[bucket]; reg; reg = reg->next_hash) {
            reg->last_use = NULL;
            reg->definition_record = NULL;
            reg->flags &= ~4;
        }
    }
}
void __fastcall initialize_allocation_work_sets(C2Function *function)
{
    C2BasicBlock *block;

    for (block = function->flow->last; block; block = block->previous) {
        block->allocation_region_uses = copy_optimizer_symbol_set(block->allocation_live_out);
        block->allocation_edge_uses = new_register_set(8);
        block->allocation_edge_definitions = new_register_set(8);
        block->allocation_region_input = new_register_set(6);
        ++allocation_block_count;
    }
}
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);

void __fastcall normalize_final_float_opcode(C2Expression *record)
{
    C2Expression *output = record->tail.right, *input, *first, *second;
    unsigned number;

    switch (record->opcode) {
    case 0x62: case 0x63:
        if (output->kind != 1 || (output->type & 0xf000) != 0x4000) return;
        if (output->original_register != (C2FunctionRegister *)&machine_registers[33] &&
            output->original_register != (C2FunctionRegister *)&machine_registers[34] &&
            output->original_register != (C2FunctionRegister *)&machine_registers[35])
            ALLOCATION_KNOWN_NUMBER(output->original_register);
        number = output->original_register->number;
        if (number <= 31) return;
        for (input = record->left; input; input = input->next) {
            if (input->kind == 1) {
                if (input->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    input->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                    input->original_register != (C2FunctionRegister *)&machine_registers[35])
                    ALLOCATION_KNOWN_NUMBER(input->original_register);
                if (input->original_register->number == number) return;
            }
        }
        copy_input_operand(record, copy_input_expression(output));
        return;
    case 0x86: case 0x91: case 0x92:
        if (output->kind != 1)
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0xccb);
        first = record->left;
        if (first->kind != 1)
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0xccc);
        second = first->next;
        if (!second || second->kind != 1)
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0xccd);
        if (!output->next)
            attach_optimizer_output(record, new_fixed_register_operand(31, output->type));
        output->original_register = (C2FunctionRegister *)(machine_registers + 32);
        output->next->original_register = (C2FunctionRegister *)(machine_registers + 31);
        first = record->left;
        if (first->original_register != (C2FunctionRegister *)&machine_registers[33] &&
            first->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                    first->original_register != (C2FunctionRegister *)&machine_registers[35])
            ALLOCATION_KNOWN_NUMBER(first->original_register);
        if (first->original_register->number == 31) {
            record->left = second = first->next;
            second->next = first;
            first->next = NULL;
        }
        return;
    case 0x87: case 0x8a: case 0xd4:
        if (!record->left->next)
            copy_input_operand(record, new_fixed_register_operand(32, output->type));
        return;
    }
}
int __fastcall final_float_stack_effect(C2Expression *record)
{
    C2Expression *operand;
    int change = 0;

    if (record->kind == 13 || record->opcode == 0x16c || record->opcode == 0x13c) {
        for (operand = record->tail.right; operand; operand = operand->next)
            if (operand->kind == 1 && (operand->type & 0xf000) == 0x4000) ++change;
        for (operand = record->left; operand; operand = operand->next)
            if (operand->kind == 1 && (operand->type & 0xf000) == 0x4000) --change;
        return change;
    }
    if ((record->type & 0xf000) != 0x4000) return 0;
    switch (record->opcode) {
    case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x50:
    case 0x53: case 0x5b: case 0x5d: case 0x63: case 0x69:
    case 0x86: case 0x91: case 0x92: case 0xd2:
        return -1;
    case 0x5e: case 0xd3:
        return -2;
    case 0x56: case 0x60: case 0x64:
    case 0x7f: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84:
    case 0x88: case 0x90: case 0x94:
        return 1;
    }
    return 0;
}
void __fastcall unlink_final_output(C2Expression *record, C2Expression *operand)
{
    C2Expression *scan, *previous;

    previous = NULL;
    for (scan = record->tail.right; scan && scan != operand; scan = scan->next)
        previous = scan;
    /* The original also splices after the last output if operand is absent. */
    if (previous) previous->next = operand->next;
    else record->tail.right = operand->next;
    operand->next = NULL;
    if ((1U << (operand->kind & 31)) & 0x7fe)
        operand->location.expression.properties &= ~0x10;
}

/* This pass reads the independently proved physical-number column only. */
#define FINAL_FLOAT_ORDER_NUMBER(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[33] && \
        (reg) != (C2FunctionRegister *)&machine_registers[34] && \
        (reg) != (C2FunctionRegister *)&machine_registers[35]) \
        ALLOCATION_KNOWN_NUMBER(reg); \
} while (0)
void __fastcall normalize_final_float_order(C2Expression *record)
{
    C2Expression *output, *input, *second;
    unsigned number;

    if (final_float_stack_effect(record) >= 0) {
        if (record->opcode != 0x5c && record->opcode != 0x62) return;
        second = record->tail.right->next;
        if (second && second->kind == 1) {
            FINAL_FLOAT_ORDER_NUMBER(second->original_register);
            if (second->original_register->number == 31) {
                unlink_final_output(record, second);
                release_small_node(second);
            }
        }
        return;
    }
    output = record->tail.right;
    switch (record->opcode) {
    case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: case 0x50:
        FINAL_FLOAT_ORDER_NUMBER(output->original_register);
        if (output->original_register->number == 31) {
            input = record->left;
            FINAL_FLOAT_ORDER_NUMBER(input->original_register);
            number = input->original_register->number;
            if (number > C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
            output->original_register = (C2FunctionRegister *)(machine_registers + number);
        }
        break;
    case 0x86: case 0x91: case 0x92:
        FINAL_FLOAT_ORDER_NUMBER(output->original_register);
        if (output->original_register->number == 31)
            output->original_register = (C2FunctionRegister *)(machine_registers + 32);
        break;
    }
    for (; output; output = output->next)
        if (output->kind == 1 &&
            output->original_register == (C2FunctionRegister *)(machine_registers + 31)) return;
    attach_optimizer_output(record, new_temporary_register_expression(0x11f, 0x4008,
        (C2FunctionRegister *)(machine_registers + 31),
        (C2FunctionRegister *)(machine_registers + 31)));
}
#undef FINAL_FLOAT_ORDER_NUMBER
void __fastcall fold_final_float_record(C2Expression *record)
{
    normalize_final_float_opcode(record);
    normalize_final_float_order(record);
}
C2Real *__fastcall find_symbol_real(C2Symbol *symbol);
C2Real *__fastcall find_final_real_value(C2Expression *operand);
C2Expression *__fastcall new_final_real_bits(C2Real *value, unsigned short type);
void __fastcall normalize_final_move(C2Expression *record)
{
    unsigned short old_type = record->type, type;
    C2BranchTarget *entry;
    C2Expression *input, *operand;
    C2Real *value;

    if (old_type == 0x4004) type = 0x1004;
    else if (old_type == 0x4008) {
        type = 0x1008;
        if (c2_global_optimize && c2_favor_speed) {
            entry = (C2BranchTarget *)record;
            if (entry->kind != 24) entry = get_symbol_branch_target(entry, 24);
            final_lowering_state += frame_reference_weight(5, (short)entry->block->number86);
        }
    } else {
        /* Original defect: its PUSH ECX local is reused as a type WORD
         * outside the two floating widths. Preserve the defined low bits of
         * that Win32 record address, rather than inventing a default type. */
#ifdef __linux__
        /* Native addresses cannot represent those original numeric bits. */
        __asm { int 3 }
#endif
        type = (unsigned short)(unsigned long)record;
    }
    input = record->left;
    record->type = type;
    value = find_final_real_value(input);
    if (value) attach_input_expression(record, input, new_final_real_bits(value, type));
    input = record->left->next;
    if (input) {
        value = find_final_real_value(input);
        if (value) attach_input_expression(record, input, new_final_real_bits(value, type));
    }
    for (operand = record->left; operand; operand = operand->next)
        if (operand->type == old_type && !((1U << (operand->kind & 31)) & 0x180))
            set_input_expression_type(operand, type);
    for (operand = record->tail.right; operand; operand = operand->next)
        if (operand->type == old_type) set_input_expression_type(operand, type);
}
extern int spill_float_count, spill_float_top;
extern unsigned spill_float_order[16];
int __fastcall rewrite_spill_float_block(C2Expression *first, C2Expression *last);
void finish_spill_float_block(void);
void __fastcall restore_spill_float_order(C2Expression *first, C2Expression *last);
void __fastcall finish_spill_float_records(C2Expression *first);
C2Expression *__fastcall find_final_float_source(C2Expression *record)
{
    C2Expression *output = record->tail.right;
    C2Expression *input = record->left, *definition, *first, *last, *end, *scan, *operand;
    C2FunctionRegister *reg;
    unsigned active = 0, i;

    if (output->kind == 1 || input->kind != 1) return NULL;
    reg = input->value.reg;
    if (reg != (C2FunctionRegister *)&machine_registers[31] &&
        reg != (C2FunctionRegister *)&machine_registers[32] &&
        reg != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(reg);
    if (reg->kind != 3 || !(definition = reg->value) || record->type != 0x4004 ||
        output->kind == 6 || definition->opcode != 0x60) return NULL;
    input = definition->left;
    if (input->kind == 1 || input->type != output->type ||
        (input->kind == 6 && input->aggregate_second)) return NULL;
    first = definition->next;
    if (first != record) {
        end = record->previous->next;
        for (scan = first; scan != end; scan = scan->next) {
            if (!(scan->flags & 1)) continue;
            if (active++ > 6) return NULL;
            for (operand = scan->left; operand; operand = operand->next)
                if (input_operands_may_alias(operand, output)) return NULL;
            for (operand = scan->tail.right; operand; operand = operand->next)
                if (input_operands_may_alias(operand, output)) return NULL;
        }
    }
    spill_float_top = 7;
    for (i = 0; i < 8; ++i) spill_float_order[7 - i] = i;
    spill_float_count = 8;
    last = record->previous;
    if (rewrite_spill_float_block(first, last) == -1) return NULL;
    finish_spill_float_block();
    for (i = 0; i < 8; ++i) spill_float_order[7 - i] = i;
    spill_float_top = 6;
    restore_spill_float_order(first, last);
    finish_spill_float_records(record);
    return definition;
}
C2Expression *__fastcall widen_final_integer_record(C2Expression *record);
int c2_real_compare(int condition, C2Real *left, C2Real *right);

void __fastcall finish_register_moves(C2Function *function)
{
    C2Expression *record, *next, *end, *previous, *nearby, *input, *output, *inserted;
    C2Real *constant;
    unsigned opcode, number;
    unsigned short line;

    end = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != end; record = next) {
        next = record->next;
        line = record->location.line;
        if (line) {
            record_source_line = function->source_line_base + line;
            record_line_delta = line;
        }
        if (record->kind != 11) continue;
        fold_final_float_record(record);
        switch (record->opcode) {
        case 0x46:
            input = record->left;
            if (input->next->opcode == 0x121) {
                constant = find_symbol_real(input->next->value.reg->symbol);
                if (constant && c2_real_compare(1, constant, &c2_real_two)) {
                    record->opcode = 0x45;
                    attach_input_expression(record, input->next, input);
                }
            }
            break;
        case 0x5c: case 0x5d: case 0x5e:
            nearby = previous = record->previous;
            if (previous->kind != 11) break;
            for (;;) {
                opcode = previous->opcode;
                if (opcode != 0x76 && opcode != 0x75 && opcode != 0x77 && opcode != 0x89 &&
                    opcode != 0x8c && opcode != 0xd7 && opcode != 0xd6) break;
                if (previous->tail.right->value.reg != previous->next->left->value.reg) break;
                previous = previous->previous;
                if (previous->kind != 11) break;
            }
            if (previous->kind == 11 && next->kind == 11 && previous->opcode == 0x60 &&
                next->opcode == 0x63 && previous->left->kind == 1 &&
                previous->left->original_register == (C2FunctionRegister *)(machine_registers + 31) &&
                next->tail.right->kind == 1 &&
                next->tail.right->original_register == (C2FunctionRegister *)(machine_registers + 31) &&
                record->left->kind == 1 &&
                (record->left->value.reg == previous->tail.right->value.reg || nearby != previous)) {
                attach_input_expression(previous->next, previous->next->left, previous->left);
                inserted = new_temporary_register_expression(0x11f, record->type,
                    (C2FunctionRegister *)(machine_registers + 31),
                    (C2FunctionRegister *)(machine_registers + 31));
                attach_optimizer_output(record, inserted);
                input = record->left->next;
                if (input->kind == 1) {
                    ALLOCATION_KNOWN_NUMBER(input->original_register);
                    number = input->original_register->number;
                    /* Original494708 + number*80 is the established table
                     * identity at index(number-1). No row contents are read. */
                    if (!number || number > C2_MACHINE_REGISTER_COUNT + 1) { __asm { int 3 } }
                    input->original_register = (C2FunctionRegister *)(machine_registers + number - 1);
                }
                next = next->next;
                unlink_and_release_input_record((C2Record *)previous);
                unlink_and_release_input_record((C2Record *)record->next);
            }
            break;
        case 0x63:
            if (c2_float_precision) break;
            previous = record->previous;
            if (previous->kind == 11 && previous->opcode == 0x60) {
                input = previous->left;
                output = record->tail.right;
                if (input->kind != 1 && output->kind != 1 && input->type == output->type &&
                    (c2_favor_speed || previous->type == 0x4004) &&
                    (input->kind != 6 || !input->aggregate_second) &&
                    (output->kind != 6 || !output->aggregate_second)) goto combine_final_float_move;
            }
            previous = find_final_float_source(record);
            if (!previous) break;
combine_final_float_move:
            previous->opcode = 1;
            replace_input_result(previous, previous->tail.right, record->tail.right);
            normalize_final_move(previous);
            if (previous->left->kind == 1) make_global_register_operand(previous->left);
            if (previous->tail.right->kind == 1) make_global_register_operand(previous->tail.right);
            next = record->next;
            unlink_and_release_input_record((C2Record *)record);
            prepare_final_record_operands(previous);
            finish_final_integer_record(previous);
            if (previous->type == 0x1008) widen_final_integer_record(previous);
            break;
        }
    }
}
void __fastcall apply_allocation_spills(C2Function *function, C2RegisterSet *registers, int mode);
void __fastcall finish_register_moves(C2Function *function);
C2Record *__fastcall select_allocation_spill_choice(C2Expression *record,
    C2RegisterSet *live, C2RegisterSet *first, C2RegisterSet *second);
void __fastcall insert_float_allocation_barrier(C2Expression *record, C2RegisterSet *registers)
{
    if (record->next->opcode == 0x13a) record = record->next;
    select_allocation_spill_choice(record, NULL, NULL, registers);
}
void __fastcall prepare_float_division_operands(C2Expression *record)
{
    C2Expression *first = record->left, *other = first->next;
    C2Expression *load, *start, *scan, *input, *replacement, *branch;
    C2FunctionRegister *first_register, *loaded_register;
    unsigned condition;

    /* Only the kind column is read in these fixed-register tests. */
    if (first->original_register != (C2FunctionRegister *)&machine_registers[31] &&
        first->original_register != (C2FunctionRegister *)&machine_registers[32])
        ALLOCATION_KNOWN_KIND(first->original_register);
    if (first->original_register->kind != 1 || other->kind != 1) return;
    ALLOCATION_KNOWN_KIND(other->original_register);
    first_register = other->original_register;
    if (first_register->kind != 2) return;
    load = first->value.reg->value;
    if (load->opcode != 0x60 || load->left->kind != 1) return;
    loaded_register = load->left->original_register;
    ALLOCATION_KNOWN_KIND(loaded_register);
    if (loaded_register->kind != 2 || load->next != record) return;
    start = record->previous;
    for (scan = start; scan->flags & 1; scan = scan->previous)
        if (scan->tail.right && scan->tail.right->kind == 1 &&
            scan->tail.right->original_register == loaded_register) break;
    if (!(scan->flags & 1) || scan->opcode != 0x63) return;
    load = scan->left->value.reg->value;
    if (load->opcode != 0x60 || load->left->kind != 1) return;
    input = load->left;
    ALLOCATION_KNOWN_KIND(input->original_register);
    if (input->original_register->kind != 2) return;
    for (scan = start; scan->flags & 1; scan = scan->previous)
        if (scan->tail.right && scan->tail.right->kind == 1 &&
            scan->tail.right->original_register == first_register) break;
    if (!(scan->flags & 1) || scan->opcode != 0x63 ||
        scan->left->value.reg->value->opcode == 0x60) return;
    ALLOCATION_KNOWN_KIND(input->value.reg);
    if (input->value.reg->kind == 3) {
        for (scan = load->next; scan != record; scan = scan->next) {
            for (other = scan->tail.right; other; other = other->next)
                if (((1U << (other->kind & 31)) & 6) &&
                    other->value.reg->canonical == input->value.reg->canonical) break;
            if (other) break;
        }
        if (scan == record) attach_input_expression(start, start->left, input);
    }
    input = start->left;
    replacement = copy_input_expression(input);
    attach_input_expression(start, input, first->next);
    attach_input_expression(record, first->next, replacement);
    for (branch = record->next; branch->opcode != 0xf; branch = branch->next) {}
    condition = branch->condition_code;
    /* Established48d250 BYTE relation swaps ordered comparison operands;
     * codes0..2 stay,3..10 pair-swap and11..17 map to0. */
    if (condition > 17) { __asm { int 3 } }
    if (condition >= 3 && condition <= 10)
        branch->condition_code = (unsigned char)((condition & 1) ? condition + 1 : condition - 1);
    else if (condition >= 11) branch->condition_code = 0;
}
#define FLOAT_ALLOCATION_KIND(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[33] && \
        (reg) != (C2FunctionRegister *)&machine_registers[74]) ALLOCATION_KNOWN_KIND(reg); \
} while (0)
C2RegisterSet *__fastcall prepare_float_allocation(C2Function *function, C2RegisterSet *registers)
{
    C2BasicBlock *block;
    C2Expression *record, *scan, *operand, *output, *input, *result, *inserted;
    C2FunctionRegister *temporary;
    C2RegisterSet *live = new_register_set(6), *retry = new_register_set(6), *single;
    unsigned phase, number;
    unsigned short type;
    int depth = 0, pending = 0, barrier;

    for (block = function->flow->last; block; block = block->previous) {
        /* The genuine allocator requests32 bytes. No fields in this new
         * per-block state are interpreted until their own consumers. */
        block->float_allocation = c2_pool_allocate(4, 32);
        copy_optimizer_symbol_set_into(live, registers);
        intersect_optimizer_symbol_sets(live, block->allocation_live_in_after);
        for (record = (C2Expression *)block->last_record->previous;
             ; record = record->previous) {
            if (record->location.line) {
                record_source_line = function->source_line_base + record->location.line;
                record_line_delta = record->location.line;
            }
            if (pending && !depth) {
                pending = 0;
                if (live->links.first) {
                    insert_float_allocation_barrier(record, live);
                    merge_optimizer_symbol_sets(retry, live);
                    block->flags |= 0x40000;
                }
                for (scan = record->next; scan->kind != 24; scan = scan->next) {
                    if (!(scan->flags & 1)) continue;
                    for (phase = 0; phase != 2; ++phase) {
                        if (!phase && !scan->float_depth_before) continue;
                        for (operand = phase ? scan->left : scan->tail.right;
                             operand; operand = operand->next) {
                            if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                            FLOAT_ALLOCATION_KIND(operand->original_register);
                            if (operand->original_register->kind != 2) continue;
                            ALLOCATION_KNOWN_NUMBER(operand->original_register);
                            single = new_register_set(6);
                            add_register_to_set(single, operand->original_register->number);
                            insert_float_allocation_barrier(scan, single);
                            insert_float_allocation_barrier(scan->previous, single);
                            release_optimizer_symbol_set(single);
                            add_register_to_set(retry, operand->original_register->number);
                            block->flags |= 0x40000;
                        }
                    }
                    if (!scan->float_depth_before) break;
                }
            }
            if (!(record->flags & 1)) {
                if (record->kind == 24) break;
                continue;
            }
            barrier = record->kind == 13 &&
                (record->left->kind != 4 || !(record->left->value.symbol->flags & 0x10000));
            if (depth && barrier) pending = 1;
            if ((barrier || record->kind == 20 || record->opcode == 0x15c) && live->links.first) {
                insert_float_allocation_barrier(record, live);
                merge_optimizer_symbol_sets(retry, live);
                block->flags |= 0x40000;
            }
            record->float_depth_before = record->float_depth_after = (unsigned char)depth;
            if (depth == 8) pending = 1;
            output = record->tail.right;
            if (output && (output->type & 0xf000) == 0x4000 && output->kind == 1) {
                FLOAT_ALLOCATION_KIND(output->original_register);
                if (output->original_register->kind == 2) {
                    ALLOCATION_KNOWN_NUMBER(output->original_register);
                    number = output->original_register->number;
                    remove_optimizer_symbol(live, number);
                    add_register_to_set(block->allocation_region_uses, number);
                } else {
                    FLOAT_ALLOCATION_KIND(output->value.reg);
                    if (output->value.reg->kind == 1 && record->opcode != 0x63 &&
                        record->opcode != 0x5f &&
                        (record->opcode != 0x14c || (record->type & 0xf000) == 0x4000)) {
                        type = output->type;
                        temporary = new_temporary_register(type);
                        temporary->value = record;
                        input = new_temporary_register_expression(0x11f, type, temporary,
                            (C2FunctionRegister *)&machine_registers[31]);
                        result = new_temporary_register_expression(0x11f, type,
                            (C2FunctionRegister *)&machine_registers[31],
                            (C2FunctionRegister *)&machine_registers[31]);
                        inserted = new_binary_expression(0x62, type, result, input,
                            insert_input_record, (C2Record *)record);
                        inserted->float_depth_before = inserted->float_depth_after = (unsigned char)depth;
                        replace_input_result(record, output, inserted->left);
                        depth = (int)((unsigned)depth + 1U);
                        record->float_depth_before = record->float_depth_after = (unsigned char)depth;
                    }
                }
            }
            if (record->opcode == 0x5d) prepare_float_division_operands(record);
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    operand->previous = NULL;
                    if ((operand->type & 0xf000) == 0x4000) {
                        FLOAT_ALLOCATION_KIND(operand->original_register);
                        if (operand->original_register->kind == 2) {
                            ALLOCATION_KNOWN_NUMBER(operand->original_register);
                            number = operand->original_register->number;
                            if (!optimizer_set_contains(live, number)) {
                                add_register_to_set(live, number);
                                operand->location.expression.use_flags |= 0x10;
                            } else operand->location.expression.use_flags &= ~0x10;
                        }
                    }
                } else if (operand->kind == 2) operand->previous = NULL;
            }
            depth = (int)((unsigned)depth - (unsigned)final_float_stack_effect(record));
        }
    }
    release_optimizer_symbol_set(live);
    if (!retry->links.first) {
        release_optimizer_symbol_set(retry);
        retry = NULL;
    }
    return retry;
}
#undef FLOAT_ALLOCATION_KIND
int __fastcall float_allocation_store_cost(C2FunctionRegister *reg, C2Function *function)
{
    if (!c2_favor_speed) {
        ALLOCATION_KNOWN_KIND(reg);
        return reg->kind >= 3 && reg->kind <= 6 ? 3 : 6;
    }
    /* The input is a genuine function register; static rows with unknown
     * size fields remain outside this cost consumer's modeled domain. */
    if ((unsigned long)reg >= (unsigned long)machine_registers &&
        (unsigned long)reg <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
    if (reg->value_bytes == 4 || reg->kind == 7 || reg->kind == 8) return 2;
    if ((function->flags & 0x300000) && reg->kind != 5) return 2;
    return 4;
}
int __fastcall float_allocation_record_needs_reload(C2Expression *record)
{
    C2FunctionRegister *reg = record->tail.right->value.reg;
    C2Expression *next;

    if (reg != (C2FunctionRegister *)&machine_registers[31] &&
        reg != (C2FunctionRegister *)&machine_registers[32] &&
        reg != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(reg);
    if (reg->kind != 3 || !reg->value) return 0;
    next = record->next;
    if (!(next->flags & 1) || !next->left || next->left->kind != 1 ||
        next->left->value.reg != reg) return 0;
    switch (next->opcode) {
    case 0x5c: case 0x5d: case 0x5e: case 0x62: case 0x63:
        return 1;
    }
    return 0;
}
int __fastcall float_allocation_load_cost(C2FunctionRegister *reg, C2Function *function)
{
    if (!c2_favor_speed) {
        ALLOCATION_KNOWN_KIND(reg);
        return reg->kind >= 3 && reg->kind <= 6 ? 1 : 4;
    }
    if ((unsigned long)reg >= (unsigned long)machine_registers &&
        (unsigned long)reg <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
    if (reg->value_bytes == 4 || reg->kind == 7 || reg->kind == 8) return 0;
    if ((function->flags & 0x300000) && reg->kind != 5) return 0;
    return 1;
}
int __fastcall float_allocation_reload_cost(C2FunctionRegister *reg, C2Function *function)
{
    if (!c2_favor_speed) {
        ALLOCATION_KNOWN_KIND(reg);
        return reg->kind >= 3 && reg->kind <= 6 ? 3 : 6;
    }
    if ((unsigned long)reg >= (unsigned long)machine_registers &&
        (unsigned long)reg <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
    if (reg->value_bytes == 4 || reg->kind == 7 || reg->kind == 8) return 1;
    if ((function->flags & 0x300000) && reg->kind != 5) return 1;
    return 2;
}
void __fastcall move_final_label_after(C2Expression *after, C2Expression *label);
#define FLOAT_REGION_KIND(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(reg); \
} while (0)
void __fastcall initialize_float_allocation_regions(C2Function *function, C2RegisterSet *registers)
{
    C2RegisterSet *live = new_register_set(6), *leaving = new_register_set(6);
    C2BasicBlock *block, *successor;
    C2Expression *record, *next, *operand, *position;
    C2AllocationRegister *reg;
    C2AllocationMoveLink *link, *found;
    C2FlowEdge *edge;
    unsigned number, weight;
    int cost, repeated;

    for (block = function->flow->last; block; block = block->previous) {
        if (!block->float_allocation) {
            block->float_allocation = c2_pool_allocate(4, 32);
            /* Exact32-byte runtime state copy from the actual predecessor.
             * Its individual fields remain opaque at this consumer. */
            memcpy(block->float_allocation, block->incoming->source->float_allocation, 32);
        }
        if (!optimizer_symbol_sets_intersect(block->allocation_region_uses, registers)) continue;
        copy_optimizer_symbol_set_into(live, registers);
        intersect_optimizer_symbol_sets(live, block->allocation_live_out);
        for (number = next_optimizer_symbol(live); number != (unsigned)-1;
             number = next_optimizer_symbol(NULL)) {
            reg = lookup_allocation_register(number);
            if (!reg->first_block) reg->first_block = block;
            reg->last_block = block;
        }
        weight = c2_favor_speed ? 1U << (((unsigned)block->region_depth + 1U) & 31) : 1;
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            if (!(record->flags & 1) || (record->type & 0xf000) != 0x4000) continue;
            if (record->location.line) {
                record_source_line = function->source_line_base + record->location.line;
                record_line_delta = record->location.line;
            }
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                FLOAT_REGION_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 1)) continue;
                if (record->opcode == 0x13a) {
                    if (!c2_favor_speed || !block->block_scope ||
                        optimizer_set_contains(block->block_scope->allocation_eligible,
                            operand->value.reg->allocation->number)) {
                        cost = float_allocation_store_cost(operand->value.reg, function);
                        reg->weighted_pressure = (int)((unsigned)reg->weighted_pressure - (unsigned)cost * weight);
                        reg->priority = (int)((unsigned)reg->priority - (unsigned)cost * weight);
                    }
                } else if (!(operand->location.expression.use_flags & 0x10) ||
                           float_allocation_record_needs_reload(record)) {
                    if (record->opcode == 0x60 && (operand->location.expression.use_flags & 0x10))
                        cost = float_allocation_reload_cost(operand->value.reg, function);
                    else cost = float_allocation_load_cost(operand->value.reg, function);
                    reg->weighted_pressure = (int)((unsigned)reg->weighted_pressure + (unsigned)cost * weight);
                    reg->priority = (int)((unsigned)reg->priority + (unsigned)cost * weight);
                }
                if (!(operand->location.expression.use_flags & 0x10)) continue;
                repeated = 0;
                position = record;
                if (record->opcode != 0x13a) {
                    for (; position; position = position->next) {
                        if (!(position->flags & 1)) continue;
                        if (position->opcode == 0x63 && position->tail.right->kind == 1 &&
                            position->tail.right->original_register == (C2FunctionRegister *)reg) {
                            repeated = 1;
                            operand->location.expression.use_flags &= ~0x10;
                        }
                        if (!position->float_depth_after && position->opcode != 0x139 &&
                            position->opcode != 0x13a) break;
                    }
                }
                if (!repeated) {
                    link = new_allocation_move();
                    link->next = reg->float_uses;
                    link->record = position;
                    reg->float_uses = link;
                    remove_optimizer_symbol(live, reg->number);
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                FLOAT_REGION_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 1)) continue;
                if (record->opcode == 0x139) {
                    cost = float_allocation_reload_cost(operand->value.reg, function);
                    reg->weighted_pressure = (int)((unsigned)reg->weighted_pressure - (unsigned)cost * weight);
                    reg->priority = (int)((unsigned)reg->priority - (unsigned)cost * weight);
                    position = record->previous;
                    while (position->kind != 24 && (!(position->flags & 1) ||
                           position->opcode == 0x139 || position->opcode == 0x13a)) position = position->previous;
                    if (position->flags & 1) {
                        while ((position->flags & 1) && (position->float_depth_after ||
                               position->opcode == 0x139 || position->opcode == 0x13a)) position = position->previous;
                        if (position->next->opcode == 0x13a) position = position->next;
                        move_final_label_after(position, record);
                    }
                } else {
                    cost = float_allocation_store_cost(operand->value.reg, function);
                    reg->weighted_pressure = (int)((unsigned)reg->weighted_pressure + (unsigned)cost * weight);
                    reg->priority = (int)((unsigned)reg->priority + (unsigned)cost * weight);
                }
                number = reg->number;
                if (!optimizer_set_contains(live, number)) {
                    link = new_allocation_move();
                    link->next = reg->float_definitions;
                    link->record = record;
                    reg->float_definitions = link;
                    if (!reg->first_block) reg->first_block = block;
                    reg->last_block = block;
                    add_register_to_set(live, number);
                }
            }
        }
        if (!live->links.first) continue;
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            successor = edge->destination;
            copy_optimizer_symbol_set_into(leaving, live);
            subtract_optimizer_symbol_sets(leaving, successor->allocation_live_out);
            if (!leaving->links.first) continue;
            if (c2_favor_speed) weight = 1U << (((unsigned)successor->region_depth + 1U) & 31);
            position = (C2Expression *)successor->records;
            for (number = next_optimizer_symbol(leaving); number != (unsigned)-1;
                 number = next_optimizer_symbol(NULL)) {
                reg = lookup_allocation_register(number);
                reg->weighted_pressure = (int)((unsigned)reg->weighted_pressure - weight);
                reg->priority = (int)((unsigned)reg->priority - weight);
                found = reg->float_uses;
                while (found && found->record != position) found = found->next;
                if (!found) {
                    link = new_allocation_move();
                    link->record = position;
                    link->next = reg->float_uses;
                    reg->float_uses = link;
                }
            }
        }
    }
    release_optimizer_symbol_set(live);
    release_optimizer_symbol_set(leaving);
}
#undef FLOAT_REGION_KIND

C2AllocationRegister *__fastcall order_float_allocation_registers(C2RegisterSet *registers,
    C2AllocationRegister *order)
{
    C2AllocationRegister *reg, *position;
    C2AllocationMoveLink *link;
    unsigned number;

    if (registers->links.first) {
        for (number = next_optimizer_symbol(registers); number != (unsigned)-1;
             number = next_optimizer_symbol(NULL)) {
            reg = lookup_allocation_register(number);
            reg->flags &= ~1;
            if (!reg->float_uses) {
                link = new_allocation_move();
                link->next = NULL;
                link->record = (C2Expression *)reg->last_block->last_record;
                reg->float_uses = link;
            }
            if (!order) {
                order = reg;
                reg->float_next = reg->float_previous = NULL;
            } else if (reg->weighted_pressure > order->weighted_pressure ||
                       (reg->weighted_pressure == order->weighted_pressure &&
                        reg->definition_sequence >= order->definition_sequence)) {
                reg->float_previous = NULL;
                reg->float_next = order;
                order->float_previous = reg;
                order = reg;
            } else {
                position = order;
                while (position->float_next &&
                       (position->weighted_pressure > reg->weighted_pressure ||
                        (position->weighted_pressure == reg->weighted_pressure &&
                         position->definition_sequence > reg->definition_sequence)))
                    position = position->float_next;
                if (position->weighted_pressure > reg->weighted_pressure ||
                    (position->weighted_pressure == reg->weighted_pressure &&
                     position->definition_sequence > reg->definition_sequence)) {
                    position->float_next = reg;
                    reg->float_previous = position;
                    reg->float_next = NULL;
                } else {
                    reg->float_next = position;
                    reg->float_previous = position->float_previous;
                    position->float_previous->float_next = reg;
                    position->float_previous = reg;
                }
            }
        }
    }
    clear_register_set(registers);
    return order;
}
void __fastcall move_float_allocation_region(C2Expression *record)
{ __asm { int 3 } }
C2AllocationRegister *__fastcall select_float_allocation_register(C2Function *function,
    C2AllocationRegister *order, C2RegisterSet *retry)
{
    C2AllocationRegister *reg, *next;
    C2BasicBlock *block;
    C2Expression *record, *next_record, *operand;
    unsigned number;
    int side;

    for (reg = order; reg; reg = next) {
        next = reg->float_next;
        if (reg->priority > 0 || (retry && optimizer_set_contains(retry, reg->number))) continue;
        if (reg == order) {
            order = next;
            if (next) next->float_previous = NULL;
        } else {
            reg->float_previous->float_next = next;
            if (next) next->float_previous = reg->float_previous;
        }
        number = reg->number;
        for (block = reg->first_block; ; block = block->previous) {
            if (!optimizer_set_contains(block->allocation_region_uses, number)) continue;
            remove_optimizer_symbol(block->allocation_region_uses, number);
            remove_optimizer_symbol(block->allocation_live_out, number);
            remove_optimizer_symbol(block->allocation_live_in_after, number);
            for (record = (C2Expression *)block->records;
                 record != (C2Expression *)block->last_record; record = next_record) {
                next_record = record->next;
                if (!(record->flags & 1) || (record->type & 0xf000) != 0x4000) continue;
                if (record->location.line) {
                    record_source_line = function->source_line_base + record->location.line;
                    record_line_delta = record->location.line;
                }
                if (record->opcode == 0x139) {
                    if (record->tail.right->original_register == (C2FunctionRegister *)reg)
                        unlink_and_release_input_record((C2Record *)record);
                } else if (record->opcode == 0x13a) {
                    if (record->left->original_register == (C2FunctionRegister *)reg)
                        unlink_and_release_input_record((C2Record *)record);
                } else {
                    for (side = 0; side != 2; ++side) {
                        for (operand = side ? record->left : record->tail.right;
                             operand; operand = operand->next) {
                            if (operand->kind != 1 ||
                                operand->original_register != (C2FunctionRegister *)reg) continue;
                            make_global_register_operand(operand);
                            if (!side && record->float_depth_after)
                                move_float_allocation_region(record);
                        }
                    }
                }
            }
            if (block == reg->last_block) break;
        }
        release_allocation_moves(reg->float_definitions);
        release_allocation_moves(reg->float_uses);
        release_allocation_register(reg, 0);
    }
    return order;
}
C2AllocationRegister *__fastcall update_float_allocation_retry(C2AllocationRegister *order,
    C2RegisterSet *retry)
{
    C2AllocationRegister *reg, *previous, *next;
    unsigned number;

    for (number = next_optimizer_symbol(retry); number != (unsigned)-1;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_allocation_register(number);
        reg->flags |= 2;
        release_allocation_moves(reg->float_definitions);
        release_allocation_moves(reg->float_uses);
        if (reg == order) {
            order = order->float_next;
            if (order) order->float_previous = NULL;
        } else {
            previous = reg->float_previous;
            next = reg->float_next;
            previous->float_next = next;
            if (next) next->float_previous = previous;
        }
    }
    return order;
}
C2RegisterSet *__fastcall float_allocation_live_at_record(C2Expression *record,
    C2RegisterSet *assigned)
{
    C2RegisterSet *live = copy_optimizer_symbol_set(assigned);
    C2BasicBlock *block;
    C2Expression *position, *operand;
    C2AllocationRegister *reg;

    if (record->kind == 24) {
        intersect_optimizer_symbol_sets(live, ((C2Record *)record)->owner.block->allocation_live_out);
        return live;
    }
    block = allocation_block_for_record(record);
    intersect_optimizer_symbol_sets(live, block->allocation_live_out);
    for (position = (C2Expression *)block->records; position != record; position = position->next) {
        if (!(position->flags & 1) || (position->type & 0xf000) != 0x4000) continue;
        for (operand = position->left; operand; operand = operand->next) {
            if (operand->kind != 1 || !(operand->location.expression.use_flags & 0x10)) continue;
            if (operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[33])
                ALLOCATION_KNOWN_KIND(operand->original_register);
            reg = (C2AllocationRegister *)operand->original_register;
            if (reg->kind == 2 && reg->pinned_register) remove_optimizer_symbol(live, reg->number);
        }
        for (operand = position->tail.right; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            if (operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[33])
                ALLOCATION_KNOWN_KIND(operand->original_register);
            reg = (C2AllocationRegister *)operand->original_register;
            if (reg->kind == 2 && reg->pinned_register) add_register_to_set(live, reg->number);
        }
    }
    return live;
}
int __fastcall spill_float_allocation_use(C2Expression *record, C2AllocationRegister *reg)
{
    C2BasicBlock *block = allocation_block_for_record(record);
    C2Expression *position, *operand, *output;
    C2RegisterSet *registers;
    int live;

    if (!optimizer_set_contains(block->allocation_region_uses, reg->number)) return 0;
    live = optimizer_set_contains(block->allocation_live_out, reg->number) != 0;
    for (position = (C2Expression *)block->records; ; position = position->next) {
        if (position->flags & 1) {
            if (!live) {
                if (find_final_register_use((C2FunctionRegister *)reg, position)) live = 1;
            } else {
                operand = final_register_has_other_use((C2FunctionRegister *)reg, position);
                if (operand && (operand->location.expression.use_flags & 0x10)) live = 0;
            }
        }
        if (position == record) break;
    }
    if (!live) return 0;
    registers = new_register_set(6);
    add_register_to_set(registers, reg->number);
    insert_float_allocation_barrier(record, registers);
    output = record->tail.right;
    if (record->opcode == 0x63 && output->kind == 1) {
        if (output->original_register != (C2FunctionRegister *)&machine_registers[31] &&
            output->original_register != (C2FunctionRegister *)&machine_registers[32] &&
            output->original_register != (C2FunctionRegister *)&machine_registers[33])
            ALLOCATION_KNOWN_KIND(output->original_register);
        if (output->original_register->kind == 2 && record->float_depth_after) {
            for (position = record->next; position->kind != 24; position = position->next) {
                if (!(position->flags & 1) || position->opcode == 0x139 || position->opcode == 0x13a)
                    continue;
                if (!position->float_depth_after) break;
                if (final_register_has_other_use((C2FunctionRegister *)reg, position)) {
                    insert_float_allocation_barrier(position, registers);
                    insert_float_allocation_barrier(position->previous, registers);
                }
            }
        }
    }
    release_optimizer_symbol_set(registers);
    block->flags |= 0x40000;
    return 1;
}
C2Expression *__fastcall move_float_allocation_use(C2AllocationRegister *reg,
    C2Expression *from, C2Expression *to, C2RegisterSet *registers)
{ __asm { int 3 } return NULL; }
/* The32-byte block state is eight opaque register identities. This consumer
 * counts the populated prefix only; it does not inspect their pointees. */
struct C2FloatBlockState { C2AllocationRegister *stack[8]; };
typedef char C2FloatBlockStateSizeCheck[sizeof(struct C2FloatBlockState) == 32 ? 1 : -1];
int __fastcall float_depth_before_record(C2Expression *record)
{
    C2Expression *position;
    C2AllocationRegister *reg;
    C2BasicBlock *block;
    struct C2FloatBlockState *state;
    unsigned count;
    int pushed = 0;

    if (record->opcode != 0x139 && record->opcode != 0x13a)
        return (int)record->float_depth_before - (int)record->float_depth_after;
    position = record->previous;
    for (;;) {
        if (position->kind == 24 && ((C2Record *)position)->owner.block) break;
        if ((position->flags & 1) &&
            ((position->type & 0xf000) == 0x4000 || position->kind == 13)) {
            if (position->opcode == 0x139) {
                reg = (C2AllocationRegister *)position->tail.right->original_register;
                if ((unsigned long)reg >= (unsigned long)machine_registers &&
                    (unsigned long)reg <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                    { __asm { int 3 } }
                if (reg->pinned_register) {
                    ++pushed;
                    position = position->previous;
                    continue;
                }
            } else if (position->opcode != 0x13a) break;
        }
        position = position->previous;
    }
    if (position->flags & 1) return (int)position->float_depth_before + pushed;
    block = ((C2Record *)position)->owner.block;
    if (!block->incoming) return pushed;
    state = block->incoming->source->float_allocation;
    for (count = 0; count < 8 && state->stack[count]; ++count) {}
    return (int)count + pushed;
}
int __fastcall rebalance_float_allocation(C2Function *function, C2AllocationRegister *reg)
{ __asm { int 3 } return 0; }
int __fastcall float_allocation_use_conflicts(C2AllocationRegister *reg, C2Expression *record)
{
    C2Expression *position, *output;
    C2AllocationRegister *other;
    C2RegisterSet *registers;
    int conflict = 0;

    if (!(record->flags & 1)) return 0;
    while (!final_register_has_other_use((C2FunctionRegister *)reg, record))
        record = record->previous;
    position = record->previous;
    while ((position->flags & 1) && (position->float_depth_after ||
           position->opcode == 0x139 || position->opcode == 0x13a)) {
        output = position->tail.right;
        if (output && (output->type & 0xf000) == 0x4000 && output->kind == 1) {
            if (output->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                output->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                output->original_register != (C2FunctionRegister *)&machine_registers[33])
                ALLOCATION_KNOWN_KIND(output->original_register);
            other = (C2AllocationRegister *)output->original_register;
            if (other->kind == 2 && position->opcode != 0x139 && other != reg) {
                conflict = 1;
                break;
            }
        }
        position = position->previous;
    }
    if (!conflict) return 0;
    allocation_block_for_record(position)->flags |= 0x40000;
    registers = new_register_set(6);
    add_register_to_set(registers, reg->number);
    insert_float_allocation_barrier(record, registers);
    insert_float_allocation_barrier(record->previous, registers);
    release_optimizer_symbol_set(registers);
    return 1;
}
void __fastcall release_float_allocation_adjustment(C2Expression *record)
{ __asm { int 3 } }
int __fastcall try_allocate_float_register(C2Function *function, C2AllocationRegister *reg,
    C2RegisterSet *assigned)
{
    C2AllocationMoveLink *definitions = NULL, *definition_records = NULL;
    C2AllocationMoveLink *uses = NULL, *use_records = NULL;
    C2AllocationMoveLink *link, *item, *definition, *use, *definition_record, *use_record, *found;
    C2AllocationRegister *other;
    C2RegisterSet *difference;
    C2Expression *record, *common, *adjustment = NULL;
    unsigned number;
    int changed = 0, depth;

    for (link = reg->float_definitions; link; link = link->next) {
        item = new_allocation_move();
        item->next = definitions;
        item->registers = float_allocation_live_at_record(link->record, assigned);
        definitions = item;
        item = new_allocation_move();
        item->next = definition_records;
        item->record = link->record;
        definition_records = item;
    }
    for (link = reg->float_uses; link; link = link->next) {
        item = new_allocation_move();
        item->next = uses;
        item->registers = float_allocation_live_at_record(link->record, assigned);
        uses = item;
        item = new_allocation_move();
        item->next = use_records;
        item->record = link->record;
        use_records = item;
    }
    difference = new_register_set(6);
    definition_record = definition_records;
    for (definition = definitions; definition; definition = definition->next) {
        use_record = use_records;
        for (use = uses; use; use = use->next) {
            copy_optimizer_symbol_set_into(difference, definition->registers);
            subtract_optimizer_symbol_sets(difference, use->registers);
            if (difference->links.first) {
                for (number = next_optimizer_symbol(difference); number != (unsigned)-1;
                     number = next_optimizer_symbol(NULL)) {
                    other = lookup_allocation_register(number);
                    for (found = other->float_uses; found; found = found->next)
                        if (found->record == use_record->record) break;
                    if (!found)
                        for (link = other->float_uses; link; link = link->next)
                            if (spill_float_allocation_use(link->record, reg)) changed = 1;
                }
            }
            copy_optimizer_symbol_set_into(difference, use->registers);
            subtract_optimizer_symbol_sets(difference, definition->registers);
            if (difference->links.first) {
                if (definition_record->record->opcode == 0x139) {
                    record = definition_record->record->next;
                    while (record->opcode == 0x139) {
                        other = (C2AllocationRegister *)record->tail.right->original_register;
                        if (other->pinned_register && optimizer_set_contains(difference, other->number)) {
                            remove_optimizer_symbol(difference, other->number);
                            add_register_to_set(definition->registers, other->number);
                            move_final_label_after(record, definition_record->record);
                            record = definition_record->record;
                        }
                        record = record->next;
                    }
                }
                if (difference->links.first) {
                    if (!use_record->next) {
                        common = NULL;
                        for (number = next_optimizer_symbol(difference); number != (unsigned)-1;
                             number = next_optimizer_symbol(NULL)) {
                            other = lookup_allocation_register(number);
                            link = other->float_uses;
                            if (link->next) break;
                            if (!common) common = link->record;
                            else if (common != link->record) break;
                        }
                        if (number == (unsigned)-1 && common != use_record->record) {
                            adjustment = move_float_allocation_use(reg, use_record->record,
                                common, difference);
                            if (adjustment) {
                                use_record->record = common;
                                reg->float_uses->record = common;
                                clear_register_set(difference);
                            }
                        }
                    }
                    if (difference->links.first) {
                        for (number = next_optimizer_symbol(difference); number != (unsigned)-1;
                             number = next_optimizer_symbol(NULL)) {
                            other = lookup_allocation_register(number);
                            for (found = other->float_uses; found; found = found->next)
                                if (found->record == use_record->record) break;
                            if (!found)
                                for (link = other->float_definitions; link; link = link->next)
                                    if (spill_float_allocation_use(link->record, reg)) changed = 1;
                        }
                    }
                }
            }
            use_record = use_record->next;
        }
        for (number = next_optimizer_symbol(definition->registers); number != (unsigned)-1;
             number = next_optimizer_symbol(NULL)) {
            other = lookup_allocation_register(number);
            for (link = other->float_uses; link; link = link->next)
                if (spill_float_allocation_use(link->record, reg)) changed = 1;
        }
        definition_record = definition_record->next;
    }
    release_optimizer_symbol_set(difference);
    for (link = definitions; link; link = link->next)
        release_optimizer_symbol_set(link->registers);
    for (link = uses; link; link = link->next)
        release_optimizer_symbol_set(link->registers);
    release_allocation_moves(definitions);
    release_allocation_moves(definition_records);
    release_allocation_moves(uses);
    release_allocation_moves(use_records);
    if (!changed) {
        link = reg->float_definitions->next;
        if (link) {
            depth = float_depth_before_record(reg->float_definitions->record);
            for (; link; link = link->next)
                if (depth != float_depth_before_record(link->record)) break;
            if (link && rebalance_float_allocation(function, reg)) goto failed;
        }
        for (link = reg->float_uses; link; link = link->next)
            if (float_allocation_use_conflicts(reg, link->record)) changed = 1;
        if (!changed) return 1;
    }
failed:
    if (adjustment) release_float_allocation_adjustment(adjustment);
    return 0;
}
typedef struct C2AllocationSpillBlock {
    struct C2AllocationSpillBlock *next, *previous;
    C2BasicBlock *block;
    C2RegisterSet *saved_uses, *saved_definitions;
} C2AllocationSpillBlock;
typedef char C2AllocationSpillBlockSize[sizeof(C2AllocationSpillBlock) == 20 ? 1 : -1];
C2AllocationSpillBlock *new_allocation_spill_block(void);
void __fastcall release_allocation_spill_block(C2AllocationSpillBlock *block);
#define FLOAT_SPILL_KIND(value) do { \
    if ((value) != (C2FunctionRegister *)&machine_registers[31] && \
        (value) != (C2FunctionRegister *)&machine_registers[32] && \
        (value) != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(value); \
} while (0)
C2RegisterSet *__fastcall spill_float_allocation_register(C2Function *function,
    C2AllocationRegister *reg, C2RegisterSet *assigned, C2RegisterSet *seen)
{
    C2AllocationMoveLink *link;
    C2AllocationSpillBlock *work = NULL, *tail = NULL, *item;
    C2BasicBlock *block, *last, *neighbor, *split;
    C2FlowEdge *edge, *next_edge;
    C2Expression *record, *position, *scan, *operand, *input, *output, *inserted;
    C2Expression *resume, *stop = NULL;
    C2AllocationRegister *other, *stack[8];
    C2RegisterSet *scratch, *retry, *single;
    unsigned short type = reg->source->type;
    unsigned number = reg->number, line;
    int top, i, selected_live, full = 0, side;

    for (link = reg->float_uses; link; link = link->next) {
        record = link->record;
        if (record->kind == 24) continue;
        while (!(operand = final_register_has_other_use((C2FunctionRegister *)reg, record)))
            record = record->previous;
        operand->location.expression.use_flags &= ~0x10;
        position = record;
        if (record != link->record) {
            position = record->next;
            while (position != link->record) {
                output = position->tail.right;
                if (output && (output->type & 0xf000) == 0x4000 && output->kind == 1) {
                    FLOAT_SPILL_KIND(output->original_register);
                    if (output->original_register->kind == 2) break;
                }
                position = position->next;
            }
        }
        output = position->tail.right;
        if (output && (output->type & 0xf000) == 0x4000) {
            position = position->previous;
            while (position->opcode == 0x63) {
                FLOAT_SPILL_KIND(position->left->original_register);
                if (position->left->original_register->kind != 2) break;
                position = position->previous;
            }
        }
        if (record->opcode == 0x16c) {
            if (position == record) position = position->previous;
            unlink_and_release_input_record((C2Record *)record);
        }
        line = position->location.line;
        if (line) {
            record_source_line = function->source_line_base + line;
            record_line_delta = (unsigned short)line;
        }
        input = new_temporary_register_expression(0x11f, type, reg->source,
            (C2FunctionRegister *)reg);
        output = new_temporary_register_expression(0x11f, type,
            (C2FunctionRegister *)&machine_registers[31],
            (C2FunctionRegister *)&machine_registers[31]);
        inserted = new_binary_expression(0x63, type, output, input,
            insert_input_record, (C2Record *)position);
        scan = inserted->previous;
        while ((scan->type & 0xf000) != 0x4000 && scan->kind != 13) scan = scan->previous;
        inserted->float_depth_before = scan->float_depth_before;
        inserted->float_depth_after = scan->float_depth_after;
        inserted->left->location.expression.use_flags |= 0x10;
    }
    scratch = new_register_set(6);
    retry = new_register_set(6);
    last = reg->last_block;
    for (block = reg->first_block; ; block = block->previous) {
        if (optimizer_set_contains(block->allocation_region_uses, number)) {
            item = new_allocation_spill_block();
            item->block = block;
            item->next = NULL;
            if (tail) tail->next = item;
            else work = item;
            tail = item;
        }
        if (block == last) break;
    }
    while (work) {
        item = work;
        block = item->block;
        work = item->next;
        release_allocation_spill_block(item);
        if (optimizer_set_contains(block->allocation_live_out, number)) {
            for (edge = block->incoming; edge; edge = edge->next_incoming) {
                for (i = 0; i < 8; ++i)
                    if (edge->source->float_allocation->stack[i] == reg) break;
                if (i < 8) break;
            }
            if (!edge) {
                item = new_allocation_spill_block();
                item->next = NULL;
                item->block = block;
                tail->next = item;
                tail = item;
                continue;
            }
            memcpy(stack, edge->source->float_allocation, 32);
            selected_live = 1;
        } else {
            if (!block->incoming) memset(stack, 0, sizeof(stack));
            else memcpy(stack, block->incoming->source->float_allocation, 32);
            selected_live = 0;
        }
        /* The original counts until a NULL in its eight local cells, without
         * a ninth-cell bound. Native stack neighbors are not equivalent if
         * that fully populated incidental-stack domain is ever encountered. */
        for (top = 0; ; ++top) {
#ifdef __linux__
            if (top == 8) { __asm { int 3 } }
#endif
            if (!stack[top]) break;
        }
        --top;
        copy_optimizer_symbol_set_into(scratch, seen);
        subtract_optimizer_symbol_sets(scratch, assigned);
        intersect_optimizer_symbol_sets(scratch, block->allocation_live_out);
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = resume->next) {
            resume = record;
            if (!(record->flags & 1) ||
                ((record->type & 0xf000) != 0x4000 && record->kind != 13)) continue;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = (unsigned short)line;
            }
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1 ||
                    !(operand->location.expression.use_flags & 0x10)) continue;
                FLOAT_SPILL_KIND(operand->original_register);
                other = (C2AllocationRegister *)operand->original_register;
                if (other->kind != 2) continue;
                if (other != reg) {
                    if (!other->pinned_register) remove_optimizer_symbol(scratch, other->number);
                    else stack[top--] = NULL;
                    continue;
                }
                if (stack[top] != reg) {
                    resume = record->previous;
                    position = record->next;
                    i = top - 1;
                    while (stack[i] != reg) {
                        position = position->next;
                        --i;
                    }
                    move_final_label_after(position, record);
                    goto next_float_spill_record;
                }
                stack[top--] = NULL;
                if (!record->float_depth_after) selected_live = 0;
                else {
                    position = record->next;
                    while (position->float_depth_after ||
                           ((position->type & 0xf000) != 0x4000 && position->kind != 13) ||
                           position->opcode == 0x139 || position->opcode == 0x13a)
                        position = position->next;
                    position = position->previous;
                    while ((position->type & 0xf000) != 0x4000 && position->kind != 13)
                        position = position->previous;
                    if (position == record) selected_live = 0;
                    else stop = position;
                }
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                FLOAT_SPILL_KIND(operand->original_register);
                other = (C2AllocationRegister *)operand->original_register;
                if (other->kind != 2) continue;
                if (!other->pinned_register) add_register_to_set(scratch, other->number);
                else {
                    for (i = 0; i <= top; ++i) if (stack[i] == other) break;
                    if (i > top) {
                        stack[i] = other;
                        ++top;
                        if (other == reg) selected_live = 1;
                    }
                }
            }
            if (selected_live) {
                ++record->float_depth_before;
                if (record->float_depth_before == 8) full = 1;
                if (record == stop) {
                    selected_live = 0;
                    stop = NULL;
                }
            }
            if (full && !record->float_depth_after &&
                record->opcode != 0x139 && record->opcode != 0x13a) {
                full = 0;
                if (scratch->links.first) {
                    insert_float_allocation_barrier(record, scratch);
                    merge_optimizer_symbol_sets(retry, scratch);
                    block->flags |= 0x40000;
                }
                for (scan = record->previous; scan->kind != 24; scan = scan->previous) {
                    if (!(scan->flags & 1) || scan->opcode == 0x139 || scan->opcode == 0x13a) continue;
                    for (side = 0; side < 2; ++side) {
                        if (!side && !scan->float_depth_after) continue;
                        for (operand = side ? scan->left : scan->tail.right;
                             operand; operand = operand->next) {
                            if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                            FLOAT_SPILL_KIND(operand->original_register);
                            other = (C2AllocationRegister *)operand->original_register;
                            if (other->kind != 2 || other->pinned_register || other == reg) continue;
                            single = new_register_set(6);
                            add_register_to_set(single, other->number);
                            insert_float_allocation_barrier(scan, single);
                            insert_float_allocation_barrier(scan->previous, single);
                            release_optimizer_symbol_set(single);
                            add_register_to_set(retry, operand->original_register->number);
                            block->flags |= 0x40000;
                        }
                    }
                    if (!scan->float_depth_after) break;
                }
            }
next_float_spill_record: ;
        }
        if (!selected_live) continue;
        for (edge = block->outgoing; edge; edge = next_edge) {
            next_edge = edge->next_outgoing;
            neighbor = edge->destination;
            if (optimizer_set_contains(neighbor->allocation_live_out, number)) continue;
            if (!neighbor->incoming->next_incoming) {
                position = (C2Expression *)neighbor->records;
                if (position->kind == 24) {
                    do {
                        position = position->next;
                        if (position == (C2Expression *)neighbor->last_record) {
                            position = position->previous;
                            break;
                        }
                    } while (position->kind == 24);
                }
                if (position->flags & 1) position = position->previous;
                i = top;
                while (stack[i] != reg) {
                    position = position->next;
                    --i;
                }
            } else {
                split = split_allocation_edge(function, edge, block, neighbor);
                split->allocation_live_out = copy_optimizer_symbol_set(neighbor->allocation_live_out);
                split->allocation_live_in_after = copy_optimizer_symbol_set(neighbor->allocation_live_out);
                split->allocation_region_uses = copy_optimizer_symbol_set(neighbor->allocation_live_out);
                split->allocation_edge_uses = new_register_set(8);
                split->allocation_edge_definitions = new_register_set(8);
                split->allocation_region_input = new_register_set(6);
                split->block_scope = neighbor->block_scope;
                split->region_depth = neighbor->region_depth;
                split->float_allocation = c2_pool_allocate(4, 32);
                memcpy(split->float_allocation, block->float_allocation, 32);
                position = (C2Expression *)split->records;
                if (position->next->kind == 25) position = position->next;
                neighbor = split;
            }
            line = position->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = (unsigned short)line;
            }
            input = new_temporary_register_expression(0x11f, type, reg->source,
                (C2FunctionRegister *)reg);
            output = new_temporary_register_expression(0x11f, type,
                (C2FunctionRegister *)&machine_registers[31],
                (C2FunctionRegister *)&machine_registers[31]);
            inserted = new_binary_expression(0x63, type, output, input,
                insert_input_record, (C2Record *)position);
            inserted->left->location.expression.use_flags |= 0x10;
            add_register_to_set(neighbor->allocation_live_out, reg->number);
            add_register_to_set(neighbor->allocation_region_uses, reg->number);
        }
        memcpy(block->float_allocation, stack, 32);
    }
    release_optimizer_symbol_set(scratch);
    if (!retry->links.first) {
        release_optimizer_symbol_set(retry);
        retry = NULL;
    }
    return retry;
}
#undef FLOAT_SPILL_KIND
C2FunctionRegister *__fastcall new_spill_temporary_register(unsigned short type);
void __fastcall finish_float_allocation_machine(void)
{
    C2FlowRegion *region;
    C2BasicBlock *header, *before, *after;
    C2FlowEdge *edge;
    C2AllocationRegister *reg, *index, *allocated;
    C2AllocationMoveLink *link;
    C2FunctionRegister *temporary_reg;
    C2Expression *first, *last, *record, *scan, *move, *definition;
    C2Expression *operand, *candidate, *temporary, *inserted, *result;
    unsigned count;
    unsigned short type;

    for (region = allocation_region_chain; region; region = region->allocation_next) {
        header = region->header;
        before = header->next;
        edge = before->outgoing;
        if (edge->destination != header || edge->next_outgoing) continue;
        after = header->previous;
        edge = after->incoming;
        if (edge->source != header || edge->next_incoming) continue;
        edge = header->outgoing;
        if (edge->destination != header && edge->destination != after) continue;
        edge = edge->next_outgoing;
        if (!edge || (edge->destination != header && edge->destination != after)) continue;
        last = (C2Expression *)header->last_record;
        for (count = 0; ; ++count) {
#ifdef __linux__
            /* The original reads beyond its eight-cell state if every cell is
             * populated. Native adjacent storage has no equivalent identity. */
            if (count == 8) { __asm { int 3 } }
#endif
            reg = header->float_allocation->stack[count];
            if (!reg) break;
            for (link = reg->float_uses; link; link = link->next) {
                if (allocation_block_for_record(link->record) != header) continue;
                if (link->record->kind == 24) goto next_float_finish_region;
                first = (C2Expression *)header->records;
                for (move = first; move != last; move = move->next) {
                    operand = move->left;
                    if (move->opcode == 0x63 && operand->kind == 1 &&
                        operand->original_register == (C2FunctionRegister *)reg &&
                        (operand->location.expression.use_flags & 0x10)) break;
                }
                for (definition = first; definition != last; definition = definition->next) {
                    operand = definition->tail.right;
                    if ((definition->flags & 1) && operand && operand->kind == 1 &&
                        operand->original_register == (C2FunctionRegister *)reg) break;
                }
                if (definition->opcode == 0x139) goto next_float_finish_region;
                for (scan = move->next; scan != definition; scan = scan->next) {
                    if (!(scan->flags & 1)) continue;
                    if ((scan->kind == 13 && (scan->left->kind != 4 ||
                         !(scan->left->value.symbol->flags & 0x10000))) ||
                        scan->kind == 20 || scan->float_depth_before == 8) break;
                }
                if (scan != definition) goto next_float_finish_region;
                for (scan = move->next; scan != definition; scan = scan->next)
                    if (scan->flags & 1) ++scan->float_depth_before;
                unlink_and_release_input_record((C2Record *)move);
            }
        }
        if (count >= 7) continue;
        first = (C2Expression *)header->records;
        for (record = first; record != last; record = record->next) {
            if (!(record->flags & 1)) continue;
            candidate = record->tail.right;
            if (!candidate || candidate->kind != 6 ||
                (candidate->type & 0xf000) != 0x4000 || candidate->opcode != 0x122) continue;
            index = (C2AllocationRegister *)candidate->aggregate_value->original_register;
            if ((unsigned long)index >= (unsigned long)machine_registers &&
                (unsigned long)index <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                { __asm { int 3 } }
            if (index->kind != 2 ||
                !optimizer_set_contains(header->allocation_live_out, index->number) ||
                !optimizer_set_contains(header->allocation_live_in_after, index->number)) continue;
            for (scan = first; scan != last; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                if ((scan->kind == 13 && (scan->left->kind != 4 ||
                     !(scan->left->value.symbol->flags & 0x10000))) ||
                    scan->kind == 20 || scan->float_depth_before == 8) break;
                for (operand = scan->tail.right; operand; operand = operand->next) {
                    switch (operand->kind) {
                    case 1:
                        if (operand->original_register == (C2FunctionRegister *)index)
                            goto rejected_float_loop_value;
                        break;
                    case 2:
                        if (register_may_alias_index(operand->value.reg, candidate->immediate))
                            goto rejected_float_loop_value;
                        break;
                    case 6:
                        if (operand != candidate &&
                            optimizer_indices_overlap(candidate->immediate, operand->immediate))
                            goto rejected_float_loop_value;
                        break;
                    case 10:
                        if (optimizer_indices_overlap(candidate->immediate, operand->value.optimizer_index))
                            goto rejected_float_loop_value;
                        break;
                    }
                }
                for (operand = scan->left; operand; operand = operand->next) {
                    if (operand->kind == 2) {
                        if (register_may_alias_index(operand->value.reg, candidate->immediate))
                            goto rejected_float_loop_value;
                    } else if (operand->kind == 6) {
                        if (optimizer_indices_overlap(candidate->immediate, operand->immediate) &&
                            (operand->opcode != 0x122 ||
                             operand->aggregate_value->original_register != (C2FunctionRegister *)index ||
                             (operand->type & 0xf000) != 0x4000 ||
                             ((operand->type ^ candidate->type) & 0xfff) ||
                             operand->detail.data != candidate->detail.data ||
                             operand->tail.right != candidate->tail.right))
                            goto rejected_float_loop_value;
                    } else if (operand->kind == 10 &&
                        optimizer_indices_overlap(candidate->immediate, operand->value.optimizer_index))
                        goto rejected_float_loop_value;
                }
            }
rejected_float_loop_value:
            if (scan != last) continue;
            type = candidate->type;
            temporary_reg = new_spill_temporary_register(type);
            allocated = (C2AllocationRegister *)get_allocation_register(temporary_reg);
            allocated->flags &= ~1;
            clear_register_set(allocation_float_registers);
            allocated->pinned_register = (C2FunctionRegister *)&machine_registers[31];
            temporary = new_temporary_register_expression(0x11f, type, temporary_reg,
                (C2FunctionRegister *)allocated);
            record_line_delta = 0;
            inserted = new_assignment_expression(0x60, type, candidate,
                prepend_input_record, before->last_record);
            result = inserted->tail.right;
            result->original_register = (C2FunctionRegister *)&machine_registers[31];
            inserted->float_depth_after = 1;
            inserted->float_depth_before = (unsigned char)count;
            inserted = new_binary_expression(0x63, type, temporary, result,
                insert_input_record, (C2Record *)inserted);
            inserted->float_depth_before = (unsigned char)(count + 1);
            inserted->tail.right->float_stack_index = 1;
            inserted = new_assignment_expression(0x60, type, temporary,
                insert_input_record, after->records);
            result = inserted->tail.right;
            inserted->float_depth_after = 1;
            inserted->float_depth_before = (unsigned char)(count + 2);
            result->original_register = (C2FunctionRegister *)&machine_registers[31];
            inserted = new_binary_expression(0x63, type, candidate, result,
                insert_input_record, (C2Record *)inserted);
            inserted->float_depth_before = (unsigned char)(count + 1);
            operand = new_temporary_register_expression(0x11f, type,
                (C2FunctionRegister *)&machine_registers[31],
                (C2FunctionRegister *)&machine_registers[31]);
            inserted = new_binary_expression(0x63, type, operand, temporary,
                insert_input_record, (C2Record *)inserted);
            inserted->left->location.expression.use_flags |= 0x10;
            inserted->float_depth_before = (unsigned char)count;
            header->float_allocation->stack[count] = allocated;
            before->float_allocation->stack[count] = allocated;
            for (scan = first; scan != last; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                for (operand = scan->left; operand; operand = operand->next) {
                    if (operand->kind == 6 && operand->opcode == 0x122 &&
                        operand->aggregate_value->original_register == (C2FunctionRegister *)index &&
                        operand->detail.data == candidate->detail.data) {
                        attach_input_expression(scan, operand, temporary);
                        break;
                    }
                }
                ++scan->float_depth_before;
            }
            replace_input_result(record, candidate, temporary)->allocation_class_number = 1;
            if (!optimizer_set_contains(after->allocation_live_out, index->number)) {
                add_register_to_set(after->allocation_live_out, index->number);
                add_register_to_set(after->allocation_region_uses, index->number);
                index->last_block = after;
            }
            if (++count >= 7) break;
        }
next_float_finish_region: ;
    }
}
void __fastcall move_record_range_before(C2Expression *destination,
    C2Expression *first, C2Expression *last);
C2Expression *__fastcall new_fixed_register_operand(unsigned number, unsigned short type);
void __fastcall move_float_allocation_store_to_exits(C2FlowRegion *region,
    C2Expression *record, C2FunctionRegister *source, C2RegisterSet *seen, C2Function *function)
{
    C2FlowRegion *chosen = region, *parent;
    C2FlowBlockLink *exit;
    C2FlowEdge *edge, *next_edge;
    C2BasicBlock *from, *target, *destination;
    C2Expression *first, *scan, *operand, *copy, *swap, *left, *right;
    struct C2FloatBlockState *state;
    unsigned index, top;
    unsigned short type;
    /* The original carries the non-NULL source in its search-result register
     * until a real operand scan writes it, including across empty exits. */
    int has_use = source != NULL;

    for (parent = region->parent; parent; parent = parent->parent) {
        if (optimizer_set_contains(parent->allocation_eligible, source->allocation->number)) break;
        chosen = parent;
    }
    for (exit = chosen->exits; exit; exit = exit->next) {
        from = exit->block;
        for (edge = from->outgoing; edge; edge = next_edge) {
            next_edge = edge->next_outgoing;
            target = edge->destination;
            if (target->block_scope == chosen) continue;
            destination = target;
            if (target->incoming->next_incoming) {
                destination = split_allocation_edge(function, edge, from, target);
                destination->allocation_live_out = copy_optimizer_symbol_set(target->allocation_live_out);
                subtract_optimizer_symbol_sets(destination->allocation_live_out, seen);
                destination->allocation_live_in_after = copy_optimizer_symbol_set(target->allocation_live_out);
                subtract_optimizer_symbol_sets(destination->allocation_live_in_after, seen);
                destination->allocation_region_uses = copy_optimizer_symbol_set(target->allocation_live_out);
                subtract_optimizer_symbol_sets(destination->allocation_region_uses, seen);
                destination->allocation_edge_uses = new_register_set(8);
                destination->allocation_edge_definitions = new_register_set(8);
                destination->allocation_region_input = new_register_set(6);
                destination->region_depth = target->region_depth;
                destination->block_scope = target->block_scope;
                destination->float_allocation = c2_pool_allocate(4, 32);
                memcpy(destination->float_allocation, from->float_allocation, 32);
            }
            first = (C2Expression *)destination->records;
            for (scan = first; scan != (C2Expression *)destination->last_record; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                has_use = 0;
                for (operand = scan->left; operand; operand = operand->next) {
                    if (operand->kind == 1 && operand->value.reg == source) {
                        has_use = 1;
                        goto found_float_exit_use;
                    }
                }
                for (operand = scan->tail.right; operand; operand = operand->next) {
                    if (operand->kind == 1 && operand->value.reg == source && scan->opcode != 0x5f)
                        goto found_float_exit_use;
                }
            }
found_float_exit_use:
            if (has_use) {
                if ((scan->opcode == 0x62 || scan->opcode == 0x63) &&
                    scan->tail.right->kind == 2 && scan->tail.right->value.reg == source) continue;
                if (scan->opcode == 0x63) {
                    replace_input_result(scan, scan->tail.right, record->tail.right);
                    continue;
                }
            }
            state = from->float_allocation;
            for (index = 0; ; ++index) {
                /* This consumer owns exactly the eight initialized stack
                 * cells; absent source identities or the sentinel's unmodeled
                 * leading DWORD must stop before any unknown payload read. */
                if (index >= 8 || state->stack[index] ==
                    (C2AllocationRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                    { __asm { int 3 } }
                if (state->stack[index]->source == source) break;
            }
            top = index;
            if (index != 7) {
                top = index + 1;
                for (;;) {
                    if (top >= 8) { __asm { int 3 } }
                    if (!state->stack[top]) break;
                    ++top;
                }
                --top;
            }
            while (!(first->flags & 1) && first->kind != 23 &&
                   first != (C2Expression *)destination->last_record) first = first->next;
            copy = copy_input_expression(record);
            prepend_input_record((C2Record *)first, (C2Record *)copy);
            if (top != index) {
                type = copy->type;
                left = new_temporary_register_expression(0x11f, type, state->stack[top]->source,
                    (C2FunctionRegister *)&machine_registers[31 + top - index]);
                right = new_temporary_register_expression(0x11f, type, source,
                    (C2FunctionRegister *)&machine_registers[31]);
                swap = new_binary_expression(0x5f, type, right, left,
                    prepend_input_record, (C2Record *)copy);
                operand = copy_input_expression(swap);
                insert_input_record((C2Record *)copy, (C2Record *)operand);
            }
        }
    }
    unlink_and_release_input_record((C2Record *)record);
}
#define FLOAT_REWRITE_KIND(value) do { \
    if ((value) != (C2FunctionRegister *)&machine_registers[31] && \
        (value) != (C2FunctionRegister *)&machine_registers[32] && \
        (value) != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(value); \
} while (0)
/* These expressions construct only identities within the existing75-row owner,
 * including its proved one-past sentinel; they read no new row payload. */
#define FLOAT_REWRITE_PHYSICAL(number) do { \
    physical = (unsigned)(number); \
    if (physical > C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } } \
} while (0)
void __fastcall rewrite_float_allocation_registers(C2Function *function,
    C2RegisterSet *seen, C2RegisterSet *assigned)
{
    C2BasicBlock *block;
    C2FlowRegion *region;
    C2AllocationRegister *reg;
    C2FunctionRegister *stack[8], *value, *saved;
    C2FunctionRegister *sentinel = (C2FunctionRegister *)(machine_registers + C2_MACHINE_REGISTER_COUNT);
    C2FunctionRegister *st0 = (C2FunctionRegister *)&machine_registers[31];
    C2Expression *record, *next, *operand, *output, *scan, *position, *definition;
    C2Expression *match, *created, *left, *right;
    unsigned number, line, physical;
    int top, position_index, effect, i, anonymous = 0, displaced = 0;

    for (block = function->flow->last; block; block = block->previous) {
        if (!block->incoming) memset(stack, 0, sizeof(stack));
        else memcpy(stack, block->incoming->source->float_allocation, 32);
        for (top = 0; ; ++top) {
#ifdef __linux__
            /* Original unbounded prefix counting may read adjacent stack if
             * all eight cells are occupied; native incidental bytes differ. */
            if (top == 8) { __asm { int 3 } }
#endif
            if (!stack[top]) break;
        }
        --top;
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            record->float_depth_before = 0;
            if (!(record->flags & 1) ||
                ((record->type & 0xf000) != 0x4000 && record->kind != 13)) continue;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = (unsigned short)line;
            }
            if (record->float_depth_after == 0x13) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->type & 0xf000) != 0x4000 || operand->kind != 1) continue;
                FLOAT_REWRITE_KIND(operand->original_register);
                value = operand->original_register;
                if (value->kind != 2) continue;
                if (record->opcode == 0x63) {
                    if (stack[top] == value) {
                        operand->original_register = st0;
                        continue;
                    }
                    for (position_index = 0; stack[position_index] != value; ++position_index) {}
                    for (i = position_index; i < top; ++i) stack[i] = stack[i + 1];
                    stack[top--] = NULL;
                    ++displaced;
                    goto next_float_rewrite_record;
                }
                if (operand->location.expression.use_flags & 0x10) continue;
                for (position_index = 0; stack[position_index] != value; ++position_index) {}
                if (record->opcode == 0x60 && position_index == top) {
                    scan = record->next;
                    if (scan->flags & 1) {
                        while (scan->float_depth_after || (scan->type & 0xf000) != 0x4000 ||
                               scan->opcode == 0x139 || scan->opcode == 0x13a) {
                            if (final_register_has_other_use(value, scan)) break;
                            scan = scan->next;
                            if (!(scan->flags & 1)) break;
                        }
                    }
                    if (scan->opcode == 0x63 && scan->left->original_register == value) {
                        position = record->next;
                        if (position == scan) position = position->next;
                        for (match = position->left; match; match = match->next)
                            if (match->kind == 1 &&
                                match->value.reg->canonical == record->tail.right->value.reg->canonical) break;
                        if (match) {
                            attach_input_expression(position, match, record->left)->original_register = st0;
                            unlink_and_release_input_record((C2Record *)record);
                            if (scan == next) next = scan->next;
                            unlink_and_release_input_record((C2Record *)scan);
                            ++anonymous;
                            stack[top] = sentinel;
                            goto next_float_rewrite_record;
                        }
                    }
                }
                FLOAT_REWRITE_PHYSICAL(31U + (unsigned)displaced + (unsigned)(top - position_index));
                operand->original_register = (C2FunctionRegister *)&machine_registers[physical];
            }
            effect = final_float_stack_effect(record);
            if (effect < 0) {
                if (stack[top] == sentinel) --anonymous;
                stack[top--] = NULL;
                if (effect == -2) {
                    if (stack[top] == sentinel) --anonymous;
                    stack[top--] = NULL;
                }
            }
            output = record->tail.right;
            if (output && output->kind == 1 && (output->type & 0xf000) == 0x4000) {
                FLOAT_REWRITE_KIND(output->original_register);
                value = output->original_register;
            } else value = NULL;
            if (!value || value->kind != 2) {
                if (effect > 0) {
                    stack[++top] = sentinel;
                    ++anonymous;
                } else if (record->opcode == 0x62 && output->kind == 1 &&
                           output->original_register == st0 && output->value.reg == st0) {
                    stack[top--] = NULL;
                    --anonymous;
                }
                goto reorder_float_rewrite_tail;
            }
            for (position_index = 0; position_index <= top; ++position_index)
                if (stack[position_index] == value) break;
            if (position_index > top) {
                stack[++top] = value;
                output->original_register = st0;
                record->opcode = record->opcode == 0x63 ? 0x62 : 0x60;
            } else {
                FLOAT_REWRITE_PHYSICAL(32U + (unsigned)(top - position_index));
                output->original_register = (C2FunctionRegister *)&machine_registers[physical];
            }
            if (!output->allocation_class_number && record->opcode != 0x60) {
                for (scan = next; scan->opcode != 0x13a; scan = scan->next) {}
                operand = scan->left;
                scan->float_depth_before = 0;
                scan->opcode = 0x62;
                operand->original_register = st0;
                region = block->block_scope;
                if (region) {
                    value = operand->value.reg;
                    if (!optimizer_set_contains(region->allocation_eligible, value->allocation->number) &&
                        (c2_favor_speed || !region->exits->next)) {
                        if (scan == next) next = scan->next;
                        move_float_allocation_store_to_exits(region, scan, value, seen, function);
                        record->tail.right->allocation_class_number = 1;
                        goto reorder_float_rewrite;
                    }
                }
                unlink_input_record((C2Record *)scan);
                prepend_input_record((C2Record *)record, (C2Record *)scan);
                next = record->next;
            }
reorder_float_rewrite:
            if (!displaced) {
                if (anonymous < 1 || record->opcode == 0x63) goto reorder_float_rewrite_tail;
                definition = record->left->value.reg->value;
                for (scan = record->previous; scan != definition &&
                     ((scan->type & 0xf000) != 0x4000 || scan->opcode == 0x138);
                     scan = scan->previous) {}
                if (scan->opcode == 0x5f &&
                    (int)(scan->left->original_register->number - 31) == anonymous) {
                    unlink_and_release_input_record((C2Record *)scan);
                    created = record;
                    for (i = anonymous - 1; i > 0; --i) {
                        left = new_fixed_register_operand((unsigned)i + 31, 0x4008);
                        right = new_fixed_register_operand(31, 0x4008);
                        created = new_binary_expression(0x5f, 0x4008, right, left,
                            insert_input_record, (C2Record *)created);
                    }
                    replace_input_result(definition, definition->tail.right, record->tail.right);
                    if (!record->tail.right->allocation_class_number) {
                        position = record->previous;
                        unlink_input_record((C2Record *)position);
                        insert_input_record((C2Record *)definition, (C2Record *)position);
                    }
                } else {
                    created = record;
                    for (i = anonymous; i > 0; --i) {
                        left = new_fixed_register_operand((unsigned)i + 31, 0x4008);
                        right = new_fixed_register_operand(31, 0x4008);
                        created = new_binary_expression(0x5f, 0x4008, right, left,
                            insert_input_record, (C2Record *)created);
                    }
                    value = record->left->value.reg;
                    definition = value->value;
                    for (match = definition->tail.right; match; match = match->next)
                        if (match->kind == 1 && match->value.reg->canonical == value->canonical) break;
                    replace_input_result(definition, match, record->tail.right);
                }
                unlink_and_release_input_record((C2Record *)record);
                saved = stack[top];
                for (i = 0; i < anonymous; ++i) stack[top - i] = stack[top - i - 1];
                stack[top - anonymous] = saved;
                goto next_float_rewrite_record;
            }
            if (anonymous < 1) goto reorder_float_rewrite_tail;
            position = record->previous;
            if (!record->tail.right->allocation_class_number) position = position->previous;
            scan = position->previous;
            position->float_depth_after = 0x13;
            for (i = displaced - 1; i > 0; --i) {
                scan->float_depth_after = 0x13;
                scan = scan->previous;
            }
            definition = scan->next;
            for (scan = next; ; scan = scan->next) {
                output = scan->tail.right;
                if (output && (output->type & 0xf000) == 0x4000 && output->kind == 1) {
                    FLOAT_REWRITE_KIND(output->original_register);
                    if (output->original_register->kind == 2) break;
                }
                if (!scan->float_depth_after && scan->opcode != 0x139 && scan->opcode != 0x13a) break;
            }
            move_record_range_before(scan, definition, position);
            if (record->opcode == 0x63) {
                FLOAT_REWRITE_PHYSICAL(record->tail.right->original_register->number + (unsigned)displaced);
                record->tail.right->original_register = (C2FunctionRegister *)&machine_registers[physical];
            } else {
                record->opcode = 0x63;
                FLOAT_REWRITE_PHYSICAL(31U + (unsigned)displaced + (unsigned)anonymous);
                --displaced;
                record->tail.right->original_register = (C2FunctionRegister *)&machine_registers[physical];
                unlink_and_release_input_record((C2Record *)position);
                saved = stack[top];
                for (i = 0; i < anonymous; ++i) stack[top - i] = stack[top - i - 1];
                stack[top - anonymous] = saved;
            }
            goto next_float_rewrite_record;
reorder_float_rewrite_tail:
            if (displaced && !anonymous) {
                output = record->tail.right;
                if ((record->opcode == 0x62 || record->opcode == 0x63) && output->kind == 1) {
                    FLOAT_REWRITE_PHYSICAL(output->original_register->number + (unsigned)displaced);
                    output->original_register = (C2FunctionRegister *)&machine_registers[physical];
                    position = record->previous;
                    if (position->opcode == 0x62) position = position->previous;
                    if (record->opcode == 0x62) {
                        unlink_and_release_input_record((C2Record *)position);
                        record->opcode = 0x63;
                        --displaced;
                    }
                }
                position = record->previous;
                if (position->opcode == 0x62 && position->tail.right->kind != 1)
                    position = position->previous;
                for (i = displaced; i; --i) {
                    position->left->original_register = st0;
                    scan = position->previous;
                    move_final_label_after(record, position);
                    --displaced;
                    position = scan;
                }
            }
            output = record->tail.right;
            if (record->opcode == 0x62 && output->kind == 1 && output->original_register == st0) {
                value = record->left->value.reg;
                if ((unsigned long)value >= (unsigned long)machine_registers &&
                    (unsigned long)value <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT))
                    { __asm { int 3 } }
                if (value->kind == 3 && (definition = value->value) != NULL) {
                    for (match = definition->tail.right; match; match = match->next)
                        if (match->kind == 1 && match->value.reg->canonical == value->canonical) break;
                    replace_input_result(definition, match, output);
                }
                unlink_and_release_input_record((C2Record *)record);
            }
next_float_rewrite_record: ;
        }
        subtract_optimizer_symbol_sets(block->allocation_live_out, seen);
        subtract_optimizer_symbol_sets(block->allocation_live_in_after, seen);
        subtract_optimizer_symbol_sets(block->allocation_region_uses, seen);
    }
    for (number = next_optimizer_symbol(assigned); number != (unsigned)-1;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_allocation_register(number);
        release_allocation_moves(reg->float_definitions);
        release_allocation_moves(reg->float_uses);
        release_allocation_register(reg, 0);
    }
}
#undef FLOAT_REWRITE_KIND
#undef FLOAT_REWRITE_PHYSICAL
void __fastcall allocate_floating_function(C2Function *function)
{
    C2RegisterSet *assigned = new_register_set(6);
    C2RegisterSet *seen = copy_optimizer_symbol_set(allocation_float_registers);
    C2RegisterSet *retry = prepare_float_allocation(function, allocation_float_registers);
    C2AllocationRegister *order, *reg, *next, *restart;

    initialize_float_allocation_regions(function, allocation_float_registers);
    order = order_float_allocation_registers(allocation_float_registers, NULL);
    reg = select_float_allocation_register(function, order, retry);
    while (reg) {
        if (retry) {
            restart = update_float_allocation_retry(reg, retry);
            apply_allocation_spills(function, retry, 1);
            merge_optimizer_symbol_sets(seen, allocation_float_registers);
            initialize_float_allocation_regions(function, allocation_float_registers);
            order = order_float_allocation_registers(allocation_float_registers, restart);
            reg = select_float_allocation_register(function, order, NULL);
            if (!reg) break;
        }
        next = reg->float_next;
        if (next) next->float_previous = NULL;
        if (!try_allocate_float_register(function, reg, assigned)) {
            reg->float_next = next;
            if (next) next->float_previous = reg;
            retry = new_register_set(6);
            add_register_to_set(retry, reg->number);
        } else {
            reg->pinned_register = (C2FunctionRegister *)&machine_registers[31];
            add_register_to_set(assigned, reg->number);
            retry = spill_float_allocation_register(function, reg, assigned, seen);
            reg = next;
        }
    }
    finish_float_allocation_machine();
    rewrite_float_allocation_registers(function, seen, assigned);
    release_optimizer_symbol_set(assigned);
    release_optimizer_symbol_set(seen);
    finish_register_moves(function);
}
unsigned __fastcall allocation_integer_limit(C2Function *function)
{
    if (function->flags & 0x10) {
        allocation_frame_register_available = 1;
        return 7;
    }
    allocation_frame_register_available = 0;
    return 6;
}
/* These are real callees, with their own pending entry traps. */
void __fastcall disable_frame_register(C2Function *function) { __asm { int 3 } }
C2Expression *__fastcall find_final_register_definition(C2Expression *record, C2FunctionRegister *reg);
int __fastcall spill_result_is_redundant(C2Expression *record, C2Expression *result)
{
    C2Expression *first = record->tail.right;
    if (result == first) return 0;
    if ((record->opcode == 0xd9 || record->opcode == 9) && result == first->next) return 0;
    return 1;
}
extern C2RegisterSet *allocation_byte_exclusions;

void __fastcall finish_spill_moves(C2AllocationMoveLink *moves)
{
    C2AllocationMoveLink *link, *other_move;
    C2Expression *move, *input, *output, *reference, *original_reference;
    C2Expression *record, *scan, *operand, *next, *previous, *candidate, *match;
    C2FunctionRegister *temporary, *physical, *value, *width_register;
    unsigned small_displacement, input_number, output_number;
    unsigned char flags;
    int reverse, transferred, wrote_result, match_known = 0;

    /* The old scan register is carried between move candidates. An initial
     * path with no real output-list write leaves it unwritten; preserve the
     * carried value and deny that unknown first use rather than seed NULL. */
    for (link = moves; link; link = link->next) {
        move = link->record;
        if (!move) continue;
        input = move->left;
        output = move->tail.right;
        ALLOCATION_KNOWN_KIND(output->original_register);
        if (output->original_register->kind == 3) {
            temporary = output->original_register;
            reference = input;
            value = input->value.reg;
            reverse = 0;
        } else {
            ALLOCATION_KNOWN_KIND(input->original_register);
            if (input->original_register->kind != 3) {
                link->record = NULL;
                continue;
            }
            temporary = input->original_register;
            reference = output;
            value = output->value.reg;
            reverse = 1;
        }
        original_reference = reference;
        physical = reference->original_register;
        if ((temporary->view_flags & 0x80) && !(physical->view_flags & 0x80)) continue;
        if ((move->type & 0xfff) == 1) {
            /* The byte-layout decisions consume dynamic register slice data,
             * not reserved payload in a partly modeled machine row. */
            width_register = reverse ? input->value.reg : output->value.reg;
            if (((unsigned long)value >= (unsigned long)machine_registers &&
                 (unsigned long)value < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) ||
                ((unsigned long)width_register >= (unsigned long)machine_registers &&
                 (unsigned long)width_register < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)))
                { __asm { int 3 } }
            small_displacement = width_register->displacement >= 4 ? 0 :
                (unsigned)width_register->displacement;
            if (value->allocation_root && value->displacement != value->allocation_root->displacement) {
                if (small_displacement != 8) continue;
            } else if (small_displacement) continue;
            if (optimizer_set_contains(allocation_byte_exclusions, temporary->number) &&
                !small_displacement) continue;
        } else if (optimizer_set_contains(allocation_byte_exclusions, temporary->number)) continue;

        scan = reverse ? move : find_final_register_definition(move, output->value.reg);
        for (scan = scan->previous; scan; scan = scan->previous) {
            if (!(scan->flags & 1)) continue;
            match = scan->tail.right;
            match_known = 1;
            for (; match; match = match->next) {
                if (match->kind == 1 && match->original_register == temporary) goto found_spill_use;
                if (input_operands_may_alias(match, reference)) {
                    match = NULL;
                    goto found_spill_use;
                }
            }
        }
found_spill_use:
        if (!match_known) { __asm { int 3 } }
        if (!match) continue;
        if (temporary->canonical->variant_next) bind_input_temporary(temporary, NULL);
        else temporary->value = NULL;
        temporary->allocation_definition_record = NULL;
        temporary->allocation_definition_operand = NULL;
        transferred = 0;
        wrote_result = 0;
        record = move;
        for (;;) {
            do {
                record = reverse ? record->previous : record->next;
            } while (!(record->flags & 1));
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1 || operand->original_register != temporary) continue;
                operand->location.expression.properties &= 0xf0;
                operand->original_register = physical;
                if (transferred || reverse) operand->value.reg = temporary;
                else {
                    transferred = 1;
                    operand->value.reg = value;
                }
                if (!reverse && (operand->location.expression.use_flags & 0x10)) {
                    if (!(original_reference->location.expression.use_flags & 0x10)) {
                        flags = operand->location.expression.use_flags & ~0x10;
                        operand->location.expression.use_flags = flags;
                        for (scan = record->previous; scan != link->record; scan = scan->previous) {
                            if (!(scan->flags & 1)) continue;
                            candidate = NULL;
                            for (previous = scan->left; previous; previous = previous->next)
                                if (previous->kind == 1 && previous->original_register == physical)
                                    candidate = previous;
                            if (!candidate) continue;
                            if (candidate->location.expression.use_flags & 0x10) {
                                for (other_move = link->next; other_move; other_move = other_move->next) {
                                    if (other_move->record != scan) continue;
                                    previous = scan->tail.right;
                                    if (!((1U << (previous->kind & 31)) & 6) || !previous->value.reg->value)
                                        continue;
                                    if (!(previous->original_register->allocation_definition_operand->
                                          location.expression.use_flags & 0x10)) other_move->record = NULL;
                                    break;
                                }
                                candidate->location.expression.use_flags &= ~0x10;
                                operand->location.expression.use_flags = flags | 0x10;
                            }
                            break;
                        }
                    }
                    goto finish_spill_transfer;
                }
                operand->location.expression.use_flags |= 0x10;
            }
            output_number = 1;
            match = record->tail.right;
            match_known = 1;
            while (match) {
                next = match->next;
                if (match->kind == 1 && match->original_register == temporary) {
                    match->location.expression.properties &= 0xf0;
                    if (spill_result_is_redundant(record, match)) {
                        remove_input_result(record, match);
                        if (!reverse && !wrote_result) transferred = 0;
                        for (operand = record->left; operand; operand = operand->next)
                            if (operand->kind == 1 && operand->original_register == physical) {
                                operand->location.expression.use_flags &= ~0x10;
                                break;
                            }
                    } else {
                        match->original_register = physical;
                        wrote_result = 1;
                        if (transferred || !reverse) {
                            match->order_key = 1;
                            match->value.reg = temporary;
                        } else {
                            match->value.reg = value;
                            transferred = 1;
                            match->order_key = link->record->tail.right->order_key;
                            if (reverse && !match->order_key && link->record->next->opcode == 0x13a &&
                                record->next != link->record) {
                                previous = link->record->next;
                                unlink_input_record((C2Record *)previous);
                                insert_input_record((C2Record *)record, (C2Record *)previous);
                            }
                        }
                        if (reverse && (match->location.expression.use_flags & 0x10))
                            goto remove_spill_move;
                        input_number = 1;
                        for (operand = record->left; operand; operand = operand->next, ++input_number)
                            if (operand->kind == 1 && operand->original_register == physical) {
                                match->location.expression.properties =
                                    (match->location.expression.properties & 0xf0) | (input_number & 15);
                                operand->location.expression.properties =
                                    (operand->location.expression.properties & 0xf0) | (output_number & 15);
                                break;
                            }
                    }
                }
                ++output_number;
                match = next;
            }
        }
finish_spill_transfer:
        previous = NULL;
        match = record->left;
        match_known = 1;
        for (; match; match = match->next) {
            if (match->kind != 1 || match->original_register != physical) continue;
            if (previous && (previous->location.expression.use_flags & 0x10)) {
                previous->location.expression.use_flags &= ~0x10;
                match->location.expression.use_flags |= 0x10;
            }
            previous = match;
        }
remove_spill_move:
        unlink_and_release_input_record((C2Record *)link->record);
    }
}
int __fastcall allocation_input_is_repeated(C2Expression *operand)
{
    C2FunctionRegister *original = operand->original_register;

    for (operand = operand->next; operand; operand = operand->next)
        if (operand->kind == 1 && operand->original_register == original) return 1;
    return 0;
}
C2Expression *__fastcall find_final_register_output(C2Expression *, C2FunctionRegister *);
void __fastcall adjust_allocation_input(C2Expression *record, C2AllocationRegister *reg)
{
    C2Expression *previous = record->previous, *output, *operand, *address, *first;
    long amount;
    unsigned scale;

    if (!(previous->flags & 1) || !(output = previous->tail.right) ||
        output->kind != 1 || output->original_register != (C2FunctionRegister *)reg) return;
    switch (previous->opcode) {
    case 0x1e: amount = -1; break;
    case 0x21: amount = 1; break;
    case 0x2d:
        if (previous->left->kind != 7) return;
        amount = (long)previous->left->integer.low;
        break;
    default: return;
    }
    output = find_final_register_output(previous,
        (C2FunctionRegister *)&machine_registers[66]);
    if (output && ((1U << (output->kind & 31)) & 6) &&
        output->value.reg->kind == 3) return;
    for (operand = record->left; operand; operand = operand->next)
        if (operand->kind == 1 && operand->original_register == (C2FunctionRegister *)reg &&
            !(operand->location.expression.properties & 0x20)) return;
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1 || operand->original_register != (C2FunctionRegister *)reg) continue;
        address = find_aggregate_operand_owner(record, operand, &first);
        if (address->aggregate_second == operand &&
            (scale = address->location.expression.properties & 0xf) != 0)
            address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement +
                ((1U << scale) - 1) * (unsigned)amount);
        address->detail.aggregate_displacement = (long)((unsigned)address->detail.aggregate_displacement + (unsigned)amount);
    }
    unlink_and_release_input_record((C2Record *)previous);
}
/* Canonical identity has its own proved domain: row18 is a known view of
 * row2, even though row18's kind byte remains unmodeled. */
#define ALLOCATION_KNOWN_CANONICAL(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)
int __fastcall allocation_copy_uses_fixed_register(C2Expression *record,
    C2FunctionRegister *original, C2FunctionRegister *canonical)
{
    C2Expression *first = record->left;
    C2Expression *definition = first->value.reg->value;
    C2Expression *operand, *scan;
    long displacement;

    if ((definition->type & 0xfff) == 1) {
        ALLOCATION_KNOWN_NUMBER(canonical);
        if (canonical->number > 4) return 0;
    }
    operand = final_register_has_other_use(original, definition);
    while (operand) {
        if ((operand->type & 0xfff) == 1) {
            ALLOCATION_KNOWN_NUMBER(canonical);
            if (canonical->number > 4) return 0;
        }
        if ((operand->type ^ definition->type) & 0xfff) return 0;
        definition = operand->value.reg->value;
        if ((definition->type & 0xfff) == 1) {
            ALLOCATION_KNOWN_NUMBER(canonical);
            if (canonical->number > 4) return 0;
        }
        operand = final_register_has_other_use(original, definition);
    }
    for (scan = definition->next; scan != record; scan = scan->next) {
        if (!(scan->flags & 1)) continue;
        for (operand = scan->left; operand; operand = operand->next) {
            if (operand->kind == 1) {
                ALLOCATION_KNOWN_CANONICAL(operand->original_register);
                if (operand->original_register->canonical == canonical) return 0;
            }
        }
        for (operand = scan->tail.right; operand; operand = operand->next) {
            if (operand->kind == 1) {
                ALLOCATION_KNOWN_CANONICAL(operand->original_register);
                if (operand->original_register->canonical == canonical) return 0;
            } else if (operand->kind == 9) {
                ALLOCATION_KNOWN_NUMBER(canonical);
                if (optimizer_set_contains(operand->value.registers, canonical->number)) return 0;
            }
        }
    }
    for (operand = first; operand;
         operand = final_register_has_other_use(original, definition)) {
        displacement = operand->value.reg->displacement;
        if (displacement >= 4) displacement = 0;
        operand->original_register = new_bitfield_register(canonical,
            (operand->type & 0xfff) << 3, (long)((unsigned)displacement << 3));
        definition = operand->value.reg->value;
        operand = find_final_register_use(original, definition);
        displacement = operand->value.reg->displacement;
        if (displacement >= 4) displacement = 0;
        operand->original_register = new_bitfield_register(canonical,
            (operand->type & 0xfff) << 3, (long)((unsigned)displacement << 3));
    }
    return 1;
}
C2FunctionRegister *__fastcall spill_allocation_pressure(C2Function *function, int which,
    C2Expression *record) { __asm { int 3 } return NULL; }
/* The spill choice is the actual register-set pseudo record produced below. */
typedef C2Record C2AllocationSpillChoice;
C2AllocationSpillChoice *__fastcall select_allocation_spill_choice(C2Expression *record,
    C2RegisterSet *live, C2RegisterSet *first, C2RegisterSet *second)
{
    C2RegisterSet *selected = live ? copy_optimizer_symbol_set(live) : new_register_set(6);
    C2Expression *operand;
    C2FunctionRegister *original;
    C2Record *choice;
    int side;

    if (live) {
        for (side = 0; side < 2; ++side)
            for (operand = side ? record->left : record->tail.right;
                 operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                original = operand->original_register;
                if (original != (C2FunctionRegister *)&machine_registers[7] &&
                    original != (C2FunctionRegister *)&machine_registers[8])
                    ALLOCATION_KNOWN_KIND(original);
                if (original->kind == 2) remove_optimizer_symbol(selected, original->number);
            }
    }
    if (first) subtract_optimizer_symbol_sets(selected, first);
    if (second) merge_optimizer_symbol_sets(selected, second);
    if (!selected->links.first) {
        release_optimizer_symbol_set(selected);
        return NULL;
    }
    choice = new_global_register_set_record(selected);
    insert_input_record((C2Record *)record, choice);
    return choice;
}
int __fastcall spill_frame_candidates(C2RegisterSet *retained, int pressure, int *peak)
{ __asm { int 3 } return 0; }
int __fastcall final_flags_need_register(C2Expression *record);
C2RegisterSet *allocation_byte_exclusions; /* Original493204; real entry writer. */

/* Only the encountered DWORD type-class rows1/2/4/8 of494350 are represented.
 * Unknown rows are incomplete semantics in both builds, not host differences. */
#define ALLOCATION_TYPE_CLASS(type) (((type) >> 12) == 8 ? -1 : (((type) >> 12) == 4 ? 1 : 0))
#define ALLOCATION_KNOWN_TYPE_CLASS(type) \
    do { if (((type) >> 12) != 1 && ((type) >> 12) != 2 && \
        ((type) >> 12) != 4 && ((type) >> 12) != 8) { __asm { int 3 } } } while (0)

/* This consumer additionally reaches the already modeled fixed address
 * rows7/8. Their kind1 bytes grant no unknown payload or other-row defaults. */
#define SPILL_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9]) ALLOCATION_KNOWN_KIND(reg); } while (0)
C2RegisterSet *__fastcall select_allocation_spills(C2Function *function, int which,
    C2RegisterSet *registers, unsigned limit)
{
    C2AllocationMoveLink *moves = NULL, *link;
    C2RegisterSet *spills, *retained, *live;
    C2BasicBlock *block;
    C2Expression *record, *operand, *next, *inserted, *use, *output;
    C2Expression *definition, *defined_operand, *removed;
    C2FunctionRegister *original;
    C2AllocationRegister *assigned;
    C2AllocationSpillChoice *choice;
    unsigned number, line, opcode;
    int track_frame, retained_count, frame_peak = 0, frame_traffic = 0;
    int frame_weighted_cost = 0, fixed_live = 0, memory_live = 0;
    int out_refs, in_refs, before_inputs, pressure;

    track_frame = allocation_frame_register_available &&
        (!c2_favor_speed || frame_reference_cost);
    allocation_byte_exclusions = new_register_set(6);
    spills = new_register_set(6);
    if (track_frame) retained = new_register_set(6);
    live = new_register_set(6);
    for (block = function->flow->first; block; block = block->next) {
        copy_optimizer_symbol_set_into(live, block->allocation_live_in_after);
        if (track_frame) {
            intersect_optimizer_symbol_sets(retained, live);
            retained_count = count_optimizer_symbols(retained);
        }
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
            record->control_flags = 0;
            out_refs = in_refs = 0;
            for (operand = record->tail.right; operand; operand = next) {
                next = operand->next;
                if (operand->kind == 1) {
                    ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                    if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                    original = operand->original_register;
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                        SPILL_KNOWN_KIND(original);
                    switch (original->kind) {
                    case 1:
                        if (original->canonical != (C2FunctionRegister *)&machine_registers[5])
                            ALLOCATION_KNOWN_NUMBER(original->canonical);
                        number = original->canonical->number;
                        if (optimizer_set_contains(allocation_class_registers[which], number)) {
                            add_register_to_set(block->allocation_region_input, number);
                            SPILL_KNOWN_KIND(operand->value.reg);
                            if (operand->value.reg->kind == 1) ++out_refs;
                            else if (!final_register_has_other_use(original, record)) {
                                --fixed_live;
                                ++out_refs;
                                operand->location.expression.use_flags |= 0x10;
                            } else operand->location.expression.use_flags &= ~0x10;
                        }
                        break;
                    case 2:
                        assigned = (C2AllocationRegister *)original;
                        number = assigned->number;
                        if (!optimizer_set_contains(live, number)) {
                            if (!(operand->location.expression.properties & 0xf)) {
                                remove_input_result(record, operand);
                                continue;
                            }
                            inserted = new_input_expression(0x16c, operand->type);
                            copy_input_operand(inserted, operand);
                            inserted->left->location.expression.properties &= 0xf0;
                            inserted->left->location.expression.use_flags |= 0x10;
                            inserted->flags |= 0x10;
                            insert_input_record((C2Record *)record, (C2Record *)inserted);
                            operand->allocation_class_number = 0;
                        }
                        SPILL_KNOWN_KIND(operand->value.reg);
                        if (record->opcode == 0x139 && operand->value.reg->kind == 13 &&
                            optimizer_set_contains(block->allocation_live_out, number)) {
                            removed = record;
                            record = record->next;
                            unlink_and_release_input_record((C2Record *)removed);
                            goto previous_spill_record;
                        }
                        remove_optimizer_symbol(live, number);
                        if (track_frame && optimizer_set_contains(retained, number)) {
                            --retained_count;
                            remove_optimizer_symbol(retained, number);
                        }
                        if (record->opcode != 0x139) {
                            ++out_refs;
                            operand->location.expression.use_flags &= ~0x80;
                            if (record->opcode == 1 && record->left->kind == 1) {
                                SPILL_KNOWN_KIND(record->left->original_register);
                                if (record->left->original_register->kind == 3 &&
                                    !record->left->next && !record->tail.right->next &&
                                    !optimizer_set_contains(block->allocation_live_out, number)) {
                                    link = new_allocation_move();
                                    link->record = record;
                                    link->next = moves;
                                    moves = link;
                                }
                            }
                        }
                        add_register_to_set(block->allocation_region_uses, number);
                        if ((operand->type & 0xfff) == 1) assigned->flags |= 0x80;
                        break;
                    case 3:
                        operand->location.expression.use_flags &= ~0x80;
                        if (!final_register_has_other_use(original, record)) {
                            --memory_live;
                            ++out_refs;
                            operand->location.expression.use_flags |= 0x10;
                        } else operand->location.expression.use_flags &= ~0x10;
                        if ((operand->type & 0xfff) == 1) {
                            original->view_flags |= 0x80;
                            if (operand->value.reg->displacement)
                                add_register_to_set(allocation_byte_exclusions, original->number);
                        }
                        break;
                    default:
                        c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0x315);
                        break;
                    }
                } else if (operand->kind == 9) {
                    for (number = next_optimizer_symbol(operand->value.registers); number != ~0U;
                         number = next_optimizer_symbol(NULL)) {
                        if (optimizer_set_contains(allocation_class_registers[which], number))
                            ++out_refs;
                    }
                    merge_optimizer_symbol_sets(block->allocation_region_input, operand->value.registers);
                } else if (operand->opcode == 0x120 || operand->opcode == 0x124) ++frame_traffic;
            }
            before_inputs = fixed_live + memory_live;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    operand->allocation_class_number = 0;
                    operand->location.expression.properties &= ~0x80;
                    ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                    if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                    original = operand->original_register;
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                        SPILL_KNOWN_KIND(original);
                    switch (original->kind) {
                    case 1:
                        if (original->canonical != (C2FunctionRegister *)&machine_registers[5])
                            ALLOCATION_KNOWN_NUMBER(original->canonical);
                        number = original->canonical->number;
                        if (optimizer_set_contains(allocation_class_registers[which], number)) {
                            SPILL_KNOWN_KIND(operand->value.reg);
                            if (operand->value.reg->kind == 1)
                                add_register_to_set(block->allocation_region_input, number);
                            else {
                                use = find_final_register_use(original, record);
                                if (use) SPILL_KNOWN_KIND(use->value.reg);
                                if (!use || use->value.reg->kind != 3) {
                                    ++fixed_live;
                                    ++in_refs;
                                    operand->location.expression.use_flags |= 0x10;
                                } else operand->location.expression.use_flags &= ~0x10;
                            }
                        }
                        break;
                    case 2:
                        assigned = (C2AllocationRegister *)original;
                        operand->location.expression.use_flags &= ~0x80;
                        if ((operand->type & 0xfff) == 1) assigned->flags |= 0x80;
                        if (allocation_input_is_repeated(operand)) {
                            operand->location.expression.use_flags &= ~0x10;
                            break;
                        }
                        number = assigned->number;
                        SPILL_KNOWN_KIND(operand->value.reg);
                        if (record->opcode != 0x13a && operand->value.reg->kind != 13) ++in_refs;
                        if (track_frame && !optimizer_set_contains(retained, number)) {
                            add_register_to_set(retained, number);
                            ++retained_count;
                        }
                        if (!optimizer_set_contains(live, number)) {
                            operand->location.expression.use_flags |= 0x10;
                            add_register_to_set(live, number);
                            if (operand->location.expression.properties & 0x20)
                                adjust_allocation_input(record, assigned);
                        } else operand->location.expression.use_flags &= ~0x10;
                        output = record->tail.right;
                        if (record->opcode == 1 && operand->value.reg->kind != 13 && output->kind == 1) {
                            SPILL_KNOWN_KIND(output->original_register);
                            if (output->original_register->kind == 3 && !output->next && !record->left->next) {
                                original = output->original_register;
                                definition = original->allocation_definition_record;
                                defined_operand = original->allocation_definition_operand;
                                opcode = definition->opcode;
                                if (opcode != 0x13c &&
                                    (opcode != 0x15d || !final_flags_need_register(definition) || c2_favor_speed) &&
                                    (!opcode || opcode > 0x11a || !ALLOCATION_PAIRED_OPCODE(opcode) ||
                                     definition->left != defined_operand || find_final_register_use(original, definition)) &&
                                    ((defined_operand->type & 0xfff) <= (operand->type & 0xfff) ||
                                     operand->value.reg->kind == 3 || operand->value.reg->kind == 4 ||
                                     operand->value.reg->kind == 5 || operand->value.reg->kind == 6) &&
                                    ((defined_operand->location.expression.use_flags & 0x10) ||
                                     ((operand->location.expression.use_flags & 0x10) &&
                                      !optimizer_set_contains(block->allocation_live_in_after, number)))) {
                                    link = new_allocation_move();
                                    link->record = record;
                                    link->next = moves;
                                    moves = link;
                                }
                            }
                        }
                        break;
                    case 3:
                        operand->location.expression.use_flags &= ~0x80;
                        use = find_final_register_use(original, record);
                        if (!use) {
                            ++memory_live;
                            ++in_refs;
                            operand->location.expression.use_flags |= 0x10;
                        } else operand->location.expression.use_flags &= ~0x10;
                        output = record->tail.right;
                        if (record->opcode == 1 && output->kind == 1) {
                            SPILL_KNOWN_KIND(output->original_register);
                            if (output->original_register->kind == 1 && operand == record->left &&
                                allocation_copy_uses_fixed_register(record, original,
                                    output->original_register->canonical)) {
                                --memory_live;
                                ++fixed_live;
                                break;
                            }
                        }
                        if ((operand->type & 0xfff) == 1) {
                            original->view_flags |= 0x80;
                            if (operand->value.reg->displacement)
                                add_register_to_set(allocation_byte_exclusions, original->number);
                        }
                        original->allocation_definition_record = record;
                        original->allocation_definition_operand = operand;
                        break;
                    default:
                        c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0x3cc);
                        break;
                    }
                } else if (operand->kind == 2) {
                    operand->allocation_class_number = 0;
                    if (operand->opcode == 0x120) ++frame_traffic;
                } else if (operand->opcode == 0x120 || operand->opcode == 0x124) ++frame_traffic;
            }
            pressure = before_inputs + (in_refs < out_refs ? out_refs : in_refs);
            while (pressure > (int)limit) {
                spill_allocation_pressure(function, which, record);
                --memory_live;
                --pressure;
            }
            if (pressure == (int)limit) {
                choice = select_allocation_spill_choice(record, live, NULL, NULL);
                if (choice) {
                    merge_optimizer_symbol_sets(spills, choice->owner.registers);
                    block->flags |= 0x40000;
                }
            }
            if (track_frame && retained_count + pressure > 7) {
                if (frame_reference_cost && retained_count + pressure > 6)
                    frame_weighted_cost += frame_reference_weight(5, (short)block->number86);
                frame_traffic += spill_frame_candidates(retained, pressure, &frame_peak);
                retained_count = 0;
                clear_register_set(retained);
            }
previous_spill_record: ;
        }
    }
    release_optimizer_symbol_set(live);
    if (track_frame) {
        release_optimizer_symbol_set(retained);
        if (!c2_favor_speed) {
            if (frame_traffic > frame_peak + 1) {
                allocation_frame_register_available = 0;
                --allocation_class_limits[0];
            }
        } else if (frame_weighted_cost * 2 <= (int)frame_reference_cost) {
            allocation_frame_register_available = 0;
            --allocation_class_limits[0];
            disable_frame_register(function);
        }
    }
    if (moves) {
        finish_spill_moves(moves);
        release_allocation_moves(moves);
    }
    release_optimizer_symbol_set(allocation_byte_exclusions);
    if (!spills->links.first) {
        release_optimizer_symbol_set(spills);
        return NULL;
    }
    for (number = next_optimizer_symbol(spills); number != ~0U; number = next_optimizer_symbol(NULL))
        lookup_allocation_register(number)->flags |= 2;
    return spills;
}
#undef SPILL_KNOWN_KIND
unsigned __fastcall allocation_use_cost(C2FunctionRegister *reg)
{
    C2Expression *constant;
    long value;

    ALLOCATION_KNOWN_KIND(reg);
    if (c2_favor_speed) return reg->kind == 13 ? 1 : 2;
    switch (reg->kind) {
    case 3: case 4: case 5: case 6:
        return allocation_frame_register_available + 3;
    case 13:
        constant = reg->allocation_operand;
        if (constant->kind != 7) return 5;
        /* This cost deliberately consults only the original low DWORD. */
        value = (long)constant->integer.low;
        if (!value) return 2;
        if (value == -1) return 3;
        return (long)(signed char)value == value ? 3 : 5;
    default:
        return 6;
    }
}
unsigned __fastcall allocation_definition_cost(C2FunctionRegister *reg)
{
    if (c2_favor_speed) return 2;
    ALLOCATION_KNOWN_KIND(reg);
    if (reg->kind == 3 || reg->kind == 4 || reg->kind == 5 || reg->kind == 6)
        return allocation_frame_register_available + 3;
    return 6;
}
C2Expression *__fastcall find_record_constrained_operand(C2Expression *record,
    unsigned char constraint)
{
    C2Expression *operand = record->tail.right;

    while (constraint > 1) {
        operand = operand->next;
        --constraint;
    }
    return operand;
}

#define ALLOCATION_BINARY_OPCODE(opcode) \
    ((opcode >= 1 && opcode <= 2) || opcode == 0xa || opcode == 0xc || \
        (opcode >= 0x11 && opcode <= 0x15) || (opcode >= 0x1c && opcode <= 0x1d) || \
        (opcode >= 0x40 && opcode <= 0x42) || opcode == 0x56 || \
        (opcode >= 0x5a && opcode <= 0x5b) || opcode == 0x60 || \
        (opcode >= 0x62 && opcode <= 0x63) || (opcode >= 0x68 && opcode <= 0x70) || \
        opcode == 0x7e || opcode == 0x93 || opcode == 0xa3 || opcode == 0xb9 || \
        (opcode >= 0xbd && opcode <= 0xc0) || (opcode >= 0xc2 && opcode <= 0xc5) || \
        (opcode >= 0xc8 && opcode <= 0xc9) || opcode == 0xd0 || opcode == 0xe2 || \
        (opcode >= 0x10b && opcode <= 0x11a))

/* The reached copy test reads only the already modeled kind1 byte
 * from fixed address rows7/8; other static payload remains unavailable. */
#define COPY_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8]) ALLOCATION_KNOWN_KIND(reg); } while (0)
int __fastcall allocation_operand_is_copy(C2Expression *record, C2Expression *operand)
{
    C2Expression *input, *output, *selected, *constrained;
    unsigned opcode = record->opcode;
    unsigned char kind, constraint;
    int binary;

    COPY_KNOWN_KIND(operand->value.reg);
    if (operand->value.reg->kind == 13) return 1;
    if (operand->location.expression.properties & 0x20) return 0;
    switch (opcode) {
    case 6:
        return 1;
    case 0xe:
        return operand == record->left;
    case 0xa3: case 0xc2: case 0xc3: case 0xc4: case 0xc5:
        return operand != record->tail.right;
    case 0xc1:
        return operand != record->tail.right && operand != record->left;
    case 0x33: case 0xca: case 0xcb:
        input = record->left->next;
        kind = input->kind;
        return operand != input && kind != 2 && !((1U << (kind & 31)) & 0x60);
    case 0x12: case 0x107: case 0x108: case 0x109:
    case 0x13c: case 0x14c: case 0x15d: case 0x16c:
        return 0;
    }
    /* The consumed0x800 opcode column, only through proved domain0..175h.
     * No other WORD bits, neighboring storage, or total capacity is modeled. */
    if (opcode > 0x175) { __asm { int 3 } }
    binary = ALLOCATION_BINARY_OPCODE(opcode);
    input = record->left;
    output = record->tail.right;
    if (binary) {
        if (operand == output) {
            selected = input;
            kind = selected->kind;
            if (kind == 2 || ((1U << (kind & 31)) & 0x60)) return 0;
            if (kind == 1) {
                COPY_KNOWN_KIND(selected->original_register);
                COPY_KNOWN_KIND(selected->value.reg);
                if (selected->original_register->kind == 2 && selected->value.reg->kind != 13)
                    selected->location.expression.use_flags |= 0x80;
            }
        } else {
            selected = output;
            kind = selected->kind;
            if (kind == 2 || ((1U << (kind & 31)) & 0x60)) return 0;
            if (operand == input->next) return 0;
            if (kind == 1) {
                COPY_KNOWN_KIND(selected->original_register);
                if (selected->original_register->kind == 2)
                    selected->location.expression.use_flags |= 0x80;
            }
        }
        return 1;
    }
    if (operand == input->next) {
        kind = input->kind;
        if (kind == 2 || ((1U << (kind & 31)) & 0x60)) return 0;
        constraint = input->next->location.expression.properties & 0xf;
        if (constraint) {
            constrained = find_record_constrained_operand(record, constraint);
            input = record->left;
            if (constrained->value.reg != input->next->value.reg) return 0;
        }
        selected = input;
    } else {
        selected = input->next;
        if (!selected) {
            if (input->kind != 1) return 1;
        } else {
            kind = selected->kind;
            if (kind == 2 || ((1U << (kind & 31)) & 0x60)) return 0;
        }
        constraint = input->location.expression.properties & 0xf;
        if (constraint) {
            constrained = find_record_constrained_operand(record, constraint);
            input = record->left;
            if (constrained->value.reg != input->value.reg) return 0;
        }
        selected = input->next;
    }
    if (selected && selected->kind == 1) {
        /* Reached stack row5 has an independently modeled kind1. */
        if (selected->original_register != (C2FunctionRegister *)&machine_registers[5])
            COPY_KNOWN_KIND(selected->original_register);
        if (selected->original_register->kind == 2) {
            COPY_KNOWN_KIND(selected->value.reg);
            if (selected->value.reg->kind != 13)
                selected->location.expression.use_flags |= 0x80;
        }
    }
    return 1;
}
#undef COPY_KNOWN_KIND
int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value);
unsigned __fastcall allocation_copy_cost(C2Expression *record, C2FunctionRegister *reg)
{
    C2Expression *operand, *constant;
    unsigned opcode = record->opcode, bytes = record->type & 0xfff;
    long immediate;

    ALLOCATION_KNOWN_KIND(reg);
    if (c2_favor_speed) {
        if (reg->kind != 13) return 2;
        if (!opcode || opcode > 0x11a || ALLOCATION_BINARY_OPCODE(opcode))
            operand = record->tail.right;
        else operand = record->left;
        constant = reg->allocation_operand;
        if (opcode == 0x2f && constant->kind == 7 && !constant->integer.low) return 0;
        if (operand->kind == 2 || (operand->kind == 6 &&
            (operand->tail.right || operand->detail.data))) return 1;
        if (operand->kind == 1) {
            ALLOCATION_KNOWN_KIND(operand->original_register);
            if (operand->original_register->kind == 2 && operand->value.reg != reg)
                operand->location.expression.use_flags |= 0x80;
        }
        return 0;
    }
    if (reg->kind != 13) {
        if (reg->kind == 3 || reg->kind == 4 || reg->kind == 5 || reg->kind == 6)
            return allocation_frame_register_available + 1;
        return 4;
    }
    constant = reg->allocation_operand;
    if (constant->kind != 7) return 4;
    if (!constant->integer.low) {
        if (opcode == 1) operand = record->tail.right;
        else if (opcode == 0x2f) operand = record->left;
        else return 1;
        if (operand->kind != 1) return 1;
        ALLOCATION_KNOWN_KIND(operand->original_register);
        if (operand->original_register->kind == 2)
            operand->location.expression.use_flags |= 0x80;
        return 0;
    }
    if (integer_constant_differs(constant, ~(unsigned __int64)0)) {
        immediate = (long)constant->integer.low;
        if (opcode == 1) {
            if (bytes != 4 || (long)(signed char)immediate != immediate) return bytes - 1;
            operand = record->tail.right;
            if (operand->kind != 1) return 3;
            ALLOCATION_KNOWN_KIND(operand->original_register);
            if (operand->original_register->kind == 2)
                operand->location.expression.use_flags |= 0x80;
        } else if ((long)(signed char)immediate != immediate) return bytes;
    }
    return 1;
}
C2AllocationRegister *__fastcall unlink_allocation_live(C2AllocationRegister *head,
    C2AllocationRegister *reg)
{
    C2AllocationRegister *previous;

    if (head == reg) return reg->next_active;
    for (previous = head; previous->next_active != reg; previous = previous->next_active) {
    }
    previous->next_active = reg->next_active;
    return head;
}


void __fastcall coalesce_fixed_register(C2AllocationRegister *reg,
    C2FunctionRegister *physical)
{
    C2AllocationPreference *preference;

    if (reg->pinned_register) return;
    for (preference = reg->preferences; preference; preference = preference->next) {
        if (preference->physical == physical) {
            ++preference->count;
            return;
        }
    }
    preference = c2_pool_allocate(4, sizeof(C2AllocationPreference));
    preference->physical = physical;
    preference->count = 1;
    preference->next = reg->preferences;
    reg->preferences = preference;
}
int __fastcall allocation_definition_is_reusable(C2Expression *record)
{
    C2Expression *input;

    if (record->tail.right->next || record->opcode != 0x12) return 0;
    input = record->left;
    if (input->kind != 5) return 1;
    if (input->aggregate_value) {
        ALLOCATION_KNOWN_KIND(input->aggregate_value->original_register);
        if (input->aggregate_value->original_register->kind != 2) return 0;
    }
    if (input->aggregate_second) {
        ALLOCATION_KNOWN_KIND(input->aggregate_second->original_register);
        return input->aggregate_second->original_register->kind == 2;
    }
    return 1;
}
int __fastcall allocation_blocks_are_adjacent(C2BasicBlock *first, C2BasicBlock *last)
{
    C2FlowEdge *edge;

    for (edge = first->incoming; edge; edge = edge->next_incoming)
        if (edge->source == last) return 1;
    return 0;
}
int __fastcall allocation_record_breaks_reuse(C2Expression *record)
{
    C2Expression *operand;

    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind == 1) {
            ALLOCATION_KNOWN_KIND(operand->original_register);
            if (operand->original_register->kind != 2) return 1;
        }
    }
    return 0;
}

/* Reached fixed rows7/8 already have their kind1 and canonical self
 * identities. Extend only this consumer's reads of those existing columns. */
#define COALESCE_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9]) ALLOCATION_KNOWN_KIND(reg); } while (0)
#define COALESCE_KNOWN_CANONICAL(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9]) ALLOCATION_KNOWN_CANONICAL(reg); } while (0)
void __fastcall coalesce_allocation_registers(C2Function *function, int which,
    C2RegisterSet *registers)
{
    C2BasicBlock *block, *previous = NULL;
    C2RegisterSet *live = new_register_set(6), *fixed = new_register_set(6), *untracked;
    C2AllocationRegister *active, *inactive, *reg, *other;
    C2FunctionRegister *original, *physical;
    C2Expression *record, *operand, *output, *input, *use;
    unsigned number, cost;
    int weight, population, weighted_population;
    unsigned char flags;

    for (block = function->flow->last; block; block = block->previous) {
        if (!optimizer_symbol_sets_intersect(block->allocation_region_uses, registers)) continue;
        if (!previous || !allocation_blocks_are_adjacent(block, previous) ||
            !optimizer_symbol_sets_equal(block->allocation_live_out, previous->allocation_live_in_after)) {
            copy_optimizer_symbol_set_into(live, block->allocation_live_out);
            intersect_optimizer_symbol_sets(live, registers);
            active = NULL;
            for (number = next_optimizer_symbol(live); number != ~0U; number = next_optimizer_symbol(NULL)) {
                reg = lookup_allocation_register(number);
                reg->next_active = active;
                active = reg;
            }
        }
        copy_optimizer_symbol_set_into(fixed, block->allocation_edge_uses);
        for (reg = active; reg; reg = reg->next_active) {
            if (!reg->first_block) reg->first_block = block;
            reg->last_block = block;
        }
        inactive = NULL;
        untracked = new_register_set(6);
        weight = c2_favor_speed ? 1U << ((block->number86 + 1) & 31) : 1;
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
                    COALESCE_KNOWN_CANONICAL(operand->original_register);
                original = operand->original_register->canonical;
                if (original != (C2FunctionRegister *)&machine_registers[5])
                    COALESCE_KNOWN_KIND(original);
                if (original->kind == 2) {
                    reg = (C2AllocationRegister *)original;
                    physical = reg->pinned_register;
                    if (physical) {
                        if (!(reg->attributes & 8)) {
                            reg->attributes |= 8;
                            reg->next_active = inactive;
                            inactive = reg;
                        }
                        if ((operand->location.expression.use_flags & 0x10) &&
                            !(operand->location.expression.properties & 0x80)) {
                            if (physical != (C2FunctionRegister *)&machine_registers[4])
                                if (physical != (C2FunctionRegister *)&machine_registers[6])
                                    ALLOCATION_KNOWN_NUMBER(physical);
                            remove_optimizer_symbol(fixed, physical->number);
                        }
                        output = record->tail.right;
                        if ((record->opcode == 1 || record->opcode == 0x12) && output->kind == 1) {
                            COALESCE_KNOWN_KIND(output->original_register);
                            if (output->original_register->kind == 2)
                                coalesce_fixed_register((C2AllocationRegister *)output->original_register, physical);
                        }
                    } else if (!(reg->flags & 1)) add_register_to_set(untracked, reg->number);
                    else {
                        if (record->opcode == 0x13a) {
                            if (!c2_favor_speed || !block->block_scope ||
                                !operand->value.reg->allocation ||
                                optimizer_set_contains(block->block_scope->allocation_eligible,
                                    operand->value.reg->allocation->number)) {
                                reg->priority -= allocation_definition_cost(operand->value.reg) * weight;
                                reg->flags |= 4;
                            }
                            ++reg->coalesce_rank;
                        } else {
                            if (!allocation_operand_is_copy(record, operand)) {
                                cost = allocation_use_cost(operand->value.reg);
                                if ((operand->location.expression.properties & 0x20) &&
                                    allocation_input_is_repeated(operand)) ++reg->coalesce_rank;
                            } else cost = allocation_copy_cost(record, operand->value.reg);
                            reg->priority += weight * cost;
                            reg->reference_cost += cost;
                            ++reg->use_count;
                            if (!(operand->location.expression.properties & 0x20)) reg->flags |= 4;
                        }
                        reg->attributes |= 0x10;
                        if ((operand->location.expression.use_flags & 0x10) &&
                            !(operand->location.expression.properties & 0xf)) {
                            use = find_final_register_use((C2FunctionRegister *)reg, record);
                            if (!use || !(use->location.expression.properties & 0xf)) {
                                active = unlink_allocation_live(active, reg);
                                reg->next_active = inactive;
                                reg->attributes |= 8;
                                inactive = reg;
                            }
                        }
                    }
                } else if (original->kind == 1) {
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                    ALLOCATION_KNOWN_NUMBER(original);
                    number = original->number;
                    if (!optimizer_set_contains(allocation_class_registers[which], number)) continue;
                    COALESCE_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind == 3) {
                        if ((operand->location.expression.use_flags & 0x10) &&
                            !(operand->location.expression.properties & 0x80))
                            remove_optimizer_symbol(fixed, number);
                    } else {
                        for (other = active; other; other = other->next_active)
                            remove_optimizer_symbol(other->available, number);
                    }
                    output = record->tail.right;
                    if (record->opcode == 1 && output->kind == 1) {
                        COALESCE_KNOWN_KIND(output->original_register);
                        if (output->original_register->kind == 2)
                            coalesce_fixed_register((C2AllocationRegister *)output->original_register, original);
                    }
                }
            }
            if (record->control_flags & 2)
                for (reg = active; reg; reg = reg->next_active) reg->attributes |= 2;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                    if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
                    COALESCE_KNOWN_CANONICAL(operand->original_register);
                    original = operand->original_register->canonical;
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                    COALESCE_KNOWN_KIND(original);
                    if (original->kind == 2) {
                        reg = (C2AllocationRegister *)original;
                        physical = reg->pinned_register;
                        if (physical) {
                            if (!(reg->attributes & 8)) {
                                reg->attributes |= 8;
                                reg->next_active = inactive;
                                inactive = reg;
                            }
                            if (physical != (C2FunctionRegister *)&machine_registers[4])
                                if (physical != (C2FunctionRegister *)&machine_registers[6])
                                    ALLOCATION_KNOWN_NUMBER(physical);
                            number = physical->number;
                            add_register_to_set(fixed, number);
                            input = record->left;
                            if (record->opcode == 0x12 && input->kind == 5 && input->aggregate_value) {
                                COALESCE_KNOWN_KIND(input->aggregate_value->original_register);
                                if (input->aggregate_value->original_register->kind == 2)
                                    coalesce_fixed_register((C2AllocationRegister *)input->aggregate_value->original_register,
                                        physical);
                            }
                            if (record->opcode == 1 && input->kind == 1) {
                                COALESCE_KNOWN_KIND(input->original_register);
                                other = (C2AllocationRegister *)input->original_register;
                                if (other->kind == 2 && (other->flags & 1) &&
                                    ((record->type & 0xfff) != 1 ||
                                     (!(input->value.reg->displacement & 1) &&
                                      !(operand->value.reg->displacement & 1)))) {
                                    coalesce_fixed_register(other, physical);
                                    for (reg = active; reg; reg = reg->next_active)
                                        if (reg != other) remove_optimizer_symbol(reg->available, number);
                                    continue;
                                }
                            }
                            for (reg = active; reg; reg = reg->next_active)
                                remove_optimizer_symbol(reg->available, number);
                        } else if (!(reg->flags & 1)) add_register_to_set(untracked, reg->number);
                        else {
                            if (record->opcode == 0x139) {
                                reg->priority -= allocation_use_cost(operand->value.reg) * weight;
                                reg->flags |= 4;
                                if (operand->location.expression.use_flags & 0x80) reg->attributes |= 1;
                            } else {
                                if (!allocation_operand_is_copy(record, operand))
                                    cost = allocation_definition_cost(operand->value.reg);
                                else if (!(operand->location.expression.properties & 0xf))
                                    cost = allocation_copy_cost(record, operand->value.reg);
                                else cost = 0;
                                reg->priority += weight * cost;
                                ++reg->use_count;
                                reg->reference_cost += cost;
                                if (!reg->definition_record) {
                                    if (!allocation_definition_is_reusable(record)) reg->flags |= 4;
                                } else if (!(reg->flags & 4) &&
                                    (reg->definition_record->opcode != record->opcode ||
                                     compare_expression_trees(reg->definition_record, record))) reg->flags |= 4;
                            }
                            if (!(reg->flags & 8) && (!operand->allocation_class_number ||
                                allocation_record_breaks_reuse(record) ||
                                (reg->definition_record &&
                                 (reg->definition_record->opcode != record->opcode ||
                                  compare_expression_trees(reg->definition_record, record) ||
                                  record->control_flags != reg->definition_record->control_flags)))) reg->flags |= 8;
                            reg->definition_record = record;
                            subtract_optimizer_symbol_sets(reg->available, fixed);
                            if (!reg->first_block) reg->first_block = block;
                            reg->last_block = block;
                            flags = reg->attributes;
                            reg->attributes |= 0x10;
                            if (!(operand->location.expression.properties & 0xf)) {
                                if (flags & 8) {
                                    reg->attributes &= ~8;
                                    inactive = unlink_allocation_live(inactive, reg);
                                }
                                reg->next_active = active;
                                active = reg;
                            }
                        }
                    } else if (original->kind == 1) {
                        if (original != (C2FunctionRegister *)&machine_registers[5])
                    ALLOCATION_KNOWN_NUMBER(original);
                        number = original->number;
                        if (!optimizer_set_contains(allocation_class_registers[which], number)) continue;
                        input = record->left;
                        if (record->opcode == 1 && input->kind == 1) {
                            COALESCE_KNOWN_KIND(input->original_register);
                            other = (C2AllocationRegister *)input->original_register;
                            if (other->kind == 2 && (other->flags & 1)) {
                                coalesce_fixed_register(other, original);
                                for (reg = active; reg; reg = reg->next_active)
                                    if (reg != other) remove_optimizer_symbol(reg->available, number);
                                goto fixed_output_recorded;
                            }
                        }
                        for (reg = active; reg; reg = reg->next_active)
                            remove_optimizer_symbol(reg->available, number);
fixed_output_recorded:
                        COALESCE_KNOWN_KIND(operand->value.reg);
                        if (operand->value.reg->kind == 3 &&
                            (operand->location.expression.use_flags & 0x10)) add_register_to_set(fixed, number);
                    }
                } else if (operand->kind == 9) {
                    for (reg = active; reg; reg = reg->next_active)
                        subtract_optimizer_symbol_sets(reg->available, operand->value.registers);
                }
            }
        }
        population = count_optimizer_symbols(untracked);
        release_optimizer_symbol_set(untracked);
        for (reg = inactive; reg; reg = reg->next_active) ++population;
        for (reg = active; reg; reg = reg->next_active)
            if (reg->attributes & 0x10) ++population;
        weighted_population = weight * population;
        for (reg = inactive; reg; reg = reg->next_active) {
            if (!reg->pinned_register) {
                reg->weighted_pressure += weighted_population * reg->reference_cost;
                reg->reference_cost = 0;
                reg->attributes &= ~0x10;
            }
            reg->attributes &= ~8;
        }
        for (reg = active; reg; reg = reg->next_active) {
            if (!(reg->attributes & 0x10)) reg->weighted_pressure -= weighted_population;
            else {
                reg->attributes &= ~0x10;
                reg->weighted_pressure += weighted_population * reg->reference_cost;
                reg->reference_cost = 0;
            }
        }
        previous = block;
    }
    release_optimizer_symbol_set(live);
    release_optimizer_symbol_set(fixed);
}
#undef COALESCE_KNOWN_KIND
#undef COALESCE_KNOWN_CANONICAL
void __fastcall spill_allocation_register(C2Function *function, C2AllocationRegister *reg);
/* Reached fixed address rows7/8 have already modeled kind1 bytes. */
#define REFRESH_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8]) ALLOCATION_KNOWN_KIND(reg); } while (0)
void __fastcall refresh_allocation_work(C2Function *function, int which);

/* The former provisional compute_allocation_conflicts label belonged to this
 * actual dead/redundant-register pruning pass at41a6f8. */
void __fastcall prune_allocation_registers(C2Function *function, int which,
    C2RegisterSet *registers, C2RegisterSet *spills)
{
    C2RegisterSet *removed = NULL;
    C2AllocationRegister *reg;
    C2Expression *definition, *following, *use, *input, *replacement;
    C2FunctionRegister *address;
    C2BasicBlock *block;
    unsigned number, old_bytes, bytes;
    unsigned short type;
    unsigned char properties;
    long displacement;

    for (number = next_optimizer_symbol(registers); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_allocation_register(number);
        definition = reg->definition_record;
        if (reg->use_count < 2 && (reg->priority < 1 ||
            (definition && definition->opcode != 0x139))) {
            spill_allocation_register(function, reg);
            if (!removed) removed = new_register_set(6);
            add_register_to_set(removed, number);
            continue;
        }
        if (reg->use_count != 2 || definition->opcode != 1 ||
            !definition->tail.right->allocation_class_number) continue;
        following = definition->next;
        input = definition->left;
        if (!(following->flags & 1) || (input->kind != 2 && input->kind != 6)) continue;
        use = final_register_has_other_use((C2FunctionRegister *)reg, following);
        if (!use || !allocation_operand_is_copy(following, use)) continue;
        if (use == following->left) {
            if (following->opcode > 0x175) { __asm { int 3 } }
            if (ALLOCATION_PAIRED_OPCODE(following->opcode)) continue;
        }
        if (c2_favor_speed && following->opcode == 0xd) continue;
        replacement = input;
        type = use->type;
        old_bytes = replacement->type & 0xfff;
        bytes = type & 0xfff;
        if (old_bytes != bytes) {
            if (old_bytes < bytes) {
                if (replacement->kind == 6) continue;
                REFRESH_KNOWN_KIND(replacement->value.reg);
                if (replacement->kind == 2 &&
                    (replacement->value.reg->kind == 7 || replacement->value.reg->kind == 8)) continue;
            }
            displacement = use->value.reg->displacement - definition->tail.right->value.reg->displacement;
            if (replacement->kind == 2) {
                address = new_address_register(replacement->value.reg, type, bytes, displacement);
                replacement = new_typed_register_expression(replacement->opcode, type, address);
            } else {
                properties = replacement->location.expression.properties;
                replacement = new_aggregate_expression(replacement->opcode, type,
                    replacement->tail.aggregate_register,
                    replacement->detail.aggregate_displacement + displacement,
                    replacement->aggregate_value, replacement->aggregate_second, replacement->immediate);
                replacement->location.expression.properties =
                    (replacement->location.expression.properties & 0xf0) | (properties & 0xf);
            }
        }
        attach_input_expression(following, use, replacement);
        unlink_and_release_input_record((C2Record *)definition);
        if (!removed) removed = new_register_set(6);
        add_register_to_set(removed, number);
        block = reg->first_block;
        remove_optimizer_symbol(block->allocation_region_uses, number);
        block->flags |= 0x80000;
        release_allocation_register(reg, 1);
    }
    if (!removed) return;
    subtract_optimizer_symbol_sets(registers, removed);
    if (spills) subtract_optimizer_symbol_sets(spills, removed);
    else if (allocation_deferred_registers) {
        subtract_optimizer_symbol_sets(allocation_deferred_registers, removed);
        if (!allocation_deferred_registers->links.first) {
            release_optimizer_symbol_set(allocation_deferred_registers);
            allocation_deferred_registers = NULL;
        }
    }
    release_optimizer_symbol_set(removed);
    refresh_allocation_work(function, which);
}
void __fastcall restrict_allocation_register(C2AllocationRegister *reg)
{
    if (reg->flags & 0x80) {
        intersect_optimizer_symbol_sets(reg->available, allocation_call_clobbered);
        reg->attributes |= 2;
        return;
    }
    if (!allocation_frame_register_available) remove_optimizer_symbol(reg->available, 6);
    if (reg->attributes & 2) {
        intersect_optimizer_symbol_sets(reg->available, allocation_call_preserved);
        reg->attributes &= ~2;
    }
}
void __fastcall force_allocation_register(C2AllocationRegister *reg, C2RegisterSet *available);
C2AllocationRegister *__fastcall insert_allocation_work(C2AllocationRegister *reg,
    C2AllocationRegister *head);

C2RegisterSet *__fastcall order_allocation_work(C2Function *function, int which,
    C2RegisterSet *spills)
{
    C2RegisterSet *allowed = copy_optimizer_symbol_set(allocation_class_registers[which]);
    C2AllocationRegister *reg, *head = NULL;
    unsigned bucket;

    if (!allocation_frame_register_available) remove_optimizer_symbol(allowed, 6);
    for (bucket = 0; bucket < 1024; ++bucket) {
        for (reg = allocation_register_hash[bucket]; reg; reg = reg->next_hash) {
            ALLOCATION_KNOWN_TYPE_CLASS(reg->source->type);
            if (ALLOCATION_TYPE_CLASS(reg->source->type) != which) continue;
            restrict_allocation_register(reg);
            if (!reg->available->links.first) {
                reg->flags |= 2;
                if (!spills) spills = new_register_set(6);
                add_register_to_set(spills, reg->number);
                force_allocation_register(reg, (reg->flags & 0x80) ? allocation_call_clobbered : allowed);
            }
            head = insert_allocation_work(reg, head);
        }
    }
    release_optimizer_symbol_set(allowed);
    active_allocation_registers = head;
    return spills;
}
C2AllocationRegister *__fastcall sort_allocation_work(C2AllocationRegister *head)
{
    C2AllocationRegister *reg, *next, *previous;

    for (reg = head; reg; reg = next) {
        next = reg->next_active;
        if (!(reg->flags & 2)) continue;
        if (reg == head) {
            head = next;
            if (next) next->previous_active = NULL;
        } else {
            previous = reg->previous_active;
            previous->next_active = next;
            if (next) next->previous_active = previous;
        }
    }
    return head;
}
C2AllocationSpillBlock *free_allocation_spill_blocks; /* Original4910e0, real reset before use. */
C2AllocationSpillBlock *new_allocation_spill_block(void)
{
    C2AllocationSpillBlock *block = free_allocation_spill_blocks;

    if (block) free_allocation_spill_blocks = block->next;
    else block = c2_pool_allocate(4, sizeof(*block));
    return block;
}
typedef struct C2AllocationSpillUse {
    struct C2AllocationSpillUse *next;
    C2AllocationRegister *reg;
} C2AllocationSpillUse;
typedef char C2AllocationSpillUseSize[sizeof(C2AllocationSpillUse) == 8 ? 1 : -1];
C2AllocationSpillUse *free_allocation_spill_uses; /* Original4910e4, reset before use. */
C2AllocationSpillUse *new_allocation_spill_use(void)
{
    C2AllocationSpillUse *use = free_allocation_spill_uses;

    if (use) free_allocation_spill_uses = use->next;
    else use = c2_pool_allocate(4, sizeof(*use));
    return use;
}
unsigned __fastcall collect_spill_block_masks(C2AllocationSpillBlock *blocks,
    C2RegisterSet *spills)
{
    C2BasicBlock *block;
    C2Expression *record, *previous, *operand;
    C2AllocationRegister *reg;
    C2AllocationSpillUse *uses, *use;
    C2RegisterSet *set;
    unsigned allocated = 0, number, required, live, defined, memory, killed, bits;

    for (; blocks; blocks = blocks->next) {
        block = blocks->block;
        required = live = defined = memory = killed = 0;
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = previous) {
            previous = record->previous;
            if (!(record->flags & 1)) {
                if (record->kind != 26) continue;
                if (!record->control_flags) {
                    set = record->value.registers;
                    intersect_optimizer_symbol_sets(set, spills);
                    intersect_optimizer_symbol_sets(set, block->allocation_region_uses);
                    if (!set->links.first) {
                        /* Original continuation is record->next->previous
                         * after unlinking, rather than a stale prior link. */
                        previous = record->next;
                        unlink_and_release_input_record((C2Record *)record);
                        release_optimizer_symbol_set(set);
                        previous = previous->previous;
                        continue;
                    }
                    record->control_flags = 1;
                    uses = NULL;
                    for (number = next_optimizer_symbol(set); number != ~0U;
                         number = next_optimizer_symbol(NULL)) {
                        use = new_allocation_spill_use();
                        use->reg = lookup_allocation_register(number);
                        use->next = uses;
                        uses = use;
                    }
                    record->value.pointer = uses;
                    release_optimizer_symbol_set(set);
                }
                bits = 0;
                for (use = record->value.pointer; use; use = use->next) bits |= use->reg->spill_mask;
                record->immediate = bits;
                defined |= bits;
                live &= ~bits;
                memory |= bits;
                continue;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[73] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74])
                    REFRESH_KNOWN_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 2)) continue;
                bits = reg->spill_mask;
                if (!(bits & defined) && !(operand->location.expression.properties & 0xf)) {
                    required |= bits;
                    defined |= bits;
                }
                live &= ~bits;
                memory &= ~bits;
                killed |= bits;
                allocated |= operand->allocation_class_number;
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[73] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74])
                        REFRESH_KNOWN_KIND(operand->original_register);
                    reg = (C2AllocationRegister *)operand->original_register;
                    if (reg->kind != 2 || !(reg->flags & 2)) continue;
                    bits = reg->spill_mask;
                    if (!(defined & bits)) required |= bits;
                    live |= bits;
                    if (operand->location.expression.use_flags & 0x10) {
                        killed |= bits;
                        memory &= ~bits;
                    }
                } else if (operand->kind == 2 && operand->allocation_class_number) {
                    reg = lookup_allocation_register(operand->allocation_class_number);
                    memory |= reg->spill_mask;
                }
            }
        }
        block->spill_required_mask = required;
        block->spill_live_mask = live;
        block->spill_defined_mask = defined;
        block->spill_memory_mask = memory;
        block->spill_killed_mask = killed;
        block->spill_live_in_mask = live;
    }
    return allocated;
}
void __fastcall solve_spill_block_masks(C2AllocationSpillBlock *first,
    C2AllocationSpillBlock *last, unsigned mask)
{
    C2AllocationSpillBlock *item;
    C2BasicBlock *block;
    C2FlowEdge *edge;
    unsigned changed, bits, old, value;

    do {
        changed = 0;
        for (item = last; item; item = item->previous) {
            block = item->block;
            bits = 0;
            for (edge = block->outgoing; edge; edge = edge->next_outgoing)
                bits |= edge->destination->spill_live_in_mask;
            if (bits == block->spill_live_out_mask) continue;
            old = block->spill_live_in_mask;
            block->spill_live_out_mask = bits;
            value = (bits & ~block->spill_defined_mask) | block->spill_live_mask;
            block->spill_live_in_mask = value;
            changed |= value ^ old;
        }
    } while (changed);
    for (item = first; item; item = item->next) {
        block = item->block;
        block->spill_limit_in_mask = block->spill_live_in_mask;
        block->spill_live_in_mask = 0;
        block->spill_limit_out_mask = block->spill_live_out_mask;
        block->spill_live_out_mask &= block->spill_required_mask;
    }
    do {
        changed = 0;
        for (item = first; item; item = item->next) {
            block = item->block;
            bits = 0;
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                bits |= edge->source->spill_live_out_mask;
            bits &= block->spill_limit_in_mask;
            if (bits == block->spill_live_in_mask) continue;
            old = block->spill_live_out_mask;
            block->spill_live_in_mask = bits;
            value = ((bits & ~block->spill_defined_mask) | block->spill_required_mask) &
                block->spill_limit_out_mask;
            block->spill_live_out_mask = value;
            changed |= value ^ old;
        }
    } while (changed);
    if (!mask) {
        for (item = first; item; item = item->next) {
            block = item->block;
            block->spill_limit_in_mask = block->spill_live_in_mask;
            block->spill_live_in_mask = 0;
            block->spill_limit_out_mask = block->spill_live_out_mask;
            block->spill_live_out_mask = 0;
            block->spill_memory_mask = 0;
        }
        return;
    }
    for (item = first; item; item = item->next) {
        block = item->block;
        block->spill_limit_in_mask = block->spill_live_in_mask;
        block->spill_live_in_mask = block->spill_memory_mask;
        block->spill_limit_out_mask = block->spill_live_out_mask;
        block->spill_live_out_mask = 0;
    }
    do {
        changed = 0;
        for (item = last; item; item = item->previous) {
            block = item->block;
            bits = 0;
            for (edge = block->outgoing; edge; edge = edge->next_outgoing)
                bits |= edge->destination->spill_live_in_mask;
            if (bits == block->spill_live_out_mask) continue;
            old = block->spill_live_in_mask;
            block->spill_live_out_mask = bits;
            value = (bits & ~block->spill_killed_mask) | block->spill_memory_mask;
            block->spill_live_in_mask = value;
            changed |= value ^ old;
        }
    } while (changed);
    for (item = first; item; item = item->next) {
        block = item->block;
        block->spill_live_in_mask = 0;
        block->spill_memory_mask = block->spill_live_out_mask;
        block->spill_live_out_mask = 0;
    }
}
typedef struct C2AllocationSpillClass {
    struct C2AllocationSpillClass *next, *parent;
    unsigned number;
} C2AllocationSpillClass;
typedef char C2AllocationSpillClassSize[sizeof(C2AllocationSpillClass) == 12 ? 1 : -1];
C2AllocationSpillClass *free_allocation_spill_classes; /* Original4910d8, reset before use. */
C2AllocationSpillClass *new_allocation_spill_class(void)
{
    C2AllocationSpillClass *group = free_allocation_spill_classes;

    if (group) {
        free_allocation_spill_classes = group->next;
        group->parent = NULL;
    } else group = c2_pool_allocate(4, sizeof(*group));
    return group;
}
void __fastcall merge_allocation_spill_classes(C2AllocationSpillClass *first,
    C2AllocationSpillClass *second)
{
    while (first->parent) first = first->parent;
    while (second->parent) second = second->parent;
    if (first != second) second->parent = first;
}
void __fastcall merge_spill_register_constraints(C2AllocationRegister *registers)
{
    C2AllocationRegister *reg, *assigned;
    C2AllocationSpillClass *classes, *group, *in, *out, *other, *last, *root;
    C2BasicBlock *block;
    C2FlowEdge *edge;
    unsigned mask, in_mask, out_mask;

    for (reg = registers; reg; reg = reg->next_active) {
        mask = reg->spill_mask;
        if (!mask) return;
        classes = NULL;
        for (block = reg->first_block;; block = block->previous) {
            if (!(block->flags & 1)) continue;
            in_mask = block->spill_limit_in_mask;
            out_mask = block->spill_limit_out_mask;
            in = out = NULL;
            if (in_mask & mask) {
                for (edge = block->incoming; edge; edge = edge->next_incoming) {
                    other = edge->source->spill_class_out;
                    if (!other) continue;
                    if (!in) block->spill_class_in = in = other;
                    else if (in != other) merge_allocation_spill_classes(in, other);
                }
                if (!(out_mask & mask) || (block->spill_defined_mask & mask)) {
                    if (!in) {
                        in = new_allocation_spill_class();
                        in->next = classes;
                        classes = in;
                    }
                } else {
                    for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                        other = edge->destination->spill_class_in;
                        if (!other) continue;
                        if (!in) in = other;
                        else if (in != other) merge_allocation_spill_classes(in, other);
                    }
                    out = in;
                    if (!in) {
                        out = in = new_allocation_spill_class();
                        in->next = classes;
                        classes = in;
                    }
                }
                block->spill_class_in = in;
            }
            if ((out_mask & mask) && (!(in_mask & mask) || (block->spill_defined_mask & mask))) {
                for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                    other = edge->destination->spill_class_in;
                    if (!other) continue;
                    if (!out) out = other;
                    else if (out != other) merge_allocation_spill_classes(out, other);
                }
                if (!out) {
                    out = new_allocation_spill_class();
                    out->next = classes;
                    classes = out;
                }
            }
            block->spill_class_out = out;
            if (block == reg->last_block) break;
        }
        clear_register_set(reg->available);
        for (group = classes; group; group = group->next) {
            if (group->parent) continue;
            assigned = get_allocation_register(reg->source);
            assigned->definition_sequence = reg->definition_sequence;
            group->number = assigned->number;
            assigned->next_active = reg;
            add_register_to_set(reg->available, assigned->number);
        }
        if (!classes) continue;
        for (group = classes; group; group = group->next) {
            if (!group->parent) continue;
            root = group->parent;
            while (root->parent) root = root->parent;
            group->number = root->number;
        }
        for (last = classes; last->next; last = last->next) {
        }
        last->next = free_allocation_spill_classes;
        free_allocation_spill_classes = classes;
        for (block = reg->first_block;; block = block->previous) {
            in = block->spill_class_in;
            if (in) {
                add_register_to_set(block->allocation_live_out, in->number);
                add_register_to_set(block->allocation_region_uses, in->number);
                block->spill_class_in = NULL;
            }
            out = block->spill_class_out;
            if (out) {
                add_register_to_set(block->allocation_live_in_after, out->number);
                add_register_to_set(block->allocation_region_uses, out->number);
                block->spill_class_out = NULL;
            }
            if (block == reg->last_block) break;
        }
    }
}
void __fastcall mark_region_allocation_spill(C2FlowRegion *region, C2FunctionRegister *reg);
void __fastcall increase_allocation_spill_cost(C2Expression *record,
    C2AllocationRegister *reg, C2BasicBlock *block);
int __fastcall region_spill_mask_is_available(C2FlowRegion *region, unsigned mask)
{
    C2FlowBlockLink *exit;

    if (!(region->header->spill_limit_in_mask & mask)) return 0;
    for (exit = region->exits; exit; exit = exit->next)
        if (!(exit->block->spill_limit_out_mask & mask)) return 0;
    return 1;
}
C2Expression *__fastcall rewrite_spill_output(C2Expression *record, C2Expression *operand)
{ __asm { int 3 } return NULL; }
void __fastcall rewrite_spill_block_records(C2Function *function,
    C2AllocationSpillBlock *blocks)
{
    C2BasicBlock *block;
    C2Expression *record, *previous, *operand, *inserted, *input;
    C2AllocationRegister *reg;
    unsigned live, unavailable, memory, bits;
    unsigned short line;

    for (; blocks; blocks = blocks->next) {
        block = blocks->block;
        live = block->spill_limit_out_mask;
        unavailable = ~live;
        memory = block->spill_memory_mask;
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = previous) {
            previous = record->previous;
            if (!(record->flags & 1)) {
                if (record->kind == 26) {
                    bits = record->immediate;
                    live &= ~bits;
                    unavailable |= bits;
                    memory |= bits;
                }
                continue;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[73])
                    REFRESH_KNOWN_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 2)) continue;
                bits = reg->spill_mask;
                if (operand->allocation_class_number && !(reg->flags & 0x10) && (memory & bits)) {
                    operand->allocation_class_number = 0;
                    line = record->location.line;
                    if (line) {
                        record_source_line = function->source_line_base + line;
                        record_line_delta = line;
                    }
                    inserted = spill_allocation_result(operand->value.reg, reg, record);
                    input = inserted->left;
                    if (unavailable & bits) input->location.expression.use_flags &= ~0x40;
                    else input->location.expression.use_flags |= 0x40;
                    if (live & bits) input->location.expression.use_flags &= ~0x10;
                    else input->location.expression.use_flags |= 0x10;
                }
                memory &= ~bits;
                if (block->block_scope && !region_spill_mask_is_available(block->block_scope, bits))
                    mark_region_allocation_spill(block->block_scope, operand->value.reg);
                if (!(operand->location.expression.properties & 0xf)) {
                    if (!(live & bits)) {
                        if (record->opcode == 0x139) {
                            unavailable |= bits;
                            unlink_and_release_input_record((C2Record *)record);
                            goto previous_spill_block_record;
                        }
                        if (!operand->allocation_class_number && (operand->type & 0xf000) != 0x4000 &&
                            !((operand->value.reg->type ^ operand->type) & 0xfff)) {
                            unavailable |= bits;
                            if (operand->location.expression.use_flags & 0x80)
                                increase_allocation_spill_cost(record, reg, block);
                            line = record->location.line;
                            if (line) {
                                record_source_line = function->source_line_base + line;
                                record_line_delta = line;
                            }
                            operand = rewrite_spill_output(record, operand);
                            continue;
                        }
                    } else live &= ~bits;
                    if (!(unavailable & bits)) {
                        unavailable |= bits;
                        operand->location.expression.use_flags |= 0x40;
                    } else operand->location.expression.use_flags &= ~0x40;
                } else {
                    if (unavailable & bits) operand->location.expression.use_flags &= ~0x40;
                    else operand->location.expression.use_flags |= 0x40;
                    live &= ~bits;
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[73])
                        REFRESH_KNOWN_KIND(operand->original_register);
                    reg = (C2AllocationRegister *)operand->original_register;
                    if (reg->kind != 2 || !(reg->flags & 2)) continue;
                    bits = reg->spill_mask;
                    if (unavailable & bits) operand->location.expression.use_flags &= ~0x40;
                    else operand->location.expression.use_flags |= 0x40;
                    if (operand->location.expression.use_flags & 0x10) memory &= ~bits;
                    if (!(live & bits) && !allocation_input_is_repeated(operand)) {
                        live |= bits;
                        operand->location.expression.use_flags |= 0x10;
                    } else operand->location.expression.use_flags &= ~0x10;
                } else if (operand->kind == 2 && operand->allocation_class_number) {
                    reg = lookup_allocation_register(operand->allocation_class_number);
                    bits = reg->spill_mask;
                    if (bits) {
                        operand->allocation_class_number = 0;
                        memory |= bits;
                    }
                }
            }
previous_spill_block_record: ;
        }
    }
}
void __fastcall release_allocation_spill_block(C2AllocationSpillBlock *block)
{
    C2AllocationSpillBlock *old = free_allocation_spill_blocks;

    free_allocation_spill_blocks = block;
    block->next = old;
}
void __fastcall release_allocation_spill_uses(C2AllocationSpillUse *uses)
{
    C2AllocationSpillUse *last = uses, *old;

    while (last->next) last = last->next;
    old = free_allocation_spill_uses;
    free_allocation_spill_uses = uses;
    last->next = old;
}
C2Expression *__fastcall reload_spill_input(C2Expression *record, C2Expression *operand)
{
    C2Expression *replacement;

    operand->allocation_class_number = 0;
    if (allocation_operand_is_copy(record, operand) &&
        (record->opcode != 0xd || !c2_favor_speed)) {
        make_global_register_operand(operand);
        return operand;
    }
    make_global_register_operand(operand);
    replacement = new_assignment_expression(1, operand->type, operand,
        prepend_input_record, (C2Record *)record)->tail.right;
    replacement->location.expression.use_flags |= 0x10;
    if ((operand->type & 0xfff) == 1) replacement->original_register->view_flags |= 0x80;
    replacement = attach_input_expression(record, operand, replacement);
    replacement->location.expression.use_flags |= 0x10;
    return replacement;
}
C2Expression *__fastcall find_spill_constrained_operand(C2Expression *record,
    unsigned char constraint)
{
    C2Expression *operand = record->left;

    while (constraint > 1) {
        operand = operand->next;
        --constraint;
    }
    return operand;
}
void __fastcall finish_spill_block_records(C2Function *function,
    C2AllocationSpillBlock *blocks, C2RegisterSet *registers)
{
    C2RegisterSet *live = new_register_set(6), *scratch = new_register_set(6), *available;
    C2AllocationSpillBlock *next;
    C2AllocationSpillUse *use;
    C2BasicBlock *block;
    C2Expression *record, *operand, *inserted;
    C2AllocationRegister *reg, *assigned;
    unsigned number;
    unsigned char flags, constraint;
    unsigned short line;

    for (; blocks; blocks = next) {
        next = blocks->next;
        block = blocks->block;
        release_allocation_spill_block(blocks);
        initialize_block_register_regions(function, block, 1);
        copy_optimizer_symbol_set_into(live, registers);
        intersect_optimizer_symbol_sets(live, block->allocation_live_out);
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) {
                if (record->kind == 26) {
                    for (use = record->value.pointer; use; use = use->next) {
                        subtract_optimizer_symbol_sets(live, use->reg->available);
                        if (block->block_scope)
                            mark_region_allocation_spill(block->block_scope, use->reg->source);
                    }
                    release_allocation_spill_uses(record->value.pointer);
                    record = record->previous;
                    unlink_and_release_input_record((C2Record *)record->next);
                }
                continue;
            }
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[73])
                    REFRESH_KNOWN_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 2)) continue;
                available = reg->available;
                number = find_optimizer_set_intersection(available, live);
                if (number == ~0U) {
                    flags = operand->location.expression.use_flags;
                    if (!(flags & 0x40)) {
                        if ((flags & 0x10) && (operand->type & 0xf000) != 0x4000 &&
                            !(operand->location.expression.properties & 0xf)) {
                            REFRESH_KNOWN_KIND(operand->value.reg);
                            if (operand->value.reg->kind == 13)
                                operand = materialize_allocation_input(record, operand, operand->value.reg);
                            else {
                                if (flags & 0x80) increase_allocation_spill_cost(record, reg, block);
                                operand = reload_spill_input(record, operand);
                            }
                            continue;
                        }
                        assigned = get_allocation_register(operand->value.reg);
                        assigned->definition_sequence = reg->definition_sequence;
                        assigned->next_active = reg;
                        number = assigned->number;
                        add_register_to_set(available, number);
                        add_register_to_set(block->allocation_region_uses, number);
                        if (!(flags & 0x10)) add_register_to_set(live, number);
                    } else {
                        number = find_optimizer_set_intersection(available, block->allocation_live_in_after);
                        assigned = lookup_allocation_register(number);
                        number = assigned->number;
                        add_register_to_set(live, number);
                    }
                    operand->original_register = (C2FunctionRegister *)assigned;
                    inserted = bind_allocation_spill(operand->value.reg,
                        (C2FunctionRegister *)assigned, record);
                    if ((inserted->type & 0xfff) == 1) assigned->flags |= 0x80;
                } else {
                    assigned = lookup_allocation_register(number);
                    operand->original_register = (C2FunctionRegister *)assigned;
                    if (operand->location.expression.use_flags & 0x10)
                        remove_optimizer_symbol(live, number);
                }
                if ((operand->type & 0xfff) == 1) assigned->flags |= 0x80;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[74] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[67] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[73])
                    REFRESH_KNOWN_KIND(operand->original_register);
                reg = (C2AllocationRegister *)operand->original_register;
                if (reg->kind != 2 || !(reg->flags & 2)) continue;
                available = reg->available;
                subtract_optimizer_symbol_sets(live, available);
                constraint = operand->location.expression.properties & 0xf;
                if (!constraint) {
                    if (!(operand->location.expression.use_flags & 0x40)) {
                        assigned = get_allocation_register(operand->value.reg);
                        assigned->definition_sequence = reg->definition_sequence;
                        assigned->next_active = reg;
                        number = assigned->number;
                        add_register_to_set(available, number);
                        add_register_to_set(block->allocation_region_uses, number);
                    } else {
                        number = find_optimizer_set_intersection(available, block->allocation_live_in_after);
                        assigned = lookup_allocation_register(number);
                    }
                } else assigned = (C2AllocationRegister *)
                    find_spill_constrained_operand(record, constraint)->original_register;
                operand->original_register = (C2FunctionRegister *)assigned;
                add_register_to_set(live, assigned->number);
                if ((operand->type & 0xfff) == 1) assigned->flags |= 0x80;
            }
        }
    }
    release_optimizer_symbol_set(scratch);
    release_optimizer_symbol_set(live);
}
void __fastcall apply_allocation_spills(C2Function *function, C2RegisterSet *spills,
    int which)
{
    C2AllocationRegister *registers, *reg, *last_reg, *batch, *next_batch, *next_reg;
    C2AllocationSpillBlock *first = NULL, *last = NULL, *item;
    C2BasicBlock *block;
    C2Expression *record, *next;
    unsigned number, mask, used;

    registers = lookup_allocation_register(next_optimizer_symbol(spills));
    registers->spill_mask = 0;
    registers->next_active = NULL;
    last_reg = registers;
    for (number = next_optimizer_symbol(NULL); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_allocation_register(number);
        reg->next_active = NULL;
        reg->spill_mask = 0;
        reg->flags &= ~2;
        last_reg->next_active = reg;
        last_reg = reg;
    }
    for (block = function->flow->last; block; block = block->previous) {
        if (optimizer_symbol_sets_intersect(block->allocation_region_uses, spills)) {
            item = new_allocation_spill_block();
            item->block = block;
            item->next = NULL;
            item->previous = last;
            if (last) last->next = item;
            else first = item;
            last = item;
            block->flags &= ~0x40000;
            subtract_optimizer_symbol_sets(block->allocation_live_out, spills);
            subtract_optimizer_symbol_sets(block->allocation_live_in_after, spills);
            item->saved_uses = block->allocation_edge_uses;
            item->saved_definitions = block->allocation_edge_definitions;
            block->flags |= 1;
        } else {
            block->flags &= ~1;
            if (!(block->flags & 0x40000)) continue;
            block->flags &= ~0x40000;
            for (record = (C2Expression *)block->records;
                 record != (C2Expression *)block->last_record; record = next) {
                next = record->next;
                if (record->kind != 26) continue;
                release_optimizer_symbol_set(record->value.registers);
                unlink_and_release_input_record((C2Record *)record);
            }
        }
    }
    batch = registers;
    do {
        mask = 1;
        for (reg = batch; reg; reg = reg->next_active) {
            reg->spill_mask = mask;
            reg->flags |= 2;
            mask <<= 1;
            if (!mask) break;
        }
        next_batch = reg ? reg->next_active : NULL;
        used = collect_spill_block_masks(first, spills);
        solve_spill_block_masks(first, last, used);
        merge_spill_register_constraints(batch);
        rewrite_spill_block_records(function, first);
        if (!next_batch) break;
        for (reg = batch; reg != next_batch; reg = reg->next_active) {
            reg->spill_mask = 0;
            reg->flags &= ~2;
        }
        batch = next_batch;
    } while (1);
    for (item = first; item; item = item->next) {
        block = item->block;
        block->allocation_edge_uses = item->saved_uses;
        block->allocation_edge_definitions = item->saved_definitions;
        subtract_optimizer_symbol_sets(block->allocation_region_uses, spills);
        block->flags &= ~1;
    }
    release_optimizer_symbol_set(spills);
    for (reg = registers; reg; reg = reg->next_active) reg->flags |= 2;
    finish_spill_block_records(function, first, allocation_register_classes[which]);
    for (reg = registers; reg; reg = next_reg) {
        next_reg = reg->next_active;
        release_allocation_register(reg, 0);
    }
}
C2AllocationRegister *__fastcall merge_allocation_work(C2RegisterSet *registers,
    C2AllocationRegister *head)
{
    C2AllocationRegister *reg;
    unsigned number;

    if (!registers->links.first) return head;
    for (number = next_optimizer_symbol(registers); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        reg = lookup_allocation_register(number);
        reg->flags |= 0x20;
        restrict_allocation_register(reg);
        head = insert_allocation_work(reg, head);
    }
    return head;
}
int __fastcall allocation_can_coalesce(C2AllocationRegister *reg)
{
    C2AllocationPreference *preference;
    C2FunctionRegister *physical = NULL;
    C2Expression *definition;
    int best_count = 0;

    if ((int)(allocation_use_cost(reg->source) + (unsigned)reg->priority) <= 0) return 0;
    for (preference = reg->preferences; preference; preference = preference->next) {
        if (preference->physical != (C2FunctionRegister *)&machine_registers[4])
            ALLOCATION_KNOWN_NUMBER(preference->physical);
        if (!optimizer_set_contains(reg->available, preference->physical->number)) continue;
        if (!physical || (int)preference->count > best_count) {
            physical = preference->physical;
            best_count = (int)preference->count;
        }
    }
    if (!physical) return 0;
    if (reg->use_count == 3 && (definition = reg->definition_record) != NULL &&
        definition->opcode > 0 && definition->opcode < 0x11b &&
        ALLOCATION_PAIRED_OPCODE(definition->opcode) &&
        !definition->tail.right->allocation_class_number) return 0;
    clear_register_set(reg->available);
    add_register_to_set(reg->available, physical->number);
    reg->priority = 1;
    return 1;
}
C2AllocationRegister *__fastcall delay_allocation_register(C2AllocationRegister *head,
    C2AllocationRegister *reg)
{
    C2AllocationRegister *position, *previous;

    REFRESH_KNOWN_KIND(reg->source);
    reg->next_active = reg->previous_active = NULL;
    if (reg->source->kind == 13) {
        for (position = head; position->next_active; position = position->next_active) {
        }
        position->next_active = reg;
        reg->previous_active = position;
        reg->weighted_pressure = position->weighted_pressure;
        return head;
    }
    for (position = head; !(position->flags & 0x40) && position->next_active;
         position = position->next_active) {
    }
    if (!(position->flags & 0x40)) goto append_delayed;
    REFRESH_KNOWN_KIND(position->source);
    if ((reg->priority <= position->priority || position->source->kind == 13) && position == head) {
        reg->next_active = head;
        head->previous_active = reg;
        reg->weighted_pressure = head->weighted_pressure;
        return reg;
    }
    while (position->priority < reg->priority) {
        REFRESH_KNOWN_KIND(position->source);
        if (position->source->kind == 13 || !position->next_active) break;
        position = position->next_active;
    }
    if (position->priority < reg->priority) {
        REFRESH_KNOWN_KIND(position->source);
        if (position->source->kind != 13) goto append_delayed;
    }
    previous = position->previous_active;
    reg->next_active = position;
    reg->previous_active = previous;
    previous->next_active = reg;
    position->previous_active = reg;
    reg->weighted_pressure = position->weighted_pressure;
    return head;
append_delayed:
    position->next_active = reg;
    reg->previous_active = position;
    reg->weighted_pressure = position->weighted_pressure;
    return head;
}
void __fastcall increase_allocation_spill_cost(C2Expression *record,
    C2AllocationRegister *excluded, C2BasicBlock *block)
{
    C2Expression *operand, *constant;
    C2FunctionRegister *source;
    C2AllocationRegister *reg;
    unsigned cost;

    operand = record->tail.right;
    if (operand->kind == 1) {
        if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
            REFRESH_KNOWN_KIND(operand->original_register);
        if (operand->original_register->kind == 2 &&
            operand->original_register != (C2FunctionRegister *)excluded) goto found_cost_operand;
    }
    operand = record->left;
    if (operand->kind == 1) {
        if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
            REFRESH_KNOWN_KIND(operand->original_register);
        if (operand->original_register->kind == 2 &&
            operand->original_register != (C2FunctionRegister *)excluded) goto found_cost_operand;
    }
    operand = operand->next;
    if (!operand || operand->kind != 1) return;
    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
        REFRESH_KNOWN_KIND(operand->original_register);
    if (operand->original_register->kind != 2 ||
        operand->original_register == (C2FunctionRegister *)excluded) return;
found_cost_operand:
    source = operand->value.reg;
    reg = (C2AllocationRegister *)operand->original_register;
    REFRESH_KNOWN_KIND(source);
    if (source->kind == 13) {
        if (c2_favor_speed) cost = 1U << ((block->number86 + 1) & 31);
        else {
            constant = source->allocation_operand;
            if (constant->kind != 7) return;
            if (!constant->integer.low) {
                if (record->opcode != 1 && record->opcode != 0x2f) return;
                cost = 1;
            } else {
                if (!integer_constant_differs(constant, ~(unsigned __int64)0) ||
                    record->opcode != 1 || (record->type & 0xfff) != 4 ||
                    (int)(signed char)constant->integer.low != (int)constant->integer.low) return;
                cost = 2;
            }
        }
    } else {
        cost = allocation_copy_cost(record, source);
        cost = allocation_use_cost(source) - cost;
        if (c2_favor_speed) cost *= 1U << ((block->number86 + 1) & 31);
    }
    reg->priority = (int)((unsigned)reg->priority + cost);
}
void __fastcall spill_allocation_register(C2Function *function, C2AllocationRegister *reg)
{
    C2BasicBlock *block = reg->first_block;
    C2Expression *record, *next, *operand, *scan, *temporary, *move;
    C2FunctionRegister *original;
    unsigned number = reg->number;
    unsigned short type, line;
    int side, prepare = 0, finish = 0;

    for (;;) {
        if (!optimizer_set_contains(block->allocation_region_uses, number)) {
            block = block->previous;
            continue;
        }
        remove_optimizer_symbol(block->allocation_region_uses, number);
        remove_optimizer_symbol(block->allocation_live_out, number);
        remove_optimizer_symbol(block->allocation_live_in_after, number);
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            if (!(record->flags & 1)) continue;
            if (record->opcode == 0x139) {
                if (record->tail.right->original_register == (C2FunctionRegister *)reg)
                    unlink_and_release_input_record((C2Record *)record);
                continue;
            }
            if (record->opcode == 0x13a) {
                if (record->left->original_register == (C2FunctionRegister *)reg)
                    unlink_and_release_input_record((C2Record *)record);
                continue;
            }
            for (side = 0; side < 2; ++side) {
                for (operand = side ? record->left : record->tail.right;
                     operand; operand = operand->next) {
                    if (operand->kind != 1 ||
                        operand->original_register != (C2FunctionRegister *)reg) continue;
                    REFRESH_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind == 13) {
                        operand = materialize_allocation_input(record, operand, operand->value.reg);
                        continue;
                    }
                    if (!side && operand->previous && reg->use_count == 1) {
                        for (scan = record->left; scan; scan = scan->next) {
                            if (scan->kind != 1) continue;
                            original = scan->original_register;
                            REFRESH_KNOWN_KIND(original);
                            if (original->kind == 3 && original->value)
                                unlink_and_release_input_record((C2Record *)original->value);
                            else if (original->kind == 2 &&
                                     !((C2AllocationRegister *)original)->pinned_register) {
                                ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                                ALLOCATION_KNOWN_TYPE_CLASS(scan->type);
                                if (ALLOCATION_TYPE_CLASS(operand->type) !=
                                    ALLOCATION_TYPE_CLASS(scan->type)) continue;
                                if (!allocation_deferred_registers)
                                    allocation_deferred_registers = new_register_set(6);
                                add_register_to_set(allocation_deferred_registers, original->number);
                                original->view_flags |= 2;
                                block->flags |= 0x40000;
                            }
                        }
                        unlink_and_release_input_record((C2Record *)record);
                        block->flags |= 0x80000;
                        release_allocation_register(reg, 1);
                        return;
                    }
                    if (operand->location.expression.properties & 0x20) prepare = 1;
                    else {
                        if (operand->location.expression.use_flags & 0x80)
                            increase_allocation_spill_cost(record, reg, block);
                        finish = 1;
                    }
                    make_global_register_operand(operand);
                    type = operand->value.reg->type;
                    if (!side && (type & 0xfff) < (operand->type & 0xfff)) {
                        temporary = new_temporary_operand(operand->type);
                        temporary->value.reg->value = record;
                        move = new_binary_expression(1, type, operand, temporary,
                            insert_input_record, (C2Record *)record);
                        operand = replace_input_result(record, operand, temporary);
                        type = move->type;
                        move->tail.right->type = move->left->type = type;
                        move->left->value.reg = new_address_register(move->left->value.reg,
                            type, type & 0xfff, 0);
                        if ((type & 0xfff) == 1)
                            temporary->original_register->view_flags |= 0x80;
                    }
                }
            }
            if (prepare) {
                prepare = 0;
                line = record->location.line;
                if (line) {
                    record_source_line = function->source_line_base + line;
                    record_line_delta = line;
                }
                prepare_final_record_operands(record);
                block->flags |= 0x80000;
            }
            if (finish) {
                finish = 0;
                line = record->location.line;
                if (line) {
                    record_source_line = function->source_line_base + line;
                    record_line_delta = line;
                }
                record = finish_final_integer_record(record);
                block->flags |= 0x80000;
            }
            next = record->next;
        }
        if (block == reg->last_block) break;
        block = block->previous;
    }
    release_allocation_register(reg, 1);
}
void __fastcall refresh_allocation_work(C2Function *function, int which)
{
    C2RegisterSet *live = new_register_set(6);
    C2BasicBlock *block;
    C2Expression *record, *operand, *use;
    C2FunctionRegister *original;
    int needed;

    for (block = function->flow->last; block; block = block->previous) {
        if (!(block->flags & 0x80000)) continue;
        block->flags &= ~0x80000;
        copy_optimizer_symbol_set_into(live, block->allocation_live_in_after);
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                original = operand->original_register;
                REFRESH_KNOWN_KIND(original);
                switch (original->kind) {
                case 1:
                    REFRESH_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind == 1) break;
                    needed = !final_register_has_other_use(original, record);
                    if (needed) operand->location.expression.use_flags |= 0x10;
                    else operand->location.expression.use_flags &= ~0x10;
                    break;
                case 2:
                    remove_optimizer_symbol(live, original->number);
                    break;
                case 3:
                    needed = !final_register_has_other_use(original, record);
                    if (needed) operand->location.expression.use_flags |= 0x10;
                    else operand->location.expression.use_flags &= ~0x10;
                    if ((operand->type & 0xfff) == 1) original->view_flags |= 0x80;
                    break;
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind == 2) {
                    operand->allocation_class_number = 0;
                    continue;
                }
                if (operand->kind != 1) continue;
                operand->allocation_class_number = 0;
                ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                original = operand->original_register;
                REFRESH_KNOWN_KIND(original);
                switch (original->kind) {
                case 1:
                    REFRESH_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind == 1) break;
                    use = find_final_register_use(original, record);
                    if (use) REFRESH_KNOWN_KIND(use->value.reg);
                    if (use && use->value.reg->kind == 3)
                        operand->location.expression.use_flags &= ~0x10;
                    else operand->location.expression.use_flags |= 0x10;
                    break;
                case 2:
                    if (optimizer_set_contains(live, original->number) ||
                        allocation_input_is_repeated(operand))
                        operand->location.expression.use_flags &= ~0x10;
                    else {
                        operand->location.expression.use_flags |= 0x10;
                        add_register_to_set(live, original->number);
                    }
                    break;
                case 3:
                    use = find_final_register_use(original, record);
                    if (use) REFRESH_KNOWN_KIND(use->value.reg);
                    if (use && use->value.reg->kind == 3)
                        operand->location.expression.use_flags &= ~0x10;
                    else operand->location.expression.use_flags |= 0x10;
                    if ((operand->type & 0xfff) == 1) original->view_flags |= 0x80;
                    break;
                }
            }
        }
    }
    release_optimizer_symbol_set(live);
}
#undef REFRESH_KNOWN_KIND
void __fastcall relax_pinned_allocation(C2AllocationRegister *reg)
{
    C2RegisterSet *numbers = new_register_set(6);
    C2BasicBlock *block, *destination;
    C2FlowEdge *edge;
    C2Expression *record, *next, *position;
    unsigned number = reg->number;

    add_register_to_set(numbers, number);
    for (block = reg->first_block;; block = block->previous) {
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            if (record->opcode != 0x139 ||
                record->tail.right->original_register != (C2FunctionRegister *)reg ||
                !(record->tail.right->location.expression.use_flags & 0x80)) continue;
            for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                destination = edge->destination;
                if (!optimizer_set_contains(destination->allocation_live_out, number)) continue;
                position = (C2Expression *)destination->records;
                if (position->next->kind == 25) position = position->next;
                select_allocation_spill_choice(position, NULL, NULL, numbers);
                destination->flags |= 0x40000;
                /* The original unlinks this same record for every matching
                 * outgoing edge; it does not move the unlink outside the loop. */
                unlink_and_release_input_record((C2Record *)record);
            }
        }
        if (block == reg->last_block) break;
    }
    release_optimizer_symbol_set(numbers);
}
void __fastcall relax_allocation_candidate(C2AllocationRegister *reg)
{
    C2RegisterSet *numbers = new_register_set(6);
    C2BasicBlock *block, *last;
    C2Expression *position;
    unsigned number = reg->number;

    add_register_to_set(numbers, number);
    last = reg->last_block;
    for (block = reg->first_block;; block = block->previous) {
        if (optimizer_set_contains(block->allocation_live_out, number)) {
            position = (C2Expression *)block->records;
            if (position->next->kind == 25) position = position->next;
            select_allocation_spill_choice(position, NULL, NULL, numbers);
            block->flags |= 0x40000;
        }
        if (block == last) break;
    }
    release_optimizer_symbol_set(numbers);
}
int __fastcall try_assign_allocation_register(C2AllocationRegister *reg)
{ __asm { int 3 } return 0; }
/* Original41b6fa inserts by descending weighted pressure, then sequence.
 * It does not initialize the new head's unused previous link. */
C2AllocationRegister *__fastcall insert_allocation_work(C2AllocationRegister *reg,
    C2AllocationRegister *head)
{
    C2AllocationRegister *previous, *next;

    reg->flags &= ~1;
    reg->next_active = NULL;
    if (!head) return reg;
    if (reg->weighted_pressure > head->weighted_pressure ||
        (reg->weighted_pressure == head->weighted_pressure &&
         reg->definition_sequence >= head->definition_sequence)) {
        reg->next_active = head;
        head->previous_active = reg;
        return reg;
    }
    previous = head;
    while (previous->next_active) {
        next = previous->next_active;
        if (reg->weighted_pressure > next->weighted_pressure ||
            (reg->weighted_pressure == next->weighted_pressure &&
             reg->definition_sequence >= next->definition_sequence)) break;
        previous = next;
    }
    reg->next_active = previous->next_active;
    reg->previous_active = previous;
    if (reg->next_active) reg->next_active->previous_active = reg;
    previous->next_active = reg;
    return head;
}
void __fastcall prepare_clobbered_allocation(C2AllocationRegister *reg) { __asm { int 3 } }
/* Only the existing kind/canonical columns of fixed integer/flag views
 * enter this pass. The sentinel has no allocation-register payload. */
#define CONSTRAIN_KNOWN_REGISTER(reg) \
    do { if ((unsigned)(reg) >= (unsigned)machine_registers && \
        (unsigned)(reg) <= (unsigned)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        !((unsigned)(reg) >= (unsigned)&machine_registers[1] && \
          (unsigned)(reg) <= (unsigned)&machine_registers[8]) && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        !((unsigned)(reg) >= (unsigned)&machine_registers[66] && \
          (unsigned)(reg) <= (unsigned)&machine_registers[73])) { __asm { int 3 } } } while (0)
#define CONSTRAIN_KNOWN_NUMBER(reg) \
    do { if ((unsigned)(reg) >= (unsigned)machine_registers && \
        (unsigned)(reg) <= (unsigned)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        !((unsigned)(reg) >= (unsigned)&machine_registers[1] && \
          (unsigned)(reg) <= (unsigned)&machine_registers[8]) && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)
int __fastcall constrain_spill_block(C2AllocationRegister *reg, C2BasicBlock *block,
    C2RegisterSet *available, C2RegisterSet *selected, int first, C2RegisterSet *allowed)
{
    C2RegisterSet *blocked = copy_optimizer_symbol_set(block->allocation_edge_uses);
    C2RegisterSet *candidates = copy_optimizer_symbol_set(available);
    C2Expression *record, *operand;
    C2AllocationRegister *assigned;
    C2FunctionRegister *original, *physical;
    C2Record *position;
    unsigned number;
    int result;

    for (record = (C2Expression *)block->records;
         record != (C2Expression *)block->last_record; record = record->next) {
        if (!(record->flags & 1)) {
            if (record->kind == 26 && optimizer_set_contains(((C2Record *)record)->owner.registers, reg->number)) {
                if (!first) goto failed_spill_block;
                clear_register_set(candidates);
            }
            continue;
        }
        if (record->opcode != 0x13a) {
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1 || (C2AllocationRegister *)operand->original_register != reg) continue;
                if (!candidates->links.first) {
                    copy_optimizer_symbol_set_into(candidates, allowed);
                    subtract_optimizer_symbol_sets(candidates, blocked);
                    select_allocation_spill_choice(record->previous, NULL, NULL, selected);
                    block->flags |= 0x40000;
                    if (!first) goto failed_spill_block;
                } else copy_optimizer_symbol_set_into(available, candidates);
            }
        }
        for (operand = record->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            original = operand->original_register;
            CONSTRAIN_KNOWN_REGISTER(original);
            if (original->kind == 2) {
                assigned = (C2AllocationRegister *)original;
                if (!assigned->pinned_register || !(operand->location.expression.use_flags & 0x10) ||
                    (operand->location.expression.properties & 0x80)) continue;
                physical = assigned->pinned_register;
                CONSTRAIN_KNOWN_NUMBER(physical);
                remove_optimizer_symbol(blocked, physical->number);
            } else if (original->kind == 1) {
                CONSTRAIN_KNOWN_REGISTER(operand->value.reg);
                physical = original->canonical;
                CONSTRAIN_KNOWN_NUMBER(physical);
                if (operand->value.reg->kind == 3) {
                    if ((operand->location.expression.use_flags & 0x10) &&
                        !(operand->location.expression.properties & 0x80))
                        remove_optimizer_symbol(blocked, physical->number);
                } else remove_optimizer_symbol(candidates, physical->number);
            }
        }
        if ((record->control_flags & 2) && !(reg->attributes & 2)) {
            intersect_optimizer_symbol_sets(candidates, allocation_call_preserved);
            if (!candidates->links.first) {
                copy_optimizer_symbol_set_into(candidates, allowed);
                subtract_optimizer_symbol_sets(candidates, blocked);
                intersect_optimizer_symbol_sets(candidates, allocation_call_preserved);
                select_allocation_spill_choice(record->previous, NULL, NULL, selected);
                if (!first) goto failed_spill_block;
            }
        }
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (operand->kind == 1) {
                original = operand->original_register;
                CONSTRAIN_KNOWN_REGISTER(original);
                if (original->kind == 2) {
                    physical = ((C2AllocationRegister *)original)->pinned_register;
                    if (physical) {
                        CONSTRAIN_KNOWN_NUMBER(physical);
                        number = physical->number;
                        remove_optimizer_symbol(candidates, number);
                        add_register_to_set(blocked, number);
                    }
                } else if (original->kind == 1) {
                    physical = original->canonical;
                    CONSTRAIN_KNOWN_NUMBER(physical);
                    number = physical->number;
                    remove_optimizer_symbol(candidates, number);
                    CONSTRAIN_KNOWN_REGISTER(operand->value.reg);
                    if (operand->value.reg->kind == 3) add_register_to_set(blocked, number);
                }
            } else if (operand->kind == 9)
                subtract_optimizer_symbol_sets(candidates, operand->value.registers);
        }
        if (record->opcode != 0x139) {
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1 || (C2AllocationRegister *)operand->original_register != reg) continue;
                if (operand->location.expression.properties & 15) break;
                copy_optimizer_symbol_set_into(candidates, allowed);
                subtract_optimizer_symbol_sets(candidates, blocked);
                if (!first) goto failed_spill_block;
            }
        }
    }
    if (!first) block->spill_visit = block->spill_partition;
    if (!candidates->links.first) {
        position = block->last_record->previous;
        if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, selected);
        else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, selected);
        block->flags |= 0x40000;
        result = 0;
    } else {
        copy_optimizer_symbol_set_into(available, candidates);
        result = 1;
    }
    goto finish_spill_block;
failed_spill_block:
    result = 0;
finish_spill_block:
    release_optimizer_symbol_set(blocked);
    release_optimizer_symbol_set(candidates);
    return result;
}
#undef CONSTRAIN_KNOWN_REGISTER
#undef CONSTRAIN_KNOWN_NUMBER

void __fastcall force_allocation_register(C2AllocationRegister *reg, C2RegisterSet *available)
{
    C2RegisterSet *scratch = new_register_set(6), *selected = new_register_set(6);
    C2AllocationSpillBlock *work, *wave, *item;
    C2BasicBlock *block, *current, *neighbor;
    C2FlowEdge *edge, *scan;
    C2Record *position;
    unsigned visit = 0;

    add_register_to_set(selected, reg->number);
    for (block = reg->first_block; ; block = block->previous) {
        if (!optimizer_set_contains(block->allocation_region_uses, reg->number)) continue;
        if (!block->spill_visit) {
            block->spill_visit = ++visit;
            if (!block->spill_partition) {
                block->spill_partition = visit;
                clear_register_set(scratch);
            } else copy_optimizer_symbol_set_into(scratch, available);
            work = NULL;
            if (constrain_spill_block(reg, block, scratch, selected, 1, available) &&
                optimizer_set_contains(block->allocation_live_in_after, reg->number)) {
                for (edge = block->outgoing; edge; edge = edge->next_outgoing)
                    if (edge->destination->spill_partition) break;
                if (edge) {
                    position = block->last_record->previous;
                    if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, selected);
                    else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, selected);
                    block->flags |= 0x40000;
                } else for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                    neighbor = edge->destination;
                    if (!optimizer_set_contains(neighbor->allocation_live_out, reg->number)) continue;
                    item = new_allocation_spill_block();
                    item->next = work;
                    item->block = neighbor;
                    neighbor->spill_partition = visit;
                    work = item;
                }
            }
            while (work) {
                wave = NULL;
                while (work) {
                    current = work->block;
                    item = work;
                    work = work->next;
                    release_allocation_spill_block(item);
                    if (!constrain_spill_block(reg, current, scratch, selected, 0, available) ||
                        !optimizer_set_contains(current->allocation_live_in_after, reg->number)) continue;
                    for (scan = current->outgoing; scan; scan = scan->next_outgoing)
                        if (scan->destination->spill_partition &&
                            scan->destination->spill_partition != visit) break;
                    if (scan) {
                        position = current->last_record->previous;
                        if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, selected);
                        else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, selected);
                        current->flags |= 0x40000;
                        continue;
                    }
                    for (edge = current->outgoing; edge; edge = edge->next_outgoing) {
                        neighbor = edge->destination;
                        if (neighbor->spill_partition ||
                            !optimizer_set_contains(neighbor->allocation_live_out, reg->number)) continue;
                        item = new_allocation_spill_block();
                        item->next = wave;
                        item->block = neighbor;
                        neighbor->spill_partition = visit;
                        wave = item;
                    }
                }
                work = wave;
            }
        }
        if (block == reg->last_block) break;
    }
    release_optimizer_symbol_set(scratch);
    release_optimizer_symbol_set(selected);
    for (block = reg->first_block; ; block = block->previous) {
        block->spill_partition = 0;
        block->spill_visit = 0;
        if (block == reg->last_block) break;
    }
}
/* Width-dependent copies may inspect only the proved static ranges. */
#define ALLOCATION_KNOWN_DISPLACEMENT(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)

void __fastcall prepare_allocation_spill_candidate(C2AllocationRegister *reg)
{ __asm { int 3 } }
C2AllocationSpillChoice *__fastcall select_allocation_byte_spill(C2Expression *record,
    C2RegisterSet *live, C2AllocationRegister *reg)
{
    C2Expression *operand;
    C2AllocationRegister *assigned;
    C2AllocationSpillChoice *choice;
    C2RegisterSet *candidates = NULL, *one;
    unsigned number;
    int inputs = 0, clobbered;

    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        if (operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
            operand->original_register != (C2FunctionRegister *)&machine_registers[8])
            ALLOCATION_KNOWN_KIND(operand->original_register);
        assigned = (C2AllocationRegister *)operand->original_register;
        if (assigned->kind != 2 || assigned == reg || assigned->pinned_register) continue;
        ALLOCATION_KNOWN_KIND(operand->value.reg);
        if (operand->value.reg->kind == 13) {
            if (!candidates) candidates = new_register_set(6);
            add_register_to_set(candidates, assigned->number);
            operand = materialize_allocation_input(record, operand, operand->value.reg);
        } else ++inputs;
    }
    for (number = next_optimizer_symbol(live); number != ~0U; number = next_optimizer_symbol(NULL)) {
        assigned = lookup_allocation_register(number);
        if (assigned == reg || assigned->pinned_register ||
            (candidates && optimizer_set_contains(candidates, number))) continue;
        clobbered = !optimizer_symbol_sets_intersect(assigned->available, allocation_call_preserved);
        if (!clobbered) {
            if (inputs > 1 && final_register_has_other_use((C2FunctionRegister *)assigned, record) &&
                !find_final_register_use((C2FunctionRegister *)assigned, record)) clobbered = 1;
            else {
                intersect_optimizer_symbol_sets(assigned->available, allocation_call_preserved);
                continue;
            }
        }
        assigned->attributes |= 2;
        if (find_final_register_use((C2FunctionRegister *)assigned, record)) continue;
        if ((assigned->flags & 0x80) && final_register_has_other_use((C2FunctionRegister *)assigned, record) &&
            (record->left->type & 0xfff) == 1) continue;
        if (!candidates) candidates = new_register_set(6);
        add_register_to_set(candidates, number);
        if (inputs > 1 && final_register_has_other_use((C2FunctionRegister *)assigned, record)) {
            one = new_register_set(6);
            add_register_to_set(one, number);
            select_allocation_spill_choice(record->previous, NULL, NULL, one);
            release_optimizer_symbol_set(one);
        }
    }
    if (!candidates) return NULL;
    if (record->control_flags && record->next->kind == 26) {
        choice = (C2Record *)record->next;
        merge_optimizer_symbol_sets(choice->owner.registers, candidates);
    } else choice = select_allocation_spill_choice(record, NULL, NULL, candidates);
    release_optimizer_symbol_set(candidates);
    return choice;
}
C2RegisterSet *__fastcall finish_allocation_reuse(C2AllocationMoveLink *moves,
    C2AllocationRegister *reg) { __asm { int 3 } return NULL; }
void __fastcall restrict_allocation_width(C2AllocationRegister *reg, unsigned bytes)
{ __asm { int 3 } }
void __fastcall restrict_allocation_constraint(C2AllocationRegister *reg, unsigned char constraint)
{ __asm { int 3 } }
C2Expression *__fastcall find_allocation_choice_operand(C2Expression *record,
    C2AllocationRegister *excluded)
{
    C2Expression *output, *operand;
    C2AllocationRegister *reg;
    unsigned opcode = record->opcode;

    if (!opcode || opcode >= 0x11b || record->kind == 13) return NULL;
    output = record->tail.right;
    if (output && (output->kind != 1 || !record->left || !record->left->next)) return NULL;
    for (operand = record->left; operand; operand = operand->next)
        if (operand->kind == 2 || ((1U << (operand->kind & 31)) & 0x60)) return NULL;
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        if (operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
            operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
            operand->original_register != (C2FunctionRegister *)&machine_registers[8])
            ALLOCATION_KNOWN_KIND(operand->original_register);
        reg = (C2AllocationRegister *)operand->original_register;
        if (reg->kind != 2 || reg == excluded || reg->pinned_register) continue;
        ALLOCATION_KNOWN_KIND(operand->value.reg);
        if (operand->value.reg->kind == 13 || operand->value.reg->kind == 3) continue;
        if (output && output->original_register == (C2FunctionRegister *)reg) continue;
        if (opcode == 0xca || opcode == 0xcb) continue;
        if (opcode == 0x33 && operand != record->left) continue;
        return operand;
    }
    return NULL;
}
void __fastcall mark_region_allocation_spill(C2FlowRegion *region,
    C2FunctionRegister *reg)
{
    unsigned number;

    /* Machine-row allocation-info payload is outside the partial static model. */
    if ((unsigned)reg >= (unsigned)machine_registers &&
        (unsigned)reg <= (unsigned)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
    if (!reg->allocation || reg->kind == 13) return;
    if (reg->allocation_root) reg = reg->allocation_root;
    number = reg->allocation->number;
    do {
        add_register_to_set(region->allocation_eligible, number);
        region = region->parent;
    } while (region);
}
void __fastcall finish_allocation_choices(C2AllocationMoveLink *moves,
    C2RegisterSet *choices, C2AllocationRegister *reg)
{
    C2RegisterSet *profitable = new_register_set(6), *selected, *other_numbers, *remaining;
    C2AllocationMoveLink *link, *other;
    C2AllocationRegister *assigned;
    C2Expression *record, *operand;
    C2BasicBlock *block;
    unsigned number;

    for (number = next_optimizer_symbol(choices); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        assigned = lookup_allocation_register(number);
        ALLOCATION_KNOWN_KIND(assigned->source);
        if (assigned->source->kind != 13 && assigned->priority > 0)
            add_register_to_set(profitable, number);
    }
    for (link = moves; link; link = link->next) {
        record = link->record;
        if (!record) continue;
        selected = record->next->value.registers;
        if (!optimizer_symbol_sets_intersect(selected, profitable) ||
            record->control_flags != 1) continue;
        operand = find_allocation_choice_operand(record, reg);
        if (!operand) continue;
        remaining = copy_optimizer_symbol_set(selected);
        intersect_optimizer_symbol_sets(remaining, profitable);
        other_numbers = new_register_set(6);
        for (other = moves; other; other = other->next) {
            if (!other->record || other == link) continue;
            /* Original defect: this loop rereads the current record's choice,
             * rather than the other move's record. Preserve that exact owner. */
            selected = record->next->value.registers;
            if (optimizer_symbol_sets_intersect(selected, remaining))
                merge_optimizer_symbol_sets(other_numbers, selected);
        }
        intersect_optimizer_symbol_sets(other_numbers, remaining);
        assigned = (C2AllocationRegister *)operand->original_register;
        if (!other_numbers->links.first || assigned->priority < 1) {
            subtract_optimizer_symbol_sets(selected, remaining);
            subtract_optimizer_symbol_sets(remaining, other_numbers);
            subtract_optimizer_symbol_sets(choices, remaining);
            record->control_flags = 0;
            add_register_to_set(selected, assigned->number);
            add_register_to_set(choices, assigned->number);
            block = allocation_block_for_record(record);
            if (block->block_scope)
                mark_region_allocation_spill(block->block_scope, operand->value.reg);
            if (operand->location.expression.use_flags & 0x80)
                increase_allocation_spill_cost(record, assigned, block);
            make_global_register_operand(operand);
            operand->allocation_class_number = assigned->number;
            assigned->flags |= 8;
        }
        release_optimizer_symbol_set(remaining);
        release_optimizer_symbol_set(other_numbers);
    }
    release_optimizer_symbol_set(profitable);
}
C2AllocationMoveLink *allocation_fixed_moves; /* Original4931c4, initialized NULL. */
C2AllocationMoveLink *allocation_current_choices; /* Original493200, set before consumer. */

/* Fixed address rows7/8 already have their kind and canonical identity.
 * This local admission leaves every unmodeled static column unavailable. */
#define CHOICE_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[33]) ALLOCATION_KNOWN_KIND(reg); } while (0)
#define CHOICE_KNOWN_CANONICAL(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9]) ALLOCATION_KNOWN_CANONICAL(reg); } while (0)
/* Row6's original DWORD number is already modeled as6. */
#define CHOICE_KNOWN_NUMBER(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[6]) ALLOCATION_KNOWN_NUMBER(reg); } while (0)
C2RegisterSet *__fastcall choose_allocation_spill(C2Function *function,
    C2AllocationRegister *reg, int which, C2RegisterSet *available, C2RegisterSet *spills,
    C2RegisterSet *excluded, unsigned limit)
{
    C2RegisterSet *choices, *live, *fixed, *discard, *reused;
    C2AllocationMoveLink *reuse_moves = NULL, *choice_moves = NULL, *link;
    C2AllocationSpillChoice *choice;
    C2BasicBlock *block;
    C2Expression *record, *operand, *input, *output, *use;
    C2FunctionRegister *original, *physical, *spilled;
    C2AllocationRegister *assigned, *other, *best;
    unsigned number = reg->number, physical_number, other_number, line, bytes;
    unsigned max_input_bytes = 0, max_definition_bytes = 0;
    unsigned char constraint = 0;
    int total, byte_total, memory_live = 0, byte_live = 0, was_byte_live;
    int out_refs, in_refs, byte_out_refs, byte_in_refs, other_live;
    int before_inputs, before_byte_inputs, pressure, byte_pressure, alive, remaining;

    if (reg->attributes & 4) prepare_allocation_spill_candidate(reg);
    choices = new_register_set(6);
    live = new_register_set(6);
    fixed = new_register_set(6);
    for (block = reg->first_block; ; block = block->previous) {
        if (!optimizer_set_contains(block->allocation_region_uses, number)) continue;
        copy_optimizer_symbol_set_into(live, block->allocation_live_in_after);
        copy_optimizer_symbol_set_into(fixed, block->allocation_edge_definitions);
        total = byte_total = count_optimizer_symbols(fixed);
        if (optimizer_set_contains(fixed, 7)) --byte_total;
        if (optimizer_set_contains(fixed, 8)) --byte_total;
        if (optimizer_set_contains(fixed, 6)) --byte_total;
        alive = optimizer_set_contains(live, number) != 0;
        if (alive) ++total;
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            line = record->location.line;
            if (line) {
                record_source_line = function->source_line_base + line;
                record_line_delta = line;
            }
            out_refs = in_refs = byte_out_refs = byte_in_refs = 0;
            was_byte_live = byte_live;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                    if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                    original = operand->original_register;
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                        CHOICE_KNOWN_KIND(original);
                    if (original->kind == 1) {
                        if (original != (C2FunctionRegister *)&machine_registers[5])
                        CHOICE_KNOWN_CANONICAL(original);
                        physical = original->canonical;
                        if (physical != (C2FunctionRegister *)&machine_registers[5] &&
                            physical != (C2FunctionRegister *)&machine_registers[4])
                        CHOICE_KNOWN_NUMBER(physical);
                        physical_number = physical->number;
                        if (!optimizer_set_contains(allocation_class_registers[which], physical_number)) continue;
                        CHOICE_KNOWN_KIND(operand->value.reg);
                        if (operand->value.reg->kind == 1) {
                            ++out_refs;
                            if (optimizer_set_contains(allocation_call_clobbered, physical_number)) ++byte_out_refs;
                        } else if (operand->location.expression.use_flags & 0x10) {
                            input = record->left;
                            if (record->opcode == 1 && input->kind == 1) {
                                CHOICE_KNOWN_KIND(input->original_register);
                                other = (C2AllocationRegister *)input->original_register;
                                if (other->kind == 2 && other->pinned_register == physical) continue;
                            }
                            --total;
                            ++out_refs;
                            remove_optimizer_symbol(fixed, physical_number);
                            if (optimizer_set_contains(allocation_call_clobbered, physical_number)) {
                                --byte_total;
                                ++byte_out_refs;
                            }
                        }
                    } else if (original->kind == 2) {
                        assigned = (C2AllocationRegister *)original;
                        physical = assigned->pinned_register;
                        if (physical) {
                            if (!final_register_has_other_use(original, record)) {
                                remove_optimizer_symbol(live, assigned->number);
                                input = record->left;
                                if (record->opcode == 1 && input->kind == 1) {
                                    CHOICE_KNOWN_KIND(input->original_register);
                                    other = (C2AllocationRegister *)input->original_register;
                                    if (other->kind == 2 && other->pinned_register == physical) continue;
                                }
                                --total;
                                ++out_refs;
                                if (physical != (C2FunctionRegister *)&machine_registers[5] &&
                            physical != (C2FunctionRegister *)&machine_registers[4])
                        CHOICE_KNOWN_NUMBER(physical);
                                physical_number = physical->number;
                                remove_optimizer_symbol(fixed, physical_number);
                                if (optimizer_set_contains(allocation_call_clobbered, physical_number)) {
                                    --byte_total;
                                    ++byte_out_refs;
                                }
                            }
                        } else if (assigned == reg) {
                            use = final_register_has_other_use(original, record);
                            if (!use) {
                                remove_optimizer_symbol(live, number);
                                --total;
                                ++out_refs;
                                alive = 0;
                                other_number = 0;
                                if (record->opcode == 0x139) max_definition_bytes = record->type & 0xfff;
                                input = record->left;
                                if (record->opcode == 1 && input->kind == 1) {
                                    CHOICE_KNOWN_KIND(input->original_register);
                                    other = (C2AllocationRegister *)input->original_register;
                                    if (other->kind == 2 && (record->type & 0xfff) == 1) {
                                        ALLOCATION_KNOWN_DISPLACEMENT(input->value.reg);
                                        ALLOCATION_KNOWN_DISPLACEMENT(operand->value.reg);
                                    }
                                    if (other->kind == 2 && ((record->type & 0xfff) != 1 ||
                                        (!(input->value.reg->displacement & 1) &&
                                         !(operand->value.reg->displacement & 1)))) {
                                        if (!other->pinned_register) {
                                            other_number = other->number;
                                            if (optimizer_set_contains(spills, other_number)) other_number = 0;
                                            if ((input->location.expression.use_flags & 0x10) || other_number)
                                                add_register_to_set(allocation_spilled_registers, other->number);
                                        } else {
                                            link = new_allocation_move();
                                            link->record = record;
                                            link->next = allocation_fixed_moves;
                                            allocation_fixed_moves = link;
                                        }
                                    }
                                }
                                merge_optimizer_symbol_sets(spills, live);
                                if (other_number) remove_optimizer_symbol(spills, other_number);
                            } else merge_optimizer_symbol_sets(spills, live);
                        } else {
                            other_number = assigned->number;
                            remove_optimizer_symbol(live, other_number);
                            if (record->opcode != 0x139) {
                                ++out_refs;
                                if (assigned->flags & 0x80) {
                                    was_byte_live = 1;
                                    ++byte_out_refs;
                                }
                            }
                            if (alive) {
                                input = record->left;
                                if (record->opcode == 1 && input->kind == 1 &&
                                    (C2AllocationRegister *)input->original_register == reg &&
                                    (record->type & 0xfff) == 1) {
                                    ALLOCATION_KNOWN_DISPLACEMENT(input->value.reg);
                                    ALLOCATION_KNOWN_DISPLACEMENT(operand->value.reg);
                                }
                                if (record->opcode == 1 && input->kind == 1 &&
                                    (C2AllocationRegister *)input->original_register == reg &&
                                    ((record->type & 0xfff) != 1 ||
                                     (!(input->value.reg->displacement & 1) &&
                                      !(operand->value.reg->displacement & 1)))) continue;
                                add_register_to_set(spills, other_number);
                            }
                        }
                    } else if (original->kind == 3) {
                        if (operand->location.expression.use_flags & 0x10) {
                            --memory_live;
                            ++out_refs;
                            if (original->view_flags & 0x80) {
                                --byte_live;
                                ++byte_out_refs;
                            }
                        }
                    } else c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0xe0d);
                } else if (operand->kind == 9) {
                    for (physical_number = next_optimizer_symbol(operand->value.registers);
                         physical_number != ~0U; physical_number = next_optimizer_symbol(NULL)) {
                        if (optimizer_set_contains(allocation_class_registers[which], physical_number)) {
                            ++out_refs;
                            if (optimizer_set_contains(allocation_call_clobbered, physical_number)) ++byte_out_refs;
                        }
                    }
                }
            }
            before_inputs = total + memory_live;
            before_byte_inputs = byte_total + byte_live;
            other_live = 0;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                original = operand->original_register;
                if (original != (C2FunctionRegister *)&machine_registers[5])
                        CHOICE_KNOWN_KIND(original);
                if (original->kind == 1) {
                    if (original != (C2FunctionRegister *)&machine_registers[5])
                        CHOICE_KNOWN_CANONICAL(original);
                    physical = original->canonical;
                    if (physical != (C2FunctionRegister *)&machine_registers[5] &&
                            physical != (C2FunctionRegister *)&machine_registers[4])
                        CHOICE_KNOWN_NUMBER(physical);
                    physical_number = physical->number;
                    if (!optimizer_set_contains(allocation_class_registers[which], physical_number)) continue;
                    CHOICE_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind != 1 &&
                        (operand->location.expression.use_flags & 0x10) &&
                        !optimizer_set_contains(fixed, physical_number)) {
                        ++total;
                        ++in_refs;
                        add_register_to_set(fixed, physical_number);
                        if (optimizer_set_contains(allocation_call_clobbered, physical_number)) {
                            ++byte_total;
                            ++byte_in_refs;
                        }
                    }
                } else if (original->kind == 2) {
                    assigned = (C2AllocationRegister *)original;
                    physical = assigned->pinned_register;
                    if (physical) {
                        if (!optimizer_set_contains(live, assigned->number)) {
                            add_register_to_set(live, assigned->number);
                            if (physical != (C2FunctionRegister *)&machine_registers[5] &&
                            physical != (C2FunctionRegister *)&machine_registers[4])
                        CHOICE_KNOWN_NUMBER(physical);
                            physical_number = physical->number;
                            if (!optimizer_set_contains(fixed, physical_number)) {
                                ++total;
                                ++in_refs;
                                add_register_to_set(fixed, physical_number);
                                if (optimizer_set_contains(allocation_call_clobbered, physical_number)) {
                                    ++byte_total;
                                    ++byte_in_refs;
                                }
                            }
                        }
                    } else if (assigned == reg) {
                        if (!optimizer_set_contains(live, number)) {
                            add_register_to_set(live, number);
                            ++total;
                            ++in_refs;
                            alive = 1;
                            output = record->tail.right;
                            if (record->opcode == 1 && output->kind == 1) {
                                if (output->original_register != (C2FunctionRegister *)&machine_registers[5])
                                CHOICE_KNOWN_KIND(output->original_register);
                                other = (C2AllocationRegister *)output->original_register;
                                if (other->kind == 2) add_register_to_set(allocation_spilled_registers, other->number);
                            }
                        }
                        output = record->tail.right;
                        constraint = operand->location.expression.use_flags & 7;
                        if (output && output->kind == 1) {
                            if (output->original_register != (C2FunctionRegister *)&machine_registers[5])
                                CHOICE_KNOWN_KIND(output->original_register);
                            other = (C2AllocationRegister *)output->original_register;
                            if (other->kind == 2 && !(other->flags & 4) && !other->pinned_register &&
                                other->definition_record == record && !allocation_input_is_repeated(operand)) {
                                link = new_allocation_move();
                                link->record = record;
                                link->next = reuse_moves;
                                reuse_moves = link;
                            }
                        }
                        if (record->opcode == 1 && !(operand->location.expression.use_flags & 0x10) &&
                            operand == record->left && output->kind == 1) {
                            if (output->original_register != (C2FunctionRegister *)&machine_registers[5])
                                CHOICE_KNOWN_KIND(output->original_register);
                            other = (C2AllocationRegister *)output->original_register;
                            if (other->kind == 1) CHOICE_KNOWN_KIND(output->value.reg);
                            if ((other->kind == 1 && output->value.reg->kind == 3) ||
                                (other->kind == 2 && other->pinned_register)) {
                                link = new_allocation_move();
                                link->record = record;
                                link->next = allocation_fixed_moves;
                                allocation_fixed_moves = link;
                            }
                        }
                        bytes = operand->type & 0xfff;
                        if (max_input_bytes < bytes) max_input_bytes = bytes;
                        /* Static machine payload+52 is not an allocation-root view. */
                        if ((unsigned long)operand->value.reg >= (unsigned long)machine_registers &&
                            (unsigned long)operand->value.reg <=
                                (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
                        if (operand->value.reg->allocation_root) {
                            ALLOCATION_KNOWN_DISPLACEMENT(operand->value.reg->allocation_root);
                            ALLOCATION_KNOWN_DISPLACEMENT(operand->value.reg);
                        }
                        if (operand->value.reg->allocation_root &&
                            operand->value.reg->allocation_root->displacement < operand->value.reg->displacement &&
                            max_input_bytes == 1) max_input_bytes = 2;
                    } else if (!allocation_input_is_repeated(operand)) {
                        CHOICE_KNOWN_KIND(operand->value.reg);
                        if (record->opcode != 0x13a && operand->value.reg->kind != 13) {
                            ++in_refs;
                            if (assigned->flags & 0x80) {
                                was_byte_live = 1;
                                ++byte_in_refs;
                            }
                        }
                        if (!(operand->location.expression.use_flags & 0x10)) ++other_live;
                        else add_register_to_set(live, assigned->number);
                    }
                } else if (original->kind == 3) {
                    if (operand->location.expression.use_flags & 0x10) {
                        ++in_refs;
                        ++memory_live;
                        if (original->view_flags & 0x80) {
                            ++byte_live;
                            ++byte_in_refs;
                        }
                    }
                } else c2_report_internal("E:\\utc\\src\\\\P2\\color.c", 0xedd);
            }
            pressure = before_inputs + (in_refs < out_refs ? out_refs : in_refs);
            byte_pressure = before_byte_inputs + (byte_in_refs < byte_out_refs ? byte_out_refs : byte_in_refs);
            while (pressure > (int)limit) {
                spilled = spill_allocation_pressure(function, which, record);
                if (spilled->view_flags & 0x80) {
                    --byte_live;
                    --byte_pressure;
                }
                --memory_live;
                --pressure;
            }
            if (pressure >= (int)limit && !(record->control_flags & 1)) {
                discard = new_register_set(6);
                for (operand = record->left; operand; operand = operand->next) {
                    if (operand->kind != 1) continue;
                    CHOICE_KNOWN_KIND(operand->value.reg);
                    if (operand->value.reg->kind != 13) continue;
                    ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                    if (ALLOCATION_TYPE_CLASS(operand->type) != which) continue;
                    assigned = (C2AllocationRegister *)operand->original_register;
                    if (assigned == reg || assigned->pinned_register) continue;
                    if (!(operand->location.expression.use_flags & 0x10)) --other_live;
                    add_register_to_set(discard, assigned->number);
                    operand = materialize_allocation_input(record, operand, operand->value.reg);
                }
                if (other_live > 0 && in_refs - out_refs < other_live) {
                    remaining = other_live - (in_refs - out_refs);
                    do {
                        best = NULL;
                        for (operand = record->left; operand; operand = operand->next) {
                            if (operand->kind != 1) continue;
                            CHOICE_KNOWN_KIND(operand->original_register);
                            assigned = (C2AllocationRegister *)operand->original_register;
                            if (assigned->kind != 2 || assigned == reg) continue;
                            ALLOCATION_KNOWN_TYPE_CLASS(operand->type);
                            if (ALLOCATION_TYPE_CLASS(operand->type) != which || assigned->pinned_register ||
                                (operand->location.expression.use_flags & 0x10)) continue;
                            if (!best || best->weighted_pressure > assigned->weighted_pressure) best = assigned;
                        }
                        add_register_to_set(discard, best->number);
                    } while (--remaining);
                }
                choice = select_allocation_spill_choice(record, live, excluded, discard);
                release_optimizer_symbol_set(discard);
                if (choice) {
                    merge_optimizer_symbol_sets(choices, choice->owner.registers);
                    block->flags |= 0x40000;
                    link = new_allocation_move();
                    link->record = record;
                    link->next = choice_moves;
                    choice_moves = link;
                }
                record->control_flags |= 1;
            }
            if (alive && was_byte_live + byte_live) {
                if (!(record->control_flags & 2)) {
                    if (byte_pressure == 3 &&
                        optimizer_symbol_sets_intersect(reg->available, allocation_call_preserved)) {
                        intersect_optimizer_symbol_sets(reg->available, allocation_call_preserved);
                    } else if (byte_pressure >= 3) {
                        choice = select_allocation_byte_spill(record, live, reg);
                        if (choice) {
                            merge_optimizer_symbol_sets(choices, choice->owner.registers);
                            block->flags |= 0x40000;
                            link = new_allocation_move();
                            link->record = record;
                            link->next = choice_moves;
                            choice_moves = link;
                        }
                        record->control_flags |= 2;
                        merge_optimizer_symbol_sets(block->allocation_region_input, allocation_call_clobbered);
                    }
                } else if (optimizer_symbol_sets_intersect(reg->available, allocation_call_preserved))
                    intersect_optimizer_symbol_sets(reg->available, allocation_call_preserved);
            }
        }
        if (block == reg->last_block) break;
    }
    release_optimizer_symbol_set(live);
    release_optimizer_symbol_set(fixed);
    if (reuse_moves) {
        allocation_current_choices = choice_moves;
        reused = finish_allocation_reuse(reuse_moves, reg);
        if (reused) {
            subtract_optimizer_symbol_sets(spills, reused);
            subtract_optimizer_symbol_sets(choices, reused);
            release_optimizer_symbol_set(reused);
        }
        release_allocation_moves(reuse_moves);
        allocation_current_choices = NULL;
    }
    subtract_optimizer_symbol_sets(spills, excluded);
    if (max_input_bytes < max_definition_bytes && max_input_bytes != 2)
        restrict_allocation_width(reg, max_input_bytes);
    if (constraint) restrict_allocation_constraint(reg, constraint);
    if (!choices->links.first) {
        release_optimizer_symbol_set(choices);
        return NULL;
    }
    if (choice_moves) {
        finish_allocation_choices(choice_moves, choices, reg);
        release_allocation_moves(choice_moves);
    }
    return choices;
}
#undef CHOICE_KNOWN_NUMBER
#undef CHOICE_KNOWN_KIND
#undef CHOICE_KNOWN_CANONICAL
void __fastcall finish_fixed_allocation_copy(C2AllocationRegister *reg,
    C2Expression *record) { __asm { int 3 } }
void __fastcall finish_coalesced_allocation_copy(C2AllocationRegister *reg,
    C2Expression *record)
{
    C2AllocationRegister *other = (C2AllocationRegister *)record->tail.right->original_register;
    C2BasicBlock *block;
    C2Expression *cursor, *operand;
    unsigned live, other_live;

    for (block = reg->first_block; ; block = block->previous) {
        if (!optimizer_set_contains(block->allocation_region_uses, reg->number)) continue;
        live = optimizer_set_contains(block->allocation_live_in_after, reg->number);
        other_live = optimizer_set_contains(block->allocation_live_in_after, other->number);
        for (cursor = (C2Expression *)block->last_record;
             cursor != (C2Expression *)block->records; cursor = cursor->previous) {
            if (!(cursor->flags & 1)) continue;
            for (operand = cursor->tail.right; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                if ((C2AllocationRegister *)operand->original_register == reg) live = 0;
                else if ((C2AllocationRegister *)operand->original_register == other) other_live = 0;
            }
            for (operand = cursor->left; operand; operand = operand->next) {
                if (operand->kind != 1 || !(operand->location.expression.use_flags & 0x10)) continue;
                if ((C2AllocationRegister *)operand->original_register == reg) {
                    live = 1;
                    if (other_live) operand->location.expression.properties |= 0x80;
                } else if ((C2AllocationRegister *)operand->original_register == other) {
                    other_live = 1;
                    if (live) operand->location.expression.properties |= 0x80;
                }
            }
        }
        if (block == reg->last_block) break;
    }
}
int allocation_register_costs[9]; /* Original4931d8..4931fc, reset by actual writer. */

/* Reached fixed address rows7/8 retain their already modeled kind1 and
 * canonical self columns; no other original static payload is claimed. */
#define UPDATE_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8]) ALLOCATION_KNOWN_KIND(reg); } while (0)
#define UPDATE_KNOWN_CANONICAL(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8]) ALLOCATION_KNOWN_CANONICAL(reg); } while (0)
/* The actual fixed preference order includes already modeled rows4/6. */
#define UPDATE_KNOWN_NUMBER(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6]) ALLOCATION_KNOWN_NUMBER(reg); } while (0)
void __fastcall update_allocation_choices(C2AllocationRegister *reg, int which,
    C2RegisterSet *spills)
{
    /* Integer preference order at49b4a8, terminated by0; the float list at
     *49e2c8 is empty. Only these two selector rows are represented. */
    static const unsigned integer_order[] = {1, 2, 3, 7, 8, 4, 6, 0};
    C2AllocationRegister *other, *source;
    C2AllocationPreference *preference;
    C2AllocationMoveLink *move;
    C2BasicBlock *block;
    C2Expression *record;
    C2FunctionRegister *physical, *original;
    C2RegisterSet *allowed;
    unsigned number, selected = 0, index;
    int priority, best_cost;

    memset(allocation_register_costs, 0, sizeof(allocation_register_costs));
    for (number = next_optimizer_symbol(spills); number != ~0U;
         number = next_optimizer_symbol(NULL)) {
        other = lookup_allocation_register(number);
        priority = other->priority;
        if (priority <= 0) continue;
        allowed = other->available;
        for (preference = other->preferences; preference; preference = preference->next) {
            if (preference->physical != (C2FunctionRegister *)&machine_registers[4])
                UPDATE_KNOWN_NUMBER(preference->physical);
            index = preference->physical->number;
            if (optimizer_set_contains(allowed, index)) {
                if (index > 8) { __asm { int 3 } }
                allocation_register_costs[index] += preference->count;
            }
        }
        if (count_optimizer_symbols(allowed) == 1) {
            for (index = 0; !optimizer_set_contains(allowed, index); ++index) {
            }
            if (index > 8) { __asm { int 3 } }
            allocation_register_costs[index] += priority * 100;
        }
    }
    for (preference = reg->preferences; preference; preference = preference->next) {
        if (preference->physical != (C2FunctionRegister *)&machine_registers[4])
                UPDATE_KNOWN_NUMBER(preference->physical);
        index = preference->physical->number;
        if (index > 8) { __asm { int 3 } }
        allocation_register_costs[index] -= preference->count;
    }
    if (which != 0 && which != 1) { __asm { int 3 } }
    if (!which) {
        for (index = 0; integer_order[index]; ++index) {
            number = integer_order[index];
            if (optimizer_set_contains(reg->available, number) &&
                (!selected || allocation_register_costs[number] < best_cost)) {
                selected = number;
                best_cost = allocation_register_costs[number];
            }
        }
    }
    /* Chosen row address is passed/stored; its unmodeled payload is not read. */
    reg->pinned_register = (C2FunctionRegister *)&machine_registers[selected];
    release_optimizer_symbol_set(reg->available);
    reg->available = NULL;
    for (block = reg->first_block; ; block = block->previous) {
        if (!optimizer_set_contains(block->allocation_region_uses, reg->number)) continue;
        add_register_to_set(block->allocation_region_input, selected);
        if (optimizer_set_contains(block->allocation_live_out, reg->number))
            add_register_to_set(block->allocation_edge_uses, selected);
        if (optimizer_set_contains(block->allocation_live_in_after, reg->number))
            add_register_to_set(block->allocation_edge_definitions, selected);
        if (block == reg->last_block) break;
    }
    if (allocation_fixed_moves) {
        for (move = allocation_fixed_moves; move; move = move->next) {
            record = move->record;
            original = record->tail.right->original_register;
            UPDATE_KNOWN_KIND(original);
            if (original->kind == 1) {
                UPDATE_KNOWN_CANONICAL(original);
                physical = original->canonical;
                if (physical != (C2FunctionRegister *)&machine_registers[6])
                    UPDATE_KNOWN_NUMBER(physical);
                if (physical->number == selected) finish_fixed_allocation_copy(reg, record);
            } else {
                other = (C2AllocationRegister *)original;
                source = (C2AllocationRegister *)record->left->original_register;
                if (source == reg) physical = other->pinned_register;
                else physical = source->pinned_register;
                if (physical != (C2FunctionRegister *)&machine_registers[6])
                    UPDATE_KNOWN_NUMBER(physical);
                if (physical->number == selected) finish_coalesced_allocation_copy(source, record);
            }
        }
        release_allocation_moves(allocation_fixed_moves);
        allocation_fixed_moves = NULL;
    }
    for (number = next_optimizer_symbol(allocation_spilled_registers); number != ~0U;
         number = next_optimizer_symbol(NULL))
        coalesce_fixed_register(lookup_allocation_register(number), reg->pinned_register);
    clear_register_set(allocation_spilled_registers);
}
#undef UPDATE_KNOWN_NUMBER
#undef UPDATE_KNOWN_KIND
#undef UPDATE_KNOWN_CANONICAL
int __fastcall allocation_can_be_deferred(C2AllocationRegister *reg)
{ __asm { int 3 } return 0; }
int __fastcall allocation_definition_can_be_reused(C2Expression *record, int spilling)
{
    C2Expression *operand;
    C2AllocationRegister *reg;

    if (record->opcode == 0x139 || (record->control_flags & 2)) return 0;
    if (record->tail.right->next) return 0;
    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
            ALLOCATION_KNOWN_KIND(operand->original_register);
        reg = (C2AllocationRegister *)operand->original_register;
        if (reg->kind != 2 || (operand->location.expression.use_flags & 0x10)) return 0;
        if (spilling) {
            ALLOCATION_KNOWN_KIND(operand->value.reg);
            if (operand->value.reg->kind != 13 && !reg->pinned_register &&
                reg->available->links.first) return 0;
        }
        if (reg->flags & 0x10) return 0;
    }
    return 1;
}
void __fastcall prepare_reused_allocation(C2AllocationRegister *reg)
{
    C2Expression *definition = reg->definition_record, *memory = NULL;
    C2Expression *operand, *record, *copy, *scan;
    C2AllocationRegister *source;
    C2RegisterSet *dependencies = NULL, *missing;
    C2BasicBlock *block, *first = reg->first_block, *last = reg->last_block;
    C2BasicBlock *special = NULL;
    C2FlowEdge *edge;
    C2AllocationMoveLink *moves = NULL, *link, *move;
    unsigned unpinned = 0;
    int changed, active, source_live;

    for (operand = definition->left; operand; operand = operand->next) {
        if (operand->kind == 1) {
            ALLOCATION_KNOWN_KIND(operand->value.reg);
            if (operand->value.reg->kind == 13) continue;
            if (!dependencies) dependencies = new_register_set(6);
            source = (C2AllocationRegister *)operand->original_register;
            add_register_to_set(dependencies, source->number);
            if (!source->pinned_register) {
                if (source->flags & 0x80) goto abandon;
                ++unpinned;
            }
        } else if (operand->kind == 2 || operand->kind == 6) memory = operand;
    }
    if (unpinned > 1) goto abandon;
    if (first == last) first->flags &= ~0x100000;
    else {
        for (block = first;; block = block->previous) {
            block->flags &= 0xff2fffffU;
            for (record = (C2Expression *)block->records;
                 record != (C2Expression *)block->last_record; record = record->next) {
                if (record->flags & 1) {
                    if (!(block->flags & 0x400000) &&
                        (find_final_register_use((C2FunctionRegister *)reg, record) ||
                         (!(block->flags & 0x400000) &&
                          final_register_has_other_use((C2FunctionRegister *)reg, record))))
                        block->flags |= 0x400000;
                } else if (record->kind == 26 &&
                    optimizer_set_contains(record->value.registers, reg->number))
                    block->flags = (block->flags & ~0x400000) | 0x800000;
            }
            if (block->flags & 0x400000) block->flags |= 0x200000;
            else block->flags &= ~0x200000;
            if (block == last) break;
        }
        do {
            changed = 0;
            for (block = first->previous;; block = block->previous) {
                if (!(block->flags & 0x100000)) {
                    for (edge = block->incoming; edge; edge = edge->next_incoming)
                        if (!(edge->source->flags & 0x200000)) break;
                    if (!edge) {
                        block->flags |= 0x100000;
                        if (!(block->flags & 0x800000)) {
                            block->flags |= 0x200000;
                            changed = 1;
                        }
                    }
                }
                if (block == last) break;
            }
        } while (changed);
    }
    for (block = first;; block = block->previous) {
        active = (block->flags & 0x100000) != 0;
        source_live = optimizer_set_contains(block->allocation_live_out, reg->number) != 0;
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) {
                if (record->kind == 26) {
                    if (optimizer_set_contains(record->value.registers, reg->number)) active = 0;
                    else if (source_live && dependencies &&
                        optimizer_symbol_sets_intersect(record->value.registers, dependencies)) goto abandon;
                }
                continue;
            }
            if (!active) {
                if (find_final_register_use((C2FunctionRegister *)reg, record)) {
                    active = source_live = 1;
                    continue;
                }
                for (operand = record->left; operand; operand = operand->next) {
                    if (operand->kind != 1 ||
                        operand->original_register != (C2FunctionRegister *)reg) continue;
                    if (dependencies && optimizer_set_contains(block->allocation_live_out, reg->number)) {
                        missing = copy_optimizer_symbol_set(dependencies);
                        subtract_optimizer_symbol_sets(missing, block->allocation_live_out);
                        if (missing->links.first) {
                            scan = record->tail.right;
                            if (special || record != ((C2Expression *)block->records)->next ||
                                block->incoming->next_incoming ||
                                !optimizer_symbol_sets_intersect(block->incoming->source->allocation_live_in_after,
                                    dependencies) || count_optimizer_symbols(dependencies) != 1 ||
                                !scan || scan->kind != 1) {
                                release_optimizer_symbol_set(missing);
                                goto abandon;
                            }
                            ALLOCATION_KNOWN_KIND(scan->original_register);
                            if (scan->original_register->kind != 2 ||
                                !optimizer_set_contains(dependencies, scan->original_register->number)) {
                                release_optimizer_symbol_set(missing);
                                goto abandon;
                            }
                            special = block;
                        }
                        release_optimizer_symbol_set(missing);
                    }
                    move = new_allocation_move();
                    move->record = record;
                    move->next = moves;
                    moves = move;
                    if (operand->location.expression.use_flags & 0x10) source_live = 0;
                    else active = 1;
                }
            }
            if (!source_live) continue;
            if (dependencies) {
                for (operand = record->tail.right; operand; operand = operand->next) {
                    if (operand->kind != 1) continue;
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
                        ALLOCATION_KNOWN_KIND(operand->original_register);
                    if (operand->original_register->kind == 2 &&
                        optimizer_set_contains(dependencies, operand->original_register->number)) goto abandon;
                }
            }
            if (memory)
                for (operand = record->tail.right; operand; operand = operand->next)
                    if (input_operands_may_alias(operand, memory)) goto abandon;
            for (operand = record->left; operand; operand = operand->next)
                if (operand->kind == 1 && operand->original_register == (C2FunctionRegister *)reg &&
                    (operand->location.expression.use_flags & 0x10)) active = source_live = 0;
        }
        if (block == last) break;
    }
    if (!moves) goto abandon;
    if (dependencies) {
        for (link = moves; link; link = link->next) {
            for (record = link->record->previous; record->kind != 24; record = record->previous) {
                if (!(record->flags & 1)) continue;
                for (operand = record->left; operand; operand = operand->next) {
                    if (operand->kind != 1) continue;
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
                        ALLOCATION_KNOWN_KIND(operand->original_register);
                    if (operand->original_register->kind == 2 &&
                        (operand->location.expression.use_flags & 0x10) &&
                        optimizer_set_contains(dependencies, operand->original_register->number)) goto abandon;
                }
            }
        }
        release_optimizer_symbol_set(dependencies);
        dependencies = NULL;
    }
    for (link = moves; link; link = link->next) {
        copy = copy_input_expression(definition);
        copy->location.line = link->record->location.line;
        for (operand = copy->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            ALLOCATION_KNOWN_KIND(operand->value.reg);
            if (operand->value.reg->kind == 13)
                operand = materialize_allocation_input(copy, operand, operand->value.reg);
        }
        prepend_input_record((C2Record *)link->record, (C2Record *)copy);
        allocation_block_for_record(link->record)->flags |= 0x40000;
    }
    release_allocation_moves(moves);
    moves = NULL;
    reg->flags |= 0x10;
    if (special) {
        for (operand = definition->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            if (operand->original_register != (C2FunctionRegister *)&machine_registers[5])
                        ALLOCATION_KNOWN_KIND(operand->original_register);
            ALLOCATION_KNOWN_KIND(operand->value.reg);
            if (operand->original_register->kind == 2 && operand->value.reg->kind != 13) break;
        }
        /* The original assumes a suitable definition input exists here and
         * dereferences NULL if that invariant is broken. */
        source = (C2AllocationRegister *)operand->original_register;
        add_register_to_set(special->allocation_live_out, source->number);
        if (source->pinned_register) {
            ALLOCATION_KNOWN_NUMBER(source->pinned_register);
            add_register_to_set(special->allocation_edge_uses, source->pinned_register->number);
        }
        for (operand = ((C2Expression *)special->records)->next->left;
             operand; operand = operand->next)
            if (operand->kind == 1 && operand->original_register == (C2FunctionRegister *)source) {
                operand->location.expression.use_flags |= 0x10;
                break;
            }
    }
    return;
abandon:
    if (dependencies) release_optimizer_symbol_set(dependencies);
    if (moves) release_allocation_moves(moves);
}
void __fastcall assign_block_spill_register(C2BasicBlock *block,
    C2AllocationRegister *reg, C2RegisterSet *allowed) { __asm { int 3 } }

void __fastcall assign_allocation_spill_registers(C2Function *function,
    C2RegisterSet *spills, C2RegisterSet *allowed)
{
    C2AllocationMoveLink *groups = NULL, *group, *created;
    C2BasicBlock *block, *work = NULL, *next, *neighbor;
    C2FlowEdge *edge;
    C2RegisterSet *missing, *scratch;
    C2Record *position;
    unsigned number, depth = 0, level, identity;
    int total, same;

    if (count_optimizer_symbols(spills) < 3 || (int)allocation_block_count < 100) {
        for (number = next_optimizer_symbol(spills); number != ~0U; number = next_optimizer_symbol(NULL))
            force_allocation_register(lookup_allocation_register(number), allowed);
        return;
    }
    missing = new_register_set(6);
    scratch = new_register_set(6);
    for (block = function->flow->last; block; block = block->previous) {
        if (!optimizer_symbol_sets_intersect(block->allocation_region_uses, spills)) continue;
        block->spill_partition_next = work;
        work = block;
        copy_optimizer_symbol_set_into(missing, allowed);
        subtract_optimizer_symbol_sets(missing, block->allocation_region_input);
        if (!missing->links.first) {
            for (number = next_optimizer_symbol(spills); number != ~0U; number = next_optimizer_symbol(NULL))
                assign_block_spill_register(block, lookup_allocation_register(number), allowed);
            continue;
        }
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            identity = edge->source->spill_partition;
            if (!identity) continue;
            group = groups;
            for (level = depth; level != identity; --level) group = group->next;
            copy_optimizer_symbol_set_into(scratch, missing);
            subtract_optimizer_symbol_sets(scratch, group->registers);
            if (scratch->links.first) goto merge_spill_partition;
        }
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            identity = edge->destination->spill_partition;
            if (!identity) continue;
            group = groups;
            for (level = depth; level != identity; --level) group = group->next;
            copy_optimizer_symbol_set_into(scratch, missing);
            subtract_optimizer_symbol_sets(scratch, group->registers);
            if (scratch->links.first) goto merge_spill_partition;
        }
        created = new_allocation_move();
        created->registers = copy_optimizer_symbol_set(block->allocation_region_input);
        created->next = groups;
        groups = created;
        block->spill_partition = ++depth;
        continue;
merge_spill_partition:
        block->spill_partition = identity;
        merge_optimizer_symbol_sets(group->registers, block->allocation_region_input);
    }
    for (block = work; block; block = block->spill_partition_next) {
        if (!block->spill_partition) {
            position = block->records->next;
            if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
            else select_allocation_spill_choice((C2Expression *)block->records, NULL, NULL, spills);
            position = block->last_record->previous;
            if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
            else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, spills);
            block->flags |= 0x40000;
            continue;
        }
        total = same = 0;
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            ++total;
            if (edge->source->spill_partition == block->spill_partition) ++same;
        }
        if (same < total) {
            if (!same) {
                position = block->records->next;
                if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
                else select_allocation_spill_choice((C2Expression *)block->records, NULL, NULL, spills);
                block->flags |= 0x40000;
            } else for (edge = block->incoming; edge; edge = edge->next_incoming) {
                neighbor = edge->source;
                if (neighbor->spill_partition == block->spill_partition) continue;
                position = neighbor->last_record->previous;
                if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
                else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, spills);
                neighbor->flags |= 0x40000;
            }
        }
        total = same = 0;
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            ++total;
            if (edge->destination->spill_partition == block->spill_partition) ++same;
        }
        if (same < total) {
            if (!same) {
                position = block->last_record->previous;
                if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
                else select_allocation_spill_choice((C2Expression *)position, NULL, NULL, spills);
                block->flags |= 0x40000;
            } else for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
                neighbor = edge->destination;
                if (neighbor->spill_partition == block->spill_partition) continue;
                position = neighbor->records->next;
                if (position->kind == 26) merge_optimizer_symbol_sets(position->owner.registers, spills);
                else select_allocation_spill_choice((C2Expression *)neighbor->records, NULL, NULL, spills);
                neighbor->flags |= 0x40000;
            }
        }
    }
    while (work) {
        next = work->spill_partition_next;
        work->spill_partition = 0;
        work->spill_partition_next = NULL;
        work = next;
    }
    release_optimizer_symbol_set(missing);
    release_optimizer_symbol_set(scratch);
    if (groups) release_allocation_moves(groups);
}

C2RegisterSet *__fastcall finish_allocation_spill(C2Function *function, int which,
    C2AllocationRegister *reg, C2RegisterSet *spills, C2RegisterSet *selection)
{
    C2RegisterSet *needed = NULL, *byte_needed = NULL, *deferred = NULL, *allowed;
    C2AllocationRegister *other;
    unsigned number, physical_number;

    if (spills->links.first) {
        /* Reached rows4/6 have their independently modeled DWORD numbers. */
        if (reg->pinned_register != (C2FunctionRegister *)&machine_registers[4] &&
            reg->pinned_register != (C2FunctionRegister *)&machine_registers[6])
            ALLOCATION_KNOWN_NUMBER(reg->pinned_register);
        physical_number = reg->pinned_register->number;
        for (number = next_optimizer_symbol(spills); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            other = lookup_allocation_register(number);
            remove_optimizer_symbol(other->available, physical_number);
            if (other->available->links.first) continue;
            if (!(other->flags & 4) && allocation_can_be_deferred(other)) {
                if (selection) remove_optimizer_symbol(selection, number);
            } else {
                if (!needed) needed = new_register_set(6);
                add_register_to_set(needed, number);
                if (other->flags & 0x80) {
                    ALLOCATION_KNOWN_KIND(other->source);
                    if (other->source->kind == 13) prepare_clobbered_allocation(other);
                    else {
                        if (!byte_needed) byte_needed = new_register_set(6);
                        add_register_to_set(byte_needed, number);
                    }
                }
            }
        }
    }
    release_optimizer_symbol_set(spills);
    if (selection) {
        for (number = next_optimizer_symbol(selection); number != ~0U;
             number = next_optimizer_symbol(NULL)) {
            other = lookup_allocation_register(number);
            other->flags |= 2;
            if (!(other->flags & 4) && allocation_can_be_deferred(other)) {
                if (!deferred) deferred = new_register_set(6);
                add_register_to_set(deferred, number);
                continue;
            }
            if (!(other->flags & 8) &&
                allocation_definition_can_be_reused(other->definition_record, 0) &&
                other->available->links.first) prepare_reused_allocation(other);
        }
        if (deferred) {
            subtract_optimizer_symbol_sets(selection, deferred);
            release_optimizer_symbol_set(deferred);
        }
    }
    if (needed) {
        if (byte_needed) {
            assign_allocation_spill_registers(function, byte_needed, allocation_call_clobbered);
            for (number = next_optimizer_symbol(byte_needed); number != ~0U;
                 number = next_optimizer_symbol(NULL)) {
                other = lookup_allocation_register(number);
                other->flags |= 2;
                if (!(other->flags & 8) &&
                    allocation_definition_can_be_reused(other->definition_record, 1))
                    prepare_reused_allocation(other);
                if (!selection) selection = new_register_set(6);
                add_register_to_set(selection, number);
            }
            subtract_optimizer_symbol_sets(needed, byte_needed);
            release_optimizer_symbol_set(byte_needed);
        }
        if (needed->links.first) {
            allowed = copy_optimizer_symbol_set(allocation_class_registers[which]);
            if (!allocation_frame_register_available) remove_optimizer_symbol(allowed, 6);
            assign_allocation_spill_registers(function, needed, allowed);
            release_optimizer_symbol_set(allowed);
            for (number = next_optimizer_symbol(needed); number != ~0U;
                 number = next_optimizer_symbol(NULL)) {
                other = lookup_allocation_register(number);
                other->flags |= 2;
                if (!(other->flags & 8) &&
                    allocation_definition_can_be_reused(other->definition_record, 1))
                    prepare_reused_allocation(other);
                if (!selection) selection = new_register_set(6);
                add_register_to_set(selection, number);
            }
        }
        release_optimizer_symbol_set(needed);
    }
    if (selection && !selection->links.first) {
        release_optimizer_symbol_set(selection);
        return NULL;
    }
    return selection;
}
void __fastcall lower_allocation_pseudo_record(C2Expression *record)
{
    record->opcode = 1;
}
int __fastcall reuse_region_allocation(C2FlowRegion *region, C2Expression *record,
    C2FunctionRegister *source, C2Function *function) { __asm { int 3 } return 0; }

void __fastcall finish_machine_register_allocation(C2Function *function)
{
    C2BasicBlock *block;
    C2FlowRegion *region;
    C2Expression *record, *next, *operand;
    C2FunctionRegister *source, *root;
    C2AllocationRegister *assigned;
    long displacement;
    int side;

    for (block = function->flow->last; block; block = block->previous) {
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = next) {
            next = record->next;
            if (!(record->flags & 1)) {
                if (record->kind == 0x1a) unlink_and_release_input_record((C2Record *)record);
                continue;
            }
            if (record->opcode == 0x139) lower_allocation_pseudo_record(record);
            else if (record->opcode == 0x13a) {
                operand = record->tail.right;
                source = operand->value.reg;
                region = block->block_scope;
                if (region && (unsigned long)source >= (unsigned long)machine_registers &&
                    (unsigned long)source <=
                        (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
                if (region && source->allocation &&
                    !optimizer_set_contains(region->allocation_eligible, source->allocation->number) &&
                    (c2_favor_speed || !region->exits->next) &&
                    !(operand->location.expression.use_flags & 8) &&
                    reuse_region_allocation(region, record, source, function)) {
                    unlink_and_release_input_record((C2Record *)record);
                    continue;
                }
                lower_allocation_pseudo_record(record);
            }
            for (side = 0; side < 2; ++side) {
                operand = side ? record->left : record->tail.right;
                for (; operand; operand = operand->next) {
                    if (operand->kind != 1) continue;
                    /* These already modeled physical and flag rows have kind 1. */
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[4] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[5] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[6] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[7] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[8] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[35] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[74] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[68] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[69] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[70] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[71] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[72] &&
                        operand->original_register != (C2FunctionRegister *)&machine_registers[73])
                        ALLOCATION_KNOWN_KIND(operand->original_register);
                    if (operand->original_register->kind != 2) continue;
                    assigned = (C2AllocationRegister *)operand->original_register;
                    source = operand->value.reg;
                    /* Static rows have no allocation-root payload view. */
                    if ((unsigned long)source >= (unsigned long)machine_registers &&
                        (unsigned long)source <=
                            (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT)) { __asm { int 3 } }
                    root = source->allocation_root;
                    displacement = 0;
                    if (root) {
                        ALLOCATION_KNOWN_DISPLACEMENT(source);
                        ALLOCATION_KNOWN_DISPLACEMENT(root);
                        displacement = (long)((unsigned)(source->displacement - root->displacement) << 3);
                    }
                    operand->original_register = new_bitfield_register(assigned->pinned_register,
                        (operand->type & 0xfff) << 3, displacement);
                }
            }
        }
    }
}

void __fastcall assign_machine_registers(C2Function *function)
{
    C2RegisterSet *registers, *spills, *available, *excluded, *choices, *selection;
    C2AllocationRegister *reg, *other;
    C2RegisterSet *forced;
    int which, is_constant;
    unsigned char flags;

    if (!allocation_integer_registers->links.first && !allocation_float_registers->links.first) {
        reset_input_pool(4);
        reset_input_pool(6);
        allocation_is_empty = 1;
        finish_register_moves(function);
        return;
    }
    allocation_is_empty = 0;
    free_allocation_spill_blocks = NULL;
    free_allocation_spill_uses = NULL;
    free_allocation_moves = NULL;
    free_allocation_spill_classes = NULL;
    allocation_block_count = 0;
    active_allocation_registers = NULL;
    allocation_spilled_registers = new_register_set(6);
    coalesce_single_use_registers(function, allocation_register_classes);
    build_allocation_preferences(function, allocation_integer_registers);
    initialize_allocation_work_sets(function);
    if (allocation_float_registers->links.first ||
        ((function->flags & 8) && allocation_region_chain && !c2_float_precision))
        allocate_floating_function(function);
    else finish_register_moves(function);
    plan_function_frame_pointer(function);
    allocation_class_limits[0] = allocation_integer_limit(function);
    for (which = 1; which >= 0; --which) {
        registers = allocation_register_classes[which];
        if (!registers->links.first) continue;
        spills = select_allocation_spills(function, which, registers, allocation_class_limits[which]);
        coalesce_allocation_registers(function, which, registers);
        prune_allocation_registers(function, which, registers, spills);
        if (spills && !spills->links.first) {
            release_optimizer_symbol_set(spills);
            spills = NULL;
        }
        available = copy_optimizer_symbol_set(registers);
        clear_register_set(registers);
        spills = order_allocation_work(function, which, spills);
        excluded = new_register_set(6);
        while (active_allocation_registers) {
            if (spills) {
                active_allocation_registers = sort_allocation_work(active_allocation_registers);
                apply_allocation_spills(function, spills, which);
                spills = NULL;
                merge_optimizer_symbol_sets(available, registers);
                coalesce_allocation_registers(function, which, registers);
                prune_allocation_registers(function, which, registers, NULL);
                active_allocation_registers = merge_allocation_work(registers, active_allocation_registers);
                clear_register_set(registers);
                if (!active_allocation_registers) break;
                if (allocation_deferred_registers) {
                    spills = allocation_deferred_registers;
                    allocation_deferred_registers = NULL;
                    continue;
                }
            }
            reg = active_allocation_registers;
            active_allocation_registers = reg->next_active;
            if (active_allocation_registers) active_allocation_registers->previous_active = NULL;
            if (reg->priority <= 0 &&
                (reg->priority != 0 || reg->degree != reg->remaining_degree ||
                 reg->source->kind == 13 || reg->use_count <= 2 || !c2_favor_speed)) {
                if (!(reg->attributes & 1) && allocation_can_coalesce(reg)) {
                    reg->next_active = active_allocation_registers;
                    if (active_allocation_registers) active_allocation_registers->previous_active = reg;
                    active_allocation_registers = reg;
                } else if (!(reg->flags & 0x40) && active_allocation_registers) {
                    reg->flags |= 0x40;
                    active_allocation_registers = delay_allocation_register(active_allocation_registers, reg);
                } else if (!(reg->attributes & 1) &&
                    (reg->degree == reg->remaining_degree || !reg->coalesce_rank)) {
                    spill_allocation_register(function, reg);
                    refresh_allocation_work(function, which);
                } else {
                    if (reg->attributes & 1) relax_pinned_allocation(reg);
                    else relax_allocation_candidate(reg);
                    reg->next_active = active_allocation_registers;
                    if (active_allocation_registers) active_allocation_registers->previous_active = reg;
                    active_allocation_registers = reg;
                    spills = new_register_set(6);
                    add_register_to_set(spills, reg->number);
                    reg->flags |= 2;
                    is_constant = reg->source->kind == 13;
                    for (other = reg->next_active; other; other = other->next_active) {
                        if (other->priority <= 0 &&
                            ((other->attributes & 1) ||
                             (other->degree != other->remaining_degree && other->coalesce_rank)) &&
                            is_constant == (other->source->kind == 13) && (other->flags & 0x40)) {
                            if (other->attributes & 1) relax_pinned_allocation(other);
                            else relax_allocation_candidate(other);
                            add_register_to_set(spills, other->number);
                            other->flags |= 2;
                        }
                    }
                }
            } else if (!(reg->flags & 4) && try_assign_allocation_register(reg)) {
                if (reg->available)
                    active_allocation_registers = insert_allocation_work(reg, active_allocation_registers);
            } else if (!reg->available->links.first) {
                forced = copy_optimizer_symbol_set(allocation_class_registers[which]);
                if (!allocation_frame_register_available) remove_optimizer_symbol(forced, 6);
                flags = reg->flags;
                reg->flags |= 2;
                spills = new_register_set(6);
                add_register_to_set(spills, reg->number);
                if (flags & 0x80) {
                    if (reg->source->kind == 13) prepare_clobbered_allocation(reg);
                    force_allocation_register(reg, allocation_call_clobbered);
                } else force_allocation_register(reg, forced);
                release_optimizer_symbol_set(forced);
                if (!active_allocation_registers) active_allocation_registers = reg;
            } else {
                add_register_to_set(excluded, reg->number);
                spills = new_register_set(6);
                selection = choose_allocation_spill(function, reg, which, available,
                    spills, excluded, allocation_class_limits[which]);
                update_allocation_choices(reg, which, spills);
                spills = finish_allocation_spill(function, which, reg, spills, selection);
            }
        }
        release_optimizer_symbol_set(excluded);
        release_optimizer_symbol_set(available);
    }
    finish_machine_register_allocation(function);
    reset_input_pool(4);
    reset_input_pool(6);
}

/* Original491100: the terminated physical-register preference order. Element6
 * is the frame-register choice, overwritten by the real pass before use.
 * The cursor shares this array identity; it is not separate copied storage. */
unsigned spill_register_order[8] = {1, 2, 3, 7, 8, 4, 6, 0};
unsigned *spill_register_cursor; /* Original491120; assigned before consumption. */
C2RegisterSet *spill_register_sets[8]; /* Original491144..491164, all eight written here. */
C2FunctionRegister *spill_register_values[9]; /* Original491180..4911a4; actual36B clear. */
extern int c2_global_optimize;
/* Kind-only admission does not authorize canonical/number payload reads. */
#define SPILL_PREPARE_KNOWN_KIND(reg) \
    (SPILL_PREPARE_KNOWN_VIEW(reg) || \
     (reg) == (C2FunctionRegister *)&machine_registers[33] || \
     (reg) == (C2FunctionRegister *)&machine_registers[34] || \
     (reg) == (C2FunctionRegister *)&machine_registers[35])

void __fastcall prepare_spill_register_allocation(C2Function *function);
void __fastcall optimize_spill_float_order(C2Function *function);
void __fastcall assign_local_spill_registers(C2Function *function) { __asm { int 3 } }
void __fastcall lower_spilled_record_inputs(C2Expression *record);
void __fastcall lower_spilled_record_outputs(C2Expression *record);
void optimize_region_spill_moves(void);
void __fastcall finish_spill_registers(C2Function *function);

void __fastcall rewrite_spilled_function_registers(C2Function *function)
{
    C2Expression *record;
    unsigned i;

    memset(spill_register_values, 0, sizeof(spill_register_values));
    for (i = 0; i < 8; ++i) spill_register_sets[i] = new_register_set(6);
    if (c2_global_optimize) {
        spill_register_order[6] = allocation_frame_register_available ? 6 : 0;
        if (!allocation_is_empty) {
            prepare_spill_register_allocation(function);
            optimize_spill_float_order(function);
        } else assign_local_spill_registers(function);
    } else {
        spill_register_order[6] = 0;
        assign_local_spill_registers(function);
    }
    spill_register_cursor = spill_register_order;
    for (record = (C2Expression *)function->flow->last->records; record; record = record->next) {
        if (!(record->flags & 1)) continue;
        if (record->location.line) {
            record_line_delta = record->location.line;
            record_source_line = function->source_line_base + record->location.line;
        }
        lower_spilled_record_inputs(record);
        lower_spilled_record_outputs(record);
    }
    if (c2_global_optimize && !allocation_is_empty) {
        optimize_region_spill_moves();
        finish_spill_registers(function);
    }
    reset_input_pool(6);
}

void initialize_spill_register_costs(void)
{
    unsigned i = 0, number;

    /* The real preference order contains7, then the remaining choices and
     * its zero terminator. Only the actual writes define these cost fields. */
    while (spill_register_order[i] != 7) {
        number = spill_register_order[i++];
        if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
        machine_registers[number].spill_cost = 0x1000000;
    }
    while ((number = spill_register_order[i++]) != 0) {
        if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
        machine_registers[number].spill_cost = 0;
    }
}

/* The eight pointer cells begin at491144; machine numbers1..8 index them
 * relative to491140. Neither the preceding cell nor other indices is modeled. */
#define SPILL_SET_INDEX(number) \
    do { if ((number) < 1 || (number) > 8) { __asm { int 3 } } } while (0)
#define SPILL_MACHINE_INDEX(number) \
    do { if ((number) >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } } } while (0)

#define SPILL_KNOWN_CANONICAL(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[67] && \
        (reg) != (C2FunctionRegister *)&machine_registers[5] && \
        (reg) != (C2FunctionRegister *)&machine_registers[19] && \
        (reg) != (C2FunctionRegister *)&machine_registers[68] && \
        (reg) != (C2FunctionRegister *)&machine_registers[69] && \
        (reg) != (C2FunctionRegister *)&machine_registers[70] && \
        (reg) != (C2FunctionRegister *)&machine_registers[71] && \
        (reg) != (C2FunctionRegister *)&machine_registers[72] && \
        (reg) != (C2FunctionRegister *)&machine_registers[73]) { __asm { int 3 } } } while (0)
#define SPILL_KNOWN_NUMBER(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
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
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[5]) { __asm { int 3 } } } while (0)



/* This pass consumes the already modeled kind/canonical/number columns.
 * Admission here does not expose any other payload of these static rows. */
#define SPILL_PREPARE_KNOWN_VIEW(reg) \
    ((reg) == (C2FunctionRegister *)&machine_registers[4] || \
     (reg) == (C2FunctionRegister *)&machine_registers[5] || \
     (reg) == (C2FunctionRegister *)&machine_registers[6] || \
     (reg) == (C2FunctionRegister *)&machine_registers[7] || \
     (reg) == (C2FunctionRegister *)&machine_registers[8] || \
     (reg) == (C2FunctionRegister *)&machine_registers[9] || \
     (reg) == (C2FunctionRegister *)&machine_registers[31] || \
     (reg) == (C2FunctionRegister *)&machine_registers[32] || \
     (reg) == (C2FunctionRegister *)&machine_registers[74] || \
     (reg) == (C2FunctionRegister *)&machine_registers[68] || \
     (reg) == (C2FunctionRegister *)&machine_registers[69] || \
     (reg) == (C2FunctionRegister *)&machine_registers[70] || \
     (reg) == (C2FunctionRegister *)&machine_registers[71] || \
     (reg) == (C2FunctionRegister *)&machine_registers[72] || \
     (reg) == (C2FunctionRegister *)&machine_registers[73])
void __fastcall prepare_spill_register_allocation(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *record, *operand, *other;
    C2FunctionRegister *source, *reg, *root;
    C2RegisterSet *live, *values;
    unsigned number, member, cost;
    unsigned char flags;

    initialize_spill_register_costs();
    live = new_register_set(6);
    values = new_register_set(6);
    for (block = function->flow->last; block; block = block->previous) {
        copy_optimizer_symbol_set_into(live, block->allocation_edge_definitions);
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record->location.line;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    source = operand->value.reg;
                    if (!SPILL_PREPARE_KNOWN_KIND(source)) ALLOCATION_KNOWN_KIND(source);
                    if (source->kind == 3 && source->value) {
                        reg = operand->original_register;
                        if (!SPILL_PREPARE_KNOWN_KIND(reg)) ALLOCATION_KNOWN_KIND(reg);
                        if (reg->kind == 1) {
                            if (reg != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(reg)) SPILL_KNOWN_CANONICAL(reg);
                            root = reg->canonical;
                            root->spill_cost = 0x1000000;
                            if (!final_register_has_other_use(reg, record)) {
                                if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                                number = root->number;
                                if (number && number < 9 && number != 5) {
                                    remove_optimizer_symbol(live, number);
                                    machine_registers[number].flags |= 0x10;
                                }
                            }
                        } else {
                            if ((operand->type & 0xfff) == 1) {
                                number = reg->number;
                                add_register_to_set(spill_register_sets[6], number);
                                add_register_to_set(spill_register_sets[7], number);
                                add_register_to_set(spill_register_sets[5], number);
                                reg->view_flags |= 0x80;
                            }
                            if (!final_register_has_other_use(reg, record)) {
                                remove_optimizer_symbol(values, reg->number);
                                if (record->opcode == 1 && record->left->kind == 1) {
                                    other = record->left;
                                    if (!SPILL_PREPARE_KNOWN_KIND(other->original_register)) ALLOCATION_KNOWN_KIND(other->original_register);
                                    if (other->original_register->kind == 1) {
                                        if (other->original_register != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(other->original_register)) SPILL_KNOWN_CANONICAL(other->original_register);
                                        root = other->original_register->canonical;
                                        if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                                        number = root->number;
                                        if (number && number < 9 && number != 5)
                                            reg->spill_preferred_register = root;
                                    }
                                }
                            }
                        }
                    } else if (source->kind == 1) {
                        if (operand->original_register != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(operand->original_register)) SPILL_KNOWN_CANONICAL(operand->original_register);
                        root = operand->original_register->canonical;
                        if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                        number = root->number;
                        if (number && number < 9 && number != 5) {
                            merge_optimizer_symbol_sets(spill_register_sets[number - 1], values);
                            root->spill_cost = 0x1000000;
                            machine_registers[number].flags |= 0x10;
                        }
                    } else if ((operand->type & 0xf000) != 0x4000) {
                        if (operand->original_register != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(operand->original_register)) SPILL_KNOWN_CANONICAL(operand->original_register);
                        root = operand->original_register->canonical;
                        if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                        number = root->number;
                        remove_optimizer_symbol(live, number);
                        SPILL_MACHINE_INDEX(number);
                        machine_registers[number].flags |= 0x10;
                        if (source->kind == 13) {
                            if (root->spill_cost < 0x1000000) {
                                cost = allocation_use_cost(source);
                                if (c2_favor_speed) cost <<= (block->number86 + 1) & 31;
                                root->spill_cost = (int)((unsigned)root->spill_cost - cost);
                            }
                        } else root->spill_cost = 0x1000000;
                    }
                } else if (operand->kind == 9) {
                    for (number = next_optimizer_symbol(operand->value.registers); number != ~0U;
                         number = next_optimizer_symbol(NULL)) {
                        if (number && number < 9 && number != 5) {
                            merge_optimizer_symbol_sets(spill_register_sets[number - 1], values);
                            machine_registers[number].flags |= 0x10;
                            machine_registers[number].spill_cost = 0x1000000;
                        }
                    }
                }
            }
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1) continue;
                source = operand->value.reg;
                flags = operand->location.expression.use_flags & ~0x10;
                operand->location.expression.use_flags = flags;
                if (!SPILL_PREPARE_KNOWN_KIND(source)) ALLOCATION_KNOWN_KIND(source);
                if (source->kind == 3 && source->value) {
                    reg = operand->original_register;
                    if (!SPILL_PREPARE_KNOWN_KIND(reg)) ALLOCATION_KNOWN_KIND(reg);
                    if (reg->kind == 1) {
                        if (reg != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(reg)) SPILL_KNOWN_CANONICAL(reg);
                        if (!SPILL_PREPARE_KNOWN_VIEW(reg->canonical)) SPILL_KNOWN_NUMBER(reg->canonical);
                        number = reg->canonical->number;
                        if (number && number < 9 && number != 5 &&
                            !optimizer_set_contains(live, number)) {
                            merge_optimizer_symbol_sets(spill_register_sets[number - 1], values);
                            add_register_to_set(live, number);
                            operand->location.expression.use_flags = flags | 0x10;
                        }
                    } else {
                        if ((operand->type & 0xfff) == 1) {
                            number = reg->number;
                            add_register_to_set(spill_register_sets[6], number);
                            add_register_to_set(spill_register_sets[7], number);
                            add_register_to_set(spill_register_sets[5], number);
                            reg->view_flags |= 0x80;
                        }
                        if (!find_final_register_use(reg, record)) {
                            number = reg->number;
                            add_register_to_set(values, number);
                            operand->location.expression.use_flags |= 0x10;
                            if (live->links.first) {
                                for (member = next_optimizer_symbol(live); member != ~0U;
                                     member = next_optimizer_symbol(NULL)) {
                                    SPILL_SET_INDEX(member);
                                    add_register_to_set(spill_register_sets[member - 1], number);
                                }
                            }
                            if (record->opcode == 1 && record->tail.right->kind == 1 &&
                                !(operand->location.expression.properties & 0x20)) {
                                other = record->tail.right;
                                if (!SPILL_PREPARE_KNOWN_KIND(other->original_register)) ALLOCATION_KNOWN_KIND(other->original_register);
                                if (other->original_register->kind == 1) {
                                    if (other->original_register != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(other->original_register)) SPILL_KNOWN_CANONICAL(other->original_register);
                                    root = other->original_register->canonical;
                                    if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                                    number = root->number;
                                    if (number && number < 9 && number != 5) {
                                        reg->spill_preferred_register = root;
                                        continue;
                                    }
                                }
                            }
                            reg->spill_preferred_register = NULL;
                        }
                    }
                } else if (source->kind != 1 && (operand->type & 0xf000) != 0x4000) {
                    if (operand->original_register != (C2FunctionRegister *)&machine_registers[19] &&
                        !SPILL_PREPARE_KNOWN_VIEW(operand->original_register)) SPILL_KNOWN_CANONICAL(operand->original_register);
                    root = operand->original_register->canonical;
                    if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
                    number = root->number;
                    if (!optimizer_set_contains(live, number)) {
                        SPILL_SET_INDEX(number);
                        merge_optimizer_symbol_sets(spill_register_sets[number - 1], values);
                        add_register_to_set(live, number);
                        operand->location.expression.use_flags = flags | 0x10;
                    }
                    if (source->kind == 13) {
                        if (root->spill_cost < 0x1000000) {
                            cost = allocation_copy_cost(record, source);
                            if (c2_favor_speed) cost <<= (block->number86 + 1) & 31;
                            root->spill_cost = (int)((unsigned)root->spill_cost + cost);
                        }
                    } else root->spill_cost = 0x1000000;
                }
            }
        }
    }
    release_optimizer_symbol_set(values);
    release_optimizer_symbol_set(live);
}

/* Sixteen cells are the exact493180..4931c0 order owner copied by the
 * later snapshot leaf. The first eight are defined by each real block reset;
 * upper cells are UNMODELED until actual appends write them.4931c0 remains
 * the distinct allocation_call_clobbered owner, not a seventeenth cell. */
int spill_float_count, spill_float_top;
unsigned spill_float_order[16];
int __fastcall rewrite_spill_float_block(C2Expression *first, C2Expression *last)
{
    C2Expression *record, *next, *end, *operand, *output;
    unsigned temporary;
    int top, position, index, effect, previous_swap = 0, multiple_swaps = 0;

    end = last->next;
    top = spill_float_top;
    for (record = first; record != end; record = next) {
        next = record->next;
        if (!(record->flags & 1) ||
            ((record->type & 0xf000) != 0x4000 && record->kind != 13)) continue;
        switch (record->opcode) {
        case 0x5f:
            if ((unsigned)top >= 16) { __asm { int 3 } }
            temporary = spill_float_order[top];
            if (record->left->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    record->left->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                    record->left->original_register != (C2FunctionRegister *)&machine_registers[35])
                ALLOCATION_KNOWN_NUMBER(record->left->original_register);
            position = (int)(record->left->original_register->number - 31);
            if (position >= 8) {
                c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x196);
                top = spill_float_top;
            }
            index = top - position;
            /* The modeled live prefix was written by reset/append. */
            if ((unsigned)top >= 16 || index < 0 || index > top) { __asm { int 3 } }
            spill_float_order[top] = spill_float_order[index];
            spill_float_order[index] = temporary;
            if (previous_swap || multiple_swaps) multiple_swaps = previous_swap = 1;
            else previous_swap = 1;
            break;
        case 0x8e:
        case 0x138:
            break;
        case 0x13c:
            return -1;
        default:
            previous_swap = 0;
            output = record->tail.right;
            for (operand = record->left; operand; operand = operand->next) {
                if (operand->kind != 1 || (operand->type & 0xf000) != 0x4000) continue;
                if (operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                    operand->original_register != (C2FunctionRegister *)&machine_registers[35])
                    ALLOCATION_KNOWN_NUMBER(operand->original_register);
                position = (int)(operand->original_register->number - 31);
                index = top - position;
                if ((unsigned)top >= 16 || index < 0 || index > top) { __asm { int 3 } }
                operand->float_stack_index = spill_float_order[index];
            }
            position = 0;
            if (output->kind == 1 && (output->type & 0xf000) == 0x4000) {
                if (output->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                    output->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                    output->original_register != (C2FunctionRegister *)&machine_registers[35])
                    ALLOCATION_KNOWN_NUMBER(output->original_register);
                position = (int)(output->original_register->number - 31);
            }
            effect = final_float_stack_effect(record);
            top = spill_float_top;
            switch (effect) {
            case 1:
                if (top >= 16) {
                    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x401);
                    top = spill_float_top;
                }
                /* Original old-top15 appends into a distinct neighboring
                 * object; that cross-owner domain is not modeled here. */
                if (top < -1 || top >= 15) { __asm { int 3 } }
                spill_float_count = (int)((unsigned)spill_float_count + 1);
                spill_float_top = ++top;
                spill_float_order[top] = spill_float_count;
                output->float_stack_index = spill_float_count;
                break;
            case 0:
            case -1:
            case -2:
                index = top - position;
                if ((unsigned)top >= 16 || index < 0 || index > top) { __asm { int 3 } }
                spill_float_count = (int)((unsigned)spill_float_count + 1);
                output->float_stack_index = spill_float_count;
                spill_float_order[index] = spill_float_count;
                if (effect < 0) spill_float_top = top += effect;
                break;
            }
            break;
        }
    }
    return multiple_swaps ? 1 : -2;
}

/* Original49a064/49a068: overwritten by this snapshot, with no modeled
 * initial default. All sixteen order cells are copied as storage; unavailable
 * upper-cell contents gain no semantic meaning from being saved. */
int saved_spill_float_top;
unsigned saved_spill_float_order[16];
void finish_spill_float_block(void)
{
    memcpy(saved_spill_float_order, spill_float_order, sizeof(saved_spill_float_order));
    saved_spill_float_top = spill_float_top;
}
int __fastcall is_spill_float_binary(C2Expression *record)
{
    switch (record->opcode) {
    case 0x5e: case 0x86: case 0x87: case 0x8a:
    case 0x91: case 0x92: case 0xd4:
        return 1;
    }
    return 0;
}
int __fastcall find_spill_float_position(long value)
{
    int position, top;

    top = spill_float_top;
    if (top >= 16) { __asm { int 3 } }
    for (position = 0; position <= top; ++position)
        if (spill_float_order[top - position] == (unsigned)value) return position;
    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x18f);
    return position;
}
void __fastcall exchange_spill_float_top(C2Expression *record, int position)
{
    C2Expression *scan, *left, *right;
    unsigned saved;
    int top, index;
    unsigned char kind;

    if (!position) return;
    top = spill_float_top;
    if ((unsigned)top >= 16) { __asm { int 3 } }
    saved = spill_float_order[top];
    if (position >= 8) {
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x196);
        top = spill_float_top;
    }
    index = top - position;
    if ((unsigned)top >= 16 || index < 0 || index > top ||
        (unsigned)position >= 8) { __asm { int 3 } }
    spill_float_order[top] = spill_float_order[index];
    spill_float_order[index] = saved;
    for (scan = record->previous; scan; scan = scan->previous) {
        kind = scan->kind;
        if ((scan->type & 0xf000) != 0x4000 &&
            kind != 13 && kind != 16 && kind != 25 &&
            kind != 18 && kind != 17 && kind != 24) continue;
        if (scan->opcode == 0x138) continue;
        left = new_fixed_register_operand(position + 31, 0x400a);
        right = new_fixed_register_operand(31, 0x400a);
        new_binary_expression(0x5f, 0x400a, right, left,
            insert_input_record, (C2Record *)scan);
        return;
    }
    c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x1af);
}
void __fastcall reconcile_spill_float_operands(C2Expression *record, int position, int original_position)
{ __asm { int 3 } }

#define RESTORE_FLOAT_NUMBER(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[33] && \
        (reg) != (C2FunctionRegister *)&machine_registers[34] && \
        (reg) != (C2FunctionRegister *)&machine_registers[35]) \
        ALLOCATION_KNOWN_NUMBER(reg); \
} while (0)
void __fastcall restore_spill_float_order(C2Expression *first, C2Expression *last)
{
    C2Expression *record, *next, *end, *output, *operand, *left, *right;
    int output_position, original_position, position, left_position, right_position;
    int binary, effect, index, top;

    end = last->next;
    for (record = first; record != end; record = next) {
        next = record->next;
        if (!(record->flags & 1) ||
            ((record->type & 0xf000) != 0x4000 && record->kind != 13) ||
            record->opcode == 0x138) continue;
        if (record->opcode == 0x5f) {
            unlink_input_record((C2Record *)record);
            release_small_node(record);
            continue;
        }
        output = record->tail.right;
        output_position = 0;
        if (output->kind == 1 && (output->type & 0xf000) == 0x4000) {
            RESTORE_FLOAT_NUMBER(output->original_register);
            output_position = (int)(output->original_register->number - 31);
        }
        binary = is_spill_float_binary(record);
        if (binary) {
            left = record->left;
            right = left->next;
            if (!right) c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x442);
            if (right->kind != 1) c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x443);
            RESTORE_FLOAT_NUMBER(left->original_register);
            RESTORE_FLOAT_NUMBER(right->original_register);
            left_position = (int)(left->original_register->number - 31);
            right_position = (int)(right->original_register->number - 31);
            if (!left_position) {
                if (right_position < 1) c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x447);
                position = find_spill_float_position(right->float_stack_index);
                reconcile_spill_float_operands(record, position, right_position);
                position = find_spill_float_position(left->float_stack_index);
            } else {
                if (right_position) c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x44b);
                position = find_spill_float_position(left->float_stack_index);
                reconcile_spill_float_operands(record, position, left_position);
                position = find_spill_float_position(right->float_stack_index);
            }
            exchange_spill_float_top(record, position);
        }
        for (operand = record->left; operand && !binary; operand = operand->next) {
            if (operand->kind != 1 || (operand->type & 0xf000) != 0x4000) continue;
            RESTORE_FLOAT_NUMBER(operand->original_register);
            original_position = (int)(operand->original_register->number - 31);
            position = find_spill_float_position(operand->float_stack_index);
            if (!original_position) {
                exchange_spill_float_top(record, position);
            } else if (record->kind == 13) {
                if (original_position != position) {
                    exchange_spill_float_top(record, original_position);
                    exchange_spill_float_top(record, position);
                    exchange_spill_float_top(record, original_position);
                }
            } else {
                if (!position) {
                    if (operand != record->left)
                        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x462);
                    right = operand->next;
                    if (right) {
                        if (right->kind != 1)
                            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x464);
                        position = find_spill_float_position(right->float_stack_index);
                        if (position < 1)
                            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x466);
                        exchange_spill_float_top(record, position);
                        operand->next->original_register = (C2FunctionRegister *)&machine_registers[31];
                    } else {
                        if (record->opcode != 0x60)
                            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x46b);
                        position = 0;
                    }
                }
                if ((unsigned)position >= 8) { __asm { int 3 } }
                operand->original_register = (C2FunctionRegister *)&machine_registers[31 + position];
                if (output_position == original_position) {
                    output->original_register = operand->original_register;
                    output_position = position;
                }
            }
        }
        effect = final_float_stack_effect(record);
        top = spill_float_top;
        switch (effect) {
        case 1:
            if (top >= 16) {
                c2_report_internal("E:\\utc\\src\\\\P2\\x86\\fppeeps.c", 0x47c);
                top = spill_float_top;
            }
            /* Old-top15 writes the distinct neighboring owner, not this array. */
            if (top < -1 || top >= 15) { __asm { int 3 } }
            spill_float_top = ++top;
            spill_float_order[top] = output->float_stack_index;
            break;
        case 0: case -1: case -2:
            index = top - output_position;
            if ((unsigned)top >= 16 || index < 0 || index > top) { __asm { int 3 } }
            spill_float_order[index] = output->float_stack_index;
            if (effect < 0) spill_float_top = top + effect;
            break;
        }
    }
}
#undef RESTORE_FLOAT_NUMBER
void __fastcall finish_spill_float_records(C2Expression *first)
{
    int i, position;

    /* Each recovered caller takes finish_spill_float_block's snapshot before
     * restoring and finishing this same block. No initial snapshot default
     * or value for an unwritten upper order cell is implied. */
    for (i = 0; i <= spill_float_top; ++i) {
        if ((unsigned)spill_float_top >= 16 || i >= 16) { __asm { int 3 } }
        if (spill_float_order[i] != saved_spill_float_order[i]) {
            position = find_spill_float_position(saved_spill_float_order[i]);
            exchange_spill_float_top(first, position);
            exchange_spill_float_top(first, spill_float_top - i);
        }
    }
}

void __fastcall optimize_spill_float_order(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *first, *last, *record;
    int i;

    if (!c2_favor_speed || !(function->flags & 8)) return;
    for (block = function->flow->last; block; block = block->previous) {
        first = (C2Expression *)block->records;
        last = (C2Expression *)block->last_record;
        spill_float_top = 7;
        for (i = 0; i < 8; ++i) spill_float_order[7 - i] = i;
        spill_float_count = 8;
        if (rewrite_spill_float_block(first, last) >= 0) {
            finish_spill_float_block();
            spill_float_top = 7;
            for (i = 0; i < 8; ++i) spill_float_order[7 - i] = i;
            restore_spill_float_order(first, last);
            record = last;
            while (record && record != first && (record->type & 0xf000) != 0x4000 &&
                record->kind != 13 && record->kind != 17) record = record->previous;
            /* The original then dereferences a missing terminator. There is
             * no defined continuation for such a malformed block. */
            if (!record) { __asm { int 3 } }
            finish_spill_float_records(record->next);
        }
    }
}

C2FunctionRegister *__fastcall find_spill_register(C2FunctionRegister *reg)
{
    unsigned *choice, number;

    for (choice = spill_register_order; (number = *choice) != 0; ++choice) {
        if (number >= 9) { __asm { int 3 } }
        if (spill_register_values[number] == reg)
            return (C2FunctionRegister *)&machine_registers[number];
    }
    return (C2FunctionRegister *)machine_registers;
}

void __fastcall lower_spilled_record_inputs(C2Expression *record)
{
    C2Expression *operand;
    C2FunctionRegister *source, *reg, *root, *assigned;
    unsigned number;
    long displacement;

    for (operand = record->left; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        source = operand->value.reg;
        if (!SPILL_PREPARE_KNOWN_KIND(source)) ALLOCATION_KNOWN_KIND(source);
        if (source->kind != 3 || !source->value) continue;
        reg = operand->original_register;
        if (!SPILL_PREPARE_KNOWN_VIEW(reg)) ALLOCATION_KNOWN_KIND(reg);
        if (reg->kind == 1) {
            if (!SPILL_PREPARE_KNOWN_VIEW(reg)) SPILL_KNOWN_CANONICAL(reg);
            root = reg->canonical;
            if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
            number = root->number;
            if (number < 9) spill_register_values[number] = NULL;
        } else {
            assigned = find_spill_register(reg);
            displacement = source->displacement < 4 ?
                (long)((unsigned)source->displacement << 3) : 0;
            operand->original_register = new_bitfield_register(assigned,
                (operand->type & 0xfff) << 3, displacement);
            if (!find_final_register_use(reg, record)) {
                if (!SPILL_PREPARE_KNOWN_VIEW(assigned)) SPILL_KNOWN_NUMBER(assigned);
                number = assigned->number;
                /* The original lookup returns a physical register index. */
                if (number >= 9) { __asm { int 3 } }
                spill_register_values[number] = NULL;
            }
        }
    }
}

C2FunctionRegister *__fastcall assign_spill_register(C2Expression *operand,
    C2Expression *record);

void __fastcall lower_spilled_record_outputs(C2Expression *record)
{
    C2Expression *operand;
    C2FunctionRegister *source, *reg, *root, *assigned;
    unsigned number;
    long displacement;

    for (operand = record->tail.right; operand; operand = operand->next) {
        if (operand->kind != 1) continue;
        source = operand->value.reg;
        if (!SPILL_PREPARE_KNOWN_KIND(source)) ALLOCATION_KNOWN_KIND(source);
        if (source->kind != 3 || !source->value) continue;
        reg = operand->original_register;
        if (!SPILL_PREPARE_KNOWN_VIEW(reg)) ALLOCATION_KNOWN_KIND(reg);
        if (reg->kind == 1) {
            if (!SPILL_PREPARE_KNOWN_VIEW(reg)) SPILL_KNOWN_CANONICAL(reg);
            root = reg->canonical;
            if (!SPILL_PREPARE_KNOWN_VIEW(root)) SPILL_KNOWN_NUMBER(root);
            number = root->number;
            if (number < 9) spill_register_values[number] = source;
        } else {
            assigned = find_spill_register(reg);
            if (assigned == (C2FunctionRegister *)machine_registers) {
                assigned = assign_spill_register(operand, record);
                operand->location.expression.use_flags |= 0x10;
            } else operand->location.expression.use_flags &= ~0x10;
            source = operand->value.reg;
            displacement = source->displacement < 4 ?
                (long)((unsigned)source->displacement << 3) : 0;
            operand->original_register = new_bitfield_register(assigned,
                (operand->type & 0xfff) << 3, displacement);
        }
    }
}

#undef SPILL_PREPARE_KNOWN_KIND
#undef SPILL_PREPARE_KNOWN_VIEW

C2FunctionRegister *__fastcall bind_spill_register(unsigned number,
    C2FunctionRegister *reg)
{
    if (number >= 9) { __asm { int 3 } }
    spill_register_values[number] = reg;
    mark_spill_register_used(number);
    machine_registers[number].spill_cost = 0x1000000;
    return (C2FunctionRegister *)&machine_registers[number];
}
unsigned __fastcall select_spill_input_register(C2Expression *record,
    C2Expression *operand, unsigned number) { __asm { int 3 } return 0; }
unsigned __fastcall select_spill_output_register(C2Expression *record,
    unsigned number) { __asm { int 3 } return 0; }
unsigned __fastcall select_spill_register_to_release(C2FunctionRegister *reg,
    C2Expression *record) { __asm { int 3 } return 0; }
C2Expression *__fastcall find_spill_restore_point(C2FunctionRegister *reg,
    C2Expression *first, int *mode) { __asm { int 3 } return NULL; }
void __fastcall insert_spill_restore(unsigned number, C2Expression *record,
    C2Expression *last, int mode) { __asm { int 3 } }

C2FunctionRegister *__fastcall assign_spill_register(C2Expression *operand,
    C2Expression *record)
{
    C2FunctionRegister *reg = operand->original_register, *root;
    C2Expression *input, *next, *last;
    unsigned number = reg->number, selected, candidate = 0;
    unsigned *choice, *start;
    int mode;

    if (!reg->spill_preferred_register && record->opcode == 1 &&
        record->left->kind == 1) {
        input = record->left;
        SPILL_KNOWN_CANONICAL(input->original_register);
        root = input->original_register->canonical;
        SPILL_KNOWN_NUMBER(root);
        selected = root->number;
        if (selected && selected < 9 && selected != 5)
            reg->spill_preferred_register = root;
    }
    if (record->opcode == 0x12 && record->left->kind == 5 &&
        !reg->spill_preferred_register) {
        input = record->left;
        if (input->aggregate_value && !input->tail.right &&
            (!input->aggregate_second ||
             (!(input->location.expression.properties & 0xf) && !input->detail.aggregate_displacement)))
            reg->spill_preferred_register = input->aggregate_value->original_register;
    }
    root = reg->spill_preferred_register;
    if (root) {
        SPILL_KNOWN_NUMBER(root);
        selected = root->number;
        SPILL_SET_INDEX(selected);
        if (!spill_register_values[selected] &&
            !optimizer_set_contains(spill_register_sets[selected - 1], number))
            return bind_spill_register(selected, reg);
    }
    if (c2_favor_speed) {
        choice = start = spill_register_cursor;
        for (;;) {
            if (!*choice || *choice >= 4) {
                choice = spill_register_order;
                if (start == spill_register_order) break;
            }
            selected = *choice++;
            SPILL_SET_INDEX(selected);
            if (!spill_register_values[selected] &&
                !optimizer_set_contains(spill_register_sets[selected - 1], number)) {
                spill_register_cursor = *choice && *choice < 4 ? choice : spill_register_order;
                return bind_spill_register(selected, reg);
            }
            if (choice == start) break;
        }
    }
    for (choice = spill_register_order; (selected = *choice) != 0; ++choice) {
        SPILL_SET_INDEX(selected);
        if (!spill_register_values[selected] &&
            !optimizer_set_contains(spill_register_sets[selected - 1], number))
            return bind_spill_register(selected, reg);
    }
    selected = select_spill_input_register(record, operand, number);
    if (selected) return bind_spill_register(selected, reg);
    if (c2_global_optimize && !record->tail.right->next) {
        selected = select_spill_output_register(record, number);
        if (selected) return bind_spill_register(selected, reg);
    }
    for (next = record->next; next && !(next->flags & 1); next = next->next) {}
    /* The original requires a following executable record on this path. */
    if (!next) { __asm { int 3 } }
    for (choice = spill_register_order; (selected = *choice) != 0; ++choice) {
        SPILL_SET_INDEX(selected);
        if (!optimizer_set_contains(spill_register_sets[selected - 1], number) &&
            !find_final_register_use(spill_register_values[selected], record)) {
            last = find_spill_restore_point(spill_register_values[selected], next, &mode);
            if (last != next) {
                insert_spill_restore(selected, record, last, mode);
                return bind_spill_register(selected, reg);
            }
            candidate = selected;
        }
    }
    selected = select_spill_register_to_release(reg, record);
    if (selected) return bind_spill_register(selected, reg);
    if (!candidate) c2_report_internal("E:\\utc\\src\\\\P2\\regasg.c", 0x269);
    SPILL_SET_INDEX(candidate);
    last = find_spill_restore_point(spill_register_values[candidate], next, &mode);
    insert_spill_restore(candidate, record, last, mode);
    return bind_spill_register(candidate, reg);
}

C2FunctionRegister *__fastcall new_spill_temporary_register(unsigned short type)
{
    C2FunctionRegister *reg = new_function_register(3);
    reg->type = type;
    reg->kind = 3;
    reg->value_bytes = type & 0xfff;
    reg->canonical = reg;
    return reg;
}

void optimize_region_spill_moves(void)
{
    C2FlowRegion *region;
    C2BasicBlock *header, *before, *after;
    C2FlowEdge *edge;
    C2Expression *first, *last, *record, *scan, *candidate, *operand, *temporary;
    C2FunctionRegister *index, *assigned, *reg;
    C2RegisterSet *written, *available, *uses, *definitions;
    unsigned *choice, number;
    unsigned short type;

    for (region = allocation_region_chain; region; region = region->allocation_next) {
        header = region->header;
        before = header->next;
        edge = before->outgoing;
        if (edge->destination != header || edge->next_outgoing) continue;
        after = header->previous;
        edge = after->incoming;
        if (!edge || edge->source != header || edge->next_incoming) continue;
        edge = header->outgoing;
        if (edge->destination != header && edge->destination != after) continue;
        edge = edge->next_outgoing;
        if (!edge || (edge->destination != header && edge->destination != after)) continue;
        first = (C2Expression *)header->records;
        last = (C2Expression *)header->last_record;
        written = new_register_set(6);
        for (record = first; record != last; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (operand = record->tail.right; operand; operand = operand->next) {
                if (operand->kind == 1) {
                    SPILL_KNOWN_CANONICAL(operand->original_register);
                    reg = operand->original_register->canonical;
                    SPILL_KNOWN_NUMBER(reg);
                    add_register_to_set(written, reg->number);
                } else if (operand->kind == 9)
                    merge_optimizer_symbol_sets(written, operand->value.registers);
            }
        }
        for (record = first; record != last; record = record->next) {
            if (!(record->flags & 1)) continue;
            candidate = record->tail.right;
            if (!candidate || candidate->kind != 6 ||
                (candidate->type & 0xf000) == 0x4000 || candidate->opcode != 0x122 ||
                (candidate->location.expression.properties & 0x40)) continue;
            index = candidate->aggregate_value->original_register;
            number = index->number;
            if (optimizer_set_contains(written, number)) continue;
            uses = header->allocation_edge_uses;
            if (!optimizer_set_contains(uses, number)) continue;
            definitions = header->allocation_edge_definitions;
            if (!optimizer_set_contains(definitions, number)) continue;
            type = candidate->type;
            available = new_register_set(6);
            for (choice = spill_register_order; *choice; ++choice)
                add_register_to_set(available, *choice);
            if ((type & 0xfff) == 1) {
                remove_optimizer_symbol(available, 7);
                remove_optimizer_symbol(available, 8);
                remove_optimizer_symbol(available, 6);
            }
            subtract_optimizer_symbol_sets(available, written);
            subtract_optimizer_symbol_sets(available, uses);
            if (!available->links.first) {
                release_optimizer_symbol_set(available);
                continue;
            }
            for (scan = first; scan != last; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                for (operand = scan->tail.right; operand; operand = operand->next) {
                    if (operand->kind == 2) {
                        if (register_may_alias_index(operand->value.reg, candidate->immediate))
                            goto rejected_region_spill;
                    } else if (operand->kind == 6) {
                        if (operand != candidate &&
                            optimizer_indices_overlap(candidate->immediate, operand->immediate))
                            goto rejected_region_spill;
                    } else if (operand->kind == 10 &&
                        optimizer_indices_overlap(candidate->immediate, operand->value.optimizer_index))
                        goto rejected_region_spill;
                }
                for (operand = scan->left; operand; operand = operand->next) {
                    if (operand->kind == 2) {
                        if (register_may_alias_index(operand->value.reg, candidate->immediate))
                            goto rejected_region_spill;
                    } else if (operand->kind == 6) {
                        if (optimizer_indices_overlap(candidate->immediate, operand->immediate) &&
                            (operand->opcode != 0x122 ||
                             operand->aggregate_value->original_register != index ||
                             operand->type != type || ((candidate->type ^ operand->type) & 0xfff) ||
                             operand->detail.data != candidate->detail.data ||
                             operand->tail.right != candidate->tail.right))
                            goto rejected_region_spill;
                    } else if (operand->kind == 10 &&
                        optimizer_indices_overlap(candidate->immediate, operand->value.optimizer_index))
                        goto rejected_region_spill;
                }
            }
            for (choice = spill_register_order; (number = *choice) != 0; ++choice)
                if (optimizer_set_contains(available, number)) break;
            release_optimizer_symbol_set(available);
            SPILL_MACHINE_INDEX(number);
            machine_registers[number].flags |= 0x10;
            machine_registers[number].spill_cost = 0x1000000;
            assigned = new_bitfield_register((C2FunctionRegister *)&machine_registers[number],
                (type & 0xfff) << 3, 0);
            reg = new_spill_temporary_register(type);
            temporary = new_temporary_register_expression(0x11f, type, reg, assigned);
            record_line_delta = 0;
            new_binary_expression(1, type, temporary, candidate,
                prepend_input_record, before->last_record);
            new_binary_expression(1, type, candidate, temporary,
                insert_input_record, after->records);
            for (scan = first; scan != last; scan = scan->next) {
                if (!(scan->flags & 1)) continue;
                for (operand = scan->left; operand; operand = operand->next) {
                    if (operand->kind == 6 && operand->opcode == 0x122 &&
                        operand->aggregate_value->original_register == index &&
                        operand->detail.data == candidate->detail.data) {
                        attach_input_expression(scan, operand, temporary);
                        break;
                    }
                }
            }
            replace_input_result(record, candidate, temporary);
            add_register_to_set(before->allocation_edge_definitions, number);
            add_register_to_set(uses, number);
            add_register_to_set(definitions, number);
            add_register_to_set(after->allocation_edge_uses, number);
            add_register_to_set(after->allocation_edge_uses, index->number);
            continue;
rejected_region_spill:
            release_optimizer_symbol_set(available);
        }
        release_optimizer_symbol_set(written);
    }
}

void __fastcall release_unused_spill_register(C2Function *function,
    C2FunctionRegister *reg)
{
    C2Expression *record, *next, *operand;
    for (record = (C2Expression *)function->flow->last->records; record; record = next) {
        next = record->next;
        if (!(record->flags & 1)) continue;
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            if (operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[35] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[74])
                SPILL_KNOWN_CANONICAL(operand->original_register);
            if (operand->original_register->canonical == reg) {
                unlink_and_release_input_record((C2Record *)record);
                goto next_unused_spill_record;
            }
        }
        for (operand = record->left; operand; operand = operand->next) {
            if (operand->kind != 1) continue;
            if (operand->original_register != (C2FunctionRegister *)&machine_registers[9] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[31] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[32] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[33] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[34] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[35] &&
                operand->original_register != (C2FunctionRegister *)&machine_registers[74])
                SPILL_KNOWN_CANONICAL(operand->original_register);
            if (operand->original_register->canonical == reg)
                materialize_allocation_input(record, operand, operand->value.reg);
        }
next_unused_spill_record: ;
    }
}
void __fastcall replace_spill_frame_register(C2Function *function,
    C2FunctionRegister *reg) { __asm { int 3 } }

void __fastcall finish_spill_registers(C2Function *function)
{
    C2MachineRegister *reg;
    unsigned *choice, number;

    for (choice = spill_register_order; *choice; ++choice) {
        number = *choice;
        SPILL_SET_INDEX(number);
        reg = &machine_registers[number];
        /* Each listed cost was written by the real initializer and subsequent
         * assignment pass; this is not a static-row default. */
        if (reg->spill_cost <= 2) {
            release_unused_spill_register(function, (C2FunctionRegister *)reg);
            SPILL_KNOWN_NUMBER((C2FunctionRegister *)reg);
            number = reg->number;
            SPILL_MACHINE_INDEX(number);
            machine_registers[number].flags &= ~0x10;
        }
    }
    if (allocation_frame_register_available && (machine_registers[6].flags & 0x10) &&
        !c2_favor_speed) {
        if (!(machine_registers[4].flags & 0x10)) {
            replace_spill_frame_register(function, (C2FunctionRegister *)&machine_registers[4]);
            machine_registers[4].flags |= 0x10;
            machine_registers[6].flags &= ~0x10;
        } else if (!(machine_registers[7].flags & 0x10)) {
            replace_spill_frame_register(function, (C2FunctionRegister *)&machine_registers[7]);
            machine_registers[7].flags |= 0x10;
            machine_registers[6].flags &= ~0x10;
        } else if (!(machine_registers[8].flags & 0x10)) {
            replace_spill_frame_register(function, (C2FunctionRegister *)&machine_registers[8]);
            machine_registers[8].flags |= 0x10;
            machine_registers[6].flags &= ~0x10;
        }
    }
}

/* Original4911a4 is an unchanged initialized DWORD8;4911a8 is reset to7
 * for each block before the cyclic selector is called. */
int spill_register_order_count = 8, spill_register_order_index;
C2Expression *__fastcall find_physical_register_operand(unsigned number,
    C2Expression *operands)
{
    C2FunctionRegister *root;

    for (; operands; operands = operands->next) {
        if (operands->kind != 1) continue;
        if (operands->original_register != (C2FunctionRegister *)&machine_registers[17] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[20] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[9] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[31] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[32] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[33] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[34] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[35] &&
            operands->original_register != (C2FunctionRegister *)&machine_registers[74])
            SPILL_KNOWN_CANONICAL(operands->original_register);
        root = operands->original_register->canonical;
        if (root != (C2FunctionRegister *)&machine_registers[33] &&
            root != (C2FunctionRegister *)&machine_registers[34] &&
            root != (C2FunctionRegister *)&machine_registers[35] &&
            root != (C2FunctionRegister *)&machine_registers[74])
            SPILL_KNOWN_NUMBER(root);
        if (root->number == number) return operands;
    }
    return NULL;
}
unsigned __fastcall choose_spill_scratch_register(C2RegisterSet *used,
    int byte_only)
{
    unsigned number;
    int pass, index, start, count;

    start = spill_register_order_index;
    count = spill_register_order_count;
    if (count != 8 || start < 0 || start >= count) { __asm { int 3 } }
    for (pass = 0; pass < 2; ++pass) {
        index = pass ? 0 : start + 1;
        if (index >= count) index = 0;
        while (index != start) {
            number = spill_register_order[index];
            SPILL_MACHINE_INDEX(number);
            if (number && ((machine_registers[number].flags & 0x10) || pass) &&
                (!byte_only || number < 5) && !optimizer_set_contains(used, number)) {
                spill_register_order_index = index;
                return spill_register_order[index];
            }
            start = spill_register_order_index;
            count = spill_register_order_count;
            if (++index >= count) index = 0;
        }
    }
    for (index = 0; (number = spill_register_order[index]) != 0; ++index) {
        if ((!byte_only || number <= 4) && !optimizer_set_contains(used, number)) {
            spill_register_order_index = index;
            return spill_register_order[index];
        }
    }
    return 0;
}
void __fastcall materialize_spill_scratch(C2Expression *record,
    C2Expression *operand, unsigned number, int output)
{
    C2FunctionRegister *view;
    C2Expression *temporary, *move, *first;
    unsigned short type = operand->type;

    if ((type & 0xfff) == 1) {
        if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
        view = new_bitfield_register((C2FunctionRegister *)&machine_registers[number], 8, 0);
        SPILL_KNOWN_NUMBER(view);
        number = view->number;
    }
    temporary = new_fixed_register_operand(number, type);
    move = new_binary_expression(1, type, temporary, operand,
        prepend_input_record, (C2Record *)record);
    move->flags |= 4;
    if (output) {
        type = operand->type;
        temporary = new_fixed_register_operand(number, type);
        move = new_binary_expression(1, type, operand, temporary,
            insert_input_record, (C2Record *)record);
        move->flags |= 4;
    }
    if (!output) {
        temporary = new_fixed_register_operand(number, operand->type);
        attach_input_expression(record, operand, temporary);
        record->flags |= 4;
        return;
    }
    first = record->left;
    if (!compare_input_operands(operand, first)) {
        temporary = new_fixed_register_operand(number, operand->type);
        attach_input_expression(record, first, temporary);
    }
    temporary = new_fixed_register_operand(number, operand->type);
    replace_input_result(record, operand, temporary);
    record->flags |= 4;
}

#define SPILL_KNOWN_KIND(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[18] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[5] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[67] && \
        (reg) != (C2FunctionRegister *)&machine_registers[68] && \
        (reg) != (C2FunctionRegister *)&machine_registers[69] && \
        (reg) != (C2FunctionRegister *)&machine_registers[70] && \
        (reg) != (C2FunctionRegister *)&machine_registers[71] && \
        (reg) != (C2FunctionRegister *)&machine_registers[72] && \
        (reg) != (C2FunctionRegister *)&machine_registers[73]) { __asm { int 3 } } } while (0)
/* The current byte views17/20 expose only their proved kind/canonical
 * columns in addition to their earlier range/address identities. */
#define SCRATCH_KNOWN_KIND(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[74]) SPILL_KNOWN_KIND(reg); } while (0)
#define SCRATCH_KNOWN_CANONICAL(reg) \
    do { if ((reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20] && \
        (reg) != (C2FunctionRegister *)&machine_registers[9] && \
        (reg) != (C2FunctionRegister *)&machine_registers[74]) SPILL_KNOWN_CANONICAL(reg); } while (0)
void __fastcall materialize_spill_scratch_operands(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2FunctionRegister *source, *reg, *root;
    C2RegisterSet *live;
    unsigned number, selected, saved;
    unsigned short bytes;
    int side;

    spill_register_order[6] = allocation_frame_register_available ? 6 : 0;
    live = new_register_set(6);
    for (block = function->flow->last; block; block = block->previous) {
        spill_register_order_index = spill_register_order_count - 1;
        if (!allocation_is_empty)
            copy_optimizer_symbol_set_into(live, block->allocation_edge_definitions);
        else clear_register_set(live);
        for (record = (C2Expression *)block->last_record;
             record != (C2Expression *)block->records; record = record->previous) {
            if (!(record->flags & 1)) continue;
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record->location.line;
            }
            for (operand = record->tail.right; operand; operand = operand->next) {
                if ((operand->type & 0xf000) == 0x4000 || operand->kind != 1) continue;
                source = operand->value.reg;
                SCRATCH_KNOWN_KIND(source);
                reg = operand->original_register;
                if (source->kind == 3 && source->value) {
                    SCRATCH_KNOWN_KIND(reg);
                    if (reg->kind != 1) continue;
                    SCRATCH_KNOWN_CANONICAL(reg);
                    root = reg->canonical;
                    if (root != (C2FunctionRegister *)&machine_registers[74])
                        SPILL_KNOWN_NUMBER(root);
                    number = root->number;
                    if (find_physical_register_operand(number, record->left)) continue;
                    if (operand->next && find_physical_register_operand(number, operand->next)) continue;
                } else {
                    SCRATCH_KNOWN_CANONICAL(reg);
                    root = reg->canonical;
                    if (root != (C2FunctionRegister *)&machine_registers[74])
                        SPILL_KNOWN_NUMBER(root);
                    number = root->number;
                    if (find_physical_register_operand(number, record->left)) continue;
                }
                if (number && number < 9 && number != 5)
                    remove_optimizer_symbol(live, number);
            }
            for (operand = record->left; operand; operand = operand->next) {
                if ((operand->type & 0xf000) == 0x4000 || operand->kind != 1) continue;
                source = operand->value.reg;
                SCRATCH_KNOWN_KIND(source);
                if (source->kind == 3 && source->value) {
                    reg = operand->original_register;
                    SCRATCH_KNOWN_KIND(reg);
                    if (reg->kind != 1) continue;
                } else {
                    if (source->kind == 1) continue;
                    reg = operand->original_register;
                }
                SCRATCH_KNOWN_CANONICAL(reg);
                root = reg->canonical;
                if (root != (C2FunctionRegister *)&machine_registers[74])
                    SPILL_KNOWN_NUMBER(root);
                number = root->number;
                if (number && number < 9 && number != 5 && !optimizer_set_contains(live, number))
                    add_register_to_set(live, number);
            }
            /* Only bit0x400 is consumed here, for opcodes1..11a. Its22 true
             * rows describe operations that admit a scratch memory operand;
             * neighboring WORD bits and addresses are not represented. */
            number = record->opcode;
            if (!number || number >= 0x11b ||
                !(number == 9 || number == 0xd || number == 0x1e || number == 0x21 ||
                  (number >= 0x23 && number <= 0x34))) continue;
            selected = choose_spill_scratch_register(live, 0);
            if (!selected) continue;
            for (side = 0; side < 2; ++side) {
                operand = side ? record->left : record->tail.right;
                for (; operand; operand = operand->next) {
                    if (operand->kind != 2 && operand->kind != 6) continue;
                    if ((operand->type & 0xf000) == 0x4000 ||
                        (operand->location.expression.properties & 0x20)) continue;
                    bytes = operand->type & 0xfff;
                    if (bytes == 2) continue;
                    if (bytes == 1 && selected > 4) {
                        saved = selected;
                        selected = choose_spill_scratch_register(live, 1);
                        if (!selected) { selected = saved; continue; }
                    }
                    materialize_spill_scratch(record, operand, selected, side == 0);
                    SPILL_MACHINE_INDEX(selected);
                    machine_registers[selected].flags |= 0x10;
                    break;
                }
            }
        }
    }
    release_optimizer_symbol_set(live);
}

/* Fold register copies across the instruction pairs chosen by the scheduler,
 * then remove saves/restores for physical registers no longer written. */
#undef SCRATCH_KNOWN_KIND
#undef SCRATCH_KNOWN_CANONICAL
void __fastcall fold_scheduled_register_moves(C2Function *function)
{
    C2Expression *first = (C2Expression *)function->flow->last->records;
    C2Expression *cursor = ((C2Expression *)function->flow->first->last_record)->previous;
    C2Expression *record, *middle, *prior, *operand, *replacement, *next;
    C2FunctionRegister *reg, *root, *other;
    C2RegisterSet *used;
    unsigned number;
    int changed_saves = 0;
    while (cursor != first) {
        record = cursor;
        cursor = middle = record->previous;
        prior = middle->previous;
        if (!(record->flags & 1) || !(record->flags & 4)) continue;
        if (record->opcode == 1) {
            if (!(middle->flags & 4)) continue;
            if ((prior->flags & 4) && prior->opcode == 1 && prior->tail.right->kind == 1 && record->left->kind == 1) {
                reg = prior->tail.right->original_register;
                other = record->left->original_register;
                SPILL_KNOWN_NUMBER(reg);
                SPILL_KNOWN_NUMBER(other);
                if (reg->number == other->number && !compare_input_operands(prior->left, record->tail.right)) {
                    reg = record->left->original_register;
                    SPILL_KNOWN_CANONICAL(reg);
                    root = reg->canonical;
                    SPILL_KNOWN_NUMBER(root);
                    number = root->number;
                    operand = find_physical_register_operand(number, middle->tail.right);
                    if (operand) {
                        SPILL_KNOWN_NUMBER(operand->original_register);
                        if (reg->number == operand->original_register->number) {
                            if (number > 3) changed_saves = 1;
                            if (!compare_input_operands(operand, middle->left))
                                attach_input_expression(middle, middle->left, record->tail.right);
                            replace_input_result(middle, operand, record->tail.right);
                            middle->flags &= 0xfb;
                            unlink_and_release_input_record(record);
                            unlink_and_release_input_record(prior);
                            cursor = middle->previous;
                            continue;
                        }
                    }
                }
            }
        }
        if (!(middle->flags & 4) || middle->opcode != 1 || middle->tail.right->kind != 1) continue;
        reg = middle->tail.right->original_register;
        SPILL_KNOWN_CANONICAL(reg);
        root = reg->canonical;
        SPILL_KNOWN_NUMBER(root);
        number = root->number;
        if (find_physical_register_operand(number, record->tail.right)) continue;
        operand = find_physical_register_operand(number, record->left);
        if (!operand) continue;
        SPILL_KNOWN_NUMBER(reg);
        SPILL_KNOWN_NUMBER(operand->original_register);
        if (reg->number != operand->original_register->number ||
            (record->opcode == 1 && record->tail.right->kind != 1)) continue;
        if (!operand->next || !find_physical_register_operand(number, operand->next)) {
            if (number > 3) changed_saves = 1;
            attach_input_expression(record, operand, middle->left);
        } else {
            replacement = middle->left;
            if (record->opcode != 0x33 || compare_input_operands(record->left, record->left->next) ||
                replacement->kind != 6 || replacement->detail.aggregate_displacement || replacement->tail.right) continue;
            if (number > 3) changed_saves = 1;
            attach_input_expression(record, record->left, replacement);
            operand = record->left->next;
            replacement = new_integer_expression(operand->type, 0);
            attach_input_expression(record, operand, replacement);
            record->opcode = 0x2f;
        }
        record->flags &= 0xfb;
        unlink_and_release_input_record(middle);
        cursor = record->previous;
    }
    if (!changed_saves) return;
    machine_registers[6].flags &= ~0x10;
    machine_registers[4].flags &= ~0x10;
    machine_registers[8].flags &= ~0x10;
    machine_registers[7].flags &= ~0x10;
    used = new_register_set(6);
    for (record = first; record; record = record->next) {
        if (!(record->flags & 1) || record->opcode == 0xb9) continue;
        for (operand = record->tail.right; operand; operand = operand->next) {
            if (operand->kind == 1) {
                reg = operand->original_register;
                SPILL_KNOWN_CANONICAL(reg);
                root = reg->canonical;
                SPILL_KNOWN_NUMBER(root);
                number = root->number;
                SPILL_MACHINE_INDEX(number);
                machine_registers[number].flags |= 0x10;
            } else if (operand->kind == 9) merge_optimizer_symbol_sets(used, operand->value.registers);
        }
    }
    if (optimizer_set_contains(used, 4)) machine_registers[4].flags |= 0x10;
    if (optimizer_set_contains(used, 6)) machine_registers[6].flags |= 0x10;
    if (optimizer_set_contains(used, 7)) machine_registers[7].flags |= 0x10;
    if (optimizer_set_contains(used, 8)) machine_registers[8].flags |= 0x10;
    release_optimizer_symbol_set(used);
    if ((machine_registers[4].flags & 0x10) &&
        ((machine_registers[6].flags & 0x10) || !allocation_frame_register_available) &&
        (machine_registers[7].flags & 0x10) && (machine_registers[8].flags & 0x10)) return;
    for (record = first; record; record = next) {
        next = record->next;
        if (record->opcode == 0xba) reg = record->left->original_register;
        else if (record->opcode == 0xb9) reg = record->tail.right->original_register;
        else continue;
        SPILL_KNOWN_NUMBER(reg);
        number = reg->number;
        SPILL_MACHINE_INDEX(number);
        if (!(machine_registers[number].flags & 0x10)) unlink_and_release_input_record(record);
    }
}

void __fastcall redirect_switch_branch(C2Expression *branch, C2Record *label, int mode)
{ __asm { int 3 } }

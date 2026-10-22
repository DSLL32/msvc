/* Late x87 store movement across branch edges. */
#include "backend-passes.h"
#include "instruction-rewrite.h"
#include "machine-layout.h"
#include "jump-threading.h"
#include "register-use.h"
#include <recon.h>

#define SPLIT_KIND(i) ((i)->opcode & 15)
#define SPLIT_OPERATION(i) (((i)->opcode >> 8) & 255)
#define SPLIT_SKIP(i) (SPLIT_KIND(i) != 0 && SPLIT_KIND(i) != 1)
#define SPLIT_DETACH(i) do { \
    (i)->previous->next = (i)->next; \
    (i)->next->previous = (i)->previous; \
} while (0)
#define SPLIT_LABEL_SYMBOL(i) ((LabelSymbol *)(i)->record.symbol)
#define SPLIT_TO_NONPOPPING_STORE(i) \
    ((i)->opcode = ((i)->opcode & ~0xff00UL) | (MC_FST << 8))

/* Original [0004b950,0004c0f6). Distribute a delayed FST/FSTP across a
   branch, splitting target labels as required by their other incoming edges.
   Paired FLD/FSTP sequences can then retain the value already on the x87 stack.
   The first integer hello function takes only the no-floating-work guard. */
void __fastcall split_machine_instructions(MachineContext *context)
{
    MachineInstruction *branch, *store, *target, *prior, *anchor, *created;
    MachineInstruction *copy, *following, *back, *edge_branch, *return_jump;
    LabelSymbol *label;
    MachineOperand operand;
    unsigned operation;
    int float_block = 0, crossed_label, moved;
    if (!(current_function->field28 & 8)) return;
    for (branch = context; branch; branch = branch->next) {
        if (SPLIT_KIND(branch) == 7) float_block = (((MachineBlockHeader *)branch)->flags & 1) != 0;
        operation = SPLIT_OPERATION(branch);
        if (SPLIT_KIND(branch) != 0 || float_block || (operation != MC_JCC && operation != MC_JMP) ||
            (branch->operands[0].flags & 15) != OPERAND_SYMBOL || branch->operands[0].value.symbol->kind != 3) continue;
        for (store = branch->previous; SPLIT_SKIP(store); store = store->previous) ;
        operation = SPLIT_OPERATION(store);
        if (SPLIT_KIND(store) != 0 || (operation != MC_FSTP && operation != MC_FST) || !(store->opcode & 0x200000UL)) continue;
        target = label_machine_instruction((LabelSymbol *)branch->operands[0].value.symbol);
        if (!(target->labels.flags & 1)) {
            /* A target with no fall-through entry can accept the store after
               its label, or share an identical store on the incoming edge. */
            for (prior = target->previous; SPLIT_KIND(prior) != 0; prior = prior->previous) ;
            if (SPLIT_OPERATION(prior) != MC_JMP &&
                (SPLIT_OPERATION(store) != SPLIT_OPERATION(prior) ||
                 !operands_equal(&prior->operands[0], &store->operands[0]) ||
                 !operands_equal(&prior->operands[1], &store->operands[1]))) continue;
            if (SPLIT_OPERATION(prior) == MC_JMP) {
                if (SPLIT_LABEL_SYMBOL(target)->references == 1) {
                    copy = clone_machine_record(store);
                    insert_machine_after(target, copy);
                } else {
                    label = new_internal_label();
                    created = new_machine_record(1, label);
                    created = insert_machine_after(prior, created);
                    register_machine_label(created);
                    SPLIT_LABEL_SYMBOL(created)->references = 1;
                    branch->operands[0].value.symbol = created->record.symbol;
                    --SPLIT_LABEL_SYMBOL(target)->references;
                    copy = clone_machine_record(store);
                    insert_machine_after(created, copy);
                }
            } else if (SPLIT_LABEL_SYMBOL(target)->references == 1) {
                SPLIT_DETACH(prior);
                insert_machine_after(target, prior);
            } else {
                label = new_internal_label();
                created = new_machine_record(1, label);
                created = insert_machine_after(prior->previous, created);
                register_machine_label(created);
                SPLIT_LABEL_SYMBOL(created)->references = 1;
                branch->operands[0].value.symbol = created->record.symbol;
                --SPLIT_LABEL_SYMBOL(target)->references;
            }
            SPLIT_DETACH(store);
            if (SPLIT_OPERATION(branch) == MC_JMP) {
                release_machine_instruction(store);
                continue;
            }
            insert_machine_after(branch, store);
            for (following = store->next; SPLIT_SKIP(following); following = following->next) ;
            if (SPLIT_KIND(following) == 0 && SPLIT_OPERATION(following) == MC_FLD &&
                SPLIT_OPERATION(store) == MC_FSTP &&
                operands_equal(&following->operands[1], &store->operands[0]) &&
                operands_equal(&following->operands[0], &store->operands[1])) {
                SPLIT_TO_NONPOPPING_STORE(store);
                remove_machine_instruction(following);
            }
        } else {
            /* A fall-through target needs a private edge when its predecessor
               already stores the same value, or a new label plus copied store. */
            crossed_label = 0;
            anchor = target->previous;
            for (prior = anchor; SPLIT_KIND(prior) != 0; prior = prior->previous)
                if (SPLIT_KIND(prior) == 1) crossed_label = 1;
            edge_branch = 0;
            return_jump = 0;
            SPLIT_DETACH(store);
            if (SPLIT_OPERATION(store) == SPLIT_OPERATION(prior) &&
                operands_equal(&prior->operands[0], &store->operands[0]) &&
                operands_equal(&prior->operands[1], &store->operands[1])) {
                if (SPLIT_LABEL_SYMBOL(target)->references != 1 || crossed_label) {
                    label = new_internal_label();
                    created = new_machine_record(1, label);
                    created = insert_machine_after(prior->previous, created);
                    created->labels.flags |= 1;
                    target->labels.flags &= ~1U;
                    register_machine_label(created);
                    SPLIT_LABEL_SYMBOL(created)->references = 1;
                    branch->operands[0].value.symbol = created->record.symbol;
                    --SPLIT_LABEL_SYMBOL(target)->references;
                } else {
                    SPLIT_DETACH(prior);
                    insert_machine_after(target, prior);
                    for (following = prior->next; SPLIT_SKIP(following); following = following->next) ;
                    if (SPLIT_KIND(following) == 0 && SPLIT_OPERATION(following) == MC_FLD &&
                        SPLIT_OPERATION(prior) == MC_FSTP && (prior->opcode & 0xf0) != 0xa0 &&
                        operands_equal(&following->operands[1], &prior->operands[0]) &&
                        operands_equal(&following->operands[0], &prior->operands[1])) {
                        SPLIT_TO_NONPOPPING_STORE(prior);
                        remove_machine_instruction(following);
                    }
                }
            } else {
                label = new_internal_label();
                created = new_machine_record(1, label);
                created = insert_machine_after(anchor, created);
                created->labels.flags |= 1;
                target->labels.flags &= ~1U;
                register_machine_label(created);
                SPLIT_LABEL_SYMBOL(created)->references = 1;
                branch->operands[0].value.symbol = created->record.symbol;
                --SPLIT_LABEL_SYMBOL(target)->references;
                copy = clone_machine_record(store);
                insert_machine_after(target->previous, copy);
                operation = SPLIT_OPERATION(prior);
                if ((operation == MC_JCC || operation == MC_JMP) && (prior->operands[0].flags & 15) == OPERAND_SYMBOL)
                    edge_branch = prior;
                if (operation != MC_JMP) {
                    initialize_operand(&operand, OPERAND_SYMBOL, 0, 0, target->record.symbol);
                    copy = new_machine_record(0, MC_JMP, 0x104, &operand);
                    return_jump = insert_machine_after(prior, copy);
                    ++SPLIT_LABEL_SYMBOL(target)->references;
                }
                if (!SPLIT_LABEL_SYMBOL(target)->references) {
                    for (following = target->next; SPLIT_SKIP(following); following = following->next) ;
                    if (SPLIT_KIND(following) == 0 && SPLIT_OPERATION(following) == MC_FLD &&
                        SPLIT_OPERATION(store) == MC_FSTP && (store->opcode & 0xf0) != 0xa0 &&
                        operands_equal(&following->operands[1], &store->operands[0]) &&
                        operands_equal(&following->operands[0], &store->operands[1])) {
                        SPLIT_TO_NONPOPPING_STORE(target->previous);
                        remove_machine_instruction(following);
                    }
                }
            }
            if (SPLIT_OPERATION(branch) == MC_JMP) {
                release_machine_instruction(store);
                continue;
            }
            for (following = branch->next; SPLIT_SKIP(following); following = following->next) ;
            moved = 0;
            if (SPLIT_KIND(following) == 1 && edge_branch && edge_branch->operands[0].value.symbol == following->record.symbol) {
                for (back = edge_branch->previous; SPLIT_SKIP(back); back = back->previous) ;
                if (SPLIT_OPERATION(back) == SPLIT_OPERATION(store) &&
                    operands_equal(&back->operands[0], &store->operands[0]) &&
                    operands_equal(&back->operands[1], &store->operands[1])) {
                    if (return_jump) {
                        --SPLIT_LABEL_SYMBOL(target)->references;
                        remove_machine_instruction(return_jump);
                    }
                    if (SPLIT_LABEL_SYMBOL(following)->references != 1) {
                        label = new_internal_label();
                        created = new_machine_record(1, label);
                        created = insert_machine_after(following->previous, created);
                        register_machine_label(created);
                        SPLIT_LABEL_SYMBOL(created)->references = 1;
                        next_machine_operation(back)->operands[0].value.symbol = created->record.symbol;
                        --SPLIT_LABEL_SYMBOL(following)->references;
                        following = created;
                    }
                    insert_machine_after(following, store);
                    remove_machine_instruction(back);
                    moved = 1;
                }
            }
            if (!moved) insert_machine_after(branch, store);
            for (following = store->next; SPLIT_SKIP(following); following = following->next) ;
            if (SPLIT_KIND(following) == 0 && SPLIT_OPERATION(following) == MC_FLD &&
                SPLIT_OPERATION(store) == MC_FSTP && (store->opcode & 0xf0) != 0xa0 &&
                operands_equal(&following->operands[1], &store->operands[0]) &&
                operands_equal(&following->operands[0], &store->operands[1])) {
                SPLIT_TO_NONPOPPING_STORE(store);
                remove_machine_instruction(following);
            }
        }
        branch = store->next;
    }
}

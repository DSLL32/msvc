#include "functions.h"
#include "global-optimizer.h"
#include "inlining.h"
#include <recon.h>

extern int c2_global_optimize, c2_favor_speed;
C2Expression *__fastcall next_final_marker(C2Expression *record);
void __fastcall redirect_final_symbol_uses(C2Expression *record, C2Expression *target);
int __fastcall prune_after_final_transfer(C2Expression *record);
int __fastcall fold_final_branch_target(C2Expression *record);
int __fastcall invert_final_branch(C2Expression *record);
void __fastcall number_final_peephole_records(C2Function *function)
{
    C2Expression *record, *end;
    unsigned number = 0;
    record = (C2Expression *)function->flow->last->records;
    end = (C2Expression *)function->flow->first->last_record->next;
    for (; record != end; record = record->next) {
        if ((record->flags & 1) || record->kind == 25)
            record->peephole_number = (unsigned short)++number;
    }
}
int __fastcall peephole_432f03(C2Expression *record)
{ __asm { int 3 } return 0; }
C2Expression *__fastcall peephole_432924(C2Expression *record)
{ __asm { int 3 } return NULL; }
int __fastcall peephole_4329c6(C2Expression *record)
{ __asm { int 3 } return 0; }
int __fastcall peephole_432ada(C2Expression *record)
{ __asm { int 3 } return 0; }
int __fastcall peephole_432bbe(C2Expression *record)
{ __asm { int 3 } return 0; }
C2Expression *__fastcall peephole_432ff1(C2Expression *record)
{ __asm { int 3 } return NULL; }
int __fastcall peephole_432be2(C2Expression *first, C2Expression *last)
{ __asm { int 3 } return 0; }
int __fastcall peephole_4363c0(C2Expression *record)
{ __asm { int 3 } return 0; }
C2Expression *__fastcall peephole_4364af(C2Expression *record)
{ __asm { int 3 } return NULL; }
C2Expression *__fastcall resume_final_peephole_scan(C2Expression *record)
{ __asm { int 3 } return NULL; }

int __fastcall optimize_final_record_pass(C2Function *function)
{
    C2Expression *record, *next, *end, *marker, *target, *result;
    unsigned changed, total = 0;

    number_final_peephole_records(function);
    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        changed = 0;
        switch (record->kind) {
        case 25:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            if (!symbol_range_boundary(record) && simplify_symbol_record(record)) {
                changed = 1;
            } else if (c2_global_optimize && record->tail.right) {
                marker = next_final_marker(record);
                target = NULL;
                if (marker && marker->kind == 16 && !marker->detail.data &&
                    marker->opcode != 0x15c && marker->left->kind == 4)
                    target = ((C2InputSymbol *)marker->left->value.symbol)->input_operand;
                if (target && target != record && marker->opcode != 0x15b) {
                    redirect_final_symbol_uses(record, target);
                    changed = 1;
                } else if (!code_generation_state && peephole_432f03(record)) changed = 1;
            }
            break;
        case 18:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            /* Fall through to the transfer pruning shared with kind14. */
        case 14:
            if (prune_after_final_transfer(record)) { changed = 1; next = record; }
            break;
        case 16:
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            if (!record->detail.data && record->opcode != 0x15c &&
                prune_after_final_transfer(record)) {
                changed = 1; next = record;
                break;
            }
            if (record->left->kind != 4 ||
                !((C2InputSymbol *)record->left->value.symbol)->input_operand ||
                record->opcode == 0x15c || record->opcode == 0x15b) break;
            if (fold_final_branch_target(record)) { changed = 1; break; }
            if (!c2_global_optimize) break;
            if (!record->detail.data && record->opcode != 0x15c) {
                if (!code_generation_state) {
                    if (peephole_4363c0(record)) { changed = 1; next = record; }
                    result = peephole_4364af(record);
                    if (result) { ++changed; next = result; }
                }
                break;
            }
            result = peephole_432924(record);
            if (result) { changed = 1; next = result; break; }
            if (invert_final_branch(record) || peephole_4329c6(record)) {
                changed = 1; next = record; break;
            }
            if (code_generation_state) break;
            if (peephole_432ada(record)) { changed = 1; break; }
            if (peephole_432bbe(record)) { changed = 1; next = record; break; }
            marker = peephole_432ff1(record);
            if ((marker->opcode == 0x2f || marker->opcode == 0x33 || marker->opcode == 0x30) &&
                peephole_432be2(marker, record)) { changed = 1; next = record; }
            break;
        }
        if (changed) {
            total += changed;
            next = resume_final_peephole_scan(next);
        }
        record = next;
    }
    return total;
}

void __fastcall optimize_final_record_sequence(C2Function *function)
{
    int pass;
    /* One initial pass plus at most16 further successful-change retries. */
    for (pass = 0; pass <= 16; ++pass)
        if (!optimize_final_record_pass(function)) break;
}

C2Expression *__fastcall previous_peephole_marker(C2Expression *record)
{ __asm { int 3 } return NULL; }
/* The genuine encoding entry receives a20-byte caller-owned scratch buffer.
 * The optional trailing pointer roles remain opaque until that body is reached. */
int __fastcall encode_final_record(C2Expression *record, unsigned char *encoding,
    unsigned char *marks, void *relocations);
int __fastcall final_record_code_size(C2Expression *record)
{
    unsigned char encoding[20];
    return encode_final_record(record, encoding, NULL, NULL);
}
void __fastcall move_record_range_before(C2Expression *destination,
    C2Expression *first, C2Expression *last)
{ __asm { int 3 } }

/* Labels and the inactive0x138 marker may be crossed. Other non-machine
 * records bound the short instruction sequence considered for duplication. */
#define SEQUENCE_MACHINE_RECORD(record) ((record)->opcode == 0x138 || \
    !((record)->flags & 1) || ((record)->opcode > 0 && (record)->opcode < 0x11b))

void __fastcall optimize_final_branch_sequences(C2Function *function)
{
    C2Expression *record, *next, *end, *target, *last, *scan, *previous;
    C2Expression *copy, *source, *destination;
    int saw_label, size;

    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        if (record->kind != 16 || record->detail.data || record->opcode == 0x15c ||
            record->left->kind != 4 ||
            !((C2InputSymbol *)record->left->value.symbol)->input_operand ||
            fold_final_branch_target(record)) goto next_sequence;
        target = ((C2InputSymbol *)record->left->value.symbol)->input_operand;
        saw_label = 0;
        for (last = target->next; last; last = last->next) {
            if (last->opcode == 0x138) continue;
            if (!SEQUENCE_MACHINE_RECORD(last)) break;
            switch (last->kind) {
            case 14: case 16: case 18: case 23: goto sequence_end;
            case 25: saw_label = 1; break;
            }
        }
sequence_end:
        if (last->kind == 23 || !SEQUENCE_MACHINE_RECORD(last) ||
            last->kind == 18 || last == record) goto next_sequence;
        if (last->kind == 16 && (last->detail.data || last->opcode == 0x15c ||
            last->left->kind != 4 ||
            !((C2InputSymbol *)last->left->value.symbol)->input_operand)) goto next_sequence;
        previous = previous_peephole_marker(target);
        if (!saw_label && previous->kind == 16 && !previous->detail.data &&
            previous->opcode != 0x15c &&
            (previous->left->kind != 4 ||
             ((C2InputSymbol *)previous->left->value.symbol)->input_operand != target)) {
            target->previous->next = last->next;
            last->next->previous = target->previous;
            last->next = NULL;
            link_code_nodes((C2Record *)record, (C2Record *)target);
            unlink_and_release_input_record((C2Record *)record);
            next = last;
            goto next_sequence;
        }
        size = 0;
        scan = target->next;
        do {
            if (scan->flags & 1) size += final_record_code_size(scan);
            if (scan == last) break;
            scan = scan->next;
        } while (size <= (c2_favor_speed ? 20 : 2));
        if (size > (c2_favor_speed ? 20 : 2)) goto next_sequence;
        source = target->next;
        destination = record;
        for (;;) {
            if (source->kind != 25) {
                copy = copy_input_expression(source);
                insert_input_record((C2Record *)destination, (C2Record *)copy);
                if (copy->kind == 16 && copy->left->kind == 4 &&
                    ((C2InputSymbol *)copy->left->value.symbol)->input_operand)
                    bind_symbol_input_use(((C2InputSymbol *)copy->left->value.symbol)->input_operand, copy);
                destination = copy;
            }
            if (source == last) break;
            source = source->next;
        }
        unlink_and_release_input_record((C2Record *)record);
        if (destination->kind == 16 && !destination->detail.data && destination->opcode != 0x15c)
            next = destination;
next_sequence:
        record = next;
    }

    end = (C2Expression *)function->flow->first->last_record;
    record = (C2Expression *)function->flow->last->records->next;
    while (record != end) {
        next = record->next;
        if (record->kind != 16 || record->detail.data || record->opcode == 0x15c ||
            record->left->kind != 4 ||
            !(target = ((C2InputSymbol *)record->left->value.symbol)->input_operand) ||
            next->kind != 25) goto next_range;
        previous = target->previous;
        if ((previous->kind != 16 || previous->detail.data || previous->opcode == 0x15c) &&
            previous->kind != 14) goto next_range;
        for (scan = next; scan && scan != target; scan = scan->next) ;
        if (scan) {
            destination = ((C2Expression *)function->flow->first->last_record)->previous;
            move_record_range_before(destination, next, previous);
            unlink_and_release_input_record((C2Record *)record);
        }
next_range:
        record = next;
    }
}

void __fastcall prune_final_transfer_ranges(C2Function *function)
{
    C2Expression *record, *next, *end;

    record = (C2Expression *)function->flow->last->records->next;
    end = (C2Expression *)function->flow->first->last_record;
    while (record != end) {
        next = record->next;
        if ((record->kind == 14 ||
            (record->kind == 16 && !record->detail.data && record->opcode != 0x15c)) &&
            prune_after_final_transfer(record)) next = record;
        record = next;
    }
}

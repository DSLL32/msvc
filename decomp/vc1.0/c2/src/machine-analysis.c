/* Structured regions over the selected machine stream. */
#include "machine-analysis.h"
#include "backend-passes.h"
#include "instruction-selection.h"
#include "machine-layout.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "bitsets.h"
#include "jump-threading.h"
#include "transformation-cursor.h"
#include <recon.h>
MachineFlowRegion *active_machine_region; /* 000830c8 */
MachineBlockHeader *region_insertion_block; /*000830d4: enclosing insertion block*/
MachineFlowRegion *machine_region_root; /* 000830c4 */
int machine_analysis_needs_label_cleanup; /* 0007c29c */

/* Original [00048040,00048070). Rebuild regions before processing children;
   later label cleanup is required only if a transformation drops references. */
void __fastcall analyze_machine_instructions(MachineInstruction *instructions, unsigned flags)
{
    machine_analysis_needs_label_cleanup = 0;
    build_machine_regions(instructions);
    if (machine_region_root->children) analyze_machine_regions(machine_region_root, flags);
}

/* Original [000480e0,000483a0), including its11-entry record-kind table.
   Build nested regions from function/label boundaries and references. Block
   headers before a boundary retain the native0x200 marking and flag barriers. */
void __fastcall build_machine_regions(MachineInstruction *instructions)
{
    MachineFlowRegion *region = 0;
    MachineInstruction *instruction, *scan, *boundary;
    MachineBlockHeader *block;
    SymbolHeader *label;
    unsigned kind, scan_kind, operation;
    int references;
    for (instruction = instructions; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        switch (kind) {
        case 0: case 8: case 9: case 10:
            label = machine_operand_label(instruction);
            operation = (unsigned char)(instruction->opcode >> 8);
            if (label && operation != MC_DATA)
                region = process_machine_region_reference(instruction, region, label);
            else if (operation == MC_CALL) region->flags |= 8;
            else if ((operation >= MC_FADD && operation <= MC_FLD1) ||
                     (operation >= MC_FUCOM && operation <= MC_FCOS)) region->flags |= 4;
            break;
        case 1:
            label = instruction->record.symbol;
            if (label->field0d == 1) break;
            instruction->labels.attachment = 0;
            if (!region) compiler_internal_error("@(#)range.c:1.0", 0x6a);
            references = ((LabelSymbol *)label)->references - count_region_label_references(region, label);
            if ((!region->references[0] && !region->references[1]) || references) {
                boundary = instruction;
                scan = instruction->previous;
                for (;;) {
                    scan_kind = scan->opcode & 15;
                    if (scan_kind == 10 || scan_kind == 8 || scan_kind == 9 ||
                        scan_kind == 0 || scan_kind == 1 || scan_kind == 2) break;
                    if (scan_kind == 7) {
                        boundary = scan;
                        block = (MachineBlockHeader *)scan;
                        if (!region->references[0] && !region->references[1]) block->flags |= 0x200;
                        if (block->flags & 3) break;
                    }
                    scan = scan->previous;
                }
                if (references) {
                    operation = (unsigned char)(scan->opcode >> 8);
                    if ((scan_kind == 10 || scan_kind == 8 || scan_kind == 9 || scan_kind == 0) &&
                        (operation == MC_JCC || operation == MC_JMP) && (boundary->opcode & 15) == 7) {
                        boundary = instruction->previous;
                        while ((boundary->opcode & 15) != 7) boundary = boundary->previous;
                    }
                    region = new_machine_region(region, boundary, instruction);
                    region->pending_references = region->initial_references = references;
                    if ((boundary->opcode & 15) != 7 || ((MachineBlockHeader *)boundary)->flags & 3)
                        region->flags |= 1;
                    break;
                }
            }
            region->flags |= 0x40;
            break;
        case 2:
            boundary = instruction->previous;
            for (;;) {
                scan_kind = boundary->opcode & 15;
                if (scan_kind == 7 || scan_kind == 10 || scan_kind == 8 || scan_kind == 9 || scan_kind == 0) break;
                boundary = boundary->previous;
            }
            if (scan_kind != 7) compiler_internal_error("@(#)range.c:1.0", 0xae);
            ((MachineBlockHeader *)boundary)->flags |= 0x200;
            region = new_machine_region(region, boundary, instruction);
            region->pending_references = 0;
            machine_region_root = region;
            break;
        case 3:
            if (machine_region_root != region) compiler_internal_error("@(#)range.c:1.0", 0xa5);
            region->last = instruction;
            region->flags |= 1;
            break;
        case 7:
            block = (MachineBlockHeader *)instruction;
            if (region && !region->references[0] && !region->references[1]) block->flags |= 0x200;
            if (block->flags & 3) region->flags |= 1;
            break;
        }
    }
}

/* Original [000483a0,00048400). Clear the complete96-byte arena2 record,
   attach it to its parent, and bind the stream boundary and label metadata. */
MachineFlowRegion *__fastcall new_machine_region(MachineFlowRegion *parent, MachineInstruction *boundary, MachineInstruction *label)
{
    MachineFlowRegion *region = arena_alloc(2, sizeof(MachineFlowRegion));
    unsigned i;
    region->parent = parent;
    region->next = region->children = 0;
    region->first = region->last = region->label = region->exit_label = 0;
    region->references[0] = region->references[1] = region->references[2] = 0;
    for (i = 0; i < 8; ++i) region->registers[i] = 0;
    region->first_block = region->last_block = 0;
    region->pending_references = region->initial_references = 0;
    region->field58 = region->flags = 0;
    if (parent) replace_machine_region_suffix(parent, 0, region);
    region->first = boundary;
    if ((boundary->opcode & 15) == 7)
        region->first_block = region->last_block = ((MachineBlockHeader *)boundary)->number;
    region->label = label;
    label->labels.attachment = region;
    return region;
}

/* Original [00048590,00048600). Find a local label in explicit operands,
   visiting the second operand first. Address forms carry it in their typed
   symbol field; direct symbol operands carry the descriptor itself. */
SymbolHeader *__fastcall machine_operand_label(MachineInstruction *instruction)
{
    int count = machine_operand_counts[(unsigned char)(instruction->opcode >> 8)];
    MachineOperand *operand;
    SymbolHeader *symbol;
    unsigned kind;
    while (count) {
        operand = &instruction->operands[count == 1 ? 0 : 1];
        kind = operand->flags & 15;
        if (kind >= OPERAND_ABSOLUTE && kind <= OPERAND_LABEL_ADDRESS) {
            symbol = operand->value.address->symbol;
            if (symbol && symbol->kind == 3) return symbol;
        } else if (kind == OPERAND_SYMBOL) {
            symbol = operand->value.symbol;
            if (symbol->kind == 3) return symbol;
        }
        count = (int)((unsigned)count - 1);
    }
    return 0;
}

/* Original [00048440,00048590). Resolve references against open regions.
   A region closes when its entry count reaches zero; remaining references
   move from its second pending chain to the first in their native order. */
MachineFlowRegion *__fastcall process_machine_region_reference(MachineInstruction *instruction, MachineFlowRegion *region, SymbolHeader *label)
{
    MachineFlowRegion *scan, *parent;
    MachineRegionEdge *tail;
    unsigned operation;
    if (label->field0d == 1) return process_special_machine_region_reference(region, label);
    for (scan = region; scan; scan = scan->parent) {
        parent = scan->parent;
        if (scan->label->record.symbol != label) continue;
        --scan->pending_references;
        scan->last = instruction;
        if (!scan->pending_references) {
            if (scan != region) {
                add_machine_region_reference(region, label, 2);
                return close_machine_region(parent, scan) ? parent : region;
            }
            operation = (unsigned char)(instruction->opcode >> 8);
            if (operation != MC_JCC && operation != MC_JMP) {
                close_machine_region(parent, scan);
                return parent;
            }
            if ((instruction->operands[0].flags & 15) != OPERAND_SYMBOL)
                compiler_internal_error("@(#)range.c:1.0", 0x118);
            if (scan->references[1]) {
                for (tail = scan->references[1]; tail->next; tail = tail->next) ;
                tail->next = scan->references[0];
                scan->references[0] = scan->references[1];
                scan->references[1] = 0;
            }
            finish_closed_machine_region(region, parent);
            return parent;
        }
        if (scan->references[1]) {
            for (tail = scan->references[1]; tail->next; tail = tail->next) ;
            tail->next = scan->references[0];
            scan->references[0] = scan->references[1];
            scan->references[1] = 0;
        }
        if (scan != region) add_machine_region_reference(region, label, 2);
        break;
    }
    if (!scan) add_machine_region_reference(region, label, region->last != 0);
    return region;
}

/* Original [00048600,00048630). Each of the three native pending-reference
   chains owns arena2 eight-byte next/label records, inserted at its head. */
MachineRegionEdge *__fastcall add_machine_region_reference(MachineFlowRegion *region, SymbolHeader *label, unsigned chain)
{
    MachineRegionEdge *reference = arena_alloc(2, sizeof(MachineRegionEdge));
    reference->label = label;
    reference->next = region->references[chain];
    region->references[chain] = reference;
    return reference;
}

/* Original [00048400,00048440). Count and consume matching forward
   references from the first two chains. The third chain remains untouched;
   removed nodes retain arena ownership and are not individually freed. */
int __fastcall count_region_label_references(MachineFlowRegion *region, SymbolHeader *label)
{
    MachineRegionEdge *reference, **link;
    unsigned chain, count = 0;
    for (chain = 0; chain < 2; ++chain) {
        link = &region->references[chain];
        for (reference = *link; reference; reference = reference->next) {
            if (reference->label == label) {
                ++count;
                *link = reference->next;
            } else link = &reference->next;
        }
    }
    return (int)count;
}

/* Original [000488e0,00048916). Replace the suffix beginning at before
   (or append when before is null), then adopt every inserted sibling. */
void __fastcall replace_machine_region_suffix(MachineFlowRegion *parent,
    MachineFlowRegion *before, MachineFlowRegion *regions)
{
    MachineFlowRegion *previous = parent->children, *region;
    if (previous == before) parent->children = regions;
    else {
        while (previous->next != before) previous = previous->next;
        previous->next = regions;
    }
    for (region = regions; region; region = region->next) region->parent = parent;
}

/* Original [00048690,00048783). Mark outward target labels, preserve a
   unique common exit, merge outgoing reference chains into the parent, and
   update the region's block-number bounds while skipping nested regions. */
void __fastcall finish_closed_machine_region(MachineFlowRegion *region, MachineFlowRegion *parent)
{
    MachineRegionEdge *edge;
    MachineFlowRegion *children;
    SymbolHeader *common = 0;
    MachineInstruction *instruction;
    int multiple = 0, number;
    edge = region->references[2];
    if (edge) {
        region->flags |= 0x10;
        common = edge->label;
        for (; edge; edge = edge->next) {
            label_machine_instruction((LabelSymbol *)edge->label)->labels.flags |= 2;
            if (edge->label != common) multiple = 1;
        }
    }
    edge = region->references[0];
    if (edge) {
        region->flags |= 0x10;
        if (!common) common = edge->label;
        for (; edge; edge = edge->next) {
            label_machine_instruction((LabelSymbol *)edge->label)->labels.flags |= 2;
            if (edge->label != common) multiple = 1;
        }
    }
    if ((region->flags & 0x10) && !multiple)
        region->exit_label = label_machine_instruction((LabelSymbol *)common);
    transfer_machine_region_references(region, parent);
    children = region->children;
    instruction = next_machine_region_record(7, region, region->first, &children);
    while (instruction && instruction != region->last) {
        number = ((MachineBlockHeader *)instruction)->number;
        if (number != -1) {
            if (region->first_block >= number) region->first_block = number;
            if (region->last_block <= number) region->last_block = number;
        }
        instruction = next_machine_region_record(7, region, instruction, &children);
    }
}

/* Original [00048920,00048962). Move both pending outgoing chains into
   the parent's open or closed reference bucket, preserving each chain order. */
void __fastcall transfer_machine_region_references(MachineFlowRegion *region, MachineFlowRegion *parent)
{
    unsigned chain = parent->last ? 1 : 0, i;
    MachineRegionEdge *head, *tail;
    for (i = 0; i < 2; ++i) {
        head = region->references[i];
        if (head) {
            for (tail = head; tail->next; tail = tail->next) ;
            tail->next = parent->references[chain];
            parent->references[chain] = head;
            region->references[i] = 0;
        }
    }
}

/* Original [00048970,00048a1c). Scan after previous, skipping nested
   unremoved regions through a caller-owned child cursor. Kind0 also accepts
   the three extended machine instruction kinds8/9/10. */
MachineInstruction *__fastcall next_machine_region_record(unsigned kind, MachineFlowRegion *region,
    MachineInstruction *previous, MachineFlowRegion **children)
{
    MachineInstruction *instruction, *boundary;
    MachineFlowRegion *child;
    unsigned candidate_kind;
    while (*children && ((*children)->flags & 2)) *children = (*children)->next;
    boundary = *children ? (*children)->first : region->last->next;
    instruction = previous->next;
    for (;;) {
        while (instruction == boundary) {
            if (instruction == region->last->next) return 0;
            child = *children;
            instruction = child->last->next;
            *children = child->next;
            while (*children && ((*children)->flags & 2)) *children = (*children)->next;
            /* The original retains the last boundary when the cursor empties;
               callers stop on their region's last matching record. */
            if (*children) boundary = (*children)->first;
        }
        candidate_kind = instruction->opcode & 15;
        if (candidate_kind == kind || (!kind && candidate_kind >= 8 && candidate_kind <= 10))
            return instruction;
        instruction = instruction->next;
    }
}

/* Original [00048070,000480d3). Process child regions inside out; optional
   transformations require an unrestricted region, while local simplification
   always runs after the child graph is ready. */
void __fastcall analyze_machine_regions(MachineFlowRegion *region, unsigned flags)
{
    MachineFlowRegion *child;
    for (child = region->children; child; child = child->next) {
        if (child->children) analyze_machine_regions(child, flags);
        if (((LabelSymbol *)child->label->record.symbol)->references != child->initial_references &&
            flags && !(child->flags & 1)) prepare_machine_region_transform(child);
        simplify_machine_region_operations(child);
        if (flags && !(child->flags & 1)) transform_machine_region_blocks(child);
    }
}

/* Original [00048ba0,00048e17). Summarize local register reads/writes,
   combine nested-region invariance/liveness, and recognize a single constant
   ADD/SUB update whose register is live at region entry. */
void __fastcall simplify_machine_region_operations(MachineFlowRegion *region)
{
    MachineFlowRegion *children = region->children;
    MachineInstruction *instruction, *definition;
    MachineOperand *left, *right;
    MachineRegionRegister *record;
    unsigned opcode, kind, flags, i;
    int count;
    instruction = next_machine_region_record(0, region, region->first, &children);
    while (instruction != region->last) {
        opcode = (unsigned char)(instruction->opcode >> 8);
        count = machine_operand_counts[opcode];
        left = count ? &instruction->operands[0] : 0;
        right = count == 2 ? &instruction->operands[1] : 0;
        if (right) {
            kind = right->flags & 15;
            switch (kind) {
            case OPERAND_REGISTER:
                if (!overwrites_second_register(instruction, 0)) {
                    record = find_machine_region_register(region, right->value.reg.words.number, 1);
                    ++record->uses;
                }
                break;
            case OPERAND_INDEXED:
                record = find_machine_region_register(region, right->value.address->index.words.number, 1);
                ++record->uses;
                /* Fall through: indexed addresses also use their base. */
            case OPERAND_BASED:
                record = find_machine_region_register(region, right->value.address->base.words.number, 1);
                ++record->uses;
                break;
            }
        }
        if (left) {
            kind = left->flags & 15;
            switch (kind) {
            case OPERAND_REGISTER:
                record = find_machine_region_register(region, left->value.reg.words.number, 1);
                if (writes_first_operand(instruction)) {
                    ++record->definitions;
                    if (record->definitions == 1) record->definition = instruction;
                }
                if (!overwrites_first_register(instruction, 0)) ++record->uses;
                break;
            case OPERAND_INDEXED:
                record = find_machine_region_register(region, left->value.address->index.words.number, 1);
                ++record->uses;
                /* Fall through: indexed addresses also use their base. */
            case OPERAND_BASED:
                record = find_machine_region_register(region, left->value.address->base.words.number, 1);
                ++record->uses;
                break;
            }
        }
        instruction = next_machine_region_record(0, region, instruction, &children);
    }
    for (i = 0; i < 8; ++i) {
        for (record = region->registers[i]; record; record = record->next) {
            flags = record->flags;
            record->flags = (flags & ~2U) |
                ((register_is_invariant_in_regions(record->number, region->children) * 2U) & 2);
            if (region->flags & 1) record->flags &= ~8U;
            else {
                flags = record->flags;
                record->flags = (flags & ~8U) |
                    ((register_dead_after_region(record->number, region, 1) * 8U) & 8);
            }
            if (!record->definitions && (record->flags & 2)) record->flags |= 1;
            else if (record->definitions == 1 && (record->flags & 2)) {
                definition = record->definition;
                opcode = (unsigned char)(definition->opcode >> 8);
                if ((opcode == MC_ADD || opcode == MC_SUB) &&
                    (definition->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                    !(region->flags & 1) && instruction_precedes_region_entry(definition) &&
                    bitset_contains(((MachineBlockHeader *)region->first)->uses, record->number & 0x3fff)) {
                    record->flags |= 4;
                    record->increment = definition->operands[1].value.constant;
                    if (opcode != MC_ADD) record->increment = 0U - record->increment;
                }
            }
        }
    }
}

/* Original [00049150,000491cc). Intern a region's full16-bit register
   identity in eight buckets; lookup-only calls never allocate a record. */
MachineRegionRegister *__fastcall find_machine_region_register(MachineFlowRegion *region,
    unsigned short number, int create)
{
    MachineRegionRegister **bucket = &region->registers[number & 7], *record;
    for (record = *bucket; record; record = record->next)
        if (record->number == number) return record;
    if (!create) return 0;
    record = arena_alloc(2, sizeof(MachineRegionRegister));
    record->number = number;
    record->next = *bucket;
    *bucket = record;
    record->uses = 0;
    record->definitions = 0;
    record->definition = 0;
    record->flags &= ~0x17U;
    return record;
}

/* Original [000490e0,0004914e). A region's own register summary supersedes
   its children; otherwise every nested child and later sibling must agree. */
int __fastcall register_is_invariant_in_regions(unsigned short number, MachineFlowRegion *regions)
{
    MachineRegionRegister *record;
    if (!regions) return 1;
    record = find_machine_region_register(regions, number, 0);
    if (record)
        return (record->flags & 1) && register_is_invariant_in_regions(number, regions->next);
    return register_is_invariant_in_regions(number, regions->children) &&
           register_is_invariant_in_regions(number, regions->next);
}

/* Original [00048ff0,000490d4). Flagged virtual values must be absent
   from the entry and all visible exits before the region can discard them.
   Physical registers and the excluded virtual class are immediate successes. */
int __fastcall register_dead_after_region(unsigned short number, MachineFlowRegion *region, int check_entry)
{
    MachineInstruction *instruction;
    unsigned identifier = number & 0x3fff;
    if ((number & 0x8000) || !(number & 0xc000)) return 1;
    if (check_entry && bitset_contains(((MachineBlockHeader *)region->first)->uses, identifier))
        return 0;
    if (region->flags & 0x10) {
        instruction = region->exit_label;
        if (!instruction) return 0;
        while ((instruction->opcode & 15) != 7) instruction = instruction->previous;
        if (bitset_contains(((MachineBlockHeader *)instruction)->uses, identifier)) return 0;
    }
    if ((unsigned char)(region->last->opcode >> 8) != MC_JMP) {
        instruction = region->last->next;
        while ((instruction->opcode & 15) != 7) instruction = instruction->next;
        if (bitset_contains(((MachineBlockHeader *)instruction)->uses, identifier)) return 0;
    }
    return 1;
}

/* Original [00048fb0,00048fee). A referenced label before this instruction
   provides an entry; otherwise the preceding block header supplies its flag. */
int __fastcall instruction_precedes_region_entry(MachineInstruction *instruction)
{
    MachineInstruction *previous = instruction->previous;
    while ((previous->opcode & 15) != 7) {
        if ((previous->opcode & 15) == 1 && previous->labels.attachment) return 1;
        previous = previous->previous;
    }
    return ((MachineBlockHeader *)previous)->flags & 0x200;
}

/* Original [00048790,000487f1). A singly referenced table label owns
   consecutive DATA records. Account for each destination as an ordinary
   region reference, then stop at the first different record or empty target. */
MachineFlowRegion *__fastcall process_special_machine_region_reference(MachineFlowRegion *region, SymbolHeader *label)
{
    MachineInstruction *instruction;
    SymbolHeader *target;
    if (((LabelSymbol *)label)->references != 1)
        compiler_internal_error("@(#)range.c:1.0", 0x240);
    instruction = label_machine_instruction((LabelSymbol *)label)->next;
    while (!(instruction->opcode & 15) &&
           (unsigned char)(instruction->opcode >> 8) == MC_DATA) {
        target = machine_operand_label(instruction);
        if (!target) break;
        region = process_machine_region_reference(instruction, region, target);
        instruction = instruction->next;
    }
    return region;
}

/* Original48e20..48ed7, ECX region/RET0. Extra incoming references
   receive a separate entry label before the preheader; the region's original
   reference count remains with the old label. */
void __fastcall prepare_machine_region_transform(MachineFlowRegion *region)
{
    MachineInstruction *label = region->label, *before, *created;
    LabelSymbol *symbol = (LabelSymbol *)label->record.symbol;
    int transferred;
    if (symbol->references == region->initial_references) return;
    before = region->first->previous;
    while ((before->opcode & 15) == 4) before = before->previous;
    created = new_machine_record(1, new_internal_label());
    created->labels.attachment = 0;
    insert_machine_after(before, created);
    register_machine_label(created);
    transferred = retarget_machine_region_entry(symbol, (LabelSymbol *)created->record.symbol,
        region->parent->first, before);
    created->labels.flags = (created->labels.flags & ~2U) | (label->labels.flags & 2);
    created->labels.flags &= ~2U;
    if (region->initial_references + transferred != symbol->references)
        compiler_internal_error("@(#)range.c:1.0", 0x2d0);
    ((LabelSymbol *)region->label->record.symbol)->references -= transferred;
    ((LabelSymbol *)created->record.symbol)->references = transferred;
}

/* Original49230..4927a, ECX region/RET0. Keep both the instruction
   insertion cursor and its preceding block owner. The final forward walk is
   present in the original even though its terminal value is not published. */
void __fastcall prepare_region_insertion(MachineFlowRegion *region)
{
    MachineInstruction *position, *scan;
    active_machine_region = region;
    position = region->first->previous;
    region_insertion_block = (MachineBlockHeader *)position;
    while ((region_insertion_block->opcode & 15) != 7)
        region_insertion_block = (MachineBlockHeader *)region_insertion_block->previous;
    transformation_cursor.instruction = position;
    for (scan = position->next; scan != region->label; scan = scan->next) ;
}

/* Original4ab40..4aca6, ECX region/EDX register, add and preserved block
   on stack, RET8. Keep both the WorkItem sets and their machine-header aliases
   synchronized. Removal deliberately leaves the preserved block definition. */
void __fastcall record_region_register(MachineFlowRegion *region, unsigned short number,
    int add, MachineBlockHeader *preserved_definition)
{
    MachineBlockHeader *before, *last, *block;
    WorkItem *work;
    unsigned member = number & 0x3fff;
    before = (MachineBlockHeader *)region->first->previous;
    while ((before->opcode & 15) != 7) before = (MachineBlockHeader *)before->previous;
    last = (MachineBlockHeader *)region->last;
    while ((last->opcode & 15) != 7) last = (MachineBlockHeader *)last->previous;
    for (work = current_work; work->body.block != before; work = work->next) ;
    if ((MachineInstruction *)work->next->body.block != region->first)
        compiler_internal_error("@(#)range.c:1.0", 0x8c1);
    if (add) {
        work->pass.selection.definitions = bitset_include(work->pass.selection.definitions, member);
        before->definitions = bitset_include(before->definitions, member);
    } else {
        bitset_remove(work->pass.selection.definitions, member);
        bitset_remove(before->definitions, member);
    }
    do {
        work = work->next;
        block = work->body.block;
        if (add) {
            work->pass.selection.uses = bitset_include(work->pass.selection.uses, member);
            block->uses = bitset_include(block->uses, member);
            work->pass.selection.definitions = bitset_include(work->pass.selection.definitions, member);
            block->definitions = bitset_include(block->definitions, member);
        } else {
            bitset_remove(work->pass.selection.uses, member);
            bitset_remove(block->uses, member);
            if (block != preserved_definition) {
                bitset_remove(work->pass.selection.definitions, member);
                bitset_remove(block->definitions, member);
            }
        }
    } while (block != last);
}

/* Original4a010..4a071, generation ECX/old record EDX/new record stack,
   RET4. Replace matching records in the deferred generation's instruction
   run. Save each next link before the arena record is released. */
void __fastcall replace_machine_definition(unsigned identifier,
    MachineInstruction *previous, MachineInstruction *replacement)
{
    MachineInstruction *instruction, *next, *copy;
    for (instruction = deferred_instructions; instruction->opcode >> 24 != identifier;
         instruction = instruction->next) ;
    while (!(instruction->opcode & 15)) {
        next = instruction->next;
        if (machine_records_equal(previous, instruction)) {
            copy = clone_machine_record(replacement);
            insert_machine_after(instruction, copy);
            remove_machine_instruction(instruction);
        }
        instruction = next;
    }
}

int __fastcall resolve_machine_region_suffix(MachineFlowRegion *parent, MachineFlowRegion *region);
/* Original48630..48690. Detach the closed label owner, merge its
   outgoing references, and replace it with its nested region suffix. The
   final nested sibling can require another reference-resolution step. */
int __fastcall close_machine_region(MachineFlowRegion *parent, MachineFlowRegion *region)
{
    MachineFlowRegion *last;
    region->label->labels.attachment = 0;
    if (region->flags & 1) parent->flags |= 1;
    transfer_machine_region_references(region, parent);
    replace_machine_region_suffix(parent, region, region->children);
    last = region->children;
    if (last) {
        while (last->next) last = last->next;
        if (last->pending_references) return resolve_machine_region_suffix(parent, last);
    }
    return 1;
}

/* Original48800..488de. Consume parent references, keep unresolved nested
   entries outside the closing prefix, and continue resolving the first
   remaining pending sibling. Each region retains its real arena owner. */
int __fastcall resolve_machine_region_suffix(MachineFlowRegion *parent, MachineFlowRegion *region)
{
    MachineFlowRegion *children, *attached;
    MachineInstruction *instruction, *next;
    int consumed;
    for (;;) {
        consumed = count_region_label_references(parent, region->label->record.symbol);
        region->initial_references -= consumed;
        region->pending_references -= consumed;
        if (region->pending_references) return 0;
        if (!region->last) return close_machine_region(parent, region);
        children = region->children;
        instruction = region->label->next;
        while (children && instruction != region->last) {
            next = instruction->next;
            if ((instruction->opcode & 15) == 1) {
                attached = instruction->labels.attachment;
                if (attached) {
                    if (attached->pending_references) return close_machine_region(parent, region);
                    if (children == attached) {
                        next = children->last->next;
                        children = children->next;
                    }
                }
            }
            instruction = next;
        }
        if (children) {
            replace_machine_region_suffix(parent, 0, children);
            replace_machine_region_suffix(region, children, 0);
        }
        finish_closed_machine_region(region, parent);
        while (children && !children->pending_references) children = children->next;
        if (!children) return 1;
        region = children;
    }
}

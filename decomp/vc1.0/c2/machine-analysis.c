/* Structured regions over the selected machine stream. */
#include "machine-analysis.h"
#include "backend-passes.h"
#include "instruction-selection.h"
MachineFlowRegion *active_machine_region; /* 000830c8 */
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
    region->parent = parent;
    region->next = region->children = 0;
    region->first = region->last = region->label = region->exit_label = 0;
    region->references[0] = region->references[1] = region->references[2] = 0;
    region->field28 = region->field2c = region->field30 = 0;
    region->field34 = region->field38 = region->field3c = region->field40 = region->field44 = 0;
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

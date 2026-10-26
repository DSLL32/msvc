/* Machine-code offsets and source/debug positions. */
#include "backend-passes.h"
#include "machine-layout.h"
#include "records.h"
#include "function-input.h"
#include "instruction-encoding.h"
#include "register-use.h"
#include <recon.h>
extern int optimize_global, codeview_debug, string_blocks;
/* Original [000685d0,00068870), including its twelve-entry kind table. */
void __fastcall layout_machine_stream(MachineInstruction *instruction, unsigned apply)
{
    SymbolHeader *segment = lookup_global_symbol(current_symbol->symbol.field18.identifier);
    DebugScope *scope = current_function->debug_scope, *nested = 0;
    MachineInstruction *next;
    int offset = -1, excluded = 0, prologue_pending = 0;
    unsigned length;
    short control;
    current_source_location = 0;
    if (codeview_debug && !optimize_global) scope = scope->next;
    for (; instruction; instruction = next) {
        next = instruction->next;
        switch (instruction->opcode & 15) {
        case 0:
            length = machine_instruction_size(instruction);
            instruction->record.encoding_length = (instruction->record.encoding_length & ~0x3ffU) | (length & 0x3ff);
            offset += instruction->record.encoding_length & 0x3ff;
            break;
        case 1:
            instruction->record.symbol->field18.identifier = current_symbol->symbol.field18.identifier;
            if (!excluded) offset = align_machine_location(offset, instruction, apply);
            instruction->record.symbol->field04 = (unsigned)offset;
            break;
        case 2:
            offset = string_blocks ? align_machine_location((int)segment->field04, instruction, 0) : (int)segment->field04;
            current_symbol->symbol.field04 = (unsigned)offset;
            function_source_line = current_source_location;
            prologue_pending = 1;
            break;
        case 3:
            offset = align_machine_location(offset, instruction, apply);
            current_symbol->code_bytes = offset == -1 ? 0 : (unsigned)offset - current_symbol->symbol.field04;
            break;
        case 4:
            current_source_location = instruction->record.source_location;
            break;
        case 5:
            if (!function_source_filename) function_source_filename = instruction->record.filename;
            break;
        case 7:
            excluded = ((MachineBlockHeader *)instruction)->flags & 2;
            break;
        case 11:
            control = instruction->record.directive.code;
            if (control == 0xf) {
                if (prologue_pending) {
                    prologue_pending = 0;
                    current_symbol->prologue_bytes = offset == -1 ? 0 :
                        (unsigned short)((unsigned)offset - current_symbol->symbol.field04);
                }
                if (!optimize_global && codeview_debug) {
                    scope = scope->next;
                    scope->start_offset = (unsigned)offset;
                    scope->parent = nested;
                    nested = scope;
                }
            } else if (control == 0x10) {
                if (!optimize_global) {
                    nested->end_offset = (unsigned)offset;
                    nested = nested->parent;
                }
            } else if (control == 0x12)
                current_symbol->epilogue_offset = offset == -1 ? 0 :
                    (unsigned short)((unsigned)offset - current_symbol->symbol.field04);
            else if (control == 0x18) offset = align_machine_location(offset, instruction, apply);
            break;
        }
    }
    if (!function_source_filename) function_source_filename = listing_source_filename;
}

/* Alignment and padding of the selected machine stream. */
#include "machine-layout.h"
#include "instruction-rewrite.h"
#include <recon.h>
extern int optimization_align, optimize_speed, optimize_global, byte_block_second, cpu_model;
/* Original [00032bb0,00032e60), including its eight-entry padding table. */
int __fastcall align_machine_location(int offset, MachineInstruction *instruction, unsigned apply)
{
    unsigned kind = instruction->opcode & 15, padding = 0, alignment, loop, piece;
    int bytes, boundary;
    MachineInstruction *following, *second, *previous, *inserted;
    if (!optimization_align && kind != 11) return offset;
    if ((!optimize_speed || !optimize_global || byte_block_second) && kind != 11) return offset;
    switch (kind) {
    case 1:
        if (instruction->record.symbol->field0d == 1) padding = (0U - (unsigned)offset) & 3;
        else {
            loop = label_has_fallthrough_predecessor(instruction);
            if (!loop || (instruction->operands[1].flags & 1)) {
                if (cpu_model < 4) {
                    if (!loop || (int)(((unsigned)offset + 3) & ~3U) <
                        (int)(((unsigned)offset + 5) & ~3U)) padding = (0U - (unsigned)offset) & 3;
                } else {
                    following = next_machine_operation(instruction);
                    bytes = (int)machine_instruction_size(following);
                    boundary = (int)(((unsigned)offset + 15) & ~15U);
                    if (boundary < (int)(((unsigned)offset + bytes + 15) & ~15U))
                        padding = (0U - (unsigned)offset) & 15;
                    else if (bytes < 8 && (!loop || alignment_lookahead(instruction))) {
                        second = next_machine_operation(following);
                        if (second) bytes += machine_instruction_size(second);
                        if (bytes > 8) bytes = 8;
                        if (boundary < (int)(((unsigned)offset + bytes + 15) & ~15U))
                            padding = (0U - (unsigned)offset) & 15;
                    }
                }
            }
        }
        break;
    case 2: case 3:
        alignment = cpu_model < 4 ? 4 : 16;
        padding = (alignment - (unsigned)offset) & (alignment - 1);
        break;
    case 11:
        alignment = (unsigned short)instruction->record.directive.value;
        padding = (alignment - (unsigned)offset) & (alignment - 1);
        break;
    default:
        compiler_internal_error("@(#)pgoMD.c:1.49", 0x12b);
        break;
    }
    offset += padding;
    if (!apply || !padding) return offset;
    previous = instruction->previous;
    while ((previous->opcode & 15) && (previous->opcode & 15) != 1) previous = previous->previous;
    while (padding) {
        piece = padding;
        switch (padding) {
        case 1: case 3: case 4:
            if (extend_previous_instruction(previous, padding)) { padding = 0; continue; }
            break;
        case 2: case 5: case 6: break;
        case 8: piece = 4; break;
        default: piece = 7; break;
        }
        inserted = new_machine_record(0, MC_NOP, piece);
        insert_machine_after(previous, inserted);
        inserted->record.encoding_length = (inserted->record.encoding_length & ~0x3ffU) | piece;
        padding -= piece;
    }
    return offset;
}

/* Original [00068c10,00068c30). Advance at least once and skip every
   non-operation record, including labels, line markers and block headers. */
MachineInstruction *__fastcall next_machine_operation(MachineInstruction *instruction)
{
    do {
        instruction = instruction->next;
        if (!instruction) return 0;
    } while ((instruction->opcode & 15) != 0);
    return instruction;
}

/* Original [00068d20,00068d30). Labels retain their defining stream record. */
MachineInstruction *__fastcall label_machine_instruction(LabelSymbol *label)
{
    return label->instruction;
}

/* Original [00068c30,00068c50). Move backward to the preceding kind0
   operation, skipping every non-operation record. */
MachineInstruction *__fastcall previous_machine_operation(MachineInstruction *instruction)
{
    do {
        instruction = instruction->previous;
        if (!instruction) return 0;
    } while ((instruction->opcode & 15) != 0);
    return instruction;
}
/* Original [00033330,00033524). Begin local branches in short form and
   repeatedly widen those outside signed-byte reach, propagating changed label
   offsets and alignment until no length or label position changes. */
void __fastcall finish_machine_lowering(MachineContext *context)
{
    MachineInstruction *instruction, *label, *target;
    SymbolHeader *symbol;
    unsigned operation, kind, old_length, new_length;
    int offset = -1, growth, changed, excluded, destination, distance;
    unsigned old_offset;
    for (instruction = context; instruction; instruction = instruction->next) {
        operation = (instruction->opcode >> 8) & 255;
        if ((instruction->opcode & 15) == 0 && (operation == MC_JCC || operation == MC_JMP) &&
            (instruction->operands[0].flags & 15) == 8 && instruction->operands[0].value.symbol->kind == 3)
            instruction->opcode = (instruction->opcode & 0xffffff1fUL) | 0x10;
    }
    layout_machine_stream(context, 0);
    label = context->labels.next;
    do {
        growth = 0;
        changed = 0;
        for (instruction = label; instruction; instruction = instruction->labels.next)
            instruction->opcode &= ~0x800000UL;
        excluded = 0;
        for (instruction = context; instruction; instruction = instruction->next) {
            kind = instruction->opcode & 15;
            switch (kind) {
            case 0:
                operation = (instruction->opcode >> 8) & 255;
                if ((operation == MC_JCC || operation == MC_JMP) &&
                    (instruction->operands[0].flags & 15) == 8 && (instruction->opcode & 0xf0) == 0x10) {
                    symbol = instruction->operands[0].value.symbol;
                    destination = (int)symbol->field04;
                    target = label_machine_instruction((LabelSymbol *)symbol);
                    if (!(target->opcode & 0x800000UL)) destination += growth;
                    old_length = instruction->record.encoding_length & 0x3ff;
                    distance = destination - old_length - offset;
                    if (distance < 0) distance = -distance;
                    if (distance > 127) {
                        changed = 1;
                        instruction->opcode = (instruction->opcode & 0xffffff4fUL) | 0x40;
                        new_length = machine_instruction_size(instruction) & 0x3ff;
                        instruction->record.encoding_length = (instruction->record.encoding_length & ~0x3ffUL) | new_length;
                        growth += new_length - old_length;
                    }
                }
                offset += instruction->record.encoding_length & 0x3ff;
                break;
            case 1:
                old_offset = instruction->record.symbol->field04;
                if (!excluded) offset = align_machine_location(offset, instruction, 0);
                instruction->record.symbol->field04 = (unsigned)offset;
                if (old_offset != instruction->record.symbol->field04) changed = 1;
                instruction->opcode |= 0x800000UL;
                break;
            case 2: offset = (int)current_symbol->symbol.field04; break;
            case 7: excluded = ((MachineBlockHeader *)instruction)->flags & 2; break;
            }
        }
    } while (changed);
}

/* Original [00032ec0,00032f2f). A marked label benefits from alignment
   when a later direct branch returns to it before another marked label. */
int __fastcall alignment_lookahead(MachineInstruction *instruction)
{
    MachineInstruction *scan;
    unsigned kind, operation;
    if (!(instruction->labels.flags & 1)) return 0;
    for (scan = instruction->next; scan; scan = scan->next) {
        kind = scan->opcode & 15;
        if (kind == 1 && (scan->labels.flags & 1)) return 0;
        if (kind != 0) continue;
        operation = (unsigned char)(scan->opcode >> 8);
        if ((operation == MC_JCC || operation == MC_JMP) &&
            (scan->operands[0].flags & 15) == OPERAND_SYMBOL &&
            scan->operands[0].value.symbol == instruction->record.symbol) return 1;
    }
    return 0;
}

/* Original [00032e60,00032eb5). Absorb alignment in a preceding
   instruction when possible; i386 may also use a redundant one-byte prefix. */
int __fastcall extend_previous_instruction(MachineInstruction *instruction, unsigned bytes)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_JMP || operation == MC_RET) return 0;
    if (bytes == 1 && cpu_model == 3) {
        if (widen_instruction_encoding(instruction, bytes) || insert_alignment_increment(instruction)) return 1;
        return 0;
    }
    return widen_instruction_encoding(instruction, bytes);
}

/* Original [00068d00,00068d15). Add a newly split label to the current
   function's side chain and publish its machine-stream position. */
void __fastcall register_machine_label(MachineInstruction *instruction)
{
    instruction->labels.next = machine_context->labels.next;
    machine_context->labels.next = instruction;
    ((LabelSymbol *)instruction->record.symbol)->instruction = instruction;
}

/* Original [00032ff0,0003316e). Absorb alignment by enlarging an
   existing memory displacement. Search backwards within the same block;
   no-base indexed addresses already require a full displacement. */
int __fastcall widen_instruction_encoding(MachineInstruction *instruction, unsigned bytes)
{
    MachineOperand *operand;
    unsigned kind, displacement_bytes, encoding;
    int index, length;
    enum { NO_BASE_REGISTER = 0x1b };
    while ((instruction->opcode & 15) != 7 && (instruction->opcode & 15) != 1) {
        if (!(instruction->opcode & 15)) {
            for (index = 0; index < machine_operand_counts[(unsigned char)(instruction->opcode >> 8)]; ++index) {
                operand = &instruction->operands[index == 0 ? 0 : 1];
                kind = operand->flags & 15;
                if (kind != OPERAND_BASED && (kind != OPERAND_INDEXED ||
                    operand->value.address->base.words.number == NO_BASE_REGISTER)) continue;
                displacement_mode(instruction, operand, &displacement_bytes);
                length = (int)machine_instruction_size(instruction);
                encoding = instruction->record.encoding_length;
                if (bytes == 1 && length <= 4 && !displacement_bytes) {
                    encoding = (encoding & ~0xc000U) | 0x2000;
                    instruction->record.encoding_length =
                        (encoding & ~0x3ffU) | ((unsigned)(length + 1) & 0x3ff);
                    return 1;
                }
                if (bytes == 3 && displacement_bytes == 1) {
                    encoding = (encoding & ~0x8000U) | 0x6000;
                    instruction->record.encoding_length =
                        (encoding & ~0x3ffU) | ((unsigned)(length + 3) & 0x3ff);
                    return 1;
                }
                if (bytes == 4 && !displacement_bytes) {
                    encoding = (encoding & ~0x6000U) | 0x8000;
                    instruction->record.encoding_length =
                        (encoding & ~0x3ffU) | ((unsigned)(length + 4) & 0x3ff);
                    return 1;
                }
            }
        }
        instruction = instruction->previous;
    }
    return 0;
}

/* Original33170..332a4, fastcall ECX machine record/RET0. On i386,
   fill a single alignment byte with INC of a dead register. Scan backward
   for a candidate, and forward to the next flag access: a read forbids the
   insertion while an overwrite or block boundary permits it. Native opcode
   property bits2/4 at82210 agree exactly with the named flag properties.
   The register-eligibility child remains its own original graph vertex. */
int __fastcall insert_alignment_increment(MachineInstruction *instruction)
{
    MachineInstruction *previous, *scan, *inserted;
    MachineOperand operand;
    unsigned kind, operation;
    unsigned short number;
    for (;;) {
        kind = instruction->opcode & 15;
        if (kind == 1 || kind == 2) return 0;
        if (kind == 0) {
            operation = (instruction->opcode >> 8) & 255;
            if (operation == MC_JCC || operation == MC_JMP) return 0;
            if (machine_operand_counts[operation] > 0) {
                number = alignment_increment_register(instruction);
                if (number != 0xffff) {
                    previous = instruction->previous;
                    scan = previous->next;
                    while ((scan->opcode & 15) != 7) {
                        if (!(scan->opcode & 15) &&
                            (machine_selection_properties[(scan->opcode >> 8) & 255] &
                             (MC_WRITES_FLAGS | MC_READS_FLAGS))) break;
                        scan = scan->next;
                    }
                    if ((scan->opcode & 15) ||
                        !(machine_selection_properties[(scan->opcode >> 8) & 255] & MC_READS_FLAGS)) {
                        initialize_operand(&operand, OPERAND_REGISTER, number, 0, 0);
                        inserted = new_machine_record(0, MC_INC, 4, &operand);
                        insert_machine_after(previous, inserted);
                        inserted->record.encoding_length =
                            (inserted->record.encoding_length & ~0x3feU) | 1 | 0xe000;
                        return 1;
                    }
                }
            }
        }
        instruction = instruction->previous;
    }
}

/* Original332b0..33327, fastcall ECX candidate/AX result, RET0.
   A full-width first-register overwrite may tolerate INC immediately before
   it only if its other operand does not read the old value. ESP/EBP and the
   byte/word alias range8..31 are excluded; preserve the original >31 allowance. */
unsigned short __fastcall alignment_increment_register(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    int count = machine_operand_counts[operation];
    unsigned short number;
    if (count < 1 || (instruction->operands[0].flags & 15) != OPERAND_REGISTER)
        return 0xffff;
    number = instruction->operands[0].value.reg.words.number;
    if ((number > 7 && number <= 31) || number == 5 || number == 4 ||
        (instruction->opcode & 0xf0) != 0x40) return 0xffff;
    if (count > 1 && operand_mentions_register(&instruction->operands[1], number))
        return 0xffff;
    if (!overwrites_first_register(instruction, 0)) return 0xffff;
    return number;
}

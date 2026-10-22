/* Machine-code offsets and source/debug positions. */
#include "backend-passes.h"
#include "machine-layout.h"
#include "records.h"
#include "function-input.h"
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

/* Final machine stream, object bytes and source-line callbacks. */
#include "backend-passes.h"
#include "machine.h"
#include "instruction-encoding.h"
#include "output.h"
#include "records.h"
#include "function-input.h"
#include "control-input.h"
#include "options.h"
#include <recon.h>
extern int string_blocks;
/* Original [00068390,000685d0), including twelve-way record dispatch. */
void __fastcall emit_machine_instructions(MachineInstruction *instruction)
{
    Segment *segment = (Segment *)lookup_global_symbol(current_symbol->symbol.field18.identifier);
    MachineBlockHeader *block = NULL;
    unsigned last_line = 0, length, padding, i, operation;
    int offset = -1, displacement;
    unsigned char bytes[20];
    current_source_location = 0;
    for (; instruction; instruction = instruction->next) {
        switch (instruction->opcode & 15) {
        case 0:
            output_callbacks.select_segment(segment);
            length = encode_machine_instruction(instruction, bytes, offset, 1);
            output_callbacks.data(bytes, length);
            if (current_source_location && current_source_location != last_line) {
                output_callbacks.debug_record(current_source_location, offset);
                last_line = current_source_location;
            }
            offset += length;
            segment->symbol.field04 += length;
            operation = (instruction->opcode & 0xff00) >> 8;
            if ((operation == MC_JCC || operation == MC_JMP || operation == MC_JCXZ ||
                 operation == MC_LOOP || operation == MC_LOOPNZ || operation == MC_LOOPZ) &&
                (instruction->operands[0].flags & 15) == 8 && (instruction->opcode & 0xf0) == 0x10) {
                displacement = instruction->operands[0].value.symbol->field04 - offset;
                if (displacement != (signed char)displacement && (block->flags & 2))
                    compiler_error(0x1ab, format_function_name(instruction->operands[0].value.symbol));
            }
            break;
        case 1:
            offset = instruction->record.symbol->field04;
            break;
        case 2:
            offset = current_symbol->symbol.field04;
            output_callbacks.select_segment(segment);
            if (string_blocks && segment->symbol.field04 != (unsigned)offset) {
                padding = offset - segment->symbol.field04;
                for (i = 0; i < padding; ++i) bytes[i] = 0;
                output_callbacks.data(bytes, padding);
                segment->symbol.field04 += padding;
            }
            output_callbacks.symbol(current_symbol);
            break;
        case 3: case 8: case 9: case 10: case 11:
            break;
        case 4:
            current_source_location = instruction->record.source_location;
            break;
        case 5:
            if (instruction->record.filename != listing_source_filename) {
                current_source_filename = listing_source_filename = instruction->record.filename;
                output_callbacks.debug_pair(current_source_filename);
            }
            break;
        case 7:
            block = (MachineBlockHeader *)instruction;
            break;
        default:
            compiler_internal_error("@(#)pgo.c:1.110", 0x38a);
            break;
        }
    }
    output_callbacks.function(current_symbol);
}

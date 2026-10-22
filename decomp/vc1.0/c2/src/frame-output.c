/* Function frame/FPO metadata adapters. */
#include "backend-passes.h"
#include "output.h"
#include "machine.h"
#include "coff-output.h"
#include "options.h"
#include <recon.h>
/* Original [00069440,0006948b). The early frontier name predates recovery
   of its frame-record arguments: this wrapper supplies FPO-style frame sizes,
   prologue length and saved-register count, rather than a scope-table walk. */
void __fastcall emit_function_exception_table(FunctionSymbol *symbol, unsigned flags)
{
    unsigned saved;
    if ((short)symbol->prologue_bytes >= 0x100)
        compiler_internal_error("@(#)emit.c:1.20", 0x1fb);
    saved = count_function_saved_registers();
    write_function_frame_record(symbol, 0, symbol->code_bytes, symbol->field30 >> 2,
        symbol->parameter_bytes >> 2, symbol->prologue_bytes, saved, 0, flags);
}

/* Original [00027b90,00027bbf). Count explicit PUSHs before the first
   kind7 block: the prologue saves used by the frame record. This zero-argument
   cdecl call leaves the outer frame-writer arguments already on the stack. */
unsigned count_function_saved_registers(void)
{
    MachineInstruction *instruction = machine_context;
    unsigned count = 0;
    while ((instruction->opcode & 15) != 7) {
        if ((instruction->opcode & 15) == 0 && ((instruction->opcode >> 8) & 255) == MC_PUSH) ++count;
        instruction = instruction->next;
    }
    return count;
}

/* The serialized frame record is16 bytes, with the final byte holding saved
   registers and frame/base-pointer flags. No compiler-specific bitfield layout. */
typedef struct FunctionFrameRecord {
    unsigned offset, bytes, locals;
    unsigned short parameters;
    unsigned char prologue, attributes;
} FunctionFrameRecord;
typedef char FunctionFrameRecordSizeCheck[sizeof(FunctionFrameRecord) == 16 ? 1 : -1];
extern int coff_object;
/* Original [00069490,0006951f). Object output in COFF mode writes a frame
   record and binds its start-offset field to the function symbol. */
void __fastcall write_function_frame_record(FunctionSymbol *symbol, unsigned offset,
    unsigned bytes, unsigned locals, unsigned short parameters, unsigned char prologue,
    unsigned char saved, unsigned char frame, unsigned char base_pointer)
{
    FunctionFrameRecord record;
    if (object_filename && coff_object) {
        output_callbacks.select_segment(coff_function_fpo_segment);
        record.offset = offset;
        record.bytes = bytes;
        record.locals = locals;
        record.parameters = parameters;
        record.prologue = prologue;
        record.attributes = (((base_pointer * 2) | frame) << 3) | saved;
        output_callbacks.data(&record, sizeof(record));
        output_callbacks.address(0U - sizeof(record), &symbol->symbol);
        coff_function_fpo_segment->symbol.field04 += sizeof(record);
    }
}

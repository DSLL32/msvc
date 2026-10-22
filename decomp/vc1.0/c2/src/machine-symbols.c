/* Symbol discovery in the machine-instruction stream. */
#include "machine.h"
#include "records.h"
#include <recon.h>
extern int codeview_debug;
const signed char machine_operand_counts[MC_OPCODE_COUNT] = {
#define OP(name,count,properties) count,
#include "machine-opcodes.def"
#undef OP
};
/* Original [00068870,000689b0). */
BackendSymbolList *__fastcall collect_machine_symbols(MachineInstruction *instruction)
{
    MachineOperand *operand;
    SymbolHeader *symbol;
    BackendSymbol *storage;
    LabelSymbol *label;
    BackendSymbolList *symbols, *entry;
    DebugScope *scope;
    DebugLink *link;
    int count;
    unsigned kind;
    for (; instruction; instruction = instruction->next) {
        switch (instruction->opcode & 15) {
        case 0:
            count = machine_operand_counts[(instruction->opcode >> 8) & 255];
            while (count > 0) {
                operand = &instruction->operands[count == 2 ? 1 : 0];
                kind = operand->flags & 15;
                if (kind >= 3 && kind <= 6) symbol = operand->value.address->symbol;
                else if (kind == 8) {
                    symbol = operand->value.symbol;
                    if (symbol->kind == 3) symbol = 0;
                } else symbol = 0;
                if (symbol && symbol->kind != 19) {
                    queue_symbol_debug(symbol);
                    storage = (BackendSymbol *)symbol;
                    if (symbol->kind == 4 && (storage->attributes & 0x4000) &&
                        !(storage->attributes & 0x1000))
                        ((FunctionSymbol *)symbol)->flags52 |= 0x20;
                }
                --count;
            }
            break;
        case 1:
            symbol = instruction->record.symbol;
            label = (LabelSymbol *)symbol;
            if (codeview_debug && (!symbol->field0d || (symbol->field0e & 0x1000))) {
                if (symbol->field0e & 0x1000) {
                    scope = current_function->debug_scope;
                    link = arena_alloc(1, sizeof(DebugLink));
                    link->symbol = symbol;
                    link->next = scope->records;
                    scope->records = link;
                }
                queue_symbol_debug(symbol);
            }
            if (label->label_kind == 4) queue_symbol_debug(symbol);
            break;
        case 2:
            queue_symbol_debug(instruction->record.symbol);
            break;
        }
    }
    symbols = collect_global_symbols();
    for (entry = symbols; entry; entry = entry->next)
        reserve_symbol_storage(entry->symbol);
    return symbols;
}

#include "work-prepare.h"
#include "function-input.h"
#include "tree-rewrite.h"
#include "expression-fields.h"
#include "records.h"
#include "frame.h"
#include "instruction-selection.h"
#include <recon.h>
extern int optimize_global;
BitSet *spilled_parameter_registers; /* 0007b990 */
/* Original [00067170,00067350). Find the procedure entry, assign incoming
   parameter locations and insert stores for register parameters needing homes. */
void __fastcall prepare_parameter_storage(WorkItem *work)
{
    WorkItem *entry_work;
    Expression *expression, *spills;
    Node *entry, *parameters, *parameter, *register_leaf, *symbol_leaf, *store;
    TemporarySymbol *symbol;
    unsigned used_registers = 0, bytes = 0, convention, type;
    int reg;
    for (entry_work = work; entry_work; entry_work = entry_work->next) {
        for (expression = entry_work->body.expressions; expression; expression = expression->next) {
            entry = expression->tree;
            if (entry->opcode == 0x46) goto found_entry;
        }
    }
    return;
found_entry:
    parameter_stack_offset = function_stack_alignment();
    convention = current_symbol->symbol.field0e & 0x1c;
    if (convention == 4 || convention == 20) {
        parameter = entry->second.node;
        while (parameter->opcode == 0x2d) {
            assign_parameter_register(&used_registers, parameter, convention);
            parameter = parameter->second.node;
        }
    }
    parameters = entry->second.node;
    for (parameter = parameters; parameter->opcode == 0x2d; parameter = parameter->second.node) {
        symbol = (TemporarySymbol *)parameter->right.symbol;
        if (!(parameter->attributes.words.flags & 0x20)) bytes += reserve_symbol_storage(&symbol->symbol);
        else {
            if (optimize_global) {
                symbol->field28 = parameter->info.words.identifier;
                symbol->attributes |= 0x8000;
            }
            if ((symbol->symbol.field0e & 0x100) || !optimize_global) {
                symbol->symbol.field0e |= 0x100;
                symbol->attributes = (symbol->attributes & ~0x60U) | 0x10;
                reserve_symbol_storage(&symbol->symbol);
            }
        }
    }
    current_symbol->parameter_bytes = bytes;
    spilled_parameter_registers = 0;
    if (convention == 4 || convention == 20) {
        spills = entry_work->next->body.expressions;
        for (parameter = parameters; parameter->opcode != 0x4c; parameter = parameter->second.node) {
            symbol = (TemporarySymbol *)parameter->right.symbol;
            if ((parameter->attributes.words.flags & 0x20) && (symbol->symbol.field0e & 0x100)) {
                reg = parameter->info.words.identifier;
                spilled_parameter_registers = bitset_include(spilled_parameter_registers, reg);
                type = encode_node_type((signed char)(symbol->attributes & 15), symbol->symbol.value);
                register_leaf = intern_leaf(0xa2, 0, 0x404, reg);
                symbol_leaf = intern_leaf(0x26, 0, &symbol->symbol, 0);
                store = build_tree("PPNtNt", symbol_leaf, register_leaf, 0x30, type, 0x32, type);
                spills = insert_tree_expression(spills, spills ? spills->tree : 0, store);
            }
        }
        entry_work->next->body.expressions = spills;
    }
}

/* Original27a20..27b22: ECX used-register mask, EDX argument/parameter,
   stack calling convention, RET4. Original80ae0..80ae8 names ECX/EDX for
   fastcall;80ae8..80aec names the single ECX thiscall parameter. */
int __fastcall assign_parameter_register(unsigned *used, Node *argument, int convention)
{
    static const unsigned fastcall_registers[] = { REG_ECX, REG_EDX };
    static const unsigned thiscall_registers[] = { REG_ECX };
    const unsigned *registers;
    TemporarySymbol *symbol;
    unsigned type, bit, reg;
    int count, index;
    if (argument->opcode == 0x55) type = argument->attributes.words.type;
    else {
        symbol = (TemporarySymbol *)argument->right.symbol;
        type = encode_node_type((signed char)(symbol->attributes & 15), symbol->symbol.value);
    }
    switch (type) {
    case 0x101: case 0x102: case 0x104:
    case 0x201: case 0x202: case 0x204: case 0x404:
        break;
    default:
        return 0;
    }
    if (convention == 4) {
        registers = fastcall_registers;
        count = 2;
    } else {
        registers = thiscall_registers;
        count = 1;
    }
    for (index = 0; index < count; ++index) {
        reg = registers[index];
        bit = 1U << reg;
        if (!(*used & bit)) {
            *used |= bit;
            argument->attributes.words.flags |= 0x20;
            argument->info.words.identifier = (short)reg;
            return 1;
        }
    }
    return 0;
}

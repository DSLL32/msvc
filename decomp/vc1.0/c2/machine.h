#ifndef MACHINE_H
#define MACHINE_H
#include "backend-flow.h"
typedef enum MachineOpcode {
#define OP(name,count) MC_##name,
#include "machine-opcodes.def"
#undef OP
    MC_OPCODE_COUNT
} MachineOpcode;
extern const signed char machine_operand_counts[MC_OPCODE_COUNT];
typedef struct BackendSymbol {
    SymbolHeader symbol;
    unsigned short field20, output_flags;
    unsigned attributes;
} BackendSymbol;
#pragma pack(2)
typedef struct LabelSymbol {
    BackendSymbol storage;
    unsigned field28;
    long line;
    unsigned char label_kind, lexical_level;
} LabelSymbol;
#pragma pack()
typedef struct MachineAddress {
    unsigned flags;
    SymbolHeader *symbol;
} MachineAddress;
typedef struct MachineOperand {
    unsigned flags;
    union {
        unsigned constant;
        SymbolHeader *symbol;
        MachineAddress *address;
    } value;
} MachineOperand;
struct MachineInstruction {
    MachineInstruction *next, *previous;
    unsigned opcode;
    SymbolHeader *symbol;
    MachineOperand operands[2];
};
struct BackendSymbolList {
    BackendSymbolList *next;
    SymbolHeader *symbol;
};
BackendSymbolList *collect_global_symbols(void);
typedef struct BitSet BitSet;
extern MachineInstruction *machine_tail, *free_machine_instructions;
extern MachineInstruction *deferred_instructions, *deferred_tail;
extern void *machine_unused_pool;
extern MachineAddress *free_machine_addresses;
extern BitSet *machine_register_set;
extern unsigned short next_machine_register, next_tree_register;
#endif

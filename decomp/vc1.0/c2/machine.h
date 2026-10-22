#ifndef MACHINE_H
#define MACHINE_H
#include "backend-flow.h"
typedef enum MachineOpcode {
#define OP(name,count,properties) MC_##name,
#include "machine-opcodes.def"
#undef OP
    MC_OPCODE_COUNT
} MachineOpcode;
extern const signed char machine_operand_counts[MC_OPCODE_COUNT];
enum MachineSelectionProperty { MC_IMPLICIT_REGISTERS = 1 };
extern const unsigned char machine_selection_properties[MC_OPCODE_COUNT];
typedef struct BackendSymbol {
    SymbolHeader symbol;
    unsigned short field20, output_flags;
    unsigned attributes;
} BackendSymbol;
#pragma pack(2)
typedef struct LabelSymbol {
    SymbolHeader symbol;
    unsigned short field20, output_flags;
    MachineInstruction *instruction;
    unsigned field28;
    long line;
    unsigned char label_kind, lexical_level;
} LabelSymbol;
#pragma pack()
typedef char LabelSymbolSizeCheck[sizeof(LabelSymbol) == 50 ? 1 : -1];
typedef struct MachineAddress {
    union { long displacement; struct MachineAddress *next; } first;
    SymbolHeader *symbol;
    unsigned short base_register, field0a, index_register, field0e;
} MachineAddress;
typedef char MachineAddressSizeCheck[sizeof(MachineAddress) == 16 ? 1 : -1];
typedef struct MachineOperand {
    unsigned flags;
    union {
        unsigned constant;
        SymbolHeader *symbol;
        MachineAddress *address;
        struct MachineCallInfo *call;
    } value;
} MachineOperand;
typedef char MachineOperandSizeCheck[sizeof(MachineOperand) == 8 ? 1 : -1];
typedef struct MachineCallInfo {
    unsigned field00, field04, field08;
} MachineCallInfo;
typedef char MachineCallInfoSizeCheck[sizeof(MachineCallInfo) == 12 ? 1 : -1];
struct MachineInstruction {
    MachineInstruction *next, *previous;
    unsigned opcode;
    union { SymbolHeader *symbol; char *filename; unsigned source_location;
        MachineInstruction *next_use; } record;
    MachineOperand operands[2];
};
typedef char MachineInstructionSizeCheck[sizeof(MachineInstruction) == 32 ? 1 : -1];
/* Kind 7 shares the machine-record header and carries block register sets. */
typedef struct MachineBlockHeader {
    MachineInstruction *next, *previous;
    unsigned opcode;
    struct BitSet *uses, *definitions;
    int number;
    unsigned field18;
    unsigned short flags, field1e;
} MachineBlockHeader;
typedef char MachineBlockHeaderSizeCheck[sizeof(MachineBlockHeader) == 32 ? 1 : -1];
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

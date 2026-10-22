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
enum MachineSelectionProperty { MC_IMPLICIT_REGISTERS = 1, MC_WRITES_FLAGS = 2, MC_READS_FLAGS = 4,
    MC_STRING_OPERATION = 8, MC_REPEAT_PREFIX = 16, MC_WAIT_PREFIX = 32,
    MC_NO_OPERAND_PREFIX = 64, MC_ESCAPE_OPCODE = 128 };
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
    long references;
    unsigned char label_kind, lexical_level;
} LabelSymbol;
#pragma pack()
typedef char LabelSymbolSizeCheck[sizeof(LabelSymbol) == 50 ? 1 : -1];
typedef union MachineRegisterValue {
    unsigned value;
    struct { unsigned short number, flags; } words;
} MachineRegisterValue;
typedef struct MachineAddress {
    union { long displacement; struct MachineAddress *next; } first;
    SymbolHeader *symbol;
    MachineRegisterValue base, index;
} MachineAddress;
typedef char MachineAddressSizeCheck[sizeof(MachineAddress) == 16 ? 1 : -1];
typedef struct MachineOperand {
    unsigned flags;
    union {
        unsigned constant;
        SymbolHeader *symbol;
        MachineAddress *address;
        struct MachineCallInfo *call;
        MachineRegisterValue reg;
    } value;
} MachineOperand;
typedef char MachineOperandSizeCheck[sizeof(MachineOperand) == 8 ? 1 : -1];
typedef struct MachineCallInfo {
    unsigned argument_bytes, register_mask, result_flags;
} MachineCallInfo;
typedef char MachineCallInfoSizeCheck[sizeof(MachineCallInfo) == 12 ? 1 : -1];
typedef struct MachineFlowRegion MachineFlowRegion;
struct MachineInstruction {
    MachineInstruction *next, *previous;
    unsigned opcode;
    union { SymbolHeader *symbol; char *filename; unsigned source_location;
        MachineInstruction *next_use;
        unsigned encoding_length;
        struct { short code, value; } directive; } record;
    union {
        MachineOperand operands[2];
        /* Kind1 labels and the kind2 function header share a label chain.
           The later label-alignment pass consumes flags at offset0x18. */
        struct { struct MachineInstruction *next; MachineFlowRegion *attachment; unsigned flags, field1c; } labels;
    };
};
typedef char MachineInstructionSizeCheck[sizeof(MachineInstruction) == 32 ? 1 : -1];
/* Kind 7 shares the machine-record header and carries block register sets. */
typedef struct MachineBlockHeader {
    MachineInstruction *next, *previous;
    unsigned opcode;
    struct BitSet *uses, *definitions;
    int number;
    union { unsigned field18; int rematerialization_cost; };
    unsigned short flags, field1e;
} MachineBlockHeader;
typedef char MachineBlockHeaderSizeCheck[sizeof(MachineBlockHeader) == 32 ? 1 : -1];
struct BackendSymbolList {
    BackendSymbolList *next;
    SymbolHeader *symbol;
};
BackendSymbolList *collect_global_symbols(void);
typedef struct BitSet BitSet;
int __fastcall machine_records_equal(MachineInstruction *left, MachineInstruction *right);
extern MachineInstruction *machine_tail, *free_machine_instructions;
extern MachineInstruction *deferred_instructions, *deferred_tail;
extern void *machine_unused_pool;
extern MachineAddress *free_machine_addresses;
extern BitSet *machine_register_set;
extern unsigned short next_machine_register, next_tree_register;
#endif

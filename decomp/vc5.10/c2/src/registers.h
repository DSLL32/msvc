#ifndef C2_REGISTERS_H
#define C2_REGISTERS_H
#include "c2.h"
#include <recon.h>

#define C2_MACHINE_REGISTER_COUNT 75
/* Flags have their original zero initializers. Row zero's number and row
 * one's number/canonical/bit-range and rows two/three's numbers are modeled,
 * along with row66's canonical/number/range. Rows31/32 have only their number
 * column. All other reserved backing remains UNMODELED. */
typedef struct C2MachineRegister {
    unsigned char pending00[4];
    unsigned char kind;
    unsigned char flags, pending06[2];
    C2FunctionRegister *canonical;
    union {
        struct { union { unsigned char pending0c_before_number[16];
            struct { C2FunctionRegister *variant_next;
                union { unsigned char pending10[12];
                    struct { unsigned char pending10_before_definition[4];
                        C2Expression *definition; unsigned char pending18[4]; }; };
            }; }; unsigned number; };
        unsigned char pending0c[20];
    };
    unsigned bit_width;
    long bit_displacement;
    unsigned short parameter_number;
    unsigned char pending2a[10];
    int spill_cost; /* +52, defined by the actual spill-cost initializer. */
    unsigned char pending38[24];
} C2MachineRegister;
typedef char C2MachineRegisterSizeCheck[sizeof(C2MachineRegister) == 80 ? 1 : -1];
typedef char C2MachineRegisterKindCheck[offsetof(C2MachineRegister, kind) == 4 ? 1 : -1];
typedef char C2MachineRegisterFlagsCheck[offsetof(C2MachineRegister, flags) == 5 ? 1 : -1];
typedef char C2MachineRegisterCanonicalCheck[offsetof(C2MachineRegister, canonical) == 8 ? 1 : -1];
typedef char C2MachineRegisterParameterCheck[offsetof(C2MachineRegister, parameter_number) == 40 ? 1 : -1];
typedef char C2MachineRegisterDefinitionCheck[offsetof(C2MachineRegister, definition) == 20 ? 1 : -1];
typedef char C2MachineRegisterNumberCheck[offsetof(C2MachineRegister, number) == 28 ? 1 : -1];
typedef char C2MachineRegisterBitWidthCheck[offsetof(C2MachineRegister, bit_width) == 32 ? 1 : -1];
typedef char C2MachineRegisterBitDisplacementCheck[offsetof(C2MachineRegister, bit_displacement) == 36 ? 1 : -1];
/* The reached kind byte and later canonical-root comparison establish only
 * kind2 and the self pointer at+8. The other bytes are reserved backing,
 * not original payload/defaults or a claimed full sentinel object extent. */
typedef struct C2MachineRegisterSentinel {
    unsigned char pending00[4];
    unsigned char kind;
    unsigned char pending05[3];
    C2FunctionRegister *canonical;
} C2MachineRegisterSentinel;
typedef struct C2MachineRegisterStorage {
    C2MachineRegister rows[C2_MACHINE_REGISTER_COUNT];
    C2MachineRegisterSentinel sentinel;
} C2MachineRegisterStorage;
typedef char C2MachineSentinelPrefixSize[sizeof(C2MachineRegisterSentinel) == 12 ? 1 : -1];
typedef char C2MachineSentinelCanonicalOffset[offsetof(C2MachineRegisterSentinel, canonical) == 8 ? 1 : -1];
typedef char C2MachineSentinelOffset[offsetof(C2MachineRegisterStorage, sentinel) == 6000 ? 1 : -1];
typedef char C2MachineSentinelKindOffset[offsetof(C2MachineRegisterStorage, sentinel) +
    offsetof(C2MachineRegisterSentinel, kind) == 6004 ? 1 : -1];
extern C2MachineRegisterStorage machine_register_storage;
#define machine_registers (machine_register_storage.rows)
extern C2MachineRegister *final_division_registers[3];
void reset_register_marks(void);
void __fastcall mark_spill_register_used(unsigned number);
#endif

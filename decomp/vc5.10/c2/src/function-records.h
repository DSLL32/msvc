#ifndef C2_FUNCTION_RECORDS_H
#define C2_FUNCTION_RECORDS_H
#include "functions.h"
#include <recon.h>

typedef struct C2RegisterBlock C2RegisterBlock;
/* Only the encountered kind and number fields are named. The actual allocator's80B
 * records justify the extent; other fields/defaults remain unmodeled. */
struct C2FunctionRegister {
    C2Symbol *symbol;
    unsigned char kind, view_flags, flags, pending07;
    C2FunctionRegister *canonical;
    C2FunctionRegister *variant_next;
    unsigned short type, pending12;
    struct C2Expression *value;
    union { unsigned pending18; struct C2GlobalExpression *global_expression; };
    unsigned number;
    unsigned value_bytes;
    union { long displacement; unsigned optimizer_scope_number; };
    union { C2FunctionRegister *next_free, *copied_register; long frame_displacement;
        C2Expression *diagnostic_record; C2Expression *allocation_operand; };
    union {
        unsigned pending2c;
        C2FunctionRegister *previous_parameter;
        C2OptimizerUse *dependent_uses;
        C2FunctionRegister *global_next;
        C2FunctionRegister *diagnostic_next;
        C2FunctionRegister *allocation_next;
        C2FunctionRegister *spill_preferred_register;
        C2FunctionRegister *frame_next;
        C2FunctionRegister *cache_equivalent_next;
    };
    union { C2FunctionRegister *cache_group_next; unsigned properties; C2FunctionRegister *frame_previous; C2OptimizerRegisterInfo *optimizer; C2OptimizerUse *optimizer_uses;
        struct C2AllocationInfo *allocation;
        struct { unsigned char rewrite_seen, rewrite_queued, rewrite_flags, pending33; };
        /* Final width analysis reuses these bytes for forward facts and
         * backward use requirements after its real scratch reset. */
        struct { unsigned char known_zero_bits, unused_high_bits, extension_flags,
            pending_extension_flags; }; };
    union { C2Expression *cache_values; unsigned optimizer_index; C2RegisterSet *optimizer_set; struct C2GlobalDependency *global_dependents;
        C2FunctionRegister *allocation_root; int spill_cost, frame_uses; };
    union {
        unsigned char pending38[24];
        struct {
            union { unsigned char pending38_prefix[12];
                struct {
                    union { struct C2GlobalDependency *global_dependencies; int frame_index; };
                    struct C2GlobalRegisterLink *global_assignments;
                    C2RegisterSet *global_dependency_set;
                };
            };
            union { C2OptimizerUse *global_followers; unsigned global_invalidation_epoch; };
            union { struct C2OptimizerRecordLink *global_definitions; C2Expression *global_definition_record;
                C2Expression *allocation_definition_record; };
            union { struct C2OptimizerRecordLink *global_uses;
                C2Expression *global_last_input_operand; C2Expression *allocation_definition_operand; };
        };
    };
};
typedef char C2CacheEquivalentOffset[offsetof(C2FunctionRegister, cache_equivalent_next) == 44 ? 1 : -1];
typedef char C2CacheGroupOffset[offsetof(C2FunctionRegister, cache_group_next) == 48 ? 1 : -1];
typedef char C2CacheValuesOffset[offsetof(C2FunctionRegister, cache_values) == 52 ? 1 : -1];
typedef char C2FunctionRegisterValueCheck[offsetof(C2FunctionRegister, value) == 20 ? 1 : -1];
typedef char C2FunctionRegisterCanonicalCheck[offsetof(C2FunctionRegister, canonical) == 8 ? 1 : -1];
typedef char C2FunctionRegisterPropertiesCheck[offsetof(C2FunctionRegister, properties) == 48 ? 1 : -1];
typedef char C2GlobalLastOperandOffset[offsetof(C2FunctionRegister, global_last_input_operand) == 76 ? 1 : -1];
typedef char C2FunctionRegisterSizeCheck[sizeof(C2FunctionRegister) == 80 ? 1 : -1];
typedef char C2FunctionRegisterKindCheck[offsetof(C2FunctionRegister,kind) == 4 ? 1 : -1];
typedef char C2FunctionRegisterNumberCheck[offsetof(C2FunctionRegister,number) == 28 ? 1 : -1];
typedef char C2FunctionRegisterFreeCheck[offsetof(C2FunctionRegister,next_free) == 40 ? 1 : -1];
struct C2RegisterBlock {
    C2RegisterBlock *next;
    C2FunctionRegister *cursor, *end;
    C2FunctionRegister records[32];
};
typedef char C2RegisterBlockSizeCheck[sizeof(C2RegisterBlock) == 2572 ? 1 : -1];
typedef char C2RegisterBlockRowsCheck[offsetof(C2RegisterBlock,records) == 12 ? 1 : -1];

extern C2RegisterBlock **function_register_blocks;
extern C2RegisterBlock **next_function_register_block, **end_function_register_blocks;
extern C2RegisterBlock *first_function_register_block, *last_function_register_block;
extern unsigned next_function_register_number;
extern C2RegisterBlock *tracked_register_block, *secondary_register_block;
extern C2RegisterBlock *plain_register_block, *default_register_block, *extra_register_block;
extern C2FunctionRegister *free_plain_register_nodes, *free_tracked_register_nodes;
extern C2RegisterSet *secondary_register_set, *tracked_register_set;
extern C2FunctionRegister *empty_function_register;
C2RegisterBlock *new_register_block(void);
C2RegisterSet *__fastcall new_register_set(int pool);
C2FunctionRegister *__fastcall new_function_register(unsigned char kind);
C2FunctionRegister *__fastcall new_address_register(C2FunctionRegister *source,
                                      unsigned short type, unsigned bytes, long displacement);
int __fastcall register_types_match(unsigned short first, unsigned short second);
C2FunctionRegister *__fastcall new_bitfield_register(C2FunctionRegister *source,
                                                    unsigned bits, long displacement);
void __fastcall replace_register_uses(C2FunctionRegister *source, C2FunctionRegister *replacement);
void __fastcall clear_register_set(C2RegisterSet *set);
void __fastcall add_register_to_set(C2RegisterSet *set, unsigned number);
int __fastcall compare_function_registers(C2FunctionRegister *left, C2FunctionRegister *right);
#endif

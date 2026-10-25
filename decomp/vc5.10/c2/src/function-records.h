#ifndef C2_FUNCTION_RECORDS_H
#define C2_FUNCTION_RECORDS_H
#include "functions.h"
#include <recon.h>

typedef struct C2RegisterBlock C2RegisterBlock;
typedef struct C2RegisterSet C2RegisterSet;
/* Only the encountered kind and number fields are named. The actual allocator's80B
 * records justify the extent; other fields/defaults remain unmodeled. */
struct C2FunctionRegister {
    C2Symbol *symbol;
    unsigned char kind, pending05, flags, pending07;
    C2FunctionRegister *canonical;
    unsigned pending0c;
    unsigned short type, pending12;
    struct C2Expression *value;
    unsigned pending18;
    unsigned number;
    unsigned value_bytes, pending24;
    C2FunctionRegister *next_free;
    unsigned pending2c, properties;
    unsigned char pending34[28];
};
typedef char C2FunctionRegisterValueCheck[offsetof(C2FunctionRegister, value) == 20 ? 1 : -1];
typedef char C2FunctionRegisterCanonicalCheck[offsetof(C2FunctionRegister, canonical) == 8 ? 1 : -1];
typedef char C2FunctionRegisterPropertiesCheck[offsetof(C2FunctionRegister, properties) == 48 ? 1 : -1];
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
void __fastcall clear_register_set(C2RegisterSet *set);
void __fastcall add_register_to_set(C2RegisterSet *set, unsigned number);
#endif

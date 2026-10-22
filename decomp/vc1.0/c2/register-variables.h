#ifndef REGISTER_VARIABLES_H
#define REGISTER_VARIABLES_H
#include "symbols.h"
/* Native 52-byte variable. Unconsumed allocation fields retain offset
   names until their owning pass establishes their meanings. */
typedef struct RegisterVariable {
    struct RegisterVariable *next;
    unsigned field04, field08, field0c, field10, field14;
    unsigned short number, field1a;
    unsigned flags, field20;
    short first_block, last_block;
    signed char storage_bytes;
    unsigned char field29;
    unsigned short field2a;
    SymbolHeader *home_symbol;
    unsigned home_offset;
} RegisterVariable;
typedef char RegisterVariableSizeCheck[sizeof(RegisterVariable) == 52 ? 1 : -1];
enum { REGISTER_VARIABLE_CLASSES = 3, REGISTER_VARIABLE_BUCKETS = 32 };
extern RegisterVariable *register_variable_buckets[REGISTER_VARIABLE_CLASSES][REGISTER_VARIABLE_BUCKETS];
void reset_global_register_map(void);
void __fastcall record_register_variable_range(unsigned short number, short first_block, short last_block,
    int observable, int parameter, unsigned type, SymbolHeader *symbol, unsigned offset);
RegisterVariable *__fastcall create_register_variable(unsigned short number);
RegisterVariable *__fastcall new_register_variable(unsigned short number);
RegisterVariable *__fastcall lookup_register_variable(unsigned short number, int required);
#endif

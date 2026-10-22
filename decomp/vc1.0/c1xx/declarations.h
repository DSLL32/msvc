#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include "hash-table.h"
#include "types.h"

/* C++ declarations extend the common frontend symbol with two trailing words. */
struct CppSymbol {
    unsigned char kind, flags;
    unsigned short field02;
    CppSymbol *next;
    char *name;
    CType *type;
    unsigned index, attributes;
    unsigned short references;
    unsigned char storage[3], field1d;
    unsigned short field1e;
    union {
        struct { unsigned short operation, flags; } intrinsic;
        unsigned value;
        CppSymbol *parameter_next;
    } detail;
    unsigned field24, field28, field2c, field30;
};
typedef struct DeclarationList {
    CppSymbol *head;
} DeclarationList;
extern HashTable *declaration_table;
extern CppSymbol *translation_sentinel, *translation_placeholder;
extern char *anonymous_symbol_name;
extern Vector *translation_symbols;
void initialize_declaration_table(void);
typedef char CppSymbolSizeMustBe52[(sizeof(CppSymbol) == 52) ? 1 : -1];
#endif

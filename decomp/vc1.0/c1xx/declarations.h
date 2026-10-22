#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include "hash-table.h"
#include "types.h"

typedef struct ClassInfo {
    unsigned field00, field04, field08, field0c;
    unsigned field10, field14, field18, field1c, field20, field24;
    short type_index;
    unsigned short pointer_attributes;
    unsigned field2c;
    unsigned short debug_index;
    unsigned char alignment, flags;
} ClassInfo;

/* C++ declarations extend the common frontend symbol with two trailing words. */
struct CppSymbol {
    unsigned char kind, flags;
    unsigned short declaration_flags;
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
        char *text;
        ClassInfo *class_info;
        CppSymbol *parameter_next;
    } detail;
    unsigned field24, field28, field2c, cpp_flags;
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
typedef char ClassInfoSizeMustBe52[(sizeof(ClassInfo) == 52) ? 1 : -1];
#endif

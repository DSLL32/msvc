#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include "hash-table.h"
#include "types.h"

/* Declaration prefix encountered by parameter-list construction. */
struct CppSymbol {
    unsigned short flags, field02;
    CppSymbol *next;
    void *field08;
    CType *type;
    unsigned field10, field14, field18, field1c;
    CppSymbol *circular_next;
};
typedef struct DeclarationList {
    CppSymbol *head;
} DeclarationList;
extern HashTable *declaration_table;
void initialize_declaration_table(void);
#endif

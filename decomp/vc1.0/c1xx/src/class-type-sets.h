#ifndef CLASS_TYPE_SETS_H
#define CLASS_TYPE_SETS_H
#include "class-finalization.h"
#include "inline-vector.h"
#include <recon.h>
/* Sorted type sets use the existing permanent inline-vector representation. */
typedef InlineVector TypeSet;
typedef struct TypeSetIterator {
    TypeSet *set;
    short index;
    unsigned short reserved;
    CType *type;
} TypeSetIterator;
typedef struct ConversionTypeIterator {
    TypeSet *ambiguous_types;
    TypeSetIterator types;
    CType *type;
    int ambiguous;
} ConversionTypeIterator;
typedef char ConversionTypeIteratorSizeMustBe24[(sizeof(ConversionTypeIterator) == 24) ? 1 : -1];
typedef char ConversionTypeIteratorTypesOffsetMustBe4[(offsetof(ConversionTypeIterator, types) == 4) ? 1 : -1];
typedef char ConversionTypeIteratorTypeOffsetMustBe16[(offsetof(ConversionTypeIterator, type) == 16) ? 1 : -1];
typedef char ConversionTypeIteratorAmbiguousOffsetMustBe20[(offsetof(ConversionTypeIterator, ambiguous) == 20) ? 1 : -1];
void __stdcall begin_conversion_types(ConversionTypeIterator *iterator, CppSymbol *tag);
int __stdcall next_conversion_type(ConversionTypeIterator *iterator);
typedef struct ConversionIterator {
    CppSymbol *next;
    unsigned char kind, reserved[3];
    OverloadSet *current;
    CppSymbol *target;
    CType *type;
} ConversionIterator;
typedef char TypeSetItemsOffsetMustBe4[(offsetof(TypeSet, items) == 4) ? 1 : -1];
typedef char TypeSetIteratorSizeMustBe12[(sizeof(TypeSetIterator) == 12) ? 1 : -1];
typedef char ConversionIteratorSizeMustBe20[(sizeof(ConversionIterator) == 20) ? 1 : -1];
TypeSet *new_type_set(void);
TypeSet *union_type_sets(TypeSet *set, TypeSet *source);
void begin_type_set(TypeSetIterator *iterator, TypeSet *set);
int next_type_set(TypeSetIterator *iterator);
int type_set_contains(TypeSet *set, CType *type);
TypeSet *remove_type_set(TypeSet *set, CType *type);
void __stdcall begin_conversion_functions(ConversionIterator *iterator, CppSymbol *tag);
int __stdcall next_conversion_function(ConversionIterator *iterator);
CType *__stdcall canonical_conversion_type(CppSymbol *function);
TypeSet *__stdcall share_inherited_type_set(TypeSet *set, CppSymbol *tag);
#endif

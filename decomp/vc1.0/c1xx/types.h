#ifndef TYPES_H
#define TYPES_H
#include "initialization.h"

/* Interned C++ type descriptors are twelve bytes; their hash links live in a
   separate TypeEntry prefix. The debug index occupies the final short. */
struct CType {
    unsigned short flags, attributes;
    TypeShape *related;
    short field08;
    unsigned short debug_index;
};
typedef struct FunctionParameters FunctionParameters;
struct TypeShape {
    unsigned short flags, attributes;
    TypeShape *related;
    union {
        unsigned word;
        int array_bound;
        void *based_symbol;
        CppSymbol *tag_symbol;
        FunctionParameters *parameters;
    } data;
};
typedef struct TypeEntry {
    struct TypeEntry *next;
    CType type;
} TypeEntry;
typedef struct TypeShapeEntry {
    struct TypeShapeEntry *next;
    TypeShape shape;
} TypeShapeEntry;
typedef struct ParameterEntry ParameterEntry;
typedef struct DeclarationParameterEntry DeclarationParameterEntry;
extern TypeEntry **type_descriptor_buckets;
extern TypeShapeEntry **type_shape_buckets;
extern ParameterEntry **parameter_buckets;
extern DeclarationParameterEntry **declaration_parameter_buckets;
TypeDescription *__stdcall copy_type_description(CType *type);
TypeShape *__stdcall intern_type_shape(TypeShape *shape);
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes,
                                    void *parameter);
FunctionParameters *__stdcall build_parameter_info(void *declarations,
                                                   unsigned short *kind);
typedef char CTypeSizeMustBe12[(sizeof(CType) == 12) ? 1 : -1];
typedef char TypeShapeSizeMustBe12[(sizeof(TypeShape) == 12) ? 1 : -1];
#endif

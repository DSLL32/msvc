#ifndef TYPES_H
#define TYPES_H
#include <stddef.h>
#include "initialization.h"
#include <recon.h>

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
extern CppSymbol *pointer_base_symbol;
extern int type_description_depth;
extern TypeDescription type_description_stack[10];
CType *__stdcall copy_unfinished_type(CType *type);
CType *__stdcall finish_type(CType *type);
TypeDescription *__stdcall copy_type_description(CType *type);
TypeShape *__stdcall intern_type_shape(TypeShape *shape);
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes,
                                    void *parameter);
FunctionParameters *__stdcall build_parameter_info(void *declarations,
                                                   unsigned short *kind);
/* Declaration action 88 loads this signed class-context index from CType,
   whereas kind-15 class references use their distinct +0xc index field. */
typedef char CTypeContextIndexOffsetMustBe8[(offsetof(CType, field08) == 8) ? 1 : -1];
typedef char CTypeContextIndexSizeMustBe2[(sizeof(((CType *)0)->field08) == 2) ? 1 : -1];
typedef char CTypeSizeMustBe12[(sizeof(CType) == 12) ? 1 : -1];
typedef char TypeShapeSizeMustBe12[(sizeof(TypeShape) == 12) ? 1 : -1];
#endif

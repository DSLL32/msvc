#ifndef TYPES_H
#define TYPES_H
#include "initialization.h"

typedef struct TypeEntry {
    struct TypeEntry *next;
    CType type;
} TypeEntry;
struct TypeShape {
    unsigned short flags, attributes;
    TypeShape *related;
    unsigned field08;
};
typedef struct TypeShapeEntry {
    struct TypeShapeEntry *next;
    TypeShape shape;
} TypeShapeEntry;
typedef struct ParameterInfo {
    unsigned count, auxiliary;
    CType *types[1];
} ParameterInfo;
typedef struct ParameterEntry {
    struct ParameterEntry *next;
    ParameterInfo info;
} ParameterEntry;
typedef struct ParameterDeclaration {
    unsigned char kind, flags;
    unsigned short field02;
    struct ParameterDeclaration *next;
    unsigned field08;
    CType *type;
    unsigned field10, field14, field18, field1c;
    struct ParameterDeclaration *circular_next;
} ParameterDeclaration;
typedef struct ParameterList {
    ParameterDeclaration *head;
} ParameterList;
extern TypeEntry **type_descriptor_buckets;
extern TypeShapeEntry **type_shape_buckets;
extern ParameterEntry **parameter_buckets;
extern TypeShape *pointer_shapes[37];
extern CType pointer_type_scratch;
extern TypeShape pointer_shape_scratch;
extern int active_plmn_pragma, active_plmf_pragma;
ParameterInfo *__stdcall build_parameter_info(void *parameters, unsigned short *flags);
ParameterInfo *empty_parameter_info(void);
ParameterInfo *void_parameter_info(void);
unsigned __stdcall hash_parameters(ParameterInfo *parameters);
extern int declaration_context;
extern int type_description_depth;
TypeDescription *__stdcall copy_type_description(CType *type);
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes, void *parameter);
TypeShape *__stdcall intern_type_shape(TypeShape *shape);
#endif

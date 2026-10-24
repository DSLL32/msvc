#ifndef TYPES_H
#define TYPES_H
#include "initialization.h"
#include <recon.h>

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
extern ParameterInfo *empty_parameters, *void_parameters;
extern TypeShape *function_shapes[21];
extern CType function_type_scratch;
extern TypeShape function_shape_scratch;
extern int active_plmn_pragma, active_plmf_pragma;
ParameterInfo *__stdcall build_parameter_info(void *parameters, unsigned short *flags);
ParameterInfo *empty_parameter_info(void);
ParameterInfo *void_parameter_info(void);
unsigned __stdcall hash_parameters(ParameterInfo *parameters);
extern int declaration_context;
extern int type_description_depth;
char *__stdcall type_specifier_name(unsigned short flags);
extern TypeDescription type_description_stack[10];
void __stdcall require_complete_type(CType *type);
int __stdcall base_type_size(unsigned short flags, void *information);
int __stdcall array_element_size(unsigned short flags, void *information, TypeShape *shape);
int __stdcall pointer_type_size(unsigned short attributes);
CType *__stdcall drop_first_type_shape(CType *type);
CType *__stdcall remove_type_prefix(CType *type);
CType *__stdcall replace_outer_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter);
TypeDescription *__stdcall copy_mutable_type(CType *type);
CType *__stdcall finalize_type_description(TypeDescription *description);
unsigned short __stdcall merge_type_attributes(unsigned short existing, unsigned short added);
TypeDescription *__stdcall copy_type_description(CType *type);
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes, void *parameter);
TypeShape *__stdcall intern_type_shape(TypeShape *shape);
TypeShape *__stdcall prepend_interned_type_shape(TypeShape *related, unsigned short kind, unsigned short attributes, void *parameter);
int __stdcall parameter_type_size(CType *type, int promote_float);
extern struct FrontendSymbol *pointer_base_symbol;
CType *__stdcall qualify_member_type(CType *type, unsigned short model);
CType *__stdcall clear_type_attributes(CType *type, unsigned short attributes);
CType *__stdcall make_pointer_model_type(unsigned short model, CType *type);
int __stdcall merge_selected_flags(unsigned *target, unsigned added);
TypeShape *__stdcall last_type_shape(TypeShape *shape);
CType *__stdcall identity_type(CType *type);
TypeShape *__stdcall identity_type_shape(TypeShape *shape);
#endif

#ifndef WPP_TYPES_H
#define WPP_TYPES_H
#include <stdarg.h>
#include "carve.h"
#include "compiler.h"
#include <recon.h>
/* Actual 1994 ids: BOOL and the later INT64 pair are absent. */
enum {
    TYP_ERROR, TYP_CHAR, TYP_SCHAR, TYP_UCHAR, TYP_WCHAR,
    TYP_SSHORT, TYP_USHORT, TYP_SINT, TYP_UINT, TYP_SLONG, TYP_ULONG,
    TYP_FLOAT, TYP_DOUBLE, TYP_LONG_DOUBLE, TYP_ENUM, TYP_POINTER,
    TYP_TYPEDEF, TYP_CLASS, TYP_BITFIELD, TYP_FUNCTION, TYP_ARRAY,
    TYP_DOT_DOT_DOT, TYP_VOID, TYP_MODIFIER, TYP_MEMBER_POINTER,
    TYP_GENERIC, TYP_FREE, TYP_MAX
};
typedef unsigned char type_id;
typedef struct type *TYPE;
/* Reached pointer-difference roles in the compilation information object. */
typedef struct comp_info_types {
    TYPE ptr_diff_near,ptr_diff_far,ptr_diff_far16,ptr_diff_huge;
} COMP_INFO_TYPES;
extern COMP_INFO_TYPES CompInfoTypes;

struct scope;
/* The old declaration specifier predates the name and __declspec fields.
 * Its packed storage-class/scalar/qualifier tail begins at28. */
#pragma pack(1)
typedef struct decl_spec DECL_SPEC;
struct decl_spec {
    DECL_SPEC *prev;
    struct scope *scope;
    TYPE partial;
    void *linkage;
    struct symbol *typedef_defined;
    char *name;
    struct parse_tree_node *id;
    unsigned char stg_class;
    unsigned short scalar;
    unsigned char specifier;
    unsigned char type_defined:1,type_declared:1,ctor_name:1,is_default:1,
        diagnosed:1,decl_checked:1,type_elaborated:1,nameless_allowed:1;
    unsigned char generic:1,class_instantiation:1,no_more_linkage:1,
        arg_declspec:1,reserved:4;
};
#pragma pack()
DECL_SPEC *PTypeDefault(void);
DECL_SPEC *PTypeActualType(TYPE type);
DECL_SPEC *PTypeActualTypeName(TYPE type,struct parse_tree_node *id);
DECL_SPEC *PTypeDone(DECL_SPEC *spec,int declaration_done);
DECL_SPEC *CheckArgDSpec(DECL_SPEC *spec);
DECL_SPEC *CheckNoDeclarator(DECL_SPEC *spec);
void CheckDeclarationDSpec(DECL_SPEC *spec,struct scope *scope);
/* Reached53-byte declaration record. Unconsumed pointer roles stay unnamed
 * rather than importing the later expanded declaration layout. */
#pragma pack(1)
typedef struct decl_info DECL_INFO;
struct decl_info {
    DECL_INFO *next;
    struct parse_tree_node *id;
    struct scope *scope;
    struct scope *friend_scope;
    TYPE list;
    struct symbol *sym;
    struct symbol *generic_sym;
    char *name;
    TYPE type;
    DECL_INFO *parms;
    struct parse_tree_node *defarg_expr;
    struct rewrite_package *body,*mem_init;
    unsigned char flags;
};
#pragma pack()
typedef char DeclInfoSize[(sizeof(DECL_INFO)==53)?1:-1];
TYPE MakeFnType(DECL_INFO **args,unsigned char qualifiers,struct parse_tree_node *exceptions);
DECL_INFO *AddDeclarator(DECL_INFO *info,TYPE type);
DECL_INFO *AddExplicitParms(DECL_INFO *info,DECL_INFO *args);
void FreeDeclInfo(DECL_INFO *info);
DECL_INFO *MakeDeclarator(DECL_SPEC *spec,struct parse_tree_node *id);
DECL_INFO *DeclFunction(DECL_SPEC *spec,DECL_INFO *info);
DECL_INFO *FinishDeclarator(DECL_SPEC *spec,DECL_INFO *info);
DECL_INFO *DeclSpecDeclarator(DECL_SPEC *spec);
DECL_INFO *AddArgument(DECL_INFO *head,DECL_INFO *argument);
DECL_SPEC *PTypeCombine(DECL_SPEC *first,DECL_SPEC *second);
TYPE StructType(TYPE type);
/* Actual argument lists align the pointer at8 despite the 16-bit qualifier. */
#pragma pack(4)
typedef struct arg_list {
    unsigned num_args;
    unsigned short qualifier;
    TYPE *except_spec;
    TYPE type_list[1];
} arg_list;
#pragma pack()
#pragma pack(1)
/* Actual52-byte 1994 class metadata, before class_mod and the later
 * expanded debug handle. The two bytes at40 are not initialized by newINFO;
 * their role is not yet consumed. */
struct class_info {
    struct base_class *bases;
    struct friend_entry *friends;
    char *name;
    void *cdopt_cache;
    unsigned refno,size,vsize;
    long vb_offset,vf_offset;
    unsigned short last_vfn,last_vbase,unclassified_40,index;
    unsigned char max_align;
    unsigned char defined:1,opened:1,unnamed:1,corrupted:1,abstract:1,
        abstract_OK:1,anonymous:1,has_def_opeq:1;
    unsigned char has_ctor:1,has_dtor:1,has_pure:1,has_vfptr:1,
        has_vbptr:1,has_data:1,has_vfn:1,has_vcdtor:1;
    unsigned char ctor_defined:1,copy_defined:1,dtor_defined:1,assign_defined:1,
        ctor_gen:1,copy_gen:1,dtor_gen:1,assign_gen:1;
    unsigned char ctor_user_code:1,copy_user_code:1,dtor_user_code:1,
        assign_user_code:1,ctor_user_code_checked:1,copy_user_code_checked:1,
        dtor_user_code_checked:1,assign_user_code_checked:1;
    unsigned char needs_ctor:1,needs_dtor:1,needs_vdtor:1,needs_assign:1,
        const_copy:1,const_assign:1,const_ref:1,zero_array:1;
    unsigned char flags50,padding;
};
typedef char ClassInfoSize[(sizeof(struct class_info)==52)?1:-1];
struct type {
    TYPE next, of;
    union {
        struct { void *init1, *init2; } i;
        struct { struct scope *scope; struct class_info *info; } c;
        struct { struct symbol *sym; struct scope *scope; } t;
        struct { void *base, *pragma; } m;
        struct { arg_list *args; void *pragma; } f;
        struct { unsigned refno, array_size; } a;
        struct { unsigned char field_start, field_width; } b;
        struct { TYPE host; } mp;
        struct { unsigned index; } g;
    } u;
    unsigned dbg_handle;
    type_id id;
    unsigned short flag;
    unsigned char dbgflag;
};
#pragma pack()
typedef char TypeSize[(sizeof(struct type)==24)?1:-1];
extern TYPE basicTypes[TYP_MAX];
extern carve_t carveTYPE;
TYPE MakeType(type_id id);
TYPE MakeClassType(void);
TYPE CheckDupType(TYPE type);
TYPE typeDuplicated(TYPE type, TYPE *head);
int dupCompare(TYPE first, TYPE second);
void typeFree(TYPE type);
void *RingPromote(void **head, void *item, void *previous);
TYPE MakeConstTypeOf(TYPE type);
TYPE MakeConstModifier(void);
TYPE MakeFlagModifier(unsigned short flag);
TYPE makeFullModifier(unsigned short flag, void *base, void *pragma);
TYPE MakePointerTo(TYPE type);
TYPE MakeTypeOf(TYPE type, TYPE base);
TYPE GetBasicType(type_id id);
TYPE TypePtrToVoid(void);
TYPE TypeVoidFunOfPtrVoid(void);
TYPE FunctionDeclarationType(TYPE type);
TYPE TypedefRemove(TYPE type);
arg_list *TypeArgList(TYPE type);
struct symbol;
unsigned short FunctionThisFlags(struct symbol *sym);
TYPE TypedefModifierRemove(TYPE type);
TYPE MakePlusPlusFunction(TYPE type);
TYPE GetBasicUInt(void); /* Original wrapper name is not recovered. */
TYPE __cdecl MakeSimpleFunction(TYPE result, ...);
TYPE buildFnType(TYPE result, va_list count_args, va_list use_args);
arg_list *AllocArgList(int count, int permanent);
arg_list *InitArgList(arg_list *args);
void ClassInit(void);
void *PragmaLookup(char *name, unsigned index);
#endif

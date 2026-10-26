#ifndef WPP_CLASS_H
#define WPP_CLASS_H
#include <stddef.h>
#include "types.h"
#include <recon.h>
/* Actual old class-state fields occupy99 bytes in the100-byte GLOBAL_STACK
 * union slot; default-argument and later modifier-type fields are absent. */
typedef struct class_data CLASS_DATA;
#pragma pack(1)
struct base_class {
    struct base_class *next;
    TYPE type;
    long delta;
    unsigned short vb_index,flag;
};
struct class_data {
    CLASS_DATA *next,*inline_data;
    char *name;
    struct symbol *sym;
    TYPE type;
    struct scope *scope;
    struct class_info *info;
    struct base_class *bases,*base_vbptr,*base_vfptr;
    struct decl_info *inlines;
    void *fn_pragma;
    unsigned errors,start,offset,boffset,max_align,pack_amount;
    unsigned vf_offset,vb_offset;
    unsigned short vf_index,vb_index,tflag,fn_flags;
    unsigned perm;
    unsigned char bit_offset,bit_available,bit_unit_width;
    unsigned flags;
};
#pragma pack()
typedef char ClassDataFields[(sizeof(CLASS_DATA)==99 &&
    offsetof(CLASS_DATA,flags)==95 && offsetof(CLASS_DATA,tflag)==84)?1:-1];
enum {
    CD_DEFINED=2,CD_SPECIFIC_DEFN=0x800,CD_LOCAL_CLASS=4,CD_NESTED_CLASS=8,CD_IS_UNION=0x20,
    CD_ALLOW_TYPEDEF=0x10,CD_IS_EXPLICIT=0x40,CD_OWN_VFPTR=0x80,CD_CLASS_TEMPLATE=0x400,
    CD_HAS_CONST_OPEQ=0x10000,CD_HAS_NONCONST_OPEQ=0x20000,
    CD_ZERO_ARRAY_DEFD=0x1000,CD_MEMBER_MOD_ADJUST=0x2000,
    CD_HAS_CONST_COPY=0x4000,CD_HAS_NONCONST_COPY=0x8000,
    CD_A_PRIVATE=0x40000,CD_A_PROTECTED=0x80000,CD_A_PUBLIC=0x100000,
    CD_A_CONST=0x200000,CD_A_REFERENCE=0x400000
};
unsigned char ClassName(struct parse_tree_node *id,unsigned char declaration);
void ClassStart(void);
void ClassPush(CLASS_DATA *state);
void ClassPop(CLASS_DATA *state);
DECL_SPEC *ClassRefDef(void);
TYPE ClassUnboundTemplate(char *name);
void ClassInitState(unsigned short flags,unsigned char extra,TYPE modifiers);
#endif

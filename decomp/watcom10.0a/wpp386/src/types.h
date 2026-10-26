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
struct type {
    TYPE next, of;
    union {
        struct { void *init1, *init2; } i;
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
TYPE GetBasicUInt(void); /* Original wrapper name is not recovered. */
TYPE __cdecl MakeSimpleFunction(TYPE result, ...);
TYPE buildFnType(TYPE result, va_list count_args, va_list use_args);
arg_list *AllocArgList(int count, int permanent);
arg_list *InitArgList(arg_list *args);
void ClassInit(void);
void *PragmaLookup(char *name, unsigned index);
#endif

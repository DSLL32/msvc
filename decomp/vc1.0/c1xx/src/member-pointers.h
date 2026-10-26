#ifndef MEMBER_POINTERS_H
#define MEMBER_POINTERS_H
#include "declarations.h"
#include <recon.h>

/* The twelve-byte key distinguishes a virtual member's generated thunk
   kind and this displacement. The hash table owns a permanent copy. */
typedef struct MemberPointerThunkKey {
    CppSymbol *function;
    unsigned kind;
    int displacement;
} MemberPointerThunkKey;
/* Deferred vbtable-conversion identity: both owner classes and direction.
   Original constructor4d710 initializes exactly these three DWORDs. */
typedef struct MemberConversionKey {
    CppSymbol *target, *source;
    int forward;
} MemberConversionKey;
typedef char MemberConversionKeySizeMustBe12[
    (sizeof(MemberConversionKey) == 12) ? 1 : -1];
typedef char MemberPointerThunkKeySizeMustBe12[
    (sizeof(MemberPointerThunkKey) == 12) ? 1 : -1];
/* Comparison lowering callbacks receive one expression and three operator
   bytes; every implementation returns an expression and removes four words. */
typedef IrToken *(__stdcall *MemberPointerComparison)(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
IrToken *__stdcall dispatch_member_pointer_comparison(IrToken *expression,
    MemberPointerComparison compare);
IrToken *__stdcall compare_multiple_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
IrToken *__stdcall compare_virtual_data_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
IrToken *__stdcall compare_general_data_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
IrToken *__stdcall compare_virtual_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
IrToken *__stdcall compare_general_method_pointer(IrToken *expression,
    unsigned char comparison, unsigned char combine, unsigned char null_combine);
MemberPointerThunkKey *__stdcall new_member_pointer_thunk_key(CppSymbol *member, short arena);
MemberPointerThunkKey *__stdcall copy_member_pointer_thunk_key(MemberPointerThunkKey *key, short arena);
CppSymbol *__stdcall emit_member_pointer_virtual_thunk(MemberPointerThunkKey *key);
int __stdcall virtual_member_uses_far_table(CppSymbol *member);
int __stdcall member_uses_far_this(CppSymbol *member);
int __stdcall member_uses_far_code(CppSymbol *member);
#endif

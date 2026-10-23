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
typedef char MemberPointerThunkKeySizeMustBe12[
    (sizeof(MemberPointerThunkKey) == 12) ? 1 : -1];
MemberPointerThunkKey *__stdcall new_member_pointer_thunk_key(CppSymbol *member, short arena);
MemberPointerThunkKey *__stdcall copy_member_pointer_thunk_key(MemberPointerThunkKey *key, short arena);
CppSymbol *__stdcall emit_member_pointer_virtual_thunk(MemberPointerThunkKey *key);
#endif

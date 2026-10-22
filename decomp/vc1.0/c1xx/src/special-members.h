#ifndef SPECIAL_MEMBERS_H
#define SPECIAL_MEMBERS_H
#include "class-finalization.h"
#include <recon.h>
/* One 28-byte record per default/copy constructor, assignment and destructor.
   The word view also admits the native model -1 prefix accesses used while
   classifying declarations without an associated this type. */
typedef struct SpecialMemberFields {
    int can_generate, nontrivial, declared;
    int available, nonprivate, ambiguous;
    unsigned char qualifiers, reserved[3];
} SpecialMemberFields;
typedef union SpecialMemberState {
    SpecialMemberFields fields;
    unsigned words[7];
} SpecialMemberState;
typedef char SpecialMemberStateSizeMustBe28[(sizeof(SpecialMemberState) == 28) ? 1 : -1];
typedef char SpecialMemberQualifiersOffsetMustBe24[(offsetof(SpecialMemberFields, qualifiers) == 24) ? 1 : -1];
int __stdcall special_member_model(unsigned short pointer_kind);
void __stdcall inherit_base_special_members(CppSymbol *tag, SpecialMemberState *states, int model);
void __stdcall inherit_field_special_members(CppSymbol *tag, SpecialMemberState *states, int model);
void __stdcall inspect_declared_special_members(CppSymbol *tag, SpecialMemberState *states,
    int *constructors, int *assignments);
/* Native RET 28: the final count is passed by callers but not read here. */
void __stdcall complete_special_member(CppSymbol *tag, SpecialMemberState *states,
    int kind, int model, unsigned warning, int enabled, int unused_count);
void __stdcall merge_subobject_special_members(CppSymbol *tag, CppSymbol *subobject,
    SpecialMemberState *states, int model);
int __stdcall is_const_or_reference_member(CppSymbol *member);
void __stdcall record_special_member(CppSymbol *tag, SpecialMemberState *states,
    int kind, int model, unsigned nonprivate, unsigned warning);
CppSymbol *__stdcall create_implicit_special_member(CppSymbol *tag, int kind, int model,
    unsigned short attributes, int generated);
CppSymbol *__stdcall make_copy_parameter(unsigned short pointer_kind, unsigned short qualifiers);
void __stdcall defer_generated_member(CppSymbol *member);
int __stdcall find_special_member_model(CppSymbol *tag, unsigned char kind, int first);
#endif

#ifndef SPECIAL_MEMBER_LOOKUP_H
#define SPECIAL_MEMBER_LOOKUP_H
#include "special-members.h"
#include "object-initializers.h"
#include <recon.h>
typedef int (__cdecl *SpecialMemberPredicate)(CppSymbol *candidate);
CppSymbol *__stdcall lookup_special_member(CType *type, char *name, SpecialMemberPredicate accept);
#endif

#ifndef MEMBER_ACCESS_H
#define MEMBER_ACCESS_H
#include "member-expressions.h"
#include <recon.h>
CppSymbol *__stdcall declaring_class_for_symbol(CppSymbol *symbol);
void reset_member_access_failure(void);
void __stdcall set_initial_member_access_failure(unsigned char protection,
    CppSymbol *member, CppSymbol *owner);
int __stdcall find_friend_access(CppSymbol *candidate, CppSymbol *owner);
QualifierBinding *__stdcall skip_base_qualifier_path(QualifierBinding *binding);
unsigned char __stdcall virtual_base_access_level(CppSymbol *base, CppSymbol *derived);
int __stdcall has_member_access_declaration(CppSymbol *owner, BaseClassLink *base,
    CppSymbol *member);
unsigned char __stdcall combine_member_access(unsigned char member_access,
    unsigned char base_access);
void __stdcall set_inherited_member_access_failure(CppSymbol *member, CppSymbol *base,
    unsigned char previous_access, unsigned char member_access, CppSymbol *previous_base);
/* The access table keys are three original pointer words in this order. */
typedef struct AccessDeclaration {
    CppSymbol *owner;
    BaseClassLink *base;
    CppSymbol *member;
} AccessDeclaration;
typedef char AccessDeclarationSizeMustBe12[(sizeof(AccessDeclaration) == 12) ? 1 : -1];
AccessDeclaration *__stdcall new_access_declaration(CppSymbol *owner,
    BaseClassLink *base, CppSymbol *member, short arena);
HashTable *access_declaration_table(void);
extern CppSymbol *access_failure_owner, *access_failure_member;
extern unsigned char access_failure_protection;
extern int access_through_virtual_base;
unsigned __stdcall base_qualifier_displacement(QualifierBinding *path);
#endif

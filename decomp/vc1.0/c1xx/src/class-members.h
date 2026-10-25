#ifndef CLASS_MEMBERS_H
#define CLASS_MEMBERS_H
#include "lexer.h"
#include <recon.h>
/* Original class-member cursor:the selected member is its second word. */
typedef struct ClassMemberIterator {
    CppSymbol *next, *member;
} ClassMemberIterator;
typedef char ClassMemberIteratorSizeMustBe8[(sizeof(ClassMemberIterator) == 8) ? 1 : -1];
typedef char ClassMemberIteratorMemberOffsetMustBe4[(offsetof(ClassMemberIterator, member) == 4) ? 1 : -1];
void __stdcall begin_class_members(ClassMemberIterator *iterator, CppSymbol *tag);
int __stdcall next_class_member(ClassMemberIterator *iterator);

void __stdcall record_friend(CppSymbol *owner, DeclarationType *type, CppSymbol *function);
typedef struct FriendRecord {
    unsigned owner_index;
    CppSymbol *symbol;
    struct FriendRecord *next;
} FriendRecord;
typedef char FriendRecordSizeMustBe12[(sizeof(FriendRecord) == 12) ? 1 : -1];
typedef char FriendRecordSymbolOffsetMustBe4[(offsetof(FriendRecord, symbol) == 4) ? 1 : -1];
typedef char FriendRecordNextOffsetMustBe8[(offsetof(FriendRecord, next) == 8) ? 1 : -1];
void __stdcall append_friend_record(FriendRecord *record);
FriendRecord *__stdcall find_friend_record(unsigned *owner_index);
void __stdcall browser_friend_reference(CppSymbol *symbol);
char *__stdcall member_access_keyword(unsigned access);
void __stdcall validate_anonymous_aggregate(DeclarationType *type);
void __stdcall declare_anonymous_aggregate(CType *type);
void __stdcall select_destructor_type(CppSymbol *member, DeclarationType *context,
    CType **base, StorageSpec *storage);
void __stdcall declare_friend_member(DeclarationType *context, CppSymbol *member,
    QualifiedDeclarator *qualified, CppSymbol *owner, StorageSpec storage, int pure);
void __stdcall apply_member_access_declaration(CppSymbol *owner, QualifiedDeclarator *qualified);
TypeShape *__stdcall last_type_shape(TypeShape *shape);
void __stdcall inherit_class_dll_storage(DeclarationType *context, CppSymbol *member);
void __stdcall capture_inline_member_declaration(CppSymbol *symbol, int generated_kind, int generation_context, unsigned short pointer_kind);
CppSymbol *__stdcall find_class_member_name(char *name);
void __stdcall layout_bit_field(CppSymbol *member, char *name, unsigned short flags, int new_member);
/* Original 0x2fef0 uses three stack arguments and RET 12. The decompiler's
   apparent this pointer is dead upper-ECX state after a byte write to CL. */
void __stdcall configure_member_function(StorageSpec storage, CppSymbol *member, int pure);
int __stdcall has_qualified_this_type(CType *type);
typedef FunctionParameters *(__stdcall *ParameterTransform)(FunctionParameters *parameters);
CType *__stdcall transform_function_parameters(CType *type, ParameterTransform transform);
FunctionParameters *__stdcall mark_static_parameters(FunctionParameters *parameters);
void __stdcall classify_constructor(CppSymbol *member);
void __stdcall classify_copy_assignment(CppSymbol *member);
int __stdcall overrides_virtual_member(CppSymbol *member, CppSymbol *owner, int diagnose);
int __stdcall compatible_override_return(CType *member, CType *inherited);
void __stdcall set_pure_virtual_member(CppSymbol *member, CppSymbol *owner, StorageSpec storage);
void __stdcall validate_function_default_arguments(CppSymbol *function);
int __stdcall validate_union_member(CppSymbol *owner, CppSymbol *member);
CppSymbol *__stdcall class_type_of_member(CppSymbol *member);
unsigned __stdcall layout_data_member(CppSymbol *member);
void __stdcall align_class_member(unsigned alignment);
unsigned __stdcall class_alignment_padding(unsigned alignment);
unsigned __stdcall alignment_padding_at(unsigned alignment, unsigned offset);
int __stdcall addition_overflows(int left, unsigned right, short unsigned_arithmetic, unsigned short bytes);
unsigned __stdcall add_member_offset(unsigned offset, unsigned displacement);
void __stdcall create_static_member_alias(CppSymbol *member);
void __stdcall insert_class_member(CppSymbol *symbol);
void __stdcall append_class_members(CppSymbol *first, CppSymbol *last);
CppSymbol *__stdcall make_overload_symbol(CppSymbol *symbol);
CppSymbol *__stdcall reconcile_declaration(CppSymbol *existing, CppSymbol *declaration);
void __stdcall prepend_class_member(CppSymbol *member);
char *__stdcall mangle_static_member_name(CppSymbol *owner, CppSymbol *member);
void __stdcall register_static_member_alias(CppSymbol *member, CppSymbol *alias);
#endif

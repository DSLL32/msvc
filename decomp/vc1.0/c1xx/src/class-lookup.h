#ifndef CLASS_LOOKUP_H
#define CLASS_LOOKUP_H
#include "declarations.h"
#include <recon.h>

/* Stack-local state shared by recursive base/member lookup. */
typedef struct ClassNameSearch {
    char *name;
    unsigned categories;
    HashTable *visited_virtual_bases, *shadowed_virtual_bases;
    Vector *candidates;
} ClassNameSearch;
typedef char ClassNameSearchSizeMustBe20[(sizeof(ClassNameSearch) == 20) ? 1 : -1];
QualifierResolution *__stdcall search_class_name_paths(QualifierResolution *path,
    ClassNameSearch *search);
/* Virtual-base group iteration writes the selected group plus its first
   two fields; callers may use only the class at +16. */
typedef struct VirtualBaseSearch {
    Vector *groups;
    short index;
    unsigned short reserved;
    VirtualTableGroup *group;
    int active;
    CppSymbol *tag;
    unsigned field14;
} VirtualBaseSearch;
typedef char VirtualBaseSearchSizeMustBe24[(sizeof(VirtualBaseSearch) == 24) ? 1 : -1];
typedef char VirtualBaseSearchTagOffsetMustBe16[(offsetof(VirtualBaseSearch, tag) == 16) ? 1 : -1];
void __stdcall begin_virtual_base_search(VirtualBaseSearch *search, CppSymbol *tag);
int __stdcall next_virtual_base_search(VirtualBaseSearch *search);
int __stdcall class_candidate_is_distinct(CppSymbol *symbol, Vector *candidates);
int __stdcall qualifier_path_is_hidden(QualifierBinding *path, HashTable *hidden);
void __stdcall prune_hidden_class_paths(QualifierResolution *paths, HashTable *hidden);
CppSymbol *__stdcall find_matching_overload(CppSymbol *functions, CType *type,
    int merge_defaults);
/* Shared native search state for list and completed-index lookup. */
typedef struct ClassMemberSearch {
    int indexed;
    union { CppSymbol *next; CppSymbol **items; } position;
    unsigned remaining;
    char *name;
} ClassMemberSearch;
typedef char ClassMemberSearchSizeMustBe16[(sizeof(ClassMemberSearch) == 16) ? 1 : -1];
typedef char ClassMemberSearchPositionOffsetMustBe4[(offsetof(ClassMemberSearch, position) == 4) ? 1 : -1];
typedef char ClassMemberSearchCountOffsetMustBe8[(offsetof(ClassMemberSearch, remaining) == 8) ? 1 : -1];
typedef char ClassMemberSearchNameOffsetMustBe12[(offsetof(ClassMemberSearch, name) == 12) ? 1 : -1];
void __stdcall begin_class_member_search(ClassMemberSearch *search, char *name, CppSymbol *tag);
CppSymbol *__stdcall next_class_member_search(ClassMemberSearch *search);
CppSymbol *__stdcall find_class_special_member(CppSymbol *tag, char *name);
QualifierBinding *__stdcall attach_qualifier_parent(QualifierBinding *binding, QualifierBinding *parent);
#endif

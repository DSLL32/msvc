#ifndef CLASS_LOOKUP_H
#define CLASS_LOOKUP_H
#include "declarations.h"

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
void __stdcall prune_hidden_class_paths(QualifierResolution *paths, HashTable *hidden);
CppSymbol *__stdcall find_matching_overload(CppSymbol *functions, CType *type,
    int merge_defaults);
#endif

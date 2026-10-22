#ifndef LINKAGE_H
#define LINKAGE_H
#include <recon.h>
typedef enum LanguageLinkage {
    LINKAGE_DEFAULT = 0,
    LINKAGE_CPP = 1,
    LINKAGE_C = 2
} LanguageLinkage;
extern int linkage_depth;
extern LanguageLinkage linkage_stack[10];
void __stdcall push_linkage(LanguageLinkage linkage);
#endif

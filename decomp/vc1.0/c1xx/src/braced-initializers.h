#ifndef BRACED_INITIALIZERS_H
#define BRACED_INITIALIZERS_H
#include "object-initializers.h"
#include <recon.h>
typedef struct BracedInitializer {
    CppSymbol *symbol;
    ClassInfo *class_info;
    CType *type;
    unsigned offset;
    unsigned char flags, phase;
    unsigned short reserved;
} BracedInitializer;
typedef char BracedInitializerSizeMustBe20[(sizeof(BracedInitializer) == 20) ? 1 : -1];
typedef char BracedInitializerFlagsOffsetMustBe16[(offsetof(BracedInitializer, flags) == 16) ? 1 : -1];
extern BracedInitializer braced_initializers[32], *current_braced_initializer;
extern int braced_initializer_depth, initializer_excess_reported;
extern CType *initializer_selected_type;
void __stdcall push_braced_initializer(CppSymbol *symbol);
void flush_initializer_constructors(void);
int __stdcall advance_braced_initializer(int mode);
void __stdcall copy_initializer_symbol(CppSymbol *destination, ClassInfo *class_info, CppSymbol *source);
typedef char InitializerCopiedClassInfoMustBe60[(sizeof(ClassInfo) == 60) ? 1 : -1];
#endif

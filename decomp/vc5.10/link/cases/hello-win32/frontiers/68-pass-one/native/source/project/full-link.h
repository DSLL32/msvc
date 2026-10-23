#ifndef LINK_FULL_LINK_H
#define LINK_FULL_LINK_H
#include <windows.h>
#include <stddef.h>
#include "arguments.h"
#include "symbols.h"
#include <recon.h>

typedef struct OutputModule OutputModule;
typedef struct OutputLibrary OutputLibrary;
typedef struct OutputSection OutputSection;
typedef struct ContributionGroup ContributionGroup;
typedef struct Contribution Contribution;
typedef struct ComdatRoot ComdatRoot;

/* Only fields consumed by FullLink are named here. These are prefixes, not
 * claims about an allocation's complete size or its as-yet unknown tail. */
/* The initializer allocates these complete records (156 and68 bytes). Only
 * its name/library fields have yet been consumed; the other fields remain
 * explicitly unnamed rather than borrowing another linker's layout. */
struct OutputLibrary {
    char *name;
    DWORD reserved04[6];
    /* Opaque library-owned index buffers, released after input processing. */
    void *index_data[5];
    DWORD flags, reserved34[2];
    OutputLibrary *next;
    DWORD reserved40;
};
struct OutputModule {
    DWORD reserved00;
    char *name;
    BYTE reserved08[92];
    OutputLibrary *library;
    BYTE reserved68[52];
};
struct OutputSection {
    char *name;
    DWORD rva, reserved08, raw_size, virtual_size, reserved14, file_offset;
    DWORD reserved1c, characteristics, original_characteristics, reserved28[2];
    WORD index, reserved32;
    DWORD reserved34[2];
    OutputSection *merge_target;
    ContributionGroup *groups;
    OutputSection *next;
};
typedef char OutputSectionSize[(sizeof(OutputSection)==72)?1:-1];
struct ContributionGroup {
    char *name;
    DWORD contribution_count, rva, reserved0c, file_offset, size;
    BYTE alignment, reserved19[3];
    Contribution *first, *last;
    ContributionGroup *next;
    OutputSection *section;
};
struct Contribution {
    DWORD rva, characteristics, raw_size, file_offset, reserved10, reserved14;
    OutputModule *module;
    ContributionGroup *group;
    Contribution *next;
};
/* A COFF symbol is the public prefix of the linker's extended symbol record.
 * The flags and owner slots below are read by FullLink; later tail members
 * remain unknown until their users are reconstructed. */
struct StringNode {
    IMAGE_SYMBOL symbol;
    WORD reserved12;
    Contribution *contribution;
    DWORD final_value, reserved1c[5];
    BYTE flags;
};
typedef struct SectionList {
    OutputSection *first, **last;
} SectionList;
typedef struct SectionEnumerator {
    BYTE (__stdcall *next)(struct SectionEnumerator *);
    void (__stdcall *end)(struct SectionEnumerator *);
    OutputSection *first, *current;
} SectionEnumerator;
typedef struct MacResource {
    struct MacResource *next;
    DWORD kind;
    const char *filename;
    void *data;
    DWORD size;
    Contribution *contribution;
} MacResource;

typedef char OutputLibrarySize[(sizeof(OutputLibrary)==68)?1:-1];
typedef char LibraryIndexDataAt1c[(offsetof(OutputLibrary,index_data)==0x1c)?1:-1];
typedef char LibraryFlagsAt30[(offsetof(OutputLibrary,flags)==0x30)?1:-1];
typedef char LibraryNextAt3c[(offsetof(OutputLibrary,next)==0x3c)?1:-1];
typedef char OutputModuleSize[(sizeof(OutputModule)==156)?1:-1];
typedef char OutputModuleLibraryAt64[(offsetof(OutputModule,library)==0x64)?1:-1];
typedef char SectionFlagsAt20[(offsetof(OutputSection,characteristics)==0x20)?1:-1];
typedef char SectionFileOffsetAt18[(offsetof(OutputSection,file_offset)==0x18)?1:-1];
typedef char SectionIndexAt30[(offsetof(OutputSection,index)==0x30)?1:-1];
typedef char GroupAlignmentAt18[(offsetof(ContributionGroup,alignment)==0x18)?1:-1];
typedef char GroupFirstAt1c[(offsetof(ContributionGroup,first)==0x1c)?1:-1];
typedef char GroupNextAt24[(offsetof(ContributionGroup,next)==0x24)?1:-1];
typedef char GroupSectionAt28[(offsetof(ContributionGroup,section)==0x28)?1:-1];
typedef char ContributionGroupAt1c[(offsetof(Contribution,group)==0x1c)?1:-1];
typedef char ContributionNextAt20[(offsetof(Contribution,next)==0x20)?1:-1];
typedef char SymbolOwnerAt14[(offsetof(StringNode,contribution)==0x14)?1:-1];
typedef char SymbolValueAt18[(offsetof(StringNode,final_value)==0x18)?1:-1];
typedef char SymbolFlagsAt30[(offsetof(StringNode,flags)==0x30)?1:-1];

#endif

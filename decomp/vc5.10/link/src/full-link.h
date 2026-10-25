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
typedef struct SectionFileMap {
    DWORD virtual_address,line_number_offset;
} SectionFileMap;
typedef char SectionFileMapSize[(sizeof(SectionFileMap)==8)?1:-1];

/* Only fields consumed by reached reconstruction logic are named here. These are prefixes, not
 * claims about an allocation's complete size or its as-yet unknown tail. */
/* The initializer allocates complete 156-byte module and 68-byte library records.
 * Name fields as their producers/consumers are reached; retain unknown gaps. */
struct OutputLibrary {
    char *name;
    OutputModule **modules;
    DWORD module_capacity;
    LONG module_count;
    BYTE active, reserved11[3];
    const char *path;
    DWORD reserved18;
    /* Opaque library-owned index buffers, released after input processing. */
    void *index_data[5];
    DWORD flags, input_tag;
    OutputModule *module_head;
    OutputLibrary *next;
    DWORD reserved40;
#ifdef __cplusplus
    OutputModule *RECON_THISCALL FindModule(const char *,const void *);
#endif
};
struct OutputModule {
    /* Duplicated original filename, or a borrowed opaque identity when absent. */
    const void *identity;
    char *name;
    DWORD module_index, symbol_offset, symbol_count;
    WORD optional_header_size, characteristics;
    DWORD section_count;
    BYTE reserved1c[16];
    SectionFileMap *section_map;
    DWORD reserved30, input_tag, timestamp, file_size;
    DWORD section_offset;
    void *optional_data;
    IMAGE_SECTION_HEADER *section_headers;
    BYTE *section_flags;
    DWORD auxiliary;
    BYTE reserved54[12];
    IMAGE_SECTION_HEADER *saved_section_headers;
    OutputLibrary *library;
    /* Original allocation uses 64 bytes per section; the public Contribution
       prefix does not yet describe that complete record. */
    void *contributions;
    OutputModule *next;
    BYTE reserved70[20];
    WORD *incremental_state;
    DWORD reserved88;
    DWORD *symbol_bits[2], *symbol_indices;
    BYTE flags, reserved99[3];
#ifdef __cplusplus
    IMAGE_SECTION_HEADER *RECON_THISCALL ReadSectionHeaders(IMAGE_SECTION_HEADER **);
#endif
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
    DWORD rva, characteristics, raw_size, file_offset;
    Contribution *associated_contribution;
    DWORD padding;
    OutputModule *module;
    ContributionGroup *group;
    Contribution *next;
    WORD relocation_count,line_number_count;
    DWORD relocation_offset,input_raw_offset;
    BYTE reserved30[16];
};
typedef char ContributionSize[(sizeof(Contribution)==64)?1:-1];
typedef char ContributionInputFields[(offsetof(Contribution,relocation_count)==0x24 &&
    offsetof(Contribution,line_number_count)==0x26 &&
    offsetof(Contribution,relocation_offset)==0x28 &&
    offsetof(Contribution,input_raw_offset)==0x2c)?1:-1];
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
typedef char LibraryCreatorFields[(offsetof(OutputLibrary,modules)==4 &&
    offsetof(OutputLibrary,module_capacity)==8 && offsetof(OutputLibrary,module_count)==12 &&
    offsetof(OutputLibrary,active)==16 && offsetof(OutputLibrary,path)==20)?1:-1];
typedef char LibraryIndexDataAt1c[(offsetof(OutputLibrary,index_data)==0x1c)?1:-1];
typedef char LibraryFlagsAt30[(offsetof(OutputLibrary,flags)==0x30)?1:-1];
typedef char LibraryNextAt3c[(offsetof(OutputLibrary,next)==0x3c)?1:-1];
typedef char OutputModuleSize[(sizeof(OutputModule)==156)?1:-1];
typedef char ModuleConstructorFields[(offsetof(OutputModule,module_index)==8 &&
    offsetof(OutputModule,symbol_offset)==12 && offsetof(OutputModule,symbol_count)==16 &&
    offsetof(OutputModule,section_count)==24 && offsetof(OutputModule,section_offset)==64 &&
    offsetof(OutputModule,optional_data)==68 && offsetof(OutputModule,section_headers)==72 &&
    offsetof(OutputModule,section_flags)==76 && offsetof(OutputModule,auxiliary)==80 &&
    offsetof(OutputModule,contributions)==104 && offsetof(OutputModule,next)==108 &&
    offsetof(OutputModule,incremental_state)==132 && offsetof(OutputModule,symbol_bits)==140 &&
    offsetof(OutputModule,symbol_indices)==148 && offsetof(OutputModule,flags)==152)?1:-1];
typedef char LibraryModuleHeadAt38[(offsetof(OutputLibrary,module_head)==56)?1:-1];
typedef char OutputModuleSectionMapAt2c[(offsetof(OutputModule,section_map)==0x2c)?1:-1];
typedef char SavedSectionHeadersAt60[(offsetof(OutputModule,saved_section_headers)==0x60)?1:-1];
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

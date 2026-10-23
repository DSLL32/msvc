#ifndef LINK_FULL_LINK_H
#define LINK_FULL_LINK_H
#include <windows.h>
#include <stddef.h>
#include "arguments.h"
#include <recon.h>

typedef struct OutputModule OutputModule;
typedef struct OutputSection OutputSection;
typedef struct ContributionGroup ContributionGroup;
typedef struct Contribution Contribution;
typedef struct ComdatRoot ComdatRoot;

/* Only fields consumed by FullLink are named here. These are prefixes, not
 * claims about an allocation's complete size or its as-yet unknown tail. */
struct OutputSection {
    char *name;
    DWORD rva, reserved08, raw_size, virtual_size, reserved14, file_offset;
    DWORD reserved1c, characteristics, reserved24[3];
    WORD index;
};
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

typedef char SectionFlagsAt20[(offsetof(OutputSection,characteristics)==0x20)?1:-1];
typedef char SectionFileOffsetAt18[(offsetof(OutputSection,file_offset)==0x18)?1:-1];
typedef char SectionIndexAt30[(offsetof(OutputSection,index)==0x30)?1:-1];
typedef char GroupAlignmentAt18[(offsetof(ContributionGroup,alignment)==0x18)?1:-1];
typedef char GroupFirstAt1c[(offsetof(ContributionGroup,first)==0x1c)?1:-1];
typedef char GroupNextAt24[(offsetof(ContributionGroup,next)==0x24)?1:-1];
typedef char GroupSectionAt28[(offsetof(ContributionGroup,section)==0x28)?1:-1];
typedef char ContributionGroupAt1c[(offsetof(Contribution,group)==0x1c)?1:-1];
typedef char ContributionNextAt20[(offsetof(Contribution,next)==0x20)?1:-1];

#endif

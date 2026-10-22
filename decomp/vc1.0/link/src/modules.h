#ifndef LINK_MODULES_H
#define LINK_MODULES_H
#include "linker-types.h"
typedef struct ComdatNode ComdatNode;
typedef struct ComdatModule ComdatModule;
typedef struct Module Module;
typedef struct Library Library;
typedef struct Contribution Contribution;
typedef struct Section Section;
typedef struct Group Group;
typedef struct ExternalLink ExternalLink;
struct Contribution {
    unsigned long input_rva, rva, flags, raw_size;
    unsigned short relocation_count, line_count;
    unsigned long relocation_offset, line_offset, data_offset;
    unsigned long file_offset;
    union { unsigned long checksum; Contribution *associated; } comdat;
    short comdat_section;
    unsigned char comdat_selection,padding;
    IMAGE_RELOCATION *relocations;
    ComdatNode *graph;
    Module *module;
    Group *group;
    Contribution *next;
};
struct Module {
    union { char *display_name; unsigned long member_offset; } name;
    char *path;
    unsigned long machine, symbol_offset, symbol_count;
    unsigned short optional_header_size, characteristics;
    unsigned long section_count, contribution_count;
    ExternalLink *common_symbols;
    ExternalLink *public_symbols;
    ComdatModule *comdat;
    Contribution *contributions;
    Library *library;
    Module *next;
};
struct Library {
    int file;
    char *name;
    unsigned long flags;
    unsigned long symbol_count;
    unsigned long *member_offsets;
    unsigned short *symbol_indices;
    char *symbol_name_buffer;
    char **symbol_names;
    char *long_names;
    unsigned long search_flags;
    Module *modules;
    Library *next;
};
struct Group {
    char *name;
    unsigned long contribution_count, rva, file_offset, size;
    unsigned char alignment;
    Contribution *last;
    Section *section;
    Contribution *first;
    Group *next;
};
struct Section {
    char *name;
    unsigned long rva, file_end, raw_size, virtual_size, file_offset;
    unsigned long relocation_count, line_offset, relocation_offset, characteristics;
    unsigned long source_name_capacity;
    unsigned short line_count, index;
    unsigned long mac_resource_id, mac_type;
    unsigned short merge_count;
    Section *merge;
    Group *groups;
    Section *next;
};
typedef struct SectionList { Section *first, **tail; } SectionList;
int compare_sections_by_rva(const void *,const void *);
typedef struct DefaultLibrary {
    struct DefaultLibrary *next;
    char *name;
    unsigned char enabled;
} DefaultLibrary;
typedef struct LibraryList {
    Library *first, **tail;
    unsigned char exclude_defaults;
    DefaultLibrary *default_libraries;
} LibraryList;
typedef struct SectionEnumerator {
    unsigned char (*next)(struct SectionEnumerator *);
    void (*end)(struct SectionEnumerator *);
    Section *first, *current;
} SectionEnumerator;
typedef struct LibraryEnumerator {
    unsigned char (*next)(struct LibraryEnumerator *);
    void (*end)(struct LibraryEnumerator *);
    Library *first, *current;
} LibraryEnumerator;
typedef struct GroupEnumerator {
    unsigned char (*next)(struct GroupEnumerator *);
    void (*end)(struct GroupEnumerator *);
    Section *section;
    Group *current;
} GroupEnumerator;
typedef struct ModuleEnumerator {
    unsigned char (*next)(struct ModuleEnumerator *);
    void (*end)(struct ModuleEnumerator *);
    Library *library;
    Module *current;
} ModuleEnumerator;

typedef struct GroupContributionEnumerator {
    unsigned char (*next)(struct GroupContributionEnumerator *);
    void (*end)(struct GroupContributionEnumerator *);
    Group *group;
    Contribution *current;
} GroupContributionEnumerator;
void initialize_group_contribution_enumerator(GroupContributionEnumerator *,Group *);
unsigned char next_group_contribution(GroupContributionEnumerator *);
void end_group_contribution_enumerator(GroupContributionEnumerator *);
int compare_contribution_module_names(const void *,const void *);
void sort_contributions_by_module(Group *);
void move_contribution_to_module_tail(Contribution *);

typedef struct ContributionEnumerator {
    unsigned char (*next)(struct ContributionEnumerator *);
    void (*end)(struct ContributionEnumerator *);
    Module *module;
    Contribution *current;
    unsigned long index;
} ContributionEnumerator;
void initialize_contribution_enumerator(ContributionEnumerator *,Module *);
unsigned char next_contribution(ContributionEnumerator *);
void end_contribution_enumerator(ContributionEnumerator *);

extern void (*module_error)(char *,int,char *);
extern unsigned short section_count;
void initialize_modules(void (*)(char *,int,char *),Module **);
Library *create_library(const char *,unsigned long,unsigned char,LibraryList *);
Library *find_library(const char *,Library *,unsigned char);
Module *create_module(const char *,const char *,unsigned long,unsigned short,
    unsigned long,unsigned long,unsigned short,unsigned short,unsigned short,
    Library *,unsigned char *);
Module *find_module(Library *,const char *,unsigned long);
Contribution *create_contribution(const char *,unsigned long,unsigned short,
    unsigned short,unsigned long,unsigned long,unsigned long,unsigned long,
    unsigned long,unsigned long,Module *,unsigned char,SectionList *,IMAGE_OPTIONAL_HEADER *);
Section *create_section(const char *,unsigned long,SectionList *,IMAGE_OPTIONAL_HEADER *);
Section *find_section_for_group(const char *,SectionList *);
Section *find_section_by_name(const char *,SectionList *);
void merge_sections(Section *,Section *);
Section *find_section(const char *,unsigned long,SectionList *,IMAGE_OPTIONAL_HEADER *);
Section *resolve_section(Section *);
Group *create_group(const char *,Section *);
Group *find_group(Section *,const char *);
void split_section_group(const char *,char **,char **);
void normalize_section_flags(unsigned long *,const char *,IMAGE_OPTIONAL_HEADER *);
void initialize_section_enumerator(SectionEnumerator *,SectionList *);
unsigned char next_section(SectionEnumerator *);
void end_section_enumerator(SectionEnumerator *);
void initialize_library_enumerator(LibraryEnumerator *,Library *);
unsigned char next_library(LibraryEnumerator *);
void end_library_enumerator(LibraryEnumerator *);
void initialize_group_enumerator(GroupEnumerator *,Section *);
unsigned char next_group(GroupEnumerator *);
void end_group_enumerator(GroupEnumerator *);
void initialize_module_enumerator(ModuleEnumerator *,Library *);
unsigned char next_module(ModuleEnumerator *);
void end_module_enumerator(ModuleEnumerator *);
void initialize_mac_section(Section *);
void set_mac_resource(IMAGE_SECTION_HEADER *,Section *,unsigned char);
#endif

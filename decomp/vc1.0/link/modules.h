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
struct Contribution {
    unsigned long input_rva, rva, flags, raw_size;
    unsigned short relocation_count, line_count;
    unsigned long relocation_offset, line_offset, data_offset;
    unsigned long file_offset, characteristics, checksum;
    IMAGE_RELOCATION *relocations;
    ComdatNode *graph;
    Module *module;
    Group *group;
    Contribution *next;
};
struct Module {
    union { char *display_name; unsigned long member_offset; } name;
    char *path;
    unsigned long timestamp, symbol_offset, symbol_count;
    unsigned short optional_header_size, characteristics;
    unsigned long section_count, contribution_count;
    void *debug_info, *symbols;
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
    char *symbol_names;
    unsigned long *symbol_offsets, *member_offsets;
    unsigned short *member_indices;
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
    unsigned long line_offset, relocation_offset, contribution_count, characteristics;
    unsigned long alignment;
    unsigned short line_count, index;
    unsigned long mac_resource_id, mac_type, mac_attributes;
    Section *merge;
    Group *groups;
    Section *next;
};
typedef struct SectionList { Section *first, **tail; } SectionList;
typedef struct LibraryList {
    Library *first, **tail;
    unsigned char flags;
    void *default_libraries;
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
void set_mac_resource(const char *,Section *,unsigned char);
#endif

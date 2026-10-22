/* Input modules and libraries, output sections, and sorted contribution groups. */
#include "modules.h"
#include <recon.h>
#pragma intrinsic(strcat,strcmp)
extern unsigned char mac_image, mac_small_model;
void (*module_error)(char *,int,char *);
char split_section_name[9], split_group_name[9];
unsigned short section_count;

void initialize_modules(void (*error)(char *,int,char *),Module **output)
{
    Module *module;
    module_error=error;
    module=calloc(1,sizeof(*module));
    if (!module) module_error(NULL,5,"");
    module->library=calloc(1,sizeof(*module->library));
    if (!module->library) module_error(NULL,5,"");
    module->path=_strdup(" linker defined module");
    if (!module->path) module_error(NULL,5,"");
    module->library->name=_strdup(" linker defined library");
    if (!module->library->name) module_error(NULL,5,"");
    module->contributions=calloc(1,sizeof(*module->contributions));
    if (!module->contributions) module_error(NULL,5,"");
    *output=module;
}

Library *find_library(const char *name,Library *first,unsigned char basename)
{
    LibraryEnumerator scan;
    Library *result=NULL;
    char filename[_MAX_FNAME], extension[_MAX_EXT], combined[_MAX_FNAME];
    char *destination,*source,*candidate;
    initialize_library_enumerator(&scan,first);
    while (next_library(&scan)) {
        candidate=scan.current->name;
        if (basename && candidate) {
            _splitpath(candidate,NULL,NULL,filename,extension);
            destination=combined; source=filename;
            while ((*destination++=*source++)!=0) {}
            --destination; source=extension;
            while ((*destination++=*source++)!=0) {}
            candidate=combined;
        }
        if ((!name && !scan.current->name) ||
            (scan.current->name && name && !_strcmpi(candidate,name))) {
            result=scan.current;
            end_library_enumerator(&scan);
            break;
        }
    }
    return result;
}

Library *create_library(const char *name,unsigned long flags,unsigned char search,LibraryList *list)
{
    Library *library=find_library(name,list->first,0);
    if (!library) {
        library=calloc(1,sizeof(*library));
        if (!library) module_error(NULL,5,"");
        if (name) {
            library->name=_strdup(name);
            if (!library->name) module_error(NULL,5,"");
        } else library->name=NULL;
        library->flags=flags;
        library->modules=NULL;
        library->next=NULL;
        *list->tail=library;
        list->tail=&library->next;
    }
    return library;
}

Module *find_module(Library *library,const char *path,unsigned long member_offset)
{
    ModuleEnumerator scan;
    Module *result=NULL;
    const char *left,*right;
    initialize_module_enumerator(&scan,library);
    while (next_module(&scan)) {
        if (member_offset) {
            if (scan.current->name.member_offset!=member_offset) continue;
        } else {
            left=scan.current->path; right=path;
            while (*left==*right && *left) { ++left; ++right; }
            if (*left!=*right) continue;
        }
        result=scan.current;
        end_module_enumerator(&scan);
        break;
    }
    return result;
}

Module *create_module(const char *display_name,const char *path,
    unsigned long member_offset,unsigned short machine,unsigned long symbol_offset,
    unsigned long symbol_count,unsigned short optional_size,unsigned short characteristics,
    unsigned short sections,Library *library,unsigned char *inserted)
{
    Module *module=find_module(library,path,member_offset);
    if (inserted) *inserted=module==NULL;
    if (!module) {
        module=calloc(1,sizeof(*module)+sections*sizeof(Contribution));
        if (!module) module_error(NULL,5,"");
        if (display_name) {
            module->name.display_name=_strdup(display_name);
            if (!module->name.display_name) module_error(NULL,5,"");
        } else module->name.member_offset=member_offset;
        if (path) {
            module->path=_strdup(path);
            if (!module->path) module_error(NULL,5,"");
        }
        module->symbol_offset=symbol_offset;
        module->symbol_count=symbol_count;
        module->optional_header_size=optional_size;
        module->characteristics=characteristics;
        module->section_count=sections;
        module->contribution_count=0;
        module->contributions=(Contribution *)(module+1);
        module->library=library;
        module->next=library->modules;
        library->modules=module;
    }
    return module;
}

void split_section_group(const char *name,char **section,char **group)
{
    char *delimiter;
    split_group_name[8]=split_section_name[8]=0;
    strncpy(split_group_name,name,8);
    strncpy(split_section_name,name,8);
    *group=split_group_name;
    *section=split_section_name;
    delimiter=strchr(split_section_name,'#');
    if (delimiter) *delimiter=0;
    else {
        delimiter=strchr(split_section_name,'$');
        if (delimiter) *delimiter=0;
    }
}

void normalize_section_flags(unsigned long *flags,const char *name,IMAGE_OPTIONAL_HEADER *header)
{
    const char *left,*right;
    *flags &= mac_image ? 0xffffeff7UL : 0xff0feff7UL;
    if (!_strnicmp(name,".debug",6)) *flags |= IMAGE_SCN_MEM_DISCARDABLE;
    if (header->SectionAlignment<0x1000 && !mac_image && (*flags&IMAGE_SCN_CNT_UNINITIALIZED_DATA))
        *flags=(*flags&~IMAGE_SCN_CNT_UNINITIALIZED_DATA)|IMAGE_SCN_CNT_INITIALIZED_DATA;
    if (header->Subsystem==IMAGE_SUBSYSTEM_NATIVE) {
        left=name; right=".rsrc";
        while (*left==*right && *left) { ++left; ++right; }
        if (*left==*right) *flags |= IMAGE_SCN_MEM_DISCARDABLE;
    }
    if (!(*flags&0xe0)) *flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;
    if (!(*flags&0xe0000000UL))
        *flags |= (*flags&IMAGE_SCN_CNT_CODE) ?
            IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE : IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
}

Section *resolve_section(Section *section)
{
    Section *result=section;
    if (!section) return NULL;
    while (result->merge) {
        result=result->merge;
        if (result==section) module_error("",0x30,section->name);
    }
    return result;
}

Section *find_section(const char *name,unsigned long flags,SectionList *list,IMAGE_OPTIONAL_HEADER *header)
{
    SectionEnumerator scan;
    Section *result=NULL;
    normalize_section_flags(&flags,name,header);
    initialize_section_enumerator(&scan,list);
    while (next_section(&scan)) {
        if (!strncmp(scan.current->name,name,8) && scan.current->characteristics==flags) {
            result=scan.current;
            end_section_enumerator(&scan);
            break;
        }
    }
    return resolve_section(result);
}

Section *create_section(const char *name,unsigned long flags,SectionList *list,IMAGE_OPTIONAL_HEADER *header)
{
    Section *section=find_section(name,flags,list,header);
    if (!section) {
        normalize_section_flags(&flags,name,header);
        section=calloc(1,sizeof(*section));
        if (!section) module_error(NULL,5,"");
        section->name=_strdup(name);
        if (!section->name) module_error(NULL,5,"");
        section->characteristics=flags;
        section->groups=NULL;
        section->merge=NULL;
        if (mac_image && (flags&IMAGE_SCN_CNT_CODE)) {
            initialize_mac_section(section);
            section->mac_type=0x45444f43; /* 'CODE' in the original host byte order. */
            set_mac_resource(NULL,section,1);
            if (section->mac_type==0x45444f43) mac_small_model=0;
        }
        *list->tail=section;
        list->tail=&section->next;
        ++section_count;
    }
    return section;
}

Group *find_group(Section *section,const char *name)
{
    GroupEnumerator scan;
    Group *result=NULL;
    const char *left,*right;
    initialize_group_enumerator(&scan,section);
    while (next_group(&scan)) {
        left=scan.current->name; right=name;
        while (*left==*right && *left) { ++left; ++right; }
        if (*left==*right) {
            result=scan.current;
            end_group_enumerator(&scan);
            break;
        }
    }
    return result;
}

Group *create_group(const char *name,Section *section)
{
    Group *group=find_group(section,name),**link,*next;
    const unsigned char *left,*right;
    if (!group) {
        group=calloc(1,sizeof(*group));
        if (!group) module_error(NULL,5,"");
        group->name=_strdup(name);
        if (!group->name) module_error(NULL,5,"");
        group->contribution_count=0;
        group->section=section;
        group->alignment=1;
        group->first=NULL;
        link=&section->groups;
        while ((next=*link)!=NULL) {
            left=(unsigned char *)group->name; right=(unsigned char *)next->name;
            while (*left==*right && *left) { ++left; ++right; }
            if (*left<*right) break;
            link=&next->next;
        }
        *link=group;
        group->next=next;
    }
    return group;
}

Contribution *create_contribution(const char *name,unsigned long raw_size,
    unsigned short relocations,unsigned short lines,unsigned long relocation_offset,
    unsigned long line_offset,unsigned long data_offset,unsigned long flags,
    unsigned long section_flags,unsigned long rva,Module *module,unsigned char synthetic,
    SectionList *sections,IMAGE_OPTIONAL_HEADER *header)
{
    char *section_name,*group_name;
    Section *section;
    Group *group;
    Contribution *contribution;
    split_section_group(name,&section_name,&group_name);
    section=create_section(section_name,section_flags,sections,header);
    group=create_group(group_name,section);
    if (synthetic) {
        contribution=calloc(1,sizeof(*contribution));
        if (!contribution) module_error(NULL,5,"");
    } else {
        ++group->contribution_count;
        contribution=&module->contributions[module->contribution_count];
    }
    contribution->raw_size=raw_size;
    contribution->relocation_count=relocations;
    contribution->line_count=lines;
    contribution->data_offset=data_offset;
    contribution->relocation_offset=relocation_offset;
    contribution->line_offset=line_offset;
    contribution->group=group;
    contribution->flags=flags;
    contribution->input_rva=rva;
    contribution->module=module;
    contribution->next=NULL;
    ++module->contribution_count;
    if (group->last) group->last->next=contribution;
    else group->first=contribution;
    group->last=contribution;
    return contribution;
}

void initialize_section_enumerator(SectionEnumerator *scan,SectionList *list)
{
    scan->next=next_section;
    scan->end=end_section_enumerator;
    scan->current=NULL;
    scan->first=list->first;
}
unsigned char next_section(SectionEnumerator *scan)
{
    scan->current=scan->current ? scan->current->next : scan->first;
    return scan->current!=NULL;
}
void initialize_library_enumerator(LibraryEnumerator *scan,Library *first)
{
    scan->next=next_library;
    scan->end=end_library_enumerator;
    scan->current=NULL;
    scan->first=first;
}
unsigned char next_library(LibraryEnumerator *scan)
{
    scan->current=scan->current ? scan->current->next : scan->first;
    return scan->current!=NULL;
}
void initialize_group_enumerator(GroupEnumerator *scan,Section *section)
{
    scan->next=next_group;
    scan->end=end_group_enumerator;
    scan->current=NULL;
    scan->section=section;
}
unsigned char next_group(GroupEnumerator *scan)
{
    scan->current=scan->current ? scan->current->next : scan->section->groups;
    return scan->current!=NULL;
}
void initialize_module_enumerator(ModuleEnumerator *scan,Library *library)
{
    scan->next=next_module;
    scan->end=end_module_enumerator;
    scan->current=NULL;
    scan->library=library;
}
unsigned char next_module(ModuleEnumerator *scan)
{
    if (scan->library)
        scan->current=scan->current ? scan->current->next : scan->library->modules;
    return scan->current!=NULL;
}

/* Original [0003af40,0003b0b0). Archive members display as library(member). */
char *format_module_name(Module *module,char *display)
{
    char filename[_MAX_FNAME],extension[_MAX_EXT];
    display[0]=0;
    if (module) {
        if (module->library->name) {
            _splitpath(module->library->name,NULL,NULL,filename,extension);
            strcat(display,filename);
            strcat(display,extension);
            strcat(display,"(");
        }
        _splitpath(module->path,NULL,NULL,filename,extension);
        strcat(display,filename);
        strcat(display,extension);
        if (module->library->name) strcat(display,")");
    }
    return display;
}

void end_module_enumerator(ModuleEnumerator *scan)
{
}

void end_section_enumerator(SectionEnumerator *scan)
{
}

void end_group_enumerator(GroupEnumerator *scan)
{
}

void initialize_contribution_enumerator(ContributionEnumerator *scan,Module *module)
{
    scan->next=next_contribution;
    scan->end=end_contribution_enumerator;
    scan->module=module;
    scan->current=NULL;
    scan->index=0;
}

unsigned char next_contribution(ContributionEnumerator *scan)
{
    while (scan->index<scan->module->section_count) {
        scan->current=&scan->module->contributions[scan->index++];
        if (scan->current) return 1;
    }
    scan->current=NULL;
    return 0;
}

Section *find_section_by_name(const char *name,SectionList *sections)
{
    SectionEnumerator scan;
    Section *section=NULL;
    initialize_section_enumerator(&scan,sections);
    while (next_section(&scan)) {
        if (!strncmp(scan.current->name,name,8)) {
            section=scan.current;
            end_section_enumerator(&scan);
            break;
        }
    }
    return resolve_section(section);
}

void sort_contributions_by_module(Group *group)
{
    GroupContributionEnumerator scan;
    Contribution **sorted;
    unsigned long index=0;
    sorted=malloc(group->contribution_count*sizeof(*sorted));
    if (!sorted) module_error(NULL,5,"");
    initialize_group_contribution_enumerator(&scan,group);
    while (next_group_contribution(&scan)) sorted[index++]=scan.current;
    qsort(sorted,group->contribution_count,sizeof(*sorted),compare_contribution_module_names);
    for (index=0;index<group->contribution_count-1;++index)
        sorted[index]->next=sorted[index+1];
    group->first=sorted[0];
    sorted[group->contribution_count-1]->next=NULL;
    free(sorted);
}

void initialize_group_contribution_enumerator(GroupContributionEnumerator *scan,Group *group)
{
    scan->next=next_group_contribution;
    scan->end=end_group_contribution_enumerator;
    scan->group=group;
    scan->current=NULL;
}

unsigned char next_group_contribution(GroupContributionEnumerator *scan)
{
    if (!scan->current) scan->current=scan->group->first;
    else scan->current=scan->current->next;
    return scan->current!=NULL;
}

int compare_contribution_module_names(const void *left,const void *right)
{
    const Contribution *a=*(const Contribution *const *)left;
    const Contribution *b=*(const Contribution *const *)right;
    return strcmp(a->module->path,b->module->path);
}

void move_contribution_to_module_tail(Contribution *contribution)
{
    Contribution **link=&contribution->group->first;
    Contribution *tail=contribution,*current=contribution;
    while (*link!=contribution) link=&(*link)->next;
    while (current && !strcmp(tail->module->path,current->module->path)) {
        tail=current;
        current=current->next;
    }
    if (tail!=contribution) {
        *link=contribution->next;
        contribution->next=tail->next;
        tail->next=contribution;
    }
}

void merge_sections(Section *source,Section *destination)
{
    Group **link;
    if (destination==source) return;
    link=&destination->groups;
    while (*link) link=&(*link)->next;
    *link=source->groups;
    while (*link) {
        (*link)->section=destination;
        link=&(*link)->next;
    }
    destination->merge_count+=source->merge_count;
    source->merge_count=0;
    source->groups=NULL;
    source->merge=destination;
}

void end_group_contribution_enumerator(GroupContributionEnumerator *scan)
{
}


void move_section_last(Section *section,SectionList *sections)
{
    Section **link;
    if (sections->tail==&section->next) return;
    link=&sections->first;
    while (*link!=section) link=&(*link)->next;
    *link=section->next;
    *sections->tail=section;
    sections->tail=&section->next;
    section->next=NULL;
}


void sort_sections_by_rva(SectionList *sections)
{
    SectionEnumerator scan;
    Section **sorted;
    unsigned short index=0;
    sorted=malloc((unsigned long)section_count*sizeof(*sorted));
    if (!sorted) module_error(NULL,5,"");
    initialize_section_enumerator(&scan,sections);
    while (next_section(&scan)) sorted[index++]=scan.current;
    qsort(sorted,section_count,sizeof(*sorted),compare_sections_by_rva);
    sections->tail=&sections->first;
    for (index=0;index<section_count;++index) {
        *sections->tail=sorted[index];
        sections->tail=&(*sections->tail)->next;
    }
    *sections->tail=NULL;
    free(sorted);
}


int compare_sections_by_rva(const void *left,const void *right)
{
    Section *a=*(Section *const *)left,*b=*(Section *const *)right;
    if (a->rva>b->rva) return 1;
    return a->rva<b->rva ? -1 : 0;
}


Section *find_section_for_group(const char *name,SectionList *sections)
{
    char *section_name,*group_name;
    split_section_group(name,&section_name,&group_name);
    return find_section_by_name(section_name,sections);
}


void end_library_enumerator(LibraryEnumerator *scan)
{
}


/* Original [0001a050,0001a0d0). Size the CodeView module-information array. */
unsigned long count_modules(Library *libraries)
{
    LibraryEnumerator scan;
    ModuleEnumerator modules;
    unsigned long count=0;
    initialize_library_enumerator(&scan,libraries);
    while (next_library(&scan)) {
        initialize_module_enumerator(&modules,scan.current);
        while (next_module(&modules)) ++count;
    }
    return count;
}

/* Original [0003b3e0,0003b4b0). Release archive indexes and library nodes. */
void release_library_list(LibraryList *libraries)
{
    LibraryEnumerator scan;
    Library *previous=NULL,*library;
    initialize_library_enumerator(&scan,libraries->first);
    while (next_library(&scan)) {
        library=scan.current;
        if (previous) free(previous);
        if (library->member_offsets) free(library->member_offsets);
        if (library->symbol_indices) free(library->symbol_indices);
        if (library->symbol_name_buffer) free(library->symbol_name_buffer);
        if (library->symbol_names) free(library->symbol_names);
        if (library->long_names) free(library->long_names);
        previous=library;
    }
    if (previous) free(previous);
    libraries->first=NULL;
    libraries->tail=&libraries->first;
    libraries->exclude_defaults=0;
    libraries->default_libraries=NULL;
}

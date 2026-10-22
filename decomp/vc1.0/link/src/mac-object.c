/* Macintosh per-input symbol references and section associations. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "link-pass.h"
#include "options.h"
#include "mac.h"
#include <recon.h>

MacSymbolReference *mac_symbol_references;
extern SymbolTable *mac_symbol_table;
extern void **mac_object_symbols;
extern MacFixupList mac_data_references;
MacFixupList mac_static_code_references;
extern unsigned char mac_loader_present,mac_image;

typedef struct MacJumpEntry {
    unsigned long offset;
    External *external;
} MacJumpEntry;
typedef struct MacJumpList {
    MacJumpEntry *entries;
    unsigned short capacity,count;
    unsigned long offset;
} MacJumpList;
MacJumpList *mac_near_jump_lists,*mac_far_jump_lists;
unsigned long mac_jump_table_size;
unsigned long mac_jump_table_cursor;
extern unsigned short mac_entry_section;
External *resolve_mac_external(External *);
void append_mac_jump_entry(External *,unsigned short,unsigned long);
void finalize_mac_jump_list(unsigned short,MacJumpList *);
int compare_mac_jump_entries(const void *,const void *);
void write_mac_jump_table(void);
void write_mac_far_jump(unsigned short,unsigned long);
void write_mac_near_jump(unsigned short,unsigned long);
extern Contribution *mac_jump_table;
extern unsigned long mac_data_size;
/* Read by the original jump-table header; no writer found in this image. */
unsigned char mac_jump_extra_offsets;

/* Original [00039a30,00039bb4). */
void finalize_mac_fixups(void)
{
    MacFixupList *block=&mac_static_code_references;
    MacRelocationRef *reference=block->records;
    External **entries,*external;
    Contribution *contribution;
    Section *section;
    unsigned short index;
    unsigned long in_block=0,count,offset;
    for (index=0;index<mac_static_code_references.count;++index) {
        if (in_block==512) {
            free(block->records);
            block=block->next;
            in_block=0;
            reference=block->records;
        }
        contribution=reference->contribution;
        section=contribution->group->section;
        offset=reference->offset-section->rva+contribution->rva;
        append_mac_jump_entry(NULL,section->index,offset);
        ++reference;
        ++in_block;
    }
    entries=get_externals_by_name(mac_symbol_table);
    count=count_externals(mac_symbol_table);
    for (index=0;index<count;++index) {
        external=entries[index];
        if (!(external->flags&0x40000000)) continue;
        if (!(external->owner.contribution->flags&0x20)) continue;
        if (external->symbol.Type&0x8000) external=resolve_mac_external(external);
        contribution=external->owner.contribution;
        section=contribution->group->section;
        offset=contribution->rva-section->rva+external->symbol.Value;
        append_mac_jump_entry(external,section->index,offset);
    }
    mac_jump_table_cursor=mac_loader_present?48:40;
    finalize_mac_jump_list(mac_entry_section,mac_near_jump_lists);
    for (index=1;index<=mac_code_section_count;++index)
        finalize_mac_jump_list(index,mac_near_jump_lists);
    for (index=1;index<=mac_code_section_count;++index)
        finalize_mac_jump_list(index,mac_far_jump_lists);
    write_mac_jump_table();
}
External *resolve_mac_external(External *external) { __asm { int 3 } }
void append_mac_jump_entry(External *external,unsigned short section,unsigned long offset) { __asm { int 3 } }
/* Original [00037100,00037178). */
void finalize_mac_jump_list(unsigned short section,MacJumpList *lists)
{
    MacJumpList *list=&lists[section];
    unsigned long index,offset;
    if (!list->capacity) return;
    list->offset=mac_jump_table_cursor;
    mac_jump_table_cursor+=list->count*sizeof(MacJumpEntry);
    qsort(list->entries,list->count,sizeof(MacJumpEntry),compare_mac_jump_entries);
    offset=list->offset;
    for (index=0;index<list->count;++index,offset+=8)
        if (list->entries[index].external)
            list->entries[index].external->mac_jump_offset=offset;
}
int compare_mac_jump_entries(const void *left,const void *right) { __asm { int 3 } }
#define MAC_BIG_LONG(x) (((unsigned long)(x)>>24)|(((unsigned long)(x)&0xff0000)>>8)| \
    (((unsigned long)(x)&0xff00)<<8)|((unsigned long)(x)<<24))
#define MAC_BIG_SHORT(x) ((unsigned short)(((unsigned short)(x)>>8)|((unsigned short)(x)<<8)))

/* Original [00036300,00036742).  CODE resources have a four-byte header;
 * nonstartup resources with a loader use its forty-byte extended header. */
void write_mac_loader_prologue(Contribution *contribution)
{
    Section *section=contribution->group->section;
    unsigned short id=section->index,word;
    unsigned long value,offset,count;
    extern unsigned char mac_small_model;
    if (!mac_small_model) {
        if (!mac_loader_present) goto short_header;
        if (id==mac_entry_section &&
            (mac_near_jump_lists[id].count || mac_far_jump_lists[id].count))
            fatal_message(NULL,56,"");
    }
    if (!mac_loader_present || (id==mac_entry_section && section->mac_type==0x45444f43))
        goto short_header;
    seek_file(output_file,contribution->file_offset-40,0);
    word=0xffff; write_file(output_file,&word,2);
    word=0; write_file(output_file,&word,2);
    count=mac_small_model?0:mac_near_jump_lists[id].count;
    offset=count?mac_near_jump_lists[id].offset-32:0;
    value=MAC_BIG_LONG(offset); write_file(output_file,&value,4);
    value=MAC_BIG_LONG(count); write_file(output_file,&value,4);
    count=mac_small_model?0:mac_far_jump_lists[id].count;
    offset=count?mac_far_jump_lists[id].offset-32:0;
    value=MAC_BIG_LONG(offset); write_file(output_file,&value,4);
    value=MAC_BIG_LONG(count); write_file(output_file,&value,4);
    seek_file(output_file,4,1);
    value=0; write_file(output_file,&value,4);
    seek_file(output_file,4,1);
    value=0; write_file(output_file,&value,4);
    value=0; write_file(output_file,&value,4);
    return;
short_header:
    if (section->mac_type!=0x45444f43) return;
    seek_file(output_file,contribution->file_offset-4,0);
    if (id!=mac_entry_section) {
        offset=mac_near_jump_lists[id].count?
            (unsigned short)(mac_near_jump_lists[id].offset-32):0;
        word=MAC_BIG_SHORT(offset); write_file(output_file,&word,2);
        word=MAC_BIG_SHORT(mac_near_jump_lists[id].count); write_file(output_file,&word,2);
    } else if (mac_loader_present) {
        word=0; write_file(output_file,&word,2);
        word=0x100; write_file(output_file,&word,2);
    } else {
        word=0; write_file(output_file,&word,2);
        word=MAC_BIG_SHORT(mac_near_jump_lists[id].count+1); write_file(output_file,&word,2);
    }
}

/* Original [00036a00,00036f2c).  Emit the Mac loader's big-endian header
 * and jump instructions through the original file and entry routines. */
void write_mac_jump_table(void)
{
    Contribution *contribution=entry_symbol->owner.contribution;
    unsigned long value,length=mac_jump_table_size+32,offset=40,base;
    unsigned short word,section,index;
    unsigned char warned=0;
    if (mac_jump_extra_offsets) length+=mac_code_section_count*4;
    seek_file(output_file,mac_jump_table->file_offset,0);
    value=MAC_BIG_LONG(length); write_file(output_file,&value,4);
    value=MAC_BIG_LONG(mac_data_size); write_file(output_file,&value,4);
    value=MAC_BIG_LONG(mac_jump_table_size); write_file(output_file,&value,4);
    value=0x20000000; write_file(output_file,&value,4);
    base=contribution->group->section->groups->rva;
    if ((long)((unsigned short)entry_symbol->symbol.Value-base+contribution->rva)>=0x8000)
        fatal_message(contribution->module->path,4,"Entry out of range");
    word=MAC_BIG_SHORT(entry_symbol->symbol.Value-base+contribution->rva);
    write_file(output_file,&word,2);
    word=0x3c3f; write_file(output_file,&word,2);
    word=MAC_BIG_SHORT(((unsigned short *)&contribution->group->section->mac_resource_id)[1]);
    write_file(output_file,&word,2);
    word=0xf0a9; write_file(output_file,&word,2);
    if (mac_loader_present) {
        word=0; write_file(output_file,&word,2);
        word=0xffff; write_file(output_file,&word,2);
        value=0; write_file(output_file,&value,4);
        offset=48;
        for (section=1;section<=mac_code_section_count;++section)
            for (index=0;index<mac_near_jump_lists[section].count;++index) {
                write_mac_far_jump(mac_section_resources[section-1].id,
                    mac_near_jump_lists[section].entries[index].offset);
                if (offset>=0x8000 && !warned) {
                    fatal_message(NULL,57,"");
                    warned=1;
                }
                offset+=8;
            }
        for (section=1;section<=mac_code_section_count;++section)
            for (index=0;index<mac_far_jump_lists[section].count;++index) {
                offset+=8;
                write_mac_far_jump(mac_section_resources[section-1].id,
                    mac_far_jump_lists[section].entries[index].offset);
            }
        return;
    }
    section=mac_entry_section;
    for (index=0;index<mac_near_jump_lists[section].count;++index) {
        write_mac_near_jump(mac_section_resources[section-1].id,
            mac_near_jump_lists[section].entries[index].offset);
        if (offset>=0x8000 && !warned) {
            fatal_message(NULL,57,"");
            warned=1;
        }
        offset+=8;
    }
    for (section=1;section<=mac_code_section_count;++section) {
        if (section==mac_entry_section) continue;
        for (index=0;index<mac_near_jump_lists[section].count;++index) {
            write_mac_near_jump(mac_section_resources[section-1].id,
                mac_near_jump_lists[section].entries[index].offset);
            if (offset>=0x8000 && !warned) {
                fatal_message(NULL,57,"");
                warned=1;
            }
            offset+=8;
        }
    }
}
void write_mac_far_jump(unsigned short resource,unsigned long offset) { __asm { int 3 } }
void write_mac_near_jump(unsigned short resource,unsigned long offset) { __asm { int 3 } }

/* Original [00039930,00039a2b). */
unsigned long size_mac_jump_table(SymbolTable *table)
{
    External **entries,*external;
    Contribution *contribution;
    unsigned long count,thunks=0;
    mac_near_jump_lists=calloc(mac_code_section_count+1,sizeof(MacJumpList));
    mac_far_jump_lists=calloc(mac_code_section_count+1,sizeof(MacJumpList));
    entries=get_externals_by_name(table);
    count=count_externals(table);
    while (count--) {
        external=*entries++;
        contribution=external->owner.contribution;
        if (!contribution) continue;
        if (!(contribution->flags&0x20)) {
            external->flags&=~0x40000000;
            continue;
        }
        if (!(external->flags&0x40000000)) {
            if (!external->mac_reference_section ||
                contribution->group->section==external->mac_reference_section)
                continue;
            external->flags|=0x40000000;
        }
        ++thunks;
    }
    if (verbose_link && mac_image)
        printf("\n# of static functions with thunk: %d\n\n",mac_static_code_references.count);
    mac_jump_table_size=(mac_static_code_references.count+thunks)*8+
        (mac_loader_present?16:8);
    return mac_jump_table_size+16;
}

typedef struct MacDataReference {
    Contribution *contribution;
    unsigned long offset;
    union { unsigned long symbol_index; External *external; } target;
} MacDataReference;
typedef struct MacDataReferenceBlock {
    MacDataReference *records;
    unsigned long capacity,count;
    struct MacDataReferenceBlock *next;
} MacDataReferenceBlock;
MacDataReferenceBlock mac_data_reference_blocks;
MacDataReferenceBlock *mac_data_reference_read_block=&mac_data_reference_blocks;
MacDataReference *mac_data_reference_read_records;
unsigned long mac_data_reference_read_index,mac_data_reference_resolved;

/* Original [000373e0,0003747b).  This cursor persists across input modules;
 * the final record word changes from a local symbol index to an External. */
void process_mac_object_symbols(SymbolTable *table,PIMAGE_SYMBOL symbols,char *strings)
{
    MacDataReference *record;
    unsigned long count=mac_data_reference_resolved,index;
    unsigned char storage;
    while (count<mac_data_reference_blocks.count) {
        if (mac_data_reference_read_index==341) {
            mac_data_reference_read_block=mac_data_reference_read_block->next;
            mac_data_reference_read_index=0;
            mac_data_reference_read_records=mac_data_reference_read_block->records;
        }
        record=&mac_data_reference_read_block->records[mac_data_reference_read_index];
        index=record->target.symbol_index;
        storage=symbols[index].StorageClass;
        if (storage!=2 && storage!=0x44)
            record->target.external=NULL;
        else
            record->target.external=mac_object_symbols[index];
        ++count;
        ++mac_data_reference_read_index;
    }
    mac_data_reference_resolved=mac_data_reference_blocks.count;
}

typedef struct MacExternalReference {
    External *external;
    unsigned long flags;
    PIMAGE_RELOCATION relocations;
    struct MacExternalReference *next;
} MacExternalReference;
typedef unsigned char (*MacExternalCompare)(External *,External *);
MacExternalReference *mac_external_references,*mac_address_references;
/* Original [00039550,00039578). */
External *find_mac_contribution_external(Contribution *contribution)
{
    MacExternalReference *entry;
    for (entry=mac_address_references;entry;entry=entry->next)
        if (entry->external->owner.contribution==contribution)
            return entry->external;
    return NULL;
}
LibraryList mac_segment_libraries;
extern Library *mac_segment_library;
unsigned char compare_mac_external_contributions(External *,External *);
extern short mac_max_resource_id;
unsigned char compare_mac_external_names(External *,External *);
MacExternalReference *find_mac_external_reference(MacExternalReference **,
    External *,unsigned long,MacExternalCompare,unsigned char *);
void record_mac_duplicate_fixups(Contribution *,MacExternalReference *);

/* Original [00039770,0003986a). */
void initialize_mac_code_section(Section *section,Image *image)
{
    MacExternalReference *entry;
    Contribution *source,*copy;
    Module *module,*copy_module;
    unsigned long archive;
    char *name;
    for (entry=mac_address_references;entry;entry=entry->next) {
        source=entry->external->owner.contribution;
        if (entry->external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)entry->external,8);
        else name=mac_symbol_table->strings.data+entry->external->symbol.N.Name.Long;
        copy_module=find_module(mac_segment_library,name,0);
        module=source->module;
        archive=module->library->name?module->name.member_offset:0;
        copy=create_contribution(section->name,source->raw_size,source->relocation_count,
            source->line_count,source->relocation_offset+archive,source->line_offset+archive,
            source->data_offset+archive,source->flags&~0x800,section->characteristics,
            source->input_rva,copy_module,0,&image->sections,&image->optional);
        if (copy->relocation_count) record_mac_duplicate_fixups(copy,entry);
    }
}
void record_mac_duplicate_fixups(Contribution *contribution,MacExternalReference *entry) { __asm { int 3 } }

/* Original [00039580,00039768).  The caller's byte argument is unused.
 * Address-referenced contributions become per-CODE synthetic modules. */
void set_mac_export_mode(unsigned char mode)
{
    MacExternalReference *source=mac_address_references,*entry;
    Contribution *contribution;
    Module *module;
    unsigned char created;
    char *path,*name;
    if (!source) return;
    mac_segment_libraries.first=NULL;
    mac_segment_libraries.tail=&mac_segment_libraries.first;
    mac_segment_libraries.exclude_defaults=0;
    mac_segment_libraries.default_libraries=NULL;
    mac_segment_library=create_library(NULL,0,0,&mac_segment_libraries);
    mac_address_references=NULL;
    for (;source;source=source->next) {
        entry=find_mac_external_reference(&mac_address_references,source->external,
            0,compare_mac_external_contributions,&created);
        if (!created)
            fatal_message(program_name,4,"Multiple symbols in same con refd by dupcon fixup");
        contribution=entry->external->owner.contribution;
        module=contribution->module;
        if (contribution->relocation_count) {
            entry->relocations=malloc(contribution->relocation_count*sizeof(IMAGE_RELOCATION));
            path=module->library->name;
            if (!path) path=module->name.display_name;
            input_file=open_file(path,0x8000,0);
            seek_file(input_file,contribution->relocation_offset+
                (module->library->name?module->name.member_offset:0),0);
            read_coff_relocations(input_file,entry->relocations,contribution->relocation_count);
            close_file(input_file,(unsigned char)(module->library->name!=NULL));
        }
        contribution->flags|=0x800;
        if (source->external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)source->external,8);
        else name=mac_symbol_table->strings.data+source->external->symbol.N.Name.Long;
        if (source->external->symbol.N.Name.Short)
            path=strncpy(short_symbol_name,(char *)source->external,8);
        else path=mac_symbol_table->strings.data+source->external->symbol.N.Name.Long;
        create_module(path,name,module->name.member_offset,(unsigned short)module->machine,
            module->symbol_offset,module->symbol_count,module->optional_header_size,
            module->characteristics,mac_code_section_count,mac_segment_library,NULL);
    }
}
unsigned char compare_mac_external_contributions(External *left,External *right) { __asm { int 3 } }

/* Original [00039460,00039549).  Reference kinds1/2/4 reserve one, two,
 * or four bytes per resource; other values retain the prior WORD size. */
void initialize_mac_exports(void)
{
    MacExternalReference *entry;
    External *external;
    Contribution *contribution;
    unsigned short size;
    for (entry=mac_external_references;entry;entry=entry->next) {
        switch (entry->flags) {
        case 0x01000000: size=mac_max_resource_id; break;
        case 0x02000000: size=(unsigned short)(mac_max_resource_id*2); break;
        case 0x04000000: size=(unsigned short)(mac_max_resource_id*4); break;
        }
        external=entry->external;
        if (external->flags&0x80000000) {
            external->owner.contribution=bss_contribution;
            bss_size+=size;
            contribution=bss_contribution;
        } else {
            external->symbol.StorageClass=0x44;
            external->owner.contribution=mac_far_bss;
            mac_far_bss_offset+=size;
            contribution=mac_far_bss;
        }
        contribution->raw_size+=size;
        external->symbol.Value=size;
        external->flags|=2;
    }
}

/* Original [00039220,0003933a). */
void record_mac_external(External *external,unsigned long index)
{
    MacSymbolReference *reference=&mac_symbol_references[index];
    MacExternalReference *entry;
    unsigned long flags;
    unsigned char created;
    char *name;

    if (reference->flags&0x0f000000) {
        if (external->symbol.N.Name.Short)
            name=strncpy(short_symbol_name,(char *)external,8);
        else
            name=mac_symbol_table->strings.data+external->symbol.N.Name.Long;
        flags=reference->flags&0x0f000000;
        set_external_defined(external,1,mac_symbol_table);
        entry=find_mac_external_reference(&mac_external_references,external,flags,
            compare_mac_external_names,&created);
        if (!created && entry->flags!=flags) {
            warning_message(program_name,0x86,name,"");
            entry->flags=entry->flags>flags?entry->flags:flags;
        }
    }
    if (reference->flags&0x00800000)
        find_mac_external_reference(&mac_address_references,external,0,
            compare_mac_external_names,&created);
    external->flags|=reference->flags;
    if (!(external->flags&0x40000000)) {
        if (!external->mac_reference_section) {
            external->mac_reference_section=reference->section;
            return;
        }
        if (reference->section && reference->section!=external->mac_reference_section)
            external->flags|=0x40000000;
    }
}

unsigned char compare_mac_external_names(External *left,External *right) { __asm { int 3 } }
MacExternalReference *find_mac_external_reference(MacExternalReference **list,
    External *external,unsigned long flags,MacExternalCompare compare,unsigned char *created)
{ __asm { int 3 } }

/* Original [00039110,00039125), cdecl, no allocation failure branch. */
void initialize_mac_object_symbols(unsigned long count)
{
    mac_symbol_references=calloc(count,sizeof(*mac_symbol_references));
}

/* Original [00039920,0003992f); deliberately leaves the pointer unchanged. */
void finish_mac_object_symbols(void)
{
    free(mac_symbol_references);
}

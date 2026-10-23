/* Macintosh per-input symbol references and section associations. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "link-pass.h"
#include "options.h"
#include "mac.h"
#include <recon.h>

#pragma intrinsic(strlen,strcpy,strcat)

MacSymbolReference *mac_symbol_references;

/* Original [00039130,00039174). */
void record_mac_symbol_reference(unsigned long symbol,Section *section,unsigned long flags)
{
    MacSymbolReference *reference=&mac_symbol_references[symbol];
    if (!reference->section) {
        reference->section=section;
        reference->flags|=flags;
        return;
    }
    if (reference->section!=section && section) flags|=0x40000000;
    reference->flags|=flags;
}
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
int compare_mac_jump_key(const void *,const void *);
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
/* Original [00038e50,00038f61), the old native-entry __nep name convention. */
External *resolve_mac_external(External *external)
{
    char buffer[128],*derived=buffer,*name;
    unsigned short capacity;
    External *result;
    if (!external->symbol.N.Name.Short)
        name=mac_symbol_table->strings.data+external->symbol.N.Name.Long;
    else name=strncpy(short_symbol_name,(char *)external,8);
    capacity=(unsigned short)(strlen(name)+6);
    if (capacity>sizeof(buffer)) derived=malloc(capacity);
    strcpy(derived,"__nep");
    strcat(derived,name);
    result=find_external(mac_symbol_table,(unsigned char)(strlen(derived)<9),derived);
    if (capacity>sizeof(buffer)) free(derived);
    if (!result) fatal_message(name,4,"Pcode symbol has no native symbol");
    return result;
}
/* Original [00036f30,0003706b). */
void append_mac_jump_entry(External *external,unsigned short section,unsigned long offset)
{
    MacJumpList *list;
    if (external) {
        if (!(external->flags&0x80000000) && mac_loader_present)
            list=&mac_far_jump_lists[section];
        else list=&mac_near_jump_lists[section];
    } else if (!(offset&0x80000000) && mac_loader_present) {
        list=&mac_far_jump_lists[section];
    } else {
        offset&=0x7fffffff;
        list=&mac_near_jump_lists[section];
    }
    if (!mac_loader_present) offset-=4;
    if (!list->capacity) {
        list->capacity=10;
        list->entries=malloc(80);
        if (!list->entries) fatal_message(NULL,5,"");
        list->count=0;
    } else if (list->count==list->capacity) {
        list->capacity+=10;
        list->entries=realloc(list->entries,list->capacity*sizeof(MacJumpEntry));
        if (!list->entries) fatal_message(NULL,5,"");
    }
    list->entries[list->count].external=external;
    list->entries[list->count].offset=offset;
    ++list->count;
}
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
/* Original [00037070,00037093), ascending unsigned jump offsets. */
int compare_mac_jump_entries(const void *left,const void *right)
{
    unsigned long first=((const MacJumpEntry *)left)->offset;
    unsigned long second=((const MacJumpEntry *)right)->offset;
    if (first>second) return 1;
    if (first<second) return -1;
    return 0;
}
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
typedef struct MacFarJumpRecord {
    unsigned short resource,opcode;
    unsigned long offset;
} MacFarJumpRecord;
MacFarJumpRecord mac_far_jump_record={0,0xf0a9,0};

/* Original [00036970,000369ba), mutable eight-byte loader jump record. */
void write_mac_far_jump(unsigned short resource,unsigned long offset)
{
    mac_far_jump_record.resource=MAC_BIG_SHORT(resource);
    mac_far_jump_record.offset=MAC_BIG_LONG(offset);
    write_file(output_file,&mac_far_jump_record,sizeof(mac_far_jump_record));
}
typedef struct MacNearJumpRecord {
    unsigned short offset,push,resource,trap;
} MacNearJumpRecord;
MacNearJumpRecord mac_near_jump_record={0,0x3c3f,0,0xf0a9};

/* Original [000369c0,000369f6), mutable eight-byte near jump record. */
void write_mac_near_jump(unsigned short resource,unsigned long offset)
{
    mac_near_jump_record.offset=MAC_BIG_SHORT(offset);
    mac_near_jump_record.resource=MAC_BIG_SHORT(resource);
    write_file(output_file,&mac_near_jump_record,sizeof(mac_near_jump_record));
}

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
MacDataReferenceBlock *mac_data_reference_write_block=&mac_data_reference_blocks;
MacDataReference *mac_data_reference_write_record;
unsigned long mac_data_reference_write_index;

/* Original [000372a0,000373d3).  Blocks hold341 twelve-byte records in
 * the original4096-byte allocation. Only the head tracks total capacity. */
void record_mac_data_reference(Contribution *contribution,unsigned long offset,unsigned long symbol)
{
    if (!mac_data_reference_blocks.capacity) {
        mac_data_reference_write_block->records=malloc(4096);
        if (!mac_data_reference_write_block->records) fatal_message(NULL,5,"");
        mac_data_reference_write_block->capacity=341;
        mac_data_reference_write_block->count=0;
        goto new_block;
    }
    if (mac_data_reference_blocks.count==mac_data_reference_blocks.capacity) {
        mac_data_reference_blocks.capacity+=341;
        mac_data_reference_write_block->next=malloc(sizeof(MacDataReferenceBlock));
        if (!mac_data_reference_write_block->next) fatal_message(NULL,5,"");
        mac_data_reference_write_block=mac_data_reference_write_block->next;
        mac_data_reference_write_block->records=malloc(4096);
        if (!mac_data_reference_write_block->records) fatal_message(NULL,5,"");
new_block:
        mac_data_reference_write_block->next=NULL;
        mac_data_reference_write_index=0;
        mac_data_reference_write_record=mac_data_reference_write_block->records;
    }
    mac_data_reference_write_record->offset=offset;
    mac_data_reference_write_record->contribution=contribution;
    mac_data_reference_write_record->target.symbol_index=symbol;
    ++mac_data_reference_blocks.count;
    ++mac_data_reference_write_record;
}

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

/* Original [00037480,0003754f).  Pending code-pointer references choose
 * the source resource's data or code fixup list by destination group. */
void initialize_mac_loader(void)
{
    MacDataReferenceBlock *block=&mac_data_reference_blocks;
    MacDataReference *reference=block->records;
    MacFixupList *list;
    Contribution *contribution;
    unsigned long index,in_block=0;
    unsigned short resource;
    if (mac_code_section_count>999) fatal_message(NULL,4,"");
    for (index=0;index<mac_data_reference_blocks.count;++index) {
        if (in_block==341) {
            free(block->records);
            block=block->next;
            in_block=0;
            reference=block->records;
        }
        contribution=reference->contribution;
        resource=(unsigned short)contribution->group->section->mac_resource_id;
        if (reference->target.external &&
            reference->target.external->owner.contribution->group!=contribution->group)
            list=&mac_section_data_fixups[resource];
        else
            list=&mac_section_code_fixups[resource];
        append_mac_fixup(list,contribution,reference->offset);
        ++reference;
        ++in_block;
    }
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
/* Original [00039870,0003991f), account for each per-resource code copy. */
void record_mac_duplicate_fixups(Contribution *contribution,MacExternalReference *entry)
{
    unsigned long count=entry->external->owner.contribution->relocation_count;
    PIMAGE_RELOCATION relocation=entry->relocations;
    unsigned short type;
    Section *section=contribution->group->section;
    if (mac_loader_present && entry_symbol->owner.contribution->group->section==section)
        return;
    while (count--) {
        type=relocation->Type;
        if (type==10 || type==12 || (type>=38 && type<=40))
            append_mac_fixup(&mac_section_data_fixups[(unsigned short)section->mac_resource_id],
                contribution,relocation->VirtualAddress-contribution->input_rva);
        else if (type==11)
            fatal_message(program_name,4,"Illegal CTOABSC32 reloc in dupcon");
        ++relocation;
    }
}

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
/* Original [00039200,00039216), byte predicate: distinct source contribution. */
unsigned char compare_mac_external_contributions(External *left,External *right)
{
    return left->owner.contribution!=right->owner.contribution;
}

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

/* Original [00039180,000391f9), unsigned string ordering narrowed to AL.
 * Both short names intentionally share the original scratch buffer. */
unsigned char compare_mac_external_names(External *left,External *right)
{
    const unsigned char *first,*second;
    if (left->symbol.N.Name.Short)
        first=(unsigned char *)strncpy(short_symbol_name,(const char *)left,8);
    else first=(unsigned char *)mac_symbol_table->strings.data+left->symbol.N.Name.Long;
    if (right->symbol.N.Name.Short)
        second=(unsigned char *)strncpy(short_symbol_name,(const char *)right,8);
    else second=(unsigned char *)mac_symbol_table->strings.data+right->symbol.N.Name.Long;
    while (*first==*second) {
        if (!*first) return 0;
        ++first;++second;
    }
    return (unsigned char)(*first<*second?-1:1);
}
/* Original [000393b0,00039451), insertion-ordered reference list. */
MacExternalReference *find_mac_external_reference(MacExternalReference **list,
    External *external,unsigned long flags,MacExternalCompare compare,unsigned char *created)
{
    MacExternalReference *entry=*list,*last;
    if (external->symbol.N.Name.Short) strncpy(short_symbol_name,(char *)external,8);
    while (entry && compare(external,entry->external)) {
        last=entry;
        entry=entry->next;
    }
    if (entry) { *created=0; return entry; }
    entry=malloc(sizeof(MacExternalReference));
    if (!*list) *list=entry;
    else last->next=entry;
    entry->external=external;
    entry->relocations=NULL;
    entry->flags=flags;
    entry->next=NULL;
    *created=1;
    return entry;
}

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

/* Original [00039340,000393aa). A static P-code symbol uses the previous
   COFF record's Value slot when recording its native jump reference. */
void process_mac_static_symbol(Contribution *contribution,PIMAGE_SYMBOL symbol,unsigned long index)
{
    MacSymbolReference *reference=&mac_symbol_references[index];
    unsigned long offset=symbol->Value;
    if ((reference->flags&0x40000000 ||
        (reference->section && contribution->group->section!=reference->section)) &&
        (contribution->flags&0x20)) {
        if (symbol->Type&0x8000) offset=*(unsigned long *)((char *)symbol-10);
        if (reference->flags&0x80000000) offset|=0x80000000;
        append_mac_fixup(&mac_static_code_references,contribution,offset);
    }
}

/* Original [00037da0,00037e79). Wide references may reuse a near entry. */
unsigned long find_mac_static_jump(unsigned short section,PIMAGE_SYMBOL symbol,unsigned char wide)
{
    MacJumpList *list=wide?&mac_far_jump_lists[section]:&mac_near_jump_lists[section];
    MacJumpEntry *entry;
    unsigned long offset=symbol->Value;
    if (!mac_loader_present || section==mac_entry_section) offset-=4;
    entry=bsearch(&offset,list->entries,list->count,sizeof(MacJumpEntry),compare_mac_jump_key);
    if (!entry && wide) {
        list=&mac_near_jump_lists[section];
        entry=bsearch(&offset,list->entries,list->count,sizeof(MacJumpEntry),compare_mac_jump_key);
    }
    return list->offset+2+(((unsigned long)entry-(unsigned long)list->entries)&~7UL);
}

/* Separate native bsearch callback despite identical qsort comparator bytes. */
/* Original [000370d0,000370f3), distinct from qsort callback37070. */
int compare_mac_jump_key(const void *left,const void *right)
{
    unsigned long a=*(const unsigned long *)left,b=*(const unsigned long *)right;
    return a<b?-1:(a>b?1:0);
}

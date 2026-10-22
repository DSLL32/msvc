/* NB05 CodeView module, public, library and segment-map subsections. */
#include "codeview.h"
#pragma intrinsic(strlen,strcmp)
extern ModuleDebugInfo *module_debug_info;
extern unsigned long output_module_index,codeview_start,codeview_end;
const char *codeview_module_name;
typedef char cv_module_header_layout[(sizeof(CvModuleHeader)==8)?1:-1];
typedef char cv_segment_layout[(sizeof(CvSegment)==12)?1:-1];
typedef char cv_segment_descriptor_layout[(sizeof(CvSegmentDescriptor)==20)?1:-1];
typedef char cv_directory_header_layout[(sizeof(CvDirectoryHeader)==16)?1:-1];
typedef char cv_directory_entry_layout[(sizeof(CvDirectoryEntry)==12)?1:-1];

/* Original [00034850,000351f0). NB05 CodeView subsections and directory. */
void emit_codeview(Image *image)
{
    CvModuleHeader module_header;
    CvSegment segment;
    CvSegmentDescriptor descriptor;
    CvDirectoryHeader directory;
    CvDirectoryEntry entry;
    CvModuleSegment *segments,*next;
    ModuleDebugInfo *info,*other;
    LibraryEnumerator libraries;
    SectionEnumerator sections;
    Section *section;
    unsigned long index,other_index,symbol_sections=0,type_sections=0,line_sections=0;
    unsigned long public_sections=0,signature,library_start,library_end,segment_start,segment_end;
    unsigned short library_index,segment_index,count;
    unsigned char name_length;
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        if (info->symbols.offset) ++symbol_sections;
        if (info->types.offset) ++type_sections;
        if (info->lines.offset) ++line_sections;
    }
    module_header.overlay=0;
    module_header.style=0x5643;
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        segments=collect_module_segments(info->module,&module_header.segment_count,image);
        name_length=(unsigned char)strlen(info->module_name);
        if (!info->library_name) module_header.library=0;
        else {
            library_index=0;
            initialize_library_enumerator(&libraries,image->libraries.first);
            while (next_library(&libraries)) {
                if (libraries.current->name && !strcmp(info->library_name,libraries.current->name)) {
                    module_header.library=library_index+1;
                    end_library_enumerator(&libraries);
                    break;
                }
                ++library_index;
            }
        }
        info->module_record.offset=tell_file(output_file);
        write_file(output_file,&module_header,sizeof(module_header));
        for (segment_index=0;segment_index<module_header.segment_count;++segment_index) {
            section=segments->first->group->section;
            segment.section=section->index;
            segment.flags=0;
            segment.offset=segments->first->rva-section->rva;
            segment.size=segments->last->rva-segments->first->rva+segments->last->raw_size;
            write_file(output_file,&segment,sizeof(segment));
            next=segments->next;
            free(segments);
            segments=next;
        }
        write_file(output_file,&name_length,1);
        write_file(output_file,info->module_name,name_length);
        codeview_module_name=info->module_name;
        info->module_record.size=tell_file(output_file)-info->module_record.offset;
    }
    collect_module_publics(image);
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        if (info->publics.offset==0xffffffffUL) {
            info->publics.offset=0;
            continue;
        }
        ++public_sections;
        info->publics.offset=tell_file(output_file);
        signature=1;
        write_file(output_file,&signature,4);
        emit_module_publics(image,info);
        if (info->module->library->name) {
            for (other_index=index+1;other_index<output_module_index;++other_index) {
                other=&module_debug_info[other_index];
                if (other->module->library==info->module->library &&
                    !strcmp(info->module_name,other->module_name)) {
                    emit_module_publics(image,other);
                    other->publics.offset=0xffffffffUL;
                }
            }
        }
        info->publics.size=tell_file(output_file)-info->publics.offset;
    }
    library_start=tell_file(output_file);
    signature=0;
    write_file(output_file,&signature,1);
    initialize_library_enumerator(&libraries,image->libraries.first);
    while (next_library(&libraries)) {
        if (libraries.current->name) {
            signature=(unsigned char)strlen(libraries.current->name);
            write_file(output_file,&signature,1);
            write_file(output_file,libraries.current->name,signature);
        }
    }
    library_end=tell_file(output_file);
    segment_start=tell_file(output_file);
    count=image->file.NumberOfSections+1;
    write_file(output_file,&count,2);
    write_file(output_file,&count,2);
    descriptor.overlay=descriptor.group=0;
    for (segment_index=1;segment_index<=image->file.NumberOfSections;++segment_index) {
        initialize_section_enumerator(&sections,&image->sections);
        while (next_section(&sections)) {
            section=sections.current;
            if (section->index==segment_index) {
                end_section_enumerator(&sections);
                break;
            }
        }
        descriptor.flags=(unsigned short)((section->characteristics&IMAGE_SCN_MEM_READ)?0x109:0x108);
        if (section->characteristics&IMAGE_SCN_MEM_WRITE) descriptor.flags|=2;
        if (section->characteristics&IMAGE_SCN_MEM_EXECUTE) descriptor.flags|=4;
        descriptor.frame=segment_index;
        descriptor.segment_name=descriptor.class_name=0xffff;
        descriptor.offset=0;
        descriptor.length=section->raw_size;
        write_file(output_file,&descriptor,sizeof(descriptor));
    }
    descriptor.flags=0x208;
    descriptor.frame=0;
    descriptor.segment_name=descriptor.class_name=0xffff;
    descriptor.length=0xffffffffUL;
    write_file(output_file,&descriptor,sizeof(descriptor));
    segment_end=tell_file(output_file);
    codeview_end=tell_file(output_file);
    directory.header_size=16;
    directory.entry_size=12;
    directory.count=public_sections+symbol_sections+type_sections+line_sections+output_module_index+2;
    directory.next=directory.flags=0;
    write_file(output_file,&directory,sizeof(directory));
    entry.kind=0x120;
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        entry.module=(unsigned short)(index+1);
        entry.offset=info->module_record.offset-codeview_start;
        entry.size=info->module_record.size;
        write_file(output_file,&entry,sizeof(entry));
    }
    entry.kind=0x123;
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        if (!info->publics.offset) continue;
        entry.module=(unsigned short)(index+1);
        entry.offset=info->publics.offset-codeview_start;
        entry.size=info->publics.size;
        write_file(output_file,&entry,sizeof(entry));
    }
    entry.kind=0x124;
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        if (!info->symbols.offset) continue;
        entry.module=(unsigned short)(index+1);
        entry.offset=info->symbols.offset-codeview_start;
        entry.size=info->symbols.size;
        write_file(output_file,&entry,sizeof(entry));
    }
    for (index=0;index<output_module_index;++index) {
        info=&module_debug_info[index];
        if (!info->types.offset) continue;
        entry.kind=(unsigned short)(info->types.flags?0x12f:0x121);
        entry.module=(unsigned short)(index+1);
        entry.offset=info->types.offset-codeview_start;
        entry.size=info->types.size;
        write_file(output_file,&entry,sizeof(entry));
    }
    if ((image->options.debug_types&2) && (image->options.debug_types&4)) {
        entry.kind=0x126;
        for (index=0;index<output_module_index;++index) {
            info=&module_debug_info[index];
            if (!info->lines.offset) continue;
            entry.module=(unsigned short)(index+1);
            entry.offset=info->lines.offset-codeview_start;
            entry.size=info->lines.size;
            write_file(output_file,&entry,sizeof(entry));
        }
    }
    entry.kind=0x128;
    entry.module=0xffff;
    entry.offset=library_start-codeview_start;
    entry.size=library_end-library_start;
    write_file(output_file,&entry,sizeof(entry));
    entry.kind=0x12d;
    entry.module=0xffff;
    entry.offset=segment_start-codeview_start;
    entry.size=segment_end-segment_start;
    write_file(output_file,&entry,sizeof(entry));
}

/* Original [00035400,00035520). Collect contiguous code ranges for a module. */
CvModuleSegment *collect_module_segments(Module *module,unsigned short *count,Image *image)
{
    ContributionEnumerator contributions;
    Contribution *contribution;
    CvModuleSegment *head=NULL,*segment;
    *count=0;
    initialize_contribution_enumerator(&contributions,module);
    while (next_contribution(&contributions)) {
        contribution=contributions.current;
        if (!contribution->raw_size ||
            (contribution->group->section->characteristics&0x1e0)!=0x20)
            continue;
        if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
            continue;
        for (segment=head;segment;segment=segment->next) {
            if (segment->group!=contribution->group) continue;
            if (segment->last->next==contribution) {
                segment->last=contribution;
                break;
            }
            if (contribution->next==segment->first) {
                segment->first=contribution;
                break;
            }
        }
        if (segment) continue;
        segment=malloc(sizeof(*segment));
        if (!segment) out_of_memory();
        segment->group=contribution->group;
        segment->first=segment->last=contribution;
        segment->next=head;
        head=segment;
        ++*count;
    }
    return head;
}

/* Original [000351f0,00035270). Build per-module public-symbol lists. */
void collect_module_publics(Image *image)
{
    External **ordered=get_externals_by_name(image->symbols);
    unsigned long count=count_externals(image->symbols),index;
    External *external;
    Contribution *contribution;
    for (index=0;index<count;++index) {
        external=ordered[index];
        contribution=external->owner.contribution;
        if (!(external->flags&EXTERNAL_DEFINED) || !contribution) continue;
        if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
            continue;
        if (contribution->module)
            prepend_external(&contribution->module->public_symbols,external);
    }
}

/* Original [00035270,00035310). Serialize and release a module's public lists. */
void emit_module_publics(Image *image,ModuleDebugInfo *info)
{
    ExternalLink *node,*next;
    unsigned short absolute_section=(unsigned short)(image->file.NumberOfSections+1);
    for (node=info->module->public_symbols;node;node=next) {
        emit_cv_public(image->symbols,node->external,absolute_section);
        next=node->next;
        free(node);
    }
    while (info->module->common_symbols) {
        node=info->module->common_symbols;
        if ((node->external->flags&3)==3)
            emit_cv_public(image->symbols,node->external,
                (unsigned short)(image->file.NumberOfSections+1));
        info->module->common_symbols=node->next;
        free(node);
    }
}

typedef struct CvPublicRecord {
    unsigned short length,kind;
    unsigned long offset;
    unsigned short section,type;
} CvPublicRecord;
typedef char cv_public_record_layout[(sizeof(CvPublicRecord)==12)?1:-1];
extern unsigned char mac_image;

/* Original [00035310,00035400). Emit one NB05 public record and counted name. */
void emit_cv_public(SymbolTable *table,External *external,unsigned short absolute_section)
{
    const char *name;
    unsigned long length;
    CvPublicRecord record;
    Section *section;
    if (external->symbol.N.Name.Short)
        name=strncpy(short_symbol_name,(char *)&external->symbol,8);
    else name=table->strings.data+external->symbol.N.Name.Long;
    fflush(stdout);
    length=strlen(name);
    record.length=(unsigned short)(length+11);
    record.kind=0x203;
    section=external->owner.contribution->group->section;
    record.offset=mac_image?external->value.final_value:
        external->value.final_value-section->rva;
    record.section=external->owner.contribution?section->index:absolute_section;
    record.type=0;
    write_file(output_file,&record,sizeof(record));
    write_file(output_file,&length,1);
    write_file(output_file,name,length);
}

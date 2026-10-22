/* Count and lay out output sections and their live contributions. */
#include "link-pass.h"

unsigned short count_output_sections(Image *image)
{
    SectionEnumerator sections;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    Section *section;
    unsigned short count=0;
    unsigned char empty;
    initialize_section_enumerator(&sections,&image->sections);
    while (next_section(&sections)) {
        section=sections.current;
        if (image->options.rom &&
            (!strncmp(".pdata",section->name,8) ||
             !strncmp(reloc_section_defaults.name,section->name,8))) continue;
        if (section->characteristics&0x800) continue;
        if (section->raw_size) { ++count; continue; }
        empty=0;
        initialize_group_enumerator(&groups,section);
        while (next_group(&groups)) {
            initialize_group_contribution_enumerator(&contributions,groups.current);
            while (next_group_contribution(&contributions)) {
                empty=contributions.current->raw_size==0;
                if (empty) continue;
                if (_strnicmp(debug_section_defaults.name,section->name,8)) {
                    ++count;
                    end_group_contribution_enumerator(&contributions);
                    break;
                }
            }
            if (!empty) {
                end_group_enumerator(&groups);
                break;
            }
        }
    }
    return count;
}


extern unsigned char mac_image;
void order_sections(Image *image)
{
    if (!mac_image)
        move_section_last(find_section_by_name(".reloc",&image->sections),&image->sections);
    move_section_last(debug_section,&image->sections);
}


/* Original219f0..22090. Section byte counts include alignment charged to the
 * previous contribution, so its eventual writer emits the inter-item padding. */
extern unsigned char mac_loader_present;
extern unsigned short mac_entry_section;
extern unsigned long mac_near_bss_size,mac_near_data_size,mac_far_bss_size,mac_far_data_size;
extern MacFixupList *mac_section_code_fixups,*mac_section_data_fixups;
unsigned long mac_max_code_size;

void layout_sections(Image *image,unsigned long mask,unsigned long *rva,
                     unsigned long *file,unsigned long *relocations,
                     unsigned long *lines,unsigned long *source_names)
{
    SectionEnumerator sections;
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    Section *section;
    Group *group;
    Contribution *contribution,*previous;
    unsigned long far_data=mask&0x100000,discardable=mask&0x2000000;
    unsigned long kind=mac_image ? mask&0x1e0 : mask&~0x2000000UL;
    unsigned long cursor,file_cursor,size,aligned,padding,fixups;
    unsigned short prefix;
    unsigned char had_input;
    const char *format;

    initialize_section_enumerator(&sections,&image->sections);
    while (next_section(&sections)) {
        section=sections.current;
        if ((section->characteristics&0x2000000)!=discardable ||
            (section->characteristics&0x800) ||
            (section->characteristics&0x1e0)!=kind) continue;
        if (mac_image && (section->characteristics&0x100000)!=far_data) continue;
        cursor=*rva;
        file_cursor=*file;
        prefix=0;
        had_input=0;
        if (mac_image && kind==IMAGE_SCN_CNT_CODE && strncmp(section->name,".jtable",8)) {
            if (section->mac_type==0x45444f43UL &&
                (!mac_loader_present || entry_symbol->owner.contribution->group->section==section))
                prefix=4;
            else prefix=40;
            cursor+=prefix;
            file_cursor+=prefix;
        }
        size=0;
        previous=NULL;
        initialize_group_enumerator(&groups,section);
        while (next_group(&groups)) {
            group=groups.current;
            aligned=cursor&~((unsigned long)group->alignment-1);
            if (aligned!=cursor) aligned+=group->alignment;
            padding=aligned-cursor;
            if (padding) {
                cursor+=padding;
                file_cursor+=padding;
                size+=padding;
                previous->raw_size+=padding;
                previous->padding+=(unsigned char)padding;
            }
            group->rva=cursor;
            group->file_offset=file_cursor;
            initialize_group_contribution_enumerator(&contributions,group);
            while (next_group_contribution(&contributions)) {
                contribution=contributions.current;
                if (contribution->raw_size) had_input=1;
                if (contribution->flags&0x800) contribution->raw_size=0;
                if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
                    continue;
                if (contribution->raw_size) {
                    aligned=align_contribution(cursor,contribution->flags);
                    padding=aligned-cursor;
                    if (padding) {
                        cursor+=padding;
                        previous->raw_size+=padding;
                        previous->padding+=(unsigned char)padding;
                        file_cursor+=padding;
                        size+=padding;
                    }
                    contribution->rva=cursor;
                    contribution->file_offset=file_cursor;
                    previous=contribution;
                    if (mac_image && verbose_link && (kind&0xc0) &&
                        strncmp(section->name,".debug",6) && contribution!=bss_contribution &&
                        contribution!=mac_far_bss) {
                        if (far_data && (kind&0x100040))
                            format="FARDATA  (%8.8s):  %8lx bytes  (%s)\n";
                        else if (far_data && (kind&0x100080))
                            format="FARBSS   (%8.8s):  %8lx bytes  (%s)\n";
                        else if (kind&0x100040)
                            format="NEARDATA (%8.8s):  %8lx bytes  (%s)\n";
                        else if (kind&0x100080)
                            format="NEARBSS  (%8.8s):  %8lx bytes  (%s)\n";
                        else format=NULL;
                        if (format) printf(format,section->name,contribution->raw_size,
                                           contribution->module->path);
                    }
                }
                file_cursor+=contribution->raw_size;
                cursor+=contribution->raw_size;
                size+=contribution->raw_size;
                if ((image->options.debug_level==2 || image->options.debug_level==3) &&
                    contribution->line_count && contribution->raw_size)
                    *lines+=contribution->line_count;
                if (section!=debug_section) *relocations+=contribution->relocation_count;
            }
            group->size=cursor-group->rva;
        }
        if (mac_image) {
            if (!size) file_cursor=*file;
            else size+=prefix;
        }
        if (!size && had_input) --image->file.NumberOfSections;
        section->rva=*rva;
        section->file_offset=*file;
        section->file_end=*file+size;
        *source_names+=section->source_name_capacity;
        if (!section->raw_size && !size) continue;
        section->virtual_size=section->raw_size+size;
        section->raw_size=((section->raw_size+size-1)/image->optional.FileAlignment+1)*
                         image->optional.FileAlignment;
        if (kind==IMAGE_SCN_CNT_CODE) {
            if (mac_image && mac_loader_present && strncmp(section->name,".jtable",8)) {
                if (section->index==mac_entry_section && section->mac_type==0x45444f43UL) {
                    if (mac_section_code_fixups[(unsigned short)section->mac_resource_id].count ||
                        mac_section_data_fixups[(unsigned short)section->mac_resource_id].count)
                        fatal_message(program_name,4,"run-time fixups in startup segment");
                } else {
                    build_mac_section_fixups(section,image);
                    fixups=size_mac_section_fixups(section,
                        ((file_cursor-1)/image->optional.FileAlignment+1)*image->optional.FileAlignment);
                    file_cursor+=fixups;
                    section->file_end=file_cursor;
                    section->raw_size=((section->raw_size+fixups-1)/image->optional.FileAlignment+1)*
                                     image->optional.FileAlignment;
                }
            }
            image->optional.SizeOfCode+=section->raw_size;
            if (mac_image && !(section->characteristics&0x08000000UL) &&
                section->raw_size>mac_max_code_size) mac_max_code_size=section->raw_size;
        } else if (kind==IMAGE_SCN_CNT_INITIALIZED_DATA) {
            if (mac_image && strncmp(section->name,".debug",6)) {
                if (far_data) mac_far_data_size+=section->raw_size;
                else mac_near_data_size+=section->raw_size;
            }
            image->optional.SizeOfInitializedData+=section->raw_size;
        } else if (kind==IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            if (mac_image) {
                if (far_data) mac_far_bss_size+=section->raw_size;
                else mac_near_bss_size+=section->raw_size;
            }
            image->optional.SizeOfUninitializedData+=section->raw_size;
        }
        if (_strcmpi(debug_section_defaults.name,section->name))
            *rva+=((section->raw_size-1)/image->optional.SectionAlignment+1)*
                   image->optional.SectionAlignment;
        if (kind!=IMAGE_SCN_CNT_UNINITIALIZED_DATA) *file+=section->raw_size;
    }
}


unsigned short next_section_index=1,mac_code_section_count=1;
void write_section_headers(unsigned long mask,unsigned long *relocations,
                           unsigned long *lines,Image *image)
{
    unsigned long far_data=mask&0x100000,discardable=mask&0x2000000;
    unsigned long kind=mac_image ? mask&0x1e0 : mask&~0x2000000UL;
    unsigned long relocation_start,line_start,file_offset;
    SectionEnumerator sections;
    Section *section;
    IMAGE_SECTION_HEADER header;
    if (mac_image && kind==IMAGE_SCN_CNT_CODE)
        next_section_index=mac_code_section_count+1;
    initialize_section_enumerator(&sections,&image->sections);
    while (next_section(&sections)) {
        section=sections.current;
        if ((section->characteristics&0x2000000)!=discardable ||
            (section->characteristics&0x1e0)!=kind || !section->raw_size ||
            (mac_image && (section->characteristics&0x100000)!=far_data)) continue;
        relocation_start=*relocations;
        line_start=*lines;
        collect_section_records(section,image,relocations,lines);
        if (section->relocation_count)
            *relocations+=(image->options.rom?8:10)*section->relocation_count;
        if (section->relocation_count) section->relocation_offset=relocation_start;
        if (!section->line_count || !image->options.debug_level) {
            section->line_offset=0;
            section->line_count=0;
        } else section->line_offset=line_start;
        file_offset=section->file_offset;
        if (kind==IMAGE_SCN_CNT_UNINITIALIZED_DATA || !section->raw_size)
            section->file_offset=0;
        make_section_header(section,&header);
        if (section_overrides.count || mac_image) {
            set_mac_resource(&header,section,0);
            if (mac_image && image->options.debug_level) write_mac_section_debug(section);
        }
        if (image->options.rom) adjust_rom_section_header(section,&header,image);
        if ((!image->options.rom || header.Characteristics) &&
            _strnicmp(debug_section_defaults.name,section->name,8)) {
            write_section_header(output_file,&header);
            if (!mac_image || (section->characteristics&0x1e0)!=IMAGE_SCN_CNT_CODE)
                section->index=next_section_index++;
            if (mac_image) record_mac_section_resource(kind,section);
        }
        section->file_offset=file_offset;
        set_section_directory_rva(section,&image->optional,&image->options);
    }
}


void collect_section_records(Section *section,Image *image,unsigned long *relocations,
                             unsigned long *lines)
{
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    Contribution *contribution;
    initialize_group_enumerator(&groups,section);
    while (next_group(&groups)) {
        initialize_group_contribution_enumerator(&contributions,groups.current);
        while (next_group_contribution(&contributions)) {
            contribution=contributions.current;
            if (image->options.eliminate_unreferenced && contribution_discarded(contribution))
                continue;
            section->relocation_count+=contribution->relocation_count;
            *relocations+=(image->options.rom?8:10)*contribution->relocation_count;
            if ((image->options.debug_level==2 || image->options.debug_level==3) &&
                contribution->line_count && contribution->raw_size) {
                if ((image->options.debug_types&2) && (image->options.debug_types&4)) *lines+=261;
                *lines+=contribution->line_count*6;
            }
        }
    }
}


#pragma intrinsic(strcmp)
void set_section_directory_rva(Section *section,IMAGE_OPTIONAL_HEADER *header,
                               ImageOptions *options)
{
    unsigned short directory=0xffff;
    if (!strcmp(debug_section_defaults.name,section->name)) {
        if (options->debug_level) directory=IMAGE_DIRECTORY_ENTRY_DEBUG;
    } else if (!strcmp(".edata",section->name)) directory=IMAGE_DIRECTORY_ENTRY_EXPORT;
    else if (!strcmp(".rsrc",section->name)) directory=IMAGE_DIRECTORY_ENTRY_RESOURCE;
    else if (!strcmp(".pdata",section->name)) directory=IMAGE_DIRECTORY_ENTRY_EXCEPTION;
    if (directory!=0xffff && !header->DataDirectory[directory].VirtualAddress)
        header->DataDirectory[directory].VirtualAddress=section->rva;
}


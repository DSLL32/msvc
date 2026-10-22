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


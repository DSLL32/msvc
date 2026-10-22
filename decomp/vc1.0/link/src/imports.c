/* Keep imported DLL contributions together and move terminating thunks last. */
#include "link-pass.h"
#include <recon.h>
#pragma intrinsic(strlen)

void order_import_contributions(Image *image)
{
    Section *section=find_section_by_name(".idata",&image->sections);
    GroupEnumerator groups;
    GroupContributionEnumerator contributions;
    if (!section) return;
    initialize_group_enumerator(&groups,section);
    while (next_group(&groups)) sort_contributions_by_module(groups.current);
    initialize_group_contribution_enumerator(&contributions,find_group(section,".idata$4"));
    while (next_group_contribution(&contributions)) {
        if (contributions.current->rva) {
            move_contribution_to_module_tail(contributions.current);
            contributions.current->rva=0;
        }
    }
    initialize_group_contribution_enumerator(&contributions,find_group(section,".idata$5"));
    while (next_group_contribution(&contributions)) {
        if (contributions.current->rva) {
            move_contribution_to_module_tail(contributions.current);
            contributions.current->rva=0;
        }
    }
}

/* Supply pointer slots when a dllimport reference resolves to a local definition. */
void resolve_local_imports(Image *image,Contribution **storage,ExternalLink **tail)
{
    UndefinedEnumerator symbols;
    External *alias,*definition;
    ExternalLink *entry;
    TextBuffer references;
    char *name;
    unsigned long offset;
    *storage=NULL;
    initialize_undefined_enumerator(&symbols,image->symbols);
    while (next_undefined(&symbols)) {
        alias=symbols.current;
        if (alias->symbol.N.Name.Short) name=strncpy(short_symbol_name,(char *)&alias->symbol,8);
        else name=image->symbols->strings.data+alias->symbol.N.Name.Long;
        if ((alias->flags&EXTERNAL_IGNORED) || strncmp(name,"__imp_",6)) continue;
        name+=6;
        definition=find_external(image->symbols,(unsigned char)(strlen(name)<=8),name);
        if (!definition || !(definition->flags&EXTERNAL_DEFINED)) continue;
        if (warning_level>=2) {
            format_undefined_references(&references,alias);
            warning_message(program_name,0x78,name,references.data);
            release_text(&references);
        }
        if (!*storage) {
            offset=0;
            *storage=create_contribution(readonly_section_defaults.name,4,0,0,0,0,0,0,
                readonly_section_defaults.flags,0,output_module,1,&image->sections,&image->optional);
            (*storage)->flags |= 0x2000;
            (*storage)->graph=create_comdat_node(*storage,NULL,NULL,&comdat_nodes,1);
        } else {
            offset=(*storage)->raw_size;
            (*storage)->raw_size+=4;
        }
        define_external(alias,*storage,offset,0,0,0,image->symbols);
        entry=*tail=malloc(sizeof(*entry));
        if (!entry) out_of_memory();
        entry->external=alias;
        tail=&entry->next;
        entry=*tail=malloc(sizeof(*entry));
        if (!entry) out_of_memory();
        entry->external=definition;
        tail=&entry->next;
        if (!image->options.fixed && !image->options.rom)
            ++image->optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        if (image->options.eliminate_unreferenced)
            add_comdat_root(NULL,definition,NULL,&comdat_roots);
    }
    *tail=NULL;
}

void write_local_imports(Contribution *storage,ExternalLink **aliases,
                         IMAGE_OPTIONAL_HEADER *header,ImageOptions *options)
{
    ExternalLink *alias_entry,*definition_entry;
    External *alias,*definition;
    unsigned long address;
    if (!*aliases) return;
    seek_file(output_file,storage->file_offset,SEEK_SET);
    while (*aliases) {
        alias_entry=*aliases;
        definition_entry=alias_entry->next;
        alias=alias_entry->external;
        definition=definition_entry->external;
        address=definition->symbol.Value+definition->owner.contribution->rva+header->ImageBase;
        write_file(output_file,&address,sizeof(address));
        if (!options->fixed && !options->rom) {
            base_fixup_next->type=IMAGE_REL_BASED_HIGHLOW;
            base_fixup_next->rva=alias->symbol.Value+alias->owner.contribution->rva;
            base_fixup_next->value.adjustment=0;
            ++base_fixup_next;
        }
        alias->value.final_value=alias->symbol.Value+alias->owner.contribution->rva+header->ImageBase;
        alias->symbol.SectionNumber=alias->owner.contribution->group->section->index;
        *aliases=definition_entry->next;
        free(definition_entry);
        free(alias_entry);
    }
}


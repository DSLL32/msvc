/* Map-file symbol and relocation records accumulated during output. */
#include "pass-two.h"
#pragma intrinsic(strlen)
typedef struct MapRelocationBlock {
    unsigned long rvas[32];
    struct MapRelocationBlock *next;
} MapRelocationBlock;
MapRelocationBlock *map_relocations;
unsigned long map_relocations_used;
typedef char map_relocation_block_layout[(sizeof(MapRelocationBlock)==132)?1:-1];

/* Original [00025480,000254f0). */
void record_map_relocation(unsigned long rva)
{
    MapRelocationBlock *block;
    if (!map_relocations || map_relocations_used>=32) {
        block=malloc(sizeof(*block));
        if (!block) out_of_memory();
        block->next=map_relocations;
        map_relocations=block;
        map_relocations_used=0;
    }
    map_relocations->rvas[map_relocations_used++]=rva;
}

typedef struct MapSymbol {
    struct MapSymbol *next;
    char *name;
    Contribution *contribution;
    unsigned long value;
    unsigned char function;
} MapSymbol;
MapSymbol *map_static_symbols;
MapSymbol **map_symbol_tail=&map_static_symbols;
typedef char map_symbol_layout[(sizeof(MapSymbol)==20)?1:-1];

/* Original [00027820,000278a0). Preserve input order for static map symbols. */
void record_map_symbol(const char *name,Contribution *contribution,unsigned long value,unsigned char function)
{
    MapSymbol *symbol;
    *map_symbol_tail=malloc(sizeof(**map_symbol_tail));
    if (!*map_symbol_tail) out_of_memory();
    symbol=*map_symbol_tail;
    symbol->name=duplicate_string(name);
    symbol->contribution=contribution;
    symbol->value=value;
    symbol->function=function;
    symbol->next=NULL;
    map_symbol_tail=&symbol->next;
}

extern FILE *map_stream;
extern unsigned char mac_image;
External **get_externals_by_symbol(SymbolTable *);
void write_map_symbol(const char *,Contribution *,unsigned long,unsigned long,unsigned long,unsigned char);

/* Original [00027130,000276c0). Human-readable map, statics and fixup deltas. */
void write_map(int kind,Image *image,const char *name)
{
    char filename[_MAX_FNAME],buffer[84],resource[5];
    char *date;
    const char *symbol_name,*class_name;
    SectionEnumerator sections;
    GroupEnumerator groups;
    Section *section;
    Group *group;
    External **ordered,*external;
    Contribution *contribution;
    MapSymbol *symbol,*next_symbol;
    MapRelocationBlock *block,*next_block;
    unsigned long count,index,value,mac_value,entry_offset,previous,remaining;
    unsigned short entry_section,length;
    _splitpath(name,NULL,NULL,filename,NULL);
    fprintf(map_stream," %s\n\n",filename);
    date=ctime((time_t *)&image->file.TimeDateStamp);
    if (date) date[strlen(date)-1]=0;
    else date="invalid";
    fprintf(map_stream," Timestamp is %08lx (%s)\n\n",image->file.TimeDateStamp,date);
    fprintf(map_stream," Preferred load address is %08lx\n\n",image->optional.ImageBase);
    if (mac_image)
        fprintf(map_stream," Start         Length     Name                   Class     Resource\n");
    else fprintf(map_stream," Start         Length     Name                   Class\n");
    initialize_section_enumerator(&sections,&image->sections);
    while (next_section(&sections)) {
        section=sections.current;
        resource[0]=resource[1]=resource[2]=resource[3]=resource[4]=0;
        if (!section->index) continue;
        initialize_group_enumerator(&groups,section);
        while (next_group(&groups)) {
            group=groups.current;
            class_name=(section->characteristics&0x1e0)==0x20?"CODE":"DATA";
            if (mac_image && (section->characteristics&0x20)) {
                *(unsigned long *)resource=section->mac_type;
                fprintf(map_stream," %04x:%08lx %08lxH %-23s %-9s %s%04d\n",
                    section->index,group->rva-section->rva,group->size,group->name,class_name,
                    resource,(int)(short)section->mac_resource_id);
            } else fprintf(map_stream," %04x:%08lx %08lxH %-23s %s\n",
                section->index,group->rva-section->rva,group->size,group->name,class_name);
        }
    }
    fprintf(map_stream,"\n  Address         Publics by Value              Rva+Base   Lib:Object\n\n");
    ordered=mac_image?get_externals_by_symbol(image->symbols):get_externals_by_address(image->symbols);
    count=count_externals(image->symbols);
    for (index=0;index<count;++index) {
        external=ordered[index];
        contribution=external->owner.contribution;
        if (!(external->flags&EXTERNAL_DEFINED) || !contribution) continue;
        if (image->options.eliminate_unreferenced && contribution_discarded(contribution)) continue;
        if (external->symbol.N.Name.Short)
            symbol_name=strncpy(short_symbol_name,(char *)&external->symbol,8);
        else symbol_name=image->symbols->strings.data+external->symbol.N.Name.Long;
        if ((unsigned char)*symbol_name>=0x7f) {
            sprintf(buffer,"\\%o%s",(unsigned char)*symbol_name,symbol_name+1);
            symbol_name=buffer;
        }
        mac_value=mac_image?external->value.final_value:0;
        value=contribution==bss_contribution?
            external->value.final_value-contribution->rva:external->symbol.Value;
        write_map_symbol(symbol_name,contribution,value,mac_value,image->optional.ImageBase,
            (unsigned char)((external->symbol.Type&0x30)==0x20));
    }
    if (entry_symbol && (entry_symbol->flags&EXTERNAL_DEFINED)) {
        contribution=entry_symbol->owner.contribution;
        section=contribution->group->section;
        entry_section=section->index;
        entry_offset=mac_image?entry_symbol->value.final_value:
            contribution->rva-section->rva+entry_symbol->symbol.Value;
    } else { entry_section=0; entry_offset=0; }
    fprintf(map_stream,"\n entry point at        %04x:%08lx\n\n",entry_section,entry_offset);
    fprintf(map_stream," Static symbols\n\n");
    while (map_static_symbols) {
        symbol=map_static_symbols;
        contribution=symbol->contribution;
        if (!(image->options.eliminate_unreferenced && contribution_discarded(contribution))) {
            value=contribution->rva-contribution->group->section->rva+symbol->value;
            write_map_symbol(symbol->name,contribution,symbol->value,value,
                image->optional.ImageBase,symbol->function);
        }
        next_symbol=symbol->next;
        free(symbol->name);
        free(symbol);
        map_static_symbols=next_symbol;
    }
    fprintf(map_stream,"\n");
    length=0;
    remaining=map_relocations_used;
    for (block=map_relocations;block;block=block->next) {
        for (index=0;index<remaining;++index) {
            if (!length) {
                length=(unsigned short)sprintf(buffer,"FIXUPS:");
                previous=0;
            }
            length+=(unsigned short)sprintf(buffer+length," %lx",block->rvas[index]-previous);
            previous=block->rvas[index];
            if (length>=70) { fprintf(map_stream,"%s\n",buffer); length=0; }
        }
        remaining=32;
    }
    if (length) fprintf(map_stream,"%s\n",buffer);
    while (map_relocations) {
        next_block=map_relocations->next;
        free(map_relocations);
        map_relocations=next_block;
    }
}

extern Library *explicit_input_library;

/* Original [000276c0,00027820). One public/static map row with its owner. */
void write_map_symbol(const char *name,Contribution *contribution,unsigned long value,
                      unsigned long mac_value,unsigned long image_base,unsigned char function)
{
    Section *section=contribution->group->section;
    Module *module=contribution->module;
    char filename[_MAX_FNAME],extension[_MAX_EXT];
    unsigned long offset=mac_image?mac_value:contribution->rva-section->rva+value;
    fprintf(map_stream," %04x:%08lx       %-26s %08lx %c ",section->index,offset,
        name,contribution->rva+image_base+value,function?'f':' ');
    if (module->library!=explicit_input_library && module!=output_module) {
        _splitpath(module->library->name,NULL,NULL,filename,NULL);
        fprintf(map_stream,"%s:",filename);
    }
    if (contribution==bss_contribution) fprintf(map_stream,"<common>");
    else if (module==output_module) fprintf(map_stream,"<linker-defined>");
    else {
        _splitpath(module->path,NULL,NULL,filename,extension);
        fprintf(map_stream,"%s%s",filename,extension);
    }
    fprintf(map_stream,"\n");
}

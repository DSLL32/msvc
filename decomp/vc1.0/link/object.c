/* COFF sections and symbols read during pass one. */
#include "object.h"
#include "comdat.h"
#pragma intrinsic(strcmp)
char *coff_strings;
unsigned long section_header_offset;
void **mac_object_symbols;
extern unsigned char mac_image;
extern unsigned long incremental_section_header_offset,string_table_size;

void process_object(Image *image,unsigned char *new_externals,unsigned char *symbols_changed,Module *module,
    unsigned char explicit_input,unsigned short member)
{
    char *name;
    unsigned long string_bytes,index;
    PIMAGE_SYMBOL symbols;
    if (mac_image && tool_kind!=2) initialize_mac_object_symbols(module->symbol_count);
    name=module->library->name;
    if (!name) name=module->name.display_name;
    coff_strings=read_coff_strings(name,module->symbol_offset+module->symbol_count*18+
        incremental_section_header_offset,&string_bytes);
    string_table_size+=string_bytes;
    symbols=read_coff_symbols(module->symbol_offset+incremental_section_header_offset,
        module->symbol_count);
    if (mac_image) mac_object_symbols=calloc(module->symbol_count,sizeof(void *));
    section_header_offset=module->optional_header_size+20;
    for (index=0;index<module->section_count;++index)
        process_object_section(image,module,explicit_input,(unsigned short)(index+1),symbols);
    process_object_symbols(image,module,new_externals,symbols_changed,symbols,member);
    if (mac_image && tool_kind!=2) {
        finish_mac_object_symbols();
        process_mac_object_symbols(image->symbols,symbols,coff_strings);
    }
    release_coff_strings(coff_strings);
    release_coff_symbols(symbols);
    if (mac_image) free(mac_object_symbols);
    coff_strings=NULL;
}

void process_object_section(Image *image,Module *module,unsigned char explicit_input,
    unsigned short index,PIMAGE_SYMBOL symbols)
{
    IMAGE_SECTION_HEADER header;
    Contribution *contribution;
    char *comdat_name=NULL,*directives;
    char display[_MAX_PATH];
    unsigned long member_offset;
    unsigned char alignment;
    seek_file(incremental_file,section_header_offset+incremental_section_header_offset,SEEK_SET);
    read_section_header(incremental_file,&header);
    section_header_offset+=sizeof(header);
    contribution=create_contribution(coff_section_name(header.Name,coff_strings),
        header.SizeOfRawData,header.NumberOfRelocations,header.NumberOfLinenumbers,
        header.PointerToRelocations,header.PointerToLinenumbers,header.PointerToRawData,
        header.Characteristics,header.Characteristics,header.VirtualAddress,module,0,
        &image->sections,&image->optional);
    filter_debug_contribution(contribution,&image->options);
    alignment=(unsigned char)align_contribution(1,header.Characteristics);
    if (alignment>=contribution->group->alignment)
        contribution->group->alignment=(unsigned char)align_contribution(1,header.Characteristics);
    if (contribution->flags&IMAGE_SCN_LNK_COMDAT) {
        if (!select_comdat(image->symbols,contribution,symbols,index,&comdat_name)) {
            contribution->raw_size=0;
            return;
        }
    }
    if (image->options.eliminate_unreferenced) attach_comdat_node(module,contribution,comdat_name);
    if (!(contribution->flags&IMAGE_SCN_LNK_COMDAT) && image->options.eliminate_unreferenced)
        add_comdat_root(NULL,NULL,contribution,&comdat_roots);
    if (contribution->flags&IMAGE_SCN_LNK_INFO) {
        if (strcmp(".drectve",contribution->group->name)) return;
        member_offset=module->library->name ? module->name.member_offset : 0;
        seek_file(incremental_file,contribution->data_offset+member_offset,SEEK_SET);
        directives=calloc(contribution->raw_size+1,1);
        if (!directives) out_of_memory();
        read_file(incremental_file,directives,contribution->raw_size);
        process_directives(directives,image,format_module_name(module,display),
            (unsigned char)(module->library->name!=NULL));
        free(directives);
        return;
    }
    if (contribution->flags&0x800) return;
    contribution->checksum &= 0x00ffffff;
    classify_contribution(contribution,&image->options);
    if (strcmp(contribution->group->name,".debug$S") &&
        strcmp(contribution->group->name,".debug$T") &&
        strcmp(contribution->group->name,".debug$P") && tool_kind!=2)
        read_contribution_relocations(image,contribution,symbols);
}

void filter_debug_contribution(Contribution *contribution,ImageOptions *options)
{
    char *name=contribution->group->name;
    if ((!options->debug_level || !(options->debug_types&2)) &&
        (!strcmp(name,".debug$S") || !strcmp(name,".debug$T") ||
         !strcmp(name,".debug$P"))) contribution->flags |= 0x800;
    if ((!options->debug_level || !(options->debug_types&0x10)) && !strcmp(name,".debug$G"))
        contribution->flags |= 0x800;
    if (!strcmp(name,".debug$F") && (!options->debug_level || !(options->debug_types&8)))
        contribution->flags |= 0x800;
}

unsigned long align_contribution(unsigned long offset,unsigned long flags)
{
    unsigned long mask;
    if (flags&8) return offset;
    if (mac_image) mask=3;
    else switch (flags&0x700000) {
    case 0: case 0x500000: mask=15; break;
    case 0x200000: mask=1; break;
    case 0x300000: mask=3; break;
    case 0x400000: mask=7; break;
    case 0x600000: mask=31; break;
    case 0x700000: mask=63; break;
    default: return offset;
    }
    return (offset&mask) ? (offset&~mask)+mask+1 : offset;
}

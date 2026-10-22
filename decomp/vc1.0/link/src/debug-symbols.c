/* Per-input-section COFF function/line-number associations. */
#include "pass-two.h"
#include <stddef.h>
#include <recon.h>
typedef char cv_subsection_layout[(sizeof(CvSubsection)==12)?1:-1];
typedef char module_debug_layout[(sizeof(ModuleDebugInfo)==76)?1:-1];
typedef char module_debug_symbols_layout[(offsetof(ModuleDebugInfo,symbols)==0x1c)?1:-1];
typedef char module_debug_types_layout[(offsetof(ModuleDebugInfo,types)==0x28)?1:-1];
typedef char module_debug_lines_layout[(offsetof(ModuleDebugInfo,lines)==0x34)?1:-1];
typedef char module_debug_module_layout[(offsetof(ModuleDebugInfo,module_record)==0x40)?1:-1];
extern char *coff_strings;
typedef char function_symbol_layout[(sizeof(FunctionSymbol)==12)?1:-1];
typedef char function_symbol_map_layout[(sizeof(FunctionSymbolMap)==12)?1:-1];

/* Original [00035b40,00035ba0). */
FunctionSymbolMap *allocate_function_symbol_maps(unsigned short count)
{
    FunctionSymbolMap *maps=malloc(count*sizeof(*maps));
    unsigned short index;
    if (!maps) out_of_memory();
    for (index=0;index<count;++index) {
        maps[index].capacity=0;
        maps[index].count=0;
        maps[index].entries=NULL;
    }
    return maps;
}

/* Original [00035c80,00035cd0). */
void release_function_symbol_maps(FunctionSymbolMap *maps,unsigned short count)
{
    unsigned short index;
    for (index=0;index<count;++index)
        if (maps[index].entries) free(maps[index].entries);
    free(maps);
}

/* Original [0001dfd0,0001e070). Recognize a section-definition auxiliary. */
unsigned char is_section_symbol(Image *image,PIMAGE_SYMBOL symbol)
{
    const char *name;
    SectionEnumerator sections;
    if (symbol->NumberOfAuxSymbols!=1) return 0;
    if (symbol->N.Name.Short)
        name=strncpy(short_symbol_name,(char *)symbol,8);
    else name=image->symbols->strings.data+symbol->N.Name.Long;
    initialize_section_enumerator(&sections,&image->sections);
    while (next_section(&sections)) {
        if (find_group(sections.current,name)) {
            end_section_enumerator(&sections);
            return 1;
        }
    }
    return 0;
}
void append_function_symbol(FunctionSymbolMap *,unsigned short,unsigned long,unsigned long,unsigned long);

/* Original [0001dec0,0001dfd0). Chain emitted COFF function auxiliaries. */
void update_function_auxiliary(PIMAGE_SYMBOL symbol,short section,PIMAGE_AUX_SYMBOL auxiliary,
                                unsigned long *previous,unsigned long *function,
                                FunctionSymbolMap *maps,unsigned long index,Image *image)
{
    unsigned long position;
    IMAGE_AUX_SYMBOL saved;
    if ((symbol->Type&0x30)!=0x20 || symbol->NumberOfAuxSymbols!=1 ||
        image->options.debug_level!=3) return;
    position=tell_file(output_file);
    *previous=position;
    if (*function) {
        seek_file(output_file,*function,SEEK_SET);
        read_coff_symbol(output_file,(PIMAGE_SYMBOL)&saved);
        saved.Sym.FcnAry.Function.PointerToNextFunction=symbol_count-1;
        seek_file(output_file,*function,SEEK_SET);
        write_coff_auxiliary(output_file,&saved);
        seek_file(output_file,position,SEEK_SET);
        *function=position;
    } else *function=tell_file(output_file);
    auxiliary->Sym.FcnAry.Function.PointerToNextFunction=0;
    if (section!=-1)
        append_function_symbol(maps,(unsigned short)section,index-2,position,symbol_count-1);
}

/* Original [00035ba0,00035c30). */
void append_function_symbol(FunctionSymbolMap *maps,unsigned short section,
                             unsigned long input,unsigned long offset,unsigned long output)
{
    FunctionSymbolMap *map=&maps[(unsigned short)(section-1)];
    FunctionSymbol *entry;
    if (map->count>=map->capacity) {
        if (!map->capacity) {
            map->capacity=10;
            map->entries=malloc(10*sizeof(*map->entries));
        } else {
            map->capacity*=2;
            map->entries=realloc(map->entries,map->capacity*sizeof(*map->entries));
        }
        if (!map->entries) out_of_memory();
    }
    entry=&map->entries[map->count++];
    entry->input_index=input;
    entry->auxiliary_offset=offset;
    entry->output_index=output;
}

/* Original [0001e170,0001e1c0). Move debug-only names into the output strings. */
void update_debug_symbol(Image *image,PIMAGE_SYMBOL symbol)
{
    if (symbol->N.Name.Short) return;
    if (!emit_symbol_class(symbol->StorageClass,&image->options)) return;
    if (symbol->N.Name.Long)
        symbol->N.Name.Long=intern_symbol_name(image->symbols,coff_strings+symbol->N.Name.Long);
}

/* Original [0001dda0,0001dec0). Link .bf records to their function auxiliary. */
void update_block_auxiliary(PIMAGE_SYMBOL symbol,PIMAGE_AUX_SYMBOL auxiliary,
                             unsigned long *previous,unsigned long *function,
                             unsigned long *block,unsigned long remaining)
{
    unsigned long position;
    IMAGE_AUX_SYMBOL saved;
    if (((char *)symbol)[1]!='b' || symbol->NumberOfAuxSymbols!=(unsigned char)remaining)
        return;
    position=tell_file(output_file);
    if (*block) {
        seek_file(output_file,*block,SEEK_SET);
        read_coff_symbol(output_file,(PIMAGE_SYMBOL)&saved);
        saved.Sym.FcnAry.Function.PointerToNextFunction=symbol_count-1;
        seek_file(output_file,*block,SEEK_SET);
        write_coff_auxiliary(output_file,&saved);
    }
    if (*previous) {
        seek_file(output_file,*previous,SEEK_SET);
        read_coff_symbol(output_file,(PIMAGE_SYMBOL)&saved);
        saved.Sym.TagIndex=symbol_count-1;
        seek_file(output_file,*previous,SEEK_SET);
        write_coff_auxiliary(output_file,&saved);
    }
    if (*block || *previous) seek_file(output_file,position,SEEK_SET);
    *block=position;
    *previous=0;
}

/* Original [00035c30,00035c80). Resolve a COFF line table's function marker. */
unsigned char lookup_function_symbol(FunctionSymbolMap *maps,unsigned short section,
                                      unsigned long input,unsigned long *offset,unsigned long *output)
{
    FunctionSymbolMap *map=&maps[(unsigned short)(section-1)];
    unsigned long index;
    for (index=0;index<map->count;++index) {
        if (map->entries[index].input_index==input) {
            *offset=map->entries[index].auxiliary_offset;
            *output=map->entries[index].output_index;
            return 1;
        }
    }
    return 0;
}

/* Original [00025400,00025410). FPO records are sorted by procedure RVA. */
int compare_fpo_records(const void *left,const void *right)
{
    return (int)(*(const unsigned long *)left-*(const unsigned long *)right);
}

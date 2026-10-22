/* Per-input-section COFF function/line-number associations. */
#include "pass-two.h"
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

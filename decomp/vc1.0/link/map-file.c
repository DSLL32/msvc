/* Map-file symbol and relocation records accumulated during output. */
#include "pass-two.h"
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

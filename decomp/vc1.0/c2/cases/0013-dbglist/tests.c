#include <recon-tool.h>
#include "listing.h"
#include "codeview-output.h"
#include "temporaries.h"
#include "function-input.h"
#include <recon.h>
static int register_names(void)
{
    static const struct { unsigned width, reg; const char *name; } rows[] = {
        {0,0,"cr0"},
        {0,1,0},
        {0,2,"cr2"},
        {0,3,"cr3"},
        {0,4,0},
        {0,5,0},
        {0,6,0},
        {0,7,0},
        {0,8,"dr0"},
        {0,9,"dr1"},
        {0,10,"dr2"},
        {0,11,"dr3"},
        {0,12,0},
        {0,13,0},
        {0,14,"dr6"},
        {0,15,"dr7"},
        {0,16,0},
        {0,17,0},
        {0,18,0},
        {0,19,0},
        {0,20,0},
        {0,21,0},
        {0,22,"tr6"},
        {0,23,"tr7"},
        {1,0,"al"},
        {1,1,"cl"},
        {1,2,"dl"},
        {1,3,"bl"},
        {1,4,"ah"},
        {1,5,"ch"},
        {1,6,"dh"},
        {1,7,"bh"},
        {2,0,"ax"},
        {2,1,"cx"},
        {2,2,"dx"},
        {2,3,"bx"},
        {2,4,"sp"},
        {2,5,"bp"},
        {2,6,"si"},
        {2,7,"di"},
        {2,8,"es"},
        {2,9,"cs"},
        {2,10,"ss"},
        {2,11,"ds"},
        {2,12,"fs"},
        {2,13,"gs"},
        {3,0,"ax"},
        {3,1,"cx"},
        {3,2,"dx"},
        {3,3,"bx"},
        {3,4,"sp"},
        {3,5,"bp"},
        {3,6,"si"},
        {3,7,"di"},
        {3,8,"es"},
        {3,9,"cs"},
        {3,10,"ss"},
        {3,11,"ds"},
        {3,12,"fs"},
        {3,13,"gs"},
        {4,0,"eax"},
        {4,1,"ecx"},
        {4,2,"edx"},
        {4,3,"ebx"},
        {4,4,"esp"},
        {4,5,"ebp"},
        {4,6,"esi"},
        {4,7,"edi"},
        {4,8,"es"},
        {4,9,"cs"},
        {4,10,"ss"},
        {4,11,"ds"},
        {4,12,"fs"},
        {4,13,"gs"},
        {6,0,"eax"},
        {6,1,"ecx"},
        {6,2,"edx"},
        {6,3,"ebx"},
        {6,4,"esp"},
        {6,5,"ebp"},
        {6,6,"esi"},
        {6,7,"edi"},
        {6,8,"es"},
        {6,9,"cs"},
        {6,10,"ss"},
        {6,11,"ds"},
        {6,12,"fs"},
        {6,13,"gs"},
        {8,0,"ax"},
        {8,1,"cx"},
        {8,2,"dx"},
        {8,3,"bx"},
        {8,4,"sp"},
        {8,5,"bp"},
        {8,6,"si"},
        {8,7,"di"},
        {8,8,"es"},
        {8,9,"cs"},
        {8,10,"ss"},
        {8,11,"ds"},
        {8,12,"fs"},
        {8,13,"gs"},
        {10,0,"ax"},
        {10,1,"cx"},
        {10,2,"dx"},
        {10,3,"bx"},
        {10,4,"sp"},
        {10,5,"bp"},
        {10,6,"si"},
        {10,7,"di"},
        {10,8,"es"},
        {10,9,"cs"},
        {10,10,"ss"},
        {10,11,"ds"},
        {10,12,"fs"},
        {10,13,"gs"},
    };
    unsigned i;
    for(i=0;i<RECON_COUNT(rows);++i){
        char *value=machine_register_name(rows[i].reg,rows[i].width);
        if(rows[i].name ? !value||strcmp(value,rows[i].name) : value!=0)return 0;
    }
    return 1;
}
static int register_codes(void)
{
    TemporarySymbol symbol = {0};
    symbol.symbol.value=1;symbol.field28=3;
    if(codeview_register_number(&symbol.symbol)!=4)return 0;
    symbol.symbol.value=2;
    if(codeview_register_number(&symbol.symbol)!=12)return 0;
    symbol.symbol.value=4;
    return codeview_register_number(&symbol.symbol)==20;
}
static int listing_aliases(void)
{
    return sizeof(ListingStorage)==336 &&
        (void *)&listing_storage.indexed[20].name==(void *)&listing_storage.state.cursor &&
        (void *)&listing_storage.indexed[20].file==(void *)&listing_storage.state.lines &&
        (void *)&listing_storage.indexed[20].lines==(void *)&listing_storage.state.enabled &&
        (void *)&listing_storage.indexed[20].line==(void *)&listing_storage.state.segment;
}

const ReconTest c2_tests[] = {
    { "original register-name returns and NULLs", 1, register_names },
    { "original CodeView byte/word/DWORD register IDs", 1, register_codes },
    { "genuine listing row20 storage aliases", 1, listing_aliases },
};

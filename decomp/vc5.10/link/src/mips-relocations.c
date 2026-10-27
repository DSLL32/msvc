#include <windows.h>
#include <string.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
#include "sections.h"
#include "full-link-functions.h"
#include "debug-output.h"
__declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

extern char *current_module_strings;
extern char short_symbol_name[9];
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);
BYTE __stdcall classify_mips_relocation(Contribution *,WORD,DWORD,BYTE);

DWORD __stdcall read_mips_relocations(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocations,DWORD count,DWORD index)
{
    ContributionInfo *info=get_contribution_info(contribution);
    IMAGE_RELOCATION *relocation=relocations;
    IMAGE_SYMBOL *symbol;
    const char *name;
    DWORD checksum=0,total=0,i;
    WORD kind;
    BYTE folding=context->options.fold_identical_comdats && buffer4.remaining &&
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]==info;
    for (i=0;i<count;++i,++relocation) {
        if (classify_mips_relocation(contribution,relocation->Type,0,0)) continue;
        symbol=symbols+relocation->SymbolTableIndex;
        if (symbol->StorageClass==IMAGE_SYM_CLASS_UNDEFINED_STATIC) {
            name=get_symbol_name((StringNode *)symbol,current_module_strings,&kind,NULL);
            if (kind==1) name=strncpy(short_symbol_name,name,8);
            link_contribution_fatal(contribution,1186,name);
        }
        if (context->options.effective_reference_opt || context->options.fold_identical_comdats)
            record_relocation_reference(context,contribution,symbols,relocation);
        if (folding) {
            checksum=SigForPbCb(&relocation->Type,2,checksum);
            checksum=SigForPbCb(&relocation->VirtualAddress,4,checksum);
            checksum=SigForPbCb(&symbol->StorageClass,1,checksum);
        }
        if (!context->options.fixed_image) {
            switch(relocation->Type) {
            case IMAGE_REL_MIPS_REFHALF: case IMAGE_REL_MIPS_REFWORD: case IMAGE_REL_MIPS_JMPADDR: case IMAGE_REL_MIPS_REFLO: /* REFHALF/REFWORD/JMPADDR/REFLO */
                ++total;break;
            case IMAGE_REL_MIPS_REFHI: /* REFHI */
                total+=2;break;
            }
        }
    }
    if (folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return total;
}

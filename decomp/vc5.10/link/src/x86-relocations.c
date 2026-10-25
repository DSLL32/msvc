#include <windows.h>
#include <string.h>
#include "input.h"
#include "initializers.h"
#include "full-link-functions.h"
__declspec(dllimport) DWORD __cdecl SigForPbCb(const void *,long,DWORD);
#include <recon.h>

extern char *current_module_strings;
extern char short_symbol_name[9];
ContributionInfo *__stdcall get_contribution_info(Contribution *);
void __stdcall record_relocation_reference(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *);

DWORD __stdcall read_x86_relocations(LinkContext *context,Contribution *contribution,
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
    BYTE vxd=context->is_vxd==1;
    for (i=0;i<count;++i,++relocation) {
        if (classify_x86_relocation(contribution,relocation->Type,0,vxd)) continue;
        symbol=symbols+relocation->SymbolTableIndex;
        if (symbol->StorageClass==IMAGE_SYM_CLASS_LABEL) {
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
        if (context->is_vxd==1) ++total;
        else if (!context->options.fixed_image) {
            if (relocation->Type==6) ++total; /* IMAGE_REL_I386_DIR32 */
            else if (relocation->Type==20 && context->options.new_relocations &&
                (WORD)index!=(WORD)symbols[relocations->SymbolTableIndex].SectionNumber)
                ++total;
        }
    }
    if (folding) {
        checksum=SigForPbCb(&contribution->characteristics,4,checksum);
        checksum=SigForPbCb(&contribution->comdat_selection,1,checksum);
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->reserved14=checksum;
    }
    return total;
}
void __stdcall record_relocation_reference(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocation) { __asm { int 3 } }

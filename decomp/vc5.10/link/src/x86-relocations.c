#include <windows.h>
#include <string.h>
#include "input.h"
#include "initializers.h"
#include "comdat.h"
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
        ((ContributionInfo **)buffer4.base)[buffer4.remaining-1]->fold_signature=checksum;
    }
    return total;
}
extern BYTE incremental_enabled;
IMAGE_SYMBOL *__stdcall create_incremental_symbol(LinkContext *,Contribution *,DWORD,IMAGE_SYMBOL *,BYTE);
void __stdcall redirect_comdat_reference(ComdatReference *,Contribution *);
void __stdcall record_relocation_reference(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,IMAGE_RELOCATION *relocation)
{
    IMAGE_SYMBOL *symbol=symbols+relocation->SymbolTableIndex,*redirected;
    OutputModule *module=contribution->module;
    Contribution *target,*redirected_target;
    ComdatReference *reference;
    if (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL || symbol->StorageClass==0x44 ||
        symbol->StorageClass==0x69 /* weak external */) {
        reference=(ComdatReference *)add_comdat_reference(relocation->SymbolTableIndex,contribution,NULL);
        if (incremental_enabled && relocation->Type!=0x28) reference->flag_byte=1;
        return;
    }
    if (symbol->SectionNumber<=0) return;
    target=(Contribution *)module->contributions+symbol->SectionNumber-1;
    if (incremental_enabled && (symbol->Type & 0x8000) && relocation->Type!=0x28) {
        redirected=create_incremental_symbol(context,contribution,1,symbol,0);
        redirected_target=(Contribution *)module->contributions+redirected->SectionNumber-1;
        reference=(ComdatReference *)add_comdat_reference(0,contribution,target);
        redirect_comdat_reference(reference,redirected_target);
        return;
    }
    if (contribution!=target) add_comdat_reference(0,contribution,target);
}
IMAGE_SYMBOL *__stdcall create_incremental_symbol(LinkContext *context,Contribution *contribution,
    DWORD kind,IMAGE_SYMBOL *symbol,BYTE handler) { __asm { int 3 } return NULL; }
void __stdcall redirect_comdat_reference(ComdatReference *reference,Contribution *target) { __asm { int 3 } }

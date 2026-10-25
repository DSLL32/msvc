/* The contribution reader is a genuine thiscall member; its dispatcher has
 * the original C-linkage stdcall ABI. */
#include <windows.h>
#include "input.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern OutputModule *output_module;
DWORD total_relocation_count;
DWORD __stdcall read_x86_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_mips_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_alpha_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_sh_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_arm_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_mac_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
DWORD __stdcall read_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD);
void __stdcall release_relocations(IMAGE_RELOCATION *);

void __stdcall read_section_relocations(LinkContext *context,Contribution *contribution,
    IMAGE_SYMBOL *symbols,OutputModule *module)
{
    IMAGE_RELOCATION *relocations;
    DWORD count,index,size=0;
    if (contribution->module==output_module || !contribution->relocation_count) return;
    if (!context->file_header.Machine) link_contribution_fatal(contribution,1167);
    relocations=contribution->ReadRelocations(&count);
    total_relocation_count+=count;
    index=((LONG)((DWORD)contribution-(DWORD)module->contributions)>>6)+1;
    switch (context->file_header.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        size=read_x86_relocations(context,contribution,symbols,relocations,count,index);
        break;
    case 0x162: case 0x166 /* R4000 */: case 0x168:
        size=read_mips_relocations(context,contribution,symbols,relocations,count,index);
        break;
    case 0x184 /* Alpha */:
        size=read_alpha_relocations(context,contribution,symbols,relocations,count,index);
        break;
    case 0x1a2: case 0x1a6:
        size=read_sh_relocations(context,contribution,symbols,relocations,count,index);
        break;
    case 0x601:
        size=read_mac_ppc_relocations(context,module,contribution,symbols,relocations,count,index);
        break;
    case 0x1f0 /* PowerPC */:
        size=read_ppc_relocations(context,module,contribution,symbols,relocations,count,index);
        break;
    case 0x1c0:
        size=read_arm_relocations(context,contribution,symbols,relocations,count,index);
        break;
    }
    release_relocations(relocations);
    if (size) {
        context->optional_header.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size+=size;
        module->base_relocation_size+=size;
    }
}
DWORD __stdcall read_x86_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_mips_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_alpha_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_sh_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_arm_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_mac_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
void __stdcall release_relocations(IMAGE_RELOCATION *) { __asm { int 3 } }
}
IMAGE_RELOCATION *RECON_THISCALL Contribution::ReadRelocations(DWORD *)
{
    __asm { int 3 }
    return NULL;
}

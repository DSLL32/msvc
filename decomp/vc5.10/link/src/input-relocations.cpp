/* The contribution reader is a genuine thiscall member; its dispatcher has
 * the original C-linkage stdcall ABI. */
#include <windows.h>
#include "input.h"
#include "files.h"
#include "full-link-functions.h"
#include "librarian.h"
#include <recon.h>

extern "C" {
extern OutputModule *output_module;
DWORD total_relocation_count;
BYTE relocation_data_active,relocations_mapped;
ByteBuffer relocation_buffer;
extern BYTE file_access_statistics;
void __stdcall reserve_byte_buffer(ByteBuffer *,unsigned);
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
DWORD __stdcall read_mips_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_alpha_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_sh_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_arm_relocations(LinkContext *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_mac_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
DWORD __stdcall read_ppc_relocations(LinkContext *,OutputModule *,Contribution *,IMAGE_SYMBOL *,IMAGE_RELOCATION *,DWORD,DWORD) { __asm { int 3 } return 0; }
void __stdcall release_relocations(IMAGE_RELOCATION *relocations)
{
    relocation_data_active=0;
}
}
/* Converted modules carry their physical input offset in optional_data[3].
 * This expression is deliberately re-evaluated after child calls, matching
 * the original module/owner loads rather than caching mutable fields. */
#define CONTRIBUTION_INPUT_OFFSET(c) \
    (((c)->module->library->flags & 16) ? 0 : \
    (((c)->module->flags & 0x40) ? ((DWORD *)(c)->module->optional_data)[3] : \
    (DWORD)(c)->module->identity))
IMAGE_RELOCATION *RECON_THISCALL Contribution::ReadRelocations(DWORD *count)
{
    IMAGE_RELOCATION first;
    IMAGE_RELOCATION *relocations=NULL,*overflow;
    FileRecord *file;
    DWORD n,error;
    relocation_data_active=1;
    n=module==output_module ? 0 : relocation_count;
    if (characteristics & 0x01000000) { /* IMAGE_SCN_LNK_NRELOC_OVFL */
        if (n!=0xffff) link_contribution_fatal(this,1183,n);
        overflow=(IMAGE_RELOCATION *)map_file_range(current_input_file,
            CONTRIBUTION_INPUT_OFFSET(this)+relocation_offset,sizeof(first));
        if (!overflow) {
            error=0;
            seek_file(current_input_file,CONTRIBUTION_INPUT_OFFSET(this)+relocation_offset,
                FILE_BEGIN,&error);
            if (file_access_statistics) record_file_access();
            file=file_slots[current_input_file];
            if (file->flags & 8) mapped_read(file,&first,sizeof(first));
            else buffered_read(file,&first,sizeof(first));
            overflow=&first;
        }
        n=overflow->VirtualAddress;
        if (n<0xffff) link_contribution_fatal(this,1183,n);
    }
    *count=n;
    n*=sizeof(IMAGE_RELOCATION);
    if ((module->library->flags & 16) || !(module->flags & 0x40))
        relocations=(IMAGE_RELOCATION *)map_file_range(current_input_file,
            CONTRIBUTION_INPUT_OFFSET(this)+relocation_offset,n);
    relocations_mapped=relocations!=NULL;
    if (relocations) return relocations;
    reserve_byte_buffer(&relocation_buffer,n);
    relocations=(IMAGE_RELOCATION *)relocation_buffer.data;
    if (!(module->library->flags & 16) && (module->flags & 0x40)) {
        ReadConvertedRelocations(relocations,n);
        return relocations;
    }
    error=0;
    seek_file(current_input_file,CONTRIBUTION_INPUT_OFFSET(this)+relocation_offset,
        FILE_BEGIN,&error);
    if (file_access_statistics) record_file_access();
    file=file_slots[current_input_file];
    if (file->flags & 8) mapped_read(file,relocations,n);
    else buffered_read(file,relocations,n);
    return relocations;
}
typedef void (RECON_THISCALL LibrarianContext::*ConvertedSectionEmitter)(
    LibrarianImportInfo *,void *,void *);
/* Encountered short-import section kinds 0..8. The remaining format-specific
 * table entries are not recovered yet; they stop before dispatch. */
static ConvertedSectionEmitter converted_relocation_emitters[9]={
    NULL,NULL,&LibrarianContext::EmitImportCodeRelocations,
    &LibrarianContext::EmitImportFunctionRelocations,NULL,NULL,
    &LibrarianContext::EmitImportNameRelocation,
    &LibrarianContext::EmitImportNameRelocation,NULL
};
void RECON_THISCALL Contribution::ReadConvertedRelocations(void *buffer,DWORD bytes)
{
    BYTE kind=module->section_flags ? module->section_flags[
        ((LONG)((DWORD)this-(DWORD)module->contributions))>>6] : 255;
    if(kind>=9) { __asm { int 3 } }
    (((LibrarianContext *)module->auxiliary)->*converted_relocation_emitters[kind])(
        (LibrarianImportInfo *)module->optional_data,buffer,(void *)bytes);
}

/* Target-machine validation and relocation callback selection. */
#include "machine.h"
#include "comdat.h"
#include <recon.h>
typedef struct SectionDefault { char *name; unsigned long flags; } SectionDefault;
extern SectionDefault bss_section_defaults;
ApplyRelocations apply_relocations;
SymbolTable *mac_symbol_table;
Contribution *mac_far_bss;
extern unsigned char mac_image, mac_small_model, mapped_debug, machine_initialized;
extern unsigned char reference_option_explicit;
extern char *entry_point;
extern Module *output_module;
extern ComdatNode *comdat_nodes;

void initialize_machine(Image *image)
{
    unsigned long flags;
    switch (image->file.Machine) {
    case IMAGE_FILE_MACHINE_I386:
        apply_relocations=apply_i386_relocations;
        image->options.global_pointer_size=0;
        break;
    case 0x268:
        apply_relocations=apply_m68k_relocations;
        mac_image=1;
        mac_symbol_table=image->symbols;
        image->optional.FileAlignment=image->optional.SectionAlignment=4;
        if (mapped_debug==1) {
            mapped_debug=0;
            warning_message(NULL,0x84,"","");
        }
        if (!reference_option_explicit) image->options.eliminate_unreferenced=0;
        flags=bss_section_defaults.flags|0x100000;
        mac_far_bss=create_contribution(".farbss",0,0,0,0,0,0,flags,flags,0,
            output_module,1,&image->sections,&image->optional);
        mac_far_bss->flags |= 0x2000;
        if (image->options.eliminate_unreferenced)
            mac_far_bss->graph=create_comdat_node(mac_far_bss,NULL,NULL,&comdat_nodes,1);
        if (!entry_point) entry_point=duplicate_string("_mainCRTStartup");
        break;
    default:
        fatal_message(program_name,0x12,"");
        break;
    }
    if (entry_point) select_entry_point(image,1);
    machine_initialized=1;
}

void select_machine(Argument *argument,unsigned short machine,Image *image)
{
    if (!machine) return;
    if (!image->file.Machine) {
        switch (machine) {
        case IMAGE_FILE_MACHINE_I386: apply_relocations=apply_i386_relocations; break;
        case 0x268: apply_relocations=apply_m68k_relocations; break;
        default: fatal_message(argument->text,0x10,machine); break;
        }
        image->file.Machine=machine;
    } else validate_machine(argument->text,machine,image);
}

void validate_machine(const char *file,unsigned short machine,Image *image)
{
    const char *name=NULL;
    if (machine==image->file.Machine) return;
    switch (machine) {
    case IMAGE_FILE_MACHINE_I386: name="i386"; break;
    case 0x14d: name="i860"; break;
    case 0x162:
        if (image->file.Machine==0x166) return;
        name="mips";
        break;
    case 0x166:
        if (image->file.Machine==0x162) { image->file.Machine=machine; return; }
        name="mips";
        break;
    case 0x268: name="M68K"; break;
    default: fatal_message(file,0x10,machine); break;
    }
    fatal_message(file,0xf,name);
}

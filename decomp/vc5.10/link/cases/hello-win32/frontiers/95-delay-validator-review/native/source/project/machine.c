#include <windows.h>
#include "link.h"
#include "input.h"
#include "options.h"
#include "pass-one.h"
#include "sections.h"
#include "full-link-functions.h"
#include <recon.h>

extern BYTE incremental_enabled;
extern const char *entry_point_name;
extern StringNode *entry_symbol;

/* Set by the machine backend when section alignment requires initialized BSS. */
BYTE small_section_alignment;
/* The first encountered consumer reads this command-line list's count. */
InputArguments delay_libraries;

void __stdcall initialize_machine(LinkContext *context)
{
    BYTE incremental_supported=0;
    if (!context->file_header.Machine) {
        context->file_header.Machine=0x14c;
        link_warning(NULL,4068,"IX86");
    }
    validate_delay_load_options(context);
    switch (context->file_header.Machine) {
    case 0x14c: initialize_x86_backend(context,&incremental_supported); break;
    case 0x166: case 0x168:
        initialize_mips_backend(context,&incremental_supported); break;
    case 0x184: initialize_alpha_backend(context,&incremental_supported); break;
    case 0x1a2: case 0x1a6:
        initialize_sh_backend(context,&incremental_supported); break;
    case 0x601: initialize_mac_ppc_backend(context,&incremental_supported); break;
    case 0x1f0: initialize_ppc_backend(context,&incremental_supported); break;
    case 0x1c0: initialize_arm_backend(context,&incremental_supported); break;
    }
    if (incremental_enabled && !incremental_supported) link_fatal(NULL,1115);
    if (selected_tool_kind==1) {
        if (small_section_alignment) {
            bss_section->characteristics &= ~IMAGE_SCN_CNT_UNINITIALIZED_DATA;
            bss_section->characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
            bss_section->original_characteristics &= ~IMAGE_SCN_CNT_UNINITIALIZED_DATA;
            bss_section->original_characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
        }
        if (selected_tool_kind==1 && !entry_symbol && entry_point_name)
            resolve_initial_entry(context);
    }
    machine_initialized=1;
}

void __stdcall validate_delay_load_options(LinkContext *context)
{
    const char *incompatible;
    if (!delay_libraries.count) return;
    if (context->file_header.Machine!=0x14c && context->file_header.Machine!=0x184) {
        clear_argument_list(&delay_libraries);
        link_warning(NULL,4062,"DELAYLOAD",get_machine_name(context,context->file_header.Machine));
        return;
    }
    if (context->options.retain_relocations || context->options.rom ||
        context->is_vxd==1 || context->optional_header.Subsystem==7) {
        clear_argument_list(&delay_libraries);
        if (context->options.retain_relocations) incompatible="DRIVER";
        else if (context->options.rom) incompatible="ROM";
        else if (context->is_vxd==1) incompatible="VXD";
        else incompatible="SUBSYSTEM:POSIX";
        link_warning(NULL,4075,"DELAYLOAD",incompatible);
    }
}
const char *__stdcall get_machine_name(LinkContext *context,unsigned machine) { __asm { int 3 } return NULL; }
void __stdcall initialize_x86_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_mips_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_alpha_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_sh_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_mac_ppc_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_ppc_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }
void __stdcall initialize_arm_backend(LinkContext *context,BYTE *incremental_supported) { __asm { int 3 } }

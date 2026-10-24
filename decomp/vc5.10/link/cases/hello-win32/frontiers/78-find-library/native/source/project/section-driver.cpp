/* This method's genuine ECX receiver requires the original C++ ABI. */
#include <windows.h>
#include "link-driver.h"
#include "sections.h"
#include <recon.h>

void RECON_THISCALL LinkContext::InitializeSections(void)
{
    SectionList *list=&sections;
    IMAGE_OPTIONAL_HEADER *header=&optional_header;
    bss_section=create_output_section(NULL,".bss",0xc0000080,list,header);
    small_data_section=create_output_section(NULL,".sdata",0xc0000040,list,header);
    read_only_section=create_output_section(NULL,".rdata",0x40000040,list,header);
    data_section=create_output_section(NULL,".data",0xc0000040,list,header);
    procedure_section=create_output_section(NULL,".pdata",0x40000040,list,header);
    import_section=create_output_section(NULL,".idata",0xc0300040,list,header);
    import_thunk_section=import_section;
    import_descriptor_section=import_section;
    delay_import_section=create_output_section(NULL,".didat",0xc0300040,list,header);
    export_section=create_output_section(NULL,".edata",0x40000040,list,header);
    export_group=create_contribution_group(".edata",export_section);
    exception_section=create_output_section(NULL,".xdata",0x40000040,list,header);
    architecture_section=create_output_section(NULL,".arch",0x42400040,list,header);
    ppc_loader_section=create_output_section(NULL,".ppcldr",0x42000040,list,header);
    debug_section=create_output_section(NULL,".debug",0x42100040,list,header);
    debug_symbol_group=create_contribution_group(".debug$S",debug_section);
    debug_type_group=create_contribution_group(".debug$T",debug_section);
    debug_precompiled_group=create_contribution_group(".debug$P",debug_section);
    debug_fixup_group=create_contribution_group(".debug$F",debug_section);
}

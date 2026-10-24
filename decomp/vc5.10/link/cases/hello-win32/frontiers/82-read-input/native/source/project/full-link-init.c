#include <windows.h>
#include <string.h>
#include "arguments.h"
#include "full-link-functions.h"
#include "memory.h"
#include <recon.h>

extern BYTE incremental_enabled;
MemoryArena *output_module_arena, *output_library_arena;

void __stdcall initialize_output_modules(OutputModule **result)
{
    OutputModule *module;
    OutputLibrary *library;
    if (incremental_enabled) {
        module=(OutputModule *)allocate_link_memory(1,sizeof(OutputModule));
        library=(OutputLibrary *)allocate_link_memory(1,sizeof(OutputLibrary));
    } else {
        module=(OutputModule *)reserve_memory(&output_module_arena,sizeof(OutputModule));
        library=(OutputLibrary *)reserve_memory(&output_library_arena,sizeof(OutputLibrary));
    }
    module->library=library;
    module->name=duplicate_link_string(" linker defined module");
    library->name=duplicate_link_string(" linker defined library");
    *result=module;
}
#pragma intrinsic(strlen,memcpy)
char *__stdcall duplicate_link_string(const char *text)
{
    char *copy;
    unsigned bytes;
    if (!incremental_enabled) return duplicate_argument(text);
    bytes=strlen(text)+1;
    copy=(char *)allocate_incremental(bytes);
    memcpy(copy,text,strlen(text)+1);
    return copy;
}

/* The module reader fills one eight-byte map entry and one COFF section
   header per section. These reusable buffers initially hold 500 sections. */
BYTE *module_section_map;
IMAGE_SECTION_HEADER *module_section_headers;
unsigned module_section_capacity;
BYTE module_section_pools_ready;
typedef char ModuleSectionHeaderSize[(sizeof(IMAGE_SECTION_HEADER)==40)?1:-1];
void __cdecl initialize_contribution_pools(void)
{
    module_section_map=(BYTE *)allocate_virtual_memory(NULL,500*8,
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    module_section_headers=(IMAGE_SECTION_HEADER *)allocate_virtual_memory(NULL,
        500*sizeof(IMAGE_SECTION_HEADER),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    module_section_capacity=500;
    module_section_pools_ready=1;
}

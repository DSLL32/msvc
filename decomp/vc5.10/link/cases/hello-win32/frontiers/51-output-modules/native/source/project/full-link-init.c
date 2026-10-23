#include <windows.h>
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
char *__stdcall duplicate_link_string(const char *text)
{
    __asm { int 3 }
    return NULL;
}

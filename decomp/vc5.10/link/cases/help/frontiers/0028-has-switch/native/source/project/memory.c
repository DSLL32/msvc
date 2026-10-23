#include <windows.h>
#include "link.h"
#include <recon.h>

DWORD allocation_page_size, maximum_physical_memory;

void __cdecl initialize_memory(void)
{
    SYSTEM_INFO system;
    MEMORYSTATUS memory;
    if (!allocation_page_size) {
        GetSystemInfo(&system);
        allocation_page_size = system.dwPageSize;
    }
    GlobalMemoryStatus(&memory);
    maximum_physical_memory = 0x04000000;
    if (memory.dwTotalPhys >= 0x04000000)
        maximum_physical_memory = memory.dwTotalPhys;
}

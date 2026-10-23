#include <windows.h>
#include <stdlib.h>
#include "files.h"
#include "link.h"
#include <recon.h>

FileRecord **file_slots, *file_newest;
unsigned file_record_count, file_window_limit, file_statistics[15];
unsigned char file_mapping_enabled;

void __stdcall initialize_file_cache(unsigned mapped_slots, unsigned mapped_limit,
                                    unsigned ordinary_slots, unsigned ordinary_limit)
{
    HMODULE kernel;
    unsigned count, limit, i;
    FileRecord *record;
    kernel = GetModuleHandleA("kernel32.dll");
    file_mapping_enabled = !(kernel && GetProcAddress(kernel, "IsTNT"));
    if (get_debug_flag(44)) file_mapping_enabled = 0;
    if (file_mapping_enabled) {
        count = mapped_slots;
        limit = mapped_limit;
    } else {
        count = ordinary_slots;
        limit = ordinary_limit;
    }
    file_slots = (FileRecord **)zero_allocate(count * sizeof(FileRecord *));
    for (i = 0; i < count; ++i) {
        record = new_file_record();
        record->index = i;
        if (file_newest) record->previous = file_newest;
        file_newest = record;
        file_slots[i] = record;
    }
    file_window_limit = limit;
    for (i = 0; i < 15; ++i) file_statistics[i] = 0;
}
FileRecord *__cdecl new_file_record(void) { __asm { int 3 } return NULL; }
void *__stdcall zero_allocate(unsigned bytes)
{
    void *allocation;
    allocation = calloc(1, bytes);
    if (!allocation) out_of_memory();
    return allocation;
}
void __cdecl out_of_memory(void) { __asm { int 3 } }

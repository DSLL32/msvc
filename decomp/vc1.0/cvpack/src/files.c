/* Cached page/file pools. Callback installation intentionally follows allocation,
   as in the original; allocation failures before that point have no handler. */
#include "files.h"
#include <recon.h>

Page *free_pages;
FileSlot *free_files;
FileSlot **file_slots;
int file_count, page_count, file_limit, open_limit, page_limit;
unsigned char mapped_files;
void (*file_failure)(int,const char *,const char *);
CreateMapping mapping_create;
MapFileView mapping_map;
UnmapFileView mapping_unmap;

void files_initialize(int pages, int mapped_limit, int mapped_open,
                      int normal_limit, int normal_open, unsigned char mapped,
                      void (*error)(int,const char *,const char *))
{
    HMODULE kernel;
    int limit, opens, i;
    Page *page;
    FileSlot *slot;
    if (mapped) {
        kernel = GetModuleHandleA("kernel32.dll");
        if (kernel && GetProcAddress(kernel, "IsTNT")) mapped = 0;
        if (mapped) {
            kernel = LoadLibraryA("kernel32.dll");
            if (!kernel) mapped = 0;
            else {
                mapping_create = (CreateMapping)GetProcAddress(kernel, "CreateFileMappingA");
                mapping_map = (MapFileView)GetProcAddress(kernel, "MapViewOfFile");
                mapping_unmap = (UnmapFileView)GetProcAddress(kernel, "UnmapViewOfFile");
                if (!mapping_create || !mapping_map || !mapping_unmap) mapped = 0;
            }
        }
    }
    limit = (unsigned short)(mapped ? mapped_limit : normal_limit);
    opens = (unsigned short)(mapped ? mapped_open : normal_open);
    file_limit = limit;
    open_limit = opens;
    page_count = pages;
    for (i = pages; i > 0; --i) {
        page = new_page();
        if (free_pages) page->next = free_pages;
        free_pages = page;
    }
    file_slots = calloc(limit, sizeof(*file_slots));
    if (!file_slots) file_failure(13, NULL, NULL);
    for (i = 0; i < limit; ++i) {
        slot = new_file_slot();
        slot->handle = i;
        if (free_files) slot->next = free_files;
        free_files = slot;
        file_slots[i] = slot;
    }
    page_limit = opens;
    file_failure = error;
    mapped_files = mapped;
}

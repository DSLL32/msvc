/* Cached page/file pools. Callback installation intentionally follows allocation,
   as in the original; allocation failures before that point have no handler. */
#include <stdarg.h>
#include <sys/stat.h>
#include "files.h"
#include <recon.h>

Page *free_pages;
FileSlot *free_files;
FileSlot **file_slots;
FileSlot *active_files, *closed_files, *closed_files_tail;
int closed_file_count;
int file_count, page_count, file_limit, open_limit, page_limit;
unsigned char mapped_files;
void (*file_failure)(int,const char *,const char *);
CreateMapping mapping_create;
MapFileView mapping_map;
UnmapFileView mapping_unmap;

Page *new_page(void)
{
    Page *page = calloc(1, sizeof(*page));
    if (!page) file_failure(13, NULL, NULL);
    page->handle = -1;
    return page;
}

FileSlot *new_file_slot(void)
{
    FileSlot *slot = calloc(1, sizeof(*slot));
    if (!slot) file_failure(13, NULL, NULL);
    ++file_count;
    return slot;
}

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

int open_file(const char *name, int mode, ...)
{
    va_list arguments;
    int permission = 0, result = -1;
    unsigned char created = 0;
    FileSlot *slot;
    /* The original reads the optional word for any nonzero mode, even when
       no create bit is set; only the create path actually consumes it. */
    if (mode) {
        va_start(arguments, mode);
        permission = va_arg(arguments, int);
        va_end(arguments);
    }
    slot = find_cached_file(name);
    if (!slot) {
        slot = acquire_file_slot();
        created = 1;
        slot->name = _strdup(name);
        if (!slot->name) file_failure(13, NULL, NULL);
        if (!(mode & 15) || (mode & 2)) slot->flags |= 1;
        if ((mode & 1) || (mode & 2)) slot->flags |= 2;
        if (mode & 0x100) slot->flags |= 16;
    }
    if (mapped_files) result = open_mapped_file(slot, mode, created);
    if (result == -1) open_buffered_file(slot, mode, permission, created);
    return slot->handle;
}

FileSlot *find_cached_file(const char *name)
{
    FileSlot *slot;
    const char *left, *right;
    for (slot = closed_files; slot; slot = slot->next) {
        left = slot->name;
        right = name;
        while (*left == *right) {
            if (!*left) break;
            ++left;
            ++right;
        }
        if (*left == *right) {
            --closed_file_count;
            move_file_slot(slot, &closed_files, &closed_files_tail, &active_files, NULL, 0);
            slot->flags &= ~4UL;
            return slot;
        }
    }
    return NULL;
}

FileSlot *acquire_file_slot(void)
{
    FileSlot *slot;
    if (!free_files) {
        slot = closed_files;
        --closed_file_count;
        move_file_slot(slot, &closed_files, &closed_files_tail, &free_files, NULL, 0);
        release_file_slot(slot);
    }
    slot = free_files;
    move_file_slot(slot, &free_files, NULL, &active_files, NULL, 0);
    return slot;
}

void move_file_slot(FileSlot *slot, FileSlot **from, FileSlot **from_tail,
                    FileSlot **to, FileSlot **to_tail, unsigned char append)
{
    FileSlot *previous, *current;
    previous = *from;
    if (slot == previous) {
        *from = previous->next;
        if (from_tail && *from_tail == slot) *from_tail = NULL;
    } else {
        current = previous->next;
        while (current && current != slot) {
            previous = current;
            current = current->next;
        }
        if (current) previous->next = current->next;
        if (from_tail && *from_tail == slot) *from_tail = previous;
    }
    if (!*to) {
        *to = slot;
        slot->next = NULL;
        if (to_tail) *to_tail = slot;
    } else if (append) {
        slot->next = NULL;
        (*to_tail)->next = slot;
        *to_tail = slot;
    } else {
        slot->next = *to;
        *to = slot;
    }
}

int open_buffered_file(FileSlot *slot, int mode, int permission, unsigned char created)
{
    struct _stat status;
    int entries, i;
    if (created) {
        slot->source.descriptor = _open(slot->name, mode, permission);
        if (slot->source.descriptor == -1) file_failure(15, slot->name, NULL);
        slot->length = 0;
        if ((slot->flags & 2) && (!(slot->flags & 3) || (slot->flags & 16)))
            slot->contents.capacity = 0x400000UL;
        else {
            if (_stat(slot->name, &status) == -1) file_failure(15, slot->name, NULL);
            slot->contents.capacity = status.st_size;
            slot->length = status.st_size;
        }
        entries = (slot->contents.capacity + 4096) >> 12;
        slot->allocated = (unsigned long)entries << 12;
        slot->storage.pages = calloc(1, entries * sizeof(*slot->storage.pages));
        if (!slot->storage.pages) file_failure(13, NULL, NULL);
        if (!(slot->flags & 16) && (slot->flags & 1))
            for (i = 0; i < entries; ++i) slot->storage.pages[i] = (Page *)1;
    }
    seek_buffered_file(slot, 0, 0);
    return 0;
}

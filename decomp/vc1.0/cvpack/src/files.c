/* Cached page/file pools. Callback installation intentionally follows allocation,
   as in the original; allocation failures before that point have no handler. */
#include <stdarg.h>
#include <sys/stat.h>
#include "files.h"
#include "image.h"
#include <recon.h>

Page *free_pages, *used_pages, *newest_page, *oldest_page;
FileSlot *free_files;
FileSlot **file_slots;
FileSlot *active_files, *closed_files, *closed_files_tail;
int closed_file_count;
unsigned long page_reads;
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

long seek_buffered_file(FileSlot *slot, long offset, int origin)
{
    long position;
    Page *cached, *page;
    if (origin == 0) position = offset;
    else if (origin == 1) position = offset + slot->cursor.page->position;
    else position = offset + slot->length;
    while (slot->allocated < position) grow_page_table(slot);
    cached = slot->storage.pages[position >> 12];
    if (slot->cursor.page) {
        slot->cursor.page->flags &= ~16UL;
        slot->cursor.page = NULL;
    }
    if ((unsigned long)cached <= 1) {
        slot->cursor.page = allocate_file_page(slot, position);
        slot->cursor.page->flags |= 16;
        if (cached == (Page *)1) {
            read_file_page(slot);
            ++page_reads;
            slot->cursor.page->flags |= 4;
        }
    } else {
        slot->cursor.page = cached;
        cached->flags |= 16;
    }
    page = slot->cursor.page;
    page->position = position;
    page->cursor = page->data + position - page->start;
    if ((slot->flags & 2) && page->dirty_end < page->position)
        page->dirty_end = page->position;
    touch_page(page);
    return position;
}

Page *allocate_file_page(FileSlot *slot, long position)
{
    Page *page = free_pages;
    unsigned long *words;
    int i;
    if (!page) {
        page = remove_lru_page();
        release_page(page);
    }
    page->start = (position / 4096) * 4096;
    page->limit = page->start + 4096;
    page->position = position;
    page->dirty_end = page->start;
    page->cursor = page->data + position - page->start;
    if (slot->flags & 2) {
        words = (unsigned long *)page->data;
        for (i = 0; i < 1024; ++i) words[i] = 0;
    }
    if (!(slot->flags & 2)) page->flags |= 8;
    attach_page(slot, page);
    return page;
}

unsigned char attach_page(FileSlot *slot, Page *page)
{
    if (!free_pages) return 0;
    free_pages = page->next;
    page->handle = slot->handle;
    page->next = used_pages;
    used_pages = page;
    page->flags |= 1;
    slot->storage.pages[page->start >> 12] = page;
    return 1;
}

void read_file_page(FileSlot *slot)
{
    long count;
    if (_lseek(slot->source.descriptor, slot->cursor.page->start, 0) == -1)
        file_failure(17, NULL, NULL);
    count = slot->length - slot->cursor.page->start;
    if (count >= 4096) count = 4096;
    if (_read(slot->source.descriptor, slot->cursor.page->data, count) != count)
        file_failure(17, NULL, NULL);
    slot->cursor.page->flags &= ~8UL;
}

void touch_page(Page *page)
{
    Page *current;
    for (current = newest_page; current && current != page; current = current->older)
        ;
    if (current) {
        if (page->newer) page->newer->older = page->older;
        else newest_page = page->older;
        if (page->older) page->older->newer = page->newer;
        else oldest_page = page->newer;
    }
    if (!newest_page && !oldest_page) {
        newest_page = oldest_page = page;
        page->older = page->newer = NULL;
        return;
    }
    page->older = newest_page;
    page->newer = NULL;
    newest_page->newer = page;
    newest_page = page;
}

long seek_file(int handle, long offset, int origin)
{
    FileSlot *slot = file_slots[handle];
    if (slot->flags & 8) return seek_mapped_file(slot, offset, origin);
    return seek_buffered_file(slot, offset, origin);
}

int read_file(int handle, void *buffer, unsigned long size)
{
    FileSlot *slot = file_slots[handle];
    if (slot->flags & 8) return read_mapped_file(slot, buffer, size);
    return read_buffered_file(slot, buffer, size);
}

int read_buffered_file(FileSlot *slot, void *buffer, unsigned long size)
{
    unsigned long remaining;
    long copied;
    unsigned long i;
    unsigned char *destination = buffer;
    if (slot->cursor.page->position+size > slot->contents.capacity)
        size = slot->contents.capacity-slot->cursor.page->position;
    remaining = size;
    while (remaining) {
        if (slot->cursor.page->flags & 8) read_file_page(slot);
        copied = slot->cursor.page->limit-slot->cursor.page->position;
        if (copied >= (long)remaining) copied = remaining;
        for (i = 0; i < (unsigned long)copied; ++i)
            destination[i] = slot->cursor.page->cursor[i];
        slot->cursor.page->position += copied;
        slot->cursor.page->cursor += copied;
        if (slot->cursor.page->position == slot->cursor.page->limit)
            advance_file_page(slot);
        remaining -= copied;
        destination += copied;
    }
    return size;
}

long tell_file(int handle)
{
    FileSlot *slot = file_slots[handle];
    if (slot->flags & 8) return slot->cursor.position;
    return slot->cursor.page->position;
}

void advance_file_page(FileSlot *slot)
{
    Page **next = &slot->storage.pages[(slot->cursor.page->start >> 12)+1];
    Page *cached = *next;
    Page *page;
    if (cached && cached != (Page *)1) {
        slot->cursor.page->flags &= ~16UL;
        release_page(slot->cursor.page);
        slot->cursor.page = *next;
        slot->cursor.page->flags |= 16;
    } else {
        flush_file_page(slot->cursor.page);
        *next = slot->cursor.page;
        page = slot->cursor.page;
        page->start += 4096;
        page->limit += 4096;
        page->dirty_end = page->start;
        if (!(slot->flags & 2)) page->flags |= 8;
        if (cached == (Page *)1) {
            read_file_page(slot);
            ++page_reads;
            slot->cursor.page->flags |= 4;
        }
    }
    page = slot->cursor.page;
    page->position = page->start;
    page->cursor = page->data;
}

void flush_file_page(Page *page)
{
    FileSlot *slot = file_slots[page->handle];
    long count;
    if (page->flags & 2) {
        count = page->dirty_end-page->start;
        if (_lseek(slot->source.descriptor, page->start, 0) == -1)
            file_failure(20, NULL, NULL);
        if (_write(slot->source.descriptor, page->data, count) != count)
            file_failure(20, NULL, NULL);
        slot->storage.pages[page->start >> 12] = (Page *)1;
        if ((slot->flags & 2) && slot->length < page->dirty_end)
            slot->length = page->dirty_end;
    } else if (page->flags & 4) {
        slot->storage.pages[page->start >> 12] = (Page *)1;
    } else slot->storage.pages[page->start >> 12] = NULL;
}

unsigned char release_page(Page *page)
{
    Page *previous, *next;
    flush_file_page(page);
    page->flags = 0;
    page->handle = -1;
    if (used_pages == page) used_pages = used_pages->next;
    else {
        previous = used_pages;
        next = previous->next;
        while (next) {
            if (next == page) {
                previous->next = next->next;
                break;
            }
            previous = next;
            next = next->next;
        }
    }
    page->next = free_pages;
    free_pages = page;
    return 1;
}

int write_file(int handle, const void *buffer, unsigned long size)
{
    FileSlot *slot = file_slots[handle];
    if (slot->flags & 8) return write_mapped_file(slot, buffer, size);
    return write_buffered_file(slot, buffer, size);
}

int write_buffered_file(FileSlot *slot, const void *buffer, unsigned long size)
{
    const unsigned char *source = buffer;
    unsigned long remaining, i;
    long copied;
    Page *page;

    if (!size) return size;
    while ((long)(slot->cursor.page->position+size) >= slot->allocated)
        grow_page_table(slot);
    remaining = size;
    slot->cursor.page->flags |= 2;
    while (remaining) {
        page = slot->cursor.page;
        copied = page->limit-page->position;
        if (copied >= (long)remaining) copied = remaining;
        for (i = 0; i < (unsigned long)copied; ++i) page->cursor[i] = source[i];
        slot->cursor.page->position += copied;
        slot->cursor.page->cursor += copied;
        page = slot->cursor.page;
        if (page->dirty_end < page->position) page->dirty_end = page->position;
        if (page->limit == page->position) advance_file_page(slot);
        /* Test precedes subtraction in the original, including the last page. */
        if (remaining) slot->cursor.page->flags |= 2;
        remaining -= copied;
        source += copied;
    }
    return size;
}

int truncate_file(int handle, long size)
{
    FileSlot *slot = file_slots[handle];
    if (slot->length == size) return 0;
    if (slot->flags & 8) return truncate_mapped_file(slot, size);
    return truncate_buffered_file(slot, size);
}

int truncate_buffered_file(FileSlot *slot, long size)
{
    long index, count;
    Page *page;
    if (slot->length >= size) {
        count = slot->allocated >> 12;
        if (slot->cursor.page->position > size) seek_buffered_file(slot, 0, 2);
        index = size >> 12;
        page = slot->storage.pages[index];
        if (page && page != (Page *)1) page->dirty_end = size;
        for (++index; index < count; ++index) {
            page = slot->storage.pages[index];
            if (page && page != (Page *)1) release_page(page);
        }
    } else {
        while (slot->allocated < size) grow_page_table(slot);
    }
    slot->contents.capacity = size;
    slot->length = size;
    return _chsize(slot->source.descriptor, size);
}

int close_file(int handle, unsigned char release)
{
    FileSlot *slot = file_slots[handle];
    if (!release && !(slot->flags & 2)) {
        ++closed_file_count;
        move_file_slot(slot, &active_files, NULL, &closed_files, &closed_files_tail, 1);
        cache_closed_file(slot);
        return 0;
    }
    move_file_slot(slot, &active_files, NULL, &free_files, NULL, 0);
    if (slot->flags & 8) return close_mapped_file(slot);
    return release_file_slot(slot);
}

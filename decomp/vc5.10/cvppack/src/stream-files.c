/* Unmapped descriptor/page backend. Original [00401510,00401671). */
#include <sys/stat.h>
#include "files.h"
#include <recon.h>

int open_stream_file(CachedFile *file, int flags, int mode, int created)
{
    struct _stat status;
    CachedFile *previous, *entry;
    int count, index;
    if (created) {
        file->descriptor = _sopen(file->name, flags, (flags & 3) ? 0x10 : 0x20, mode);
        if (file->descriptor == -1) {
            if (active_files == file) {
                active_files = file->next;
                --file_allocations;
            } else {
                previous = active_files;
                while (previous) {
                    entry = previous->next;
                    if (entry == file) {
                        previous->next = file->next;
                        --file_allocations;
                        break;
                    }
                    previous = entry;
                }
            }
            fatal_error(1003, file->name, NULL);
        }
        file->length = 0;
        if ((file->flags & 2) && (!(file->flags & 3) || (file->flags & 0x10))) {
            file->capacity = 0x400000;
        } else {
            if (_stat(file->name, &status) == -1) fatal_error(1003, file->name, NULL);
            if (!status.st_size) {
                file->pages = allocate_zero(4);
                fatal_error(1005, NULL, NULL);
            }
            file->length = file->capacity = status.st_size;
        }
        count = ((unsigned)file->capacity + 4096) >> 12;
        file->page_capacity = count << 12;
        file->pages = allocate_zero(count * sizeof(CachedPage *));
        if ((file->flags & 1) && !(file->flags & 0x10))
            for (index = 0; index < count; ++index) file->pages[index] = (CachedPage *)1;
    }
    seek_stream_file(file, 0, 0);
    return 0;
}

/* Stream seek selects a pinned cached page [00401810,00401909). */
int page_reloads;
int seek_stream_file(CachedFile *file, int offset, int origin)
{
    int position;
    CachedPage *saved;
    switch (origin) {
    case 0: position = offset; break;
    case 1: position = file->current_page->position + offset; break;
    default: position = file->length + offset; break;
    }
    while (position >= file->page_capacity) grow_page_table(file);
    saved = file->pages[position >> 12];
    if (file->current_page) {
        file->current_page->flags &= ~0x10;
        file->current_page = NULL;
    }
    if ((unsigned)saved <= 1) {
        file->current_page = obtain_page(file, position);
        file->current_page->flags |= 0x10;
        if (saved == (CachedPage *)1) {
            read_page(file);
            ++page_reloads;
            file->current_page->flags |= 4;
        }
    } else {
        file->current_page = saved;
        saved->flags |= 0x10;
    }
    file->current_page->position = position;
    file->current_page->cursor = file->current_page->buffer + position - file->current_page->start;
    if ((file->flags & 2) && position > file->current_page->initialized_end)
        file->current_page->initialized_end = position;
    touch_page(file->current_page);
    return position;
}

/* Obtain a page from the free pool or LRU [00401f30,00401fb3). */
CachedPage *newest_page, *oldest_page, *active_pages;
CachedPage *obtain_page(CachedFile *file, int position)
{
    CachedPage *page = free_pages;
    unsigned index;
    if (!page) {
        page = evict_page();
        release_page(page);
    }
    page->position = position;
    page->start = (position / 4096) * 4096;
    page->end = page->start + 4096;
    page->initialized_end = page->start;
    page->cursor = page->buffer + position - page->start;
    if (file->flags & 2) {
        for (index = 0; index < sizeof(page->buffer); ++index) page->buffer[index] = 0;
    }
    if (!(file->flags & 2)) page->flags |= 8;
    link_page(file, page);
    return page;
}

/* Move a free page into the file and active pool [00402040,00402091). */
int link_page(CachedFile *file, CachedPage *page)
{
    if (!free_pages) return 0;
    free_pages = page->next;
    page->file = file->index;
    page->next = active_pages;
    active_pages = page;
    page->flags |= 1;
    file->pages[page->start >> 12] = page;
    return 1;
}

/* Fill only the file-backed prefix of the current page [00401aa0,00401b20). */
void read_page(CachedFile *file)
{
    int count;
    if (_lseek(file->descriptor, file->current_page->start, 0) == -1)
        fatal_error(1005, NULL, NULL);
    count = file->length - file->current_page->start;
    if (count > 4096) count = 4096;
    if (_read(file->descriptor, file->current_page->buffer, count) != count)
        fatal_error(1005, NULL, NULL);
    file->current_page->flags &= ~8;
}

/* Move a page to the newest end of the LRU [00401de0,00401e98). */
void touch_page(CachedPage *page)
{
    CachedPage *entry = newest_page;
    while (entry && entry != page) entry = entry->older;
    if (entry) {
        if (page->newer) page->newer->older = page->older;
        else newest_page = page->older;
        if (page->older) page->older->newer = page->newer;
        else oldest_page = page->newer;
    }
    if (!newest_page && !oldest_page) {
        newest_page = oldest_page = page;
        page->older = page->newer = NULL;
    } else {
        page->older = newest_page;
        page->newer = NULL;
        newest_page->newer = page;
        newest_page = page;
    }
}

/* Read through the page cursor [004019b0,00401a7d). The original bounds against
 * page capacity, not logical EOF; the DIRECT case never requests EOF overread. */
unsigned stream_read(CachedFile *file, void *buffer, unsigned size)
{
    unsigned remaining, index;
    int count, room;
    unsigned char *destination = buffer;
    if (file->current_page->position + size > (unsigned)file->page_capacity)
        size = file->capacity - file->current_page->position;
    remaining = size;
    while (remaining) {
        if (file->current_page->flags & 8) read_page(file);
        room = file->current_page->end - file->current_page->position;
        count = (int)remaining;
        if (count >= room) count = room;
        for (index = 0; index < (unsigned)count; ++index)
            destination[index] = file->current_page->cursor[index];
        file->current_page->position += count;
        file->current_page->cursor += count;
        if (file->current_page->position == file->current_page->end) advance_page(file);
        remaining -= count;
        destination += count;
    }
    return size;
}

/* Write buffered page fragments [00401cf0,00401dd5). */
unsigned stream_write(CachedFile *file, const void *buffer, unsigned size)
{
    unsigned remaining, index;
    int count, room;
    const unsigned char *source = buffer;
    if (!size) return 0;
    while (file->current_page->position + (int)size >= file->page_capacity)
        grow_page_table(file);
    remaining = size;
    file->current_page->flags |= 2;
    do {
        room = file->current_page->end - file->current_page->position;
        count = (int)remaining;
        if (count >= room) count = room;
        for (index = 0; index < (unsigned)count; ++index)
            file->current_page->cursor[index] = source[index];
        file->current_page->position += count;
        file->current_page->cursor += count;
        if (file->current_page->position > file->current_page->initialized_end)
            file->current_page->initialized_end = file->current_page->position;
        if (file->current_page->position == file->current_page->end) advance_page(file);
        if (remaining) file->current_page->flags |= 2;
        remaining -= count;
        source += count;
    } while (remaining);
    return size;
}

/* Advance a stream cursor, reusing its page when possible [00401b20,00401be8). */
void advance_page(CachedFile *file)
{
    int index = file->current_page->start >> 12;
    CachedPage *next = file->pages[index + 1];
    if (next && next != (CachedPage *)1) {
        file->current_page->flags &= ~0x10;
        release_page(file->current_page);
        file->current_page = file->pages[index + 1];
        file->current_page->flags |= 0x10;
    } else {
        flush_page(file->current_page);
        file->pages[index + 1] = file->current_page;
        file->current_page->start += 4096;
        file->current_page->end += 4096;
        file->current_page->initialized_end = file->current_page->start;
        if (!(file->flags & 2)) file->current_page->flags |= 8;
        if (next == (CachedPage *)1) {
            read_page(file);
            ++page_reloads;
            file->current_page->flags |= 4;
        }
    }
    file->current_page->position = file->current_page->start;
    file->current_page->cursor = file->current_page->buffer;
}

/* Persist the initialized dirty prefix and leave a disk-backed table marker.
 * Original [00401bf0,00401cb2). Clean unwritten pages leave a NULL marker. */
void flush_page(CachedPage *page)
{
    CachedFile *file = cached_files[page->file];
    int count;
    if (page->flags & 2) {
        count = page->initialized_end - page->start;
        if (_lseek(file->descriptor, page->start, 0) == -1)
            fatal_error(1008, NULL, NULL);
        if (_write(file->descriptor, page->buffer, count) != count)
            fatal_error(1008, NULL, NULL);
        file->pages[page->start >> 12] = (CachedPage *)1;
        if ((file->flags & 2) && page->initialized_end > file->length)
            file->length = page->initialized_end;
    } else if (page->flags & 4) {
        file->pages[page->start >> 12] = (CachedPage *)1;
    } else {
        file->pages[page->start >> 12] = NULL;
    }
}

/* Flush/recycle this descriptor's pages, close it and release its arrays.
 * Original [00401710,0040178f). Recycled owners retain stale fields until prepare. */
int release_cached_file(CachedFile *file)
{
    CachedPage *page = active_pages, *next;
    int result;
    while (page) {
        next = page->next;
        if (page->file == file->index) {
            if (page == file->current_page) page->flags &= ~0x10;
            release_page(page);
        }
        page = next;
    }
    result = _close(file->descriptor);
    if (result == -1) fatal_error(1008, NULL, NULL);
    free_bytes(file->name);
    free_bytes(file->pages);
    return result;
}

/* Return a page to the pool without changing its LRU links [00401ea0,00401f0c). */
int release_page(CachedPage *page)
{
    CachedPage *previous, *entry;
    flush_page(page);
    page->flags = 0;
    page->file = -1;
    previous = active_pages;
    entry = previous->next;
    if (previous == page) active_pages = entry;
    else {
        while (entry) {
            if (entry == page) {
                previous->next = entry->next;
                break;
            }
            previous = entry;
            entry = entry->next;
        }
    }
    page->next = free_pages;
    free_pages = page;
    return 1;
}

/* Retain an unpinned read-only descriptor, evicting at the close threshold.
 * Original [00401790,0040180e). */
int mark_closed_file(CachedFile *file)
{
    CachedFile *oldest;
    file->flags |= 4;
    if (!(file->flags & 8)) {
        file->current_page->flags &= ~0x10;
        file->current_page = NULL;
    }
    if (closed_file_count == page_increment) {
        oldest = closed_files;
        --closed_file_count;
        move_cached_file(oldest, &closed_files, &last_closed_file, &free_files, NULL, 0);
        if (oldest->flags & 8) return close_mapped_file(oldest);
        return release_cached_file(oldest);
    }
    return closed_file_count;
}

/* Remove the oldest unpinned page from the LRU [00401fc0,0040203e).
 * As in the original, exhausting the list with every page pinned is unsupported. */
CachedPage *evict_page(void)
{
    CachedPage *page = oldest_page;
    while (page && (page->flags & 0x10)) page = page->newer;
    if (page == oldest_page) oldest_page = page->newer;
    else page->older->newer = page->newer;
    if (page == newest_page) newest_page = page->older;
    else page->newer->older = page->older;
    page->older = page->newer = NULL;
    return page;
}

/* Resize the physical stream and release pages beyond the new end.
 * Original [00401240,004012ad). The page pointer table keeps its capacity. */
int resize_stream_file(CachedFile *file, int size)
{
    int index, limit;
    CachedPage *page;
    if (size > file->length) {
        while (file->page_capacity <= size) grow_page_table(file);
    } else {
        limit = file->page_capacity >> 12;
        for (index = size >> 12; index < limit; ++index) {
            page = file->pages[index];
            if (page && page != (CachedPage *)1) release_page(page);
        }
    }
    file->capacity = file->length = size;
    return _chsize(file->descriptor, size);
}

/* Original file-cache initialization [004010a0,0040118b). */
#include "files.h"
#include <recon.h>

int maximum_files, file_increment, page_increment, maximum_pages;
int file_allocations, mapped_files;
CachedPage *free_pages;
CachedFile *free_files, *closed_files, *last_closed_file, *active_files;
int closed_file_count;
CachedFile **cached_files;
CachedFile *mapped_file_slots[16];

void files_initialize(int pages, int files, int page_shift, int file_pages,
                      int file_shift, int mapped)
{
    int index, limit, increment;
    HMODULE kernel;
    CachedPage *page;
    CachedFile *file;
    if (mapped) {
        kernel = GetModuleHandleA("kernel32.dll");
        if (kernel && GetProcAddress(kernel, "IsTNT")) mapped = 0;
    }
    limit = (mapped ? files : file_pages) & 0xffff;
    increment = (mapped ? page_shift : file_shift) & 0xffff;
    maximum_files = limit;
    file_increment = increment;
    maximum_pages = pages;
    for (index = pages; index > 0; --index) {
        page = allocate_page();
        if (free_pages) page->next = free_pages;
        free_pages = page;
    }
    cached_files = allocate_zero(limit * sizeof(CachedFile *));
    for (index = 0; index < limit; ++index) {
        file = allocate_file();
        file->index = index;
        if (free_files) file->next = free_files;
        free_files = file;
        cached_files[index] = file;
    }
    mapped_files = mapped;
    for (index = 1; index < 16; ++index) mapped_file_slots[index] = NULL;
    page_increment = increment;
}

/* Original page allocation [00401f10,00401f24). */
CachedPage *allocate_page(void)
{
    CachedPage *page = allocate_zero(sizeof(CachedPage));
    page->file = -1;
    return page;
}

/* Original checked zero allocation [00401010,0040103a). */
void *allocate_zero(unsigned int size)
{
    void *result = calloc(1, size);
    if (!result) fatal_error(1001, NULL, NULL);
    return result;
}

/* Original file-descriptor allocation [00401190,004011a8). */
CachedFile *allocate_file(void)
{
    CachedFile *file = allocate_zero(sizeof(CachedFile));
    ++file_allocations;
    return file;
}

/* Original cached open dispatcher [00401310,00401376). */
int open_cached_file(const char *name, int flags, int mode)
{
    int created = 0;
    int result = -1;
    CachedFile *file = prepare_cached_file(name, flags, mode, &created);
    if (mapped_files) result = open_mapped_file(file, flags, created);
    if (result == -1) open_stream_file(file, flags, mode, created);
    return file->index;
}

/* Original descriptor preparation [00401380,004013f7). */
CachedFile *prepare_cached_file(const char *name, int flags, int mode, int *created)
{
    CachedFile *file = find_cached_file(name);
    if (!file) {
        file = take_free_file();
        *created = 1;
        file->name = duplicate_string(name);
        file->flags = 0;
        file->length = 0;
        file->handle = NULL;
        file->view = NULL;
        file->capacity = 0;
        file->position = 0;
        file->preferred_base = NULL;
        if (!(flags & 15) || (flags & 2)) file->flags = 1;
        if (flags & 3) file->flags |= 2;
        if (flags & 0x100) file->flags |= 0x10;
    }
    return file;
}

/* Original reusable named-file lookup [00401470,00401505). */
CachedFile *find_cached_file(const char *name)
{
    CachedFile *file = closed_files;
    const unsigned char *first, *second;
    while (file) {
        first = (const unsigned char *)file->name;
        second = (const unsigned char *)name;
        while (*first == *second && *first) { ++first; ++second; }
        if (*first == *second) {
            --closed_file_count;
            move_cached_file(file, &closed_files, &last_closed_file, &active_files, NULL, 0);
            file->flags &= ~4;
            return file;
        }
        file = file->next;
    }
    return NULL;
}

/* Original descriptor acquisition [00401400,00401467). */
CachedFile *take_free_file(void)
{
    CachedFile *file = free_files;
    if (!file) {
        --closed_file_count;
        file = closed_files;
        move_cached_file(file, &closed_files, &last_closed_file, &free_files, NULL, 0);
        release_cached_file(file);
        file = free_files;
    }
    move_cached_file(file, &free_files, NULL, &active_files, NULL, 0);
    return file;
}

/* Original singly linked-list transfer [004020a0,00402132). */
void move_cached_file(CachedFile *file, CachedFile **source, CachedFile **source_last,
                      CachedFile **destination, CachedFile **destination_last, int append)
{
    CachedFile *previous, *current;
    if (file == *source) {
        *source = file->next;
        if (source_last && *source_last == file) *source_last = NULL;
    } else {
        previous = *source;
        current = previous->next;
        while (current && current != file) {
            previous = current;
            current = current->next;
        }
        if (current) previous->next = current->next;
        if (source_last && *source_last == file) *source_last = previous;
    }
    if (!*destination) {
        *destination = file;
        file->next = NULL;
        if (destination_last) *destination_last = file;
    } else if (append) {
        file->next = NULL;
        (*destination_last)->next = file;
        *destination_last = file;
    } else {
        file->next = *destination;
        *destination = file;
    }
}

/* Original checked filename duplication [00401070,00401098). */
char *duplicate_string(const char *name)
{
    char *result = _strdup(name);
    if (!result) fatal_error(1001, NULL, NULL);
    return result;
}

/* Original mapped-file open [00402140,0040226b). */
int open_mapped_file(CachedFile *file, int flags, int created)
{
    int index, access, creating;
    DWORD length;
    if (!created) {
        mapped_seek(file, 0, 0);
        return 0;
    }
    access = flags & 3;
    creating = flags & 0x100;
    if (access) {
        for (index = 0; index < 16; ++index) {
            if (!mapped_file_slots[index]) {
                mapped_file_slots[index] = file;
                break;
            }
        }
        file->handle = CreateFileA(file->name, GENERIC_READ | GENERIC_WRITE,
                                  0, NULL, creating ? CREATE_ALWAYS : OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        file->handle = CreateFileA(file->name, GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, 0, NULL);
    }
    if (file->handle == INVALID_HANDLE_VALUE) {
        file->handle = NULL;
        return -1;
    }
    length = 0;
    if (!creating) {
        length = GetFileSize(file->handle, NULL);
        if (length == 0xffffffffUL) fatal_error(1005, NULL, NULL);
    }
    if (access) {
        file->length = creating ? 0 : length;
        if (!file->capacity)
            file->capacity = file->length < 0x200000 ? 0x200000 : file->length;
    } else {
        file->length = length;
        file->capacity = length;
    }
    if (!create_mapping(file)) return -1;
    file->flags |= 8;
    return 0;
}

/* Original file-mapping setup [00402480,00402537). */
int create_mapping(CachedFile *file)
{
    HANDLE mapping;
    if (file->flags & 2) {
        SetFilePointer(file->handle, file->capacity, NULL, FILE_BEGIN);
        SetEndOfFile(file->handle);
    }
    mapping = CreateFileMappingA(file->handle, NULL,
                  file->flags & 2 ? PAGE_READWRITE : PAGE_READONLY,
                  0, file->capacity, NULL);
    if (!mapping) {
        CloseHandle(file->handle);
        file->handle = NULL;
        return 0;
    }
    file->view = MapViewOfFileEx(mapping,
                  file->flags & 2 ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
                  0, 0, 0, file->preferred_base);
    CloseHandle(mapping);
    if (!file->view) {
        CloseHandle(file->handle);
        file->handle = NULL;
        return 0;
    }
    return 1;
}

/* Original mapped/stream seek dispatch [00401910,00401940). */
int seek_cached_file(int file, int offset, int origin)
{
    CachedFile *entry = cached_files[file];
    if (entry->flags & 8) return mapped_seek(entry, offset, origin);
    return seek_stream_file(entry, offset, origin);
}

/* Original mapped seek [00402270,004022e1). */
int mapped_seek(CachedFile *file, int offset, int origin)
{
    switch (origin) {
    case 0: file->position = offset; break;
    case 1: file->position += offset; break;
    case 2: file->position = file->length + offset; break;
    }
    if (file->position > file->capacity && !grow_mapping(file, file->position)) {
        if (!file->preferred_base) fatal_error(1008, NULL, NULL);
        return -1;
    }
    return file->position;
}

/* Original mapped/stream read dispatch [00401980,004019b0). */
unsigned read_cached_file(int file, void *buffer, unsigned size)
{
    CachedFile *entry = cached_files[file];
    if (entry->flags & 8) return mapped_read(entry, buffer, size);
    return stream_read(entry, buffer, size);
}

/* Original mapped read [004022f0,0040235d). */
unsigned mapped_read(CachedFile *file, void *buffer, unsigned size)
{
    unsigned index;
    int end = file->position + size;
    unsigned char *destination = buffer;
    if (end > file->capacity) {
        if (!(file->flags & 2)) return 0;
        if (!grow_mapping(file, end)) fatal_error(1008, NULL, NULL);
    }
    for (index = 0; index < size; ++index)
        destination[index] = ((unsigned char *)file->view)[file->position + index];
    file->position += size;
    return size;
}

/* Original cached position query [00401a80,00401a9e). */
unsigned tell_cached_file(int file)
{
    CachedFile *entry = cached_files[file];
    if (entry->flags & 8) return entry->position;
    return entry->current_page->position;
}

/* Public cached-file write dispatch [00401cc0,00401cf0). */
unsigned write_cached_file(int index, const void *buffer, unsigned size)
{
    CachedFile *file = cached_files[index];
    if (file->flags & 8) return mapped_write(file, buffer, size);
    return stream_write(file, buffer, size);
}

/* Write into a mapped file and extend its logical length [00402360,004023cc). */
unsigned mapped_write(CachedFile *file, const void *buffer, unsigned size)
{
    unsigned index;
    int end = file->position + size;
    const unsigned char *source = buffer;
    if (end > file->capacity && !grow_mapping(file, end))
        fatal_error(1008, NULL, NULL);
    if (end > file->length) file->length = end;
    for (index = 0; index < size; ++index)
        ((unsigned char *)file->view)[file->position + index] = source[index];
    file->position += size;
    return size;
}

/* Resize dispatch returns the selected backend's status [004011b0,004011e5). */
int resize_cached_file(int index, unsigned size)
{
    CachedFile *file = cached_files[index];
    if ((int)size == file->length) return 0;
    if (file->flags & 8) return resize_mapped_file(file, (int)size);
    return resize_stream_file(file, (int)size);
}


/* Detach the cache entry, optionally retaining a read-only open file.
 * [00401680,00401705), cdecl status result. */
int close_file(int index, int release)
{
    CachedFile *file = cached_files[index];
    if (!release && !(file->flags & 2)) {
        ++closed_file_count;
        move_cached_file(file, &active_files, NULL, &closed_files, &last_closed_file, 1);
        mark_closed_file(file);
        return 0;
    }
    move_cached_file(file, &active_files, NULL, &free_files, NULL, 0);
    if (file->flags & 8) return close_mapped_file(file);
    return release_cached_file(file);
}

/* Persist and close a mapped file, then release its retained name.
 * [004023d0,00402437). The recycled entry keeps its original stale fields. */
int close_mapped_file(CachedFile *file)
{
    int result, index;
    UnmapViewOfFile(file->view);
    if (file->flags & 2) {
        SetFilePointer(file->handle, file->length, NULL, FILE_BEGIN);
        SetEndOfFile(file->handle);
    }
    result = CloseHandle(file->handle);
    for (index = 0; index < 16; ++index)
        if (mapped_file_slots[index] == file) mapped_file_slots[index] = NULL;
    free_bytes(file->name);
    return result;
}

/* Binary-local release wrapper [00401000,0040100f). The CRT supplies free. */
void free_bytes(void *buffer)
{
    free(buffer);
}

/* Checked resize wrapper [00401040,0040106d), cdecl. */
void *resize_bytes(void *buffer, unsigned size)
{
    void *result = realloc(buffer, size);
    if (!result) fatal_error(1001, NULL, NULL);
    return result;
}

/* Set the logical length of a mapped cache entry [004011f0,00401234).
 * The underlying file is truncated when the mapping is closed. */
int resize_mapped_file(CachedFile *file, int size)
{
    if (size > file->capacity) {
        if (!grow_mapping(file, size)) fatal_error(1008, NULL, NULL);
    } else if (file->position > size) file->position = size;
    file->length = size;
    return 0;
}

/* Preserve the file's logical position while measuring its end.
 * Original [00401940,0040197b), ordinary cdecl. */
unsigned int get_file_size(int file)
{
    unsigned int position, size;
    position = seek_cached_file(file, 0, 1);
    size = seek_cached_file(file, 0, 2);
    if (size != position) seek_cached_file(file, position, 0);
    return size;
}

/* Return a range in the file's mapped storage, growing it when necessary.
 * Original [00402540,0040259a), ordinary cdecl. */
void *map_file_region(int file, unsigned int offset, unsigned int size)
{
    CachedFile *entry = cached_files[file];
    unsigned int end;
    if (!(entry->flags & 8)) return NULL;
    end = offset + size;
    if (end > (unsigned)entry->capacity) {
        if (!grow_mapping(entry, end)) fatal_error(1008, NULL, NULL);
    }
    if (end > (unsigned)entry->length) entry->length = end;
    return (unsigned char *)entry->view + offset;
}

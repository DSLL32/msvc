#ifndef CV510_FILES_H
#define CV510_FILES_H
#include <stddef.h>
#include "cvpack.h"
#include <recon.h>

typedef struct CachedFile CachedFile;
typedef struct CachedPage CachedPage;
struct CachedPage {
    int file;
    int start, end, position;
    int initialized_end;
    unsigned flags;
    unsigned char buffer[4096];
    unsigned char *cursor;
    CachedPage *next, *older, *newer;
};
struct CachedFile {
    char *name;
    unsigned flags;
    int index, length;
    union { HANDLE handle; int descriptor; };
    union { void *view; CachedPage **pages; };
    int capacity;
    union { int position; int page_capacity; };
    union { void *preferred_base; CachedPage *current_page; };
    CachedFile *next;
};
/* Field roles recovered from the stream page/flush/LRU consumers. Allocation
 * size and all previously used mapped/cache slots remain unchanged. */
typedef char CachedPageSize[(sizeof(CachedPage)==0x1028)?1:-1];
typedef char CachedPageFlags[(offsetof(CachedPage,flags)==0x14)?1:-1];
typedef char CachedPageBuffer[(offsetof(CachedPage,buffer)==0x18)?1:-1];
typedef char CachedPageCursor[(offsetof(CachedPage,cursor)==0x1018)?1:-1];
extern int maximum_files, file_increment, page_increment, maximum_pages;
extern int file_allocations, mapped_files;
extern CachedPage *free_pages;
extern int page_reloads;
extern CachedPage *newest_page, *oldest_page, *active_pages;
extern CachedFile *free_files, *closed_files, *last_closed_file, *active_files;
extern int closed_file_count;
extern CachedFile **cached_files;
extern CachedFile *mapped_file_slots[16];
extern char input_name[260];
int open_cached_file(const char *name, int flags, int mode);
CachedFile *prepare_cached_file(const char *name, int flags, int mode, int *created);
int open_mapped_file(CachedFile *file, int flags, int created);
unsigned mapped_read(CachedFile *file, void *buffer, unsigned size);
unsigned stream_read(CachedFile *file, void *buffer, unsigned size);
unsigned mapped_write(CachedFile *file, const void *buffer, unsigned size);
unsigned stream_write(CachedFile *file, const void *buffer, unsigned size);
int create_mapping(CachedFile *file);
int grow_mapping(CachedFile *file, int size);
int resize_mapped_file(CachedFile *file, int size);
int resize_stream_file(CachedFile *file, int size);
int seek_stream_file(CachedFile *file, int offset, int origin);
int seek_cached_file(int file, int offset, int origin);
unsigned read_cached_file(int file, void *buffer, unsigned size);
unsigned write_cached_file(int file, const void *buffer, unsigned size);
int resize_cached_file(int file, unsigned size);
unsigned tell_cached_file(int file);
int mapped_seek(CachedFile *file, int offset, int origin);
int open_stream_file(CachedFile *file, int flags, int mode, int created);
CachedFile *find_cached_file(const char *name);
CachedFile *take_free_file(void);
char *duplicate_string(const char *name);
void move_cached_file(CachedFile *file, CachedFile **source, CachedFile **source_last, CachedFile **destination, CachedFile **destination_last, int append);
int release_cached_file(CachedFile *file);
int close_mapped_file(CachedFile *file);
int mark_closed_file(CachedFile *file);
void free_bytes(void *buffer);
void *allocate_zero(unsigned int size);
CachedPage *allocate_page(void);
CachedFile *allocate_file(void);
void grow_page_table(CachedFile *file);
CachedPage *obtain_page(CachedFile *file, int position);
void read_page(CachedFile *file);
void touch_page(CachedPage *page);
CachedPage *evict_page(void);
int release_page(CachedPage *page);
int link_page(CachedFile *file, CachedPage *page);
void advance_page(CachedFile *file);
void flush_page(CachedPage *page);
#endif

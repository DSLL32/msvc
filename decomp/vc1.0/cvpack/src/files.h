#ifndef CVPACK_FILES_H
#define CVPACK_FILES_H
#include <stddef.h>
#include "cvpack.h"
#include <recon.h>

typedef struct Page {
    int handle;
    long start, limit, position, dirty_end;
    unsigned long flags;
    char data[4096];
    char *cursor;
    struct Page *next, *older, *newer;
} Page;

typedef struct FileSlot {
    char *name;
    unsigned long flags;
    int handle;
    long length;
    union { int descriptor; HANDLE handle; } source;
    union { Page **pages; HANDLE mapping; } storage;
    union { unsigned long capacity; char *view; } contents;
    long allocated;
    union { Page *page; long position; } cursor;
    struct FileSlot *next;
} FileSlot;

typedef HANDLE (WINAPI *CreateMapping)(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCSTR);
typedef LPVOID (WINAPI *MapFileView)(HANDLE,DWORD,DWORD,DWORD,DWORD);
typedef BOOL (WINAPI *UnmapFileView)(LPCVOID);

typedef char PageSize[(sizeof(Page)==0x1028)?1:-1];
typedef char PageLinkOffset[(offsetof(Page,next)==0x101c)?1:-1];
typedef char FileSlotSize[(sizeof(FileSlot)==40)?1:-1];
typedef char FileSlotLinkOffset[(offsetof(FileSlot,next)==36)?1:-1];

extern Page *free_pages, *used_pages, *newest_page, *oldest_page;
extern FileSlot *free_files;
extern FileSlot **file_slots;
extern FileSlot *active_files, *closed_files, *closed_files_tail;
extern int closed_file_count;
extern unsigned long page_reads;
extern int file_count, page_count, file_limit, open_limit, page_limit;
extern unsigned char mapped_files;
extern void (*file_failure)(int,const char *,const char *);
extern CreateMapping mapping_create;
extern MapFileView mapping_map;
extern UnmapFileView mapping_unmap;

Page *new_page(void);
FileSlot *new_file_slot(void);
FileSlot *find_cached_file(const char *);
FileSlot *acquire_file_slot(void);
int open_mapped_file(FileSlot *,int,unsigned char);
int map_file(FileSlot *);
int open_buffered_file(FileSlot *,int,int,unsigned char);
void move_file_slot(FileSlot *,FileSlot **,FileSlot **,FileSlot **,FileSlot **,unsigned char);
int release_file_slot(FileSlot *);
void cache_closed_file(FileSlot *);
int close_mapped_file(FileSlot *);
long seek_mapped_file(FileSlot *,long,int);
long seek_buffered_file(FileSlot *,long,int);
void grow_page_table(FileSlot *);
Page *allocate_file_page(FileSlot *,long);
void read_file_page(FileSlot *);
int read_buffered_file(FileSlot *,void *,unsigned long);
int write_buffered_file(FileSlot *,const void *,unsigned long);
int write_mapped_file(FileSlot *,const void *,unsigned long);
int truncate_buffered_file(FileSlot *,long);
int truncate_mapped_file(FileSlot *,long);
int read_mapped_file(FileSlot *,void *,unsigned long);
void advance_file_page(FileSlot *);
void flush_file_page(Page *);
void touch_page(Page *);
Page *remove_lru_page(void);
unsigned char release_page(Page *);
unsigned char attach_page(FileSlot *,Page *);

#endif

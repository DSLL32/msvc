#ifndef LINK_FILES_H
#define LINK_FILES_H
#include "linker-types.h"
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
    unsigned long allocated;
    union { Page *page; long position; } cursor;
    struct FileSlot *next;
} FileSlot;

typedef HANDLE (WINAPI *CreateMapping)(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCSTR);
typedef LPVOID (WINAPI *MapFileView)(HANDLE,DWORD,DWORD,DWORD,DWORD);
typedef BOOL (WINAPI *UnmapFileView)(LPCVOID);

Page *new_page(void);
FileSlot *new_file_slot(void);
FileSlot *find_cached_file(const char *);
FileSlot *acquire_file_slot(void);
void move_file_slot(FileSlot *,FileSlot **,FileSlot **,FileSlot **,FileSlot **,unsigned char);
int open_mapped_file(FileSlot *,int,unsigned char);
int map_file(FileSlot *);
unsigned read_mapped_file(FileSlot *,void *,unsigned);
unsigned write_mapped_file(FileSlot *,const void *,unsigned);
unsigned write_buffered_file(FileSlot *,const void *,unsigned);
unsigned write_file(int,const void *,unsigned);
long seek_mapped_file(FileSlot *,long,int);
int open_buffered_file(FileSlot *,int,int,unsigned char);
unsigned read_buffered_file(FileSlot *,void *,unsigned);
long seek_buffered_file(FileSlot *,long,int);
void cache_closed_file(FileSlot *);
int close_mapped_file(FileSlot *);
int release_file_slot(FileSlot *);
void grow_mapping(FileSlot *,long);
void grow_buffered_file(FileSlot *file);
Page * acquire_file_page(FileSlot *file,long position);
void read_file_page(FileSlot *file);
void touch_file_page(Page *page);
Page * choose_file_page(void);
unsigned char detach_file_page(Page *page);
unsigned char assign_file_page(FileSlot *file,Page *page);
void advance_file_page(FileSlot *);
void flush_file_page(Page *);
#endif

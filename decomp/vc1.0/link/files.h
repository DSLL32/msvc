#ifndef LINK_FILES_H
#define LINK_FILES_H
#include "linker-types.h"

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
long seek_mapped_file(FileSlot *,long,int);
int open_buffered_file(FileSlot *,int,int,unsigned char);
unsigned read_buffered_file(FileSlot *,void *,unsigned);
long seek_buffered_file(FileSlot *,long,int);
int release_file_slot(FileSlot *);
void grow_mapping(FileSlot *,long);
#endif

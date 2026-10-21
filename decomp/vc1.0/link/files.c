/* File table and page pool initialization, [0003c020,0003c1b0). */
#include "link.h"
typedef struct Page { int handle; char fields[0x1018]; struct Page *next; int tail[2]; } Page;
typedef struct FileSlot { int fields[9]; struct FileSlot *next; } FileSlot;
Page *free_pages;
FileSlot *free_files;
FileSlot **files;
int file_count, page_count, page_limit, open_limit, file_limit;
int statistics[15];
unsigned char mapped_files;
void (*failure)(char *,int,char *);
FARPROC mapping_create, mapping_map, mapping_unmap;
Page *new_page(void)
{
    Page *p = calloc(1,0x1028);
    if (!p) failure(NULL,5,"");
    p->handle=-1;
    return p;
}
FileSlot *new_file_slot(void)
{
    FileSlot *p=calloc(1,40);
    if (!p) failure(NULL,5,"");
    ++file_count;
    return p;
}
void files_initialize(int pages,int mapped_limit,int mapped_open,int normal_limit,int normal_open,int mapped,void (*error)(char *,int,char *))
{
    HMODULE kernel;
    FARPROC is_tnt;
    int i;
    Page *page;
    FileSlot *slot;
    if (mapped) {
        kernel=GetModuleHandleA("kernel32.dll");
        if (kernel && GetProcAddress(kernel,"IsTNT")) mapped=0;
        if (mapped) {
            kernel=LoadLibraryA("kernel32.dll");
            if (!kernel) mapped=0;
            else {
                mapping_create=GetProcAddress(kernel,"CreateFileMappingA");
                mapping_map=GetProcAddress(kernel,"MapViewOfFile");
                mapping_unmap=GetProcAddress(kernel,"UnmapViewOfFile");
                if (!mapping_create || !mapping_map || !mapping_unmap) mapped=0;
            }
        }
    }
    file_limit=(unsigned short)(mapped?mapped_limit:normal_limit);
    open_limit=(unsigned short)(mapped?mapped_open:normal_open);
    page_count=pages;
    for (i=pages;i>0;--i) {
        page=new_page();
        if (free_pages) page->next=free_pages;
        free_pages=page;
    }
    files=calloc(file_limit,4);
    if (!files) failure(NULL,5,"");
    for (i=0;i<file_limit;++i) {
        slot=new_file_slot(); slot->fields[2]=i;
        if (free_files) slot->next=free_files;
        free_files=slot; files[i]=slot;
    }
    page_limit=open_limit;
    failure=error;
    mapped_files=(unsigned char)mapped;
    if (debug_enabled(44)) mapped_files=0;
    for (i=0;i<15;++i) statistics[i]=0;
}

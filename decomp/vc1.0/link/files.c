/* Cached file slots, page pools, and optional Win32 file mapping. */
#include "files.h"
#include <sys/stat.h>
#include <io.h>

Page *free_pages;
FileSlot *free_files;
FileSlot **files;
int file_count, page_count, page_limit, open_limit, file_limit;
FileSlot *mapped_outputs[16];
unsigned char mapped_files;
void (*failure)(char *,int,char *);
CreateMapping mapping_create;
MapFileView mapping_map;
UnmapFileView mapping_unmap;

int closed_file_count;
FileSlot *active_files, *closed_files, *closed_files_tail;

Page *new_page(void)
{
    Page *p = calloc(1,sizeof(*p));
    if (!p) failure(NULL,5,"");
    p->handle=-1;
    return p;
}
FileSlot *new_file_slot(void)
{
    FileSlot *p=calloc(1,sizeof(*p));
    if (!p) failure(NULL,5,"");
    ++file_count;
    return p;
}
void files_initialize(int pages,int mapped_limit,int mapped_open,int normal_limit,int normal_open,int mapped,void (*error)(char *,int,char *))
{
    HMODULE kernel;
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
                mapping_create=(CreateMapping)GetProcAddress(kernel,"CreateFileMappingA");
                mapping_map=(MapFileView)GetProcAddress(kernel,"MapViewOfFile");
                mapping_unmap=(UnmapFileView)GetProcAddress(kernel,"UnmapViewOfFile");
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
    files=calloc(file_limit,sizeof(*files));
    if (!files) failure(NULL,5,"");
    for (i=0;i<file_limit;++i) {
        slot=new_file_slot(); slot->handle=i;
        if (free_files) slot->next=free_files;
        free_files=slot; files[i]=slot;
    }
    page_limit=open_limit;
    failure=error;
    mapped_files=(unsigned char)mapped;
    if (debug_enabled(44)) mapped_files=0;
    for (i=1;i<16;++i) mapped_outputs[i]=NULL;
}


void move_file_slot(FileSlot *file,FileSlot **from,FileSlot **from_tail,
                    FileSlot **to,FileSlot **to_tail,unsigned char append)
{
    FileSlot *previous, *scan;
    if (*from==file) {
        *from=file->next;
        if (from_tail && *from_tail==file) *from_tail=NULL;
    } else {
        previous=*from;
        scan=previous->next;
        while (scan && scan!=file) { previous=scan; scan=scan->next; }
        if (scan) previous->next=scan->next;
        if (from_tail && *from_tail==file) *from_tail=previous;
    }
    if (!*to) {
        *to=file;
        file->next=NULL;
        if (to_tail) *to_tail=file;
    } else if (append) {
        file->next=NULL;
        (*to_tail)->next=file;
        *to_tail=file;
    } else {
        file->next=*to;
        *to=file;
    }
}

FileSlot *acquire_file_slot(void)
{
    FileSlot *file;
    if (!free_files) {
        file=closed_files;
        --closed_file_count;
        move_file_slot(file,&closed_files,&closed_files_tail,&free_files,NULL,0);
        release_file_slot(file);
    }
    file=free_files;
    move_file_slot(file,&free_files,NULL,&active_files,NULL,0);
    return file;
}

FileSlot *find_cached_file(const char *name)
{
    FileSlot *file;
    const char *left, *right;
    for (file=closed_files;file;file=file->next) {
        left=file->name; right=name;
        while (*left==*right && *left) { ++left; ++right; }
        if (*left==*right) {
            --closed_file_count;
            move_file_slot(file,&closed_files,&closed_files_tail,&active_files,NULL,0);
            file->flags &= ~4UL;
            return file;
        }
    }
    return NULL;
}

int open_file(const char *name,int mode,int permission)
{
    unsigned char fresh=0;
    int result=-1;
    FileSlot *file=find_cached_file(name);
    if (!file) {
        file=acquire_file_slot();
        fresh=1;
        file->name=_strdup(name);
        if (!file->name) failure(NULL,5,"");
        if (!(mode&15) || (mode&2)) file->flags |= 1;
        if ((mode&1) || (mode&2)) file->flags |= 2;
        if (mode&0x100) file->flags |= 0x10;
    }
    if (mapped_files) result=open_mapped_file(file,mode,fresh);
    if (result==-1) open_buffered_file(file,mode,permission,fresh);
    return file->handle;
}

int map_file(FileSlot *file)
{
    file->storage.mapping=mapping_create(file->source.handle,NULL,
        (file->flags&2)?PAGE_READWRITE:PAGE_READONLY,0,file->allocated,NULL);
    if (!file->storage.mapping) {
        CloseHandle(file->source.handle);
        file->source.handle=NULL;
        file->storage.mapping=NULL;
        return 0;
    }
    file->contents.view=mapping_map(file->storage.mapping,
        (file->flags&2)?FILE_MAP_ALL_ACCESS:FILE_MAP_READ,0,0,0);
    if (!file->contents.view) {
        CloseHandle(file->source.handle);
        file->source.handle=NULL;
        file->storage.mapping=NULL;
        return 0;
    }
    return 1;
}

int open_mapped_file(FileSlot *file,int mode,unsigned char fresh)
{
    struct _stat info;
    int writable, create, i;
    long capacity;
    if (!fresh) { seek_mapped_file(file,0,SEEK_SET); return 0; }
    writable=(mode&3)!=0;
    create=mode&0x100;
    if (writable) {
        for (i=0;i<16;++i) {
            if (!mapped_outputs[i]) { mapped_outputs[i]=file; break; }
        }
        file->source.handle=CreateFileA(file->name,GENERIC_READ|GENERIC_WRITE,0,
            NULL,create?CREATE_ALWAYS:OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    } else file->source.handle=CreateFileA(file->name,GENERIC_READ,FILE_SHARE_READ,
        NULL,OPEN_EXISTING,0,NULL);
    if (file->source.handle==INVALID_HANDLE_VALUE) {
        file->source.handle=NULL;
        return -1;
    }
    if (!create && _stat(file->name,&info)==-1) failure(NULL,7,file->name);
    if (writable) {
        file->length=create?0:info.st_size;
        capacity=file->length > 0x200000 ? file->length : 0x200000;
    } else capacity=file->length=info.st_size;
    file->allocated=capacity;
    if (!map_file(file)) return -1;
    file->flags |= 8;
    return 0;
}

unsigned read_mapped_file(FileSlot *file,void *buffer,unsigned size)
{
    unsigned i;
    char *destination=buffer;
    if ((long)(file->cursor.position+size) > (long)file->allocated) {
        if (!(file->flags&2)) return 0;
        grow_mapping(file,file->cursor.position+size);
    }
    for (i=0;i<size;++i) destination[i]=file->contents.view[file->cursor.position+i];
    file->cursor.position+=size;
    return size;
}

unsigned read_file(int handle,void *buffer,unsigned size)
{
    FileSlot *file=files[handle];
    if (file->flags&8) return read_mapped_file(file,buffer,size);
    return read_buffered_file(file,buffer,size);
}

long seek_mapped_file(FileSlot *file,long offset,int origin)
{
    switch (origin) {
    case SEEK_SET: file->cursor.position=offset; break;
    case SEEK_CUR: file->cursor.position+=offset; break;
    case SEEK_END: file->cursor.position=file->length+offset; break;
    }
    if ((long)file->allocated < file->cursor.position)
        grow_mapping(file,file->cursor.position);
    return file->cursor.position;
}

long seek_file(int handle,long offset,int origin)
{
    FileSlot *file=files[handle];
    if (file->flags&8) return seek_mapped_file(file,offset,origin);
    return seek_buffered_file(file,offset,origin);
}

long file_size(int handle)
{
    long position=seek_file(handle,0,SEEK_CUR);
    long size=seek_file(handle,0,SEEK_END);
    if (position!=size) seek_file(handle,position,SEEK_SET);
    return size;
}

int close_file(int handle,unsigned char release)
{
    FileSlot *file=files[handle];
    if (!release && !(file->flags&2)) {
        ++closed_file_count;
        move_file_slot(file,&active_files,NULL,&closed_files,&closed_files_tail,1);
        cache_closed_file(file);
        return 0;
    }
    move_file_slot(file,&active_files,NULL,&free_files,NULL,0);
    if (file->flags&8) return close_mapped_file(file);
    return release_file_slot(file);
}

void cache_closed_file(FileSlot *file)
{
    FileSlot *oldest;
    file->flags |= 4;
    if (!(file->flags&8)) {
        file->cursor.page->flags &= ~0x10UL;
        file->cursor.page=NULL;
    }
    if (closed_file_count==page_limit) {
        oldest=closed_files;
        --closed_file_count;
        move_file_slot(oldest,&closed_files,&closed_files_tail,&free_files,NULL,0);
        if (oldest->flags&8) close_mapped_file(oldest);
        else release_file_slot(oldest);
    }
}

long tell_file(int handle)
{
    FileSlot *file=files[handle];
    if (file->flags&8) return file->cursor.position;
    return file->cursor.page->position;
}

unsigned write_file(int handle,const void *buffer,unsigned size)
{
    FileSlot *file=files[handle];
    if (file->flags&8) return write_mapped_file(file,buffer,size);
    return write_buffered_file(file,buffer,size);
}


#pragma intrinsic(memcpy)
unsigned write_mapped_file(FileSlot *file,const void *buffer,unsigned size)
{
    long end=file->cursor.position+size;
    if (end>(long)file->allocated) grow_mapping(file,end);
    if (file->length<end) file->length=end;
    memcpy(file->contents.view+file->cursor.position,buffer,size);
    file->cursor.position+=size;
    return size;
}


void *mapped_output_pointer(int handle,unsigned long offset,unsigned size)
{
    FileSlot *file=files[handle];
    unsigned long end;
    if (!(file->flags&8)) return NULL;
    end=offset+size;
    if (file->allocated<end) grow_mapping(file,end);
    if ((unsigned long)file->length<end) file->length=end;
    return file->contents.view+offset;
}


int close_mapped_file(FileSlot *file)
{
    int result,index;
    mapping_unmap(file->contents.view);
    if (file->flags&2) {
        SetFilePointer(file->source.handle,file->length,NULL,FILE_BEGIN);
        SetEndOfFile(file->source.handle);
    }
    CloseHandle(file->storage.mapping);
    result=CloseHandle(file->source.handle);
    for (index=0;index<16;++index)
        if (mapped_outputs[index]==file) mapped_outputs[index]=NULL;
    file->source.handle=NULL;
    file->storage.mapping=NULL;
    file->contents.view=NULL;
    file->cursor.position=0;
    file->allocated=0;
    file->flags=0;
    free(file->name);
    file->name=NULL;
    return result;
}


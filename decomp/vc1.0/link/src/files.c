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
/* Private compatibility state for the observable unused COFF auxiliary tail.
 * The original exposes part of SetFilePointer's old stack local after closing
 * the mapped .exp file. Retain the value explicitly instead of relying on the
 * reconstructed compiler's unrelated stack layout. */
unsigned long mapped_close_seek_position;

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
        mapped_close_seek_position=file->length;
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


void close_all_files(void)
{
    FileSlot *file,*next;
    for (file=active_files;file;file=next) {
        next=file->next;
        move_file_slot(file,&active_files,NULL,&free_files,NULL,0);
        if (file->flags&8) close_mapped_file(file);
        else release_file_slot(file);
    }
    for (file=closed_files;file;file=next) {
        next=file->next;
        --closed_file_count;
        move_file_slot(file,&closed_files,&closed_files_tail,&free_files,NULL,0);
        if (file->flags&8) close_mapped_file(file);
        else release_file_slot(file);
    }
}

int open_buffered_file(FileSlot *file,int mode,int permission,unsigned char fresh)
{
    struct _stat info;
    FileSlot *previous;
    unsigned long pages,index;
    if (fresh) {
        file->source.descriptor=_open(file->name,mode,permission);
        if (file->source.descriptor==-1) {
            if (active_files==file) {
                active_files=file->next;
                --file_count;
            } else {
                previous=active_files;
                while (previous && previous->next!=file) previous=previous->next;
                if (previous) {
                    previous->next=file->next;
                    --file_count;
                }
            }
            failure(NULL,7,file->name);
        }
        file->length=0;
        if ((file->flags&2) && (!(file->flags&3) || (file->flags&0x10)))
            file->contents.capacity=0x400000;
        else {
            if (_stat(file->name,&info)==-1) failure(NULL,7,file->name);
            if (!info.st_size) {
                file->storage.pages=calloc(1,sizeof(Page *));
                if (!file->storage.pages) failure(NULL,5,"");
                failure(file->name,0x27,"");
            }
            file->contents.capacity=file->length=info.st_size;
        }
        pages=(file->contents.capacity+4096)>>12;
        file->allocated=pages<<12;
        file->storage.pages=calloc(1,pages*sizeof(Page *));
        if (!file->storage.pages) failure(NULL,5,"");
        if (!(file->flags&0x10) && (file->flags&3))
            for (index=0;index<pages;++index) file->storage.pages[index]=(Page *)1;
    }
    seek_buffered_file(file,0,SEEK_SET);
    return 0;
}

unsigned long buffered_page_reads;
long seek_buffered_file(FileSlot *file,long offset,int origin)
{
    long position;
    Page *page;
    if (origin==SEEK_SET) position=offset;
    else if (origin==SEEK_CUR) position=file->cursor.page->position+offset;
    else position=file->length+offset;
    while ((long)file->allocated<=position) grow_buffered_file(file);
    page=file->storage.pages[position>>12];
    if (file->cursor.page) {
        file->cursor.page->flags &= ~0x10UL;
        file->cursor.page=NULL;
    }
    if ((unsigned long)page<=1) {
        file->cursor.page=acquire_file_page(file,position);
        file->cursor.page->flags |= 0x10;
        if (page==(Page *)1) {
            read_file_page(file);
            ++buffered_page_reads;
            file->cursor.page->flags |= 4;
        }
    } else {
        file->cursor.page=page;
        page->flags |= 0x10;
    }
    page=file->cursor.page;
    page->position=position;
    page->cursor=page->data+position-page->start;
    if ((file->flags&2) && page->dirty_end<page->position)
        page->dirty_end=page->position;
    touch_file_page(page);
    return position;
}

#pragma intrinsic(memset)
Page *acquire_file_page(FileSlot *file,long position)
{
    Page *page=free_pages;
    if (!page) {
        page=choose_file_page();
        detach_file_page(page);
    }
    page->start=(position/4096)*4096;
    page->limit=page->start+4096;
    page->position=position;
    page->dirty_end=page->start;
    page->cursor=page->data+position-page->start;
    if (file->flags&2) memset(page->data,0,sizeof(page->data));
    else page->flags |= 8;
    assign_file_page(file,page);
    return page;
}

Page *active_pages;
unsigned char assign_file_page(FileSlot *file,Page *page)
{
    if (!free_pages) return 0;
    free_pages=page->next;
    page->handle=file->handle;
    page->next=active_pages;
    active_pages=page;
    page->flags |= 1;
    file->storage.pages[page->start>>12]=page;
    return 1;
}

void read_file_page(FileSlot *file)
{
    Page *page=file->cursor.page;
    long count,position;
    position=_lseek(file->source.descriptor,page->start,SEEK_SET);
    if (position==-1) failure(file->name,9,(char *)position);
    count=file->length-page->start;
    if (count>=4096) count=4096;
    if (_read(file->source.descriptor,page->data,count)!=count)
        failure(file->name,10,(char *)_tell(file->source.descriptor));
    file->cursor.page->flags &= ~8UL;
}

Page *page_cache_head,*page_cache_tail;
void touch_file_page(Page *page)
{
    Page *scan=page_cache_head;
    while (scan && scan!=page) scan=scan->older;
    if (scan) {
        if (page->newer) page->newer->older=page->older;
        else page_cache_head=page->older;
        if (page->older) page->older->newer=page->newer;
        else page_cache_tail=page->newer;
    }
    if (!page_cache_head && !page_cache_tail) {
        page_cache_head=page_cache_tail=page;
        page->older=page->newer=NULL;
        return;
    }
    page->older=page_cache_head;
    page->newer=NULL;
    page_cache_head->newer=page;
    page_cache_head=page;
}

unsigned read_buffered_file(FileSlot *file,void *buffer,unsigned size)
{
    unsigned long end=file->cursor.page->position+size;
    unsigned remaining=size;
    long amount;
    Page *page;
    char *destination=buffer;
    if (file->allocated<end) failure(file->name,10,(char *)end);
    while (remaining) {
        if (file->cursor.page->flags&8) read_file_page(file);
        page=file->cursor.page;
        amount=page->limit-page->position;
        if (amount>=(long)remaining) amount=remaining;
        memcpy(destination,page->cursor,amount);
        file->cursor.page->position+=amount;
        file->cursor.page->cursor+=amount;
        if (file->cursor.page->position==file->cursor.page->limit)
            advance_file_page(file);
        remaining-=amount;
        destination+=amount;
    }
    return size;
}

unsigned write_buffered_file(FileSlot *file,const void *buffer,unsigned size)
{
    unsigned remaining=size;
    long amount;
    Page *page;
    const char *source=buffer;
    if (!size) return 0;
    while ((long)(file->cursor.page->position+size)>=(long)file->allocated)
        grow_buffered_file(file);
    file->cursor.page->flags |= 2;
    while (remaining) {
        page=file->cursor.page;
        amount=page->limit-page->position;
        if (amount>=(long)remaining) amount=remaining;
        memcpy(page->cursor,source,amount);
        file->cursor.page->position+=amount;
        file->cursor.page->cursor+=amount;
        page=file->cursor.page;
        if (page->dirty_end<page->position) page->dirty_end=page->position;
        if (page->position==page->limit) advance_file_page(file);
        if (remaining) file->cursor.page->flags |= 2;
        remaining-=amount;
        source+=amount;
    }
    return size;
}

void advance_file_page(FileSlot *file)
{
    Page **next=file->storage.pages+(file->cursor.page->start>>12)+1;
    Page *page=*next;
    if ((unsigned long)page>1) {
        file->cursor.page->flags &= ~0x10UL;
        detach_file_page(file->cursor.page);
        file->cursor.page=*next;
        file->cursor.page->flags |= 0x10;
    } else {
        flush_file_page(file->cursor.page);
        *next=file->cursor.page;
        file->cursor.page->start+=4096;
        file->cursor.page->limit+=4096;
        file->cursor.page->dirty_end=file->cursor.page->start;
        if (!(file->flags&2)) file->cursor.page->flags |= 8;
        if (page==(Page *)1) {
            read_file_page(file);
            ++buffered_page_reads;
            file->cursor.page->flags |= 4;
        }
    }
    file->cursor.page->position=file->cursor.page->start;
    file->cursor.page->cursor=file->cursor.page->data;
}

void flush_file_page(Page *page)
{
    FileSlot *file=files[page->handle];
    long count;
    if (page->flags&2) {
        count=page->dirty_end-page->start;
        if (_lseek(file->source.descriptor,page->start,SEEK_SET)==-1)
            failure(file->name,9,(char *)page->start);
        if (_write(file->source.descriptor,page->data,count)!=count)
            failure(file->name,11,(char *)_tell(file->source.descriptor));
        file->storage.pages[page->start>>12]=(Page *)1;
        if ((file->flags&2) && file->length<page->dirty_end)
            file->length=page->dirty_end;
    } else if (page->flags&4)
        file->storage.pages[page->start>>12]=(Page *)1;
    else file->storage.pages[page->start>>12]=NULL;
}

int release_file_slot(FileSlot *file)
{
    Page *page=active_pages,*next;
    int result;
    while (page) {
        next=page->next;
        if (page->handle==file->handle) {
            if (page==file->cursor.page) page->flags &= ~0x10UL;
            detach_file_page(page);
        }
        page=next;
    }
    result=_close(file->source.descriptor);
    if (result==-1) failure(NULL,8,file->name);
    free(file->name);
    file->name=NULL;
    file->source.descriptor=-1;
    file->flags=0;
    file->contents.capacity=0;
    file->cursor.page=NULL;
    file->length=0;
    file->allocated=0;
    free(file->storage.pages);
    file->storage.pages=NULL;
    return result;
}

unsigned char detach_file_page(Page *page)
{
    Page *previous,*next;
    flush_file_page(page);
    page->flags=0;
    page->handle=-1;
    if (active_pages==page) active_pages=page->next;
    else {
        previous=active_pages;
        next=previous->next;
        while (next && next!=page) {
            previous=next;
            next=next->next;
        }
        if (next) previous->next=next->next;
    }
    page->next=free_pages;
    free_pages=page;
    return 1;
}

/* Select the oldest unpinned buffered page [0003cf90,0003d010). */
Page *choose_file_page(void)
{
    Page *page=page_cache_tail;
    while (page && (page->flags&0x10)) page=page->newer;
    if (page==page_cache_tail) page_cache_tail=page->newer;
    else page->older->newer=page->newer;
    if (page==page_cache_head) page_cache_head=page->older;
    else page->newer->older=page->older;
    page->older=page->newer=NULL;
    return page;
}

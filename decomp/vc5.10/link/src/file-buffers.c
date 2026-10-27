/* The original ordinary-file path caches fixed 4096-byte pages. */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <share.h>
#include <errno.h>
#include <sys/stat.h>
#include "files.h"
#include "arguments.h"
#include "memory.h"
#include "link.h"
#include <recon.h>

/* Original467554,467530: initialized once, with a distinct free-list owner. */
BYTE file_buffers_initialized;
FileBufferHeader *free_file_buffers;
/* Original467534 is the active file-buffer chain, distinct from the free list. */
FileBufferHeader *used_file_buffers;
/* Original4674e0/467544 order the distinct least-recently-used chain. */
FileBufferHeader *newest_file_buffer,*oldest_file_buffer;
FileBufferHeader *__cdecl new_file_buffer(void)
{
    FileBufferHeader *buffer=(FileBufferHeader *)zero_allocate(sizeof(*buffer));
    buffer->file_index=-1;
    return buffer;
}
void __cdecl initialize_file_buffers(void)
{
    unsigned count;
    FileBufferHeader *buffer;
    if(file_buffers_initialized)return;
    file_buffers_initialized=1;
    for(count=0;count<30;++count) {
        buffer=new_file_buffer();
        if(free_file_buffers)buffer->next=free_file_buffers;
        free_file_buffers=buffer;
    }
}

extern FileRecord *pending_files;
int __stdcall open_buffered_file(FileRecord *record,int flags,int mode,BYTE created)
{
    struct _stat status;
    FileRecord *previous;
    unsigned pages,index;
    if(created) {
        record->descriptor=_sopen(record->name,flags,(flags&3)?_SH_DENYRW:_SH_DENYWR,mode);
        if(record->descriptor==-1) {
            if(record==pending_files) {
                pending_files=record->previous;
                --file_record_count;
            } else {
                previous=pending_files;
                while(previous && previous->previous!=record)previous=previous->previous;
                if(previous) {
                    previous->previous=record->previous;
                    --file_record_count;
                }
            }
            if(_doserrno==ERROR_DISK_FULL)link_fatal(record->name,1180);
            link_fatal(NULL,1104,record->name);
        }
        record->size=0;
        if((record->flags&2) && (!(record->flags&3) || (record->flags&0x10)))
            record->mapping_size=0x400000;
        else {
            if(_stat(record->name,&status)==-1)link_fatal(NULL,1104,record->name);
            if(!status.st_size) {
                record->view=zero_allocate(4);
                link_fatal(record->name,1136);
            }
            record->mapping_size=status.st_size;
            record->size=status.st_size;
        }
        pages=((DWORD)record->mapping_size+4096)>>12;
        record->file_size=pages<<12;
        record->view=zero_allocate(pages*sizeof(FileBufferHeader *));
        if((record->flags&1) && !(record->flags&0x10))
            for(index=0;index<pages;++index)((FileBufferHeader **)record->view)[index]=(FileBufferHeader *)1;
    }
    seek_buffered_file(record,0,FILE_BEGIN);
    return 0;
}

/* Original467550 counts physical page fills, separately from file accesses. */
DWORD buffered_page_reads;
#pragma intrinsic(memset)
void __stdcall extend_buffered_page_table(FileRecord *record)
{
    unsigned old_pages=record->file_size/4096;
    unsigned new_pages=old_pages*2;
    FileBufferHeader **pages=(FileBufferHeader **)reallocate_memory(record->view,
        new_pages*sizeof(*pages));
    record->view=pages;
    memset(pages+old_pages,0,(new_pages-old_pages)*sizeof(*pages));
    record->file_size=new_pages*4096;
}
FileBufferHeader *__cdecl evict_file_buffer(void)
{
    FileBufferHeader *buffer=oldest_file_buffer;
    while(buffer && (buffer->flags&0x10))buffer=buffer->newer;
    /* Like the original caller, this path requires an unlocked LRU entry. */
    if(buffer==oldest_file_buffer)oldest_file_buffer=buffer->newer;
    else buffer->older->newer=buffer->newer;
    if(buffer==newest_file_buffer)newest_file_buffer=buffer->older;
    else buffer->newer->older=buffer->older;
    buffer->older=buffer->newer=NULL;
    return buffer;
}
/* Original48a738 is initially zero and set by the fatal
 * diagnostic exit owner at30160. Normal cleanup must flush each active buffer. */
BYTE link_diagnostic_exiting;
void __stdcall flush_file_buffer(FileBufferHeader *buffer)
{
    FileRecord *record=file_slots[buffer->file_index];
    FileBufferHeader **pages=(FileBufferHeader **)record->view;
    LONG index=buffer->page_start>>12,bytes;
    if(buffer->flags&2) {
        bytes=buffer->valid_end-buffer->page_start;
        if(_lseek(record->descriptor,buffer->page_start,SEEK_SET)==-1)
            link_fatal(record->name,1106,buffer->page_start);
        if(_write(record->descriptor,buffer->data,bytes)!=bytes) {
            if(_doserrno==ERROR_DISK_FULL)link_fatal(record->name,1180);
            link_fatal(record->name,1108,_tell(record->descriptor));
        }
        pages[index]=(FileBufferHeader *)1;
        if((record->flags&2) && buffer->valid_end>record->size)record->size=buffer->valid_end;
    } else if(buffer->flags&4)pages[index]=(FileBufferHeader *)1;
    else pages[index]=NULL;
}
BYTE __stdcall release_file_buffer(FileBufferHeader *buffer)
{
    FileBufferHeader *previous,*current;
    if(!link_diagnostic_exiting)flush_file_buffer(buffer);
    buffer->flags=0;
    buffer->file_index=-1;
    previous=used_file_buffers;
    current=previous->next;
    if(buffer==previous)used_file_buffers=current;
    else {
        while(current) {
            if(current==buffer) {
                previous->next=current->next;
                break;
            }
            previous=current;
            current=current->next;
        }
    }
    buffer->next=free_file_buffers;
    free_file_buffers=buffer;
    return 1;
}
BYTE __stdcall assign_file_buffer(FileRecord *record,FileBufferHeader *buffer)
{
    if(!free_file_buffers)return 0;
    free_file_buffers=buffer->next;
    buffer->file_index=record->index;
    buffer->next=used_file_buffers;
    used_file_buffers=buffer;
    buffer->flags|=1;
    ((FileBufferHeader **)record->view)[buffer->page_start>>12]=buffer;
    return 1;
}
#pragma intrinsic(memset)
FileBufferHeader *__stdcall acquire_file_buffer(FileRecord *record,LONG offset)
{
    FileBufferHeader *buffer=free_file_buffers;
    if(!buffer) {
        buffer=evict_file_buffer();
        release_file_buffer(buffer);
    }
    buffer->cursor=offset;
    buffer->page_start=(offset/4096)*4096;
    buffer->page_end=buffer->page_start+4096;
    buffer->valid_end=buffer->page_start;
    buffer->current=buffer->data+(offset-buffer->page_start);
    if(record->flags&2)memset(buffer->data,0,sizeof(buffer->data));
    else buffer->flags|=8;
    assign_file_buffer(record,buffer);
    return buffer;
}
void __stdcall read_buffered_page(FileRecord *record)
{
    FileBufferHeader *buffer=(FileBufferHeader *)record->base;
    LONG position;
    int bytes;
    position=_lseek(record->descriptor,buffer->page_start,SEEK_SET);
    if(position==-1)link_fatal(record->name,1106,position);
    buffer=(FileBufferHeader *)record->base;
    bytes=record->size-buffer->page_start;
    if(bytes>4096)bytes=4096;
    if(_read(record->descriptor,buffer->data,bytes)!=bytes)
        link_fatal(record->name,1107,_tell(record->descriptor));
    ((FileBufferHeader *)record->base)->flags&=~8UL;
}
void __stdcall touch_file_buffer(FileBufferHeader *buffer)
{
    FileBufferHeader *current=newest_file_buffer;
    while(current && current!=buffer)current=current->older;
    if(current) {
        if(buffer->newer)buffer->newer->older=buffer->older;
        else newest_file_buffer=buffer->older;
        if(buffer->older)buffer->older->newer=buffer->newer;
        else oldest_file_buffer=buffer->newer;
    }
    if(!newest_file_buffer && !oldest_file_buffer) {
        newest_file_buffer=oldest_file_buffer=buffer;
        buffer->older=buffer->newer=NULL;
        return;
    }
    buffer->older=newest_file_buffer;
    buffer->newer=NULL;
    newest_file_buffer->newer=buffer;
    newest_file_buffer=buffer;
}
LONG __stdcall seek_buffered_file(FileRecord *record,LONG offset,int origin)
{
    LONG position;
    FileBufferHeader *old_buffer,*buffer,*current;
    switch(origin) {
    case FILE_BEGIN: position=offset;break;
    case FILE_CURRENT: position=((FileBufferHeader *)record->base)->cursor+offset;break;
    default: position=record->size+offset;break;
    }
    while(position>=record->file_size)extend_buffered_page_table(record);
    old_buffer=(FileBufferHeader *)record->base;
    buffer=((FileBufferHeader **)record->view)[position>>12];
    if(old_buffer) {
        old_buffer->flags&=~0x10UL;
        record->base=NULL;
    }
    if((DWORD)buffer<=1) {
        current=acquire_file_buffer(record,position);
        record->base=current;
        current->flags|=0x10;
        if(buffer==(FileBufferHeader *)1) {
            read_buffered_page(record);
            ++buffered_page_reads;
            ((FileBufferHeader *)record->base)->flags|=4;
        }
    } else {
        record->base=buffer;
        buffer->flags|=0x10;
    }
    current=(FileBufferHeader *)record->base;
    current->cursor=position;
    current->current=current->data+(position-current->page_start);
    if((record->flags&2) && current->cursor>current->valid_end)
        current->valid_end=current->cursor;
    touch_file_buffer(current);
    return position;
}

void __stdcall advance_buffered_page(FileRecord *record)
{
    FileBufferHeader *buffer=(FileBufferHeader *)record->base,*next;
    FileBufferHeader **pages=(FileBufferHeader **)record->view;
    LONG page=buffer->page_start>>12;
    next=pages[page+1];
    if(next && next!=(FileBufferHeader *)1) {
        buffer->flags&=~0x10UL;
        release_file_buffer(buffer);
        record->base=pages[page+1];
        ((FileBufferHeader *)record->base)->flags|=0x10;
    } else {
        flush_file_buffer(buffer);
        buffer=(FileBufferHeader *)record->base;
        pages[page+1]=buffer;
        buffer->page_start+=4096;
        buffer->page_end+=4096;
        buffer->valid_end=buffer->page_start;
        /* Sequential reuse preserves this physical page's existing data. */
        if(!(record->flags&2))buffer->flags|=8;
        if(next==(FileBufferHeader *)1) {
            read_buffered_page(record);
            ++buffered_page_reads;
            ((FileBufferHeader *)record->base)->flags|=4;
        }
    }
    buffer=(FileBufferHeader *)record->base;
    buffer->cursor=buffer->page_start;
    buffer->current=buffer->data;
}
#pragma intrinsic(memcpy)
unsigned __stdcall buffered_read(FileRecord *record,void *destination,unsigned bytes)
{
    unsigned remaining=bytes;
    BYTE *output=(BYTE *)destination;
    FileBufferHeader *buffer=(FileBufferHeader *)record->base;
    DWORD end=(DWORD)buffer->cursor+bytes;
    LONG count,available;
    if(end>(DWORD)record->file_size)link_fatal(record->name,1107,end);
    while(remaining) {
        buffer=(FileBufferHeader *)record->base;
        if(buffer->flags&8)read_buffered_page(record);
        buffer=(FileBufferHeader *)record->base;
        available=buffer->page_end-buffer->cursor;
        count=(LONG)remaining;
        if(count>=available)count=available;
        memcpy(output,buffer->current,count);
        buffer=(FileBufferHeader *)record->base;
        buffer->cursor+=count;
        buffer->current+=count;
        if(buffer->cursor==buffer->page_end)advance_buffered_page(record);
        remaining-=count;
        output+=count;
    }
    return bytes;
}

unsigned __stdcall buffered_write(FileRecord *record,const void *source,unsigned bytes)
{
    unsigned remaining=bytes;
    const BYTE *input=(const BYTE *)source;
    FileBufferHeader *buffer;
    LONG count,available;
    if(!bytes)return 0;
    while((LONG)((DWORD)((FileBufferHeader *)record->base)->cursor+bytes)>=record->file_size)
        extend_buffered_page_table(record);
    ((FileBufferHeader *)record->base)->flags|=2;
    do {
        buffer=(FileBufferHeader *)record->base;
        available=buffer->page_end-buffer->cursor;
        count=(LONG)remaining;
        if(count>=available)count=available;
        memcpy(buffer->current,input,count);
        buffer=(FileBufferHeader *)record->base;
        buffer->cursor+=count;
        buffer->current+=count;
        if(buffer->cursor>buffer->valid_end)buffer->valid_end=buffer->cursor;
        if(buffer->cursor==buffer->page_end)advance_buffered_page(record);
        /* The original marks this page before decrementing remaining, even
         * when the request ended exactly at the preceding page boundary. */
        ((FileBufferHeader *)record->base)->flags|=2;
        remaining-=count;
        input+=count;
    } while(remaining);
    return bytes;
}

int __stdcall close_buffered_file(FileRecord *record)
{
    FileBufferHeader *buffer=used_file_buffers,*next;
    int result;
    while(buffer) {
        next=buffer->next;
        if(buffer->file_index==(LONG)record->index) {
            if(buffer==(FileBufferHeader *)record->base)buffer->flags&=~0x10UL;
            release_file_buffer(buffer);
        }
        buffer=next;
    }
    result=_close(record->descriptor);
    if(result==-1)link_fatal(NULL,1105,record->name);
    release_memory(record->name);
    release_memory(record->view);
    return result;
}

int __stdcall set_buffered_file_length(FileRecord *record,LONG length)
{
    LONG page,count;
    FileBufferHeader *buffer;
    if(length>record->size) {
        while(length>=record->file_size)extend_buffered_page_table(record);
    } else {
        count=record->file_size>>12;
        for(page=length>>12;page<count;++page) {
            buffer=((FileBufferHeader **)record->view)[page];
            if(buffer && buffer!=(FileBufferHeader *)1)release_file_buffer(buffer);
        }
    }
    record->mapping_size=length;
    record->size=length;
    return _chsize(record->descriptor,length);
}

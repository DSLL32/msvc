/* Image checksum entry point shared by linking and editing. */
#include <stdio.h>
#include <io.h>
#undef _fileno
#include "pass-two.h"
#include "editor-image.h"
#include <recon.h>
unsigned long checksum_words(unsigned long,const unsigned short *,unsigned long);

/* Original [00025c60,00025cd0). This operation requires the mapped file backend. */

void update_checksum(int file)
{
    unsigned long size=file_size(file),previous;
    void *view=mapped_output_pointer(file,0,size);
    if (view) checksum_mapped_image(view,size,&previous,NULL);
    else warning_message(program_name,0x81,"","");
}

/* Original4ebd0..4ec70. Remove the checksum field before adding file length. */
PIMAGE_NT_HEADERS checksum_mapped_image(void *image,unsigned long size,
    unsigned long *previous,unsigned long *checksum)
{
    unsigned short sum,word;
    PIMAGE_NT_HEADERS headers;
    if(previous)*previous=0;
    sum=(unsigned short)checksum_words(0,(const unsigned short *)image,(size+1)>>1);
    headers=mapped_image_headers(image);
    if(headers) {
        if(previous)*previous=headers->OptionalHeader.CheckSum;
        word=(unsigned short)headers->OptionalHeader.CheckSum;
        sum=(unsigned short)(sum-word-(sum<word));
        word=(unsigned short)(headers->OptionalHeader.CheckSum>>16);
        sum=(unsigned short)(sum-word-(sum<word));
    }
    if(checksum)*checksum=sum+size;
    else headers->OptionalHeader.CheckSum=sum+size;
    return headers;
}

/* Original50840..509d0. The assembly unrolls this end-around-carry sum. */
unsigned long checksum_words(unsigned long sum,const unsigned short *words,
    unsigned long count)
{
    const unsigned char *cursor=(const unsigned char *)words;
    unsigned long bytes=count<<1,value,next;
    if(bytes && ((unsigned long)cursor&2)) {
        value=*(const unsigned short *)cursor;
        next=sum+value;
        sum=next+(next<value);
        cursor+=2;
        bytes-=2;
    }
    while(bytes>=4) {
        value=*(const unsigned long *)cursor;
        next=sum+value;
        sum=next+(next<value);
        cursor+=4;
        bytes-=4;
    }
    if(bytes) {
        value=*(const unsigned short *)cursor;
        next=sum+value;
        sum=next+(next<value);
    }
    sum=(sum&0xffff)+(sum>>16);
    sum+=sum>>16;
    return sum&0xffff;
}

/* Original [0004ec70,0004ed57). A checksum output requests read-only access;
 * omitting that output requests the existing mapped helper's file writeback. */
int checksum_image_file(const char *filename,unsigned long *previous,
    unsigned long *checksum)
{
    FILE *file;
    HANDLE mapping;
    void *view;
    unsigned long size;
    file=fopen(filename,checksum?"rb":"r+b");
    if (!file) return 1;
    mapping=CreateFileMappingA((HANDLE)_get_osfhandle(_fileno(file)),NULL,
        checksum?PAGE_READONLY:PAGE_READWRITE,0,0,NULL);
    if (!mapping) {
        fclose(file);
        return 2;
    }
    view=MapViewOfFile(mapping,checksum?FILE_MAP_READ:FILE_MAP_WRITE,0,0,0);
    CloseHandle(mapping);
    if (!view) {
        fclose(file);
        return 3;
    }
    size=_filelength(_fileno(file));
    checksum_mapped_image(view,size,previous,checksum);
    UnmapViewOfFile(view);
    fclose(file);
    return 0;
}
